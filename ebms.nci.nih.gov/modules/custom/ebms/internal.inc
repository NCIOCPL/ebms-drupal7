<?php

/**
 * @file
 *
 * Interface for managing internal EBMS articles.  Added as part of work on
 * OCEEBMS-509.
 */

/**
 * Entry point for internal tag management.
 *
 *  @param  string   $action   one of 'view', 'edit', or 'add'
 *  @param  int      $tag_id   optional integer for editing an existing tag
 *
 *  @return array    Drupal render or form array
 */
function pdq_ebms_internal_tags($action='view', $tag_id=0) {
    $form_id = 'pdq_ebms_internal_tag_form';
    switch ($action) {
        case 'edit':
            return drupal_get_form($form_id, $tag_id);
        case 'add':
            return drupal_get_form($form_id);
        case 'view':
        default:
            return pdq_ebms_show_internal_tags();
    }
}

/**
 * Build a table of all of the topics in the EBMS, with links for editing
 * the existing topics or creating a new one.
 *
 *  @return   array         Drupal render array for table of tags
 */
function pdq_ebms_show_internal_tags() {
    $query = db_select('ebms_internal_tag', 't');
    $query->fields('t');
    $query->orderBy('t.tag_name');
    $results = $query->execute();
    $rows = array();
    foreach ($results as $result) {
        $row = array(
            htmlspecialchars($result->tag_name),
            $result->active_status == 'A' ? 'Active' : 'Inactive',
            l('Edit', 'admin/internal-tags/edit/' . $result->tag_id),
        );
        $rows[] = $row;
    }
    $headers = array(
        'Tag Name',
        'Status',
        'Action',
    );
    $button_class = array('class' => array('button'));
    $attrs = array('attributes' => $button_class);
    return array(
        'description' => array(
            '#markup' =>
                '<p>' .
                'These tags are used to identify articles of interest to ' .
                'internal staff (articles which are not necessarily ' .
                'intended to be added to the board member review process).' .
                '</p>',
        ),
        'new-link' => array(
            '#markup' => l('Add New Tag', 'admin/internal-tags/add', $attrs) .
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
 * existing EBMS internal tag or creating a new one.
 *
 *  @param  array   $form         Structured array containing the
 *                                elements and properties of the form
 *  @param  array   &$form_state  Modifiable structured array
 *                                containing the current values
 *                                of the form, as well as other
 *                                form state information
 *  @param  int     $tag_id       Primary key for the tag being
 *                                edited, if we are not creating a
 *                                new tag
 *
 *  @return array                 Array containing specification
 *                                of the fields to appear on the
 *                                form, as well as other markup
 */
function pdq_ebms_internal_tag_form($form, &$form_state, $tag_id=0) {

    // Override Drupal's default breadcrumb mechanism.
    drupal_set_breadcrumb(
        array(
            l('Home', 'home'),
            l('Internal Tags', 'admin/internal-tags'),
        )
    );

    // Initialize local variables to their defaults.
    $name = '';
    $status = 'A';

    // Catch any changes to the form from the AJAX trigger.
    if (isset($form_state['values'])) {
        $values = $form_state['values'];
        $name = $values['name'];
        $status = $values['status'];
    }

    // If we're editing an existing tag, collect the current values.
    elseif ($tag_id) {
        $query = db_select('ebms_internal_tag', 't');
        $query->fields('t');
        $query->condition('t.tag_id', $tag_id);
        $tag = $query->execute()->fetch();
        $name = $tag->tag_name;
        $status = $tag->active_status;
    }

    // Assemble and return the form render array.
    $title = $tag_id ? 'Edit Tag' : 'Add Tag';
    return array(
        'title' => array(
            '#markup' => "<h2>$title</h2>",
        ),
        'tag' => array(
            '#type' => 'hidden',
            '#value' => $tag_id,
        ),
        'name' => array(
            '#type' => 'textfield',
            '#title' => 'Tag Name',
            '#required' => true,
            '#default_value' => $name,
            '#maxlength' => 255,
        ),
        'status' => array(
            '#type' => 'radios',
            '#title' => 'Status',
            '#options' => array('A' => 'Active', 'I' => 'Inactive'),
            '#default_value' => $status,
            '#required' => true,
        ),
        'save' => array(
            '#type' => 'submit',
            '#value' => 'Save',
        ),
        'cancel' => array(
            '#type' => 'submit',
            '#value' => 'Cancel',
            '#submit' => array('pdq_ebms_internal_tag_form_submit'),
            '#limit_validation_errors' => array(),
        ),
    );
}

/**
 * Make sure a unique tag name and a valid status value.
 *
 *  @param  array   $form         Structured array containing the
 *                                elements and properties of the form
 *  @param  array   $form_state   Structured array containing the
 *                                current values of the form, as well
 *                                as other form state information
 */
function pdq_ebms_internal_tag_form_validate($form, &$form_state) {
    $values = $form_state['values'];
    if ($values['op'] != 'Save')
        return;
    if ($values['status'] != 'A' && $values['status'] != 'I')
        form_set_error('status', 'Invalid status value for tag');
    $name = trim($values['name']);
    if (empty(trim($name)))
        form_set_error('name', 'You must give the tag a name');
    else {
        $tag_id = db_select('ebms_internal_tag', 't')
            ->fields('t', array('tag_id'))
            ->condition('t.tag_name', $name)
            ->execute()
            ->fetchField();
        if ($tag_id && $tag_id != $values['tag'])
            form_set_error('name', 'Tag name is already in use');
    }
}

/**
 * Store the t information if the user has clicked on the 'Save' button.
 *
 *  @param  array   $form         Structured array containing the
 *                                elements and properties of the form
 *  @param  array   $form_state   Structured array containing the
 *                                current values of the form, as well
 *                                as other form state information
 */
function pdq_ebms_internal_tag_form_submit($form, &$form_state) {
    $values = $form_state['values'];
    if ($values['op'] == 'Save') {
        $tag_id = $values['tag'];
        $name = $values['name'];
        $status = $values['status'];
        $fields = array(
            'tag_name' => $name,
            'active_status' => $status,
        );
        if (!$tag_id)
            $topic = db_insert('ebms_internal_tag')
                ->fields($fields)
                ->execute();
        else
            db_update('ebms_internal_tag')
                ->fields($fields)
                ->condition('tag_id', $tag_id)
                ->execute();
        drupal_set_message('Tag ' . htmlspecialchars($name) . ' saved.');
    }
    drupal_goto('admin/internal-tags');
}

/**
 * Import internal articles.
 *
 * Implemented separately from the main import page, because the requirements
 * for this type of import are so radically different (for example, these
 * articles don't have a board topic, which is normally required, and we
 * aren't putting anything in the state table).
 *
 * It was a tossup whether this should go here or in import.inc. It had to
 * go somewhere, and import.inc was already plenty big.
 *
 *  @param  array   $form         Structured array containing the
 *                                elements and properties of the form
 *  @param  array   $form_state   Structured array containing the
 *                                current values of the form, as well
 *                                as other form state information
 */
function pdq_ebms_import_internal_articles() {
    return array(
        'left_nav' => Ebms\Util::build_left_nav('Internal Import'),
        drupal_get_form('pdq_ebms_import_internal_articles_form'),
    );
}

function pdq_ebms_import_internal_articles_form($form, &$form_state) {
    $results = db_select('ebms_internal_tag', 't')
        ->fields('t', array('tag_id', 'tag_name'))
        ->condition('t.active_status', 'A')
        ->execute();
    $tags = array();
    foreach ($results as $result)
        $tags[$result->tag_id] = htmlspecialchars($result->tag_name);
    return array(
        '#id' => 'internal-import-form',
        'title' => array('#markup' => '<h2>Import Internal Articles</h2>'),
        'pmid' => array(
            '#type' => 'textfield',
            '#title' => 'PMID(s)',
            '#required' => true,
            '#description' => t('Separate multiple PMIDs with commas ' .
                                'or spaces.'),
        ),
        'comment' => array(
            '#type' => 'textfield',
            '#title' => 'Comment',
            '#description' => t('We don\'t yet know if the comment is ' .
                                'associated with the article or the tag.'),
        ),
        'tag' => array(
            '#type' => 'checkboxes',
            '#title' => 'Tag(s)',
            '#options' => $tags,
            '#required' => true,
            '#description' => t('Select one or more internal article tags. ' .
                                'Additional tags can be added on the ' .
                                '"Full Citation" page.'),
        ),
        'full-text' => array(
            '#title' => 'Full Text PDF File',
            '#type' => 'file',
            '#upload_validators' => ['file_validate_extensions' => ['pdf']],
            '#description' => t('Only allowed if a single article is ' .
                                'being imported. Can also be added later ' .
                                'from the "Full Citation" page.'),
        ),
        'submit' => array(
            '#type' => 'submit',
            '#value' => 'Import',
        ),
        'cancel' => array(
            '#type' => 'submit',
            '#value' => 'Clear',
            // It's a little whacky, since Drupal already knows how
            // to derive the submit callback's name from the name
            // of the form, but if you don't explicitly set this
            // property, the #limit_validation_errors property is
            // ignored. Sound like a bug to you? You're not alone. :-)
            '#submit' => 'pdq_ebms_import_internal_articles_form_submit',
            '#limit_validation_errors' => array(),
        ),
    );
}

/**
 * Ensure that the import request meets our criteria.
 *
 *  @param  array   $form         Structured array containing the
 *                                elements and properties of the form
 *  @param  array   $form_state   Structured array containing the
 *                                current values of the form, as well
 *                                as other form state information
 */
function pdq_ebms_import_internal_articles_form_validate($form, &$form_state) {
}

/**
 * Perform the requested import.
 *
 *  @param  array   $form         Structured array containing the
 *                                elements and properties of the form
 *  @param  array   $form_state   Structured array containing the
 *                                current values of the form, as well
 *                                as other form state information
 */
function pdq_ebms_import_internal_articles_form_submit($form, &$form_state) {
    drupal_set_message('Can\'t implement Import until we know about comments');
    return;
    try {
        $batch = Ebms\importArticlesFromNLM('live', $pmids, null, null,
                                            $comment, false, 'I');
    }
    catch (\Exception $e) {
        drupal_set_message($e);
        $batch = null;
        $url = 'import-internal-articles';
    }
}
