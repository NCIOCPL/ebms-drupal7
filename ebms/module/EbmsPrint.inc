<?php

namespace Ebms;

require_once('fpdf/fpdf.php');
require_once('logw.inc');

// Constants for locating system dependent paths
define ('SERVER_SETTINGS_PATH', 100);
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
define ('TOP_MARGIN', 70);
define ('LEFT_MARGIN', 70);
define ('RIGHT_MARGIN', 540);
define ('BOTTOM_MARGIN', 720);

/**
 * Some paths currently have to be wired into the script as long as we use.
 *
 * They're all defined here in order to simplify moving the program to
 * another computer.  This could be migrated to the database for the future,
 * but I don't know if it's any easier to maintain it there across
 * platforms.
 *
 *  @param int $whichPath   One of the SERVER...PATH constants.
 *  @param int $jobId       Used in constructing at least one of the paths.
 *
 *  @return string          The requested path.
 *
 *  @throws \Exception if unknown constant passed.
 */
function getServerPath($whichPath, $jobId=null) {

    // Set this as needed
    $host = 'verdi';
    // $host = 'cbiit';

    // Base paths for output files, system dependent
    if ($host == 'verdi')
        $base = '/var/www/html/ebmsdev/sites/ebms.nci.nih.gov/';
    else if ($host == 'cbiit')
        $base = '/web/appdev/sites/ebms.nci.nih.gov/';

    switch ($whichPath) {
        case SERVER_SETTINGS_PATH:
            return $base . 'settings.php';

        case SERVER_FILES_PATH:
            return $base . 'files';

        case SERVER_SAVE_PATH:
            if (is_null($jobId))
                throw new \Exception('getServerPath: missing requried jobId');

            // Same on all systems (so far)
            $template = '/tmp/ebms/PrintJobs/PrintJob%06d';
            return (sprintf($template, $jobId));

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
 * Read drupal's settings.php file to extract database connection information.
 *
 * The information could be derived much more quickly by simply executing
 * the settings.php file and extracting the $databases array, but that
 * fails when a user is logged in due to other things that the php script
 * does.
 *
 *  @param string $path     Full path to the settings.php file.
 *
 *  @return array           Array of name/value pairs.
 */
function getDbSettings($path) {
    // Patterns for parts we need
    $dbStartPat = '/^ *\$databases[^;]*=[^;]*default[^;]*default([^;]*);/ms';
    $partPat    = "/^ *['\"]XXXX['\"] *=>[^'\"]*['\"]([^'\"]*)/ms";

    // Pattern substitutions for XXXX
    $names = array('database', 'username', 'password', 'host', 'port',
                   'driver', 'prefix');

    // Read the settings file looking for the database start pattern
    $settings = file_get_contents($path);
    if (!$settings) {
        throw new \Exception("getDbSettings can't find settings.php");
    }
    $matches = array();
    if (!preg_match($dbStartPat, $settings, $matches)) {
        throw new \Exception(
            "getDbSettings can't find $databases array in settings.php");
    }
    $dbArray = $matches[1];

    // Match each of the parts
    $dbParts = array();
    foreach($names as $name) {
        $pat = str_replace('XXXX', $name, $partPat);
        $success = preg_match($pat, $dbArray, $matches);
        if ($success) {
            $dbParts[$name] = $matches[1];
        }
    }
    return $dbParts;
}

/**
 * Parse a filename and determine it's relevant file type from the extension.
 *
 *  @param string $filename     Name to parse.
 *
 *  @return string              File type, one of 'word', 'pdf', 'text'
 *
 *  @throws \Exception          If we can't figure it out.
 */
function chkFileExtent($filename) {

    // Use PHP filename parser
    $ext = strtolower(pathinfo($filename, PATHINFO_EXTENSION));

    switch ($ext) {
        case 'doc':
        case 'docx':
        case 'odt':
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
 * Construct a PDO object using EBMS database credentials.
 *
 * Gets the credential information from a Drupal settings.php,
 * the path to which is found in a file with a well known name.
 *
 *  @return object          PDO connection object.
 *
 *  @throws Exception       If unable to find connection data or to connect.
 */
function getPdoConn () {
    // Path to settings.php
    $dbConVals = getDbSettings(getServerPath(SERVER_SETTINGS_PATH));

    // Create a handle to the database
    $dbName   = $dbConVals['database'];
    $userName = $dbConVals['username'];
    $password = $dbConVals['password'];
    $host     = $dbConVals['host'];
    $port     = $dbConVals['port'];
    $driver   = $dbConVals['driver'];

    $connStr = "$driver:host=$host;port=$port;dbname=$dbName";
logw("connStr=$connStr\n");

    try {
        // Get the connection
        $pdo = new \PDO($connStr, $userName, $password);
    }
    catch (\PDOException $e) {
        throw new \Exception("EbmsPrint.getPdoConn: PDOException $e");
    }

    return $pdo;
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
        'rtf'  => 7500,
        'pdf'  => 30000,
        null   => 15000
    );

    // Get the extension
    $fileExt = pathinfo($fileName, PATHINFO_EXTENSION);

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
 * XXX - OBSOLETE.  GETTING ALL DATA FROM THE DATABASE.
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
 * Factory to create an EbmsPrintJob.
 *
 *  @return EbmsPrintJob    Initialized job object
 */
function createPrintJob() {

    // Construct the object with it's PDO database connection
    $pj = new EbmsPrintJob();

    // The current database date is the print date for this job
    $row = $pj->dbh->query("SELECT NOW()")->fetch();
    $now = $row[0];
logw("now=$now");

    // Look for a job waiting to run
    // Fatal error if none found
    $jobId = $pj->findJobToRun();
    $pj->load($jobId);

    // If this is a re-run of an old job, get the old job parms
    // $pj->old_job_id = $pj->chkParm($parms, 'old_job_id', false);
    if ($pj->old_job_id) {

        $oldJob = new EbmsPrintJob();

        // Get the old job values, fatal error if not found
        $oldJob->load($pj->old_job_id);

        // Load relevant parts into new job object
        $pj->print_job_type_id = $oldJob->print_job_type_id;
        $pj->packet_start_dt   = $oldJob->packet_start_dt;
        $pj->packet_end_dt     = $oldJob->packet_end_dt;
        $pj->board_id          = $oldJob->board_id;
        $pj->board_member_id   = $oldJob->board_member_id;
        $pj->packet_id         = $oldJob->packet_id;
    }

    else {
        // Determine any default dates
        // User who enters something other than the default dates
        //  affects future runs.  Be careful and know what you're doing!
        if (!$pj->packet_start_dt)
            $pj->packet_start_dt = $pj->getBoardStartPrintDate();
        if (!$pj->packet_end_dt)
            $pj->packet_end_dt = $now;

        // If board member specified, add some more checks
        if ($pj->board_member_id)  {
            if ($pj->print_job_type_id == 'board')
                $pj->reportError(
                    "Job specified a board member ID ({$pj->board_member_id})".
                    " but job type board is for all members of the board,".
                    " not for a single board member.  ".
                    "Please resubmit without board member or for job type of".
                    " 'package' or 'packet'.");

            if ($pj->print_job_type_id == 'package' && !$pj->board_id) {
                // Get the board ID
                $stmt = $pj->dbh->prepare(
              "SELECT board_id FROM ebms_board_member WHERE user_id=:member");
                $stmt->bindParam(':member', $pj->board_member_id);
logw("execute01");
                $stmt->execute();
                $rows = $stmt->fetchAll();

                // Only one allowed, else user must specify board
                if (count($rows) > 1) {
                    $pj->reportError(
                        "Board member with ID={$pj->board_member_id} is a " .
                        "member of multiple boards.  Please specify which " .
                        "one to use in the print job.");
                }
                $pj->board_id = $rows[0][0];
            }
        }
    }

    // Printing now, regardless of any old job
    $pj->print_dt = $now;

    // We're starting the job
    $pj->status = 'in-process';

    // Validation
    if ($pj->packet_end_dt < $pj->packet_start_dt)
        $pj->reportError("EbmsPrintJob end date '{$pj->packet_end_dt}' is " .
                         "before start_date '{$pj->packet_start_dt}'");

    // Update the new job in the database
logw("Job ID = {$pj->print_job_id}\n");
    $pj->update();

    // Define server pathnames
    $pj->serverSavePath = getServerPath(SERVER_SAVE_PATH, $pj->print_job_id);

    // Create them unless we're just reporting
    // XXX Not sure yet about this
    //if ($pj->mode != 'report') {
        $pj->createServerDirPath($pj->serverSavePath);
    //}

    // Define Windows workstation filenames and paths
    $pj->windowsTempPath  = '\\temp\\EBMS\\PrintJobs';
      sprintf("\\tmp\\ebms\\PrintJobs\\PrintJob%05d", $pj->print_job_id);
    $pj->cmdFileName = 'PrintPackets.cmd';
    $pj->rptFileName = 'PrintJobReport.html';

    return $pj;
}

/**
 * Records everything we know of interest about a file that is to be
 * printed.
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
}


/**
 * Parameters controlling the operation of a print job.
 */
class EbmsPrintJob {
    private
        $uniqDocs,          // Number of docs in tar file
        $printDocs,         // Total number of doc printouts
        $errMsgs;           // Array of error messages

    public
        // Board members, packages, docs in job
        $boardMemberList,
        $packageList,
        $docList,
        $uniqPathList;

    public
        // Some special files
        $reportEpf,         // HTML report to user of what's in printout
        $printScriptEpf,    // Windows command file to print everything
        $tarPath;           // Full path to tar archive to download to Windows

    public
        // Important paths
        $serverSavePath,    // Temporary files created here
        $windowsTempPath,   // Unarchive file here on windows client
        $cmdFileName,       // Name of the Windows command file to print docs
        $rptFileName;       // Name of the document with a full report

    public
        $dbh;               // PDO database connection

    public
        // These mirror the ebms_print_job table
        $print_job_id,      // Autoincremented unique print job ID
        $old_job_id,        // If this is a re-run, here what we're re-running
        $print_job_type_id, // 'board', 'package', 'packet'
        $packet_start_dt,   // Datetime <= first packet in the printout
        $packet_end_dt,     // Datetime > last packet in the printout
        $print_dt,          // Datetime at start of job run
        $board_id,          // If printing is for one editorial board
        $board_member_id,   // If printing is for one member
        $mode,              // 'live', 'test', 'report'
        $status,            // Status of the job queued ... success
        $comment;           // Optional free text description of the job

    /**
     * Create the object and initialize a database connection to use
     * for the life object.
     *
     *  @param object $dbh  // If we already have one, use this
     */
    function __construct($dbh=null) {

        // Connection
        if (is_null($dbh))
            $this->dbh = getPdoConn();
        else
            $this->dbh = $dbh();

        // Housekeeping variables for the job
        $this->uniqDocs  = 0;
        $this->printDocs = 0;
        $this->errMsgs   = array();

        // Board members, packages, docs
        $this->boardMemberList = array();
        $this->packageList     = array();
        $this->docList         = array();
        $this->uniqPathList    = array();
    }

    /**
     * We store data in specific places.  Check to be sure that a specified
     * place exists, creating it if it doesn't.  It's a fatal error if we
     * can neither find nor create the directory.
     *
     *  @param string $serverPath   The place we put out files.
     */
    function createServerDirPath($serverPath) {

logw("creating server dir path '$serverPath'");
        // Is it already there?
        if (is_dir($serverPath))
            return true;

        // Try to create it using mkdir() with the recursive=true option
logw("about to call mkdir");
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
logw("in getFullPath: dir=$dir");
        return "$dir/$filename";
    }

    /**
     * Create a new print job to process any job waiting to run.
     *
     *  @return EbmsPrintJob object for the job.
     */
    public function findJobToRun() {
        try {
            $stmt = $this->dbh->prepare(
              "SELECT print_job_id FROM ebms_print_job WHERE status='queued'");
logw("execute02");
            $stmt->execute();
            $rows = $stmt->fetchAll();
            $rowCount = count($rows);
            // Should be exactly one
            if ($rowCount > 1)
                $this->reportError("EbmsPrintJob.initJob(): " .
                            "More than one job has status='queued'" .
                            "  Running again will (hopefully) clear it.");
            else if ($rowCount == 0)
                $this->reportError("EbmsPrintJob.initJob(): " .
                            "Packet printing software called, but no job" .
                            " is ready to be run.");
            else
                $jobId = $rows[0]['print_job_id'];
        }
        catch (PDOException $e) {
            $this->reportError("EbmsPrint.findJobToRun: PDOException $e");
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
            $stmt = $this->dbh->prepare(
                'SELECT * FROM ebms_print_job WHERE print_job_id = :jobId');
            $stmt->bindParam(':jobId', $dbJobId);
logw("execute03");
            $stmt->execute();
            $row = $stmt->fetch();

            // Has to be there
            if (!$row)
                $this->reportError(
                        "EbmsPrintJob.load(): Job $dbJobId not found");

            // Load 'er up
            $this->print_job_id      = $row['print_job_id'];
            $this->old_job_id        = $row['old_job_id'];
            $this->user_id           = $row['user_id'];
            $this->print_job_type_id = $row['print_job_type_id'];
            $this->packet_start_dt   = $row['packet_start_dt'];
            $this->packet_end_dt     = $row['packet_end_dt'];
            $this->print_dt          = $row['print_dt'];
            $this->board_id          = $row['board_id'];
            $this->board_member_id   = $row['board_member_id'];
            $this->packet_id         = $row['packet_id'];
            $this->mode              = $row['mode'];
            $this->status            = $row['status'];
            $this->comment           = $row['comment'];
        }
        catch (PDOException $e) {
            $this->reportError("EbmsPrint.load(): PDOException: $e", true);
        }
    }

    /**
     * Store the job - this is an initial store, not an update.
     * We shouldn't need to change the rows except to update the status.
     *
     *  @return int             New job ID, also stored in $this->print_job_id.
     */
    public function store() {
        // Some validation
        if (!is_null($this->print_job_id))
            $this->reportError(
                "EbmsPrintJob.store: Can't re-insert existing job " .
                 $this->print_job_id);

        try {
            // Prepared statement
            $stmt = $this->dbh->prepare(
                'INSERT ebms_print_job
                   (old_job_id, user_id, print_job_type_id,
                    packet_start_dt, packet_end_dt, print_dt, board_id,
                    board_member_id, status, comment)
                 VALUES
                   (:old_job_id, :user_id, :print_job_type_id,
                    :packet_start_dt, :packet_end_dt, :print_dt, :board_id,
                    :board_member_id, :packet_id, :status, :comment)');
            $stmt->bindParam(':old_job_id', $this->old_job_id);
            $stmt->bindParam(':user_id', $this->user_id);
            $stmt->bindParam(':print_job_type_id', $this->print_job_type_id);
            $stmt->bindParam(':packet_start_dt', $this->packet_start_dt);
            $stmt->bindParam(':packet_end_dt', $this->packet_end_dt);
            $stmt->bindParam(':print_dt', $this->print_dt);
            $stmt->bindParam(':board_id', $this->board_id);
            $stmt->bindParam(':board_member_id', $this->board_member_id);
            $stmt->bindParam(':packet_id', $this->packet_id);
            $stmt->bindParam(':status', $this->status);
            $stmt->bindParam(':comment', $this->comment);

logw("execute04");
            $stmt->execute();
            $errInfo1 = $this->dbh->errorInfo();
            $errInfo1p = print_r($errInfo1, true);

            // Get the job ID and store it
            $this->print_job_id = $this->dbh->lastInsertId();
            if (!$this->print_job_id) {
                $errInfo2 = $this->dbh->errorInfo();
                $errInfo2p = print_r($errInfo2, true);
logw("lastInsertId={$this->print_job_id}\n");
logw("errInfo1 = $errInfo1p\n");
logw("errInfo2 = $errInfo2p\n");
            }
        }
        catch (PDOException $e) {
            $this->reportError("EbmsPrint.store(): PDOException $e");
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
                "EbmsPrintJob.update: Can't update new job " .
                 $this->print_job_id);
logw("updating: print_job_type='$this->print_job_type_id'");

        try {
            // Prepared statement
            $stmt = $this->dbh->prepare(
                'UPDATE ebms_print_job
                    SET old_job_id        = :old_job_id,
                        print_job_type_id = :print_job_type_id,
                        packet_start_dt   = :packet_start_dt,
                        packet_end_dt     = :packet_end_dt,
                        print_dt          = :print_dt,
                        board_id          = :board_id,
                        board_member_id   = :board_member_id,
                        packet_id         = :packet_id,
                        status            = :status,
                        comment           = :comment
                  WHERE print_job_id = :print_job_id');
            $stmt->bindParam(':old_job_id',        $this->old_job_id);
            $stmt->bindParam(':print_job_type_id', $this->print_job_type_id);
            $stmt->bindParam(':packet_start_dt',   $this->packet_start_dt);
            $stmt->bindParam(':packet_end_dt',     $this->packet_end_dt);
            $stmt->bindParam(':print_dt',          $this->print_dt);
            $stmt->bindParam(':board_id',          $this->board_id);
            $stmt->bindParam(':board_member_id',   $this->board_member_id);
            $stmt->bindParam(':packet_id',         $this->packet_id);
            $stmt->bindParam(':comment',           $this->comment);
            $stmt->bindParam(':status',            $this->status);
            $stmt->bindParam(':print_job_id',      $this->print_job_id);

logw("execute05");
            $stmt->execute();
        }
        catch (PDOException $e) {
            $this->reportError("EbmsPrint.update(): PDOException $e");
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
            $stmt = $this->dbh->prepare(
                'UPDATE ebms_print_job S
                    SET status = :status
                  WHERE print_job_id = :job_id');
            $stmt->bindParam(':status', $status);
            $stmt->bindParam(':job_id', $this->print_job_id);
logw("execute06");
            $stmt->execute();
        }
        catch (\PDOException $e) {
            reportError(
              "Unable to set status: PDOException='{$e->getMessage()}'");
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
            $qry = "
                SELECT DISTINCT wp.board_member_id
                  FROM ebms_member_wants_print wp
                  JOIN ebms_board_member bm
                    ON bm.user_id = wp.board_member_id
                 WHERE bm.board_id = :boardId
                   AND wp.start_dt <= :jobEndDate
                   AND (wp.end_dt IS NULL OR wp.end_dt > :jobStartDate)";

            $stmt = $this->dbh->prepare($qry);
            $stmt->bindParam(':boardId', $this->board_id);
            $stmt->bindParam(':jobEndDate', $this->packet_end_dt);
            $stmt->bindParam(':jobStartDate', $this->packet_start_dt);
logw("execute07");
            $stmt->execute();

            $rows = $stmt->fetchAll();
            foreach ($rows as $row) {
logw("Found board member ID={$row['board_member_id']}");
                $memberIds[] = $row['board_member_id'];
            }
        }
        catch (PDOException $e) {
            $this->reportError(
                "EbmsPrint.getBoardMemberPrintUsers(): PDOException $e");
        }

        return $memberIds;
    }


    /**
     * Find the datetime of the next general printing of packets for
     * print_job_type_id = 'board'.  This is a datetime in the past that is
     * immediately after the last general printing for that board.
     *
     * Used when we're printing all packets since the last printing for
     * that board.  That's expected to be the common default case.
     *
     *  @return string  Datetime of last printing for this board.
     *                  "YYYY-MM-DD HH:MM:SS" format.
     */
    public function getBoardStartPrintDate() {

        try {
            $qry = "
                SELECT packet_end_dt
                  FROM ebms_print_job
                 WHERE print_job_id = (
                   SELECT MAX(print_job_id)
                     FROM ebms_print_job
                    WHERE print_job_type_id = 'board'
                      AND board_id = :board_id
                      AND mode = 'live'
                      AND status  = 'success'
                   )";

            $stmt = $this->dbh->prepare($qry);
            $stmt->bindParam(':board_id', $this->board_id);

logw("execute08");
            $stmt->execute();
            $row = $stmt->fetch();

            if ($row)
                // We've got what we wanted
                return $row['packet_end_dt'];
            else
                // This should only ever happen once per board, for the
                //  very first printing for that board
                // In that case we'll print everything? XXX CHECK THIS XXX
                return '2012-01-01 00:00:00';
        }
        catch (PDOException $e) {
            $this->reportError(
                "EbmsPrint.getBoardStartPrintDate(): PDOException $e");
        }
    }

    /**
     * Determine what packages must be printed for one 'board' type job.
     *
     *  @param int $memberId    The member whose package we want to construct.
     *  @param int $boardId     Needed if the member is on more than one
     *                            board (rare).
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

        // This gets all the packets for this specific reviewer
        $sql = "
        SELECT pkt.packet_id
          FROM ebms_packet pkt
          JOIN ebms_packet_reviewer rev
            ON rev.packet_id = pkt.packet_id
          JOIN ebms_topic top
            ON pkt.topic_id = top.topic_id
          JOIN ebms_member_wants_print mwp
            ON rev.reviewer_id = mwp.board_member_id
         WHERE top.board_id = :boardId
           AND pkt.created_at >= :packetStartDt
           AND pkt.created_at <  :packetEndDt
           AND rev.reviewer_id = :memberId
           AND pkt.active_status = 'A'";

        // If this is not an unconditional job, add the following
        if (!$always)
            $sql .= "
           -- Note interpretation of wants_print start and end date
           AND mwp.start_dt <= pkt.created_at
           AND (mwp.end_dt IS NULL OR mwp.end_dt > pkt.created_at)
           AND pkt.packet_id NOT IN (
             SELECT pktp.packet_id
               FROM ebms_packet_printed pktp
               JOIN ebms_print_job job
                 ON pktp.print_job_id = job.print_job_id
              WHERE pktp.board_member_id = :sameMemberId
                AND job.status = 'success'
            )";

        $sql .= "
         ORDER BY pkt.packet_id
        ";
logw("sql=\n$sql");

        // "IN" parameters need to be constructed as strings for PDO
        $inMembers = implode(',', $this->boardMemberList);
logw("Members=$inMembers");
logw("board_id={$this->board_id}");
logw("pkt_start_dt={$this->packet_start_dt} end_dt={$this->packet_end_dt}");

        try {
            $stmt = $this->dbh->prepare($sql);
            $stmt->bindParam(':boardId', $this->board_id);
            $stmt->bindParam(':packetStartDt', $this->packet_start_dt);
            $stmt->bindParam(':packetEndDt', $this->packet_end_dt);
            $stmt->bindParam(':memberId', $memberId);
            $stmt->bindParam(':sameMemberId', $memberId);

logw("execute09");
            $stmt->execute();
$xxx=print_r($stmt->errorInfo(), true);
logw("errorInfo from execute09=$xxx");

            $rows = $stmt->fetchAll();
$xxx=count($rows);
logw("execute09 row count=$xxx");
$xxx=print_r($rows,true);
logw("execute09 rows=$xxx");
        }
        catch (PDOException $e) {
            $this->reportError(
                "EbmsPrint.getMemberPackage: PDOException $e");
        }

        // Look for transition boundaries to the next board member
        $lastMemberId = -1;

        // Assemble the packets for this member
        $packetList = array();
        foreach ($rows as $row) {
            $packetList[] = $row['packet_id'];
        }

        // Send the package home
        return $packetList;
    }

    /**
     * Get a list of files to print for one packet, in the order
     * that they should be printed.
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

logw("In getPacketPrintingList: packetId=$packetId, memberId=$memberId");
        // Array of EbmsPrintFile objects to return
        $epfList = array();

        // Find the topic ID for this packet
        $stmt = $this->dbh->prepare(
                "SELECT topic_id FROM ebms_packet WHERE packet_id=:packetId");
        $stmt->bindParam(':packetId', $packetId);
logw("execute10");
        $stmt->execute();
        $row = $stmt->fetch();
        if (is_array($row) && array_key_exists('topic_id', $row)) {
            $topicId = $row['topic_id'];
logw("topicId=$topicId");
        }
        else
            reportError("No topic for packet ID=$packetId - can't happen");


        // XXX Create a separator page?  Mailing label?  Ask users XXX
        // Could use a different color from a different tray

        // Selection for summaries
        $sumQry = "
            SELECT p.doc_id, d.description, f.fid, f.filename, f.uri,
                   f.filemime, f.filesize
              FROM ebms_packet_summary p
              JOIN ebms_doc d
                ON p.doc_id = d.doc_id
              JOIN file_managed f
                ON d.file_id = f.fid
             WHERE p.packet_id = :packetId";
        $stmt = $this->dbh->prepare($sumQry);
        $stmt->bindParam(':packetId', $packetId);
logw("execute11");
        $stmt->execute();

        // There's usually exactly one, but could be zero or more
        $rows = $stmt->fetchAll();
$rowCount=count($rows);
logw("Count of summaries = $rowCount");
        foreach ($rows as $row) {

            // Construct an object to represent this file
            $epf = new EbmsPrintFile();
            $epf->boardMemberId = $memberId;
            $epf->drupalFileId  = $row['fid'];
            $epf->purpose       = 'summary';
            $epf->path          = $this->getFullPath($row['uri'],
                                                     $row['filename']);
            $epf->description   = $row['description'];
            $epf->filesize      = $row['filesize'];

            // Figure out the file type
            $filemime = null;
            if (array_key_exists('filemime', $row))
                $filemime = $row['filemime'];
            $epf->filetype = $this->getFileType($epf->path, $filemime);

            // Here's our heuristic for page counts
            $epf->numPages = estimatePageCount($this->path,$this->filesize);

            // Append it to the array
            $epfList[] = $epf;
       }

       // Selection for articles
       $artQry = "
            SELECT p.article_id, f.fid, f.filename, f.uri,
                   f.filemime, f.filesize, a.article_title
              FROM ebms_packet_article p
              JOIN ebms_article a
                ON p.article_id = a.article_id
              JOIN file_managed f
                ON a.full_text_id = f.fid
             WHERE p.packet_id = :packetId";
        $stmt = $this->dbh->prepare($artQry);
        $stmt->bindParam(':packetId', $packetId);
logw("execute12");
        $stmt->execute();

        // Packets often have multple articles
        $rows = $stmt->fetchAll();

        // First article in the packet is 1 of total num rows
        $packetPos = 1;
        $packetMax = count($rows);
logw("article search found $packetMax articles");

        foreach ($rows as $row) {

            $epf = new EbmsPrintFile();
            $epf->boardMemberId = $memberId;
            $epf->drupalFileId  = $row['fid'];
            $epf->ebmsArticleId = $row['article_id'];
            $epf->purpose       = 'article';
            $epf->path          = $this->getFullPath($row['uri'],
                                                     $row['filename']);
            $epf->filesize      = $row['filesize'];
            $epf->numPages      = estimatePageCount($epf->path,
                                                    $epf->filesize);
            $epf->filetype      = $this->getFileType($epf->path,
                                                     $row['filemime']);

            // Use the article title as the description
            $epf->description   = $row['article_title'];

            // If there's a board member ID, create a response sheet for
            //   this file and append its EbmsPrintFile object to our list
            // EbmsPrintFile will be created in all modes but actual file
            //   is not created if it's just a report
logw("Board member ID={$epf->boardMemberId}");
            if ($epf->boardMemberId) {
                $respEpf = $this->createResponseSheet($epf, $packetPos,
                                          $packetMax, array($topicId));
                $epfList[] = $respEpf;
logw("Created response sheet");
            }

            // Followed by the article object
            $epfList[] = $epf;
logw("Adding {$epf->path} to epfList");
            ++$packetPos;
        }
$xxx=count($epfList);
logw("Total epfs added=$xxx");

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
    function getUniqueFileList($epfList) {

        // List to return
        $pathList = array();

        // Index of it
        $pathIndex = array();

        foreach ($epfList as $epf) {
            $path = $epf->path;
logw("path from epfList=$path");
            if (!array_key_exists($path, $pathIndex)) {
                $pathList[] = $path;
                $pathIndex[$path] = 1;
            }
        }

        return $pathList;
    }

    /**
     * Get the path to the directory where we'll create files.
     *
     * Using /tmp/ebms/PrintJobs/PrintJobN unless someone wants it otherwise.
     *
     * /tmp is world readable and writeable, it's available on all Linux and
     * UNIX systems, and gets regularly cleaned by the OS.
     *
     * If the path we want doesn't exist, we create it.
     *
     * XXX ISSUE: Do we need to worry about ebms vs. ebmsdev?
     * XXX ISSUE: Should these be stored semi-permanently in:
     *            /home/drupal/ebms/PrintJobs/PrintJobN?
     *
     *  @return string      Path.
     */
    function getPrintJobTempDir() {

        // Final directory component
        $bottomDir = 'PrintJob' . $this->print_job_id;
        $fullPath  = "/tmp/ebms/PrintJobs/$bottomDir";

        // Quick short circuit
        if (is_dir($fullPath))
            return $fullPath;

        // Slower check/create if that didn't work

        // Path components
        $pathComps = array('ebms', 'PrintJobs', $bottomDir);

        $chkPath = '/tmp';
        foreach ($pathComps as $path) {
            $chkPath = $chkPath . '/' . $path;
            if (!file_exists($chkPath)) {
                $result = mkdir($chkPath);
                if (!$result) {
                    $this->reportError(
                        "getPrintJobTempDir: Could not create directory " .
                        "'$chkPath'");
                }
            }
            elseif (!is_dir($chkPath)) {
                throw new \Exception(
                    "getPrintJobTempDir: '$chkPath' is not a directory");
            }
        }

        // If we emerged here, we found or created what we want
        return $chkPath;
    }

    /**
     * Create a printable PDF format response sheet for an article.
     * Store it where we're storing temp files for this print job.
     *
     *  @param object ref $epf  EbmsPrintFile object for the article.
     *  @param int $packetPos   Ordinal pos of article in packet, origin 1.
     *  @param int $packetMax   Count of articles in packet.
     *  @param array $topicIds  Topic(s) for use in header.  Can be more than
     *                           one.
     *
     *  @return                 New EbmsPrintFile object for the constructed
     *                           file.
     */
    function createResponseSheet($epf, $packetPos, $packetMax, $topicIds) {

        // Reviewer name tells who this response is from
        $stmt = $this->dbh->prepare(
                'SELECT name FROM users WHERE uid = :memberId');
        $stmt->bindParam(':memberId', $epf->boardMemberId);
logw("execute12.5");
        $stmt->execute();
        $row = $stmt->fetch();
        if (!$row) {
            // XXX Don't know if !$row will show this
            $this->reportError(
                "EbmsPrintJob.createResponseSheet(): " .
                " Attempt to print a response sheet but reviewer " .
                "'{$epf->boardMemberId}' not found.");
        }
        $reviewerName = $row['name'];
logw("reviewer id={$epf->boardMemberId}, name=$reviewerName");

        // We're not relying on Drupal being present, so we have to
        //  get article info without using the EbmsArticle class
        $articleId = $epf->ebmsArticleId;
        $stmt = $this->dbh->prepare(
                'SELECT source, source_id, article_title, brf_citation ' .
                '  FROM ebms_article WHERE article_id = :artId');
        $stmt->bindParam(':artId', $articleId);
logw("execute13");
        $stmt->execute();
        $row = $stmt->fetch();

        // Has to be there
        if (!$row)
            $this->reportError(
                "EbmsPrintJob.createResponseSheet(): " .
                " Article with ID $articleId not found");

        // Article IDs
        $idStr = '';
        if ($row['source'] == 'Pubmed')
            $idStr = "PMID: {$row['source_id']}   ";
        $idStr .= "CMS ID: $articleId";

        // Title info
        $articleTitle = $row['article_title'];
        $brfCitation  = $row['brf_citation'];

        // Up to three of them in a single string
        $qry = <<<EOS
SELECT auth.collective_name, auth.last_name, auth.initials
  FROM ebms_article_author auth
  JOIN ebms_article_author_cite cite
    ON auth.author_id = cite.author_id
 WHERE cite.article_id = :artId
   AND cite.cite_order <= 3
 ORDER BY cite.cite_order
EOS;
        $stmt = $this->dbh->prepare($qry);
        $stmt->bindParam(':artId', $articleId);
logw("execute14");
        $stmt->execute();
        $rows = $stmt->fetchAll();

        // Format them into a single string
        $authorStr = '';
        $authorCnt = 0;
        foreach ($rows as $row) {
            if ($authorCnt > 0)
                $authorStr .= '; ';
            if ($row['collective_name'])
                $authorStr = $row['collective_name'];
            else {
                $authorStr .= $row['last_name'];
                $authorStr .= ' ';
                $authorStr .= $row['initials'];
            }
            ++$authorCnt;
        }

        // Get the name(s) of the topic(s)
        $topicList = implode(',', $topicIds);
        $stmt = $this->dbh->prepare(
            'SELECT topic_id, topic_name FROM ebms_topic '.
            ' WHERE topic_id IN (:topicList)');
        $stmt->bindParam(':topicList', $topicList);
logw("execute15");
        $stmt->execute();
        $rows = $stmt->fetchAll();
        $topicCount  = 0;
        $topicString = '';
        foreach ($rows as $row) {
            $topicId   = $row['topic_id'];
            $topicName = $row['topic_name'];
            if ($topicName) {
                $topicName = $row['topic_name'];
                if ($topicCount == 0) {
                    $firstTopic  = $topicName;
                    $topicString = $topicName;
                }
                else
                    $topicString = ", $topicName";
                ++$topicCount;
            }
            else {
                // Fatal error
                $this->reportError("EbmsPrint.createResponseSheet(): " .
                      "No topic found for topic_id = $topicId, can't happen");
            }
        }
logw("firstTopic=$firstTopic, topicString=$topicString");

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
        // There is no setBottomMargin call!
        // $pdf->setBottomMargin(BOTTOM_MARGIN);

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
        $fname = sprintf("%s/ResponseSheetArticle%dMbr%d.pdf",
                          $this->serverSavePath,
                          $epf->ebmsArticleId, $epf->boardMemberId);
        if ($this->mode != 'report') {
            $pdf->Output($fname, 'F');
        }
        // Only needed if we didn't output, but does no harm
        $pdf->Close();

        // Construct an EbmsPrintFile object it
        $newEpf = new EbmsPrintFile();
        $newEpf->boardMemberId = $epf->boardMemberId;
        $newEpf->purpose       = 'response';
        $newEpf->path          = $fname;
        $newEpf->description   =
                "Response sheet for $reviewerName: {$epf->description}";
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
     *  @param string $text     Text to write.
     *  @param int $y           Upper left corner, vertical position.
     *                           Only needed for first line of a series of
     *                           lines.  Otherwise we position after the last
     *                           printed line
     *  @param int $wideHeight  Height of the text line.  Font must be smaller.
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
     *  @param object $pdf      FPDF object to add the text to.
     */
    function createResponseFixedText($pdf) {

        // Initialization
        $leftMargin   = LEFT_MARGIN;
        $rightMargin  = RIGHT_MARGIN;
        $startY       = $pdf->GetY() + 6;
        $wideHeight   = WIDE_HEIGHT;

        // Spacing from the top
        // $pdf->Ln($wideHeight * 2);
        // $pdf->Ln();
        // $pdf->Ln();

        // Disposition lines
        $pdf->SetFont(DFT_FONT_FAMILY, 'B', DFT_FONT_HEIGHT);
        $pdf->Write($wideHeight, 'Disposition');
        $pdf->Ln(NARROW_HEIGHT);
        $pdf->SetFont(DFT_FONT_FAMILY, '', DFT_FONT_HEIGHT);
        $pdf->Write($wideHeight,
                    'Indicate how the article might affect the summary.');
        $pdf->Ln();
        // $Y = $startY + $wideHeight + NARROW_HEIGHT;
        $Y = $startY + NARROW_HEIGHT * 2;
        // $Y = $pdf->GetY();

        $dispositions = array(
            'Warrants no changes to the summary (indicate reason[s] for exclusion on the back)',
            'Deserves citation in the summary (indicate placement in the summary document)',
            'Merits revision of the text (indicate changes in the summary document',
            'Merits discussion'
        );

        // $Y = $pdf->GetY() + NARROW_HEIGHT;
        $Y = $pdf->GetY();
        foreach ($dispositions as $disposition) {
            $this->chkLine($pdf, $disposition, $Y);
            $Y = null;
        }
        // Comments
        $pdf->SetFont(DFT_FONT_FAMILY, 'B', DFT_FONT_HEIGHT);
        $pdf->Write($wideHeight, 'Comments');
        $pdf->Ln($wideHeight * 2);

        $Y = $pdf->GetY();
        for ($i=0; $i<3; $i++) {
            $pdf->Line($leftMargin, $Y, $rightMargin, $Y);
            $Y += $wideHeight;
        }

        // Levels of Evidence
        $pdf->Ln($wideHeight * 3);
        $pdf->Write($wideHeight, 'Levels of Evidence Information');
        $Y += $wideHeight;
        $pdf->SetFont(DFT_FONT_FAMILY, '', DFT_FONT_HEIGHT);
        $pdf->Ln(NARROW_HEIGHT);
        $pdf->Write($wideHeight,
                'Enter the appropriate level of evidence for this article.');
        $Y += $wideHeight * 2;
        $pdf->Line($leftMargin, $Y, $rightMargin, $Y);
        $Y += $wideHeight;

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
        $pdf->Write($wideHeight, 'Reason(s) for Exclusion From PDQ Summary');
        $pdf->Ln(NARROW_HEIGHT);
        $pdf->SetFont(DFT_FONT_FAMILY, '', DFT_FONT_HEIGHT);
        $pdf->Write(NARROW_HEIGHT,
                    'If you indicated that the article warrants no changes ' .
                    'to the summary, indicate which of the reasons below ' .
                    'led to your decision to exclude the article.  You may ' .
                    'choose more than one reason.');
        $pdf->Ln($wideHeight);
        $Y = $pdf->GetY() + 6;

        // List of reasons
        foreach ($reasons as $reason) {
            $this->chkLine($pdf, $reason, $Y);
            $Y = null;
        }

        $Y = $pdf->GetY() + $wideHeight;
        for ($i=0; $i<3; $i++) {
            $pdf->Line($leftMargin, $Y, $rightMargin, $Y);
            $Y += $wideHeight;
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
     *   'PrintJob00000/...'  Where ... are the file names.
     *
     * The package will be sent to the workstation.
     *
     *  @param array $fileList  Array of EbmsPrintFile objects.
     *
     *  @return                 Path to the tar file
     *                          More may be added to the tar file after this.
     */
    function packagePrintFiles($fileList) {
$xxx=count($fileList);
logw("packagePrintFiles has $xxx files to packup");

        // Used for tarring files in php
        require_once('Archive/Tar.php');

        // Use this as the directory path inside the tar file
        // It will be expanded in the Windows workstation
        // It need not mirror what's in the server path
        $newPath = sprintf("PrintJobs/PrintJob%06d", $this->print_job_id);

        // Initialize the tar file in a known place
        $tarPath = $this->serverSavePath . '.tar';
        $tarObj  = new \Archive_Tar($tarPath);

        // Use these to ensure that each file is only included once
        // We expect all filenames to be unique, independent of path
        // Separate arrays are used to check that
        $pathNames = array();
        $baseNames = array();
        $tempName  = array();

        foreach ($fileList as $prtFile) {
            $oldPath     = dirname($prtFile);
            $baseName    = basename($prtFile);
            $tempName[0] = $prtFile;

logw("Tarring $prtFile");
            // Add it to the archive
            $tarObj->addModify($tempName, $newPath, $oldPath);
        }

        return $tarPath;
    }

    /**
     * Create a Windows command script for printing the contents of an
     * archive on the command line.  The script is stored in the file
     * system.
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
        $scriptEpf->filetype = 'cmd';

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
foreach($fileList as $epf) {
logw("full fileList path={$epf->path}, type={$epf->filetype}");
}
        foreach ($fileList as $epf) {

            $fname = basename($epf->path);
            $ftype = $epf->filetype;
logw("PS: scriptIdx=$scriptIdx, type=$ftype, name=$fname");
            // Setup
            fwrite($fp, ":L$scriptIdx\n");
            fwrite($fp, "if %3. == $scriptIdx. goto :done\n");

            // Document type specific printing command
            switch ($ftype) {
                case 'pdf':
                case 'application/pdf':
                    fwrite($fp, "pdfprint -duplex 2 -printer %1 $fname\n");
                    break;
                case 'word':
                    fwrite($fp, "abiword -p %1 $fname\n");
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
        fwrite($fp, ":showcount\n");
        fwrite($fp, "This script contains $docCount files\n");

        fwrite($fp, ":done\n");

        return $scriptEpf;
    }

    /**
     * Error routine.
     *
     * Fill this out later.
     *
     *  @param string $msg      Error to report
     *  @param bool $fatal      True=cleanup and abort.
     */
    function reportError($msg, $fatal=true) {

        // Prevent recursion on errors
        static $inError = false;

        // Add the message to our list of messages
        $this->errMsgs[] = $msg;
        $msgs = implode(' / ', $this->errMsgs);

        // Check for recursion
        if ($inError) {
            $msgs = 'Loop in reportError';
            pdq_ebms_debug("Fatal error: jobId={$this->print_job_id}", $msgs);
            throw new \Exception("Fatal error: '$msgs'");
        }
        $inError = true;

        // If fatal error, cleanup
        if ($fatal) {
            // Cleanup will rollback transaction, if necessary
            $this->cleanup('failure');

            // But try to mark this job as failed
            try {
                $this->setJobStatus('failure');
            }
            catch (PDOException $e) {
                // XXX - email admin?
                // But can't do much else
            }

            pdq_ebms_debug("Fatal error: jobId={$this->print_job_id}", $msgs);

            exit("Packet printing for job {$this->print_job_id} failed: $msgs");
        }
        $inError = false;
    }

    /**
     * Produce a list of documents in a package.
     * Use it to figure out where we were when a printer error occurred.
     * Also used in a test run to see what would have printed.
     *
     *  @param array $fileList  Array of all EbmsPrintFile objects for job.
     *
     *  @return EbmsPrintFile for the file list itself.
     */
    function createPrintReport($fileList) {
        try {
            if ($this->board_member_id) {
                // Get the board member name
                $stmt = $this->dbh->prepare(
                        "SELECT name FROM users WHERE uid = :memberId");
                $stmt->bindParam(':memberId', $this->board_member_id);
                $stmt->execute();
                $row = $stmt->fetch();
                $memberName = $row[0];
                $showMember = "$memberName ($this->board_member_id)";
            }
            else
                $showMember = "Not specified";

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
</head>
<body>
<h2>Documents included in Print Job {$this->print_job_id}</h2>

<h3>Print Job Parameters</h3>
<p>This job was run with the following parameters:</p>
<table border='2'>
 <tr><th>Parameter</th><th>Value</th></tr>
 <tr><td>Job ID</td><td>$this->print_job_id</td></tr>
 <tr><td>Old job ID</td><td>$this->old_job_id</td></tr>
 <tr><td>Job type</td><td>$this->print_job_type_id</td></tr>
 <tr><td>Start date</td><td>$this->packet_start_dt</td></tr>
 <tr><td>End date</td><td>$this->packet_end_dt</td></tr>
 <tr><td>Print date</td><td>$this->print_dt</td></tr>
 <tr><td>Board ID</td><td>$this->board_id</td></tr>
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
<ol>
EOS;
            fwrite($fp, $hdr);

            // Info about each file
            foreach ($fileList as $epf) {
                $fname = basename($epf->path);
                fwrite($fp, "<li>$fname:</br> {$epf->description}</li>\n");
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
            reportError("EbmsPrintJob.createPrintReport(): " .
                   "Exception raised writing report file: $e");
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
     * Append a comment to the comment field of the print job
     *
     *  @param string $text     Append this to what's there.
     */
    function appendComment($text) {
        $this->comment .= "  [$text]";
    }

    /**
     * Update the record of what packets were printed for each board member.
     * Only call this if in live mode and job was successful.
     */
    function recordPrintedPackets() {

        $stmt = $this->dbh->prepare('
            INSERT ebms_packet_printed
                   (board_member_id, packet_id, print_job_id)
            VALUES (:board_member_id, :packet_id, :print_job_id)
        ');

        // This paramter is invariant
        $stmt->bindParam('print_job_id', $this->print_job_id);

        try {
            // Wrap this in a transaction
            $this->dbh->beginTransaction();

            // Read the package list of memberId => array of packet IDs
            foreach ($this->packageList as $memberId => $packetList) {
                foreach ($packetList as $packetId) {
                    // Bind parameters to the prepared statement
                    $stmt->bindParam(':board_member_id', $memberId);
                    $stmt->bindParam(':packet_id', $packetId);

                    // Update the table
logw("execute16");
                    $stmt->execute();
                }
            }
        }
        catch (\PDOException $e) {
            $this->dbh->rollBack();
            $this->reportError(
                "ebms_packet_printed table update failed: '{$e->getMessage}'");
        }
        // If we're here, everything worked
        $this->dbh->commit();
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
            $errStr = "ERRORS: ";
            foreach ($this->errMsgs as $msg)
                $errStr .= "; $msg";
            $this->appendComment($errStr);
        }

        // Update the job info - unless we didn't get far enough to have a job
        if ($this->print_job_id) {
            $this->update();
        }
        else {
            // Primary way we could get here is if there is more than one
            //  job row in the queued state.
            // Fail all of them.
            $stmt = $this->dbh->prepare(
                "UPDATE ebms_print_job SET status='failure' " .
                " WHERE status='queued'");
logw("execute17");
            $stmt->execute();
        }

        // XXX TEMPORARY.  Mail should go to user running the job and
        //     if live job, to board manager (I think).
        $to = '***REMOVED***';

        // Send email to user
        $subject = "EBMS Print Job {$this->print_job_id} is complete";
        $body    = <<<EOS
<html>
 <head>EBMS Print Job Results</head>
<body>
<h2>EBMS Print Job Results</h2>
<p>Print job {$this->print_job_id} completed with status '{$this->status}'</p>

EOS;
        $errCount = count($this->errMsgs);
        if ($errCount == 0)
            $body .= "There were no error messages.\n\n";
        else {
            $body .= "$errCount error message(s) were recorded:\n";
            foreach ($this->errMsgs as $msg)
                $body .= '  ' . $msg . "\n";
        }

        // XXX The following links need testing
        if ($status == 'success') {
            if ($this->mode != 'report') {
                $body .= <<<EOS
<p>Please visit the following link to download the print files.</p>
<br />
<a href='{$this->tarPath}'>Download All Files</a>
EOS;
            }
            else {
                $body .= <<<EOS
<p>Please visit the following link to see a report on the results of the
test run.</p>
<br />
<a href='{$this->reportEpf->path}'>Test Mode Results</a>
EOS;
            }
        }
        $body .= "</body>\n</html>\n";

        // XXX If any files were produced, give the user a link ...
        // XXX MORE TO COME

        // mail($to, $subject, $body);
    }

} // class EbmsPrintJob

/**************************************************************
 * MAIN
 *
 * Execution begins here when program is spawned, or when invoked
 * via require_once().
 *************************************************************/

logw("Starting main");
// Create a print job from a recently created row in the table
$pj = createPrintJob();

// If we're printing documents in a specified packet, we know what
//  docs to assemble regardless off who want them.
if ($pj->print_job_type_id == 'packet') {
    // Get the list of docs.  If the member ID is not null, response
    //  sheets will be included
    $pj->docList = $pj->getPacketPrintList($packetId, $this->board_member_id);
}

// If we're printing one board member's package, we already know who it is
// Find his package, if any
// Then his docs, if any
else if ($pj->print_job_type_id == 'package') {
    // Get the package for him within the packet print dates
    // Don't care if he's already seen them or not
    $package = $pj->getMemberPackage($this->board_member_id,
                                     $this->board_id, true);
    foreach ($package as $packetId) {
        $docs = $pj->getPacketPrintList($packetId, $this->board_member_id);
        $pj->docList = array_merge($pj->docList, $docs);
    }
}

// For a 'board' type job, find out what board members need printing
// Then find their packages, if any
// Then their documents, if any
else if ($pj->print_job_type_id == 'board') {
logw("Building boardMemberList");
    $pj->boardMemberList = $pj->getBoardMemberPrintUsers();
$xxx=count($pj->boardMemberList);
logw("count boardMemberList = $xxx");
    foreach ($pj->boardMemberList as $memberId) {
logw("Getting packages for member=$memberId");
        $package = $pj->getMemberPackage($memberId, $pj->board_id);
        foreach ($package as $packetId) {
logw("Getting docs for packet$packetId");
            $docs = $pj->getPacketPrintList($packetId, $memberId);
            $pj->docList = array_merge($pj->docList, $docs);
$xxx1=count($docs);
$xxx2=count($pj->docList);
logw("Adding $xxx1 docs to docList, total now=$xxx2");
        }
    }
}

// Is there anything to print?
// If not, we'll report the error and exit from there
// No members want anything
if ($pj->print_job_type_id == 'board') {
    if (count($pj->boardMemberList) == 0)
        $pj->reportError('No members of the board want printing at this time');
}

// Couldn't find any docs ready to print
if (count($pj->docList) == 0) {
    $pj->reportError('There are no documents ready for printing at this time');
}


$xxx=print_r($pj->boardMemberList, true);
$xxxCount=count($pj->boardMemberList);
logw("boardMember IDs for 'board' type job count=$xxxCount, =$xxx");

// Construct report report html that says what will be printed, in what order
// Report is stored in tmp storage and an object representing is returned
$pj->reportEpf = $pj->createPrintReport($pj->docList);

// Construct print command script and EbmsPrintFile object for it
// In report mode, just creates the epf, no actual file is output
$pj->printScriptEpf = $pj->createPrintScript($pj->docList);

// Both of those files have to be downloaded to the workstation, so add them
$pj->docList[] = $pj->reportEpf;
$pj->docList[] = $pj->printScriptEpf;

// Extract a list of unique files from the long list
$pj->uniqPathList = $pj->getUniqueFileList($pj->docList);

// For runs other than just reporting
if ($pj->mode != 'report') {
    // Add each unique full file content to one tar archive
    $pj->tarPath = $pj->packagePrintFiles($pj->uniqPathList);

    // Update the database showing what has been printed
    if ($pj->mode == 'live')
        $pj->recordPrintedPackets();
}

// That's it!
$pj->cleanup('success');
