<?php

// $Id$

/**
 * @file
 *
 * Implementation of EBMS page for general site information.
 */
function pdq_ebms_citations() {
    Ebms\Menu::$active = 'Citation Management';
    drupal_set_title('Citation Management');
    return '<i>Stub for \'Citations\' page</i>';
}
