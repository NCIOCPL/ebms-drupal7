<?php

/**
 * @file
 *
 * Interface for managing internal EBMS articles.  Added as part of work on
 * OCEEBMS-509.
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
        'title' => array(
            '#markup' =>
                '<h2 id="internal-tags-title">EBMS Internal Tags</h2>',
        ),
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
