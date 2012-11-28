<?php

/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */



$GLOBALS['ebms_event_fields'] = array(
// event date - 3 fields
// time - from and to, am | pm
    array(
        'label' => t('Event Date'),
        'entity_type' => 'node',
        'description' => t('Date of the beginning of the event.'),
        'field_name' => 'event_date',
        'required' => TRUE,
        'type' => 'datestamp',
        'widget' => array(
            'weight' => '2',
            'type' => 'date_popup',
            'settings' => array(
                'input_format' => 'Y-m-d H:i',
                'year_range' => '-100:+5',
                'increment' => 1,
            ),
        ),
        'cardinality' => '1',
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
    /*array(
        'label' => t('End Date'),
        'entity_type' => 'node',
        'description' => t('Date of the end of the event.'),
        'field_name' => 'end_date',
        'required' => TRUE,
        'type' => 'datestamp',
        'widget' => array(
            'weight' => '3',
            'type' => 'date_popup',
            'settings' => array(
                'input_format' => 'Y-m-d H:i',
                'year_range' => '-100:+5',
                'increment' => 1,
            ),
        ),
        'cardinality' => '1',
        'settings' => array(
            'repeat' => 0,
            'todate' => '',
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
    ),*/
// event type - in person | webex / phone conf.
    array(
        'label' => t('Event Type'),
        'entity_type' => 'node',
        'description' => t('Select between an in-person or a remote webex/telephone conference.'),
        'field_name' => 'event_type',
        'required' => TRUE,
        'type' => 'list_text',
        'cardinality' => '-1',
        'widget' => array(
            'weight' => '4',
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
        'field_name' => 'board',
        'required' => FALSE,
        'type' => 'list_integer',
        'cardinality' => '1',
        'widget' => array(
            'weight' => '5',
            'type' => 'options_select',
        ),
        'settings' => array(
        ),
        'bundles' => array(
            'ebms_event'
        ),
    ),
// subgroup board - limited to those appropriate for editorial board if chosen
    array(
        'label' => t('Subgroup'),
        'entity_type' => 'node',
        'description' => t('Select from the available subgroups.'),
        'field_name' => 'subgroup',
        'required' => FALSE,
        'type' => 'list_integer',
        'cardinality' => '1',
        'widget' => array(
            'weight' => '6',
            'type' => 'options_select',
        ),
        'settings' => array(
        ),
        'bundles' => array(
            'ebms_event'
        ),
    ),
// ad-hoc group
    array(
        'label' => t('Ad-Hoc Group'),
        'entity_type' => 'node',
        'description' => t('Select from the available ad-hoc groups.'),
        'field_name' => 'adhoc_group',
        'required' => FALSE,
        'type' => 'list_integer',
        'cardinality' => '1',
        'widget' => array(
            'weight' => '7',
            'type' => 'options_select',
        ),
        'settings' => array(
        ),
        'bundles' => array(
            'ebms_event'
        ),
    ),
// in-house staff (board managers, medical librarians, administrators)
    array(
        'label' => t('In-House Staff'),
        'entity_type' => 'node',
        'description' => t('Select from the available in-house staff.'),
        'field_name' => 'inhouse_staff',
        'required' => FALSE,
        'type' => 'list_integer',
        'cardinality' => '-1',
        'widget' => array(
            'weight' => '8',
            'type' => 'options_select',
        ),
        'settings' => array(
        ),
        'bundles' => array(
            'ebms_event'
        ),
    ),
// board members - multiple select, populated/limited from editorial board?
    array(
        'label' => t('Board Members'),
        'entity_type' => 'node',
        'description' => t('Select from the available board memebers.'),
        'field_name' => 'board_members',
        'required' => FALSE,
        'type' => 'list_integer',
        'cardinality' => '-1',
        'widget' => array(
            'weight' => '9',
            'type' => 'options_select',
        ),
        'settings' => array(
        ),
        'bundles' => array(
            'ebms_event'
        ),
    ),
// notes - free form text
    array(
        'label' => t('Notes'),
        'entity_type' => 'node',
        'description' => t('Enter any notes about this event.'),
        'field_name' => 'event_notes',
        'required' => FALSE,
        'type' => 'text_long',
        'cardinality' => '1',
        'widget' => array(
            'weight' => '10',
        ),
        'settings' => array(
            'max_length' => '',
            'text_processing' => 1
        ),
        'bundles' => array(
            'ebms_event',
        ),
    ),
);
?>
