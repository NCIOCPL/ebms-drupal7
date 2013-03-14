<?php

// $Id$

function _calendar_get_upcoming_events() {
    $query = new EntityFieldQuery();
    $query
        ->entityCondition('entity_type', 'node')
        ->entityCondition('bundle', 'ebms_event')
        ->propertyCondition('status', '1')
        ->fieldCondition('field_datespan', 'value2', time(), '>')
        ->range(0, 6)
        ->fieldOrderBy('field_datespan', 'value')
        ->entityOrderBy('entity_id');

    $result = $query->execute();

    if (!isset($result['node']))
        return array();

    $eventNids = array_keys($result['node']);
    $events = entity_load('node', $eventNids);
    $groupedResults = array();
    foreach ($events as $event) {
        $event_date = field_get_items('node', $event, 'field_datespan');
        if ($event_date === false) {
            watchdog("ebms - calendar", 'bad event date');
            continue;
        }

        $timestamp = $event_date[0]['value'];

        $date = date('F j, Y', $timestamp);
        $groupedResults[$date][] = $event;
    }
    return $groupedResults;
}

function ebms_calendar_left_nav($span = null, $args = array()) {
    $output = array();

    $date = null;
    if (isset($args[0]))
        $date = $args[0];

    $all = false;
    $granularity = '';
    if ($span) {
        $span_parts = explode('_', $span);

        $all = sizeof($span_parts) > 1;
        $granularity = '/' . $span_parts[0];
    }

    $suffix = $all ? '/all' : '';

    // create the links to month, week, and day views
    $output[] = '<div id="calendar-views">';
    $output[] = '<div class="item-list">';
    if ($granularity == '/month') {
        $granularity = '';
        $output[] = '<ul><li class="active first">';
    }
    else
        $output[] = '<ul><li class="inactive first">';
    $output[] = l('Month', "calendar$suffix");
    if ($granularity == '/week')
        $output[] = '</li><li class="active">';
    else
        $output[] = '</li><li class="inactive">';
    $output[] = l('Week', "calendar/week$suffix");
    if ($granularity == '/day')
        $output[] = '</li><li class="active last">';
    else
        $output[] = '</li><li class="inactive last">';
    $output[] = l('Day', "calendar/day$suffix");

    $output[] = '</li></ul>';
    $output[] = '</div>';
    $output[] = '</div>';

    // build any actions available to the user
    $actions = array();

    if (user_access('create ebms_event content')) {
        // create event on shown day if possible
        $query = array();
        if ($granularity == '/day')
            $query['query']['date'] = $date;

        $actions[] = '<li>';
        $actions[] = l('Create Event', "node/add/ebms-event", $query);
        $actions[] = '</li>';
    }

    if (user_access('view all events')) {
        // preserve current 'all' state if possible
        $title = 'Show All Events';
        $suffix = '/all';

        if ($all) {
            $title = 'Show Own Events';
            $suffix = '';
        }

        if ($date) {
            $suffix .= "/$date";
        }

        $actions[] = '<li>';
        $actions[] = l($title, "calendar$granularity$suffix");
        $actions[] = '</li>';
    }
    
    if ($actions) {
        // build an action box
        $output[] = '<div class="nav-block">';

        $output[] = '<h3>Calendar Actions</h3>';

        $output[] = '<div class="actions">';
        $output[] = '<ul>';

        $output = array_merge($output, $actions);

        $output[] = '</ul>';
        $output[] = '</div>';

        $output[] = "<div class='footer'></div>";
        $output[] = '</div>';
    }

    // retrieve upcoming events view
    $view = views_get_view('event_calendar');
    if ($view) {

        $display = $view->preview('block_upcoming');

        $output[] = $display;
    }

    return implode('', $output);
}

/**
 * Calendar helper function for altering a Views query.
 * 
 * @param type $view  The view object about to be processed.
 * @param type $query An object describing the query.
 */
function _calendar_query_alter(&$view, &$query) {
    global $user;
    
    //If the user is UID 1 or an EBMS administrator, there is no filtering of
    //events
    if (user_access("administer ebms"))
        return;

    //TODO: If power users are viewing all items, then don't filter
    //if viewing all then just return.
    if (user_access("view all events")
        && in_array($view->current_display,
            array('month_all', 'week_all', 'day_all', 'ical_event_feed'))) {
        return;
    }

    //Initialize an OR conditional for selects
    $or_conditions = db_or();
    $nullConditions = db_and();

    //Boards:

    // Add table to query
    $boardsjoin = new views_join();
    $boardsjoin->construct("field_data_field_boards", $view->base_table,
        $view->base_field, 'entity_id',
        array(
        array('field' => 'entity_type', 'value' => $view->base_table),
        array('field' => 'deleted', 'value' => 0, 'numeric' => TRUE),
    ));
    $query->table_queue[$view->base_table . '__field_data_field_boards'] = array(
        'table' => 'field_data_field_boards',
        'num' => 1,
        'alias' => 'field_data_field_boards',
        'join' => $boardsjoin,
        'relationship' => 'node'
    );

    //Add where condition
    if($user->boards)
        $or_conditions->condition('field_data_field_boards.field_boards_value',
            $user->boards, 'IN');
    $nullConditions->isNull('field_data_field_boards.field_boards_value');

    //Subgroups:
    
    //Add table to query
    $subgroupsjoin = new views_join();
    $subgroupsjoin->construct("field_data_field_subgroups",
        $view->base_table, $view->base_field, 'entity_id',
        array(
        array('field' => 'entity_type', 'value' => $view->base_table),
        array('field' => 'deleted', 'value' => 0, 'numeric' => TRUE),
    ));
    $query->table_queue[$view->base_table . '__field_data_field_subgroups'] = array(
        'table' => 'field_data_field_subgroups',
        'num' => 1,
        'alias' => 'field_data_field_subgroups',
        'join' => $subgroupsjoin,
        'relationship' => 'node'
    );

    //Add where condition
    if($user->subgroups)
        $or_conditions->condition('field_data_field_subgroups.field_subgroups_value',
            $user->subgroups, 'IN');
    $nullConditions->isNull('field_data_field_subgroups.field_subgroups_value');

    //Ad hoc groups:
    
    //Add table to query
    $adhocgroupsjoin = new views_join();
    $adhocgroupsjoin->construct("field_data_field_ad_hoc_groups",
        $view->base_table, $view->base_field, 'entity_id',
        array(
        array('field' => 'entity_type', 'value' => $view->base_table),
        array('field' => 'deleted', 'value' => 0, 'numeric' => TRUE),
    ));
    $query->table_queue[$view->base_table . '__field_data_field_ad_hoc_groups'] = array(
        'table' => 'field_data_field_ad_hoc_groups',
        'num' => 1,
        'alias' => 'field_data_field_ad_hoc_groups',
        'join' => $adhocgroupsjoin,
        'relationship' => 'node'
    );

    //Add where condition
    if($user->ebms_ad_hoc_groups)
        $or_conditions->condition('field_data_field_ad_hoc_groups.field_ad_hoc_groups_value',
            $user->ebms_ad_hoc_groups, 'IN');
    $nullConditions->isNull('field_data_field_ad_hoc_groups.field_ad_hoc_groups_value');

    //Individual Users:
    //Add table to query
    $userjoin = new views_join();
    $userjoin->construct("field_data_field_individuals", $view->base_table,
        $view->base_field, 'entity_id',
        array(
        array('field' => 'entity_type', 'value' => $view->base_table),
        array('field' => 'deleted', 'value' => 0, 'numeric' => TRUE),
    ));
    $query->table_queue[$view->base_table . '__field_data_field_individuals'] = array(
        'table' => 'field_data_field_individuals',
        'num' => 1,
        'alias' => 'field_data_field_individuals',
        'join' => $userjoin,
        'relationship' => 'node'
    );

    //Add where condition
    $or_conditions->condition('field_data_field_individuals.field_individuals_value',
        array($user->uid), 'IN'); //Note the use of array here.  IN must have an array
    $nullConditions->isNull('field_data_field_individuals.field_individuals_value');

    // or together both conditions (at least one filter match or all filters empty)
    $or_conditions->condition($nullConditions);
    
    $query->add_where('membership', $or_conditions);
}

/**
 * Callback for editing an event node from the calendar.
 * 
 * @param string $nid Node id of the event
 */
function ebms_calendar_edit_event($nid) {
    $node = node_load($nid);

    if (!$node || $node->type != 'ebms_event') {
        return MENU_NOT_FOUND;
    }

    $form = array();
    $leftNav = ebms_calendar_left_nav();
    $form['left_nav']['#markup'] = "<div id='left-nav'>$leftNav</div>";
    $form['event'] = drupal_get_form('ebms_calendar_event_form', $node);
    $form['event']['#prefix'] = "<div id='calendar-enclosure'>";
    $form['event']['#suffix'] = "</div>";

    return $form;
}

/**
 * Local implementation of hook_form_alter.  Adds a default date to the event's
 * date fields.
 * 
 * @param array $form       The existing form to alter.
 * @param array $form_state The form's state.
 * @return array $form      The updated form.
 */
function calendar_node_form_alter(&$form, &$form_state) {
    drupal_set_breadcrumb(
        array(
            l('Calendar', 'calendar'),
            'Create Event',
        )
    );

    $params = drupal_get_query_parameters();

    if (isset($params['date'])) {
        $date = $params['date'];

        $dateTime = "$date 12:00:00";

        $date = new DateTime($dateTime);
        $timestamp = $date->getTimestamp();

        $form['field_datespan']['und'][0]['#default_value']['value'] = $timestamp;
        $form['field_datespan']['und'][0]['#default_value']['value2'] = $timestamp;
    }

    $form['#validate'][] = 'ebms_calendar_event_validate';
    $form['actions']['submit']['#submit'][] = 'ebms_redirect_handler';

    $form['revision_information']['#access'] = false;
    $form['author']['#weight'] = 95;
    $form['options']['#weight'] = 90;
    $form['options']['promote']['#access'] = false;
    $form['options']['sticky']['#access'] = false;

    return $form;
}

function calendar_event_board_select_js($form, &$form_state) {
    $form_state['rebuild'] = true;

    return $form;
}