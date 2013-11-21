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
    $instructions = 'Please refer to the documents posted below for ' .
        'step-by-step instructions on how to perform various tasks ' .
        'in the EBMS. If you are unable to find the answer to your ' .
        'question using the resources below, please contact your PDQ ' .
        'Board Manager for assistance.';
    $query = db_select('ebms_doc', 'd')
        ->fields('d', array('description'))
        ->condition('d.drop_flag', 0);
    $query->join('file_managed', 'f', 'f.fid = d.file_id');
    $query->fields('f', array('uri'));
    $query->join('ebms_doc_tag', 'dt', 'dt.doc_id = d.doc_id');
    $query->join('ebms_tag', 't', 't.tag_id = dt.tag_id');
    $query->condition('t.tag_name', 'help');
    $query->orderBy('d.description');
    $target = array('attributes' => array('target' => '_blank'));
    $list = array();
    $docs = $query->execute();
    foreach ($docs as $doc) {
        $url = file_create_url($doc->uri);
        $list[] = l($doc->description, $url, $target);
    }
    return array(
        'title' => array(
            '#type' => 'markup',
            '#markup' => '<h2>Online User Documentation</h2>',
        ),
        'instructions' => array(
            '#markup' => "<p>$instructions</p>",
        ),
        'documents' => array(
            '#theme' => 'item_list',
            '#items' => $list,
            '#title' => 'EBMS Help Documents',
        ),
    );
}
