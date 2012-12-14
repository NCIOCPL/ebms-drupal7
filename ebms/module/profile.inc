<?php

// $Id$

/**
 * @file
 *
 * Implementation of EBMS Profile pages.
 */

/**
 * Entry point used by the Drupal menu router for all of the EBMS profile
 * pages.
 */
function pdq_ebms_profile($action=null, $user_id=0) {
    $profile = new EbmsProfile($action, $user_id);
    return $profile->run();
}

/**
 * Wrapper for logic to handle profile view/edit pages.  Keeps our
 * functions out of the global namespace with less ugly syntax than
 * the PHP namespace mechanism provides, and lets us pass around
 * the current state information more easily.
 */
class EbmsProfile {

    public
        $user,
        $action;
    public static $url = 'profile';

    /**
     * Collect information we'll need for all the profile pages.  In
     * order to support the ability of managers to edit profile
     * information of other users, we load the account object for
     * the user identified in the request rather than relying on
     * the global $user object (which doesn't have EBMS-specific
     * information anyway).
     */
    public function __construct($action, $user_id) {
        $this->user    = user_load($user_id);
        $this->action  = $action;
    }

    /**
     * Assemble the render array for the profile page.
     */
    public function run() {
        if (!$this->allowed()) {
            drupal_set_message(
                'Sorry, you are not authorized to view that page.',
                'error'
            );
            drupal_goto('home');
        }
        switch ($this->action) {
            case 'all':
            case 'active':
                return $this->show_all();
                break;
            case 'memberships':
                $u = $this->user->uid;
                return drupal_get_form('pdq_embs_profile_membership_form', $u);
            case 'view':
            case 'crop':
            case 'edit-email':
            case 'edit-password':
            case 'edit-picture':
                return $this->show_profile();
                break;
        }
    }

    /**
     * Determine whether the currently logged-on user is permitted
     * to perform the action requested.
     */
    private function allowed() {
        global $user;
        switch ($this->action) {
            case 'all':
            case 'memberships':
                return user_access('administer');
            default:
                if ($user->uid == $this->user->uid)
                    return true;
                if (user_access('administer'))
                    return true;
                return false;
        }
    }

    /**
     * Generate a table displaying profile information for all of the
     * user account (excluding the super-user admin account).
     */
    private function show_all() {

        // Override the default Drupal breadcrumbs to match requirements.
        drupal_set_breadcrumb(
            array(
                l('Home', 'home'),
                'User Profiles',
            )
        );

        // Pull the information from the database tables.
        $query = db_select('users', 'u');
        $query->leftJoin('ebms_user', 'e', 'e.user_id = u.uid');
        $query->fields('u', array('uid', 'name', 'mail', 'picture'));
        $query->fields('e', array('password_changed'));
        $query->condition('u.uid', 1, '>');
        if ($this->action == 'active')
            $query->condition('u.status', 1);
        $query->orderBy('u.name');
        $users = $query->execute();

        // Set the first row as column headers.
        $rows = array(
            array(
                'data' => array('Name', 'Email', 'PW Mod', 'Boards', 'Pic'),
                'class' => array('user-accounts-header'),
                'no_striping' => 1,
            ),
        );

        // Add a row for each of the users.
        foreach ($users as $u) {
            if ($u->picture) {
                $uri = Ebms\Util::get_file_uri($u->picture);
                $photo = array(
                    '#type' => 'container',
                    '#attributes' => array('class' => 'profile-picture'),
                    'picture' => array(
                        '#theme' => 'image',
                        '#path' => $uri,
                        '#width' => 27,
                        '#height' => 27,
                        '#title' => htmlspecialchars($u->name),
                    ),
                    'frame' => array(
                        '#theme' => 'image',
                        '#path' => Ebms\IMAGES_DIR . "/picframe-45x45.png",
                        '#width' => 27,
                        '#height' => 27,
                        '#title' => htmlspecialchars($u->name),
                    ),
                );
            }
            else
                $photo = '';
            $email = htmlspecialchars($u->mail);

            // XXX Temporary code; remove for production.
            if (strpos($email, 'bkline+ebmsdev') !== false) {
                $email = '<span title="For right now we\'re using ' .
                    htmlspecialchars($u->mail) . ' to prevent ' .
                    htmlspecialchars($u->name) .
                    ' from getting test messages while we\'re ' .
                    'developing.">Bogus address</span>';
            }
            $link = l($u->name, EbmsProfile::$url . '/view/' . $u->uid);
            if ($u->password_changed)
                $pw_changed = date('m/d/y', strtotime($u->password_changed));
            else
                $pw_changed = '&mdash;';
            $boards = EbmsProfile::get_boards($u->uid, '<br />');
            $boards_url = EbmsProfile::$url . '/memberships/' . $u->uid;
            $boards_link = l($boards, $boards_url, array('html' => true));
            $rows[] = array(
                'data' => array(
                    array(
                        'data' => $link,
                        'class' => array('c1'),
                    ),
                    array(
                        'data' => $email,
                        'class' => array('c1'),
                    ),
                    array(
                        'data' => htmlspecialchars($u->mail),
                        'data' => $pw_changed,
                        'class' => array('c3'),
                    ),
                    array(
                        'data' => $boards_link,
                        'class' => array('c4'),
                    ),
                    array(
                        'data' => $photo,
                        'class' => array('c5'),
                    ),
                ),
                'no_striping' => true,
            );
        }

        // Assemble and return the completed table structure.
        return array(
            '#prefix' => '<h2>All User Profiles</h2>',
            '#attributes' => array('id' => 'user-accounts'),
            '#theme' => 'table',
            '#rows' => $rows,
        );
    }

    /**
     * Display the profile information for the specified user account.
     * Each row is populated with a read-only version of the information,
     * or an editable version, depending on the action requested.
     */
    private function show_profile() {
        drupal_set_breadcrumb(
            array(
                l('Home', 'home'),
                'Account Settings',
            )
        );
        $rows = array(
            $this->user_name_row(),
            $this->email_row(),
            $this->password_row(),
            $this->boards_row(),
            $this->picture_row(),
        );
        $page = array(
            'title' => array(
                '#type' => 'markup',
                '#markup' => "<h2 class='tight-below'>Account Settings</h2>",
            ),
            'table' => array(
                '#attributes' => array('id' => 'user-account-settings'),
                '#theme' => 'table',
                '#rows' => $rows,
            ),
        );
        if ($this->action == 'crop') {
            Ebms\Util::add_js();
            $form_id = 'pdq_ebms_profile_picture_crop_form';
            $file_id = Ebms\Util::get_request_value('fid');
            $form = drupal_get_form($form_id,  $this->user, $file_id);
            $page['cropbox'] = array(
                '#type' => 'container',
                '#attributes' => array('id' => 'cropbox'),
                'form' => $form,
            );
        }
        return $page;
    }

    /**
     * Assemble the row for displaying the user's name.  This field
     * is not editable.
     */
    private function user_name_row() {
        return array(
            'data' => array(
                array(
                    'data' => 'Name',
                    'class' => array('c1'),
                ),
                array(
                    'data' => htmlspecialchars($this->user->name),
                    'class' => array('c23'),
                    'colspan' => 2,
                ),
            ),
            'no_striping' => true,
        );
    }

    /**
     * Assemble the row for displaying or editing the user's email
     * address.
     */
    private function email_row() {
        if ($this->action == 'edit-email') {
            $form = drupal_get_form('pdq_ebms_profile_email_form',
                    $this->user);
            $data = array(
                array(
                    'data' => $form,
                    'class' => array('edit-cell'),
                    'colspan' => 3,
                ),
            );
        }
        else {
            $data = array(
                array(
                    'data' => 'Email',
                    'class' => array('c1'),
                ),
                array(
                    'data' => htmlspecialchars($this->user->mail),
                    'class' => array('c2'),
                ),
                array(
                    'data' => $this->make_edit_link('email'),
                    'class' => array('c3'),
                ),
            );
        }
        return array('data' => $data, 'no_striping' => true);
    }

    /**
     * Assemble the row for displaying the date the user's password
     * was last changed or for setting a new password for the user.
     */
    private function password_row() {
        if ($this->action == 'edit-password') {
            $form = drupal_get_form('pdq_ebms_profile_password_form',
                    $this->user);
            $data = array(
                array(
                    'data' => $form,
                    'class' => array('edit-cell'),
                    'colspan' => 3,
                ),
            );
        }
        else {
            if ($this->user->password_changed)
                $pwc = 'Password last updated on ' .
                    date('m/d/y', strtotime($this->user->password_changed));
            else
                $pwc = 'Password not yet updated';
            $data = array(
                array(
                    'data' => 'Password',
                    'class' => array('c1'),
                ),
                array(
                    'data' => $pwc,
                    'class' => array('c2'),
                ),
                array(
                    'data' => $this->make_edit_link('password'),
                    'class' => array('c3'),
                ),
            );
        }
        return array('data' => $data, 'no_striping' => true);
    }

    /**
     * Assemble the row for displaying or editing the user's profile
     * picture.  A general avatar is displayed for accounts which
     * do not yet have a profile picture uploaded.
     */
    private function picture_row() {
        if ($this->action == 'edit-picture') {
            $form = drupal_get_form('pdq_ebms_profile_picture_form',
                    $this->user);
            $data = array(
                array(
                    'data' => $form,
                    'class' => array('edit-picture-cell'),
                    'colspan' => 3,
                ),
            );
        }
        else {
            $data = array(
                array(
                    'data' => 'Profile Picture',
                    'class' => array('c1'),
                ),
                array(
                    'data' => EbmsProfile::get_picture($this->user),
                    'class' => array('c2'),
                ),
                array(
                    'data' => $this->make_edit_link('picture'),
                    'class' => array('c3'),
                ),
            );
        }
        return array(
            'data' => $data,
            'no_striping' => true,
            'class' => array('picture-row'),
        );
    }

    /**
     * Generate the renderable structure for the markup to display
     * the user's profile picture in a frame with rounded corners.
     * An avatar is used if the user's account does not yet have
     * a profile picture uploaded.
     */
    public static function get_picture($user) {
        if ($user->picture) {
            $uri = $user->picture->uri;
            $title = htmlspecialchars($user->name);
        }
        else {
            $uri = Ebms\IMAGES_DIR . '/avatar-135x135-stroked-1px.png';
            $title = 'No profile picture selected yet';
        }
        $picture = array(
            '#type' => 'container',
            '#attributes' => array('id' => 'profile-picture-box'),
            'picture' => array(
                '#theme' => 'image',
                '#path' => $uri,
                '#width' => Ebms\PICTURE_SIZE,
                '#height' => Ebms\PICTURE_SIZE,
                '#title' => $title,
            ),
        );
        if ($user->picture)
            $picture['frame'] = array(
                '#theme' => 'image',
                '#path' => Ebms\IMAGES_DIR . "/picframe-135x135.png",
                '#width' => Ebms\PICTURE_SIZE,
                '#height' => Ebms\PICTURE_SIZE,
                '#title' => $title,
            );
        return $picture;
        return $variables;
        return theme('image', $variables);
    }

    /**
     * Generate the render array to display the boards of which the specified
     * user is a member.
     */
    private function boards_row() {
        return array(
            'data' => array(
                array(
                    'data' => 'Boards',
                    'class' => array('c1'),
                ),
                array(
                    'data' => EbmsProfile::get_boards($this->user->uid),
                    'class' => array('c23'),
                    'colspan' => 2,
                ),
            ),
            'no_striping' => true,
        );
    }

    /**
     * Construct a string showing the boards of which the specified
     * user is a member.  If the user is a "member" of all boards
     * (as will be true for Bonnie) the string "All" is returned
     * instead.  If the user is not a member of any boards, the
     * string "None" is returned.
     */
    private static function get_boards($user_id, $glue = '; ') {
        $query = db_select('ebms_board', 'b');
        $query->fields('b', array('board_name'));
        $query->join('ebms_board_member', 'm', 'm.board_id = b.board_id');
        $query->condition('m.user_id', $user_id);
        $query->orderBy('b.board_name');
        $results = $query->execute();
        $boards = array();
        foreach ($results as $result)
            $boards[] = htmlspecialchars($result->board_name);
        if (count($boards) < 1)
            return 'None';
        $nboards = db_select('ebms_board', 'b')
            ->countQuery()
            ->execute()
            ->fetchField();
        if (count($boards) == $nboards)
            return 'All';
        return implode($glue, $boards);
    }

    /**
     * Helper method to create the link for editing one of the
     * profile fields.
     */
    private function make_edit_link($field) {
        $url = EbmsProfile::$url;
        return l('EDIT', "$url/edit-$field/{$this->user->uid}");
    }

    /**
     * If the user has disabled Javascript we crop and scale the
     * uploaded picture image to a square shape of the required
     * dimensions without any input from the user.  Cropping is
     * done by centering the portion of the image retained.
     */
    public static function scale_and_crop($image) {
        $width = $image->info['width'];
        $height = $image->info['height'];
        if ($width > $height) {
            $dim = $height;
            $extra = $width - $dim;
            $x = $extra / 2;
            image_crop($image, $x, 0, $dim, $dim);
        }
        elseif ($width < $height) {
            $dim = $width;
            $extra = $height - $dim;
            $y = $extra / 2;
            image_crop($image, 0, $y, $dim, $dim);
        }
        if ($dim > Ebms\PICTURE_SIZE)
            image_resize($image, Ebms\PICTURE_SIZE, Ebms\PICTURE_SIZE);
        image_save($image, $image->source);
    }
}

/**
 * Builds the renderable array for the form used to edit the
 * user's email address.
 */
function pdq_ebms_profile_email_form($form, &$form_state, $user) {
    return array(
        #'#theme' => 'pdq_ebms_profile_email_form',
        '#attributes' => array('id' => 'profile-form'),
        'uid' => array(
            '#type' => 'hidden',
            '#value' => $user->uid,
        ),
        'email' => array(
            '#type' => 'textfield',
            '#title' => 'Email',
            '#default_value' => $user->mail,
        ),
        'buttons' => array(
            '#prefix' => '<div id="buttons">',
            '#suffix' => '</div>',
            'submit' => array(
                '#type' => 'submit',
                '#value' => 'Save Changes',
            ),
            'cancel' => array(
                '#type' => 'markup',
                '#markup' => l(
                    'Cancel', EbmsProfile::$url . '/view/ ' . $user->uid,
                    array(
                        'attributes' => array(
                            'id' => 'cancel-link',
                        ),
                    )
                ),
            ),
        ),
    );
}

/**
 * Verify that the new email address is valid.  Invoked automatically
 * by the Drupal Form API based on naming conventions.
 */
function pdq_ebms_profile_email_form_validate($form, &$form_state) {
    $address = $form_state['values']['email'];
    $address = trim($address);
    $user_id = $form_state['values']['uid'];
    if (!valid_email_address($address))
        form_set_error('email', 'That email address is not valid.');
    $count = db_select('users', 'u')
        ->condition('u.mail', $address)
        ->condition('u.uid', $user_id, '<>')
        ->countQuery()
        ->execute()
        ->fetchField();
    if ($count)
        form_set_error('email',
            'That email address is associated with another user.');
}

/**
 * Store the user's new email address.
 */
function pdq_ebms_profile_email_form_submit($form, &$form_state) {
    $address = $form_state['values']['email'];
    $address = trim($address);
    $user_id = $form_state['values']['uid'];
    $user = user_load($user_id);
    user_save($user, array('mail' => $address));
    drupal_goto(EbmsProfile::$url . '/view/' . $user->uid);
}

/**
 * Constructs the renderable array for the password change form.
 * The user must provide the correct current password, and type
 * identical copies of the new password twice.
 */
function pdq_ebms_profile_password_form($form, &$form_state, $user) {
    return array(
        '#attributes' => array('id' => 'profile-form'),
        'uid' => array(
            '#type' => 'hidden',
            '#value' => $user->uid,
        ),
        'label' => array(
            '#type' => 'markup',
            '#markup' => '<div id="form-name">Password</div>',
        ),
        'fields' => array(
            '#type' => 'container',
            '#attributes' => array('id' => 'password-fields'),
            'current' => array(
                '#type' => 'password',
                '#title' => 'Current',
            ),
            'new' => array(
                '#type' => 'password',
                '#title' => 'New',
            ),
            'new2' => array(
                '#type' => 'password',
                '#title' => 'Re-type New',
            ),
        ),
        'match-check' => array(
            '#type' => 'markup',
            '#markup' => '<div id="match-check">&nbsp;</div>',
        ),
        'buttons' => array(
            '#prefix' => '<div id="buttons" class="password-buttons">',
            '#suffix' => '</div>',
            'submit' => array(
                '#type' => 'submit',
                '#value' => 'Save Changes',
            ),
            'cancel' => array(
                '#type' => 'markup',
                '#markup' => l(
                    'Cancel', EbmsProfile::$url . '/view/' . $user->uid,
                    array(
                        'attributes' => array(
                            'id' => 'cancel-link',
                        ),
                    )
                ),
            ),
        ),
    );
}

/**
 * Verify that the user has entered the correct current password, and
 * that the new password has been entered twice as the same value.
 * We do not currently have any requirements for enforcing strong
 * passwords.
 */
function pdq_ebms_profile_password_form_validate($form, &$form_state) {
    $user_id = $form_state['values']['uid'];
    $account = $form_state['account'] = user_load($user_id);
    $current = trim($form_state['values']['current']);
    $new = trim($form_state['values']['new']);
    $new2 = trim($form_state['values']['new2']);
    require_once DRUPAL_ROOT . '/' .
        variable_get('password_inc', 'includes/password.inc');
    if (!user_check_password($current, $account))
        form_set_error('current', 'Current password is not valid.');
    if (!$new)
        form_set_error('new', 'No new password specified.');
    elseif ($new != $new2)
        form_set_error('new2', 'Passwords do not match.');
}

/**
 * Change the user's password.  This has a side effect of replacing
 * the current session with a new one and destroying any other sessions
 * the user has open in other browsers.
 */
function pdq_ebms_profile_password_form_submit($form, &$form_state) {
    $current = trim($form_state['values']['current']);
    $new = trim($form_state['values']['new']);
    if ($new != $current) {
        $account = $form_state['account'];
        user_save($account, array('pass' => $new));
        $now = date('Y-m-d H:i:s');
        db_update('ebms_user')
            ->fields(array('password_changed' => $now))
            ->condition('user_id', $account->uid)
            ->execute();
    }
    drupal_goto(EbmsProfile::$url . '/view/' . $account->uid);
}

/**
 * Assembles the renderable array for the form used to upload
 * a new profile picture for the user's account.
 */
function pdq_ebms_profile_picture_form($form, &$form_state, $user) {

    return array(
        '#attributes' => array('id' => 'profile-form'),
        'uid' => array(
            '#type' => 'hidden',
            '#value' => $user->uid,
        ),
        'js' => array(
            '#attributes' => array('id' => 'js'),
            '#type' => 'hidden',
            '#default_value' => 0,
        ),
        'label' => array(
            '#type' => 'markup',
            '#markup' => '<div id="picture-form-name">Profile Picture</div>',
        ),
        'original' => EbmsProfile::get_picture($user),
        'fields' => array(
            '#type' => 'container',
            '#attributes' => array('id' => 'picture-fields'),
            'filepath' => array(
                '#id' => 'filepath',
                '#type' => 'file',
                '#title' => 'CHOOSE PHOTO',
                '#prefix' => '<div id="choose-file">',
                '#suffix' => '<div id="filename"></div></div>',
                '#element_validate' => array('pdq_ebms_doc_file_required'),
            ),
            'instructions' => array(
                '#type' => 'markup',
                '#markup' => '<p id="instructions">You can upload a ' .
                'JPG, GIF or PNG file (File size limit is 4MB).' .
                '<noscript><br /><span>Photo will be cropped to ' .
                'square proportions.</span></noscript></p>',
            ),
            'submit-box' => array(
                '#type' => 'container',
                '#attributes' => array('id' => 'submit-box'),
                'submit' => array(
                    '#type' => 'submit',
                    '#value' => 'UPLOAD PHOTO',
                ),
                'agreement' => array(
                    '#type' => 'markup',
                    '#markup' => '<p id="agreement">By clicking ' .
                    '"Upload Photo," you certify that you have ' .
                    'the right to distribute this photo and that it does ' .
                    'not violate the User Agreement.</p>',
                ),
            ),
        ),
    );
}

/**
 * Verifieds that the uploaded picture is a valid image of one of the
 * allowable types, that the file size does not exceed 4MB (though
 * we'll be scaling the image significantly if it's that large, so
 * it doesn't make a lot of difference), and that it is at least
 * the minimum dimensions.  As a side effect, the $image and $file
 * objects are stored in the $form_state structure under the 'values'
 * member of the array.
 */
function pdq_ebms_profile_picture_form_validate($form, &$form_state) {
    $validators = array(
        'file_validate_extensions' => array(),
        'file_validate_is_image' => array(),
        'file_validate_size' => array(4 * 1024 * 1024),
    );
    $file = file_save_upload('filepath', $validators);
    if (!$file)
        form_set_error('filepath', 'Failure uploading image.');
    else {
        $image = image_load($file->uri);
        $width = $image->info['width'];
        $height = $image->info['height'];
        if ($width < Ebms\PICTURE_SIZE || $height < Ebms\PICTURE_SIZE)
            form_set_error('filepath',
                'Picture must be at least ' . Ebms\PICTURE_SIZE .
                ' pixels square.');
        else {
            $form_state['values']['image'] = $image;
            $form_state['values']['file'] = $file;
        }
    }
}

/**
 * Validation function to ensure that a document was selected for a file
 * upload field (Drupal's validation of required file upload fields is
 * broken).
 */
function pdq_ebms_doc_file_required($element, &$form_state) {
    if ($_FILES['files']['name']['filepath'] == '') {
        form_set_error('filepath', t('Please select a document'));
    }
}

/**
 * Stores the user's new profile picture.  We first check to see whether
 * the image already has the expected dimensions.  If not, and the user
 * has Javascript enabled, we put up another form to let the user
 * decide how the image should be cropped, deferring final storage of the
 * image until that process has completed.  If the image is not already
 * at the required dimensions and Javascript is disabled, we do the
 * cropping for the user without her input.
 */
function pdq_ebms_profile_picture_form_submit($form, &$form_state) {
    $image = $form_state['values']['image'];
    $file = $form_state['values']['file'];
    $user_id = $form_state['values']['uid'];
    $account = $form_state['account'] = user_load($user_id);
    $js = $form_state['values']['js'];
    $width = $image->info['width'];
    $height = $image->info['height'];
    $ready = true;
    if ($width != Ebms\PICTURE_SIZE || $height != Ebms\PICTURE_SIZE) {
        if ($js) {
            $options = array('query' => array('fid' => $file->fid));
            drupal_goto(EbmsProfile::$url . "/crop/$user_id", $options);
            $ready = false;
        }
        else
            EbmsProfile::scale_and_crop($image);
    }
    if ($ready) {
        user_save($account, array('picture' => $file));
        drupal_goto(EbmsProfile::$url . '/view/' . $account->uid);
    }
}

/**
 * Assemble the renderable array for the form used to specify how
 * the uploaded profile picture should be trimmed to create a square
 * image.  Uses the jquery plugin Jcrop:
 *           http://deepliquid.com/content/Jcrop.html
 */
function pdq_ebms_profile_picture_crop_form($form, &$form_state, $user, $fid) {
    $file = file_load($fid);
    $url = file_create_url($file->uri);
    $info = image_get_info($file->uri);
    $width = $info['width'];
    $height = $info['height'];
    $jclib = libraries_get_path('jcrop');
    return array(
        '#id' => 'picture-crop-form',
        'width' => array('#type' => 'hidden', '#value' => $width),
        'height' => array('#type' => 'hidden', '#value' => $height),
        'x' => array('#type' => 'hidden'),
        'y' => array('#type' => 'hidden'),
        'w' => array('#type' => 'hidden'),
        'h' => array('#type' => 'hidden'),
        'fid' => array('#type' => 'hidden', '#value' => $fid),
        'user-id' => array('#type' => 'hidden', '#value' => $user->uid),
        'image' => array(
            '#type' => 'markup',
            '#markup' => "<img src='$url' id='image' />",
        ),
        'submit' => array(
            '#type' => 'submit',
            '#value' => 'Save',
        ),
        '#attached' => array(
            'css' => array("$jclib/css/jquery.Jcrop.min.css"),
            'js' => array("$jclib/js/jquery.Jcrop.min.js"),
        ),
    );
}

/**
 * Uses the coordinates and dimensions specified by the user through
 * the visual Jcrop plugin to trim and scale the profile picture image
 * to the required shape and size.
 */
function pdq_ebms_profile_picture_crop_form_submit($form, &$form_state) {
    $x = $form_state['values']['x'];
    $y = $form_state['values']['y'];
    $w = $form_state['values']['w'];
    $h = $form_state['values']['h'];
    $original_w = $form_state['values']['width'];
    $original_h = $form_state['values']['height'];
    $user_id = $form_state['values']['user-id'];
    if ($w >= Ebms\PICTURE_SIZE && $w == $h) {
        $account = user_load($user_id);
        $file_id = $form_state['values']['fid'];
        $file = file_load($file_id);
        $image = image_load($file->uri);
        $modified = false;
        if ($x || $y || $w < $original_w || $h < $original_h) {
            image_crop($image, $x, $y, $w, $h);
            $modified = true;
        }
        if ($w > Ebms\PICTURE_SIZE) {
            image_resize($image, Ebms\PICTURE_SIZE, Ebms\PICTURE_SIZE);
            $modified = true;
        }
        if ($modified)
            image_save($image, $image->source);
        user_save($account, array('picture' => $file));
    }
    drupal_goto(EbmsProfile::$url . '/view/' . $user_id);
}

/**
 * Didn't think we'd need this for the first release, but we no longer
 * have direct access to the database on the production or pre-production
 * tiers, so we have to have it after all.  This is the form where
 * board and subgroup membership is assigned for a given member.
 */
function pdq_embs_profile_membership_form($form, &$form_state, $uid) {
    $name = db_select('users', 'u')
        ->condition('u.uid', $uid)
        ->fields('u', array('name'))
        ->execute()
        ->fetchField();
    $name = htmlspecialchars($name);
    $results = db_select('ebms_board_member', 'm')
        ->condition('m.user_id', $uid)
        ->fields('m', array('board_id'))
        ->execute();
    $current_boards = array();
    foreach ($results as $result)
        $current_boards[] = $result->board_id;
    $boards = Ebms\Util::boards();
    $results = db_select('ebms_subgroup_member', 'm')
        ->condition('m.user_id', $uid)
        ->fields('m', array('sg_id'))
        ->execute();
    $current_subgroups = array();
    foreach ($results as $result)
        $current_subgroups[] = $result->sg_id;
    $results = db_select('ebms_subgroup', 's')
        ->fields('s', array('sg_id', 'sg_name'))
        ->orderBy('s.sg_name')
        ->execute();
    $subgroups = array();
    foreach ($results as $result)
        $subgroups[$result->sg_id] = $result->sg_name;
    return array(
        'title' => array(
            '#type' => 'markup',
            '#markup' => "<h2>$name</h2>",
        ),
        'uid' => array(
            '#type' => 'hidden',
            '#value' => $uid,
        ),
        'boards' => array(
            '#type' => 'checkboxes',
            '#title' => 'Boards',
            '#description' => 'Of which PDQ board(s) is this user a member?',
            '#options' => $boards,
            '#default_value' => $current_boards,
        ),
        'subgroups' => array(
            '#type' => 'checkboxes',
            '#title' => 'Subgroups',
            '#description' => 'Of which sub groups is this user a member?',
            '#options' => $subgroups,
            '#default_value' => $current_subgroups,
        ),
        'submit' => array(
            '#type' => 'submit',
            '#value' => 'save',
        ),
    );
}

/**
 * Clear out the rows for this user's board and subgroup memberships and
 * repopulate based on the submitted form's values.
 */
function pdq_embs_profile_membership_form_submit($form, &$form_state) {
    $values = $form_state['values'];
    $uid = $values['uid'];
    $boards = $values['boards'];
    $subgroups = $values['subgroups'];
    db_delete('ebms_board_member')
        ->condition('user_id', $uid)
        ->execute();
    db_delete('ebms_subgroup_member')
        ->condition('user_id', $uid)
        ->execute();
    foreach ($boards as $board) {
        if ($board)
            db_insert('ebms_board_member')
                ->fields(
                    array(
                        'user_id' => $uid,
                        'board_id' => $board,
                    )
                )
                ->execute();
    }
    foreach ($subgroups as $subgroup) {
        if ($subgroup)
            db_insert('ebms_subgroup_member')
                ->fields(
                    array(
                        'user_id' => $uid,
                        'sg_id' => $subgroup,
                    )
                )
                ->execute();
    }
}
