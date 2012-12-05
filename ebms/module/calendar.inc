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

function _calendar_left_nav($span = null) {
    // create the links to month, week, and day views
    print '<div id="calendar-views">';
    print '<div class="item-list">';
    if ($span == 'month')
        print '<ul><li class="active first">';
    else
        print '<ul><li class="inactive first">';
    print l('Month', 'calendar');
    if ($span == 'week')
        print '</li><li class="active">';
    else
        print '</li><li class="inactive">';
    print l('Week', 'calendar/week');
    if ($span == 'day')
        print '</li><li class="active last">';
    else
        print '</li><li class="inactive last">';
    print l('Day', 'calendar/day');
    print '</li></ul>';
    print '</div>';
    print '</div>';

    print '<div id="upcoming-events" class="more-actions">';
    print '<div class="item-list">';
    print '<h3>Upcoming Events</h3>';
    print '<ul class="ebms_left_nav_block">';

    $groupedEvents = _calendar_get_upcoming_events();
    $first = true;
    foreach ($groupedEvents as $date => $events) {
        if ($first)
            $first = false;
        else
            print "<li class='last'><hr></li>";
        print "<li class='calendar-date first'>$date</li>";
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
            print "<li class='last'>$timeLink</li>";

            $titleLink = l($event->title, $path);
            print "<li class='first last'>$titleLink</li>";

            $event_type = field_get_items('node', $event, 'event_date');
            if ($event_type === false)
                continue;

            $type = $event_type[0]['value'];
            $typeLabel = 'In Person';
            if ($type != 'in_person') {
                $typeLabel = 'WebEx/Phone Conf.';
            }

            print "<li class='first last'>$typeLabel</li>";
        }
    }

    print '</ul>';
    print '</div>';
    print '<img class="ebms-left-nav-block-bottom" alt="" src="/sites/ebms.cancer.gov/themes/ebmstheme/images/left-nav-box-bottom.png">';
    print '</div>';
}

/**
 * Calendar helper function for altering a Views query.
 * 
 * @param type $view  The view object about to be processed.
 * @param type $query An object describing the query.
 */
function _calendar_query_alter(&$view, &$query) {
    global $user;

    // if the user is a board memeber...
    if (in_array('board member', $user->roles)) {
        //dsm($user, 'user');
        // add a condition to the query to ensure the event's board is in the 
        // member's set of boards
        $query->where[1]['conditions'][] = array(
            'field' => 'field_data_board.board_value',
            'value' => $user->ebms_boards,
            'operator' => 'IN',
        );

        //dsm($query, 'calendar view query');
    }
}