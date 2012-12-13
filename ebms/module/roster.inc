<?php

// $Id$

/**
 * @file
 *
 * Generates page of links to cancer.gov PDQ board roster pages.
 * Scaled back from an original requirement, which would have had us
 * storing board roster information in the EBMS tables.  Instead
 * (partly as result of Personally Identifiable Information (PII)
 * concerns) we now just link out to existing pages on cancer.gov.
 *
 * Later development: Roster tab has been dropped from the main menu.
 * So basically this file is now obsolete.
 */
function pdq_ebms_roster() {
    drupal_set_title('PDQ Board Rosters');
    $boards = array(
        'adult-treatment' => 'Adult Treatment',
        'pediatric-treatment' => 'Pediatric Treatment',
        'supportive-care' => 'Supportive and Palliative Care',
        'screening-prevention' => 'Screening and Prevention',
        'cancer-genetics' => 'Cancer Genetics',
        'cancer-cam' => 'Cancer Complementary and Alternative Medicine',
    );
    foreach ($boards as $link => $name)
        $links[$link] = array(
            'title' => $name,
            'href' => "http://www.cancer.gov/cancertopics/pdq/$link-board",
            'attributes' => array('target' => '_blank'),
        );
    return array('#theme' => 'links', '#links' => $links);
}
