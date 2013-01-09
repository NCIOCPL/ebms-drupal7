<?php

// $Id$

// Show all errors
error_reporting(-1);
$conf['error_level'] = 2;
ini_set('display_errors', TRUE);
ini_set('display_startup_errors', TRUE);

/**
 * @file
 *
 * Testing for Alan.
 */
function pdq_ebms_alantest() {
    // Test
    // return '<i>Stub for Alan\'s testing</i>';
    // require_once('logw.inc');
    require_once('EbmsArticle.inc');
    require_once('EbmsImport.inc');
    require_once('EbmsSearch.inc');
    require_once('EbmsPrint.inc');

    pdq_ebms_debug('ALAN POST', $_POST);
    $test = Ebms\Util::get_request_value('test');
    $op = Ebms\Util::get_request_value('op');
    switch ($test) {
        case 'import':
            if ($op)
                return pdq_ebms_import_test();
            return drupal_get_form('ebms_testapi_import_form');
        case 'oldImport':
            if ($op)
                return pdq_ebms_oldimport_test();
            return drupal_get_form('ebms_testapi_oldimport_form');
        case 'setStatus':
            if ($op)
                return pdq_ebms_setstatus_test();
            return drupal_get_form('ebms_testapi_setstatus_form');
        case 'addStatusComment':
            if ($op)
                return pdq_ebms_addstatuscomment_test();
            return drupal_get_form('ebms_testapi_addstatuscomment_form');
        case 'curStatus':
            if ($op)
                return pdq_ebms_getcurrentstatus_test();
            return drupal_get_form('ebms_testapi_getcurrentstatus_form');
        case 'viewStatus':
            if ($op)
                return pdq_ebms_getstatushistory_test();
            return drupal_get_form('ebms_testapi_getstatushistory_form');
        case 'addTag':
            if ($op)
                return pdq_ebms_addtag_test();
            return drupal_get_form('ebms_testapi_addtag_form');
        case 'addTagComment':
            if ($op)
                return pdq_ebms_addtagcmt_test();
            return drupal_get_form('ebms_testapi_addtagcmt_form');
        case 'inactivateTag':
            if ($op)
                return pdq_ebms_inactivatetag_test();
            return drupal_get_form('ebms_testapi_inactivatetag_form');
        case 'execSearch':
            if ($op)
                return pdq_ebms_search_test();
            return drupal_get_form('ebms_testapi_search_form');
        case 'printPkts':
            if ($op)
                return pdq_ebms_printpkt_test();
            return drupal_get_form('ebms_testapi_printpkt_form');
        case 'getPrintouts':
            if ($op)
                return pdq_ebms_printouts_test();
            return drupal_get_form('ebms_testapi_printouts_form');
        case 'setPrintMbrs':
            if ($op)
                return pdq_ebms_setprintmbrs_test();
            return drupal_get_form('ebms_testapi_setprintmbrs_form');
        case 'fpdf':
            if ($op)
                return pdq_ebms_fpdf_test();
            return drupal_get_form('ebms_testapi_fpdf_form');
        /* Debug board picker
        case 'pickBoard':
            if ($op) return test_pickBoard_test();
            return drupal_get_form('ebms_pickBoard_form');
        */
        default:
            return pdq_ebms_api_tests();
    }
}

function pdq_ebms_import_test() {

    // Get form data
    $pmidStr    = \Ebms\Util::get_request_value('pmids');
    $fileObj    = \Ebms\Util::get_request_value('upload');
    $mode       = \Ebms\Util::get_request_value('impmode');
    $cycleId    = \Ebms\Util::get_request_value('cycleId');
    $topicId    = \Ebms\Util::get_request_value('topicId');
    $frmNotList = \Ebms\Util::get_request_value('useNotList');
    $inputType  = \Ebms\Util::get_request_value('inputType');
    $comment    = \Ebms\Util::get_request_value('comment');

    // Validate Pubmed data entered
    if ($pmidStr && $fileObj['fid'])
        return "Please upload a file or enter Pubmed IDs, not both";
    if (!$pmidStr && !$fileObj['fid'])
        return "Please upload a file or enter a list of Pubmed IDs";

    // Data conversion
    $useNotList = ($frmNotList == 'Y') ? true : false;

    if ($fileObj['fid'])
        // Convert file text to array of Pubmed IDs
        $pmids = Ebms\findPubmedIDs($fileObj['fid'], null, null);

    if ($pmidStr)
        // Convert string to array of Pubmed IDs
        $pmids = preg_split("/[\s,]/", $pmidStr, -1, PREG_SPLIT_NO_EMPTY);

    // Try it
    $ib = Ebms\importArticlesFromNLM($mode, $pmids, $topicId, $cycleId,
                                     $comment, $useNotList, $inputType, null);

    // Dump the ImportBatch object
    return $ib->dumpHtml();
}

// Implements return of the data from an existing import
function pdq_ebms_oldimport_test() {

    // Get form data
    $batchId = intval(\Ebms\Util::get_request_value('batchId'));

    // Try it.  Function will fail with exception if id doesn't exist
    $ib = new \Ebms\ImportBatch($batchId);

    // Dump the ImportBatch object
    return $ib->dumpHtml();
}

// Implements setting status
function pdq_ebms_setstatus_test() {

    // Article identifier
    $errMsg    = null;
    $articleId = getArticleId($errMsg);
    if (!$articleId)
        return $errMsg;

    // Status data, converted to string
    $statusNum  = Ebms\Util::get_request_value('status');
    $allStates  = getAllStates();
    $statusName = $allStates[$statusNum];

    // Editorioal board - optional
    $boardInfo = boardRequest();
    $boardId   = $boardInfo['boardId'];

    // Topic - optional
    $topicId = Ebms\Util::get_request_value('topicId');

    // Cycle ID
    $cycleId = Ebms\Util::get_request_value('cycleId');
    if (!$cycleId)
        $cycleId = null;

    // Text to add
    $comment = Ebms\Util::get_request_value('comment', null);

    // Set the requested status
    $stRowId = Ebms\setArticleState($articleId, $statusName, $topicId,
                                    $boardId, $comment, null, null, $cycleId);

    // Show effect on the history - show the whole history
    $hist = Ebms\getStatusHistory($articleId, null, null, true, true);
    return Ebms\statusHistoryToHtml($hist);
}

// Implements adding another comment to status
function pdq_ebms_addstatuscomment_test() {

    // Article identifier
    $errMsg    = null;
    $articleId = getArticleId($errMsg);
    if (!$articleId)
        return $errMsg;

    // Status data
    $states    = getAllStates();
    $stateNum  = Ebms\Util::get_request_value('status');
    if (!$stateNum)
        return "Please select a state";
    $stateName = $states[$stateNum];

    // Comment
    $comment = Ebms\Util::get_request_value('comment', null);
    if (!$comment)
        return "Please enter a comment";

    // Find the row in the status table for this
    $qry = db_select('ebms_article_state', 'state');
    $qry->join('ebms_article_state_type', 'stype',
                'state.state_id = stype.state_id');
    $qry->fields('state', array('article_state_id'));
    $qry->condition('state.article_id', $articleId)
        ->condition('stype.state_name', $stateName)
        ->condition('state.active_status', 'A');
    $stRowId = $qry->execute()->fetchField();

    // Was there one?
    if (!$stRowId)
        return ("No active status row for articleId=$articleId " .
                "and status=\"$stateName\"");
    // Add the comment
    $commentId = Ebms\addArticleStateComment($stRowId, $comment);

    return "<p>Success, comment_id=$commentId</p>";
}

function pdq_ebms_getcurrentstatus_test() {

    // Article identifier
    $errMsg    = null;
    $articleId = getArticleId($errMsg);
    if (!$articleId)
        return $errMsg;

    $topicStr = Ebms\Util::get_request_value('topicId', null);
    $topicId  = $topicStr ? intval($topicStr) : null;

    // If string but no ID, try converting from string
    if (!is_null($topicId) && $topicId == 0)
        $topicId = Ebms\getTopicIdByName($topicStr);

    // Get the current status
    $curStat = Ebms\getCurrentState($articleId, $topicId);

    // Convert to HTML
    $html = Ebms\currentStatusToHtml($curStat);

    return $html;
}

function pdq_ebms_getstatushistory_test() {

    // Article identifier
    $errMsg    = null;
    $articleId = getArticleId($errMsg);
    if (!$articleId)
        return $errMsg;

    // Board or topic identifer
    $boardInfo = boardRequest();
    $boardId   = $boardInfo['boardId'];

    $topicStr = Ebms\Util::get_request_value('topicId', null);
    $topicId  = $topicStr ? intval($topicStr) : null;

    // If string but no ID, try converting from string
    if (!is_null($topicId) && $topicId == 0)
        $topicId = Ebms\getTopicIdByName($topicStr);

    $showInactive = Ebms\Util::get_request_value('showInactive');

    // Sort order
    $sortOrder = Ebms\Util::get_request_value('sortOrder', 'DBST');

    // Get the status and tag history
    $statusHist = Ebms\getStatusHistory($articleId, $boardId, $topicId,
                                        true, $showInactive, $sortOrder);
    $tagHist    = Ebms\getTagHistory($articleId, $topicId, null,
                                     true, $showInactive);
    // Convert to HTML
    $html = Ebms\statusHistoryToHtml($statusHist);
    if (count($tagHist) > 0)
        $html .= "\n<hl />\n" . Ebms\tagHistoryToHtml($tagHist);

    return $html;
}

function pdq_ebms_addtag_test() {

    // Get input form parameters
    $errMsg    = null;
    $articleId = getArticleId($errMsg);
    if (!$articleId)
        return $errMsg;

    $tagInfo   = tagRequest();
    $tagTextId = $tagInfo['tagTextId'];
    if (!$tagInfo)
        return "A descriptive tag is required";

    // Optional topic
    $topicStr = Ebms\Util::get_request_value('topicId', null);
    $topicId  = $topicStr ? intval($topicStr) : null;

    // Optional comment
    $comment = Ebms\Util::get_request_value('comment', null);

    // Add the tag to the article
    $rowId = Ebms\addArticleTag($tagTextId, $articleId, $topicId, $comment);

    $tagHist = Ebms\getTagHistory($articleId, $topicId);
    return Ebms\tagHistoryToHtml($tagHist);
}

function pdq_ebms_addtagcmt_test() {

    // Input form parms
    $errMsg    = null;
    $articleId = getArticleId($errMsg);
    if (!$articleId)
        return $errMsg;

    $articleTagId = Ebms\Util::get_request_value('articleTagId');
    if (!$articleTagId)
        return "Article tag ID is required";

    // Comment
    $comment = Ebms\Util::get_request_value('comment', null);
    if (!$comment)
        return "Comment text is required";

    // Check for match on article ID and tag ID
    $errMsg = null;
    checkArticleTag($articleId, $articleTagId, $errMsg);
    if ($errMsg)
        return $errMsg;

    // Add the comment and get results
    Ebms\addTagComment($articleTagId, $comment);
    $hist = Ebms\getTagHistory($articleId);

    return Ebms\tagHistoryToHtml($hist);
}

// Inactivate a tag and, by inference, all its comments
function pdq_ebms_inactivatetag_test() {

    // Input form parms
    $errMsg    = null;
    $articleId = getArticleId($errMsg);
    if (!$articleId)
        return $errMsg;

    $articleTagId = Ebms\Util::get_request_value('articleTagId');
    if (!$articleTagId)
        return "Article tag ID is required";

    $showInactive = Ebms\Util::get_request_value('showInactive');

    // Check for match on article ID and tag ID
    $errMsg = null;
    checkArticleTag($articleId, $articleTagId, $errMsg);
    if ($errMsg)
        return $errMsg;

    // Add the comment and get results
    Ebms\inactivateArticleTag($articleTagId);
    $hist = Ebms\getTagHistory($articleId, null, null, true, $showInactive);

    return Ebms\tagHistoryToHtml($hist);
}

function pdq_ebms_search_test() {

    // Pick out a JSON search string from the database by its primary key
    $srchKey = Ebms\Util::get_request_value('srchId');
    $json = db_query(
            'SELECT search_spec FROM {ebms_search} WHERE search_id = :sid',
            array(':sid' => $srchKey))->fetchField();

    if (!$json)
        throw new \Exception("No search found with search_id=$json");

    // Get the query object
    $qry = Ebms\getSearchQuery($json);

    // Execute the search
    $result = $qry->execute();
    $rowCount = $result->rowCount();
    $html = "<p>Found $rowCount articles</p>\n<table border='1'>\n";
    $count = 0;
    $maxShow = $rowCount;
    if ($maxShow > 40)
        $maxShow = 40;
    for ($i=0; $i<$maxShow; $i++) {
        $artId = $result->fetchField();
        $artObj = new Ebms\EbmsArticle($artId);
        $html .= " <tr><td>" . $artId . "</td><td>"
              . $artObj->getBrfCite() . "</td></tr>\n";
    }
    $html .= "</table>\n";

    return $html;
}

/**
 * Function to process print request.
 */
function pdq_ebms_printpkt_test() {

    global $user;
    $userId = $user->uid;

    // Get all form parameters
    $boardInfo  = boardRequest();
    $boardId    = $boardInfo['boardId'];
    $memberIdx  = Ebms\Util::get_request_value('memberId', null);
    $packetId   = Ebms\Util::get_request_value('packetId', null);
    $startDt    = Ebms\Util::get_request_value('startDate', null);
    $endDt      = Ebms\Util::get_request_value('endDate', null);
    $oldJobId   = Ebms\Util::get_request_value('oldJobId', null);
    $jobMode    = Ebms\Util::get_request_value('jobMode', null);
    $comment    = Ebms\Util::get_request_value('comment', null);

    // Normalize
    if ($memberIdx) {
        $member = decodeMemberIdx($memberIdx);
        $memberId = $member['memberId'];
    }
    else
        $memberId = null;

    // Sometimes a returned value is '', which we want to be null
    if (!$packetId) $packetId = null;
    if (!$startDt)  $startDt  = null;
    if (!$endDt)    $endDt    = null;
    if (!$oldJobId) $oldJobId = null;
    if (!$jobMode)  $jobMode  = null;
    if (!$comment)  $comment  = null;

    // Derive the print job type
    $jobType = 'board';
    if (!is_null($packetId))
        $jobType = 'packet';
    else if (!is_null($memberId))
        $jobType = 'package';
    else if (!is_null($boardId))
        $job_type = 'board';
    else if (is_null($oldJobId))
        return("Must specify board, board member, packet ID, or old job ID");

    // Validate foreign keys before attempting an insert
    if ($packetId) {
        $val = db_query('SELECT packet_id FROM {ebms_packet}
                         WHERE packet_id = :packetId',
                        array(':packetId'=>$packetId))->fetchField();
        if (!$val)
            return "Invalid Packet ID.  Packet $packetId does not exist.";
    }
    if ($oldJobId) {
        $val = db_query('SELECT print_job_id FROM {ebms_print_job}
                         WHERE print_job_id = :oldJobId',
                        array(':oldJobId'=>$oldJobId))->fetchField();
        if (!$val)
            return "Invalid Old job ID.  Job $oldJobId does not exist.";
    }
    if ($startDt) {
        // Returns 1 if start date < now, 0 if >, Null if invalid start date
        $val = db_query('SELECT DATE(:startDt) <= NOW()',
                  array(':startDt'=>$startDt))->fetchField();
        if (is_null($val))
            return "Invalid format start date '$startDt'";
        if ($val == 0)
            return "Start date '$startDt' is in the future.  That won't work";
    }
    if ($endDt) {
        $val = db_query('SELECT DATE(:endDt) > :startDt',
                  array(':endDt'=>$endDt, ':startDt'=>$startDt))->fetchField();
        if (is_null($val))
            return "Invalid format end date '$endDt'";
        if ($val == 0)
            return "Start date '$startDt' is after end date '$endDt'";
    }

    // Create a new job, ready to run
    $qry = db_insert('ebms_print_job')
      ->fields(array(
              'old_job_id'        => $oldJobId,
              'user_id'           => $userId,
              'print_job_type_id' => $jobType,
              'packet_start_dt'   => $startDt,
              'packet_end_dt'     => $endDt,
              'print_dt'          => null,
              'board_id'          => $boardId,
              'board_member_id'   => $memberId,
              'packet_id'         => $packetId,
              'mode'              => $jobMode,
              'comment'           => $comment,
              'status'            => 'queued')
          );
    $jobId = $qry->execute();

    // Execute the print job
    $status = Ebms\runPrintJob();
    if ($status == 'success')
        downloadPrintResults($jobId);

    return "Processing complete.";
}

function pdq_ebms_show_dummy_test_results($title, $test) {
    $header = array('Who', 'What', 'When', 'Where');
    $rows = array(
        array('Fred', 'Builder', 'Stone Age', 'Bedrock'),
        array('Wilma', 'Wife', 'Stone Age', 'Bedrock'),
        array('Bamm-Bamm', 'Child', 'Stone Age', 'Bedrock'),
        array('Barney', 'Neighbor', 'Stone Age', 'Bedrock'),
    );
    $options = array('query' => array('test' => $test));
    $another = '<p>' . l('Another Test', 'alantest', $options) . '</p>';
    $main_menu = '<p>' . l('Main Test Menu', 'alantest') . '</p>';
    return array(
        '#theme' => 'table',
        '#caption' => $title . ' Test Results',
        '#header' => $header,
        '#rows' => $rows,
        '#suffix' => "$another $main_menu",
    );
}

/**
 * Function to display print job results.
 */
function pdq_ebms_printouts_test() {

    // Did the user want to see a job output?
    $jobId = Ebms\Util::get_request_value('jobId');


    if ($jobId) {
        // Check job
        $status = db_query(
            'SELECT status FROM {ebms_print_job} WHERE print_job_id = :jobId',
            array(':jobId' => $jobId))->fetchField();

        if (!$status)
            return "Print job $jobId not found";
        if ($status != 'success')
            return "Job $jobId is marked as '$status', no output is available";
        downloadPrintResults($jobId);
    }
    else
        return;
}

/**
 * Add, edit, or delete a member in the ebms_member_wants_print table.
 */
function pdq_ebms_setprintmbrs_test() {

    // Has to be a member selected to do anything
    $memberIdx = Ebms\Util::get_request_value('memberId', null);
    if (!$memberIdx)
        return 'No changes made';

    // Resolve member index into names to member ID
    $mm         = decodeMemberIdx($memberIdx);
    $memberId   = $mm['memberId'];
    $memberName = $mm['memberName'];

    $delBox  = Ebms\Util::get_request_value('delBox');
    $startDt = Ebms\Util::get_request_value('startDate', null);
    $endDt   = Ebms\Util::get_request_value('endDate', null);

    if ($delBox) {
        $rc = Ebms\delPrintMember($memberId);
        if ($rc)
            return "$memberName has been deleted from the wants-print list";
        else
            return "No changes to the wants-print list";
    }
    else {
        // Try an insert or update
        $rc = Ebms\setPrintMember($memberId, $startDt, $endDt);
        if (!$rc) {
            return "Unable to update the wants-print list for $memberName.  ".
                   "Is there a start date in YYYY-MM-DD format?";
        }
        return "$memberName updated in wants-print list";
    }
}

function pdq_ebms_api_tests() {
    $tests = array(
        array('Import articles from NLM', 'import'),
        array('View old import batch results', 'oldImport'),
        array('Set the status of an article', 'setStatus'),
        array('Add another comment to a status setting for an article',
            'addStatusComment'),
        array('View the current status of an article', 'curStatus'),
        array('View the status history for an article', 'viewStatus'),
        array('Add a descriptive tag to an article', 'addTag'),
        array('Add another comment to a descriptive tag', 'addTagComment'),
        array('Inactivate a descriptive tag and its comments', 'inactivateTag'),
        array('Execute a stored search', 'execSearch'),
        array('Run a packet printing job', 'printPkts'),
        array('View print jobs', 'getPrintouts'),
        array('Modify the list of member wanting print', 'setPrintMbrs'),
        array('Test to see if fpdf is accessible', 'fpdf'),
        // array('Pick a board', 'pickBoard'),
    );
    foreach ($tests as $test) {
        $options = array('query' => array('test' => $test[1]));
        $items[] = l($test[0], 'alantest', $options);
    }
    return array(
        '#theme' => 'item_list',
        '#type' => 'ol',
        '#title' => 'EBMS API Tests',
        '#items' => $items,
    );
}

// Form for importing from NLM
function ebms_testapi_import_form($form) {

    // drupal_set_title('Import tester');

    $form = array();
    $form['title'] = array(
        '#type' => 'markup',
        '#markup' => '<h3>Import Article Records from NLM</h3>'
    );

    $form['test'] = array(
        '#type' => 'hidden',
        '#value' => 'import',
    );

    // Get the latest cycle ID as a default
    $cycleId = Ebms\getCurrentCycleId();
    $form['cycleId'] = array(
        '#type' => 'textfield',
        '#size' => 6,
        '#title' => 'Cycle id',
        '#description' => 'Review cycle ID for import, default = latest cycle',
        '#default_value' => $cycleId,
        '#required' => true
    );

    $form['topicId'] = array(
        '#type' => 'textfield',
        '#size' => 6,
        '#title' => 'Topic id',
        '#description' => 'Numerical summary topic ID for import ' .
            '(I\'ll make a drop down only if we need it.)',
        '#required' => true,
    );

    $notListOpts = array(
        'Y' => 'Exclude articles from journals on "NOT lists"',
        'N' => 'Import anything, regardless of "NOT lists"'
    );
    $form['useNotList'] = array(
        '#type' => 'select',
        '#title' => 'Use not lists',
        '#description' =>
          'Consult the list of undesirable journals created by this ' .
          'topic\'s editorial board.',
        '#options' => $notListOpts,
        '#default_value' => 'Y',
        '#required' => true
    );

    $inputTypeOpts = array(
        'R' => 'Regular',
        'F' => 'Fast track',
        'S' => 'Special search'
    );
    $form['inputType'] = array(
        '#type' => 'select',
        '#title' => 'Import type',
        '#description' => 'What kind of import is this.',
        '#options' => $inputTypeOpts,
        '#default_value' => 'R',
        '#required' => true
    );

    $modeOpts = array(
        'test' => 'Test mode',
        'live' => 'Live mode'
    );
    $form['impmode'] = array(
        '#type' => 'select',
        '#title' => 'Import mode',
        '#description' => 'Test mode shows what would happen.<br />'.
        'Live mode updates the database and shows what actually happened.',
        '#options' => $modeOpts,
        '#default_value' => 'test',
        '#required' => true
    );

    addCommentToForm($form,
            'Store this comment in the record for this import batch');

    $form['upload'] = array(
        '#type' => 'managed_file',
        '#title' => 'Upload .txt search results file',
        '#description' => 'Upload a Pubmed Medline format search results file. Pubmed IDs will be extracted from the file and all articles in the file will be retrieved in XML format from NLM.  Click "Browse" to find the file, then "Upload" to upload it, then "Submit" to submit the entire form.',
        '#progress_indicator' => 'bar',
        '#upload_location' => 'temporary://'
    );

    $form['pmids'] = array(
        '#type' => 'textarea',
        '#title' => 'Pubmed article IDs',
        '#description' =>
       'Alternatively, enter one or more Pubmed IDs separated by spaces, newlines, or commas.',
        '#rows' => 2,
        '#cols' => 50,
        '#required' => true
    );

    $form['submit'] = array(
        '#type' => 'submit',
        '#value' => 'Submit'
    );

    return $form;
}

function ebms_testapi_oldimport_form($form) {

    $form = array();
    $form['title'] = array(
        '#type' => 'markup',
        '#markup' => '<h3>View values from an old import batch</h3>'
    );

    $form['batchId'] = array(
        '#type' => 'textfield',
        '#size' => 6,
        '#title' => 'Batch ID',
        '#description' => 'ID number of the existing batch import to display',
        '#required' => true
    );

    $form['submit'] = array(
        '#type' => 'submit',
        '#value' => 'Submit'
    );

    return $form;
}

// Form for setting the status of an article
function ebms_testapi_setstatus_form($form) {

    // Get the list of allowed status values as an array
    $stateNames = getAllStates();

    // Create the form
    $form = array();
    $form['title'] = array(
        '#type' => 'markup',
        '#markup' => '<h3>Set the Status of One Article</h3>'
    );

    $form['test'] = array(
        '#type' => 'hidden',
        '#value' => 'setStatus',
    );

    // Requires article or Pubmed id
    addArticleIdsToForm($form);

    // List of states to choose from
    addStateTypesToForm($form);

    // Board is sometimes required, sometimes not, depends on the state
    addBoardIdToForm($form,
                    'Board may or may not be required depending on state');

    // Topic ID may or may not be required
    $form['topicId'] = array(
        '#type' => 'textfield',
        '#size' => 6,
        '#title' => 'Summary topic ID number',
        '#description' => 'Some states require topics, some don\'t'
    );

    // Cycle ID required if this is a new topic
    $cycles = Ebms\Util::cycles();
    $form['cycleId'] = array(
        '#type' => 'select',
        '#title' => 'Cycle ID - Required if adding a new topic',
        '#description' => 'Every article/topic combination requires a ' .
            'cycle ID.  If you are adding a new topic, pick a cycle ID, '.
            'else leave it blank',
        '#options' => $cycles,
        '#default' => null,
        '#required' => false
    );

    // Optional comment
    addCommentToForm($form, 'Optional free text comment with the new status');

    addSubmitButtonToForm($form);

    return $form;
}

// Form for adding a comment to a status row of an article
function ebms_testapi_addstatuscomment_form($form) {

    // Create the form
    $form = array();
    $form['title'] = array(
        '#type' => 'markup',
        '#markup' => '<h3>Add a Comment to an Article Status</h3>'
    );

    $form['test'] = array(
        '#type' => 'hidden',
        '#value' => 'addStatusComment',
    );

    // Article identifier
    $articleId = addArticleIdsToForm($form);

    // Selection list of status values
    addStateTypesToForm($form);

    // Comment text
    $form['comment'] = array(
        '#type' => 'textarea',
        '#title' => 'Comment',
        '#description' => 'Free text comment to add to this status',
        '#rows' => 8,
        '#columns' => 50,
        '#required' => true
    );

    $form['submit'] = array(
        '#type' => 'submit',
        '#value' => 'Submit'
    );

    return $form;
}

// Form for viewing status history of an article
function ebms_testapi_getcurrentstatus_form($form) {
    // Get article ID or Pubmed ID
    $form = array();
    $form['title'] = array(
        '#type' => 'markup',
        '#markup' => '<h3>View the Current Status of an Article</h3>'
    );

    addArticleIdsToForm($form);

    $form['topicId'] = array(
        '#type' => 'textfield',
        '#title' => 'Summary topic name or ID',
        '#description' =>
          'Current status is now always set for one article/topic combination',
        '#size' => 50,
        '#maxlength' => 80,
        '#required' => true
    );

    $form['submit'] = array(
        '#type' => 'submit',
        '#value' => 'Submit'
    );

    return $form;
}

// Form for viewing status history of an article
function ebms_testapi_getstatushistory_form($form) {
    // Get article ID or Pubmed ID
    $form = array();
    $form['title'] = array(
        '#type' => 'markup',
        '#markup' => '<h3>View the Complete Status History for an Article</h3>'
    );

    addArticleIdsToForm($form);
    addBoardIdToForm($form,
            'Optionally restrict to particular board (or no board)');

    $form['topicId'] = array(
        '#type' => 'textfield',
        '#title' => 'Summary topic name or ID',
        '#description' => 'Optionally restrict status to one summary topic',
        '#size' => 50,
        '#maxlength' => 80,
        '#required' => false
    );

    addShowInactiveCheckboxToForm($form);

    $form['sortOrder'] = array(
        '#type' => 'textfield',
        '#title' => 'Sort keys',
        '#description' =>
          'Enter single letters (see below) to control sorting:<br />
 D = Date<br />
 S = Sequence number in status type table<br />
 B = Editorial Board name<br />
 T = Summary Topic name<br />
 U = User name (firstname lastname)',
        '#size' => 8,
        '#maxlength' => 8,
        '#default_value' => 'DBST',
        '#required' => true
    );

    $form['submit'] = array(
        '#type' => 'submit',
        '#value' => 'Submit'
    );

    return $form;
}

// For for adding a tag to an article
function ebms_testapi_addtag_form($form) {

    // Get all the elements
    $form = array();
    $form['title'] = array(
        '#type' => 'markup',
        '#markup' => '<h3>Add a Descriptive Tag to an Article</h3>'
    );
    addArticleIdsToForm($form);
    addTagIdToForm($form);

    // Topic ID may or may not be required
    $form['topicId'] = array(
        '#type' => 'textfield',
        '#size' => 6,
        '#title' => 'Summary topic ID number',
        '#description' => 'Some states require topics, some don\'t'
    );

    addCommentToForm($form, 'Optional comment to associate with the tag');
    addSubmitButtonToForm($form);

    return $form;
}

// For for adding another comment to a tag
function ebms_testapi_addtagcmt_form($form) {

    // Get all the elements
    $form = array();
    $form['title'] = array(
        '#type' => 'markup',
        '#markup' => '<h3>Add a Comment to an Existing Tag on an Article</h3>'
    );
    addArticleIdsToForm($form);
    addArticleTagIdToForm($form, 'Unique ID of the tag to add a comment to.  '
                               . '(Available in the tag history display)');
    addCommentToForm($form, 'New comment to associate with the tag', true);
    addSubmitButtonToForm($form);

    return $form;
}

// For inactivating a tag
function ebms_testapi_inactivatetag_form($form) {

    // Get all the elements
    $form = array();
    $form['title'] = array(
        '#type' => 'markup',
        '#markup' =>
 '<h3>Inactivate an Existing Tag on an Article and All Associated Comments</h3>'
    );
    addArticleIdsToForm($form);

    addArticleTagIdToForm($form, 'Unique ID of the tag to inactivate' .
                                 '(Available in the tag history display)');
    addShowInactiveCheckboxToForm($form);

    addSubmitButtonToForm($form);

    return $form;
}

/**
 * Form for search API test.
 */
function ebms_testapi_search_form($form) {

    $form = array();
    $form['title'] = array(
        '#type' => 'markup',
        '#markup' => '<h3>Test the search API</h3>'
    );

    $form['srchId'] = array(
        '#type' => 'textfield',
        '#title' => 'ebms_search.search_id',
        '#description' =>
                'Enter the internal ID of a stored search JSON string',
        '#size' => 10,
        '#maxlength' => 10,
        '#required' => true
    );
    addSubmitButtonToForm($form);

    return $form;
}

/**
 * Form to manage who gets printouts
 * XXX LEFT OFF HERE XXX
 */

/**
 * Form for packet printing test.
 */
function ebms_testapi_printpkt_form($form) {

    $form = array();
    $form['title'] = array(
        '#type' => 'markup',
        '#markup' => '<h3>Test packet printing</h3>'
    );

    addBoardIdToForm($form,
        'To print all packets for a board, select a  board name.');

    addMemberIdToForm($form,
        'To print all packets for a single board member, select a member  ' .
        'name.<br />' .
        'If the person is a member of more than one board, enter ' .
        'the board name above.<br />' .
        'To print just one packet for a board member, enter the packet ' .
        'ID below.');

    $form['packetId'] = array(
        '#type' => 'textfield',
        '#size' => 8,
        '#title' => 'Packet internal ID',
        '#description' => 'To print a single packet, regardless of board ' .
            'or board member, enter the packet internal integer ID.<br />' .
            'If a member name is entered, response sheets will be created, ' .
            'otherwise no response sheets will be created.',
        '#required' => false
    );

    $form['startDate'] = array(
        '#type' => 'textfield',
        '#size' => 16,
        '#title' => 'Only print packets created on or after this start date',
        '#description' => 'Optionally, print packets created on or after '.
            'the specified date, entered as "YYYY-MM-DD {HH:MM:SS}".<br />' .
            'Default for "board" or "package" printing is the datetime ' .
            'immediately after the last packet ' .
            'printing for the specified board or member.<br />' .
            'If a single packet ID is specified, start and end dates are ' .
            'ignored.<br />' .
            'Hours:Minutes:Seconds are optional.',
        '#required' => false
    );

    $form['endDate'] = array(
        '#type' => 'textfield',
        '#size' => 16,
        '#title' => 'Only print packets created before this end date',
        '#description' => 'Optionally, print packets created before '.
            'the specified date, entered as "YYYY-MM-DD {HH:MM:SS}".  ' .
            'Default is right now.',
        '#required' => false
    );

    $form['oldJobId'] = array(
        '#type' => 'textfield',
        '#size' => 6,
        '#title' => 'Old print job ID',
        '#description' =>
            'Optionally, the ID of a print job to be re-run.<br />' .
            'If supplied, all other parameters except Job mode and comment ' .
            'are ignored and the parameters of the old job are re-used.',
        '#required' => false
    );

    $modeOpts = array(
        'report' => 'Report mode',
        'test' => 'Test mode',
        'live' => 'Live mode'
    );
    $form['jobMode'] = array(
        '#type' => 'select',
        '#title' => 'Job mode',
        '#description' =>
          'Report mode only reports what would print.<br /> ' .
          'Test mode produces printouts but does not update packet printed data. <br />' .
          'Live mode produces printouts and records what was printed.',
        '#options' => $modeOpts,
        '#default_value' => 'report',
        '#required' => true
    );

    addCommentToForm($form,
            'Store this comment in the record for this print job');

    $form['submit'] = array(
        '#type' => 'submit',
        '#value' => 'Submit'
    );

    return $form;
}

/**
 * Form to request download of printouts
 */
function ebms_testapi_printouts_form($form) {

    $form = array();
    $form['title'] = array(
        '#type' => 'markup',
        '#markup' => '<h3>Download Printing Outputs</h3>'
    );

    $jobRows = getPrintJobData();
    $header = array('Job', 'Type', 'Date', 'Start', 'End', 'Board',
                    'Member', 'Packet', 'Mode', 'Status', 'Comment');
    $tblRows = array();
    foreach ($jobRows as $row) {
        $tblRows[] = array($row->print_job_id, $row->print_job_type_id,
               $row->print_dt, $row->packet_start_dt, $row->packet_end_dt,
               $row->board_name, $row->name, $row->packet_id,
               $row->mode, $row->status, $row->comment);
    }

    $form['jobTbl'] = array(
        '#theme' => 'table',
        '#caption' => 'Print Jobs',
        '#header' => $header,
        '#rows' => $tblRows,
    );

    $form['jobId'] = array(
        '#type' => 'textfield',
        '#size' => 8,
        '#title' => 'Job ID',
        '#description' => 'Enter a job number to download the output for ' .
            'job."
        '#required' => false
    );

    $form['submit'] = array(
        '#type' => 'submit',
        '#value' => 'Submit'
    );

    return $form;
}

/**
 * Simple form to add, edit, delete members from the members_want_print table.
 */
function ebms_testapi_setprintmbrs_form($form) {

    $form = array();
    $form['title'] = array(
        '#type' => 'markup',
        '#markup' => "<h3>Edit the Board Members Printing Table</h3>\n".
            '<p>Use this form to view the list of board members '.
            'who want printouts, to add a member to the list, to delete a '.
            'member from the list, or to change the start or end dates for '.
            'printing for a member.</p>',
    );

    // Load all of the rows from the table into an array
    $qry = db_select('ebms_member_wants_print', 'wp');
    $qry->join('users', 'usr', 'wp.board_member_id = usr.uid');
    $qry->fields('usr', array('name'));
    $qry->fields('wp', array('start_dt', 'end_dt'));
    $qry->orderBy('usr.name');
    // $rows = $qry->execute()->fetchAll();
    $rows = $qry->execute()->fetchAll(PDO::FETCH_ASSOC);

    $header = array('Board Member', 'Start Printing', 'End Printing');
    // Display them to the user
    $form['wantPrint'] = array(
        '#theme' => 'table',
        '#caption' => ' Members currently registered for printing',
        '#header' => $header,
        '#rows' => $rows,
    );

    $form['title'] = array(
        '#type' => 'markup',
        '#markup' => "<p>To add a board member to the list, select the board ".
            "member, enter start and end dates, and click Submit.<br />\n".
            "To delect a board member, select the member's name, check ".
            "the Delete checkbox, and click Submit.<br />\n".
            "To alter the dates, select the member, enter the new dates, ".
            "and click Submit.</p>\n".
            "<p>Only packets created after the Start Date will be printed ".
            "the board member.<br />\n".
            "The end date is a day on or after which, no printing will ".
            "be done for this board member.  Leave the end date empty if ".
            "printing should continue for the indefinite future.</p>\n",
    );

    addMemberIdToForm($form, 'Select a member to add, delete, or modify');

    $form['startDate'] = array(
        '#type' => 'textfield',
        '#size' => 16,
        '#title' => 'Only print packets created on or after this start date '.
                    '(YYYY-MM-DD {hh:mm:ss optional}',
        '#required' => false
    );

    $form['endDate'] = array(
        '#type' => 'textfield',
        '#size' => 16,
        '#title' => 'No printing for this member after this date '.
                    '(YYYY-MM-DD {hh:mm:ss optional}',
        '#required' => false
    );

    $form['delBox'] = array(
        '#type' => 'checkbox',
        '#title' => 'Delete the selected board member from printing',
        '#required' => false
    );

    $form['submit'] = array(
        '#type' => 'submit',
        '#value' => 'Submit'
    );

    return $form;
}


/**
 * Dumb tester for fpdf, just to see if it works.
 */
function ebms_testapi_fpdf_form($form) {

    $form = array();
    $form['title'] = array(
        '#type' => 'markup',
        '#markup' => '<h3>Create a simple pdf file with fpdf</h3>'
    );

    $form['submit'] = array(
        '#type' => 'submit',
        '#description' =>
            'Click submit to create a trivial file named /tmp/fpdftest.pdf',
        '#value' => 'Submit'
    );

    return $form;
}


/**
 * Create form elements to get an article ID or Pubmed ID.
 * Used in multiple tests.
 *
 *  @param array reference $form   Add the elements to this form.
 */
function addArticleIdsToForm(&$form) {

    $form['artId'] = array(
        '#type' => 'textfield',
        '#title' => 'EBMS article ID',
        '#description' => 'Enter an internal article ID, or use PMID below',
        '#size' => 10,
        '#maxlength' => 10,
        '#required' => false
    );
    $form['pmid'] = array(
        '#type' => 'textfield',
        '#title' => 'Pubmed citation ID',
        '#description' => 'Alternatively, enter a Pubmed ID (PMID)',
        '#size' => 10,
        '#maxlength' => 10,
        '#required' => false
    );
}


/**
 * Add a selection list of possible status values to a form
 *
 *  @param array reference $form   Add the elements to this form.
 */
function addStateTypesToForm(&$form) {

    // Get the list of allowed status values as an array
    $stateNames = array();
    $result = db_query('
        SELECT state_text_id
          FROM {ebms_article_state_type}
      ORDER BY sequence, state_id');
    foreach ($result as $row)
        $stateNames[] = $row->state_text_id;

    // Add them as a selection list to the form
    $form['status'] = array(
        '#type' => 'select',
        '#title' => 'Select a state',
        '#options' => $stateNames,
        '#required' => true
    );
}


/**
 * Read form elements to get an article ID from an article ID or Pubmed ID.
 *
 *  @param string ref $errMsg   Put error msg here, return 0.
 *
 *  @return int                 EBMS article ID.
 */
function getArticleId(&$errMsg) {

    $errMsg = null;
    $articleId = -1;

    // Read article identifiers
    $articleIdStr = Ebms\Util::get_request_value('artId');
    $pmidStr      = Ebms\Util::get_request_value('pmid');

    // Validate
    if (!$articleIdStr && !$pmidStr)
        $errMsg = 'You must supply an article ID or a Pubmed ID';

    if ($articleIdStr && $pmidStr)
        $errMsg = 'Please upply an article ID or a Pubmed ID, not both';

    if ($pmidStr) {
        // Translate PMID into article ID
        $articleId = Ebms\getArticleIdBySourceId($pmidStr);

        if (!$articleId) {
            $articleId = 0;
            $errMsg = "Pubmed ID '$pmidStr' not in the database";
        }
    }
    else {
        $articleId = intval($articleIdStr);
        if ($articleId == 0)
            $errMsg = "Article ID not a legal value";
    }

    // Does the articleId exist in the DB
    if (Ebms\verifyArticleId($articleId))
        $errMsg = "Article ID $articleId is not in the database";

    // Htmlify errors
    if (!is_null($errMsg))
        $errMsg = "<p>$errMsg</p>";

    return $articleId;
}

/**
 * Add a selection list of editorial boards to a form
 *
 *  @param array reference $form   Add the elements to this form.
 *  @param string $desc            Description of input element.
 */
function addBoardIdToForm(&$form, $desc) {

    // List of board names
    $boards = getAllBoards();

    $form['boardId'] = array(
        '#type' => 'select',
        '#title' => 'Editorial board name',
        '#description' => $desc,
        '#options' => $boards,
        '#required' => false
    );
}

/**
 * Add a selection list of descriptive tags to a form
 *
 *  @param array reference $form   Add the elements to this form.
 */
function addTagIdToForm(&$form) {

    // List of board names
    $tags = getAllTags();

    $form['tagId'] = array(
        '#type' => 'select',
        '#title' => 'Tag name',
        '#description' => 'Select a descriptive tag for an article',
        '#options' => $tags,
        '#required' => false
    );
}

/**
 * Add a selection list of board memberss to a form
 *
 *  @param array reference $form   Add the elements to this form.
 *  @param string $desc            Description of input element.
 */
function addMemberIdToForm(&$form, $desc) {

    // List of board names
    $members = getAllBoardMembers();

    $form['memberId'] = array(
        '#type' => 'select',
        '#title' => 'Board member name',
        '#description' => $desc,
        '#options' => $members,
        '#required' => false
    );
}

/**
 * Add a form element to get an article tag ID.
 *
 *  @param array reference $form   Add the element to this form.
 *  @param string $desc            Description to use on the form field.
 */
function addArticleTagIdToForm(&$form, $desc) {

    $form['articleTagId'] = array(
        '#type' => 'textfield',
        '#size' => 10,
        '#title' => 'Article Tag ID',
        '#description' => 'Unique ID of the tag to attach the comment to.  ' .
                          '(Available in the tag history display)',
        '#required' => true,
    );
}

/**
 * Add a checkbox to show or not show inactive items.
 *
 *  @param array reference $form   Add the elements to this form.
 */
function addShowInactiveCheckboxToForm(&$form) {

    $form['showInactive'] = array(
        '#type' => 'checkbox',
        '#title' => 'Show inactive information',
        '#description' => 'Check the box to include inactive status, tags and/or comments in result display',
        '#default_value' => true
    );
}

/**
 * Add a comment textfield.
 *
 *  @param array reference $form   Add the elements to this form.
 *  @param string $desc            Description to use on the form field.
 *  @param bool $required          Is a comment required here?
 */
function addCommentToForm(&$form, $desc, $required=false) {

    $form['comment'] = array(
        '#type' => 'textarea',
        '#title' => 'Comment',
        '#description' => $desc,
        '#maxlength' => 2000,
        '#required' => $required
    );
}

/**
 * Add submit button to form.
 *
 *  @param array reference $form   Add the elements to this form.
 */
function addSubmitButtonToForm(&$form) {

    $form['submit'] = array(
        '#type' => 'submit',
        '#value' => 'Submit'
    );

    return $form;
}

/**
 * Get an array of all possible status values from the database.
 *
 * @return array        All state names, in order.
 */
function getAllStates() {

    $stateTextIds = array();
    $result = db_query('
        SELECT state_text_id
          FROM {ebms_article_state_type}
      ORDER BY sequence, state_id');
    foreach ($result as $row)
        $stateTextIds[] = $row->state_text_id;

    return $stateTextIds;
}

/**
 * Get an array of all editorial boards from the database.
 *
 * @return array        All board names, in fixed order.
 */
function getAllBoards() {

    // Create array with blank first line
    $boardNames = array();
    $boardNames[] = '';

    $result = db_query('
        SELECT board_name
          FROM {ebms_board}
      ORDER BY board_name');
    foreach ($result as $row)
        $boardNames[] = $row->board_name;

    return $boardNames;
}

/**
 * Get an array of all board members from the database.
 *
 * @return array        All member names, in alpha order.
 */
function getAllBoardMembers() {

    // Create array with blank first line
    $memberNames = array();
    $memberNames[] = '';

    $result = db_query('
        SELECT name
          FROM {users} u
          JOIN {ebms_board_member} m
            ON u.uid = m.user_id
      ORDER BY u.name');
    foreach ($result as $row)
        $memberNames[] = $row->name;

    return $memberNames;
}

/**
 * Given a member selection from getAllBoardMembers, find the member
 * name and ID.
 *
 *  @param int $memberIdx   Index into array produced by getAllBoardMembers().
 *
 *  @return array           ('memberId'=>memberId, 'memberName'=>name).
 */
function decodeMemberIdx($memberIdx) {

    $result = array();

    if ($memberIdx) {
        // Convert member array index to name, then ID
        $memberList = getAllBoardMembers();
        $memberName = $memberList[$memberIdx];
        $memberId   = db_query(
            'SELECT uid FROM {users} WHERE name = :memberName',
            array(':memberName' => $memberName))->fetchField();
        $result['memberId']   = $memberId;
        $result['memberName'] = $memberName;
    }
    return $result;
}

// Return array of board name, ID, using addBoardIdToForm/getAllBoards
function boardRequest() {
    $allBoards  = getAllBoards();
    $boardNum   = Ebms\Util::get_request_value('boardId');
    $boardName  = $allBoards[$boardNum];
    $boardId    = Ebms\getBoardIdByName($boardName);
    if (!$boardId) {
        $boardName = null;
        $boardId   = null;
    }

    return array('boardName'=>$boardName, 'boardId'=>$boardId);
}

/**
 * Get stored print job information, in reverse job number order.
 *
 *  @param int $numJobs     How many to retrieve
 *  @param int $latest      ID of latest job to retrieve, null=latest possible
 *  @param string $status   Desired status, null = all
 */
function getPrintJobData($numJobs=60, $latest=null, $status=null) {

    // Fetch the data
    $qry = db_select('ebms_print_job', 'job');
    $qry->leftjoin('ebms_board', 'board', 'job.board_id = board.board_id');
    $qry->leftjoin('users', 'user', 'job.board_member_id = user.uid');
    $qry->fields('job', array('print_job_id', 'print_job_type_id', 'mode',
                 'print_dt', 'packet_start_dt', 'packet_end_dt',
                 'packet_id', 'status', 'comment'));
    $qry->fields('user', array('name'));
    $qry->fields('board', array('board_name'));

    // Conditions based on passed parms
    if (!is_null($latest))
        $qry->condition('job.print_job_id', $latest, '<=');
    if (!is_null($status))
        $qry->condition('job.status', $status, '=');

    // Limits
    $qry->range(0, $numJobs);
    $qry->orderBy('job.print_job_id', 'DESC');

    // Format into a selection list
    $result = $qry->execute();
    return $result->fetchAllAssoc('print_job_id');
}

/**
 * Get an array of all tags from the database.
 *
 * @return array        All tag names, in fixed order.
 */
function getAllTags() {

    // Create array with blank first line
    $tagNames = array();
    $tagNames[] = '';

    $result = db_query('
        SELECT tag_name
          FROM {ebms_article_tag_type}
      ORDER BY tag_name');
    foreach ($result as $row)
        $tagNames[] = $row->tag_name;

    return $tagNames;
}

// Return array of tag name, ID, using addBoardIdToForm/getAllBoards
function tagRequest() {
    $allTags   = getAllTags();
    $tagNum    = Ebms\Util::get_request_value('tagId');
    $tagName   = $allTags[$tagNum];
    $tagTextId = Ebms\getTagTextIdByName($tagName);
    if (!$tagTextId) {
        $tagName   = null;
        $tagTextId = null;
    }

    return array('tagName'=>$tagName, 'tagTextId'=>$tagTextId);
}

// Validate article tag exists
function checkArticleTag($articleId, $articleTagId, &$errMsg) {

    $errMsg = null;

    // Check for match on article ID and tag ID
    // This is to handle fumbles in the testing
    $qry = db_select('ebms_article_tag', 'tag');
    $qry->fields('tag', array('article_tag_id'));
    $qry->condition('tag.article_tag_id', $articleTagId)
        ->condition('tag.article_id', $articleId);
    $foundIt = $qry->execute()->fetchField();

    if (!$foundIt)
      $errMsg = "<p>Could not find a tag record for article ID='$articleId', "
              . "article tag ID='$articleTagId'";
    return;
}

// DEBUG
function ebms_pickBoard_form($form) {
    $form['test'] = array(
        '#type' => 'hidden',
        '#value' => 'pickBoard',
    );
    addBoardIdToForm($form, 'Testing select options');
    $form['submit'] = array(
        '#type' => 'submit',
        '#value' => 'Submit'
    );

    return $form;
}

function test_pickBoard_test() {
    $allBoards  = getAllBoards();
    $boardNum   = Ebms\Util::get_request_value('boardId');
    $boardName  = $allBoards[$boardNum];
    $boardId    = Ebms\getBoardIdByName($boardName);
    if (!$boardId)
        $boardId = null;
    $result = "boardNum=$boardNum, boardName=$boardName, boardId=$boardId";
    if (is_null($boardId))
        $result .= "  boardId is null";

    return $result;
}

/**
 * Dumb tester for fpdf, copied from /home/alan/test/php/fpdftest.php.
 * XXX We can remove this and related code when fpdf is working.
 */
function pdq_ebms_fpdf_test() {
    require('fpdf/fpdf.php');

    // Start a document orientation=Portrait, measure=points, papersize=Letter
    $pdf = new FPDF('P', 'pt', 'Letter');

    // Page 1
    $pdf->AddPage();

    $topMargin  = 70;
    $leftMargin = 70;
    $rightMargin = 540;
    $bottomMargin = 720;

    $pdf->SetTopMargin($topMargin);
    $pdf->SetLeftMargin($leftMargin);

    // Title of the response sheet, Arial, Bold, 16 point
    $pdf->SetFont('Arial','B',16);
    $pdf->setXY($leftMargin, $topMargin);
    $pdf->Write(15, 'TOENAIL CANCER (MARCH 2011)');
    $pdf->Line($leftMargin, $topMargin + 20, $rightMargin, $topMargin + 20);
    $pdf->Ln();

    $pdf->SetFont('Arial','B',14);
    $pdf->Write(35, 'ARTICLE 1 OF 7');
    $pdf->Ln();

    $pdf->SetLeftMargin($leftMargin + 10);
    $pdf->SetFont('Arial', '', 14);
    $pdf->Write(20, 'Blow j; Doe JT; Filbert FX');
    $pdf->Ln();
    $pdf->SetY($pdf->GetY() + 10);
    $pdf->Write(22, 'This is a fairly long title that I am going to allow to wrap
    around until it overflows the right margin to see what fpdf does with it.');
    $pdf->Ln();
    $pdf->SetY($pdf->GetY() + 10);
    $pdf->Write(22, 'Here is the journal brief citation');

    // ... More stuff here ...
    $pdf->Ln();
    $pdf->SetY($pdf->GetY() + 10);

    $pdf->SetLeftMargin($leftMargin);
    $pdf->SetX($leftMargin);
    $pdf->SetFont('Arial', 'B', 12);
    $pdf->Write(22, 'Summary Topic(s): ');
    $pdf->SetFont('Times', '', 13);
    $pdf->Write(22, 'Toenail Cancer, Anal Cancer, Colon Cancer');


    // $pdf->Output("/home/alan/test/test1.pdf");
    $pdf->Output("/tmp/fpdftest.pdf");

/*
 * Test of spawning with exec
 * whoami worked but the other has not yet.  Don't know why.
$output=array();
// $result=exec("pdfinfo '/var/www/html/ebmsdev/sites/ebms.nci.nih.gov/files267990_22095472.pdf'", $output);
$result=exec("whoami", $output);
$xxx=print_r($output, true);
logw("result of exec=$xxx");
*/
    return 'See if /tmp/fpdftest.pdf exists.';
}

/**
 * Downlad a print report or tar file to the workstation.
 *
 *  @param int $jobId       Job ID of successful print job
 */
function downloadPrintResults($jobId) {

    // Is this a tar file, a report, or a failure?
    $rptName = sprintf("/tmp/ebms/PrintJobs/PrintJobReport%05d.html", $jobId);
    $tarName = sprintf("/tmp/ebms/PrintJobs/PrintJob%05d.tar", $jobId);

    if (file_exists($rptName)) {
        $filepath = $rptName;
        drupal_add_http_header('Content-type', 'text.html');
    }
    else if (file_exists($tarName)) {
        $filepath = $tarName;
        $filename = pathinfo($tarName, PATHINFO_BASENAME);
        drupal_add_http_header('Content-type', 'x-tar');
        drupal_add_http_header('Content-disposition',
                               "attachment; filename=$filename");
    }
    else {
        // We're done
        return ("Sorry, job $jobId doesn't exist, produced no usable output," .
                " or the output is gone from the server.");
    }

    // Send what we've got
    drupal_send_headers();
    $fd = fopen($filepath, 'rb');
    if (!$fd)
        return "Error opening $filepath for reading.";
    while (!feof($fd))
        print(fread($fd, 1024));
    fclose($fd);

    // Exit causes data to be sent independently of drupal
    drupal_exit();
}
