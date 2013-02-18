<?php

// $Id: forum.inc 10623 2012-09-20 12:08:10Z bkline $

/**
 * @file
 *
 * Implementation of EBMS Printing.  All we have right now is packet
 * printing.
 */

require_once('EbmsArticle.inc');
require_once('EbmsPrint.inc');

function pdq_ebms_print_packets($action=null) {
    drupal_set_title('Print Packets');
    switch($action) {
        case 'run':
            return drupal_get_form('pdq_ebms_prtpkts_run_form');
        case 'view':
            return drupal_get_form('pdq_ebms_prtpkts_view_form');
        case 'mbrs':
            return drupal_get_form('pdq_ebms_prtpkts_mbrs_form');
    }
    return '<i>No action selected</i>';
}

/************************************************
 * Submit a packet printing job
 ***********************************************/

/**
 * Construct the form to get parameters for a print job submission.
 */
function pdq_ebms_prtpkts_run_form() {

    drupal_set_breadcrumb(
        array(l('Home', 'home'), 'Print Packets')
    );

    $form = array();
    $form['title'] = array(
        '#type' => 'markup',
        '#markup' => '<h2>Create a packet printing job</h2>'
    );

    addBoardIdToForm($form,
        'To print all packets for a board, select a board name.');

    addMemberIdToForm($form,
        'To print all packets for a single board member, select a member ' .
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
            'print job for the specified board or member.<br />' .
            'If a single packet ID is specified, start and end dates are ' .
            'ignored.<br />' .
            'HH:MM:SS (Hours:Minutes:Seconds) are optional.',
        '#required' => false
    );

    $form['endDate'] = array(
        '#type' => 'textfield',
        '#size' => 16,
        '#title' => 'Only print packets created before this end date',
        '#description' => 'Optionally, print packets created before '.
            'the specified date, entered as "YYYY-MM-DD {HH:MM:SS}"<br />.' .
            'The default end date is right now.',
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
          'Test mode produces printouts but does not update ' .
          'packet printed history data. <br />' .
          'Live mode produces printouts and records what was printed.',
        '#options' => $modeOpts,
        '#default_value' => 'report',
        '#required' => true
    );

    addCommentToForm($form,
        'Store this optional comment in the record for this print job');

    addSubmitButtonToForm($form);

    return $form;
}

/**
 * Validate job submission form.
 */
function pdq_ebms_prtpkts_run_form_validate($form, &$form_state) {

    // Simplify reference
    $vals = $form_state['values'];

    // User specified a board, member, packet, or job ID
    $boardId   = $vals['boardId'];
    $memberIdx = $vals['memberId'];
    $packetId  = $vals['packetId'];
    $oldJobId  = $vals['oldJobId'];

    // Do we have enough to determine the job type?
    if (!$boardId && !$memberIdx && !$packetId && !$oldJobId) {
        form_set_error('',
            'Must specify board, board member, packet ID, or old job ID');
    }

    // Validate foreign keys before attempting an insert
    if ($packetId) {
        $val = db_query('SELECT packet_id FROM {ebms_packet}
                         WHERE packet_id = :packetId',
                        array(':packetId'=>$packetId))->fetchField();
        if (!$val)
            form_set_error('packetId',
                "Invalid Packet ID.  Packet $packetId does not exist.");
    }
    if ($oldJobId) {
        $val = db_query('SELECT print_job_id FROM {ebms_print_job}
                         WHERE print_job_id = :oldJobId',
                        array(':oldJobId'=>$oldJobId))->fetchField();
        if (!$val)
            form_set_error('oldJobId',
                "Invalid old job ID.  Job $oldJobId does not exist.");
    }

    // Date validation
    $startDt = $vals['startDate'];
    $endDt   = $vals['endDate'];
    if ($startDt) {
        // Returns 1 if start date < now, 0 if >, null if invalid date format
        $val = db_query('SELECT DATE(:startDt) <= NOW()',
                  array(':startDt'=>$startDt))->fetchField();
        if (is_null($val))
            form_set_error('startDate',
                "Invalid format start date '$startDt'");
        if ($val == 0)
            form_set_error('startDate',
                "Start date '$startDt' is in the future.  That won't work");
    }
    if ($endDt) {
        $val = db_query('SELECT DATE(:endDt) > :startDt',
                  array(':endDt'=>$endDt, ':startDt'=>$startDt))->fetchField();
        if (is_null($val))
            form_set_error('endDate',
                "Invalid format end date '$endDt'");
        if ($val == 0)
            form_set_error('endDate',
                "Start date '$startDt' is after end date '$endDt'");
    }
}

/**
 * Print packets form job submission.
 */
function pdq_ebms_prtpkts_run_form_submit($form, &$form_state) {
    global $user;
    $userId = $user->uid;

    // Simplify reference
    $vals = $form_state['values'];

    // Gather all input parms
    $boardInfo = boardRequest($vals);
    $boardId   = $boardInfo['boardId'];
    $memberIdx = $vals['memberId'];
    $packetId  = $vals['packetId'];
    $oldJobId  = $vals['oldJobId'];
    $startDt   = $vals['startDate'];
    $endDt     = $vals['endDate'];
    $jobMode   = $vals['jobMode'];
    $comment   = $vals['comment'];

    // Convert member selection to user ID
    if ($memberIdx) {
        $member = decodeMemberIdx($memberIdx);
        $memberId = $member['memberId'];
    }
    else
        $memberId = null;

    // Normalize empty values, e.g., '', to null
    if (!$packetId) $packetId = null;
    if (!$startDt)  $startDt  = null;
    if (!$endDt)    $endDt    = null;
    if (!$oldJobId) $oldJobId = null;
    if (!$jobMode)  $jobMode  = null;
    if (!$comment)  $comment  = null;

    // Derive the print job type
    // It's one of the three basic types, set here or it's
    //  taken from the job type of an old job ID
    if (!is_null($packetId))
        $jobType = 'packet';
    else if (!is_null($memberId))
        $jobType = 'package';
    else if (!is_null($boardId))
        $jobType = 'board';
    else {
        // There's only one possibility left.  Should have been validated okay.
        if (is_null($oldJobId)) {
            throw new Exception (
                "Fatal Error: Print validation should have checked for packet," .
                " member, board, or old job ID.  But none of those found.");
        }
        // Find the job type
        $jobType = db_query(
          'SELECT print_job_type_id FROM {ebms_print_job} WHERE print_job_id=:jobId',
          array(':jobId' => $oldJobId))->fetchField();
        if (!$jobType) {
            throw new Exception (
                "Fatal Error: Print found no job type for old job ID=$oldJobId");
        }
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
    if ($status == 'success') {
        // Send successful output to the workstation
        downloadPrintResults($jobId);
    }
    // XXX else maybe I should get the messages here
}


/************************************************
 * Reviewing previous packet printing jobs
 ***********************************************/

/**
 * Form to request download of printouts
 */
function pdq_ebms_prtpkts_view_form() {

    drupal_set_breadcrumb(
        array(l('Home', 'home'), 'View Packet Printing Jobs')
    );

    $form = array();
    $form['title'] = array(
        '#type' => 'markup',
        '#markup' => '<h2>Download Printing Outputs</h2>'
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
            'the job.',
        '#required' => false
    );

    addSubmitButtonToForm($form);

    return $form;
}

/**
 * Validate display of print job results
 */
function pdq_ebms_prtpkts_view_form_validate($form, $form_state) {

    // Ensure that any requested job ID exists
    $jobId = $form_state['values']['jobId'];

    if ($jobId) {
        // Check job
        $status = db_query(
            'SELECT status FROM {ebms_print_job} WHERE print_job_id = :jobId',
            array(':jobId' => $jobId))->fetchField();

        if (!$status)
            form_set_error('jobId',
                "Print job $jobId not found");
        if ($status != 'success')
            form_set_error('jobId',
                "Job $jobId is marked as '$status', no output is available");
    }
}

/**
 * Retrieve old print job results.
 */
function pdq_ebms_prtpkts_view_form_submit($form, $form_state) {

    $jobId = $form_state['values']['jobId'];

    if ($jobId) {
        // Sends data directly to browser, no return from here
        downloadPrintResults($jobId);
    }
}


/************************************************
 * Maintain the members_want_print_table
 ***********************************************/

/**
 * Simple form to add, edit, delete members from the members_want_print table.
 */
function pdq_ebms_prtpkts_mbrs_form() {

    drupal_set_breadcrumb(
        array(l('Home', 'home'), 'Manage Print Recipients')
    );

    $form = array();
    $form['title'] = array(
        '#type' => 'markup',
        '#markup' => '<h2>Edit the Board Members Printing Table</h2>',
    );

    $form['explain'] = array(
        '#type' => 'markup',
        '#markup' =>
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
    $rows = $qry->execute()->fetchAll(PDO::FETCH_ASSOC);

    // Display them to the user
    $header = array('Board Member', 'Start Printing', 'End Printing');
    $form['wantPrint'] = array(
        '#theme' => 'table',
        '#caption' => ' Members currently registered for printing',
        '#header' => $header,
        '#rows' => $rows,
    );

    $form['instruct'] = array(
        '#type' => 'markup',
        '#markup' => "<p>To add a board member to the list, select the board ".
            "member, enter start and end dates, and click Submit.<br />\n".
            "To select a board member, select the member's name, check ".
            "the Delete checkbox, and click Submit.<br />\n".
            "To alter the dates, select the member, enter the new dates, ".
            "and click Submit.</p>\n".
            "<p>Only packets created after the Start Date will be printed ".
            "for the board member.<br />\n".
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

    addSubmitButtonToForm($form);

    return $form;
}

/**
 * Add, edit, or delete a member in the ebms_member_wants_print table.
 */
function pdq_ebms_prtpkts_mbrs_form_submit($form, $form_state) {

    // Simplify reference
    $vals = $form_state['values'];

    // Has to be a member selected to do anything
    $memberIdx = $vals['memberId'];
    if (!$memberIdx) {
        drupal_set_message("No changes made to the wants-print list");
        return;
    }

    // Resolve member index into names to member ID
    $mm         = decodeMemberIdx($memberIdx);
    $memberId   = $mm['memberId'];
    $memberName = $mm['memberName'];

    $delBox  = $vals['delBox'];
    $startDt = $vals['startDate'];
    $endDt   = $vals['endDate'];

    if ($delBox) {
        $rc = Ebms\delPrintMember($memberId);
        if ($rc)
            drupal_set_message(
                    "$memberName has been deleted from the wants-print list");
        else
            drupal_set_message("No changes were made to the wants-print list");
    }
    else {
        // Try an insert or update
        $rc = Ebms\setPrintMember($memberId, $startDt, $endDt);
        if (!$rc) {
            drupal_set_message(
               "Unable to update the wants-print list for $memberName.  ".
               "Is there a start date in YYYY-MM-DD format?");
        }
        drupal_set_message("$memberName updated in wants-print list");
    }
}


/************************************************
 * Common subroutines for packet printing
 ***********************************************/

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

    $result = db_query("
        SELECT distinct u.name
          FROM {users} u
          JOIN {users_roles} ur
            ON u.uid = ur.uid
          JOIN {role} r
            ON r.rid = ur.rid
         WHERE r.name = 'board member'
           AND u.status = 1
      ORDER BY u.name");
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
function boardRequest($vals) {
    $allBoards  = getAllBoards();
    $boardNum   = $vals['boardId'];
    $boardName  = $allBoards[$boardNum];
    $boardId    = Ebms\getBoardIdByName($boardName);
    if (!$boardId) {
        $boardName = null;
        $boardId   = null;
    }

    return array('boardName' => $boardName, 'boardId' => $boardId);
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
        // Validation has checked for bad job ID or status != success
        // Other possibility is that sysadmin or cron removed it
        drupal_set_message(
            "Sorry, the output for job $jobId is gone from the server.");

        // We're done
        return;
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