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
        ->fieldCondition('event_date', 'value2', time(), '>')
        ->range(0, 6)
        ->fieldOrderBy('event_date', 'value')
        ->entityOrderBy('entity_id');

    $result = $query->execute();

    if (!isset($result['node']))
        return array();

    $eventNids = array_keys($result['node']);
    $events = entity_load('node', $eventNids);
    $groupedResults = array();
    foreach ($events as $event) {
        $event_date = field_get_items('node', $event, 'event_date');
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

function ebms_calendar_left_nav($span = null) {
    $output = array();

    // create the links to month, week, and day views
    $output[] = '<div id="calendar-views">';
    $output[] = '<div class="item-list">';
    if ($span == 'month')
        $output[] = '<ul><li class="active first">';
    else
        $output[] = '<ul><li class="inactive first">';
    $output[] = l('Month', 'calendar');
    if ($span == 'week')
        $output[] = '</li><li class="active">';
    else
        $output[] = '</li><li class="inactive">';
    $output[] = l('Week', 'calendar/week');
    if ($span == 'day')
        $output[] = '</li><li class="active last">';
    else
        $output[] = '</li><li class="inactive last">';
    $output[] = l('Day', 'calendar/day');
    $output[] = '</li></ul>';
    $output[] = '</div>';
    $output[] = '</div>';

    $output[] = '<div id="upcoming-events" class="more-actions">';
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

            $event_date = field_get_items('node', $event, 'event_date');
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

            $event_type = field_get_items('node', $event, 'event_date');
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
    $imagesDir = Ebms\IMAGES_DIR;
    $output[] = "<img class='ebms-left-nav-block-bottom 'alt='' src='$imagesDir/left-nav-box-bottom.png'>";
    $output[] = '</div>';

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

    // if the user is a board member...
    if (in_array('board member', $user->roles)) {
        // add a condition to the query to ensure the event's board is in the 
        // member's set of boards
        $query->where[1]['conditions'][] = array(
            'field' => 'field_data_board.board_value',
            'value' => $user->ebms_boards,
            'operator' => 'IN',
        );
    }
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

function calendar_node_form_alter(&$form, &$form_state) {
    require_once('EbmsArticle.inc');
    require_once('common.inc');

    $params = drupal_get_query_parameters();

    if (isset($params['date'])) {
        $date = $params['date'];

        $dateTime = "$date 12:00:00";

        $date = new DateTime($dateTime);
        $timestamp = $date->getTimestamp();

        $form['event_date']['und'][0]['#default_value']['value'] = $timestamp;
        $form['event_date']['und'][0]['#default_value']['value2'] = $timestamp;
    }

    drupal_add_css(Ebms\CSS_DIR . '/calendar.css');

    $node = $form_state['node'];

    $form['#prefix'] = "<div id='ebms-event-form-wrapper'>" . (isset($form['#prefix'])
            ? $form['#prefix'] : '');
    $form['#suffix'] = "</div>";

    $boards = Ebms\Util::boards();

    $board = field_get_items('node', $node, 'board');
    if (!empty($board))
        $board = $board[0]['value'];
    if (isset($form_state['values']['board']))
        $board = $form_state['values']['board']['und'][0]['value'];

    $form['board']['und']['#options'] += $boards;
    if ($board)
        $form['board']['und']['#default_value'] = array($board);
    $form['board']['und']['#ajax'] = array(
        'callback' => 'calendar_event_board_select_js',
        'wrapper' => 'ebms-event-form-wrapper',
    );

    $subgroup = field_get_items('node', $node, 'subgroup');
    if (!empty($subgroup))
        $subgroup = $subgroup[0]['value'];
    if (isset($form_state['values']['subgroup']))
        $subgroup = $form_state['values']['subgroup'];

    $query = db_select('ebms_subgroup', 's');
    if ($board) {
        $query->condition('s.board_id', $board);
    }

    $subgroupOptions = $query->fields('s', array('sg_id', 'sg_name'))
        ->execute()
        ->fetchAllKeyed();

    $form['subgroup']['und']['#options'] += $subgroupOptions;
    if ($subgroup)
        $form['subgroup']['und']['#default_value'] = array($subgroup);

    $adhoc = field_get_items('node', $node, 'adhoc_group');
    if ($adhoc)
        $adhoc = $adhoc[0]['value'];

    $adhocOptions = array();
    $query = db_select('ebms_ad_hoc_group', 'g');
    $adhocOptions += $query->fields('g', array('group_id', 'group_name'))
        ->execute()
        ->fetchAllKeyed();

    $form['adhoc_group']['und']['#options'] += $adhocOptions;
    if ($adhoc)
        $form['adhoc_group']['und']['#default_value'] = array($adhoc);

    $users = entity_load('user');

    $inhouse = array();
    $boardMembers = array();
    foreach ($users as $user) {
        if (in_array('medical librarian', $user->roles) ||
            in_array('board manager', $user->roles) ||
            in_array('site manager', $user->roles)) {
            $inhouse[$user->uid] = $user->name;
        }

        if ((in_array('board member', $user->roles) ||
            in_array('board manager', $user->roles)) &&
            ($board == 0 || in_array($board, $user->boards))) {
            $boardMembers[$user->uid] = $user->name;
        }
    }

    $inhouseChosen = field_get_items('node', $node, 'inhouse_staff');
    $inhouseDefaults = array();
    if ($inhouseChosen)
        foreach ($inhouseChosen as $chosen) {
            $inhouseDefaults[] = $chosen['value'];
        }

    $form['inhouse_staff']['und']['#options'] = $inhouse;
    $form['inhouse_staff']['und']['#multiple'] = true;
    if (!empty($inhouseDefaults))
        $form['inhouse_staff']['und']['#default_value'] = $inhouseDefaults;


    $chosenMembers = field_get_items('node', $node, 'board_members');
    $defaultMembers = array();
    if ($chosenMembers)
        foreach ($chosenMembers as $chosen) {
            $defaultMembers[] = $chosen['value'];
        }

    $form['board_members']['und']['#options'] = $boardMembers;
    $form['board_members']['und']['#multiple'] = true;
    if (!empty($inhouseDefaults))
        $form['board_members']['und']['#default_value'] = $defaultMembers;

    return $form;
}

function calendar_event_board_select_js($form, &$form_state) {
    $form_state['rebuild'] = true;

    return $form;
}