<?php

// $Id:$

/**
 * @file
 *
 * Interface for managing EBMS subgroups.  Added as part of work on TIR #2486.
 */
function pdq_ebms_subgroups($action='view', $subgroup_id=0) {
    drupal_add_css(Ebms\CSS_DIR . '/subgroups.css');
    switch ($action) {
        case 'edit':
            return drupal_get_form('pdq_ebms_subgroup_form', $subgroup_id);
        case 'add':
            return drupal_get_form('pdq_ebms_subgroup_form');
        case 'delete':
            return pdq_ebms_delete_subgroup($subgroup_id);
        case 'view':
        default:
            return pdq_ebms_show_subgroups();
    }
}

/**
 * Drop a subgroup, bug only if it has no members.
 */
function pdq_ebms_delete_subgroup($subgroup_id) {
    $query = db_select('ebms_subgroup_member', 'm')
        ->condition('m.sg_id', $subgroup_id);
    $query->addExpression('COUNT(*)', 'n');
    $count = $query->execute()->fetchField();
    if ($count)
        drupal_set_message('Only subgroups with no members can be removed.',
            'error');
    else {
        db_delete('ebms_subgroup')
            ->condition('sg_id', $subgroup_id)
            ->execute();
        drupal_set_message('Subgroup deleted.');
    }
    drupal_goto('admin/subgroups');
}

/**
 * Build a table of all of the subgroups in the EBMS, with links for editing
 * the existing subgroups or creating a new one.
 *
 *  @return   array         Drupal render array for table of subgroups
 */
function pdq_ebms_show_subgroups() {
    $query = db_select('ebms_subgroup', 's');
    $query->join('ebms_board', 'b', 'b.board_id = s.board_id');
    $query->fields('s', array('sg_name', 'sg_id'));
    $query->fields('b', array('board_name'));
    $query->orderBy('b.board_name');
    $query->orderBy('s.sg_name');
    $results = $query->execute();
    $rows = array();
    foreach ($results as $result) {
        $row = array(
            htmlspecialchars($result->board_name),
            htmlspecialchars($result->sg_name),
            l('Edit', 'admin/subgroups/edit/' . $result->sg_id) .
            ' &nbsp; ' .
            l('Delete', 'admin/subgroups/delete/' . $result->sg_id)
        );
        $rows[] = $row;
    }
    $headers = array(
        'Board',
        'Subgroup',
        'Actions',
    );
    $button_class = array('class' => array('button'));
    $attrs = array('attributes' => $button_class);
    return array(
        'title' => array(
            '#markup' => '<h2 id="subgroups-title">EBMS Subgroups</h2>',
        ),
        'new-link' => array(
            '#markup' => l('Add New Subgroup', 'admin/subgroups/add', $attrs) .
            '<br /><br />',
        ),
        'table' => array(
            '#theme' => 'table',
            '#rows' => $rows,
            '#header' => $headers,
        ),
    );
}

/**
 * Drupal form API callback which generates the form for editing an
 * existing EBMS subgroup or creating a new one.
 *
 *  @param  array   $form         Structured array containing the
 *                                elements and properties of the form
 *  @param  array   &$form_state  Modifiable structured array
 *                                containing the current values
 *                                of the form, as well as other
 *                                form state information
 *  @param  int     $subgroup_id  Primary key for the subgroup being
 *                                edited, if we are not creating a
 *                                new subgroup
 *
 *  @return array                 Array containing specification
 *                                of the fields to appear on the
 *                                form, as well as other markup
 */
function pdq_ebms_subgroup_form($form, &$form_state, $subgroup_id=0) {

    // Override Drupal's default breadcrumb mechanism.
    drupal_set_breadcrumb(
        array(
            l('Home', 'home'),
            l('Subgroups', 'admin/subgroups'),
        )
    );

    // Initialize local variables to their defaults.
    $name = '';
    $board_id = 0;
    $selected_members = array();
    $boards = array(0 => ' ') + Ebms\Util::boards();
    $subgroup_members_wrapper = array(
        '#prefix' => '<div id="subgroup-members-wrapper">',
        '#suffix' => '</div>',
    );

    // Catch any changes to the form from the AJAX trigger.
    if (isset($form_state['values'])) {
        $values = $form_state['values'];
        $name = $values['name'];
        $board_id = $values['board'];
        foreach ($values['subgroup-members'] as $key => $value) {
            if ($value)
                $selected_members[] = $key;
        }
    }

    // If we're editing an existing subgroup, collect the current values.
    elseif ($subgroup_id) {
        $query = db_select('ebms_subgroup', 's');
        $query->fields('s');
        $query->condition('s.sg_id', $subgroup_id);
        $subgroup = $query->execute()->fetch();
        $board_id = $subgroup->board_id;
        $name = $subgroup->sg_name;
        $query = db_select('ebms_subgroup_member', 'm');
        $query->fields('m', array('user_id'));
        $query->distinct();
        $query->condition('m.sg_id', $subgroup_id);
        foreach ($query->execute() as $result)
            $selected_members[] = $result->user_id;
    }

    // We need to pass this to the AJAX callback because of a Drupal FAPI bug.
    $form_state['ebms-checked-members'] = $selected_members;

    // If a board has been selected, add the subgroup member field.
    if ($board_id) {
        $query = db_select('users', 'u');
        $query->fields('u', array('name', 'uid'));
        $query->join('ebms_board_member', 'm', 'm.user_id = u.uid');
        $query->join('users_roles', 'ur', 'u.uid = ur.uid');
        $query->join('role', 'r', 'r.rid = ur.rid');
        $query->join('ebms_board_member', 'm', 'm.user_id = u.uid');
        $query->condition('u.status', 1);
        $query->condition('m.board_id', $board_id);
        $query->condition('r.name', 'board member');
        $query->orderBy('u.name');
        $board_members = array();
        foreach ($query->execute() as $u)
            $board_members[$u->uid] = $u->name;
        $subgroup_members_wrapper['subgroup-members'] = array(
            '#title' => 'Subgroup Members',
            '#type' => 'checkboxes',
            '#options' => $board_members,
            '#default_value' => $selected_members,
        );
    }

    // Assemble and return the form render array.
    $title = $subgroup_id ? 'Edit Subgroup' : 'Add Subgroup';
    return array(
        'title' => array(
            '#markup' => "<h2>$title</h2>",
        ),
        'name' => array(
            '#type' => 'textfield',
            '#title' => 'Subgroup Name',
            '#required' => true,
            '#default_value' => $name,
            '#maxlength' => 255,
        ),
        'subgroup' => array(
            '#type' => 'hidden',
            '#value' => $subgroup_id,
        ),
        'board' => array(
            '#type' => 'select',
            '#title' => t('Board'),
            '#required' => true,
            '#default_value' => $board_id,
            '#options' => $boards,
            '#ajax' => array(
                'callback' => 'pdq_ebms_subgroup_ajax',
                'wrapper' => 'subgroup-members-wrapper',
            ),
        ),
        'subgroup-members-wrapper' => $subgroup_members_wrapper,
        'save' => array(
            '#type' => 'submit',
            '#value' => 'Save',
        ),
        'cancel' => array(
            '#type' => 'submit',
            '#value' => 'Cancel',
            '#submit' => array('pdq_ebms_subgroup_form_submit'),
            '#limit_validation_errors' => array(),
        ),
    );
}

/**
 * Return the block which is dynamically generated in response to the
 * form's board selection.  Have to manually set the checkbox defaults
 * because of a bug in the Drupal FAPI, which ignores the defaults in
 * an AJAX callback (cf. http://drupal.org/node/1100170).
 *
 *  @param  array   $form         Structured array containing the
 *                                elements and properties of the form
 *  @param  array   $form_state   Structured array containing the
 *                                current values of the form, as well
 *                                as other form state information
 *  @return array                 block to be plugged into the form
 *                                when the controlling board selection
 *                                changes
 */
function pdq_ebms_subgroup_ajax($form, $form_state) {
    $checked_memberss = $form_state['ebms-checked-members'];
    $subform = $form['subgroup-members-wrapper'];
    foreach ($subform['subgroup-members'] as $k => $v) {
        if (isset($v['#type']) and $v['#type'] == 'checkbox') {
            $checked = in_array($k, $checked_members) ? 1 : 0;
            $subform['subgroup-members'][$k]['#checked'] = $checked;
        }
    }
    return $subform;
}

/**
 * Make sure everything we need for the subgroup is present.
 *
 *  @param  array   $form         Structured array containing the
 *                                elements and properties of the form
 *  @param  array   $form_state   Structured array containing the
 *                                current values of the form, as well
 *                                as other form state information
 */
function pdq_ebms_subgroup_form_validate($form, &$form_state) {
    $values = $form_state['values'];
    if ($values['op'] != 'Save')
        return;
    if (!$values['name'])
        form_set_error('name', 'You must give the subgroup a name');
    if (!$values['board'])
        form_set_error('board', 'You must select a board for the subgroup');
}

/**
 * Store the subgroup information if the user has clicked on the 'Save' button.
 *
 *  @param  array   $form         Structured array containing the
 *                                elements and properties of the form
 *  @param  array   $form_state   Structured array containing the
 *                                current values of the form, as well
 *                                as other form state information
 */
function pdq_ebms_subgroup_form_submit($form, &$form_state) {
    $values = $form_state['values'];
    pdq_ebms_debug('SUBGROUP', $values);
    if ($values['op'] == 'Save') {
        $subgroup = $values['subgroup'];
        $name = $values['name'];
        $board = $values['board'];
        $fields = array(
            'sg_name'  => $name,
            'board_id' => $board,
        );
        if (!$subgroup)
            $subgroup = db_insert('ebms_subgroup')
                ->fields($fields)
                ->execute();
        else {
            db_update('ebms_subgroup')
                ->fields($fields)
                ->condition('sg_id', $subgroup)
                ->execute();
            db_delete('ebms_subgroup_member')
                ->condition('sg_id', $subgroup)
                ->execute();
        }
        foreach ($values['subgroup-members'] as $key => $value) {
            if ($value)
                db_insert('ebms_subgroup_member')
                    ->fields(
                        array(
                            'sg_id'   => $subgroup,
                            'user_id' => $key,
                        )
                    )
                    ->execute();
        }
        drupal_set_message('Subgroup ' . htmlspecialchars($name) . ' saved.');
    }
    drupal_goto('admin/subgroups');
}
