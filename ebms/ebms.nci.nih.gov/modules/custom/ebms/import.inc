<?php

// $Id$

/**
 * @file
 *
 * Implementation of EBMS pages for importing journal articles.  Processing
 * flow for this story goes like this (main path, ignoring validation or
 * other errors, or the cancel button):
 *
 *    1. user navigates to citations/import (without a job ID)
 *    2. the import request form is drawn
 *    3. the user completes and submits the form
 *    4. the articles are imported from NLM
 *    5. the import API returns report information about the job
 *    6. this information is stored in a database table
 *    7. the ID for this table row is used as the job ID
 *    8. the software reroutes the user to citations/import/<job-id>
 *    9. a fresh import form is drawn, with the report information
 *       for the completed job displayed below the form
 */

require_once('EbmsImport.inc');

/**
 * Callback registered for /citations/import pages.
 */
function pdq_ebms_import($job_id=null) {
    Ebms\Menu::$active = 'Citation Management';
    $control = new EbmsImport($job_id);
    return $control->run();
}

/**
 * Wrapper for logic to handle the article import pages.  Keeps our
 * functions out of the global namespace with less ugly syntax than the
 * PHP namespace mechanism provides, and lets us pass around the current
 * state information more easily.
 */
class EbmsImport {

    // Class variables.
    public static $url = 'citations/import';

    // Instance data.
    public
        $job_id;

    /**
     * Construct the control processing object for the import page.  If
     * $job_id is not null, we just processed an import request, for
     * which we will display statistics below the import request form.
     */
    public function __construct($job_id) {
        $this->job_id = $job_id;
    }

    /**
     * Draw the page, including the left navigation, import request
     * form, and (possbily) statistics from an import job (if we
     * just processed such a job).
     */
    public function run() {
        Ebms\Util::add_js();
        drupal_add_js(Ebms\JS_DIR . '/import.js', 'file');
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

    /**
     * Add a box with additional links under the main left nav links.
     * Right now there's only one item in this box.
     */
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
            '#suffix' => $img . '</div>',
        );
    }
}

/**
 * Callback to build the import request form.  The $request_id parameter will
 * be non-null if we just processed an import request, in which case we'll
 * display statistics from that request's job.
 */
function pdq_ebms_import_form($form, &$form_state, $request_id) {

    // Start off with defaults appropriate for a new form.
    // Replace tags with single "special search" checkbox (TIR 2422).
    $board = $topic = $cycle = $not_list = $test_mode = $fast_track =
        $special_search = $disposition = $past_meeting = $future_meeting = 0;
    //$selected_tags = array();
    $pmid = $comments = $placement = $ft_comments = '';

    // If this is a followup form to a previous import request, use
    // the values submitted for that request to seed the form (TIR 2194).
    if ($request_id) {
        $result = db_select('ebms_import_request', 'r')
            ->fields('r', array('request_params', 'report_data'))
            ->condition('r.request_id', $request_id)
            ->execute()
            ->fetchObject();
        $report = $result->report_data;
        $params = drupal_json_decode($result->request_params);
        $board = $params['board'];
        $topic = $params['topic'];
        $cycle = $params['cycle'];
        //$selected_tags = $params['tags'];
        $special_search = $params['special-search'];
        $pmid = $params['pmid'];
        $comments = $params['comments'];
        $not_list = $params['not-list'];
        $test_mode = $params['test-mode'];
        $fast_track = $params['fast-track'];
        $placement = $params['placement'];
        $ft_comments = $params['ft-comments'];
        $disposition = $params['disposition'];
    }

    // An ajax call can override the board selection.
    if (isset($form_state['values']))
        $board = $form_state['values']['board'];

    // Build the picklist value sets.
    $boards = array(0 => ' ') + Ebms\Util::boards();
    $topics = array(0 => ' ');
    $past_meetings = array(0 => ' ');
    $future_meetings = array(0 => ' ');
    if ($board) {
        $results = db_select('ebms_topic', 't')
            ->fields('t', array('topic_id', 'topic_name'))
            ->condition('t.board_id', $board)
            ->condition('t.active_status', 'A')
            ->orderBy('t.topic_name')
            ->execute();
        foreach ($results as $result)
            $topics[$result->topic_id] = $result->topic_name;
    }
    $results = db_select('ebms_cycle', 'c')
        ->fields('c', array('cycle_id', 'cycle_name'))
        ->orderBy('c.start_date', 'DESC')
        ->execute();
    $review_cycles = array(0 => ' ');
    foreach ($results as $result)
        $review_cycles[$result->cycle_id] = $result->cycle_name;
    //$results = db_select('ebms_article_tag_type', 't')
    //    ->fields('t', array('text_id', 'tag_name'))
    //    ->orderBy('t.tag_name')
    //    ->execute();
    //$tags = array();
    //foreach ($results as $result)
    //    $tags[$result->text_id] = $result->tag_name;
    $results = db_select('ebms_article_board_decision_value', 'v')
        ->fields('v', array('value_id', 'value_name'))
        ->orderBy('v.value_name')
        ->execute();
    $dispositions = array(0 => ' ');
    foreach ($results as $result)
        $dispositions[$result->value_id] = $result->value_name;

    if ($disposition == 'OnAgenda') {
        $now = time();
        $past_meetings = pdq_ebms_agenda_meeting_picklist(true, $now, $board);
        $future_meetings = pdq_ebms_agenda_meeting_picklist(false, $now, $board);
    }
    if (!array_key_exists($topic, $topics))
        $topic = 0;
    if (!array_key_exists($past_meeting, $past_meetings))
        $past_meeting = 0;
    if (!array_key_exists($future_meeting, $future_meetings))
        $future_meeting = 0;
    $selected_members = array();
    $members = array();

    // OCEEBMS-58: disabled (for now) by user decision.
    if (false && $board) {
        $query = db_select('users', 'u');
        $query->fields('u', array('uid', 'name'));
        $query->join('ebms_board_member', 'm', 'm.user_id = u.uid');
        $query->join('users_roles', 'ur', 'ur.uid = u.uid');
        $query->join('role', 'r', 'r.rid = ur.rid');
        $query->condition('m.board_id', $board);
        $query->condition('r.name', 'board member');
        $query->condition('u.status', 1);
        $results = $query->execute();
        foreach ($results as $result)
            $members[$result->uid] = $result->name;
    }

    // Assemble the form's render array.
    $form = array(
        '#id' => 'import-form',
        'page title' => array(
            '#type' => 'markup',
            '#markup' => '<h2>Import Citations</h2>',
        ),
        'board' => array(
            '#type' => 'select',
            '#title' => 'EDITORIAL BOARD *',
            '#options' => $boards,
            '#default_value' => $board,
            '#ajax' => array(
                'callback' => 'pdq_ebms_import_board_ajax',
                'wrapper' => 'board-controlled', #'topic',
            ),
        ),
        'board-controlled' => array(
            '#type' => 'container',
            '#attributes' => array('id' => 'board-controlled'),
            'topic' => array(
                '#type' => 'select',
                '#title' => 'SUMMARY TOPIC *',
                '#options' => $topics,
                '#default_value' => $topic,
                '#prefix' => '<div id="topic">',
                '#suffix' => '</div>',
            ),
            'cycle' => array(
                '#type' => 'select',
                '#title' => 'REVIEW CYCLE *',
                '#options' => $review_cycles,
                '#default_value' => $cycle,
            ),
            'pmid' => array(
                '#type' => 'textfield',
                '#default_value' => $pmid,
                '#title' => 'PUB MED IDS',
            ),
            'comments' => array(
                '#type' => 'textfield',
                '#default_value' => $comments,
                '#title' => 'COMMENTS',
            ),
            'not-list' => array(
                '#type' => 'checkbox',
                '#title' => 'NOT LIST',
                '#prefix' => '<div class="checkbox-line">',
                '#suffix' => '</div>',
                '#default_value' => $not_list,
            ),
            'test-mode' => array(
                '#type' => 'checkbox',
                '#title' => 'TEST MODE',
                '#prefix' => '<div class="checkbox-line">',
                '#suffix' => '</div>',
                '#default_value' => $test_mode,
            ),
            'fast-track' => array(
                '#type' => 'checkbox',
                '#title' => 'FAST TRACK',
                '#prefix' => '<div class="checkbox-line">',
                '#suffix' => '</div>',
                '#default_value' => $fast_track,
            ),
            'special-search' => array(
                '#type' => 'checkbox',
                '#title' => 'SPECIAL SEARCH',
                '#prefix' => '<div class="checkbox-line">',
                '#suffix' => '</div>',
                '#default_value' => $special_search,
            ),

            // XXX I wasn't given a list of states to include here, so I picked
            //     the ones which seemed most appropriate.  Users may wish to
            //     have others added before we go into production.
            // 2012-12-26 (Bob): Robin has told us which state are to be used;
            //                   see TIR 2206.
            // 2013-11-13 (Bob): Things have become much more complicated.
            //                   more states, with lots more magic fields
            //                   popping in and out dynamically.  See
            //                   JIRA ticket OCEEBMS-58 (cloned from TIR
            //                   2437).
            'placement' => array(
                '#type' => 'select',
                '#title' => 'PLACEMENT LEVEL *',
                '#options' => array(
                    '' => ' ',
                    'PassedBMReview' => 'Passed abstract review',
                    'PassedFullReview' => 'Passed full text review',
                    'OnAgenda' => 'On Agenda',
                    'FinalBoardDecision' => 'Editorial Board decision',
                ),
                '#default_value' => $placement,
                '#states' => array(
                    'visible' => array(
                        '#edit-fast-track' => array(
                            'checked' => true
                        ),
                    ),
                ),
            ),
            'disposition' => array(
                '#type' => 'select',
                '#title' => 'DISPOSITION *',
                '#options' => $dispositions,
                '#default_value' => $disposition,
                '#states' => array(
                    'visible' => array(
                        ':input[name="placement"]' => array(
                            'value' => 'FinalBoardDecision',
                        ),
                    ),
                ),
            ),
            'agenda-meeting' => array(
                '#type' => 'container',
                '#attributes' => array('id' => 'meeting-fields'),
                'future-meeting' => array(
                    '#type' => 'select',
                    '#title' => 'FUTURE MEETING',
                    '#options' => $future_meetings,
                    '#default_value' => $future_meeting,
                    '#states' => array(
                        'visible' => array(
                            ':input[name="placement"]' => array(
                                'value' => 'OnAgenda',
                            ),
                        ),
                    ),
                ),
                'past-meeting' => array(
                    '#type' => 'select',
                    '#title' => 'PAST MEETING',
                    '#options' => $past_meetings,
                    '#default_value' => $past_meeting,
                    '#states' => array(
                        'visible' => array(
                            ':input[name="placement"]' => array(
                                'value' => 'OnAgenda',
                            ),
                        ),
                    ),
                ),
            ),
            'members' => array(
                '#type' => 'checkboxes',
                '#title' => 'BOARD MEMBERS',
                '#options' => $members,
                '#default_value' => $selected_members,
                '#states' => array(
                    'visible' => array(
                        ':input[name="placement"]' => array(
                            'value' => 'XXXOnAgenda', // Disabled for now
                        ),
                    ),
                ),
            ),
        ),
        'ft-comments' => array(
            '#type' => 'textfield',
            '#default_value' => $ft_comments,
            '#title' => 'FAST TRACK COMMENTS',
            '#states' => array(
                'visible' => array(
                    '#edit-fast-track' => array(
                        'checked' => true
                    ),
                ),
            ),
        ),
        'file' => array(
            '#title' => 'File',
            '#title_display' => 'invisible',
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

    // If we just handled an import request, show the statistics from that job.
    if ($request_id) {
        $batch = drupal_json_decode($report);
        $batch = (object) $batch;
        $form['report'] = Ebms\Util::import_report($batch, 'Statistics');
    }
    return $form;
}

/**
 * Construct a picklist of past or future meetings.  Make the caller
 * pass in the value representing the current date/time.  This allows
 * the caller to calculate that value once, so the same moment in time
 * can be used to build both the past meeting list and the future meeting
 * list, avoiding the possibility that a meeting will omitted from
 * both lists or included in both lists, which could occur if the
 * meeting's start time were later than the value returned by time() on
 * the first call to this function, but earlier than the value for
 * the the call to time() on the second call of this function.
 * Also, although most of the logic for this function is modeled after
 * code for the "full citation" page in citations.inc, I'm not using
 * the meeting's ending date/time when collecting past meetings, as
 * that code does, because it could result in the same meeting appearing
 * on both lists, possibly leading the user to wonder whether there
 * were two different meetings with the same title in the system.
 */
function pdq_ebms_agenda_meeting_picklist($past, $now, $board=null) {
    if ($past) {
        $op = '<=';
        $ord = 'ASC';
    }
    else {
        $op = '>';
        $ord = 'DESC';
    }
    $query = new EntityFieldQuery();
    $query->entityCondition('entity_type', 'node')
        ->entityCondition('bundle', 'ebms_event')
        ->propertyCondition('status', '1')
        ->fieldCondition('field_datespan', 'value', $now, $op)
        ->fieldOrderBy('field_datespan', 'value', $ord)
        ->entityOrderBy('entity_id', $ord);
    // If the users change their minds and want only meetings for the
    // selected board, this is how we would do it (without "false &").
    if (false && $board)
        $query->fieldCondition('field_boards', 'value', $board, '=');
    $result = $query->execute();
    $meetings = array(0 => '');
    if (isset($result['node'])) {
        foreach ($result['node'] as $node) {
            $fullNode = node_load($node->nid);
            $nodeWrapper = entity_metadata_wrapper('node', $fullNode);
            $datespan = $nodeWrapper->field_datespan->value();
            $suffix = '';
            if($datespan) {
                $date = format_date($datespan['value'], 'custom', 'Y-m-d');
                $suffix = " - $date";
            }
            $meetings[$node->nid] = $fullNode->title . $suffix;
        }
    }
    return $meetings;
}

/**
 * Callback to populate the topic dropdown based on which board has been
 * selected.
 */
function pdq_ebms_import_board_ajax($form, $form_state) {
    return $form['board-controlled'];
}

/**
 * Make sure the required fields are all present.  The error messages
 * explain what the rules are, so i won't repeat them here.
 */
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
    if ($form_state['values']['fast-track']) {
        if (!$form_state['values']['placement'])
            form_set_error('placement',
                'A placement selection is required for fast-track imports');
        elseif ($form_state['values']['placement'] == 'FinalBoardDecision') {
            if (!$form_state['values']['disposition'])
                form_set_error('disposition',
                    'A disposition must be selected for Editorial Board ' .
                    'decisions.');
        }
        elseif ($form_state['values']['placement'] == 'OnAgenda') {
            $future_meeting = $form_state['values']['future-meeting'];
            $past_meeting = $form_state['values']['past-meeting'];
            if ($past_meeting && $future_meeting ||
                    !$past_meeting && !$future_meeting)
                form_set_error('future-meeting',
                    'Exactly one meeting must be selected for ' .
                    'On Agenda state.');
        }
    }
    $pmids = array();
    $validators = array('file_validate_extensions' => array());
    $file = file_save_upload('file', $validators);
    $errors = 0;
    if ($file) {
        if ($form_state['values']['pmid'])
            form_set_error('pmid', 'You cannot supply both Pubmed IDs and ' .
                'a Pubmed Print file in the same request');
        else {
            try {
                $pmids = Ebms\findPubmedIDs($file->fid, null, null);
            }
            catch (\Exception $e) {
                form_set_error('file', Ebms\Util::full_stop($e->getMessage()));
                ++$errors;
            }
        }
    }
    elseif ($form_state['values']['pmid']) {
        $pmids = preg_split('/[\s,]/', $form_state['values']['pmid'], null,
                 PREG_SPLIT_NO_EMPTY);
    }
    if (!$errors && count($pmids) < 1)
        form_set_error('pmid', 'You must either specify Pubmed IDs or ' .
            'upload a Pubmed search results file');
    $form_state['article-ids'] = $pmids;
}

/**
 * Handle the Cancel and Submit buttons.
 */
function pdq_ebms_import_form_submit($form, &$form_state) {

    // To cancel, we just route the user to a fresh form.
    if ($form_state['values']['op'] == 'CANCEL') {
        drupal_goto(EbmsImport::$url);
        return;
    }

    // Extract the values entered by the user.
    $pmids = $form_state['article-ids'];
    $values =& $form_state['values'];
    $board = $values['board'];
    $topic = $values['topic'];
    $cycle = $values['cycle'];
    $comments = $values['comments'];
    $use_not_list = $values['not-list'] ? false : true;
    $fast_track = $values['fast-track'] ? true : false;
    $special_search = $values['special-search'] ? true : false;
    $test = $values['test-mode'] ? true : false;
    $mode = $test ? 'test' : 'live';
    $import_type = $fast_track ? 'F' : 'R';

    // Invoke the article import API.
    try {
        $batch = Ebms\importArticlesFromNLM($mode, $pmids, $topic, $cycle,
                 $comments, $use_not_list, $import_type);
    }
    catch (\Exception $e) {
        drupal_set_message($e);
        $batch = null;
        $url = EbmsImport::$url;
    }

    // If we succeeded, store the statistical information about the job,
    // and do some population of related tables as appropriate.
    if ($batch) {
        if (!$test) {

            // Get a list of articles affected by the import.
            $imported = array();
            foreach ($batch->action['imported'] as $info)
                $imported[] = $info[1];
            $review_ready = array();
            foreach ($batch->action['reviewReady'] as $info)
                $review_ready[] = $info[1];

            // Apply special search tag to each article if appropriate.
            if ($special_search) {
                foreach ($imported as $article_id)
                    Ebms\addArticleTag('i_specialsearch', $article_id, $topic);
            }

            // If the user checked the "Fast Track" option, add the new state.
            if ($fast_track) {
                $disposition = $values['disposition'];
                $future_meeting = $values['future-meeting'];
                $past_meeting = $values['past-meeting'];
                $meeting = $future_meeting ? $future_meeting : $past_meeting;
                $cmt = $values['ft-comments'];
                $state = $values['placement'];
                foreach ($review_ready as $id) {
                    $state_id = Ebms\setArticleState($id, $state, $topic,
                                                     $board, $cmt);
                    Ebms\addArticleTag('i_fasttrack', $id, $topic);
                    if ($state == 'OnAgenda' && $meeting) {
                        db_insert('ebms_agenda_meeting')
                            ->fields(
                                array(
                                    'nid' => $meeting,
                                    'article_state_id' => $state_id,
                                )
                            )
                            ->execute();
                    }
                    elseif ($state == 'FinalBoardDecision' && $disposition) {
                        db_insert('ebms_article_board_decision')
                            ->fields(
                                array(
                                    'article_state_id' => $state_id,
                                    'decision_value_id' => $disposition,
                                )
                            )
                            ->execute();
                    }
                }
            }
        }

        // Save the statistical report information.
        $report_data = drupal_json_encode($batch);
        $request_params = drupal_json_encode($values);
        $request_id = db_insert('ebms_import_request')
            ->fields(
                array(
                    'request_params' => $request_params,
                    'report_data' => $report_data,
                )
            )
            ->execute();
        $url = EbmsImport::$url . '/' . $request_id;
    }

    // Draw the form again.
    drupal_goto($url);
}
