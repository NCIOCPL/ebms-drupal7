<?php

/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */

$GLOBALS['ebms_node_fields'] = array(
// event date - 3 fields
// time - from and to, am | pm
    array(
        'label' => t('Event Date'),
        'entity_type' => 'node',
        'description' => t('Date of the event.'),
        'field_name' => 'field_datespan',
        'required' => TRUE,
        'type' => 'datestamp',
        'widget' => array(
            'weight' => 2,
            'type' => 'date_popup',
            'settings' => array(
                'input_format' => 'Y-m-d H:i',
                'year_range' => '-100:+5',
                'increment' => 1,
            ),
        ),
        'cardinality' => 1,
        'settings' => array(
            'repeat' => 0,
            'todate' => 'required',
            'granularity' => array(
                'year' => 'year',
                'month' => 'month',
                'day' => 'day',
                'hour' => 'hour',
                'minute' => 'minute',
            ),
        ),
        'bundles' => array(
            'ebms_event',
        ),
    ),
// event type - in person | webex / phone conf.
    array(
        'label' => t('Event Type'),
        'entity_type' => 'node',
        'description' => t('Select between an in-person or a remote webex/telephone conference.'),
        'field_name' => 'field_event_type',
        'required' => TRUE,
        'type' => 'list_text',
        'cardinality' => 1,
        'widget' => array(
            'weight' => 4,
            'type' => 'options_select',
        ),
        'settings' => array(
            'allowed_values' => array(
                'in_person' => 'In Person',
                'remote' => 'Webex/Phone Conf.',
            ),
        ),
        'bundles' => array(
            'ebms_event'
        ),
    ),
// editorial board
    array(
        'label' => t('Board'),
        'entity_type' => 'node',
        'description' => t('Select an editorial board.'),
        'field_name' => 'field_boards',
        'required' => FALSE,
        'type' => 'list_integer',
        'cardinality' => FIELD_CARDINALITY_UNLIMITED,
        'widget' => array(
            'weight' => 5,
            'type' => 'options_select',
        ),
        'settings' => array(
            'allowed_values_function' => 'ebms_board_allowed_values_callback',
        ),
        'bundles' => array(
            'ebms_event',
        ),
    ),
// subgroup board - limited to those appropriate for editorial board if chosen
    array(
        'label' => t('Subgroup'),
        'entity_type' => 'node',
        'description' => t('Select from the available subgroups.'),
        'field_name' => 'field_subgroups',
        'required' => FALSE,
        'type' => 'list_integer',
        'cardinality' => FIELD_CARDINALITY_UNLIMITED,
        'widget' => array(
            'weight' => 6,
            'type' => 'options_select',
        ),
        'settings' => array(
            'allowed_values_function' => 'ebms_subgroup_allowed_values_callback',
        ),
        'bundles' => array(
            'ebms_event',
        ),
    ),
// ad-hoc group
    array(
        'label' => t('Ad-Hoc Group'),
        'entity_type' => 'node',
        'description' => t('Select from the available ad-hoc groups.'),
        'field_name' => 'field_ad_hoc_groups',
        'required' => FALSE,
        'type' => 'list_integer',
        'cardinality' => FIELD_CARDINALITY_UNLIMITED,
        'widget' => array(
            'weight' => 7,
            'type' => 'options_select',
        ),
        'settings' => array(
            'allowed_values_function' => 'ebms_ad_hoc_group_allowed_values_callback',
        ),
        'bundles' => array(
            'ebms_event',
        ),
    ),
    // Individuals who can be selected
    /* array(
      'label' => t('Individual Recipients'),
      'entity_type' => 'node',
      'description' => t('Select from the list of individual recipients.'),
      'field_name' => ''
      ), */
    
    // in-house staff and board members merged into 'Individuals' field
    
// in-house staff (board managers, medical librarians, administrators)
    /*array(
        'label' => t('In-House Staff'),
        'entity_type' => 'node',
        'description' => t('Select from the available in-house staff.'),
        'field_name' => 'field_inhouse_staff',
        'required' => FALSE,
        'type' => 'list_integer',
        'cardinality' => FIELD_CARDINALITY_UNLIMITED,
        'widget' => array(
            'weight' => 8,
            'type' => 'options_select',
        ),
        'settings' => array(
        ),
        'bundles' => array(
        ),
    ),
// board members - multiple select, populated/limited from editorial board?
    array(
        'label' => t('Board Members'),
        'entity_type' => 'node',
        'description' => t('Select from the available board members.'),
        'field_name' => 'field_board_members',
        'required' => FALSE,
        'type' => 'list_integer',
        'cardinality' => FIELD_CARDINALITY_UNLIMITED,
        'widget' => array(
            'weight' => 9,
            'type' => 'options_select',
        ),
        'settings' => array(
        ),
        'bundles' => array(
        ),
    ),*/
    // agenda - rich text
    array(
        'label' => t('Agenda'),
        'entity_type' => 'node',
        //'description' => t('The agenda for the event.'),
        'field_name' => 'field_agenda',
        'required' => FALSE,
        'type' => 'text_long',
        'cardinality' => 1,
        'widget' => array(
            'weight' => 10,
        ),
        'settings' => array(
            'max_length' => '',
            'text_processing' => 1
        ),
        'bundles' => array(
            'ebms_event',
        ),
    ),
    array(
        'label' => 'Meeting Documents',
        'entity_type' => 'node',
        'description' => t('Documents relating to this event.'),
        'field_name' => 'field_documents',
        'cardinality' => FIELD_CARDINALITY_UNLIMITED,
        'type' => 'file',
        'settings' => array(
            'display_field' => 1,
            'display_default' => 1,
            'file_extensions' => 'txt doc pdf docx ppt pptx xls xlsx jpg jpeg gif bmp png tiff mpg mpeg mp4 3gp avi htm html',
        ),
        'widget' => array(
            'type' => 'file_generic',
            'settings' => array(
                'description_field' => 1,
            ),
            'weight' => 11,
        ),
        'bundles' => array(
            'ebms_event',
        ),
    ),
// notes - free form text
    array(
        'label' => t('Notes'),
        'entity_type' => 'node',
        //'description' => t('Any related notes for the event.'),
        'field_name' => 'field_notes',
        'required' => FALSE,
        'type' => 'text_long',
        'cardinality' => 1,
        'widget' => array(
            'weight' => 12,
        ),
        'settings' => array(
            'max_length' => '',
            'text_processing' => 1
        ),
        'bundles' => array(
            'ebms_event',
        ),
    ),
    // individuals (list of uids)
    array(
        'label' => t('Individuals'),
        'entity_type' => 'node',
        'description' => t('Select from the available users.'),
        'field_name' => 'field_individuals',
        'required' => FALSE,
        'type' => 'list_integer',
        'cardinality' => FIELD_CARDINALITY_UNLIMITED,
        'widget' => array(
            'weight' => 9,
            'type' => 'options_select',
        ),
        'settings' => array(
            'allowed_values_function' => 'ebms_individuals_allowed_values_callback',
        ),
        'bundles' => array(
            'ebms_event',
        ),
    ),
);

// this global array will be filled with midifications of the array above
$GLOBALS['ebms_entity_fields'] = array(
    array(
        'label' => t('Title'),
        'entity_type' => 'message',
        'description' => t('A replicated node title.'),
        'field_name' => 'field_title',
        'required' => TRUE,
        'type' => 'text',
        'cardinality' => '1',
        'widget' => array(
            'weight' => '-1',
        ),
        'settings' => array(
            'max_length' => '60',
        ),
        'bundles' => array(
            'activity_new_event',
            'activity_cancelled_event',
            'activity_changed_event',
        ),
    ),
);

foreach ($GLOBALS['ebms_node_fields'] as $field) {
    switch ($field['field_name']) {
        case 'field_boards':
        case 'field_subgroups':
        case 'field_ad_hoc_groups':
        case 'field_individuals':
            $messageField = $field;
            $messageField['entity_type'] = 'message';
            $messageField['bundles'] = array(
                'activity_new_articles',
                'activity_new_event',
                'activity_cancelled_event',
                'activity_changed_event',
            );

            $GLOBALS['ebms_entity_fields'][] = $messageField;

            break;
    }
}
?>
