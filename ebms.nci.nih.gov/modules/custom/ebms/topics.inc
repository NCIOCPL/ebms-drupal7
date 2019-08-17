<?php

/**
 * @file
 *
 * Interface for managing EBMS topics.  Added as part of work on TIR #2484.
 */
function pdq_ebms_topics($action='view', $topic_id=0) {
    drupal_add_css(Ebms\CSS_DIR . '/topics.css');
    switch ($action) {
        case 'edit':
            return drupal_get_form('pdq_ebms_topic_form', $topic_id);
        case 'add':
            return drupal_get_form('pdq_ebms_topic_form');
        case 'view':
        default:
            return pdq_ebms_show_topics();
    }
}

/**
 * Interface for managing EBMS topic groups. OCEEBMS-161.
 */
function pdq_ebms_topic_groups($action='view', $group_id=0) {
    drupal_add_css(Ebms\CSS_DIR . '/topics.css');
    switch ($action) {
        case 'edit':
            return drupal_get_form('pdq_ebms_topic_group_form', $group_id);
        case 'add':
            return drupal_get_form('pdq_ebms_topic_group_form');
        case 'view':
        default:
            return pdq_ebms_show_topic_groups();
    }
}

/**
 * Build a table of all of the topics in the EBMS, with links for editing
 * the existing topics or creating a new one.
 *
 *  @return   array         Drupal render array for table of topics
 */
function pdq_ebms_show_topics() {
    $query = db_select('ebms_topic', 't');
    $query->join('ebms_board', 'b', 'b.board_id = t.board_id');
    $query->join('users', 'u', 'u.uid = t.nci_reviewer');
    $query->leftJoin('ebms_topic_group', 'g', 'g.group_id = t.topic_group');
    $query->fields('t', array('topic_name', 'active_status', 'topic_id'));
    $query->fields('b', array('board_name'));
    $query->fields('u', array('name'));
    $query->fields('g', array('group_name'));
    $query->orderBy('b.board_name');
    $query->orderBy('t.topic_name');
    $results = $query->execute();
    $rows = array();
    foreach ($results as $result) {
        $group_name = '';
        if ($result->group_name)
            $group_name = htmlspecialchars($result->group_name);
        $row = array(
            htmlspecialchars($result->board_name),
            htmlspecialchars($result->topic_name),
            $group_name,
            htmlspecialchars($result->name),
            $result->active_status == 'A' ? 'Active' : 'Inactive',
            l('Edit', 'admin/topics/edit/' . $result->topic_id),
        );
        $rows[] = $row;
    }
    $headers = array(
        'Board',
        'Topic',
        'Topic Group',
        'NCI Reviewer',
        'Status',
        'Action',
    );
    $button_class = array('class' => array('button'));
    $attrs = array('attributes' => $button_class);
    return array(
        /*'title' => array(
            '#markup' => '<h2 id="topics-title">EBMS Topics</h2>',
            ),*/
        'new-link' => array(
            '#markup' => l('Add New Topic', 'admin/topics/add', $attrs) .
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
 * Build a table of all of the topic groups in the EBMS, with links for editing
 * the existing topic groups or creating a new one.
 *
 *  @return   array         Drupal render array for table of topics groups
 */
function pdq_ebms_show_topic_groups() {
    $query = db_select('ebms_topic_group', 'g');
    $query->fields('g');
    $query->orderBy('g.group_name');
    $results = $query->execute();
    $rows = array();
    foreach ($results as $result) {
        $row = array(
            htmlspecialchars($result->group_name),
            $result->active_status == 'A' ? 'Active' : 'Inactive',
            l('Edit', 'admin/topic-groups/edit/' . $result->group_id),
        );
        $rows[] = $row;
    }
    $headers = array(
        'Group Name',
        'Status',
        'Action',
    );
    $button_class = array('class' => array('button'));
    $attrs = array('attributes' => $button_class);
    $link = l('Add New Topic Group', 'admin/topic-groups/add', $attrs);
    return array(
        /*
        'title' => array(
            '#markup' => '<h2 id="topics-title">EBMS Topic Groups</h2>',
        ),
        */
        'new-link' => array(
            '#markup' => "$link<br><br>",
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
 * existing EBMS topic or creating a new one.
 *
 *  @param  array   $form         Structured array containing the
 *                                elements and properties of the form
 *  @param  array   &$form_state  Modifiable structured array
 *                                containing the current values
 *                                of the form, as well as other
 *                                form state information
 *  @param  int     $topic_id     Primary key for the topic being
 *                                edited, if we are not creating a
 *                                new topic
 *
 *  @return array                 Array containing specification
 *                                of the fields to appear on the
 *                                form, as well as other markup
 */
function pdq_ebms_topic_form($form, &$form_state, $topic_id=0) {

    // Override Drupal's default breadcrumb mechanism.
    drupal_set_breadcrumb(
        array(
            l('Home', 'home'),
            l('Topics', 'admin/topics'),
        )
    );

    // Initialize local variables to their defaults.
    $name = '';
    $board_id = $nci_reviewer = $group_id = 0;
    $status = 'A';
    $selected_reviewers = array();
    $boards = array(0 => ' ') + Ebms\Util::boards();
    $topic_reviewers_wrapper = array(
        '#prefix' => '<div id="topic-reviewers-wrapper">',
        '#suffix' => '</div>',
    );

    // Catch any changes to the form from the AJAX trigger.
    if (isset($form_state['values'])) {
        $values = $form_state['values'];
        $name = $values['name'];
        $board_id = $values['board'];
        $group_id = $values['group'];
        $status = $values['status'];
        $nci_reviewer = $values['nci-reviewer'];
        foreach ($values['topic-reviewers'] as $key => $value) {
            if ($value)
                $selected_reviewers[] = $key;
        }
    }

    // If we're editing an existing topic, collect the current values.
    elseif ($topic_id) {
        $query = db_select('ebms_topic', 't');
        $query->fields('t');
        $query->condition('t.topic_id', $topic_id);
        $topic = $query->execute()->fetch();
        $board_id = $topic->board_id;
        $group_id = $topic->topic_group;
        $name = $topic->topic_name;
        $nci_reviewer = $topic->nci_reviewer;
        $status = $topic->active_status;
        $query = db_select('ebms_topic_reviewer', 'r');
        $query->fields('r', array('user_id'));
        $query->distinct();
        $query->condition('r.topic_id', $topic_id);
        foreach ($query->execute() as $result)
            $selected_reviewers[] = $result->user_id;
    }

    // We need to pass this to the AJAX callback because of a Drupal FAPI bug.
    $form_state['ebms-checked-reviewers'] = $selected_reviewers;

    // Build the picklist for the topic groups.
    $query = db_select('ebms_topic_group', 'g');
    $query->condition('g.active_status', 'A');
    $query->fields('g', array('group_id', 'group_name'));
    $query->orderBy('g.group_name');
    $groups = array(0 => '');
    foreach ($query->execute() as $group)
        $groups[$group->group_id] = $group->group_name;

    // Build the picklist values for the NCI Reviewer field.  Skip Bonnie
    // and the medical librarians.
    $reviewer_roles = array('board manager', 'icrdb manager');
    $query = db_select('users_roles', 'u');
    $query->join('role', 'r', 'r.rid = u.rid');
    $query->fields('u', array('uid'));
    $query->condition('r.name', array('admin assistant', 'medical librarian'),
        'IN');
    $query->distinct();
    $skip = array();
    foreach ($query->execute() as $u)
        $skip[] = $u->uid;
    $query = db_select('users', 'u');
    $query->fields('u', array('name', 'uid'));
    $query->join('users_roles', 'ur', 'u.uid = ur.uid');
    $query->join('role', 'r', 'r.rid = ur.rid');
    $query->condition('u.status', 1);
    $query->condition('r.name', $reviewer_roles, 'IN');
    $query->orderBy('u.name');
    $query->distinct();
    $nci_reviewers = array(0 => ' ');
    foreach ($query->execute() as $u) {
        if (!in_array($u->uid, $skip))
            $nci_reviewers[$u->uid] = $u->name;
    }

    // If a board has been selected, add the Topic Reviewers field.
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
        $topic_reviewers_wrapper['topic-reviewers'] = array(
            '#title' => 'Topic Reviewers',
            '#type' => 'checkboxes',
            '#options' => $board_members,
            '#default_value' => $selected_reviewers,
        );
    }

    // Assemble and return the form render array.
    $title = $topic_id ? 'Edit Topic' : 'Add Topic';
    return array(
        /*
        'title' => array(
            '#markup' => "<h2>$title</h2>",
            ),*/
        'name' => array(
            '#type' => 'textfield',
            '#title' => 'Topic Name',
            '#required' => true,
            '#default_value' => $name,
            '#maxlength' => 255,
        ),
        'topic' => array(
            '#type' => 'hidden',
            '#value' => $topic_id,
        ),
        'group' => array(
            '#type' => 'select',
            '#title' => t('Topic Group'),
            '#options' => $groups,
            '#default_value' => $group_id,
        ),
        'board' => array(
            '#type' => 'select',
            '#title' => t('Board'),
            '#required' => true,
            '#default_value' => $board_id,
            '#options' => $boards,
            '#ajax' => array(
                'callback' => 'pdq_ebms_topic_ajax',
                'wrapper' => 'topic-reviewers-wrapper',
            ),
        ),
        'status' => array(
            '#type' => 'radios',
            '#title' => 'Status',
            '#options' => array('A' => 'Active', 'I' => 'Inactive'),
            '#default_value' => $status,
            '#required' => true,
        ),
        'nci-reviewer' => array(
            '#title' => 'NCI Reviewer',
            '#type' => 'select',
            '#options' => $nci_reviewers,
            '#default_value' => $nci_reviewer,
            '#required' => true,
        ),
        'topic-reviewers-wrapper' => $topic_reviewers_wrapper,
        'save' => array(
            '#type' => 'submit',
            '#value' => 'Save',
        ),
        'cancel' => array(
            '#type' => 'submit',
            '#value' => 'Cancel',
            '#submit' => array('pdq_ebms_topic_form_submit'),
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
function pdq_ebms_topic_ajax($form, $form_state) {
    $checked_reviewers = $form_state['ebms-checked-reviewers'];
    $subform = $form['topic-reviewers-wrapper'];
    foreach ($subform['topic-reviewers'] as $k => $v) {
        if (isset($v['#type']) and $v['#type'] == 'checkbox') {
            $checked = in_array($k, $checked_reviewers) ? 1 : 0;
            $subform['topic-reviewers'][$k]['#checked'] = $checked;
        }
    }
    return $subform;
}

/**
 * Make sure everything we need for the topic is present.
 *
 *  @param  array   $form         Structured array containing the
 *                                elements and properties of the form
 *  @param  array   $form_state   Structured array containing the
 *                                current values of the form, as well
 *                                as other form state information
 */
function pdq_ebms_topic_form_validate($form, &$form_state) {
    $values = $form_state['values'];
    if ($values['op'] != 'Save')
        return;
    if (!$values['name'])
        form_set_error('name', 'You must give the topic a name');
    if (!$values['board'])
        form_set_error('board', 'You must select a board for the topic');
    if ($values['status'] != 'A' && $values['status'] != 'I')
        form_set_error('status', 'Invalid status value for topic');
    if (!$values['nci-reviewer'])
        form_set_error('nci-reviewer',
            'You must select an NCI reviewer for the topic');
}

/**
 * Store the topic information if the user has clicked on the 'Save' button.
 *
 *  @param  array   $form         Structured array containing the
 *                                elements and properties of the form
 *  @param  array   $form_state   Structured array containing the
 *                                current values of the form, as well
 *                                as other form state information
 */
function pdq_ebms_topic_form_submit($form, &$form_state) {
    $values = $form_state['values'];
    if ($values['op'] == 'Save') {
        $topic = $values['topic'];
        $name = $values['name'];
        $board = $values['board'];
        $group = $values['group'];
        $status = $values['status'];
        $nci_reviewer = $values['nci-reviewer'];
        if (!$group)
            $group = null;
        $fields = array(
            'topic_name'    => $name,
            'board_id'      => $board,
            'nci_reviewer'  => $nci_reviewer,
            'active_status' => $status,
            'topic_group'   => $group,
        );
        if (!$topic)
            $topic = db_insert('ebms_topic')
                ->fields($fields)
                ->execute();
        else {
            db_update('ebms_topic')
                ->fields($fields)
                ->condition('topic_id', $topic)
                ->execute();
            db_delete('ebms_topic_reviewer')
                ->condition('topic_id', $topic)
                ->execute();
        }
        foreach ($values['topic-reviewers'] as $key => $value) {
            if ($value)
                db_insert('ebms_topic_reviewer')
                    ->fields(
                        array(
                            'topic_id' => $topic,
                            'user_id' => $key,
                        )
                    )
                    ->execute();
        }
        drupal_set_message('Topic ' . htmlspecialchars($name) . ' saved.');
    }
    drupal_goto('admin/topics');
}

/**
 * Drupal form API callback which generates the form for editing an
 * existing EBMS topic group or creating a new one.
 *
 *  @param  array   $form         Structured array containing the
 *                                elements and properties of the form
 *  @param  array   &$form_state  Modifiable structured array
 *                                containing the current values
 *                                of the form, as well as other
 *                                form state information
 *  @param  int     $group_id     Primary key for the group being
 *                                edited, if we are not creating a
 *                                new group
 *
 *  @return array                 Array containing specification
 *                                of the fields to appear on the
 *                                form, as well as other markup
 */
function pdq_ebms_topic_group_form($form, &$form_state, $group_id=0) {

    // Override Drupal's default breadcrumb mechanism.
    drupal_set_breadcrumb(
        array(
            l('Home', 'home'),
            l('Topic Groups', 'admin/topic-groups'),
        )
    );

    // Initialize local variables to their defaults.
    $name = '';
    $status = 'A';

    // Catch any changes to the form.
    if (isset($form_state['values'])) {
        $values = $form_state['values'];
        $name = $values['name'];
        $status = $values['status'];
    }

    // If we're editing an existing group, collect the current values.
    elseif ($group_id) {
        $query = db_select('ebms_topic_group', 'g');
        $query->fields('g');
        $query->condition('g.group_id', $group_id);
        $group = $query->execute()->fetch();
        $name = $group->group_name;
        $status = $group->active_status;
    }

    // Assemble and return the form render array.
    $title = $group_id ? 'Edit Topic Group' : 'Add Topic Group';
    return array(
        /*
        'title' => array(
            '#markup' => "<h2>$title</h2>",
            ),*/
        'name' => array(
            '#type' => 'textfield',
            '#title' => 'Topic Group Name',
            '#required' => true,
            '#default_value' => $name,
            '#maxlength' => 255,
        ),
        'group' => array(
            '#type' => 'hidden',
            '#value' => $group_id,
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
            '#submit' => array('pdq_ebms_topic_group_form_submit'),
            '#limit_validation_errors' => array(),
        ),
    );
}

/**
 * Make sure everything we need for the group is present.
 *
 *  @param  array   $form         Structured array containing the
 *                                elements and properties of the form
 *  @param  array   $form_state   Structured array containing the
 *                                current values of the form, as well
 *                                as other form state information
 */
function pdq_ebms_topic_group_form_validate($form, &$form_state) {
    $values = $form_state['values'];
    if ($values['op'] != 'Save')
        return;
    if (!$values['name'])
        form_set_error('name', 'You must give the group a name');
    if ($values['status'] != 'A' && $values['status'] != 'I')
        form_set_error('status', 'Invalid status value for group');
}

/**
 * Store the group information if the user has clicked on the 'Save' button.
 *
 *  @param  array   $form         Structured array containing the
 *                                elements and properties of the form
 *  @param  array   $form_state   Structured array containing the
 *                                current values of the form, as well
 *                                as other form state information
 */
function pdq_ebms_topic_group_form_submit($form, &$form_state) {
    $values = $form_state['values'];
    if ($values['op'] == 'Save') {
        $group = $values['group'];
        $name = $values['name'];
        $status = $values['status'];
        $fields = array(
            'group_name'    => $name,
            'active_status' => $status,
        );
        if (!$group)
            db_insert('ebms_topic_group')
                ->fields($fields)
                ->execute();
        else
            db_update('ebms_topic_group')
                ->fields($fields)
                ->condition('group_id', $group)
                ->execute();
        drupal_set_message('Group ' . htmlspecialchars($name) . ' saved.');
    }
    drupal_goto('admin/topic-groups');
}
