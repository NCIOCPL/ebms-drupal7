<?php

// $Id$

/**
 * @file
 *
 * Implementation of EBMS pages for importing journal articles
 */

require_once('EbmsImport.inc');

/**
 * Callback registered for /citations pages.
 */
function pdq_ebms_import($job_id=null) {
    Ebms\Menu::$active = 'Citation Management';
    $control = new EbmsImport($job_id);
    return $control->run();
}

class EbmsImport {

    public static $url = 'citations/import';
    public
        $job_id;
    public function __construct($job_id) {
        $this->job_id = $job_id;
    }
    public function run() {
        Ebms\Util::add_js();
        drupal_add_css(Ebms\CSS_DIR . '/import.css');
        drupal_set_breadcrumb(
            array(
                l('Citation Management', 'citations'),
                'Import Citations',
            )
        );
        $left_nav = Ebms\Util::build_left_nav('Import Citations');
        $left_nav['more actions'] = $this->more_actions();
        return array(
            'left-nav' => $left_nav,
            'import form' => drupal_get_form(
                'pdq_ebms_import_form',
                $this->job_id
            ),
        );
    }

    function more_actions() {
        $items = array(
            l('View Past Imports', 'reports/import'),
        );
        $url = Ebms\IMAGES_DIR . '/left-nav-box-bottom.png';
        $img = "<img class='ebms-left-nav-block-bottom' src='$url' alt='' />";
        return array(
            '#theme' => 'item_list',
            '#title' => 'Import Actions',
            '#items' => $items,
            '#attributes' => array('class' => 'ebms_left_nav_block'),
            '#prefix' => '<div class="more-actions">',
            #. $class . '" id="' . $id . '">',
            #'#suffix' => $this->block_bottom_img() . '</div>',
            '#suffix' => $img . '</div>',
        );
    }
    /*
            '#type' => 'container',
            '#attributes' => array('id' => 'more-actions'),
            'title' => array(
                '#type' => 'markup',
                '#markup' => '<h3>Import Actions</h3>',
            ),
*/
    public static function fetch_search_spec($search_id) {
        $json = db_select('ebms_search', 's')
            ->fields('s', array('search_spec'))
            ->condition('s.search_id', $search_id)
            ->execute()
            ->fetchField();
        return drupal_json_decode($json);
    }
}

function pdq_ebms_import_form($form, &$form_state, $job_id) {
    #pdq_ebms_debug("IMPORT FORM", array($form, $form_state));
    $boards = array(0 => ' ') + Ebms\Util::boards();
    $topics = array(0 => ' ');
    if (isset($form_state['values'])) {
        #pdq_ebms_debug('IMPORT VALUES', array($form_state['values'], $boards));
        if (isset($form_state['values']['board'])) {
            if ($form_state['values']['board']) {
                $results = db_select('ebms_topic', 't')
                    ->fields('t', array('topic_id', 'topic_name'))
                    ->condition('t.board_id', $form_state['values']['board'])
                    ->condition('t.active_status', 'A')
                    ->orderBy('t.topic_name')
                    ->execute();
                foreach ($results as $result)
                    $topics[$result->topic_id] = $result->topic_name;
            }
        }
    }
    $results = db_select('ebms_cycle', 'c')
        ->fields('c', array('cycle_id', 'cycle_name'))
        ->orderBy('c.start_date', 'desc')
        ->execute();
    $review_cycles = array(0 => ' ');
    foreach ($results as $result)
        $review_cycles[$result->cycle_id] = $result->cycle_name;
    $results = db_select('ebms_article_tag_type', 't')
        ->fields('t', array('text_id', 'tag_name'))
        ->orderBy('t.tag_name')
        ->execute();
    $tags = array();
    foreach ($results as $result)
        $tags[$result->text_id] = $result->tag_name;
    $form = array(
        '#id' => 'import-form',
        'page title' => array(
            '#type' => 'markup',
            '#markup' => '<h2>Import Citations</h2>',
        ),
        'board' => array(
            '#type' => 'select',
            '#title' => 'EDITORIAL BOARD',
            '#options' => $boards,
            '#ajax' => array(
                'callback' => 'pdq_ebms_import_board_ajax',
                'wrapper' => 'topic',
            ),
        ),
        'topic' => array(
            '#type' => 'select',
            '#title' => 'SUMMARY TOPIC',
            '#options' => $topics,
            '#prefix' => '<div id="topic">',
            '#suffix' => '</div>',
        ),
        'cycle' => array(
            '#type' => 'select',
            '#title' => 'REVIEW CYCLE',
            '#options' => $review_cycles,
        ),
        'tags' => array(
            '#type' => 'select',
            '#multiple' => 'true',
            '#title' => 'CITATION TAGS',
            '#options' => $tags,
        ),
        'pmid' => array(
            '#type' => 'textfield',
            '#title' => 'PUB MED IDS',
            '#description' => 'Enter space- or comma-delimited Pubmed IDs ' .
            'or choose a file containing a Pubmed Print format search ' .
            'results file below',
        ),
        'comments' => array(
            '#type' => 'textfield',
            '#title' => 'COMMENTS',
        ),
        'options' => array(
            '#type' => 'checkboxes',
            '#title' => 'OPTIONS',
            '#options' => array(
                'not-list' => 'NOT LIST',
                'test-mode' => 'TEST MODE',
                'fast-track' => 'FAST TRACK',
            ),
        ),
        'placement' => array(
            '#type' => 'select',
            '#title' => 'PLACEMENT LEVEL',
            '#options' => array(
                '' => ' ',
                'PassedBMReview' => 'Passed Board Manager',
                'PassedFullReview' => 'Passed full text review',
                'OnAgenda' => 'On agenda',
            ),
            '#states' => array(
                'visible' => array(
                    '#edit-options-fast-track' => array(
                        'checked' => true
                    ),
                ),
            ),
        ),
        'ft-comments' => array(
            '#type' => 'textfield',
            '#title' => 'FAST TRACK COMMENTS',
            '#states' => array(
                'visible' => array(
                    '#edit-options-fast-track' => array(
                        'checked' => true
                    ),
                ),
            ),
        ),
        'file' => array(
            '#type' => 'file',
        ),
        // Tried to use a hidden field to trigger visibility of cancel
        // and submit buttons, but Drupal's #states don't work on hidden
        // fields.  I was able to confirm that the Javascript I hooked in
        // set the hidden field's value to 'y' when a file was selected,
        // but Drupal ignored the new state.
        //'file-selected' => array(
        //    '#type' => 'hidden',
        //    '#default_value' => 'n',
        //),
        'cancel' => array(
            '#type' => 'button',
            '#value' => 'CANCEL',
            //'#states' => array(
            //    'visible' => array(
            //        ':input[name="file-selected"]' => array('value' => 'y'),
            //    ),
            //),
        ),
        'submit' => array(
            '#type' => 'submit',
            '#value' => 'SUBMIT',
            //'#states' => array(
            //    'visible' => array(
            //        ':input[name="file-selected"]' => array('value' => 'y'),
            //    ),
            //),
        ),
    );
    if ($job_id) {
        $batch = db_select('ebms_temp_report', 'r')
            ->fields('r', array('report_data'))
            ->condition('r.report_id', $job_id)
            ->execute()
            ->fetchField();
        pdq_ebms_debug('ENCODED BATCH IN FORM', $batch);
        $batch = drupal_json_decode($batch);
        pdq_ebms_debug('DECODED BATCH IN FORM', $batch);
        $batch = (object) $batch;
        pdq_ebms_debug('BATCH AS OBJECT', $batch);
        $form['report'] = Ebms\Util::import_report($batch, 'Statistics');
    }
    return $form;
}

function pdq_ebms_import_board_ajax($form, $form_state) {
    return $form['topic'];
}

function pdq_ebms_import_form_validate($form, &$form_state) {
    if (isset($form_state['values']['op'])) {
        if ($form_state['values']['op'] == 'CANCEL') {
            drupal_goto(EbmsImport::$url);
            return;
        }
    }
    if (!$form_state['values']['board'])
        form_set_error('board', 'You must select a board for article import');
    if (!$form_state['values']['topic'])
        form_set_error('topic', 'You must select a topic for article import');
    if (!$form_state['values']['cycle'])
        form_set_error('cycle', 'You must select a cycle for article import');
    if ($form_state['values']['options']['fast-track']) {
        if (!$form_state['values']['placement'])
            form_set_error('placement',
                'A placement selection is required for fast-track imports');
    }
    $pmids = array();
    $validators = array('file_validate_extensions' => array());
    $file = file_save_upload('file', $validators);
    if ($file) {
        if ($form_state['values']['pmid'])
            form_set_error('pmid', 'You cannot supply both Pubmed IDs and ' .
                'a Pubmed Print file in the same request');
        else
            $pmids = Ebms\findPubmedIDs($file->fid, null, null);
    }
    elseif ($form_state['values']['pmid']) {
        $pmids = preg_split('/[\s,]/', $form_state['values']['pmid'], null,
                 PREG_SPLIT_NO_EMPTY);
    }
    if (count($pmids) < 1)
        form_set_error('pmid', 'You must either specify Pubmed IDs or ' .
            'upload a Pubmed search results file');
    $form_state['article-ids'] = $pmids;
    #pdq_ebms_debug("ARTICLE IDS", $pmids);
    #pdq_ebms_debug('FORM VALIDATION', $form_state['values']);
}

function pdq_ebms_import_form_submit($form, &$form_state) {
    #pdq_ebms_debug('SUBMIT', $form_state);
    if ($form_state['values']['op'] == 'CANCEL') {
        drupal_goto(EbmsImport::$url);
        return;
    }
    $pmids = $form_state['article-ids'];
    $values =& $form_state['values'];
    pdq_ebms_debug('REFERENCE TO IMPORT SUBMIT VALUES', $values);
    $board = $values['board'];
    $topic = $values['topic'];
    $cycle = $values['cycle'];
    $comments = $values['comments'];
    $use_not_list = $values['options']['not-list'] ? false : true;
    $fast_track = $values['options']['fast-track'] ? true : false;
    $test = $values['options']['test-mode'] ? true : false;
    # $test = true; // for debugging ...
    $mode = $test ? 'test' : 'live';
    $import_type = $fast_track ? 'F' : 'R';
    $batch = Ebms\importArticlesFromNLM($mode, $pmids, $topic, $cycle,
             $comments, $use_not_list, $import_type);
    pdq_ebms_debug('IMPORT BATCH', $batch);
    if (!$test) {
        $article_ids = array();
        foreach ($batch->action['reviewReady'] as $info)
            $article_ids[] = $info[1];
        if ($fast_track) {
            $cmt = $values['ft-comments'];
            $state = $values['placement'];
            foreach ($article_ids as $id)
                Ebms\setArticleState($id, $state, $topic, $board, $cmt);
        }
        foreach ($values['tags'] as $tag_id => $tag_val) {
            if ($tag_val) {
                foreach ($article_ids as $article_id)
                    Ebms\addArticleTag($tag_id, $article_id, $topic);
            }
        }
    }
    $report_data = drupal_json_encode($batch);
    $report_id = db_insert('ebms_temp_report')
        ->fields(array('report_data' => $report_data))
        ->execute();
    $url = EbmsImport::$url . '/' . $report_id;
    drupal_goto($url);
}
