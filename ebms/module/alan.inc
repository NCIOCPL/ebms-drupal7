<?php

// $Id$

/**
 * @file
 *
 * Testing for Alan.
 */
function pdq_ebms_alantest() {
    // Test
    // return '<i>Stub for Alan\'s testing</i>';
    require_once('logw.inc');
    // require_once('testimport.inc');
    require_once('EbmsArticle.inc');
    require_once('EbmsImport.inc');
    require_once('EbmsSearch.inc');
    pdq_ebms_debug('ALAN POST', $_POST);
    // return (tryIt());
    $test = Ebms\Util::get_request_value('test');
    $op = Ebms\Util::get_request_value('op');
    switch ($test) {
        case 'import':
            if ($op)
                return pdq_ebms_import_test();
            // print(drupal_realpath('public://anal-cancer-201202.rtf'));
            return drupal_get_form('ebms_testapi_import_form');
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
                                     $comment, $useNotList, null);

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

    // Text to add
    $comment = Ebms\Util::get_request_value('comment', null);

    // Set the requested status
    $stRowId = Ebms\setArticleState($articleId, $statusName, $topicId,
                                    $boardId, $comment);

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

    // Board or topic identifer
    $boardInfo = boardRequest();
    $boardId   = $boardInfo['boardId'];

    $topicStr = Ebms\Util::get_request_value('topicId', null);
    $topicId  = $topicStr ? intval($topicStr) : null;

    // If string but no ID, try converting from string
    if (!is_null($topicId) && $topicId == 0)
        $topicId = Ebms\getTopicIdByName($topicStr);

    // Get the current status
    $curStat = Ebms\getCurrentState($articleId, $topicId, $boardId);

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

$qryVal = print_r($qry, true);
logw($qryVal);
$qryVal = (string) $qry;
logw($qryVal);

    // Execute the search
    $result = $qry->execute();
    $rowCount = $result->rowCount();
    $html = "<p>Found $rowCount articles</p>\n<table border='1'>\n";
    $count = 0;
    $maxShow = $rowCount;
    if ($maxShow > 20)
        $maxShow = 20;
    for ($i=0; $i<$maxShow; $i++) {
        $artId = $result->fetchField();
        $artObj = new Ebms\EbmsArticle($artId);
        $html .= " <tr><td>" . $artId . "</td><td>"
              . $artObj->getBrfCite() . "</td></tr>\n";
    }
    $html .= "</table>\n";

    return $html;
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

function pdq_ebms_api_tests() {
    $tests = array(
        array('Import articles from NLM', 'import'),
        array('Set the status of an article', 'setStatus'),
        array('Add another comment to a status setting for an article',
            'addStatusComment'),
        array('View the current status of an article', 'curStatus'),
        array('View the status history for an article', 'viewStatus'),
        array('Add a descriptive tag to an article', 'addTag'),
        array('Add another comment to a descriptive tag', 'addTagComment'),
        array('Inactivate a descriptive tag and its comments', 'inactivateTag'),
        array('Execute a stored search', 'execSearch'),
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
        '#description' => 'Numerical summary topic ID for import (I\'ll make a drop down only if we need it.)',
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


    $modeOpts = array(
        'test' => 'Test mode',
        'live' => 'Live mode'
    );
    $form['impmode'] = array(
        '#type' => 'select',
        '#title' => 'Import mode',
        '#description' => 'Test mode shows what would happen. Live mode updates the actual database and shows what actually happened.',
        '#options' => array('test' => 'Test', 'live' => 'Live'),
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
        '#markup' => '<h3>Test the search API.</h3>'
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
        SELECT state_name
          FROM {ebms_article_state_type}
      ORDER BY sequence, state_id');
    foreach ($result as $row)
        $stateNames[] = $row->state_name;

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

    $stateNames = array();
    $result = db_query('
        SELECT state_name
          FROM {ebms_article_state_type}
      ORDER BY sequence, state_id');
    foreach ($result as $row)
        $stateNames[] = $row->state_name;

    return $stateNames;
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

logw($result);
    return $result;
}

