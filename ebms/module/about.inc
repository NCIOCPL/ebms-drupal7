<?php

// $Id$

/**
 * @file
 *
 * Implementation of EBMS page for general site information.
 */
function pdq_ebms_about() {
    Ebms\Menu::$active = 'About';
    drupal_set_title('About EBMS');
    return '<i>Stub for \'about\' page</i>';
}
