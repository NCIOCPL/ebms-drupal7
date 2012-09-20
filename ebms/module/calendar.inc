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
