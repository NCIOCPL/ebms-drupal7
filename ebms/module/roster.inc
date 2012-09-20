<?php

// $Id$

/**
 * @file
 *
 * Generates page of links to cancer.gov PDQ board roster pages.
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
