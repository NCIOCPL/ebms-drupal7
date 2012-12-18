<?php

namespace Ebms;

require_once('EbmsArticle.inc');

/**
 * Constants
 */
// Regex for parsing out Pubmed Articles
define('Ebms\ART_PAT', '/<PubmedArticle>.*<\/PubmedArticle>/smuU');

// Regex for parsing out errors embedded in Pubmed output
define('Ebms\ERR_PAT', '/<!-- Error>(.*)<\/Error -->/smuU');

// Max output for one line of Pubmed, should be waaay high
define('Ebms\MAX_LINE', 100000);

// Get a Pubmed ID out of an error message, 7-8 digits
define('Ebms\PMID_PAT', '/[^\d](\d{7,8})[^\d]/');

// Medline format Pubmed ID
define('Ebms\MEDLINE_PMID_PAT', '/^PMID- (\d{2,8})/m');

// Pubmed XML format recognizer
define('Ebms\PUBMED_XML_START_PAT', '/<!DOCTYPE PubmedArticleSet/m');

/**
 * Class for importing records from NLM
 */
class PubmedInterface {
    public
        // Constants for good behavior with Pubmed
        // Don't expect this to reduce memory usage.  To do that, use
        //   a reasonable $fetchCount parameter to getRecsFromNLM() or
        //   getXmlFromNLM().
        $MAX_FETCH = 500;

        // For debug, testing batching
        // $MAX_FETCH = 2;

    /**
     * Construct an interface for a set of Pubmed IDs.
     *
     *  @param array ids        Pubmed IDs as ASCII digit chars
     */
    public function __construct(array $ids) {
        $this->idList = $ids;      // List of Pubmed IDs to import
        $this->nextId = 0;         // Next one to fetch is first one

        // Constant datetime if needed for PubmedInterface use
        $this->constantDt = db_query('SELECT NOW()')->fetchField();
    }

    /**
     * Front end to getting raw XML from NLM
     *
     *  @param int $fetchCount    Max records to fetch from the idList.
     *  @param string &$xml       Put retrieved XML here.
     *                            May have more than one batch in it.
     *
     *  @return int               Number of batches (interactions with NLM)
     *                              represented in retrieved $xml.
     *
     *  @throws Exception         If thrown by lower level.
     */
    public function getXmlFromNLM($fetchCount, &$xml) {

        // Dummies needed for interface to fromNLM()
        $articleArray = array();
        $errorArray   = array();
        $recCount     = 0;
        $errCount     = 0;

        $this->fromNLM($fetchCount, true, $articleArray, $errorArray,
                       $xml, $recCount, $errCount, $xmlCount);

        // XML data was appended to $xml, return count of appended blocks
        return $xmlCount;
    }


    /**
     * Front end to getting EbmsArticle objects from NLM
     *
     *  @param int $fetchCount        Max records to fetch from the idList.
     *  @param EbmsArticle &$retArts  Put retrieved EbmsArticle objects here.
     *  @param string      &$retErrs  Put generated error msg strings here.
     *
     *  @return int                   Total number of items returned.
     *
     *  @throws Exception             If thrown by lower level.
     */
    public function getRecsFromNLM($fetchCount, &$retArts, &$retErrs) {

        // Dummies needed for interface to fromNLM()
        $xml           = '';
        $xmlBlockCount = 0;

        // Data we want to get back
        $recCount = 0;
        $errCount = 0;

        // Get the data
        $this->fromNLM($fetchCount, false, $retArts, $retErrs, $xml,
                       $recCount, $errCount, $xmlBlockCount);

        // Data was appended to $retArts and $retErrs.  Return the sum.
        return ($recCount + $errCount);
    }

    /**
     * Get size of list of Pubmed IDs in this PubmedInterface
     *
     *  @return int Number of items total, both retrieved and waiting.
     */
    public function getPmidCount() {
        return count($this->ids);
    }


    /**
     * Get number of Pubmed IDs not yet retrieved.
     *
     *  @return int Number reaining to be processed.
     */
    public function getRemainCount() {
        return (count($this->idList) - $this->nextId);
    }


    /**
     * Get citations from NLM.
     *
     * Does not store any records.  Returns data to caller.
     * However it does preserve state.  If caller calls with a large
     * idList and a smaller fetchCount, we remember the list and the
     * nextId to use on the next call for this object.
     *
     *  @param int $fetchCount     Number caller wants.  May be more or less
     *                               than count in idList or count requested
     *                               in one fetch from NLM.
     *                             We won't return more than this but may
     *                               return less if the idList is smaller, or
     *                               if one or more IDs are unknown to Pubmed.
     *                             Use low fetchCount to limit memory use, high
     *                               fetchCount to simplify or speed code.
     *  @param bool $rawMode       True  = Return raw, unparsed, XML.  No
     *                               guarantee that it's even well formed.
     *                             False = Parse and load into EbmsArticle
     *                               objects.
     *
     *  @param array &$retRecs Return EbmsArticle objects here if
     *                               $rawMode == false;
     *  @param array &$retErrors   Return error strings here, if any.
     *  @param array &$retXml      Return serial XML here, if $rawMode.
     *  @param int   &$recCount    Number of EbmsArticles constructed.
     *  @param int   &$errCount    Number of error messages produced.
     *  @param int   &$xmlCount    Number of blocks of XML fetched from NLM.
     *
     *  @return Void.
     *
     *  @throws Exception if unable to connect or parse error in rawMode.
     */
    private function fromNLM($fetchCount, $rawMode,
                             &$retRecs, &$retErrors, &$retXml,
                             &$recCount, &$errCount, &$xmlCount) {

        $SCRIPT = "fromNLM";

        // Construct a URL to fetch
        $base = 'http://eutils.ncbi.nlm.nih.gov/entrez/eutils/efetch.fcgi';

        // Fixed parameters for request
        $parms = 'db=pubmed&rettype=medline&retmode=xml&id=';

        // How many to fetch.  Note: may not be first call on this object
        $idListCount = count($this->idList);
        $remainCount = $idListCount - $this->nextId;

        // Lower remainCount to amount remaining for this call only
        if ($remainCount > $fetchCount)
            $remainCount = $fetchCount;

        // Okay to start?
        if ($idListCount < 1)
            throw new \Exception("$SCRIPT: No Pubmed IDs to fetch");

        // Return counts are for this call, not cumulative with other calls
        // If caller wants cumulation, he does it
        $recCount = 0;
        $errCount = 0;
        $xmlCount = 0;

        // Run until no more to get
        while ($remainCount > 0) {

            // This is how many we'll try to fetch
            $requestCount = min($remainCount, $this->MAX_FETCH);

            // Construct a string of pubmed ids, up to max
            $idParms = array();
            $afterLastId = $this->nextId + $requestCount;
            for ($i=$this->nextId; $i<$afterLastId; $i++)
                $idParms[] = "{$this->idList[$i]}";

            $parmStr = $parms . implode(',', $idParms);

            // Initialize a curl object for a POST request
            $ch = curl_init();
            curl_setopt($ch, CURLOPT_URL, $base);
            curl_setopt($ch, CURLOPT_POST, true);
            curl_setopt($ch, CURLOPT_POSTFIELDS, $parmStr);

            // This one puts the return in $results instead of stdout
            curl_setopt($ch, CURLOPT_RETURNTRANSFER, TRUE);

            // DEBUG
            // curl_setopt($ch, CURLINFO_HEADER_OUT, true);

            // Post it
            $results = curl_exec($ch);

            // DEBUG - Log or do something with $headers if desired
            // $headers = curl_getinfo($ch);

            curl_close($ch);

            // Save results
            if (!$rawMode) {
                // Cooked results appended to arrays
                $this->parsePubmed($results, $retRecs, $retErrors,
                                   $recCount, $errCount);
            }
            else {
                // Raw results appended to string
                $retXml .= $results;
            }


            // Update index and counters for next time through loop
            $remainCount  -= $requestCount;
            $this->nextId += $requestCount;

            // We've gone through the loop one more time
            $xmlCount++;

            // NLM wants no more than 3 requests per second, we'll
            //   limit it to one if it's big.
            // See Bugzilla issue 4937 Comment #42.
            // If we requested the max, wait a sec before requesting again.
            if ($requestCount == $this->MAX_FETCH && $remainCount > 0)
                sleep(1);
        }
    }


    /**
     * Extract records from a serial Pubmed XML results file, convert them
     * to EbmsArticle objects, and make them available to the caller.
     *
     *  @param string &$serialXml   XML as downloaded from Pubmed.
     *                              Passed as reference to avoid extra copying
     *  @param array  &$retRecs     Append EbmsArticle objects here.
     *  @param array  &$retErrors   Append error messages here.
     *  @param int    &$recCount    Return num EbmsArticles constructed.
     *  @param int    &$errCount    Return num error messages produced.
     *
     *  @return void
     *
     *  @throws Exception          From lower level XML parsing
     */
    private function parsePubmed(&$serialXml,
                                 array &$retRecs, array &$retErrors,
                                 &$recCount, &$errCount) {

        // Extract all of the errors from the data
        $offset = 0;
        while (true) {
            // Set or reset an array to receive match results
            $matches = array();

            // Search for next error
            $found = preg_match(ERR_PAT, $serialXml, $matches,
                                PREG_OFFSET_CAPTURE, $offset);
            if ($found == 1) {
                // Extract error message and append to list
                // The error message should have the passed PMID in it
                $retErrors[] = $matches[1][0];
                $errCount++;

                // Point past this to look for next one
                $offset = $matches[1][1] + strlen($matches[1][0]);
            }
            else {
                // Nothing (more) found
                break;
            }
        }

        // Extract all of the PubmedArticle records
        $offset = 0;
        while (true) {
            $matches = array();
            $found = preg_match(ART_PAT, $serialXml, $matches,
                                PREG_OFFSET_CAPTURE, $offset);
            if ($found == 1) {
                $docXml = $matches[0][0];

                // Create an EbmsArticle object for it
                // Throws Exception if parser or xml error
                $artObj = new EbmsArticle(null, $docXml, 'Pubmed');
                $offset = $matches[0][1] + strlen($docXml);

                // Return it
                $retRecs[] = $artObj;
                $recCount++;
            }
            else {
                // Nothing (more) found
                break;
            }
        }
    }
}

/**
 * Class for describing results of an import operation.
 *
 * An instance is returned when running in test mode, or when retrieving
 * data from ebms_import_batch and ebms_import_action.
 */
class ImportBatch {

    // Categories tracked in an import
    // These must EXACTLY match text_ids in ebms_import_disposition table
    static $categories = array('imported', 'notListed', 'duplicate',
                'reviewReady', 'topicAdded', 'replaced', 'error');

    /**
     *  Can load it from the database or from new import (expected to only
     *  be done inside the EbmsImport class.)
     *
     *  @param int $batchId     ebms_import_batch.import_batch_id to load.
     *                          If null, caller will initialize.
     */
    public function __construct($batchId=null) {

        if (is_null($batchId))
            // Other software must initialize for use
            $this->clear();
        else
            $this->load($batchId);
    }


    /**
     * Clear the object to a reasonable initial state.  Some values
     * will be overridden by setter functions for a new import or
     * by loading values from the database.
     */
    public function clear() {

        // Null data describing the batch as a whole
        $this->batchId      = null;
        $this->topicId      = null;
        $this->source       = null;
        $this->dateTime     = null;
        $this->cycleId      = null;
        $this->notList      = null;
        $this->inputType    = null;
        $this->userId       = null;
        $this->comment      = null;
        $this->articleCount = 0;

        // Building an instance from an import, we track unique source IDs
        // Loading from the database, we just have the count
        //  idCount == -1          = not yet loaded from the database
        //  uniqIds == empty array = not yet loaded from an import
        $this->idCount = -1;
        $this->uniqIds = array();

        // Data describing individual articles, by disposition
        // Note: One article can be in more than one category
        $this->action = array();
        foreach (self::$categories as $category)
            $this->action[$category] = array();
    }


    /**
     * Setters for overriding defaults.
     */
    public function setUserId($userId)       { $this->userId = $userId; }
    public function setSource($source)       { $this->source = $source; }
    public function setCycleId($cycleId)     { $this->cycleId = $cycleId; }
    public function setTopicId($topicId)     { $this->topicId = $topicId; }
    public function setNotList($notList)     { $this->notList = $notList; }
    public function setInputType($inputType) { $this->inputType = $inputType; }
    public function setDateTime($dateTime)   { $this->dateTime = $dateTime; }


    /**
     * Initialize instance with passed values.
     *
     *  @param string $source   Should always be 'Pubmed' for now, more later?
     *  @param int $topicId     Topic of this import, may be null?
     *  @param int $cycleId     Cycle when import occurred.
     *  @param int $notList     'Y' = NOT list used on import, else 'N'.
     *  @param string $comment  Comment to record as purpose (or whatever) of
     *                           this import batch.
     *  @param int $userId      User performing import, or use global.
     */
    public function init($source='Pubmed', $topicId=null, $cycleId=null,
                 $notList='Y', $inputType='R', $comment=null, $userId=null) {

        // Initialize to empty object
        $this->clear();

        // Topic may legitimately be null
        $this->topicId = $topicId;

        // Protect against inadvertent errors in source
        // Change this if we ever support additional sources
        if ($source != 'Pubmed')
            throw new \Exception(
             "ImportBatch->init(): not currently supporting input from '%s'," .
             "  Only supporting exact string 'Pubmed'");
        $this->source = $source;

        // Using NOT list is either 'Y' or 'N'
        if ($notList != 'Y' && $notList != 'N')
            throw new \Exception(
             "ImportBatch->init(): Valid notList values are 'Y' and 'N', " .
             " value '$notList' is invalid");
        $this->notList = $notList;

        // Input type is checked by database constraint
        $this->inputType = $inputType;

        // User
        if (is_null($userId)) {
            global $user;
            $userId = $user;
        }
        $this->userId = $userId;

        // Datetime of import is right now
        $this->dateTime = db_query('SELECT NOW()')->fetchField();

        // Cycle ID or most recent cycle before now
        if (is_null($cycleId))
            $cycleId  = getCurrentCycleId();
        $this->cycleId = $cycleId;

        $this->comment = $comment;
    }


    /**
     * Add a pubmed ID and description to an ImportBatch.
     *
     *  @param string $category   One of the legal categories, see below.
     *  @param string $sourceId   The source ID string, i.e. Pubmed ID
     *  @param int    $articleId  Internal EBMS articleId, if known.
     *  @param string $desc       Description, citation, title, err msg, etc.
     *
     *  @return void
     *
     *  @throw Exception if unrecognized category
     */
    public function addArticle($category, $sourceId, $articleId=null,
                                $desc=null) {
        // Pubmed ID + description
        $value = array($sourceId, $articleId, $desc);
        // logw("===>addArticle: $category + $sourceId + $articleId + $desc");

        // Append the value to the array accumulated for the category
        $this->action[$category][] = $value;

        // Another unique ID?
        $this->uniqIds[(string) $sourceId] = 1;
        $this->articleCount = count($this->uniqIds);
    }

    /**
     * How many unique articles were imported?  One article could have had
     * more than one disposition.
     *
     *  @return int Count.
     */
    public function getIdCount() {
        // See comments in the constructor
        if ($this->idCount < 0)
            return count($this->uniqIds);
        return $this->idCound;
    }

    /**
     * Store individual records in a list of records for each attempted
     * import.  Used to process all the imported, all the duplicates, all
     * the not listed, etc.
     *
     * Helper function for $this->store()
     *
     * No transactions here.  Transactions processed at a higher level.
     *
     *  @param int $batchId     Row ID for ebms_import_batch.
     *  @param string $dispTextId Disposition of this list, matches an
     *                           ebms_import_disposition.disposition_name.
     *  @param array $itemList  Array of data items, one per import attempt.
     *                           item[0] = Source (Pubmed) ID.
     *                           item[1] = Article ID, may be null.
     *                           item[2] = Text, may be null.
     *
     *  @return void
     *
     *  @throws Exception if serious error.
     */
    private function storeDispositions($batchId, $dispTextId, $itemList) {

        // Empty list?
        if (count($itemList) == 0)
            return;

        // Get the disposition ID for this item list
        $dispId = db_query('
            SELECT disposition_id
              FROM {ebms_import_disposition}
             WHERE text_id = :dName',
            array (':dName'=>$dispTextId))->fetchField();

        if (is_null($dispId))
            throw new \Exception(
             "storeDispositions: Unknown disposition name '$dispTextId'");

        // Store each item as a row
        foreach ($itemList as $item) {
            $fields = array(
                'source_id'       => $item[0],
                'article_id'      => $item[1],
                'import_batch_id' => $batchId,
                'disposition_id'  => $dispId,
                'message'         => $item[2]);

            // Insert the row
            $qry = db_insert('ebms_import_action')
              ->fields($fields)
              ->execute();
        }
    }

    /**
     * Store a complete EbmsImport object in the database.
     *
     *  @return int ebms_import_batch.batch_id.
     *
     *  @throws Exception if database error.
     */
    public function store() {

        // If no import attempted, do not store anything
        if ($this->idCount < 0 && count($this->uniqIds) == 0)
            throw new \Exception('EbmsImport.store: Nothing to store');

        // Start a transaction for the whole batch
        $txn = db_transaction();

        try {
            // Save information that applies to the whole batch
            $qry = db_insert('ebms_import_batch')
              ->fields(array(
                  'topic_id'     => $this->topicId,
                  'source'       => $this->source,
                  'import_date'  => $this->dateTime,
                  'cycle_id'     => $this->cycleId,
                  'not_list'     => $this->notList,
                  'input_type'   => $this->inputType,
                  'user_id'      => $this->userId,
                  'comment'      => $this->comment,
                  'article_count'=> count($this->uniqIds)
                ));
            $this->batchId = $qry->execute();

            // Save each set of attempted import IDs
            foreach (self::$categories as $category)
                $this->storeDispositions($this->batchId, $category,
                                         $this->action[$category]);
        }
        catch (Exception $e) {
            $txn->rollback();
            throw $e;
        }

        // Commit is automatic
        return $this->batchId;
    }


    /**
     * Load an ImportBatch object from the database.  Having this function
     * enables us to use the same report software in a test load, a live
     * load, and a retrospective look at past loads.
     *
     * Example use: $ib = new ImportBatch($id); // See constructor.
     *
     *          or: $ib = new ImportBatch();
     *              $ib->load($id);
     *
     *  @param int $batchId     ID of batch to load.
     *
     *  @return loaded object.
     *
     *  @throws Exception if batch ID not found or db error.
     */
    public function load($batchId) {

        // Remove whatever is there
        $this->clear();

        // Load object
        $result = db_query('
            SELECT *
              FROM {ebms_import_batch}
             WHERE import_batch_id = :batchId',
            array(':batchId' => $batchId));
        $row = $result->fetchObject();
        if (!$row)
            throw new \Exception(
                    "ImportBatch.load: no import batch with id=$batchId");
        $this->batchId      = $row->import_batch_id;
        $this->topicId      = $row->topic_id;
        $this->source       = $row->source;
        $this->dateTime     = $row->import_date;
        $this->cycleId      = $row->cycle_id;
        $this->notList      = $row->not_list;
        $this->inputType    = $row->input_type;
        $this->userId       = $row->user_id;
        $this->comment      = $row->comment;
        $this->articleCount = $row->article_count;

        // Load all of the different article actions
        $query = db_select('ebms_import_action', 'act');
        $query->join('ebms_import_disposition', 'disp',
          'act.disposition_id = disp.disposition_id');
        $query->addField('act', 'source_id');
        $query->addField('act', 'article_id');
        $query->addField('disp', 'text_id');
        $query->addField('act', 'message');
        $query->condition('act.import_batch_id', $batchId, '=');
        $result = $query->execute();

        // Install each one in the object
        while ($row = $result->fetchObject()) {
            $this->addArticle($row->text_id, $row->source_id,
                              $row->article_id, $row->message);
        }
    }


    /**
     * Dump the contents in HTML format - strictly for debugging.
     *
     * @return String of html.
     */
    public function dumpHtml() {

        // Some values to display that are derived, not stored
        $topicName = getTopicNameById($this->topicId);
        $boardName = getBoardNameById(getTopicBoard($this->topicId));

        $html = <<<EOS
<hr />
<h3>Summary of Batch Import</h3>
<table border='1'>
 <tr><th>batchId</th><td>$this->batchId</td></tr>
 <tr><th>boardName</th><td>$boardName</td>
 <tr><th>topicName</th><td>$topicName</td>
 <tr><th>source</th><td>$this->source</td></tr>
 <tr><th>dateTime</th><td>$this->dateTime</td></tr>
 <tr><th>cycleId</th><td>$this->cycleId</td></tr>
 <tr><th>notListUsed</th><td>$this->notList</td></tr>
 <tr><th>inputType</th><td>$this->inputType</td></tr>
 <tr><th>userId</th><td>$this->userId</td></tr>
 <tr><th>comment</th><td>$this->comment</td></tr>
 <tr><th>articleCount</th><td>$this->articleCount</td></tr>
</table>
<p />
EOS;

        if ($this->articleCount > 0) {
            $html .= "<h3>Disposition of Articles</h3>\n";

            // Add any categories having one or more actions
            foreach (self::$categories as $category) {
                if (count($this->action[$category]) > 0)
                  $html .= $this->dumpCat($category, $this->action[$category]);
            }
        }

        return $html;
    }


    /**
     * Dump one category, helper for dumpHtml().
     *
     * @param string $category  Name of category for display.
     * @param array  $data      Data array for category.
     *
     * @return String of html.
     */
    private function dumpCat($category, $data) {

        $catCount = count($data);
        $html = <<<EOS
<table border='1'>
 <tr><th colspan='3'><center>$category / $catCount</center></th></tr>
 <tr><th><center>sourceId</center></th>
     <th><center>articleId</center></th>
     <th><center>Messages</center></th>
 </tr>
EOS;

        // Add each row
        $rows = array();
        foreach ($data as $d)
            $rows[] = "<tr><td>$d[0]</td><td>$d[1]</td><td>$d[2]</td><tr>";

        // Join them up
        $html .= implode("\n", $rows) . "</table>\n";

        return $html;
    }
}



/**
 * Load all of the journal IDs from a NOT list into an in in-memory
 * associative array for quick searching.
 *
 * NOT lists are source specific.
 *
 *  @param int $boardId     Board controlling this NOT list.
 *  @param string $source   Generally 'Pubmed'.
 *
 *  @return array of journal IDs.  Could be empty if no journals on this
 *                                 not list or source unknown.
 *
 *  @throws Exception if database error.
 */
function loadNotList ($boardId, $source='Pubmed') {

    // Creates an array of jrnlId => 1
    $result = db_query (
            'SELECT source_jrnl_id, 1
               FROM {ebms_not_list}
              WHERE source = :source
                AND board_id = :board',
            array(':source' => $source, ':board' => $boardId));

    if (!is_null($result))
        // Array of journal_id => 1 ...
        return $result->fetchAllKeyed();
    else
        return array();
}


/**
 * Import one or more articles from NLM.  Puts all of the above together.
 *
 * The program treats each Pubmed article as a separate transaction.
 * If an error occurs in one of the reords the program rolls back any
 * changes for that one article, records the error, and continues.
 *
 * A cascade of errors could theoretically cause many errors to be
 * generated by a single bad article, but the user can fix the error and
 * safely re-run the entire import.  Articles saved in the previous run
 * will not be re-stored but treated as duplicates.
 *
 *
 *  @param string $mode         "live" to store articles, "test" to go through
 *                               the motions and reveal potential errors.
 *  @param array $articleIds    Array of Pubmed IDs, each as a strings
 *                               of digits, e.g., "12345678".
 *  @param int $cycleId         Review cycle ID, required.
 *  @param int $topicId         Summary topic for which we are importing.
 *                               Could be null but think that should be rare.
 *  @param string $comment      Comment to record in import_batch table (only
 *                               in live mode), default is none.
 *                               Comment is also recorded in setArticleState()
 *                               call.  XXX Should it be?
 *  @param boolean $useNotList  True = use the "NOT list" for the board
 *                               for this topic.  Default = true, but NOT
 *                               list is always ignored if $topicId is null.
 *  @param string $inputType    'R'egular, 'F'ast track, or 'S'pecial search.
 *  @param int $userId          User running the import, defaults to current
 *                               user.
 *
 *  @return ImportBatch object  populated with results of the run.
 *                               in live mode this information is also
 *                               saved in the import_batch and related
 *                               tables and can be recalled when needed.
 *
 *  @throws Exception if database or other error.
 */
function importArticlesFromNLM($mode, $articleIds, $topicId, $cycleId=null,
           $comment=null, $useNotList=true, $inputType='R', $userId=null) {

    // Max we'll fetch at once - good citzenship
    define('PUBMED_BATCH_SIZE', 100);

    // Have to have some info
    if ($mode != 'live' && $mode != 'test')
        throw new \Exception("importArticlesFromNLM: illegal mode \"$mode\"");
    if (count($articleIds) < 1)
        throw new \Exception("importArticlesFromNLM: empty PMID list");
    if (is_null($cycleId) || $cycleId < 1) {
        // XXX Add more checks?  Must be one of last one or two?  Ask users
        throw new \Exception(
            "importArticlesFromNLM: illegal cycleId=$cycleId");
    }
    if (is_null($userId)) {
        global $user;
        $userId = $user->uid;
    }

    // Board for this topic
    if (!is_null($topicId))
        $boardId = getTopicBoard($topicId);
    else
        $boardId = null;

    // Load not list if needed
    $notList = array();
    if ($useNotList && !is_null($boardId))
        $notList = loadNotList($boardId, 'Pubmed');

    // Instantiate a PubmedInterface to talk to NLM
    $pi = new PubmedInterface($articleIds);

    // Instantiate an ImportBatch to contain stats and report
    $ib = new ImportBatch();
    $initNotList = $useNotList ? 'Y' : 'N';
    $ib->init('Pubmed', $topicId, $cycleId, $initNotList, $inputType,
               $comment, $userId);

    // Get batches of articles from NLM until we got them all
    while ($pi->getRemainCount() > 0) {

        // Hold results of NLM request
        $recs = array();
        $errs = array();

        // Get a batch from NLM
        $pi->getRecsFromNLM(PUBMED_BATCH_SIZE, $recs, $errs);
        /*
        logw("===>getRemainCount2=" . $pi->getRemainCount());
        logw("===>Count recs=" . count($recs));
        logw("===>Count errs=" . count($errs));
        */

        // Process each one
        foreach ($recs as $rec) {
            // Properties of the record
            $pmid      = $rec->getSourceId();
            $nlmJrnlId = $rec->getSourceJrnlId();
            $topicObjs = array();
            $msgs      = null;

            // Article has no discovered state yet
            $oldTopicState = null;

            // XXX ISSUE: Do we want this?  Or should the user interface
            //  just hyperlink pmids to Pubmed?
            $brfCite = $rec->getBrfCite();

            // Search our database for the pubmed ID
            // Gets article_id or false
            $result = db_query('
                SELECT article_id
                  FROM {ebms_article}
                 WHERE source = :source
                   AND source_id = :pmid',
                array(':source'=>'Pubmed', ':pmid'=>$rec->getSourceId()));
            $foundId = $result->fetchField();

            // If found
            if ($foundId) {
                // Brand this new downloaded article with internal ID
                $rec->setArticleId($foundId);

                // Did it have any state under this topic ID?
                $oldTopicState = getCurrentState($foundId, $topicId);

                // Find out what topics we stored this under
                $topicObjs = getTopics($foundId);

                // Has the XML changed?
                $oldRec = new EbmsArticle($foundId);
                $msgs   = $oldRec->getErrorMsgs();
                if ($rec->getSourceData() != $oldRec->getSourceData()) {
                    if ($mode == 'live')
                        $rec->store($pi->constantDt, $userId);
                    $ib->addArticle('replaced', $pmid, $foundId, $msgs);
                }

                // Replaced or not, it's a duplicate
                $ib->addArticle('duplicate', $pmid, $foundId, $msgs);

                // Have we got a new topic, not seen in this article before?
                $newTopic = true;
                foreach ($topicObjs as $top) {
                    if ($top->topic_id == $topicId) {
                        $newTopic = false;
                        break;
                    }
                }
                if ($newTopic) {
                    if ($mode == 'live') {
                        $rec->setArticleState('ReadyInitReview',
                            $topicId, $boardId, $comment, $userId,
                            $pi->constantDt);
                    }

                    $ib->addArticle('topicAdded', $pmid, $foundId, $msgs);
                    $ib->addArticle('reviewReady', $pmid, $foundId, $msgs);
                }
            }
            else {
                if ($mode == 'live') {
                    // Save it, populating the article ID and import info
                    $rec->store();
                }
                $ib->addArticle('imported', $pmid, $rec->getId(), null);

                // If not marking it for not list, mark it ready for review
                if (!$useNotList || !array_key_exists($nlmJrnlId, $notList)) {
                    if ($mode == 'live') {
                        $rec->setArticleState('ReadyInitReview',
                               $topicId, $boardId, $comment, $userId,
                               $pi->constantDt);
                    }
                    $ib->addArticle('reviewReady', $pmid, $rec->getId(), null);
                }
            }

            // Is it not listed?
            if ($useNotList && array_key_exists($nlmJrnlId, $notList)) {

                // If it was previously processed for this topic,
                //   we don't want to reject it now.  That could override
                //   a human's decision.
                if (is_null($oldTopicState)) {
                    if ($mode == 'live') {
                        $rec->setArticleState('RejectJournalTitle',
                               $topicId, $boardId, $comment, $userId,
                               $pi->constantDt);
                    }
                    $ib->addArticle('notListed', $pmid, $rec->getId(), $msgs);
                }
            }

            // Done with this article, do the next
        }

        // Record errors.  We do not have PMIDs but they may be in msgs
        foreach ($errs as $err) {
            // Try to find a Pubmed ID
            if (preg_match(PMID_PAT, $err, $matches))
                $errPmid = $matches[1];
            else
                $errPmid = null;

            $ib->addArticle('error', $errPmid, $err);
        }
    }

    // Store and return results
    if ($mode == 'live')
        $ib->store();
    return($ib);
}


/**
 * Get the latest (therefore current) cycle ID.
 *
 *  @return int    Latest cycle_id.
 *
 *  @throws Exception if cannot get it.
 */
function getCurrentCycleId() {

    // Cycle ID or most recent cycle before now
    $cycleId  = db_query('
        SELECT cycle_id
          FROM {ebms_cycle}
         WHERE start_date in (
             SELECT max(start_date)
               FROM {ebms_cycle}
              WHERE start_date < now()
          )')->fetchField();

    if (!$cycleId)
        throw new \Exception(
                'EbmsImport->getCurrentCycleId() could not init cycleId');

    return $cycleId;
}


/**
 * Parse Pubmed IDs out of a Pubmed search result.
 *
 * At the outset of EBMS development, we plan to implement the importation
 * of Pubmed records using a similar starting stage as in the old CiteMS.
 * A user will run a Pubmed search, get back a result list, save it to
 * their computer, then upload it into the EBMS.
 *
 * Pubmed search results come in many flavors.  We'll attempt to recognize
 * them and parse appropriately.  The first implementation will be for
 * the Medline print format - which has been used for ten years in CiteMS.
 *
 *  @param int $resultFid       Unique file ID in the managed_file table.
 *  @param string $resultFile   Name of a file of search results, accessible
 *                               to the web server.
 *  @param string $resultStr    Instead of a file, this is the result list
 *                               itself.
 *
 *                              Use only one of Fied, File or Str.
 *
 *  @return array               Array of Pubmed IDs as ASCII digit strings.
 *
 *  @throws Exception           If bad filename, parms, out of memory, etc.
 */
function findPubmedIDs($resultFid, $resultFile, $resultStr) {

    // Resolve passed parms to a text string
    $parmCount = 0;
    if ($resultFid) {
        $fileObj = file_load($resultFid);
        if (!$fileObj)
          throw new \Exception("findPubMedIDs called with bad fid=$resultFid");
        $filepath = drupal_realpath($fileObj->uri);
        $dataStr  = file_get_contents($filepath);
        ++$parmCount;
    }
    if ($resultFile) {
        if ($parmCount == 0)
            $dataStr = file_get_contents($resultFile);
        ++$parmCount;
    }
    if ($resultStr) {
        $dataStr == $resultStr;
        ++$parmCount;
    }

    // Validate params
    if ($parmCount != 1)
        throw new \Exception(
            "findPubmedIDs: Pass exactly one fileId, filename or string");

    // One way or the other we now have a string
    // If downloaded via browser it might have a Unicode Byte Order Mark
    // BOM = Hex: EF BB BF.  It's only legal at the start of a string.
    $BOM = chr(239) . chr(187) . chr(191);
    if (substr($dataStr, 0, 3) == $BOM)
        $dataStr = substr($dataStr, 3);

    // Is there a Medline format Pubmed ID?
    $matches = array();
    $found = preg_match(MEDLINE_PMID_PAT, $dataStr, $matches,
                        PREG_OFFSET_CAPTURE);
    if ($found) {
        // Extra check.  Depending on how the file was created, there may
        //  be an xhtml declaration at the front, but little more
        if ($matches[0][1] > 500) {
            throw new \Exception(
                "Looks like Medline Print Format, " .
                "but garbage at front?");
        }
        return findPubmedIDsInMedlineFmt($dataStr);
    }
    else $found = preg_match(PUBMED_XML_START_PAT, $dataStr, $matches,
                        PREG_OFFSET_CAPTURE);
    if ($found) {
        // Won't be found if saved as html, see exception below
        return findPubmedIDsInXML($dataStr);
    }
    throw new \Exception(
        "Unsupported Pubmed search result format - " .
        "only accepting Medline print format (text) or Pubmed XML.  " .
        "For clean data, use Pubmed's 'Send to: File' to save the data)");
}

/**
 * Parse Pubmed IDs out of a Medline Print Format search result.
 *
 *  @param string $resultStr    The search result as a single string.
 *
 *  @return array               Array of Pubmed IDs as ASCII digit strings.
 *
 *  @throws Exception           If unplanned for result.
 */
function findPubmedIDsInMedlineFmt($resultStr) {

    $matches = array();
    $count = preg_match_all(MEDLINE_PMID_PAT, $resultStr, $matches);
    if ($count == 0) {
        throw new \Exception(
            "findPubmedIDsInMedlineFormat: Couldn't find any '^PMID- nnn'. " .
            " How can that happen?");
    }

    // Return the pmids - the first subpattern in MEDLINE_PMID_PAT
    return $matches[1];
}

/**
 * Helper class for findPubmedIDsInXML().
 */
class PmidSaxHandler {

    /**
     * Constructor
     */
    function __construct() {

        // Stack of element names from root to current element
        $this->elemNameStack = array();

        // Pubmed IDs that we find go here
        $this->pmidList = array();

        // Index of top of stack.  First element is at depth 0
        $this->stackTop = -1;
    }

    // Callback for element start tag
    function startElementHdlr($prsr, $name, $attribs) {
        // Record that we've started one
        $this->elemNameStack[] = $name;
        ++$this->stackTop;
    }

    // Callback for element data.
    // This version is very simple, just handles PMID strings.
    function characterHdlr($prsr, $charStr) {
        // PMID elements occur in more than one place in the schema
        // We only want the one that's a child of MedlineCitation
        if ($this->elemNameStack[$this->stackTop] == 'PMID')
            if ($this->elemNameStack[$this->stackTop-1] == 'MedlineCitation')
                // Trim isn't needed but programming defensively here
                $this->pmidList[] = trim($charStr);
    }

    // Callback for element end tag
    function endElementHdlr($prsr, $name) {
        array_pop($this->elemNameStack);
        --$this->stackTop;
    }

    // Get the Pubmed IDs we found
    function getPmids() {
        return $this->pmidList;
    }
}

/**
 * Parse Pubmed IDs out of a Pubmed XML search result format.
 * This is the full article record XML, not the brief citation result set.
 *
 *  @param string $xmlStr       UTF-8 serial XML from Pubmed.
 *
 *  @return array               Array of Pubmed IDs as ASCII digit strings.
 *
 *  @throws Exception           If SAX parsing error.
 */
function findPubmedIDsInXML($xmlStr) {

    // Instantiate the handler
    $hdlr = new PmidSaxHandler();

    // Create a parser object "resource"
    $parser = xml_parser_create('UTF-8');

    // Default php uppercases all element names - good for html, not xml
    xml_parser_set_option($parser, XML_OPTION_CASE_FOLDING, false);

    // Establish handler callbacks
    xml_set_object($parser, $hdlr);
    xml_set_element_handler($parser, 'startElementHdlr', 'endElementHdlr');
    xml_set_character_data_handler($parser, 'characterHdlr');

    // Parse out all of the Pubmed IDs
    $ok = xml_parse($parser, $xmlStr, true);
    if (!$ok)
        throw new \Exception("findPubmedIDsInXML xml_parse returned false");

    // return the results
    return $hdlr->getPmids();
}

// Unit test of findPubmedIDsInXML
function testFindPubmedIDsInXML($fname) {
    $xmlStr = file_get_contents($fname);
    $pmids = findPubmedIDsInXML($xmlStr);
    foreach($pmids as $pmid)
        print("$pmid\n");
}
