<?php

/**
 * Definitions of custom fields added to the existing ebms node types.
 *
 * These fields are created by hand and stored in $GLOBALS where they are
 * picked up via module_load_include() by ebms.install, which creates
 * or updates the fields as needed.
 *
 * Edits to the field definitions here may fail if any changes violate the
 * limits Drupal imposes in field_update_field() and field_update_instance().
 * [I'm not sure it's possible to find what those limits are except by
 * reading the source code for those Drupal functions.]
 *
 * As of this writing, deleting a field definition from this array
 * does NOT delete the field from the database if it is already there.
 */

// DEBUG
/*
function t($x) {}
define('FIELD_CARDINALITY_UNLIMITED', 0);
define('LANGUAGE_NONE', 'und');
*/

$GLOBALS['ebms_node_fields'] = array(
    // event status - on calendar or cancelled
    array(
        'label' => t('Event Status'),
        'entity_type' => 'node',
        'description' => t('Select to set this event as scheduled or cancelled.'),
        'field_name' => 'field_event_status',
        'required' => TRUE,
        'type' => 'list_text',
        'cardinality' => 1,
        'widget' => array(
            'weight' => 1,
            'type' => 'options_buttons',
        ),
        'settings' => array(
            'allowed_values' => array(
                'scheduled' => 'On Calendar',
                'cancelled' => 'Cancelled',
            ),
        ),
        'default_value' => array(0 => array('value' => 'scheduled')),
        'bundles' => array(
            'ebms_event'
        ),
    ),
    // event date
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
                'input_format' => 'M j Y - g:i:sa',
                'year_range' => '-100:+5',
                'increment' => 30,
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
            'default_value' => 'now',
            'default_value_code' => '',
            'default_value2' => 'strtotime',
            'default_value_code2' => '+1 hour',
        ),
        'bundles' => array(
            'ebms_event',
        ),
    ),
    // event category - states if the event is board or subgroup-specific
    array(
        'label' => t('Event Category'),
        'entity_type' => 'node',
        'description' => t('Select to set this event as board or subgroup-specific.'),
        'field_name' => 'field_event_category',
        'required' => FALSE,
        'type' => 'list_text',
        'cardinality' => 1,
        'widget' => array(
            'weight' => 3,
            'type' => 'options_select',
        ),
        'settings' => array(
            'allowed_values' => array(
                'board' => 'Board',
                'subgroup' => 'Subgroup',
            ),
        ),
        'bundles' => array(
            'ebms_event'
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
            'type' => 'options_buttons',
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
    // agenda status - if the agenda is published
    array(
        'label' => t('Publish Agenda'),
        'entity_type' => 'node',
        'description' => t('Set if the agenda is published and visible to board members.'),
        'field_name' => 'field_agenda_status',
        'required' => TRUE,
        'type' => 'list_text',
        'default_value' => array(0 => array('value' => '0')),
        'cardinality' => 1,
        'widget' => array(
            'weight' => 11,
            'type' => 'options_select',
        ),
        'settings' => array(
            'allowed_values' => array(
                '0' => 'Saved',
                '1' => 'Published',
            ),
        ),
        'bundles' => array(
            'ebms_event'
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
            'weight' => 12,
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
            'weight' => 13
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

// this global array will be filled with modifications of the array above
$GLOBALS['ebms_entity_fields'] = array(
    array(
        'label' => t('Title'),
        'entity_type' => 'message',
        'description' => t('A replicated node, packet, or summary title.'),
        'field_name' => 'field_title',
        'required' => TRUE,
        'type' => 'text',
        'cardinality' => '1',
        'widget' => array(
            'weight' => '-1',
        ),
        'settings' => array(
            'max_length' => '255',
        ),
        'bundles' => array(
            'activity_new_event',
            'activity_cancelled_event',
            'activity_changed_event',
            'activity_agenda_published',
            'activity_event_type',
            'activity_new_packet',
            'activity_new_summary',
        ),
    ),
    array(
        'label' => t('Event ID'),
        'entity_type' => 'message',
        'description' => t('Primary key used for creating links to events'),
        'field_name' => 'field_event_id',
        'required' => false,
        'type' => 'number_integer',
        'cardinality' => '1',
        'widget' => array(
            'weight' => '-1',
        ),
        'bundles' => array(
            'activity_new_event',
            'activity_cancelled_event',
            'activity_changed_event',
            'activity_agenda_published',
            'activity_event_type',
        ),
    ),
    array(
        'label' => t('Packet ID'),
        'entity_type' => 'message',
        'description' => t('Primary key used for creating links to EBMS review packets'),
        'field_name' => 'field_packet_id',
        'required' => true,
        'type' => 'number_integer',
        'cardinality' => '1',
        'widget' => array(
            'weight' => '-1',
        ),
        'bundles' => array('activity_new_packet'),
    ),
    array(
        'label' => t('Summary URL'),
        'entity_type' => 'message',
        'description' => t('Link to summary document'),
        'field_name' => 'field_summary_url',
        'required' => true,
        'type' => 'text',
        'cardinality' => '1',
        'widget' => array(
            'weight' => '-1',
        ),
        'settings' => array(
            'max_length' => '255',
        ),
        'bundles' => array('activity_new_summary'),
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
                'activity_agenda_published',
                'activity_event_type',
                'activity_new_packet',
                'activity_new_summary',
            );

            $GLOBALS['ebms_entity_fields'][] = $messageField;

            break;
        case 'field_event_type':
            $messageField = $field;
            $messageField['entity_type'] = 'message';
            $messageField['bundles'] = array(
                'activity_event_type',
            );

            $GLOBALS['ebms_entity_fields'][] = $messageField;

            break;
    }
}

/**
 * Per user fields, added to support some user profile information.
 *
 * Most of these are used in storing user selected default display
 * options for various screens offering options.
 *
 * As of this writing, values for these fields are set by a user
 * clicking "Save as defaults" in the user interface for Display Options
 * in one of the EBMS pages that supports defaults.  The options construction
 * and value validation for the fields is done there, not by settings
 * added here.  That way additions to values can be added without modifying
 * anything here.
 */
$GLOBALS['ebms_user_fields'] = array(
    // --------------------------------
    // Overall defaults, applying in multiple places
    // --------------------------------
    // Default board selection
    // The value is the board_id as stored in the ebms_board table
    array(
        'label' => t('Board'),
        'entity_type' => 'user',
        'description' => 'Default editorial board',
        'field_name' => 'dft_editorial_board',
        'required' => FALSE,
        'type' => 'number_integer',
        'settings' => array('max_length' => '128',),
        'cardinality' => 1,
        'bundles' => array('user'),
        'locked' => FALSE,
        'no_ui' => FALSE
    ),

    // --------------------------------
    // Citation review display options
    // --------------------------------
    // Citation review sort order, 'EBMS ID#', 'Author', etc.
    array(
        'label' => t('Sort By'),
        'entity_type' => 'user',
        'description' => 'Default field for sorting citation review queue',
        'field_name' => 'dft_cite_review_sort_key',
        'required' => FALSE,
        'type' => 'text',
        'settings' => array('max_length' => '32',),
        'cardinality' => 1,
        'bundles' => array('user'),
        'locked' => TRUE,
        'no_ui' => TRUE
    ),
    // Citation format, 'Brief', 'Abstract'
    array(
        'label' => t('Format'),
        'entity_type' => 'user',
        'description' => 'Default format for citation review display',
        'field_name' => 'dft_cite_review_format',
        'required' => FALSE,
        'type' => 'text',
        'settings' => array('max_length' => '32',),
        'cardinality' => 1,
        'bundles' => array('user'),
        'locked' => TRUE,
        'no_ui' => TRUE
    ),
    // Citations per review page
    array(
        'label' => t('Items per page'),
        'entity_type' => 'user',
        'description' => 'Default number of review citations per page',
        'field_name' => 'dft_cite_review_items_per_page',
        'required' => FALSE,
        'type' => 'number_integer',
        'cardinality' => 1,
        'bundles' => array('user'),
        'locked' => TRUE,
        'no_ui' => TRUE
    ),

    // --------------------------------
    // Citation search display options
    // --------------------------------
    // Citation search results sort order, 'EBMS ID#', 'Author', etc.
    array(
        'label' => t('Sort By'),
        'entity_type' => 'user',
        'description' => 'Default field for sorting citation search results',
        'field_name' => 'dft_cite_search_sort_key',
        'required' => FALSE,
        'type' => 'text',
        'settings' => array('max_length' => '32',),
        'cardinality' => 1,
        'bundles' => array('user'),
        'locked' => TRUE,
        'no_ui' => TRUE
    ),

    // Citations per search results page
    array(
        'label' => t('Items per page'),
        'entity_type' => 'user',
        'description' => 'Default number of search citations per page',
        'field_name' => 'dft_cite_search_items_per_page',
        'required' => FALSE,
        'type' => 'number_integer',
        'cardinality' => 1,
        'bundles' => array('user'),
        'locked' => TRUE,
        'no_ui' => TRUE
    ),

    // --------------------------------
    // Create packet display options
    // --------------------------------
    // Packet article sort order, Author or Article title
    array(
        'label' => t('Sort by'),
        'entity_type' => 'user',
        'description' => 'Default field for sorting article list',
        'field_name' => 'dft_packet_article_sort_key',
        'required' => FALSE,
        'type' => 'text',
        'settings' => array('max_length' => '32',),
        'cardinality' => 1,
        'bundles' => array('user'),
        'locked' => TRUE,
        'no_ui' => TRUE
    ),
);
