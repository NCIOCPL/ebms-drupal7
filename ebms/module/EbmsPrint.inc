<?php

// $Id$

namespace Ebms;

require_once('EbmsArticle.inc');
require_once('fpdf/fpdf.php');
require_once('logw.inc');

// Constants for locating system dependent paths
define ('SERVER_BASE_PATH',     100);
define ('SERVER_SAVE_PATH',     101);
define ('SERVER_FILES_PATH',    102);

// Font info for response sheets
define ('DFT_FONT_HEIGHT', 13);         // Default font
define ('TITLE_FONT_HEIGHT', 18);       // Used in title
define ('BIG_FONT_HEIGHT', 16);         // Used in headers
define ('NARROW_HEIGHT', 17);           // Mostly for skinny line
define ('WIDE_HEIGHT', 24);             // Mostly for fat lines
define ('DFT_FONT_FAMILY', 'Times');    // Everything done with this font

// Margins for response sheets
// fpdf does not provide for a bottom margin
define ('TOP_MARGIN', 70);
define ('LEFT_MARGIN', 70);
define ('RIGHT_MARGIN', 540);

/**
 * This function was built when I was working outside of Drupal, using
 * the PDO database API.  I could replace this with calls to drupal_realpath(),
 * and may do that, but that's deprecated and I'd like a better way to do it.
 *
 * They're all defined here in order to simplify moving the program to
 * another computer.  This could be migrated to the database for the future,
 * but I don't know if it's any easier to maintain it there across
 * platforms.
 *
 *  @param int $whichPath   One of the SERVER...PATH constants.
 *  @param int $jobId       Used in constructing one of the paths.
 *
 *  @return string          The requested path.
 *
 *  @throws \Exception if unknown constant passed.
 */
function getServerPath($whichPath, $jobId=null) {

    // Base paths for output files - system dependent
    $host = gethostname();
    if (strstr($host, 'verdi'))
        $host = 'verdi';
    if ($host == 'verdi')
        $base = '/var/www/html/ebmsdev/sites/ebms.nci.nih.gov/';
    else
        $base = '/web/appdev/sites/ebms.nci.nih.gov/';

    switch ($whichPath) {
        case SERVER_FILES_PATH:
            return $base . 'files';

        case SERVER_BASE_PATH:
            // Same on all systems (so far)
            return '/tmp/ebms/PrintJobs';

        case SERVER_SAVE_PATH:
            if (is_null($jobId))
                throw new \Exception('getServerPath: missing requried jobId');

            $baseDir = getServerPath(SERVER_BASE_PATH);
            return (sprintf("$baseDir/PrintJob%05d", $jobId));

        throw new \Exception("getServerPath: unknown path id=$whichPath");
    }
}

/**
 * The following two functions should work for all tiers for ebms data.
 * XXX - However, there may be a safer way to do this - XXX
 */
function getFilesPath($scheme) {
    // Path to our Drupal site's directory for public managed files.
    if (substr($scheme, 0, 9) == 'public://')
        return getServerPath(SERVER_FILES_PATH);

    throw new \Exception(
     "EbmsPrint.inc.getFilesPath: scheme '$scheme' not currently recognized");
}

/**
 * Parse a filename and determine its relevant file type from the extension.
 *
 *  @param string $filename     Name to parse.
 *
 *  @return string              File type, one of 'word', 'pdf', 'text', etc.
 *
 *  @throws \Exception          If we can't figure it out.
 */
function chkFileExtent($filename) {

    // Use PHP filename parser, and convert extent to lower case
    $ext = strtolower(pathinfo($filename, PATHINFO_EXTENSION));

    switch ($ext) {
        case 'doc':
        case 'docx':
        case 'odt':
        case 'rtf':
            // These should all be printable by the same program
            return 'word';
        case 'pdf':
            return 'pdf';
        case 'text':
        case 'txt':
        case 'cmd':
            return 'text';
        case 'html':
        case 'htm':
            return 'html';
    }

    // We're out of luck
    return null;
}

/**
 * Heuristics for estimating number of pages.  It will be wildly inaccurate
 * for many files, but we're hoping it will be reasonable in the aggregate.
 *
 * Tune this based on results.
 *
 *  @param string $fileName     The name of the file.  We look at extension.
 *  @param int $fileSize        Size of the file.
 *
 *  @return                     Estimated number of pages.
 */
function estimatePageCount($fileName, $fileSize) {

    // Table of estimates of bytes per page
    //    key = file extension
    //    val = bytes per page estimate
    static $estimate = array(
        'doc'  => 20000,
        'docx' => 7000,
        'odt'  => 7000,
        'rtf'  => 7500,
        'pdf'  => 30000,
        null   => 15000
    );

    // Get the extension
    $fileExt = strtolower(pathinfo($fileName, PATHINFO_EXTENSION));

    // If we don't know this filetype, treat as null/unknown
    if (!array_key_exists($fileExt, $estimate))
        $fileExt = null;

    // Return the estimate
    return intval($fileSize / $estimate[$fileExt]);
}

/**
 * Get a parameter from the parms array, or default, or error.
 * Subroutine of createPrintJob().
 *
 * XXX - Obsolete.  Getting all data from the database now.
 * XXX - Keeping it for now in case we decide not to record jobs of
 * XXX -  type 'test' or 'report' and use this instead.
 *
 *  @param array $parms     Parameter name=value array.
 *  @param string $key      Key to look for.
 *  @param bool $required   True = Throw exception if parm not present
 *
 *  @return string          The value to use.
 *
 *  @throws \Exception      If required parm not present.
 */
function chkParm($parms, $key, $default, $required=true) {

    // If there's a value, get it
    if (array_key_exists($key, $parms))
        return $parms[$key];

    else if ($required)
        throw new \Exception(
                "EbmsPrintJob.chkParm(): Missing required parameter $key");
    else
        // Default is null
        return null;
}

/**
 * Get a user name from user id.
 * We ought to put this in common.inc.
 *
 *  @param int $userId  Search for this one.
 *  @return string      The name.
 */
function getNameByUid($userId) {
    $name = db_query(
        'SELECT name
           FROM {users}
          WHERE uid = :memberId',
         array(':memberId' => $userId))->fetchField();

    return $name;
}


/**
 * Factory to create an EbmsPrintJob.
 *
 *  @return EbmsPrintJob    Initialized job object
 */
function createPrintJob() {

    // Construct a new object.
    $pj = new EbmsPrintJob();

    // The current database date is the print date for this job
    $now = db_query('SELECT NOW()')->fetchField();

    // Look for a job waiting to run
    // Fatal error if none found
    $jobId = $pj->findJobToRun();
    $pj->load($jobId);

    // If this is a re-run of an old job, get the old job parms
    // $pj->old_job_id = $pj->chkParm($parms, 'old_job_id', false);
    if ($pj->old_job_id) {

        // Get the old job values, fatal error if not found
        $oldJob = new EbmsPrintJob();
        $oldJob->load($pj->old_job_id);

        // Load relevant parts into new job object
        // A reprint has some new values and some old ones
        $pj->print_job_type  = $oldJob->print_job_type;
        $pj->packet_start_dt = $oldJob->packet_start_dt;
        $pj->packet_end_dt   = $oldJob->packet_end_dt;
        $pj->board_id        = $oldJob->board_id;
        $pj->board_member_id = $oldJob->board_member_id;
        $pj->packet_id       = $oldJob->packet_id;
    }

    else {
        // If it's a 'package' job, find boards the member belongs to
        if ($pj->print_job_type == 'package') {
            // There must be a specified board member
            // The user interface should have taken care of this but ...
            if (!$pj->board_member_id)
                $pj->reportError(
                    "No board member specified for 'package' type job");

            $qry = db_select('ebms_board_member', 'bm');
            $qry->addField('bm', 'board_id');
            $qry->condition('bm.user_id', $pj->board_member_id, '=');
            $rows = $qry->execute()->fetchAll();

            // It is rare for there to be more than one, but allow it
            $boardIdList = array();
            foreach ($rows as $row)
                $boardIdList[] = $row->board_id;

            // If a board was specified, does it match?
            if ($pj->board_id) {
                if (!in_array($pj->board_id, $boardIdList)) {
                    $pj->reportError(
                        'Both a board ID and a board member ID were specified'.
                        ' in this print job, but the board member is not a '.
                        ' a member of that board. ');
                }
            }

            else {
                // If person is a member of two boards, the job must specify
                //  which one to use.  None was specified.
                if (count($boardIdList) > 1) {
                    $pj->reportError(
                        "Board member with ID={$pj->board_member_id} is a " .
                        "member of multiple boards.  Please specify which " .
                        "one to use in the print job.");
                }

                // No board specified, only one in the list, we've got it
                $pj->board_id = $boardList[0];
            }
        }

        // Determine any default dates
        // User who enters something other than the default dates 'live'
        //  affects future runs.  Be careful and know what you're doing!
        if (!$pj->packet_start_dt)
            $pj->packet_start_dt = $pj->getBoardStartPrintDate();
        if (!$pj->packet_end_dt)
            $pj->packet_end_dt = $now;

        // If board member specified, add some more checks
        if ($pj->board_member_id && $pj->print_job_type == 'board') {
            $pj->reportError(
                "Job specified a board member ID ({$pj->board_member_id})".
                " but job type board is for all members of the board,".
                " not for a single board member.  ".
                "Please resubmit without board member or for job type of".
                " 'package' or 'packet'.");
        }
    }

    // Dates okay?
    if ($pj->packet_end_dt < $pj->packet_start_dt)
        $pj->reportError("EbmsPrintJob end date '{$pj->packet_end_dt}' is " .
                         "before start_date '{$pj->packet_start_dt}'");

    // Printing now, regardless of any old job
    $pj->print_dt = $now;

    // We're starting the job
    $pj->status = 'in-process';

    // Update the new job in the database
    $pj->update();

    // Define server pathnames
    $pj->serverBasePath = getServerPath(SERVER_BASE_PATH);
    $pj->serverSavePath = getServerPath(SERVER_SAVE_PATH, $pj->print_job_id);

    // Create them - recurses downward as needed to build the tree
    $pj->createServerDirPath($pj->serverSavePath);

    // Define Windows workstation filenames and paths
    $pj->cmdFileName = 'PrintPackets.cmd';
    $pj->rptFileName = sprintf("PrintJobReport%05d.html", $pj->print_job_id);

    return $pj;
}

/**
 * Records everything we know of interest about a file that is to be
 * printed.  This object is a stand-in for the file in various lists.
 */
class EbmsPrintFile {

    public
        $boardMemberId,     // Who the file is to be printed for, may be null
        $drupalFileId,      // For drupal managed files, may be null
        $ebmsArticleId,     // For article citations
        $purpose,           // Why this article is to be printed:
                            //  'article', 'summary', 'response'
        $path,              // Fully qualified file path
        $description,       // String a human can read to see what this file is
                            //  Content depends on purpose and file contents
        $filesize,          // May help with predicting page count
        $filetype,          // 'pdf', 'word', 'text', 'html'
        $numPages;          // Estimated page count

    /**
     * Constructor sets empty fields
     *
     * Makes a copy of any passed EbmsPrintFile object.
     *
     *  @param EbmsPrintFile $fromEps   Copy from here if passed.
     */
    function __construct($fromEps=null) {

        if (!is_null($fromEps)) {
            // Copy from passed object
            $this->boardMemberId = $fromEps->boardMemberId;
            $this->drupalFileId  = $fromEps->drupalFileId;
            $this->ebmsArticleId = $fromEps->ebmsArticleId;
            $this->purpose       = $fromEps->purpose;
            $this->path          = $fromEps->path;
            $this->description   = $fromEps->description;
            $this->filesize      = $fromEps->filesize;
            $this->filetype      = $fromEps->filetype;
            $this->numPages      = $fromEps->numPages;
        }
        // Else all values are null by default
    }
} // class EbmsPrintFile

/**
 * Parameters controlling the operation of a print job.
 */
class EbmsPrintJob {
    private
        $errMsgs;           // Array of error messages

    public
        // Board members, packages, docs in job
        $boardMemberList,   // List of board members wanting print for this job
                            //  Not all of them will have anything ready
        $packageList,       // One package/board member needing printouts
        $docList,           // One EbmsPrintFile for each doc needing printing
                            //  Includes all docs, response sheets, report,
                            //  cmd file, articles, summaries
                            //  One file can appear many times, once/package
        $uniqPathList;      // One EbmsPrintFile for each unique file to print
                            //  Some docs are printed individually for each of
                            //  a number of board members

    public
        // Some special files
        $reportEpf,         // HTML report to user of what's in printout
        $printScriptEpf,    // Windows command file to print everything
        $tarPath;           // Full path to tar archive to download to Windows

    public
        // Important paths
        $serverBasePath,    // Job directory path
        $serverSavePath,    // Temporary files created here, beneath base path
        $cmdFileName,       // Name of the Windows command file to print docs
        $rptFileName;       // Name of the document with a full report

    public
        // These mirror the ebms_print_job table
        $print_job_id,      // Autoincremented unique print job ID
        $old_job_id,        // If this is a re-run, here what we're re-running
        $print_job_type,    // 'board', 'package', 'packet'
        $packet_start_dt,   // Datetime <= first packet in the printout
        $packet_end_dt,     // Datetime > last packet in the printout
        $print_dt,          // Datetime at start of job run
        $board_id,          // If printing is for one editorial board
        $board_member_id,   // ID of single board member requested by user
        $mode,              // 'live', 'test', 'report'
        $status,            // Status of the job queued ... success
        $comment;           // Optional free text description of the job

    /**
     * Constructor.
     */
    function __construct() {

        // Board members, packages, docs, unique docs, errors
        $this->boardMemberList = array();
        $this->packageList     = array();
        $this->docList         = array();
        $this->uniqPathList    = array();
        $this->errMsgs         = array();
    }

    /**
     * We store data in specific places.  Check to be sure that a specified
     * place exists, creating it if it doesn't.  It's a fatal error if we
     * can neither find nor create the directory.
     *
     *  @param string $serverPath   The place we put out files.
     */
    function createServerDirPath($serverPath) {

        // Is it already there?
        if (is_dir($serverPath))
            return true;

        // Try to create it using mkdir() with the recursive=true option
        // Note: Might want umask(0) to enable easy test/debug
        if (!mkdir($serverPath, 0777, true)) {
            // Fatal error, mkdir failed
            $this->reportError(
                "The program needs a directory at '$serverPath', " .
                "but could not find or create it.  Please " .
                "request help from computer support staff.");
        }

    }

    /**
     * Get the real path for a drupal managed file based on scheme.
     * 'public://' should be the only scheme needed (I think.)
     *
     *  @param string $uri          uri field from the file_managed table.
     *
     *  @param string $filename     Filename, without path.
     *
     *  @return string              Fully qualified file path.
     */
    function getFullPath($uri, $filename) {

        // Find the scheme
        $dir = getFilesPath($uri);
        return "$dir/$filename";
    }

    /**
     * Create a new print job to process any job waiting to run.
     *
     *  @return EbmsPrintJob object for the job.
     */
    public function findJobToRun() {
        try {
            $qry = db_select('ebms_print_job', 'job');
            $qry->addField('job', 'print_job_id');
            $qry->condition('job.status', 'queued', '=');
            $result = $qry->execute();

            // Should be exactly one
            $rowCount = $result->rowCount();
            if ($rowCount > 1)
                $this->reportError("EbmsPrintJob.initJob(): " .
                            "More than one job has status='queued'" .
                            "  Running again will (hopefully) clear it.");
            else if ($rowCount == 0)
                $this->reportError("EbmsPrintJob.initJob(): " .
                            "Packet printing software called, but no job" .
                            " is ready to be run.");
            else
                $jobId = $result->fetchField();
        }
        catch (\Exception $e) {
            $this->reportError("EbmsPrint.findJobToRun queue check: ", $e);
        }

        // Let's issue a warning if some jobs are in-process
        try {
            $qry = db_select('ebms_print_job', 'job');
            $qry->addField('job', 'print_job_id');
            $qry->condition('job.status', 'in-process', '=');
            $result = $qry->execute();
            $rowCount = $result->rowCount();

            if ($rowCount > 0) {
                $this->reportError(
                    "WARNING: There were $rowCount print job(s) in the ".
                    "'in-process' state at the outset of this job. " .
                    "The job(s) have been marked as failures.",
                    null, true);
            }
        }
        catch (\Exception $e) {
            $this->reportError("EbmsPrint.findJobToRun in-process check: ",$e);
        }
        // Note, job found but status not changed yet
        return $jobId;
    }

    /**
     * Load an EbmsPrintJob from a row in the database.
     *
     *  @param int $dbJobId     Database primary key.
     */
    public function load($dbJobId) {
        try {
            $row = db_query('
                SELECT *
                  FROM {ebms_print_job}
                 WHERE print_job_id = :jobId',
               array(':jobId' => $dbJobId))->fetchAssoc();

            // Has to be there
            if (!$row)
                $this->reportError(
                        "EbmsPrintJob.load(): Job $dbJobId not found");

            // Load 'er up
            $this->print_job_id    = $row['print_job_id'];
            $this->old_job_id      = $row['old_job_id'];
            $this->user_id         = $row['user_id'];
            $this->print_job_type  = $row['print_job_type_id'];
            $this->packet_start_dt = $row['packet_start_dt'];
            $this->packet_end_dt   = $row['packet_end_dt'];
            $this->print_dt        = $row['print_dt'];
            $this->board_id        = $row['board_id'];
            $this->board_member_id = $row['board_member_id'];
            $this->packet_id       = $row['packet_id'];
            $this->mode            = $row['mode'];
            $this->status          = $row['status'];
            $this->comment         = $row['comment'];
        }
        catch (\Exception $e) {
            $this->reportError("EbmsPrint.load()", $e, true);
        }
    }

    /**
     * Helper function for store and update to load fields array.
     *
     *  @return array   Array of column name = value for object to store.
     */
    private function loadFields() {
        $fields = array(
            'old_job_id'        => $this->old_job_id,
            'user_id'           => $this->user_id,
            'print_job_type_id' => $this->print_job_type,
            'packet_start_dt'   => $this->packet_start_dt,
            'packet_end_dt'     => $this->packet_end_dt,
            'print_dt'          => $this->print_dt,
            'board_id'          => $this->board_id,
            'board_member_id'   => $this->board_member_id,
            'packet_id'         => $this->packet_id,
            'mode'              => $this->mode,
            'status'            => $this->status,
            'comment'           => $this->comment
        );

        return $fields;
    }

    /**
     * Store the job - this is an initial store, not an update.
     * We shouldn't need to change the rows except to update the status.
     *
     *  @return int         New job ID, also stored in $this->print_job_id.
     */
    public function store() {
        // Some validation
        if (!is_null($this->print_job_id))
            $this->reportError(
                "EbmsPrintJob.store: Can't re-insert existing job " .
                 $this->print_job_id);

        try {
            // What we'll insert
            $fields = $this->loadFields();

            // Insert the new row, saving the primary key
            $qry = db_insert('ebms_print_job')
             ->fields($fields);

            $this->print_job_id = $qry->execute();
        }
        catch (\Exception $e) {
            $this->reportError("EbmsPrint.store()", $e);
        }

        return $this->print_job_id;
    }

    /**
     * Update the job - see also store.
     */
    public function update() {
        // Some validation
        if (is_null($this->print_job_id))
            $this->reportError(
                "EbmsPrintJob.update: Can't update.  New job has no ID");

        try {
            // What we'll update
            $fields = $this->loadFields();
            $qry = db_update('ebms_print_job')
             ->fields($fields)
             ->condition('print_job_id', $this->print_job_id, '=')
             ->execute();
        }
        catch (\Exception $e) {
            $this->reportError("EbmsPrint.update()", $e);
        }

        return $this->print_job_id;
    }

    /**
     * Set the status for the job in the database.
     *
     *  @param string $status   Must be one of the legal values.
     */
    function setJobStatus($status) {

        try {
            $qry = db_update('ebms_print_job')
             ->fields(array('status' => $status))
             ->condition('print_job_id', $this->print_job_id, '=')
             ->execute();
        }
        catch (\Exception $e) {
            $this->reportError(
                "EbmsPrint.setJobStatus: Unable to set status", $e);
        }

        // Successful
        $this->status = $status;
    }

    /**
     * Get a list of board member IDs who may need to have packets printed.
     * Being on this list only means that the board member wants packet
     * printouts at some time during the period covered by this print job.
     * It doesn't mean that:
     *    Any packets are ready to be printed.
     *    Any packets ready to print are assigned to him.
     *    Any packets assigned to him fit in his wants-print time frame.
     *
     *  @return array       Array of board member_ids.  May be empty.
     */
    public function getBoardMemberPrintUsers() {

        $memberIds = array();
        try {
            $qry = db_select('ebms_member_wants_print', 'wp');
            $qry->join('ebms_board_member', 'bm',
                    'wp.board_member_id = bm.user_id');
            $qry->addField('wp', 'board_member_id');
            $qry->distinct();
            $qry->condition('bm.board_id', $this->board_id, '=');
            $qry->condition('wp.start_dt', $this->packet_end_dt, '<=');
            $qry->condition(
                  db_or()->isNull('wp.end_dt')
                      ->condition('wp.end_dt', $this->packet_start_dt, '>')
               );
            $result = $qry->execute();

            // Save them in an array
            $rows = $result->fetchAll();
            foreach ($rows as $row) {
                $memberIds[] = $row->board_member_id;
            }
        }
        catch (\Exception $e) {
            $this->reportError("EbmsPrint.getBoardMemberPrintUsers()", $e);
        }

        return $memberIds;
    }


    /**
     * Find the datetime of the next general printing of packets for
     * for a board.  This is a datetime in the past that is
     * immediately after the last general printing for that board.
     *
     * Used when we're printing all packets since the last printing for
     * that board, or for any member of the board in case of a 'package'
     * type job.
     *
     *  @return string  Datetime of last printing for this board.
     *                  "YYYY-MM-DD HH:MM:SS" format.
     */
    public function getBoardStartPrintDate() {

        try {
            $maxEndDt = db_query("
                SELECT MAX(packet_end_dt)
                  FROM {ebms_print_job}
                 WHERE board_id = :boardId
                   AND mode = 'live'
                   AND status = 'success'",
               array(':boardId' => $this->board_id))->fetchField();

            if (!$maxEndDt) {
                // This should only ever happen if there's never been a
                //  successful live run
                // In that case select an arbitrary date? XXX CHECK THIS XXX
                return '2012-11-01 00:00:00';
            }

            return($maxEndDt);
        }
        catch (\Exception $e) {
            $this->reportError("EbmsPrint.getBoardStartPrintDate()", $e);
        }
    }

    /**
     * Determine what packets are in a package to be printed for one member.
     *
     *  @param int $memberId    The member whose package we want to construct.
     *  @param int $boardId     Needed if the member is on more than one
     *                            board (rare, maybe vanishingly so.)
     *  @param bool $always     True =
     *                            Don't care if packets were previously
     *                             printed for member.
     *                            Don't care about his printing prefs, we're
     *                             requesting his packets unconditionally.
     *                          False =
     *                            Don't reprint anything.  Default.
     *
     *  @return array           Array of packet IDs needing printing for
     *                            this member.  Empty array if there are none.
     */
    public function getMemberPackage($memberId, $boardId, $always=false) {

        // Subquery to restrict returns to packets
        //  never before printed for this member
        $subqry = db_select('ebms_packet_printed', 'pktp');
        $subqry->join('ebms_print_job', 'job',
                      'pktp.print_job_id = job.print_job_id');
        $subqry->addField('pktp', 'packet_id');
        $subqry->condition('pktp.board_member_id', $memberId, '=')
               ->condition('job.status', 'success', '=');

        // Get all the packets for this specific reviewer
        $qry = db_select('ebms_packet', 'pkt');
        $qry->join('ebms_packet_reviewer', 'rev',
                   'pkt.packet_id = rev.packet_id');
        $qry->join('ebms_topic', 'top',
                   'pkt.topic_id = top.topic_id');
        $qry->join('ebms_member_wants_print', 'mwp',
                   'rev.reviewer_id = mwp.board_member_id');
        $qry->addField('pkt', 'packet_id');
        $qry->condition('top.board_id', $this->board_id, '=')
         ->condition('pkt.created_at', $this->packet_start_dt, '>=')
         ->condition('pkt.created_at', $this->packet_end_dt, '<')
         ->condition('rev.reviewer_id', $memberId, '=')
         ->condition('pkt.active_status', 'A', '=')
         ->condition('mwp.start_dt', 'pkt.created_at', '<=')
         ->condition(
            db_or()->isNull('mwp.end_dt')
                   ->condition('mwp.end_dt', 'pkt.created_at', '>'));
        if (!$always)
            $qry->condition('pkt.packet_id', $subqry, 'NOT IN');
        $qry->orderBy('pkt.packet_id');

pdq_ebms_debug("SQL for package:", $qry);
pdq_ebms_debug("Parms:",
 "board={$this->board_id}  start={$this->packet_start_dt}  " .
 "end={$this->packet_end_dt}  member=$memberId");

        try {
            // Execute
            $rows = $qry->execute()->fetchAll();
        }
        catch (\Exception $e) {
            $this->reportError("EbmsPrint.getMemberPackage", $e);
        }

        // Assemble the packets for this member
        $packetList = array();
        foreach ($rows as $row) {
pdq_ebms_debug("Packaging packetId=", $row->packet_id);
            $packetList[] = $row->packet_id;
        }

        // Send the package home
        return $packetList;
    }

    /**
     * Get a list of files to print for one packet, in the order
     * that they should be printed.
     *
     * Called once without a board member ID for a 'packet' type job.
     *
     * Called repeatedly for each packet for a board member, and if more
     * than one board member, than for each board member and each of his
     * packets.
     *
     *  @param int $packetId    Packet unique ID.
     *  @param int $memberId    Board member for whom we're printing.
     *                           If present, we'll generate a response sheet
     *                            for each article in the packet.
     *                           Else we're printing a packet by itself, no
     *                            response sheet is required.
     *
     *  @return array           Array of EbmsPrintFile objects describing
     *                           each file to print.
     */
    public function getPacketPrintList($packetId, $memberId=null) {

        // Array of EbmsPrintFile objects to return
        $epfList = array();

        // Find the topic ID for this packet
        $topicId = db_query(
            'SELECT topic_id FROM {ebms_packet} WHERE packet_id = :packet',
            array(':packet' => $packetId))->fetchField();

        // XXX Create a separator page?  Mailing label?  Ask users XXX
        // Could use a different color from a different tray

        // Selection for summaries
        // XXX - Is there a better way to do it using drupal calls?

        $qry = db_select('ebms_packet_summary', 'psum');
        $qry->join('ebms_doc', 'doc', 'psum.doc_id = doc.doc_id');
        $qry->join('file_managed', 'fman', 'doc.file_id = fman.fid');
        $qry->fields('psum', array('doc_id'));
        $qry->fields('doc', array('description'));
        $qry->fields('fman',
                array('fid', 'filename', 'uri', 'filemime', 'filesize'));
        $qry->condition('psum.packet_id', $packetId, '=');

        try {
            $rows = $qry->execute()->fetchAll();
        }
        catch (\Exception $e) {
            reportError('EbmsPrint.getPacketPrintList()/Summary: Exception',
                         $e);
        }

        // There should be exactly one but I'm taking no chances
        foreach ($rows as $row) {

            // Construct an object to represent this file
            $epf = new EbmsPrintFile();
            $epf->boardMemberId = $memberId;
            $epf->drupalFileId  = $row->fid;
            $epf->purpose       = 'summary';
            $epf->path          = $this->getFullPath($row->uri,
                                                     $row->filename);
            $epf->description   = $row->description;
            $epf->filesize      = $row->filesize;

            // Figure out the file type
            $epf->filetype = $this->getFileType($epf->path, $row->filemime);

            // Invoke our heuristic for page counts
            $epf->numPages = estimatePageCount($epf->path, $epf->filesize);

            // Append it to the array
            $epfList[] = $epf;
        }

        // Selection for articles in the packet that have full text PDFs
        $qry = db_select('ebms_packet_article', 'pktart');
        $qry->join('ebms_article', 'art', 'pktart.article_id = art.article_id');
        $qry->join('file_managed', 'fman', 'art.full_text_id = fman.fid');
        $qry->fields('art', array('article_id', 'article_title'));
        $qry->fields('fman',
                     array('fid', 'filename', 'uri', 'filemime', 'filesize'));
        $qry->condition('pktart.packet_id', $packetId, '=')
            ->condition('pktart.drop_flag', 0, '=')
            ->isNotNull('art.full_text_id');

        try {
            $rows = $qry->execute()->fetchAll();
        }
        catch (\Exception $e) {
            reportError('EbmsPrint.getPacketPrintList()/Articles: Exception',
                         $e);
        }

        // First article in the packet is 1 of total num rows
        $packetPos = 1;
        $packetMax = count($rows);

        // Construct an EbmsPrintFile object for each one found
        foreach ($rows as $row) {

            $epf = new EbmsPrintFile();
            $epf->boardMemberId = $memberId;
            $epf->drupalFileId  = $row->fid;
            $epf->ebmsArticleId = $row->article_id;
            $epf->purpose       = 'article';
            $epf->path          = $this->getFullPath($row->uri,
                                                     $row->filename);
            $epf->filesize      = $row->filesize;
            $epf->numPages      = estimatePageCount($epf->path,
                                                    $epf->filesize);
            $epf->filetype      = $this->getFileType($epf->path,
                                                     $row->filemime);

            // Use the article title as the description
            $epf->description   = $row->article_title;

            // If there's a board member ID, create a response sheet for
            //   this file and append its EbmsPrintFile object to our list
            // EbmsPrintFile will be created in all modes but actual
            //   response sheet is not created if it's just a report
            if ($epf->boardMemberId) {
                $respEpf = $this->createResponseSheet($epf, $packetPos,
                                          $packetMax, array($topicId));
                $epfList[] = $respEpf;
            }

            // Followed by the article object
            $epfList[] = $epf;
            ++$packetPos;
        }

        return $epfList;
    }

    /**
     * Get a list of all of the files, one record per file, even if
     * the file is recorded multiple times.  This specifies the names
     * of files to be put into a tar file for transmission to the
     * workstation that prints them.
     *
     * ALL files will be listed in here, including response sheets,
     * PDF article texts, summaries, and any other packet associated
     * files, each listed only once.
     *
     *  @param array $epfList   Array of EbmsPrintFile objects.
     *
     *  @return array           Array of unique absolute file paths.
     */
    function getUniqueFilePaths($epfList) {

        // List to return
        $pathList = array();

        // Index of it
        $pathIndex = array();

        foreach ($epfList as $epf) {
            $path = $epf->path;
            if (!array_key_exists($path, $pathIndex)) {
                $pathList[] = $path;
                $pathIndex[$path] = 1;
            }
        }

        return $pathList;
    }

    /**
     * Create a printable PDF format response sheet for an article.
     * Store it where we're storing temp files for this print job.
     *
     *  @param object ref $epf  EbmsPrintFile object for the article.
     *  @param int $packetPos   Ordinal pos of article in packet, origin 1.
     *  @param int $packetMax   Count of articles in packet.
     *  @param array $topicIds  Topic(s) for use in header.  Can be more than
     *                           one.  First topic becomes the title.
     *
     *  @return                 New EbmsPrintFile object for the constructed
     *                           file.
     */
    function createResponseSheet($epf, $packetPos, $packetMax, $topicIds) {

        // Reviewer name tells who this response is from
        $reviewerName = getNameByUid($epf->boardMemberId);

        // Can't happen but checking is cheap
        if (!$reviewerName) {
            $this->reportError(
                "EbmsPrintJob.createResponseSheet(): " .
                " Attempt to print a response sheet but reviewer " .
                "'{$epf->boardMemberId}' not found.");
        }

        // Get all of the article data, has to be there
        $articleId = $epf->ebmsArticleId;
        $artObj = new EbmsArticle($articleId);

        // Article IDs
        $idStr = '';
        if ($artObj == 'Pubmed')
            $idStr = "PMID: " . $artObj->getSourceId();
        $idStr .= "  CMS ID: $articleId";

        // Title info
        $articleTitle = $artObj->getArticleTitle();
        $brfCitation  = $artObj->getBrfCite();

        // Up to three authors, combined into a single string
        $authors = $artObj->getAuthors(3);

        $authorStr = '';
        $authorCnt = 0;
        foreach ($authors as $authObj) {
            if ($authorCnt > 0)
                $authorStr .= '; ';
            if ($authObj->collective_name)
                $authorStr = $authObj->collective_name;
            else {
                $authorStr .= $authObj->last_name;
                $authorStr .= ' ';
                $authorStr .= $authObj->initials;
            }
            ++$authorCnt;
        }

        // Get the name(s) of the topic(s)
        $topicList = $artObj->getTopics();

        // Remember the first topic, it goes into the title
        $firstTopic  = $topicList[0]->topic_name;

        // Format the names into a comma delimited string
        $topicStringArray = array();
        foreach ($topicList as $topObj)
            $topicNames[] = $topObj->topic_name;
        $topicString = implode(', ', $topicNames);

        // PDF dynamic construction package
        require_once('fpdf/fpdf.php');

        // Create a pdf object
        // Portrait mode, measure using points, page size US Letter
        $pdf = new \FPDF('P', 'pt', 'Letter');

        // Define font before anything else, otherwise screwy errors result
        $pdf->setFont(DFT_FONT_FAMILY, '');

        // Add a page with the margins we want
        $pdf->AddPage();
        $pdf->setLeftMargin(LEFT_MARGIN);
        // $pdf->setRightMargin(RIGHT_MARGIN);
        $pdf->SetTopMargin(TOP_MARGIN);
        // There is no setBottomMargin call

        // Header for the sheet
        // XXX Is current date (MONTH YEAR) right?
        $curDate    = date("(F Y)");
        $sheetTitle = strtoupper("$firstTopic $curDate");
        $pdf->setFontSize(TITLE_FONT_HEIGHT);
        $pdf->Write(TITLE_FONT_HEIGHT, $sheetTitle);
        $pdf->setFontSize(BIG_FONT_HEIGHT);
        $pdf->Ln(NARROW_HEIGHT);
        $Y = $pdf->GetY();
        $pdf->Line(LEFT_MARGIN, $Y, RIGHT_MARGIN, $Y);

        // Reviewer name
        $pdf->Ln(NARROW_HEIGHT);
        $pdf->setFont(DFT_FONT_FAMILY, '');
        $pdf->setFontSize(DFT_FONT_HEIGHT);
        $pdf->Write(DFT_FONT_HEIGHT, "Reviewer: $reviewerName");
        $pdf->Ln(DFT_FONT_HEIGHT);

        // Article N of M
        $pdf->Ln(NARROW_HEIGHT);
        $pdf->setFontSize(DFT_FONT_HEIGHT);
        $pdf->setFont(DFT_FONT_FAMILY, 'B');
        $pdf->Write(DFT_FONT_HEIGHT, "ARTICLE $packetPos OF $packetMax");
        $pdf->setFont(DFT_FONT_FAMILY, '');
        $pdf->Ln(WIDE_HEIGHT);

        // Description of article, indented, starting with author line
        $pdf->setLeftMargin(LEFT_MARGIN + 15);
        $pdf->Write(DFT_FONT_HEIGHT, $authorStr);
        $pdf->Ln(BIG_FONT_HEIGHT);

        // Article title in italics
        $pdf->setFont(DFT_FONT_FAMILY, 'I');
        $pdf->Write(DFT_FONT_HEIGHT, $articleTitle);
        $pdf->Ln(WIDE_HEIGHT);

        // Journal brief citation
        $pdf->setFont(DFT_FONT_FAMILY, '');
        $pdf->Write(DFT_FONT_HEIGHT, $brfCitation);
        $pdf->Ln(WIDE_HEIGHT);

        // IDs
        $pdf->setLeftMargin(LEFT_MARGIN);
        $pdf->setFont(DFT_FONT_FAMILY, '');
        $pdf->Write(DFT_FONT_HEIGHT, $idStr);
        $pdf->Ln(WIDE_HEIGHT);

        // Back to the margin
        $pdf->setLeftMargin(LEFT_MARGIN);

        // Show all the topics here
        $pdf->setFont(DFT_FONT_FAMILY, 'B');
        $pdf->Write(DFT_FONT_HEIGHT, "Summary Topics(s): ");
        $pdf->setFont(DFT_FONT_FAMILY, '');
        $pdf->Write(DFT_FONT_HEIGHT, $topicString);

        // Add in the response fixed text found in all response sheets
        $pdf->Ln(NARROW_HEIGHT * 2);
        $this->createResponseFixedText($pdf);

        // Save the pdf to temp storage
        $fname = sprintf("%s/ResponseSheetArticle%d_Mbr%d.pdf",
                          $this->serverSavePath,
                          $epf->ebmsArticleId, $epf->boardMemberId);
        if ($this->mode != 'report') {
            $pdf->Output($fname, 'F');
        }
        // Only needed if we didn't output, but does no harm
        $pdf->Close();

        // Construct an EbmsPrintFile object for it
        $newEpf                = new EbmsPrintFile();
        $newEpf->boardMemberId = $epf->boardMemberId;
        $newEpf->purpose       = 'response';
        $newEpf->path          = $fname;
        $newEpf->description   = "Response sheet for $reviewerName (ID=" .
                                  $epf->boardMemberId .
                                  "): $sheetTitle<br />" . $epf->description;
        $newEpf->filetype      = 'pdf';
        $newEpf->numPages      = 2;

        return $newEpf;
    }

    /**
     * Create a checkbox with predetermined size and position text
     * after the box.  If the text contains a parenthesized section,
     * write it in italics.  [Assumes italics to end of text, change code
     * if that ceases to be true.]
     *
     * Subroutine of createResponseFixedText().
     *
     *  @param FPDF $pdf        Object to which the checkbox will be added.
     *                           Note: as of php 5.3 this is a pointer, not
     *                           a copy.
     *  @param string $text     Text to write.
     *  @param int $y           Upper left corner, vertical position.
     *                           Only needed for first line of a series of
     *                           lines.  Otherwise we position after the last
     *                           printed line
     */
    function chkLine($pdf, $text, $y=null) {

        static $side       = 10;   // Length in points of one side of the box
        static $boxSpace   = 16;   // Horiz. Offset in points to text start
        static $lineY      = 0;    // Vert. offset to where next line goes
        static $lineX      = 70;   // Left margin for box
        static $lineHeight = 24;
        static $wrapHeight = 18;

        // If $y specified, it becomes the new base
        if ($y)
            $lineY = $y;

        // Search for a parenthetical part to put in italics
        $parenPos = strpos($text, '(');
        if ($parenPos !== false) {
            $mainTxt  = substr($text, 0, $parenPos);
            $parenTxt = substr($text, $parenPos);
        }
        else {
            $mainTxt  = $text;
            $parenTxt = null;
        }

        // Draw the box
        $pdf->Rect($lineX + 5, $lineY, $side, $side);

        // Write the main text
        $pdf->SetFont(DFT_FONT_FAMILY, '', DFT_FONT_HEIGHT);
        $pdf->setX($pdf->getX() + $boxSpace);
        $pdf->Write($wrapHeight, $mainTxt);

        // Write parenthesized italic text
        if ($parenTxt) {
            $pdf->SetFont(DFT_FONT_FAMILY, 'I', DFT_FONT_HEIGHT);
            $pdf->Write($wrapHeight, $parenTxt);
        }

        // Ready for next line
        $pdf->Ln($lineHeight);
        $lineY = $pdf->GetY() + 5;
    }

    /**
     * Create the invariant text of a response sheet in PDF format.
     *
     *  @param object $pdf      (Pointer to) FPDF object to add the text to.
     */
    function createResponseFixedText($pdf) {

        // Initialization
        $leftMargin  = LEFT_MARGIN;
        $rightMargin = RIGHT_MARGIN;
        $startY      = $pdf->GetY() + 6;

        // Disposition lines
        $pdf->SetFont(DFT_FONT_FAMILY, 'B', DFT_FONT_HEIGHT);
        $pdf->Write(WIDE_HEIGHT, 'Disposition');
        $pdf->Ln(NARROW_HEIGHT);
        $pdf->SetFont(DFT_FONT_FAMILY, '', DFT_FONT_HEIGHT);
        $pdf->Write(WIDE_HEIGHT,
                    'Indicate how the article might affect the summary.');
        $pdf->Ln();
        $Y = $startY + NARROW_HEIGHT * 2;

        $dispositions = array(
            'Warrants no changes to the summary (indicate reason[s] for exclusion on the back)',
            'Deserves citation in the summary (indicate placement in the summary document)',
            'Merits revision of the text (indicate changes in the summary document',
            'Merits discussion'
        );

        $Y = $pdf->GetY();
        foreach ($dispositions as $disposition) {
            $this->chkLine($pdf, $disposition, $Y);
            $Y = null;
        }
        // Comments
        $pdf->SetFont(DFT_FONT_FAMILY, 'B', DFT_FONT_HEIGHT);
        $pdf->Write(WIDE_HEIGHT, 'Comments');
        $pdf->Ln(WIDE_HEIGHT * 2);

        $Y = $pdf->GetY();
        for ($i=0; $i<3; $i++) {
            $pdf->Line($leftMargin, $Y, $rightMargin, $Y);
            $Y += WIDE_HEIGHT;
        }

        // Levels of Evidence
        $pdf->Ln(WIDE_HEIGHT * 3);
        $pdf->Write(WIDE_HEIGHT, 'Levels of Evidence Information');
        $Y += WIDE_HEIGHT;
        $pdf->SetFont(DFT_FONT_FAMILY, '', DFT_FONT_HEIGHT);
        $pdf->Ln(NARROW_HEIGHT);
        $pdf->Write(WIDE_HEIGHT,
                'Enter the appropriate level of evidence for this article.');
        $Y += WIDE_HEIGHT * 2;
        $pdf->Line($leftMargin, $Y, $rightMargin, $Y);
        $Y += WIDE_HEIGHT;

        // Next page
        $pdf->AddPage();

        // Reasons for exclusion
        $reasons = array(
            'Already cited in the PDQ summary',
            'Not relevant to the PDQ summary topic',
            'Findings not clinically important',
            'Preliminary findings; need confirmation',
            'Provides no new information/novel findings',
            'Review/expert opinion/commentary (no new primary data)',
            'Inadequate study population (small number of patients; underpowered study; accrual target not met)',
            'Inadequate follow-up',
            'Inappropriate interpretation of subgroup analyses',
            'Inappropriate statistical analysis (incorrect tests; lack of intent-to-treat analysis)',
            'Inappropriate study design',
            'Missing/incomplete outcome data; major protocol deviations',
            'Randomized trial with flawed or insufficiently described randomization process',
            'Unvalidated outcome measure(s) used (e.g., unvalidated surrogate endpoint[s])',
            'Other (specify reason[s] below)'
        );

        // $pdf->Ln();
        $pdf->SetFont(DFT_FONT_FAMILY, 'B', DFT_FONT_HEIGHT);
        $pdf->Write(WIDE_HEIGHT, 'Reason(s) for Exclusion From PDQ Summary');
        $pdf->Ln(NARROW_HEIGHT);
        $pdf->SetFont(DFT_FONT_FAMILY, '', DFT_FONT_HEIGHT);
        $pdf->Write(NARROW_HEIGHT,
                    'If you indicated that the article warrants no changes ' .
                    'to the summary, indicate which of the reasons below ' .
                    'led to your decision to exclude the article.  You may ' .
                    'choose more than one reason.');
        $pdf->Ln(WIDE_HEIGHT);
        $Y = $pdf->GetY() + 6;

        // List of reasons
        foreach ($reasons as $reason) {
            $this->chkLine($pdf, $reason, $Y);
            $Y = null;
        }

        $Y = $pdf->GetY() + WIDE_HEIGHT;
        for ($i=0; $i<3; $i++) {
            $pdf->Line($leftMargin, $Y, $rightMargin, $Y);
            $Y += WIDE_HEIGHT;
        }
    }

    /**
     * Get all of the files that are needed for printing into a package.
     *
     * We'll use uncompressed tar as our packager.  Most of the files will
     * be .pdf, some may be .docx, both of which are usually compressed.
     *
     * If I'm wrong about that I'll switch to using compression, but if
     * I'm right, avoiding compression will save needless CPU time.
     *
     * The structure of the tar will be like:
     *   'PrintJobs/PrintJob00000/...'  Where ... are the file names.
     *
     * The package will be downloaded to the workstation for printing.
     *
     *  @param array $fileList  Array of EbmsPrintFile objects to tar.
     *
     *  @return                 Path to the tar file
     *                          More may be added to the tar file after this.
     */
    function packagePrintFiles($fileList) {

        // Used for tarring files in php
        require_once('Archive/Tar.php');

        // Use this as the directory path inside the tar file
        // It will be expanded in the Windows workstation
        // It need not mirror what's in the server path
        $newPath = sprintf("PrintJobs/PrintJob%05d", $this->print_job_id);

        // Initialize the tar file in a known place
        $tarPath = $this->serverSavePath . '.tar';
        $tarObj  = new \Archive_Tar($tarPath);

        // Add each of them to the archive
        foreach ($fileList as $prtFile) {
            $oldPath = dirname($prtFile);
            $tarObj->addModify(array($prtFile), $newPath, $oldPath);
        }

        return $tarPath;
    }

    /**
     * Create a Windows command script for printing the contents of an
     * archive on the command line.  The script will eventually be
     * included in the tar archive.
     *
     * In report mode we don't actually create the script, just a stand
     * in EbmsPrintFile object for it.
     *
     *  @param array $fileList  Array of EbmsPrintFile objects.
     *
     *  @return an EbmsPrintFile object representing the script.
     */
    function createPrintScript($fileList) {

        // Get the output directory
        $outDir = $this->serverSavePath;

        // Name of the file
        $scriptName = "$outDir/{$this->cmdFileName}";

        // Construct an object to represent this file
        $scriptEpf       = new EbmsPrintFile();
        $scriptEpf->path = $scriptName;
        $scriptEpf->description =
            "Windows printing script for EBMS Print Job {$this->print_job_id}";
        $scriptEpf->filetype = 'text';

        // If we're in report mode, we're done.  All we need is the epf
        if ($this->mode == 'report')
            return $scriptEpf;

        // Create the output file
        $fp = fopen($scriptName, 'w');

        // Prolog
        fwrite($fp, "@echo off\n");
        fwrite($fp, "if %1. == . goto usage\n");
        fwrite($fp, "if %1. == howmany. goto showcount\n");
        fwrite($fp, "if %2. == . goto L1\n");

        // Table of gotos to allow the start of printing at numbered item
        $docCount = count($fileList);
        for ($i=1; $i<=$docCount; $i++)
            fwrite($fp, "if %2. == $i. goto L$i\n");
        fwrite($fp, "goto usage\n");

        // Actual print commands using file type specific print software
        $scriptIdx = 1;
        foreach ($fileList as $epf) {

            $fname = basename($epf->path);
            $ftype = $epf->filetype;

            // Setup
            fwrite($fp, ":L$scriptIdx\n");
            fwrite($fp, "if %3. == $scriptIdx. goto :done\n");

            // Document type specific printing command
            switch ($ftype) {
                case 'pdf':
                case 'application/pdf':
                    fwrite($fp, "call pdfprint -duplex 2 -printer %1 $fname\n");
                    break;
                case 'word':
                    fwrite($fp, "call abiword -p %1 $fname\n");
                    break;
                case 'text':
                    fwrite($fp, "copy $fname %1\n");
                    break;
                default:
                    $this->reportError("EbmsPrint.createPrintScript: " .
                            "Unknown filetype '$ftype' in file '$fname'");
            }
            ++$scriptIdx;
        }
        fwrite($fp, "goto done\n");

        // Usage
        $usageMsg = <<<EOS
:usage
echo usage: PrintPackets path-to-printer [first [last]]
echo    or: PrintPackets howmany
echo     (to show how many files the script has without printing anything)
echo  e.g.: PrintPackets \\CIPSFS1\HP8100
echo    or: PrintPackets \\CIPSFS1\HP8100 201 400
echo     (to print the second 200 files)
EOS;
        fwrite($fp, $usageMsg);

        // Count requested
        fwrite($fp, "\n:showcount\n");
        fwrite($fp, "echo This script contains $docCount files\n");

        fwrite($fp, ":done\n");
        fwrite($fp, "Processing completed");

        return $scriptEpf;
    }

    /**
     * Error routine.
     *
     * We might change this later to a more Drupalized approach.
     *
     *  @param string $msg      Error to report.
     *  @param Exception $e     Exception object, if any.
     *  @param bool $fatal      True=cleanup and abort.
     */
    function reportError($msg, $e=null, $fatal=true) {

        // Prevent recursion on errors
        static $inError = false;

        // Extract exception message
        if (!is_null($e))
            $msg = $msg . ': Exception: ' . $e->getMessage();

        // Add the message to our list of messages
        $this->errMsgs[] = $msg;
        $msgs = implode(' / ', $this->errMsgs);

        // Check for recursion
        if ($inError) {
            $msgs = 'Loop in reportError' . ': ' . $msgs;
            pdq_ebms_debug("Fatal error: jobId={$this->print_job_id}", $msgs);
            throw new \Exception("Fatal error: '$msgs'");
        }
        $inError = true;

        // If fatal error, cleanup
        if ($fatal) {

            // Cleanup and mark the job as failed
            $this->cleanup('failure');

            pdq_ebms_debug("Fatal error: jobId={$this->print_job_id}", $msgs);

            exit("<p>Packet printing for job {$this->print_job_id} " .
                 "failed.  See errors below:</p>\n<p>$msgs</p>");
        }
        $inError = false;
    }

    /**
     * Produce a list of documents in a package.
     * Use it to figure out where we were when a printer error occurred.
     * Also used in a test run to see what would have printed.
     *
     *  @param array $fileList  Array of all EbmsPrintFile objects for job,
     *                           in the correct order of their printing.
     *
     *  @return EbmsPrintFile for the file list itself.
     */
    function createPrintReport($fileList) {
        try {
            // Displayable board member info
            if ($this->board_member_id) {
                // Get the board member name
                $memberName = getNameByUid($this->board_member_id);
                $showMember = "$memberName ($this->board_member_id)";
            }
            else
                $showMember = "Not specified";

            // Displayable board info
            $boardName = getBoardNameById($this->board_id);
            $showBoard = "$boardName ($this->board_id)";

            // Estimate stats
            $docCount  = count($fileList);
            $pageCount = 0;
            for ($i=0; $i<$docCount; $i++) {
                $pageCount += $fileList[$i]->numPages;
            }

            // Output file
            $path = $this->serverSavePath . '/' . $this->rptFileName;
            $fp   = fopen($path, 'w');

            // Header
            $hdr = <<<EOS
<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Strict//EN"
                      "http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd">
<html xmlns="http://www.w3.org/1999/xhtml" xml:lang="en" lang="en">
<head>
 <title>Report of documents included in print job {$this->print_job_id}</title>
 <style type='text/css'>
  li {margin-top: 8pt; margin-bottom: 8pt;}
  li.respSheet {background-color: #A9D0F5;}
 </style>
</head>
<body>
<h2>Documents included in Print Job {$this->print_job_id}</h2>

<h3>Print Job Parameters</h3>
<p>This job was run with the following parameters:</p>
<table border='2'>
 <tr><th>Parameter</th><th>Value</th></tr>
 <tr><td>Job ID</td><td>$this->print_job_id</td></tr>
 <tr><td>Old job ID</td><td>$this->old_job_id</td></tr>
 <tr><td>Job type</td><td>$this->print_job_type</td></tr>
 <tr><td>Start date</td><td>$this->packet_start_dt</td></tr>
 <tr><td>End date</td><td>$this->packet_end_dt</td></tr>
 <tr><td>Print date</td><td>$this->print_dt</td></tr>
 <tr><td>Board ID</td><td>$showBoard</td></tr>
 <tr><td>Board member ID</td><td>$showMember</td></tr>
 <tr><td>Mode</td><td>$this->mode</td></tr>
 <tr><td>Comment</td><td>$this->comment</td></tr>
</table>

<h3>Print Job Statistics</h3>
<p>Number of documents to print: $docCount</p>
<p>Wild estimate of page count: $pageCount</p>

<h3>Documents to be Printed</h3>
<p>
<p>The following documents will be printed in the order that they appear
in the list.</p>
<p>Any highlighted documents are system generated response sheets, each
of which is a response sheet for the immediately following article
document.</p>
<ol>
EOS;
            fwrite($fp, $hdr);

            // Info about each file
            foreach ($fileList as $epf) {
                $fname = basename($epf->path);
                if ($epf->purpose == 'response')
                    $classId = " class='respSheet'";
                else
                    $classId = '';

                fwrite($fp,
                    "<li$classId>$fname:</br> {$epf->description}</li>\n");
            }

            // Trailer
            fwrite($fp, "</body>\n</html>\n");
            fclose($fp);

            // Create an object for the report to get it into the tar file
            $epf = new EbmsPrintFile();
            $epf->purpose  = 'control';
            $epf->path     = $path;
            $epf->filetype = 'html';
        }
        catch (\Exception $e) {
            $this->reportError("EbmsPrintJob.createPrintReport(): " .
                        "Exception raised writing report file", $e);
        }
        return $epf;
    }

    /**
     * Determine the type of file for printing purposes.
     * It must be one of our supported types, else error.
     *
     *  @param string $filename     OS file name.
     *  @param string $filemime     Mime type stored in drupal.
     *
     *  @return string              Type of the file for printing purposes.
     */
    private function getFileType($filename, $filemime) {

        // Allowed file types
        static $okTypes = array('word', 'pdf', 'html', 'text');

        // Use the file mimetype, if it's usable, else file name
        $filetype = null;
        switch ($filemime) {
            case 'application/msword':
            case 'application/vnd.openxmlformats-officedocument.' .
                 'wordprocessingml.document':
                $filetype = 'word';
                break;
            case 'application/pdf':
            case 'application/x-pdf':
                $filetype = 'pdf';
                break;

            default:
                // Use the extension to determine filetype
                $filetype = chkFileExtent($filename);
        }

        // Validate.  The use of reportError() mandates putting this
        //  in the class
        if (!in_array($filetype, $okTypes))
            $this->reportError(
             "EbmsPrint.getFileType(): Unknown filetype for file '$filename'");

        return $filetype;
    }

    /**
     * Update the record of what packets were printed for each board member.
     * Only call this if in live mode and job was successful.
     *
     * On a reprint of a live mode job to another live move job, there will
     * be two rows for some or all of the board_member_id / packet_id combos,
     * but each will have a different job_id.  I _think_ that's what we
     * want.
     */
    function recordPrintedPackets() {

        // Prepare the statement
        $qry = db_insert('ebms_packet_printed', 'pktptd')
         ->fields(array('board_member_id', 'packet_id', 'print_job_id'));

        // Build an array of value arrays, one per row to insert
        $values = array();
        foreach ($this->packageList as $memberId => $packetList) {
            foreach ($packetList as $packetId) {
                $values[] = array(
                    $memberId, $packetId, $this->print_job_id);
            }
        }

        // Execute a multi-insert
        // Drupal with MySQL is supposed to wrap it in a transaction
        $qry->execute();
    }

    /**
     * Close, commit, report.
     *
     *  @param string status    Final status for the job.
     */
    function cleanup($status) {

        // Set the final status
        $this->status = $status;

        // Preserve any error messages as part of the comment text
        if (count($this->errMsgs) > 0) {
            $errStr = "ERRORS: " . implode('; ', $this->errMsgs);
            $this->comment .= "  [$errStr]";
        }

        // Update the job info - unless we didn't get far enough to have a job
        if ($this->print_job_id) {
            $this->update();
        }
        else {
            // Primary way we could get here is if there is more than one
            //  job row in the queued state.
            // Fail all of them.
            $qry = db_update('ebms_print_job')
             ->fields(array('status' => 'failure'))
             ->condition('status', array('queued', 'in-process', 'IN'));
            $qry->execute();
        }

        // On success, remove any no longer needed files
        // This will remove the temporary dir and all files, leaving either
        //   a single tar file, or a single report file
        if ($status == 'success') {
            // For report type jobs, only need the html file, move it up
            if ($this->mode == 'report') {
                $oldPath = $this->serverSavePath . '/' . $this->rptFileName;
                $newPath = $this->serverBasePath . '/' . $this->rptFileName;
                rename($oldPath, $newPath);
            }
            // For all job types, remove the now unneeded directory
            $rmPath  = $this->serverSavePath . '/*';
            $rmFiles = glob($rmPath);
            foreach($rmFiles as $fname)
                unlink($fname);
            rmdir($this->serverSavePath);
        }

        // Else if no outputs were created, we also don't need the directory
        else {
            $rmPath  = $this->serverSavePath . '/*';
            $rmFiles = glob($rmPath);
            if (count($rmFiles) == 0)
                rmdir($this->serverSavePath);
            // Else we leave the stuff for debugging
        }
    }
} // class EbmsPrintJob

/**************************************************************
 * This is the routine to launch and run a print job.
 *
 * It finds the job to run in the ebms_print_job table with a
 * status = 'queued' and runs it.
 *************************************************************/
function runPrintJob() {

    // Create a print job from a recently created row in the table
    $pj = createPrintJob();

    switch ($pj->print_job_type) {

        case 'packet':
            // If we're printing documents in a specified packet, we know
            //  what docs to assemble regardless of who wants them.
            // Get the list of docs.  If the member ID is not null,
            //  response sheet EPFs will be interspersed, one before
            //  each article pdf EPF
            $pj->docList =
                $pj->getPacketPrintList($packetId, $pj->board_member_id);
            break;

        case 'package':

            // If we're printing one board member's package, we already
            //  know who it is.
            // Find his package, if any, then his docs, if any
            // Get the packets for him within the packet print dates
            // Don't care if he's already seen them or not
            $package = $pj->getMemberPackage($pj->board_member_id,
                                             $pj->board_id, true);
            foreach ($package as $packetId) {
                $docs = $pj->getPacketPrintList($packetId,
                                                $pj->board_member_id);
                $pj->docList = array_merge($pj->docList, $docs);
            }
            break;

        case 'board':
            // For a 'board' type job, find board members who need printing
            // Then find their packages, if any
            // Then their documents, if any
            $pj->boardMemberList = $pj->getBoardMemberPrintUsers();

            // If no one on the board wants printing, we're done
            if (count($pj->boardMemberList) == 0) {
                $pj->reportError(
                        'No members of the board want printing at this time');
            }

            // If job is a reprint, force printing even if the docs were
            //  already printed for this board
            $forcePrint = $pj->old_job_id ? true : false;

            // Build list of packages needed by those board members
            foreach ($pj->boardMemberList as $memberId) {
                $pj->packageList[$memberId] =
                    $pj->getMemberPackage($memberId,
                                          $pj->board_id, $forcePrint);
            }

            // Build list of EbmsPrintFile objects for all files in all
            //  packets in all packages
            foreach ($pj->packageList as $package) {
                foreach ($package as $packetId) {
                    $docs = $pj->getPacketPrintList($packetId, $memberId);
                    $pj->docList = array_merge($pj->docList, $docs);
                }
            }
            break;

        default:
            throw new \Exception(
                    "Unknown print job type '{$pj->print_job_type}'");
    }

    // Were there any docs ready to print?
    if (count($pj->docList) == 0) {
        $msgs = 'There are no documents ready for printing at this time ' .
                'for this job spec.';
        $pj->reportError($msgs);
    }

    // Construct report html showing what will be printed, in what order
    // Report is stored in tmp storage and an object representing it returned
    $pj->reportEpf = $pj->createPrintReport($pj->docList);

    // Construct print command script and EbmsPrintFile object for it
    // In report mode, just creates the epf, no actual file is output
    $pj->printScriptEpf = $pj->createPrintScript($pj->docList);

    // Both files have to be downloaded to the workstation, add to the doclist
    $pj->docList[] = $pj->reportEpf;
    $pj->docList[] = $pj->printScriptEpf;

    // For runs other than just reporting
    if ($pj->mode != 'report') {
        // Add each unique full file content to one tar archive
        $pj->uniqPathList = $pj->getUniqueFilePaths($pj->docList);
        $pj->tarPath = $pj->packagePrintFiles($pj->uniqPathList);

        if ($pj->mode == 'live') {
            // Update the database showing what has been printed
            $pj->recordPrintedPackets();
        }
    }

    // That's it!
    $pj->cleanup('success');

    return $pj->status;
}
