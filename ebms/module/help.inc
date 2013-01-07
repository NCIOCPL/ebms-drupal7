<?php

// $Id:$

/**
 * @file
 *
 * EBMS online help.
 */

/**
 * This is a stub until Ashleigh finishes the stories for user documentation.
 */
function pdq_ebms_help() {

    // Override Drupal's default breadcrumbs to match requirements.
    drupal_set_breadcrumb(
        array(
            l('Home', 'home'),
            'Help',
        )
    );

    return array(
        '#type' => 'markup',
        '#markup' =>
        '<h2>Online User Documentation</h2>' .
        '<p>The EBMS user documentation is being written, and will be ' .
        'available soon.</p>',
    );
}
