<?php

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
    Ebms\Menu::$active = 'Manage@@BR@@Citations';
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
        $special_search = $disposition = $past_meeting = $future_meeting =
        $core_journals_search = $hi_priority = 0;
    $bma_disposition = '';

    //$selected_tags = array();
    $pmid = $comments = $placement = $ft_comments = $mgr_comment = '';
    $auto_imports = [];

    // If this is a followup form to a previous import request, use
    // the values submitted for that request to seed the form (TIR 2194).
    if ($request_id) {
        $result = db_select('ebms_import_request', 'r')
            ->fields('r', array('request_params', 'report_data'))
            ->condition('r.request_id', $request_id)
            ->execute()
            ->fetchObject();
        $report = drupal_json_decode($result->report_data);
        $params = drupal_json_decode($result->request_params);
        $board = $params['board'];
        $topic = $params['topic'];
        $cycle = $params['cycle'];
        $past_meeting = $params['past-meeting'];
        $future_meeting = $params['future-meeting'];
        //$selected_tags = $params['tags'];
        $special_search = $params['special-search'];
        $core_journals_search = $params['core-journals-search'];
        $hi_priority = $params['hi-priority'];
        $pmid = $params['pmid'];
        $comments = $params['comments'];
        $mgr_comment = $params['mgr-comment'];
        $not_list = $params['not-list'];
        $test_mode = $params['test-mode'];
        $fast_track = $params['fast-track'];
        $placement = $params['placement'];
        $ft_comments = $params['ft-comments'];
        $disposition = $params['disposition'];
        $bma_disposition = $params['bma-disposition'];
        $want_auto_imports = db_select('ebms_board', 'b')
            ->fields('b', array('board_id'))
            ->condition('b.auto_imports', 1)
            ->execute()
            ->fetchCol();
        $success = $report['status'] === 'Success';
        $board_wants_auto_imports = in_array($board, $want_auto_imports);
        if (!$test_mode && $success && $board_wants_auto_imports) {
            $auto_imports = _pdq_find_related_articles($report);
            if (!empty($auto_imports)) {
                drupal_set_message('PMIDs for ' . count($auto_imports) .
                    ' related articles have been loaded into the ' .
                    'PUB MED ID field.');
                $pmid = implode(' ', array_keys($auto_imports));
            }
            $auto_imports = drupal_json_encode($auto_imports);
        }
    }
    else {

        // Add PMID if quick search failed (OCEEBMS-445).
        $parms = drupal_get_query_parameters();
        if (!empty($parms['pmid']))
            $pmid = $parms['pmid'];
    }

    // If board not set, default it
    if (!$board)
        $board = \Ebms\Util::get_user_default_board();

    // Default to the current review cycle (JIRA::OCEEBMS-166).
    if (!$cycle)
        $cycle = Ebms\Util::get_default_cycle();

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
    $review_cycles = array();
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

    $now = time();
    $past_meetings = pdq_ebms_agenda_meeting_picklist(true, $now, $board);
    $future_meetings = pdq_ebms_agenda_meeting_picklist(false, $now, $board);
    $on_hold = db_select('ebms_article_state_type', 't')
        ->fields('t', array('sequence'))
        ->condition('t.state_text_id', 'OnHold')
        ->execute()
        ->fetchField();
    $results = db_select('ebms_article_state_type', 't')
        ->fields('t', array('state_text_id', 'state_name'))
        ->condition('t.sequence', $on_hold)
        ->orderBy('t.state_name')
        ->execute();
    $bma_dispositions = array('' => ' ');
    foreach ($results as $result)
        $bma_dispositions[$result->state_text_id] = $result->state_name;
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
    $nl_tip = "Don't reject articles on the board's NOT list.";
    $tm_tip = 'See what a job would do without actually doing it.';
    $ft_tip = 'Advance the imported articles to a later state.';
    $cj_tip = 'Check when articles are from core journals.';
    $hp_tip = 'Tag everything imported as high priority.';
    $ss_tip = "Tag the articles in this batch\n" .
        'so they can be easily found later.';
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
        'auto_imports' => array(
            '#type' => 'hidden',
            '#value' => $auto_imports,
        ),
        'comments' => array(
            '#type' => 'textfield',
            '#default_value' => $comments,
            '#title' => 'IMPORT COMMENTS',
        ),
        'mgr-comment' => array(
            '#type' => 'textfield',
            '#default_value' => $mgr_comment,
            '#title' => 'MANAGER COMMENTS',
        ),
        'not-list' => array(
            '#type' => 'checkbox',
            '#title' => 'OVERRIDE NOT LIST',
            '#prefix' => "<div class='checkbox-line' title=\"$nl_tip\">",
            '#suffix' => '</div>',
            '#default_value' => $not_list,
        ),
        'test-mode' => array(
            '#type' => 'checkbox',
            '#title' => 'TEST MODE',
            '#prefix' => "<div class='checkbox-line' title=\"$tm_tip\">",
            '#suffix' => '</div>',
            '#default_value' => $test_mode,
        ),
        'fast-track' => array(
            '#type' => 'checkbox',
            '#title' => 'FAST TRACK',
            '#prefix' => "<div class='checkbox-line' title=\"$ft_tip\">",
            '#suffix' => '</div>',
            '#default_value' => $fast_track,
        ),
        'special-search' => array(
            '#type' => 'checkbox',
            '#title' => 'SPECIAL SEARCH',
            '#prefix' => "<div class='checkbox-line' title=\"$ss_tip\">",
            '#suffix' => '</div>',
            '#default_value' => $special_search,
        ),
        'core-journals-search' => array(
            '#type' => 'checkbox',
            '#title' => 'CORE JOURNALS',
            '#prefix' => "<div class='checkbox-line' title=\"$cj_tip\">",
            '#suffix' => '</div>',
            '#default_value' => $core_journals_search,
        ),
        'hi-priority' => array(
            '#type' => 'checkbox',
            '#title' => 'HIGH PRIORITY',
            '#prefix' => "<div class='checkbox-line' title=\"$hp_tip\">",
            '#suffix' => '</div>',
            '#default_value' => $hi_priority,
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
                'Published' => 'Published',
                'PassedBMReview' => 'Passed abstract review',
                'PassedFullReview' => 'Passed full text review',
                'BMA' => 'Board Manager Action',
                'OnAgenda' => 'On agenda',
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
        'bma-disposition' => array(
            '#type' => 'select',
            '#title' => 'DISPOSITION *',
            '#options' => $bma_dispositions,
            '#default_value' => $bma_disposition,
            '#states' => array(
                'visible' => array(
                    ':input[name="placement"]' => array(
                        'value' => 'BMA',
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
            // Added 'title' for OCEEBMS-418.
            '#title' => 'BATCH IMPORT ONLY',
            '#type' => 'file',
        ),
        // Added by OCEEBMS-472.
        'full-text' => array(
            '#title' => 'FULL TEXT',
            '#type' => 'file',
            '#upload_validators' => ['file_validate_extensions' => ['pdf']],
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
        $batch = (object) $report;
        $form['report'] = Ebms\Util::import_report($batch, 'Statistics');
    }
    return $form;
}

/**
 * Inspect the articles newly imported by the batch we just processed
 * to see if any other articles which we don't already have are listed
 * as related to one or more of those newly imported articles.
 *
 * @param array $report
 *   Information about the batch we just imported
 *
 * @return array
 *   Array whose keys are PMIDs for the related articles and whose
 *   values are lists of the EBMS IDs for the imported articles to
 *   which the newly found articles are related
 */
function _pdq_find_related_articles($report) {

    // Collect the article IDs for newly imported and re-imported articles.
    $states = ['imported', 'duplicate'];
    $articles = [];
    foreach ($states as $state) {
        foreach ($report['action'][$state] as $article) {
            $id = $article[1];
            $articles[$id] = $id;
        }
    }

    // Find the PMIDs of the related articles.
    $pmids = [];
    foreach ($articles as $key => $value) {
        $article_id = $key;
        if (Ebms\Util::in_core_journal($article_id)) {
            $xml = db_select('ebms_article', 'a')
                ->fields('a', array('source_data'))
                ->condition('a.article_id', $article_id)
                ->execute()
                ->fetchField();
            $doc = new \SimpleXMLElement($xml);
            $cite = $doc->MedlineCitation;
            if (isset($cite->CommentsCorrectionsList->CommentsCorrections)) {
                foreach ($cite->CommentsCorrectionsList->CommentsCorrections
                         as $cc) {
                    if (!empty($cc->PMID)) {
                        $pmid = trim($cc->PMID);
                        if (!empty($pmid)) {
                            /* The users have changed their minds. They no longer want to
                               restrict this enhancement to articles we haven't seen before,
                               but to all articles in core journals, EVERY TIME we see them.
                               (2020-09-25 OCEEBMS-568 later comments)
                            $query = db_select('ebms_article', 'a')
                                ->condition('a.source_id', $pmid);
                            $query = $query->countQuery();
                            $count = $query->execute()->fetchField();
                            if (empty($count)) */
                                $pmids[$pmid][] = $article_id;
                        }
                    }
                }
            }
        }
    }
    return $pmids;
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
        $ord = 'DESC';
    }
    else {
        $op = '>';
        $ord = 'ASC';
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
        elseif ($form_state['values']['placement'] == 'BMA') {
            if (!$form_state['values']['bma-disposition'])
                form_set_error('bma-disposition',
                    'A disposition must be selected for ' .
                    'Board Member Actions.');
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
        $pmids = preg_split('/[\s,]+/', $form_state['values']['pmid'], null,
                 PREG_SPLIT_NO_EMPTY);
        foreach ($pmids as $pmid) {
            if (!preg_match('/^\d{1,8}$/', $pmid)) {
                form_set_error('pmid', 'Invalid Pubmed ID format.');
                break;
            }
        }
    }
    if (!$errors && count($pmids) < 1)
        form_set_error('pmid', 'You must either specify Pubmed IDs or ' .
            'upload a Pubmed search results file');
    $form_state['article-ids'] = $pmids;
    $form_state['full-text-id'] = null;
    if (!empty($_FILES['files']['name']['full-text'])) {
        if (count($pmids) > 1)
            form_set_error('full-text', 'Full-text PDF upload can only be ' .
                'used for importing a single article');
        else {
            $name = $_FILES['files']['name']['full-text'];
            $path = $_FILES['files']['tmp_name']['full-text'];
            $extension_ok = Ebms\Util::has_valid_extension($name, 'pdf');
            $mime_type = Ebms\Util::get_mime_type($path);
            if (!$extension_ok || $mime_type !== 'application/pdf')
                form_set_error('full-text', 'Full text must be a PDF file');
            else {
                $validators = ['file_validate_extensions' => ['pdf']];
                $file = file_save_upload('full-text', $validators);
                if ($file) {
                    $file = file_move($file, 'public://');
                    $file->status |= FILE_STATUS_PERMANENT;
                    $file = file_save($file);
                    $form_state['full-text-id'] = $file->fid;
                }
                else
                    form_set_error('full-text', "Unable to upload $name");
            }
        }
    }
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
    $full_text_id = $form_state['full-text-id'];
    $values =& $form_state['values'];

    // Log the request so we can reproduce it if it fails (OCEEBMS-313).
    $debug_info = array(
        '$form_state[values]' => $values,
        '$pmids' => $pmids
    );
    pdq_ebms_debug('IMPORT FORM SUBMIT', $debug_info);

    $board = $values['board'];
    $topic = $values['topic'];
    $cycle = $values['cycle'];
    $comments = $values['comments'];
    $mgr_comment = $values['mgr-comment'];
    $use_not_list = $values['not-list'] ? false : true;
    $fast_track = $values['fast-track'] ? true : false;
    $special_search = $values['special-search'] ? true : false;
    $test = $values['test-mode'] ? true : false;
    $mode = $test ? 'test' : 'live';
    $import_type = $fast_track ? 'F' : 'R';
    $core_journals = !empty($values['core-journals-search']);
    $hi_priority = !empty($values['hi-priority']);

    // Invoke the article import API.
    try {
        $batch = Ebms\importArticlesFromNLM($mode, $pmids, $topic, $cycle,
                 $comments, $use_not_list, $import_type);
    }
    catch (\Exception $e) {
        drupal_set_message($e, 'error');
        $batch = null;
        $url = EbmsImport::$url;
    }

    // If we succeeded, store the statistical information about the job,
    // and do some population of related tables as appropriate.
    if ($batch) {
        if (!$test) {

            // We'll need these in more than one place.
            global $user;
            $now = strftime("%Y-%m-%d %H:%M:%S");

            // Get a list of articles affected by the import.
            $imported = array();
            foreach ($batch->action['imported'] as $info)
                $imported[] = $info[1];
            $duplicate = array();
            foreach ($batch->action['duplicate'] as $info)
                $duplicate[] = $info[1];
            $review_ready = array();
            foreach ($batch->action['reviewReady'] as $info)
                $review_ready[] = $info[1];

            // Apply special search tag to each article if appropriate.
            if ($special_search) {
                foreach ($imported as $article_id)
                    Ebms\addArticleTag('i_specialsearch', $article_id, $topic);

                // Do this for dups, too (OCEEBMS-13).
                foreach ($duplicate as $article_id)
                    Ebms\addArticleTag('i_specialsearch', $article_id, $topic);
            }

            // Assign 'High priority' tag if so requested (OCEEBMS-472).
            if ($hi_priority) {
                foreach ($imported as $article_id)
                    Ebms\addArticleTag('high_priority', $article_id, $topic);
                foreach ($duplicate as $article_id)
                    Ebms\addArticleTag('high_priority', $article_id, $topic);
            }

            // Record the location of the full-text PDF file (OCEEBMS-472).
            // Validation has already ensured that at most one article
            // has been imported (duplicate or not).
            if (!empty($full_text_id)) {
                foreach ($imported as $article_id)
                    db_update('ebms_article')
                        ->fields(array('full_text_id' => $full_text_id))
                        ->condition('article_id', $article_id)
                        ->execute();
                foreach ($duplicate as $article_id)
                    db_update('ebms_article')
                        ->fields(array('full_text_id' => $full_text_id))
                        ->condition('article_id', $article_id)
                        ->execute();
            }

            // OCEEBMS-281 & OCEEBMS-340
            if ($core_journals) {
                foreach ($review_ready as $id) {
                    Ebms\addArticleTag('i_core_journals', $id, $topic);
                    Ebms\setArticleState($id, 'Published', $topic, $board,
                        'Published as part of import from core journals');
                }
            }

            // Store the board manager comment if provided (OCEEBMS-472).
            if (!empty($mgr_comment)) {
                $table = 'ebms_article_topic_comment';
                $fields = array(
                    'topic_id' => $topic,
                    'comment' => $mgr_comment,
                    'created_by' => $user->uid,
                    'created' => $now,
                );
                foreach ($imported as $article_id) {
                    $fields['article_id'] = $article_id;
                    db_insert($table)->fields($fields)->execute();
                }
                foreach ($duplicate as $article_id) {
                    $fields['article_id'] = $article_id;
                    db_insert($table)->fields($fields)->execute();
                }
            }

            // If the user checked the "Fast Track" option, add the new state.
            if ($fast_track) {
                $disposition = $values['disposition'];
                $bma_disposition = $values['bma-disposition'];
                $future_meeting = $values['future-meeting'];
                $past_meeting = $values['past-meeting'];
                $meeting = $future_meeting ? $future_meeting : $past_meeting;
                $cmt = $values['ft-comments'];
                $state = $values['placement'];
                if ($state === 'BMA')
                    $state = $bma_disposition;
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

            // Some of these may have been imported because
            // they were designated by NLM as related to articles
            // in the previous import batch. See OCEEBMS-568.
            $related = array();
            $auto_imports = $values['auto_imports'];
            if (!empty($auto_imports)) {
                $related = drupal_json_decode($auto_imports);
                $today = date('Y-m-d');
                $comment = "linked programmatically upon import ($today)";
                $other = db_select('ebms_article_relation_type', 't')
                    ->fields('t', array('type_id'))
                    ->condition('t.type_name', 'Other')
                    ->execute()
                    ->fetchField();
                $fields = array(
                    'type_id' => $other,
                    'comment' => $comment,
                    'created_by' => $user->uid,
                    'created' => $now,
                );
                foreach ($batch->action['imported'] as $info) {
                    $pmid = $info[0];
                    $to_id = $info[1];
                    if (array_key_exists($pmid, $related)) {
                        $fields['to_id'] = $to_id;
                        foreach ($related[$pmid] as $from_id) {
                            $fields['from_id'] = $from_id;
                            $id = db_insert('ebms_related_article')
                                ->fields($fields)
                                ->execute();
                        }
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

// OCEEBMS-87
//function pdq_ebms_refresh_xml() {
//    return drupal_get_form('pdq_ebms_refresh_xml_form');
//}

// OCEEBMS-87
function pdq_ebms_refresh_xml_form($form, &$form_state) {
    return array(
        'page title' => array(
            '#markup' => '<h2>Refresh Article XML From Pubmed</h2>',
        ),
        'max' => array(
            '#type' => 'textfield',
            '#title' => 'Maximum number of articles to check',
            '#default_value' => 2500,
            '#description' => '2500 is a safe limit, running little risk of ' .
            'timing out the web server connection.',
        ),
//         'pmids' => array(
//             '#type' => 'textarea',
//             '#title' => 'Pubmed IDS',
//             '#required' => true,
//         ),
        'submit' => array(
            '#type' => 'submit',
            '#value' => 'Submit',
        ),
    );
}

/*
 * Can't use this, as it turns out, because it uses up too much memory,
 * at least in the current condition of the database.  It's possible
 * that it will be useful once we've caught up on refreshing most of
 * the article XML, so I'll leave it here until I've created at least
 * one new version of this file.  It may always be risky, though, because
 * NLM has a habit of arbitrarily changing little insignificant things
 * in massive numbers of their records all at once from time to time
 * (this has bitten us many times with their clinical trials database)
 * so we may never get to use this function.
 */
function pdq_ebms_ask_nlm_which_have_changed($since, $pmids) {

    // NLM expects dates in a funky format.
    $since = str_replace('-', '/', substr($since, 0, 10));

    // Construct a URL to fetch.
    $retmax = count($pmids);
    $base = 'https://eutils.ncbi.nlm.nih.gov/entrez/eutils/esearch.fcgi';
    $parms = "db=pubmed&retmax=$retmax&term=($since" .
        '[MDAT]:3000[MDAT])+AND+' . implode(',', $pmids) . '[UID]';
    $url = "$base?$parms";

    // Fetch the Pubmed IDs for the articles which have changed since
    // our cutoff date.
    $ch = curl_init();
    curl_setopt($ch, CURLOPT_URL, $base);
    curl_setopt($ch, CURLOPT_POST, true);
    curl_setopt($ch, CURLOPT_POSTFIELDS, $parms);
    curl_setopt($ch, CURLOPT_RETURNTRANSFER, TRUE);
    curl_setopt($ch, CURLOPT_SSL_VERIFYPEER, false);
    curl_setopt($ch, CURLOPT_SSL_VERIFYHOST, false);
    $results = curl_exec($ch);
    curl_close($ch);

    // Check for problems.
    $error = array();
    if (preg_match('#<ERROR>(.*)</ERROR>#Us', $results, $error)) {
        drupal_set_message($error[1], 'error');
        return null;
    }
    if (!preg_match('#<IdList#', $results)) {
        drupal_set_message('Failure retrieving PMID list from NLM.', 'error');
        pdq_ebms_debug('MDAT FAILURE', $results);
        return null;
    }

    // Extract the PMIDs into an array.
    $pattern = '#<Id>(\d+)</Id>#';
    $offset = 0;
    $pmids = array();
    while (true) {
        $matches = array();
        $found = preg_match($pattern, $results, $matches,
                 PREG_OFFSET_CAPTURE, $offset);
        if ($found == 1) {
            $pmids[] = trim($matches[1][0]);
            $offset = $matches[1][1] + strlen($matches[1][0]);
            $count = count($pmids);
        }
        else {
            return $pmids;
        }
    }
}

/*
 * Bulk refresh of article XML from Pubmed.  Check a large chunk
 * of articles which have never been checked before, or whose last
 * check was done over a week ago.  Can't do too many at once,
 * or we'll time out the web server connection.
 */
function pdq_ebms_refresh_xml_form_submit($form, &$form_state) {

    // Set up working variables.
    $today = date('Y-m-d');
    $start = microtime(true);
    $max_to_check = 2500;
    if ($form_state['values']['max'])
        $max_to_check = $form_state['values']['max'];
    $last_week = date('Y-m-d', strtotime('-7 days'));

    // Collect PMIDs for articles we've never checked before.
    $pmids = array();
    $results = db_select('ebms_article', 'a')
        ->condition('a.source', 'Pubmed')
        ->isNull('a.data_checked')
        ->fields('a', array('source_id'))
        ->orderBy('a.article_id')
        ->range(0, $max_to_check)
        ->execute();
    foreach ($results as $result) {
        $pmid = trim($result->source_id);
        if ($pmid)
            $pmids[] = $pmid;
    }

    // If we can handle more articles, get the ones which have gone the
    // longest since their last check.
    if (count($pmids) < $max_to_check) {
        $results = db_select('ebms_article', 'a')
            ->condition('a.source', 'Pubmed')
            ->condition('a.data_checked', $last_week, '<')
            ->fields('a', array('source_id'))
            ->orderBy('a.data_checked')
            ->range(0, $max_to_check - count($pmids))
            ->execute();
        foreach ($results as $result) {
            $pmid = trim($result->source_id);
            if ($pmid)
                $pmids[] = $pmid;
        }
    }
    $changed = pdq_ebms_ask_nlm_which_have_changed('2012/01/01', $pmids);
    if (is_null($changed))
        return;

    // Ask NLM for fresh documents for these PMIDs.
    if (!empty($changed)) {
        $batch = Ebms\importArticlesFromNLM('live', $changed, null, null,
                 'BATCH REPLACEMENT OF UPDATED ARTICLES FROM PUBMED');
        $count = count($batch->action['replaced']);
    }
    else
        $count = 0;

    // Record our check of these articles.
    $updated = db_update('ebms_article')
        ->fields(array('data_checked' => $today))
        ->condition('source', 'Pubmed')
        ->condition('source_id', $pmids, 'IN')
        ->execute();

    // Tell the user what we've done.
    $checked = count($pmids);
    $unchecked = db_select('ebms_article', 'a')
        ->condition('a.source', 'Pubmed')
        ->isNull('a.data_checked')
        ->countQuery()
        ->execute()
        ->fetchField();
    $stale = db_select('ebms_article', 'a')
        ->condition('a.source', 'Pubmed')
        ->condition('a.data_checked', $last_week, '<')
        ->countQuery()
        ->execute()
        ->fetchField();
    $pending = $unchecked + $stale;
    $stats = "Checked $checked articles; $count refreshed; " .
        "$pending still need to be checked";
    $finish = microtime(true);
    $elapsed = $finish - $start;
    $performance = sprintf("%.3f seconds", $elapsed);
    drupal_set_message("$stats ($performance).");
}

/*
 * Bulk refresh of article XML for a given source.  Look at all of the
 * articles for which the source has informed us of a modification
 * later than we last imported fresh XML or verified that the modification
 * reported by the source does not result in any actual change to the
 * XML document.  We do an update for as many as we think we can
 * safely handle in a single HTTP connection, and report back how
 * many articles from this source still need to be checked.
 *
 * JIRA::OCEEBMS-87
 */
function pdq_ebms_refresh_xml($source, $max=null) {

    // Collect PMIDs for articles we've never checked before.
    drupal_add_http_header('Content-type', 'text/plain');
    //$start = microtime(true);
    $max_to_refresh = $max ? $max : 2500;
    $pmids = array();
    $checked = array();
    $query = db_select('ebms_article', 'a')
        ->condition('a.source', $source)
        ->isNotNull('a.data_mod')
        ->where('a.data_checked IS NULL OR a.data_checked < a.data_mod');
    $total = $query->countQuery()->execute()->fetchField();
    if (!$total) {
        print "0";
        exit(0);
    }
    $fields = array('source_id', 'import_date', 'update_date', 'data_mod');
    $articles = $query->fields('a', $fields)->orderBy('a.data_mod')->execute();
    foreach ($articles as $art) {
        $checked[] = $art->source_id;
        $pmids[] = $art->source_id;
        if (count($pmids) >= $max_to_refresh)
            break;
    }

    // Ask NLM for fresh documents for these PMIDs.
    if (!empty($pmids)) {
        $comment = 'BATCH REPLACEMENT OF UPDATED ARTICLES FROM PUBMED';
        $batch = Ebms\importArticlesFromNLM('live', $pmids, null, null,
                                            comment, false);
        if ($batch->status != 'Success') {
            print '0';
            exit(0);
        }
        $count = count($batch->action['replaced']);
    }
    else
        $count = 0;

    // Record our check of these articles.  Skip any articles with errors.
    $errors = array();
    foreach ($batch->action['error'] as $error)
        $errors[] = $error[0];
    $checked = array_diff($checked, $errors);

    // If everything we looked at errored out, better stop now.
    if (empty($checked)) {
        print '0';
        exit(0);
    }
    $today = date('Y-m-d');
    $updated = db_update('ebms_article')
        ->fields(array('data_checked' => $today))
        ->condition('source', 'Pubmed')
        ->condition('source_id', $checked, 'IN')
        ->execute();

    // Tell the user what we've done.
    $remaining = $total - count($checked);
    print $remaining;
    exit(0);
}
