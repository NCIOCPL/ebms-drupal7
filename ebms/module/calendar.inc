<?php

// $Id$

/**
 * @file
 *
 * Implementation of EBMS Calendar page.
 */
function pdq_ebms_calendar() {
    Ebms\Menu::$active = 'Calendar';
    drupal_set_title('EBMS Events');
    return '<i>Stub for calendar page</i>';
}

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

    $span_parts = explode('_', $span);

    $all = sizeof($span_parts) > 1;
    $granularity = '/' . $span_parts[0];

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

    if (user_access('view all events')) {
        // build an action box
        $output[] = '<div class="nav-block">';

        $output[] = '<h3>Calendar Actions</h3>';

        $output[] = '<div class="actions">';
        $output[] = '<ul><li>';
        if ($all) {
            $output[] = l('Show Own Events', "calendar$granularity");
        } else {
            $output[] = l('Show All Events', "calendar$granularity/all");
        }
        $output[] = '</li></ul>';
        $output[] = '</div>';

        $output[] = "<div class='footer'></div>";
        $output[] = '</div>';
    }
    
    $imagesDir = Ebms\IMAGES_DIR;

    // retrieve upcoming events view
    $view = views_get_view('event_calendar');
    if ($view) {

        $display = $view->preview('block_upcoming');

        $output[] = $display;
    }


    /* $output[] = '<div id="upcoming-events" class="more-actions">';
      $output[] = '<div class="item-list">';
      $output[] = '<h3>Upcoming Events</h3>';
      $output[] = '<ul class="ebms_left_nav_block">';

      $groupedEvents = _calendar_get_upcoming_events();
      $first = true;
      foreach ($groupedEvents as $date => $events) {
      if ($first)
      $first = false;
      else
      $output[] = "<li class='last'><hr></li>";
      $output[] = "<li class='calendar-date first'>$date</li>";
      foreach ($events as $event) {
      $path = "node/$event->nid";

      $event_date = field_get_items('node', $event, 'field_datespan');
      if ($event_date === false)
      continue;

      $start = $event_date[0]['value'];
      $end = $event_date[0]['value2'];

      // render out separate pieces of the dates
      $startTime = ($start % 3600) ? date('g:i', $start) : date('g',
      $start);
      $endTime = ($end % 3600) ? date('g:i', $end) : date('g', $end);

      $startMeridiem = date('A', $end);
      $endMeridiem = date('A', $end);

      $endTime .= $endMeridiem;
      if ($startMeridiem != $endMeridiem) {
      $startTime .= $startMeridiem;
      }

      $timeLink = "$startTime - $endTime E.T.";
      $output[] = "<li class='last'>$timeLink</li>";

      $titleLink = l($event->title, $path);
      $output[] = "<li class='first last'>$titleLink</li>";

      $event_type = field_get_items('node', $event, 'field_event_type');
      if ($event_type === false)
      continue;

      $type = $event_type[0]['value'];
      $typeLabel = 'In Person';
      if ($type != 'in_person') {
      $typeLabel = 'WebEx/Phone Conf.';
      }

      $output[] = "<li class='first last'>$typeLabel</li>";
      }
      }



      $output[] = '</ul>';
      $output[] = '</div>';
      $output[] = "<img class='ebms-left-nav-block-bottom 'alt='' src='$imagesDir/left-nav-box-bottom.png'>";
      $output[] = '</div>'; */

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
            array('month_all', 'week_all', 'day_all'))) {
        return;
    }

    //Determine if we should show all board meetings or not
    $filter_boards = true;

    //If the user is an icrdb or board manager they are a power user
    //if we are looking at the month, week, or day calendar, then we
    //should not filter boards.
    if (user_access("view all events")
        && in_array($view->current_display, array('month', 'week', 'day'))) {
        $filter_boards = false;
    }

    //Initialize an OR conditional for selects
    $or_conditions = db_or();

    //Boards:
    if ($filter_boards && $user->ebms_boards) {
        //Add table to query    
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
        if (!$filter_boards) {
            $or_conditions->condition('field_data_field_boards.field_boards_value',
                '', 'IS NOT NULL');
        } else {
            $or_conditions->condition('field_data_field_boards.field_boards_value',
                $user->ebms_boards, 'IN');
        }
    }

    //Subgroups:
    if ($user->ebms_subgroups) {
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
        $or_conditions->condition('field_data_field_subgroups.field_subgroups_value',
            $user->ebms_subgroups, 'IN');
    }
    //Ad hoc groups:
    if ($user->ebms_ad_hoc_groups) {
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
        $or_conditions->condition('field_data_field_ad_hoc_groups.field_ad_hoc_groups_value',
            $user->ebms_ad_hoc_groups, 'IN');
    }

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


    $query->add_where(
        'membership', $or_conditions
    );
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

    return $form;
}

function calendar_event_board_select_js($form, &$form_state) {
    $form_state['rebuild'] = true;

    return $form;
}