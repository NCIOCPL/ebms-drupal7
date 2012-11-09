<?php

namespace Ebms;

// Maximum author elements, truncate if necessary
define('Ebms\MAX_COLLECTNAME', 1023);
define('Ebms\MAX_SURNAME', 255);
define('Ebms\MAX_FORENAME', 128);
define('Ebms\MAX_INITIALS', 128);
define('Ebms\MAX_ART_TITLE', 512);
define('Ebms\MAX_JRNL_TITLE', 512);

/**
 * A simple interface to enumerate the article state text IDs.
 */
interface EbmsArticleStates {
    const ReadyInitReview = 'ReadyInitReview';
    const RejectJournalTitle = 'RejectJournalTitle';
    const RejectInitReview = 'RejectInitReview';
    const PassedInitReview = 'PassedInitReview';
    const Published = 'Published';
    const PassedBMReview = 'PassedBMReview';
    const RejectBMReview = 'RejectBMReview';
    const RejectFullReview = 'RejectFullReview';
    const PassedFullReview = 'PassedFullReview';
    const FYI = 'FYI';
    const FullEnd = 'FullEnd';
    const NotForAgenda = 'NotForAgenda';
    const AgendaFutureChg = 'AgendaFutureChg';
    const AgendaFutureDiscuss = 'AgendaFutureDiscuss';
    const OnAgenda = 'OnAgenda';
    const FinalBoardDecision = 'FinalBoardDecision';
}

validateArticleStates();

function validateArticleStates() {
    $states = array(
        EbmsArticleStates::ReadyInitReview,
        EbmsArticleStates::RejectJournalTitle,
        EbmsArticleStates::RejectInitReview,
        EbmsArticleStates::PassedInitReview,
        EbmsArticleStates::Published,
        EbmsArticleStates::PassedBMReview,
        EbmsArticleStates::RejectBMReview,
        EbmsArticleStates::RejectFullReview,
        EbmsArticleStates::PassedFullReview,
        EbmsArticleStates::FYI,
        EbmsArticleStates::FullEnd,
        EbmsArticleStates::NotForAgenda,
        EbmsArticleStates::AgendaFutureChg,
        EbmsArticleStates::AgendaFutureDiscuss,
        EbmsArticleStates::OnAgenda,
        EbmsArticleStates::FinalBoardDecision,
    );

    $currStates = db_select('ebms_article_state_type', 't')
        ->fields('t', array('state_text_id'))
        ->execute()
        ->fetchCol();

    $localStates = array_diff($states, $currStates);
    $dbStates = array_diff($currStates, $states);
    if (!empty($localStates)) {
        $wrongStates = implode(',', $localStates);
        throw new \Exception(
            "$wrongStates = state types missing from database!");
    }
    if (!empty($dbStates)) {
        // XXX FIX ME WHEN THE DB IS REBUILT W/O THE REMOVED STATE TYPES
        $wrongStates = implode(',', $dbStates);
        // throw new \Exception(
        drupal_set_message(
            "$wrongStates = unknown state types found in database!");
    }
}

/**
 * One article / topic association
 */
class EbmsArticleTopic {
    public
        $article_id,    // Unique ID of article
        $topic_id,      // ID of topic
        $topic_name,    // Human readable name
        $board_id,      // Board that owns this topic
        $board_name;    // Human readable name

    public function __toString() {
        $term  = "<br />\n";
        $text  = "article_id: {$this->article_id}$term";
        $text .= "  topic_id: {$this->topic_id}$term";
        $text .= "topic_name: {$this->topic_name}$term";
        $text .= "  board_id: {$this->board_id}$term";
        $text .= "board_name: {$this->board_name}$term";

        return $text;
    }
}

/**
 * One author of an article.
 */
class EbmsArticleAuthor {

    /**
     * Constructor
     *
     *  @param int    $aId          - Row ID in ebms_article_author or null.
     *  @param string $collectName  - CollectiveName.
     *  @param string $lastName     - Surname.
     *  @param string $foreName     - First name(s), may be null.
     *  @param string $inits        - First and optional more initials.
     */

    function __construct($aId, $collectName, $lastName, $foreName, $inits) {

        // Translate any utf-8 characters using routine in this module
        require_once('common.inc');

        $this->msg = null;

        // Empty strings are not allowed.  Use nulls.  Otherwise we
        //  will violate our unique index constraints.
        if (!$collectName) $collectName = null;
        if (!$lastName)    $lastName    = null;
        if (!$foreName)    $foreName    = null;
        if (!$inits)       $inits       = null;

        // CollectiveName should not appear with the others but, if it
        //   does, we will accept it
        if ($collectName)
            $collectName = Util::indexify($collectName, true, true,
                                          MAX_COLLECTNAME);

        // Normalize and validate data
        $lastName = Util::indexify($lastName, true, true, MAX_SURNAME);
        $foreName = Util::indexify($foreName, true, true, MAX_FORENAME);
        $initials = Util::indexify($inits, true, true, MAX_INITIALS);

        if (!$lastName && !$collectName) {
            // Initially, this could happen due to not including
            // CollectiveName in the database design and conversion
            // In any case, let's not throw an exception
            $this->msg =
                "Article ID=$aId has author with no Collective or Last Name";
        }

        $this->author_id       = $aId;
        $this->last_name       = $lastName;
        $this->forename        = $foreName;
        $this->initials        = $initials;
        $this->collective_name = $collectName;
    }
}

/**
 * One journal article record.
 */
class EbmsArticle {

    // Variables corresponding to the cite table in the database
    private
      $article_id,          // Unique auto_increment ID
      $source,              // 'Pubmed', maybe more later
      $source_id,           // Unique string ID within source
      $source_jrnl_id,      // Unique string journal ID within source
      $source_status,       // Whatever source uses, needed for updates
      $article_title,       // Title of the journal article
      $jrnl_title,          // Title of journal at last update time
      $brf_jrnl_title,      // Comes from the source
      $import_date,         // First time saved in EBMS
      $imported_by,          // User who imported it the first time
      $update_date,         // Most recent replacement from source, or null
      $abstract,            // Abstract string
      $published_date,      // Date on journal article
      $brf_citation,        // One line citation to this article
      // Array of EbmsArticleAuthor in order in which they appeared
      //  in the article record
      $authors,
      // Source text for the data, maybe always XML
      $source_data,
      // Drupal external file identifier, not the pdf itself
      $full_text_id,
      // Used only if error occurred with this object
      // The errors are strings in an array
      // Else null
      $errorMsgs;

    /**
     * Construct an object using either data from the database identified
     * by article_id, or data from NLM in Pubmed XML article format.  Other
     * formats to be supported if and when they are required.
     *
     *  @param int    $articleId    Article ID, if constructing from the DB.
     *  @param string $xmlStr       Serial XML, if constructing from a string.
     *  @param string $sourceFmt    Source format.  The only one currently
     *                                supported is 'Pubmed', and only the
     *                                format for journal articles, not
     *                                book chapters, etc. is supported.
     *
     *  @throws Exception           If parse, format, or other error.
     */
    public function __construct($articleId=null,
                                $xmlStr=null, $sourceFmt='Pubmed') {

        // Must provide either an ID or xml
        if (is_null($articleId) && is_null($xmlStr))
            throw new \Exception(
                    "EbmsArticle constructor: Must pass either " .
                    "one of articleId or xmlStr");

        // And not both
        if ($articleId && $xmlStr)
            throw new \Exception(
                    "EbmsArticle constructor: Must pass either " .
                    "articleId or xmlStr, not both");

        // Initialize object to default values
        $this->clear();

        // Load from database
        if ($articleId)
            $this->loadFromId($articleId);

        // Load from imported XML
        else {
            // Only supporting Pubmed
            if ($sourceFmt != 'Pubmed')
                throw new \Exception(
                    "EbmsArticle constructor: unsupported format $sourceFmt");

            // Load it
            $this->loadPubmedXmlStr($xmlStr);

            // Save the xml string itself
            $this->source_data = $xmlStr;
        }
    }

    /**
     * Load the object from the database using an internal unique ID.
     *
     *  @param int $articleId   Unique row id.
     *
     *  @throws Exception       If database error or ID not found.
     */
    public function loadFromId($articleId) {

        // Only loads what is in the row, get other stuff lazily
        $result = db_query(
                'SELECT *
                   FROM {ebms_article}
                  WHERE article_id = :artId',
                array(':artId' => $articleId));

        // Caller should not be using an internal ID that doesn not exist
        $row = $result->fetchObject();
        if (!$row) {
            // Record error and throw exception
            $msg = 'EbmsArticle.__construct(id): ' .
                   "No database row found for article ID=$articleId";
            $this->addError($msg, WATCHDOG_ERROR, true);
        }

        // Can not be more than one
        $noRow = $result->fetchObject();
        if ($noRow) {
            // Record error and throw exception
            $msg = 'EbmsArticle.__construct(id): ' .
                   "$rowCount database rows found for article ID=$articleId";
            $this->addError($msg, WATCHDOG_ERROR, true);
        }

        // Load the object from the row
        // Other parts will come from elsewhere
        $this->article_id     = $row->article_id;
        $this->source         = $row->source;
        $this->source_id      = $row->source_id;
        $this->source_jrnl_id = $row->source_jrnl_id;
        $this->source_status  = $row->source_status;
        $this->article_title  = $row->article_title;
        $this->jrnl_title     = $row->jrnl_title;
        $this->brf_jrnl_title = $row->brf_jrnl_title;
        $this->import_date    = $row->import_date;
        $this->imported_by    = $row->imported_by;
        $this->update_date    = $row->update_date;
        $this->abstract       = $row->abstract;
        $this->published_date = $row->published_date;
        $this->brf_citation   = $row->brf_citation;
        $this->authors        = $this->getAuthors();
        $this->source_data    = $row->source_data;
        $this->full_text_id   = $row->full_text_id;
        $this->errorMsgs      = null;
    }

    /**
     * Clear all values from an EbmsArticle object.
     */
    public function clear() {
        $this->article_id     = null;
        $this->source         = null;
        $this->source_id      = null;
        $this->source_jrnl_id = null;
        $this->source_status  = null;
        $this->article_title  = null;
        $this->jrnl_title     = null;
        $this->brf_jrnl_title = null;
        $this->import_date    = null;
        $this->imported_by    = null;
        $this->update_date    = null;
        $this->abstract       = null;
        $this->published_date = null;
        $this->brf_citation   = null;
        $this->authors        = null;
        $this->source_data    = null;
        $this->full_text_id   = null;
        $this->errorMsgs      = null;
    }

    /**
     * Simple getters get data that was loaded from the ebms_article row.
     */
    public function getId()           { return $this->article_id; }
    public function getSource()       { return $this->source; }
    public function getSourceId()     { return $this->source_id; }
    public function getSourceJrnlId() { return $this->source_jrnl_id; }
    public function getSourceStatus() { return $this->source_status; }
    public function getArticleTitle() { return $this->article_title; }
    public function getJrnlTitle()    { return $this->jrnl_title; }
    public function getBrfJrnlTitle() { return $this->brf_jrnl_title; }
    public function getBrfCite()      { return $this->brf_citation; }
    public function getAbstract()     { return $this->abstract; }
    public function getPubDate()      { return $this->published_date; }
    public function getImportDate()   { return $this->import_date; }
    public function getImportedBy()   { return $this->imported_by; }
    public function getUpdateDate()   { return $this->update_date; }

    /**
     * Complex method of getting brief citation.
     *
     * XXX TO BE DONE XXX
     */

    /**
     * Set the article_id.
     *
     * For a new record, this is done in store().  For an existing record
     * it is done in the constructor.  But when we bring down a duplicate
     * of an existing record from NLM, we need to identify downloaded version
     * by reference to the article it replaces or duplicates.
     *
     * @param int $articleId    ID of the existing record that this duplicates.
     */
    public function setArticleId($articleId) {
        $this->article_id = $articleId;
    }

    /**
     * This one is a bit more complicated.  It returns a string of
     * all message, or null.
     */
    public function getErrorMsgs() {
        if (count($this->errorMsgs) == 0)
            return null;
        return implode('; ', $this->errorMsgs);
    }

    /**
     * Retrieve authors.  Uses cache if available, else goes to the DB.
     *
     *  @param int|null $maxAuthors      - Max number to fetch, null=all.
     *                                     If negative, fetch from end.
     *
     *  @return array(EbmsArticleAuthor) - Retrieved authors as objects.
     *                                     Null if there are no authors.
     *
     *  @throws Exception                - On database or other error.
     */
    public function getAuthors($maxAuthors = null) {

        // If no max given, get all (using an impossibly high max)
        if (!$maxAuthors)
            $maxAuthors = 99999;

        // If we do not already have them, get them
        if (!$this->authors) {
            // If no authors and no database article ID, something bad happened
            if (!$this->article_id)
                throw new \Exception("EbmsArticle->getAuthors: " .
                  "Attempted to retrieve authors when none present" .
                  " and no article ID present.");

            // Fetch all of them, otherwise caching defeats us if next
            //  caller wants more than we got.
            // This might get more than we need but we will only optimize
            //  if experience shows the need
            $result = db_query("
             SELECT a.author_id, a.collective_name,
                    a.last_name, a.forename, a.initials
               FROM {ebms_article_author} a
               JOIN {ebms_article_author_cite} c
                 ON a.author_id = c.author_id
              WHERE c.article_id = :artId
              ORDER BY c.cite_order",
                  array(':artId' => $this->article_id))->fetchAll();

            if (count($result) == 0)
                // It turns out that a very few articles don't have any
                //  in the record
                return null;

            // Copy each stdClass object to an Author object
            foreach ($result as $row) {
                $a = new EbmsArticleAuthor($row->author_id,
                        $row->collective_name, $row->last_name,
                        $row->forename, $row->initials);
                if ($a->msg)
                    $this->addError($a->msg);
                else
                    $this->authors[] = $a;
            }
        }

        // Extract from beginning or end
        if ($maxAuthors > 0)
            return array_slice($this->authors, 0, $maxAuthors);
        else
            return array_slice($this->authors, $maxAuthors, 99999);
    }

    /**
     * Retrieve source data, maybe always XML.
     * Uses cache if available, else goes to the DB.
     *
     *  @return mixed     - Source data, usually a string.
     *
     *  @throws Exception - On database or other error.
     *
     * NOTE: Current version of both ways to construct an EbmsArticle
     *       object (i.e., from XML, or by retrieval from our database)
     *       retrieve the data.  If that turns out to be too expensive
     *       in time or memory for some uses, then this function can
     *       get the data on demand, caching it for further use.
     */
    public function getSourceData() {
        if (!$this->source_data) {
            $result = db_query("
             SELECT source_data
               FROM {ebms_article}
              WHERE article_id = :artId",
               array(':artId' => $this->article_id));

            if ($result)
                $this->source_data = $result->fetchField();

            else
                // Source data should ALWAYS be present
                throw new \Exception(
                 "EbmsArticle.getSourceData: no source data, cannot happen.");
        }
        return $this->source_data;
    }

    /**
     * Get the full text ID of the article if we have it.
     *
     * This currently returns a unique Drupal file_managed.fid unsigned int.
     *
     *  @return unsigned int - Drupal file ID, null if no full text stored.
     *
     * To get the file type or the actual file, look first in the Drupal
     * file_managed table.
     */
    public function getFullTextId() {
        return $this->full_text_id;
    }

    /**
     * Set the full text ID of the article if we have it.
     *
     * Initially, this will always be for a PDF doc, but conceptually it
     * could be anything such as a Word, XML, or text document.
     *
     * Note: Nothing happens in the database unless $this->store() is called.
     *       For immediate storage of an already existing record, call
     *       storeArticleFullText() instead.
     *
     *  @param unsigned int $fullTextFileId   An ID key into the Drupal
     *                                         "file_managed" table.
     *
     *  @return void.
     *
     */
    public function setFullText($fullTextFileId) {
        $this->full_text_id = $fullTextFileId;
    }

    /**
     * EbmsArticle interface to setArticleState(), q.v.
     *
     *
     *  @param string $textId    - Name used in preference to ID.  The code is
     *                              less efficient but caller's code is more
     *                              self-documenting.
     *  @param int $topicId      - Topic identifier.
     *  @param int $boardId      - Board identifier.  See notes on free
     *                              standing routine.
     *  @param int $comment      - Optional user or program supplied comment.
     *  @param int $userId       - User setting the state.  If null use current.
     *  @param int $dateTime     - Date time of action.  If null, use now.
     *
     *  @return - int article_state_id.  This is the id of the new row
     *            in the ebms_article_state table, not the type table.
     *
     *  @throws Exception if invalid parameters  or lower level error.
     */
    public function setArticleState($textId, $topicId=null,
        $boardId=null, $comment=null, $userId=null, $dateTime=null) {

        return setArticleState($this->article_id, $textId, $topicId,
            $boardId=null, $comment=null, $userId=null, $dateTime=null);
    }

    /**
     * Append an error message.
     *
     *  @param string $msg   - Error message.  Should be unique in code.
     *                         If it's an error, add each msg individually
     *  @param const $level  - Drupal watchdog() severity level.
     *                          Null=Do not log anything via watchdog.
     *  @param bool   $throw - True = also throw exception.
     */
    private function addError($msg, $level=null, $throw=false) {

        // Messages are appeneded to the array for this article
        if (is_null($this->errorMsgs))
            $this->errorMsgs = array();

        // Combine strings if required
        if (is_array($msg)) {
            foreach ($msg as $m)
                $this->errorMsgs[] = $msg;
        }
        else
            $this->errorMsgs[] = $msg;

        // Write to Drupal watchdog log
        if (!is_null($level))
            watchdog('EbmsArticle', $msg, null, $level);

        if ($throw)
            throw new \Exception($msg);
    }

    /**
     * Load this object from a parsed XML tree.
     *
     *  @param object $pma    - PubmedArticle node object for one article.
     *
     *  @return void
     *
     *  @throws Exception     - If wrong node or other error.
     */
    public function loadPubmedXml($pma) {

        $pmaTag = $pma->getname();
        if ($pmaTag != 'PubmedArticle') {
            $errMsg = "loadPubmedXml: Wrong node passed " .
                      "Expected \"PubmedArticle\". Got: \"$pmaTag\"\n";
            // XXX Log them, then throw exception
            throw new \Exception($errMsg);
        }
        // Find the top of the record
        $mc      = $pma->MedlineCitation;
        $article = $mc->Article;

        // There are other Pubmed data types besides "Article" but, for
        //   now at least, only Article is supported.
        if (!$article) {
            $msg = 'EbmsArticle.loadPubmedXml: ' .
                   'Record does not appear to be a Pubmed journal article.';
            $this->addError(msg, WATCHDOG_NOTICE, false);

            // There's no point going on with this article
            return;
        }

        // XXX - Is there a better way to convert SimpleXMLElement to string?
        // Note: some of the fields must be plain ascii for indexing
        $journal              = $mc->Article->Journal;
        $this->source         = 'Pubmed';
        $this->source_id      = (string) $mc->PMID;
        $this->source_jrnl_id = (string) $mc->MedlineJournalInfo->NlmUniqueID;
        $this->source_status  = (string) $mc['Status'];
        $this->article_title  = Util::indexify((string) $article->ArticleTitle,
                                    true, true, MAX_ART_TITLE);
        $this->jrnl_title     = Util::indexify((string) $journal->Title,
                                    true, true, MAX_JRNL_TITLE);
        $this->brf_jrnl_title = (string) $mc->MedlineJournalInfo->MedlineTA;
        $this->abstract       = (string) $this->loadAbstract(
                                            $article->Abstract[0]);
        $this->authors        = $this->loadAuthors($article->AuthorList[0]);

        // Published date and brief citation set here
        $this->loadCite($article);

        // import_date, imported_by, update_date, and article_id are
        //  all populated in the store() method
    }

    /**
     * Load object with contents of a Pubmed format XML string.
     *
     *  @param string $xmlStr - Serial XML in UTF-8 for element "PubmedArticle".
     *
     *  @return void.
     *
     *  @throws Exception     - If parser or xml error.
     */
    public function loadPubmedXmlStr($xmlStr) {

        // Manage errors ourselves
        libxml_use_internal_errors(true);

        // Clean slate
        $this->clear();

        // Parse xml
        $topElem = simplexml_load_string($xmlStr);
        if (!$topElem) {
            $errMsg="loadPubmedXmlStr: Failure parsing xml string:\n";
            $errors = libxml_get_errors();
            foreach ($errors as $error) {
                $errMsg .= "\nLine: $error->line";
                $errMsg .= "\n Col: $error->column";
                $errMsg .= "\n Msg: $error->message";
                $this->addError("\n$errMsg");
            }
            // Add them to the list and log it
            $this->addError($errMsg, WATCHDOG_ERROR, true);

        }

        // Extract all of the elements
        $this->loadPubmedXml($topElem);

        // Save the string
        $this->xml = $xmlStr;
    }


    /**
     * Construct a single line brief citation, like used in old Medline.
     *
     * Uses the published date - so updates that as well.
     *
     * See: http://www.nlm.noh.gov/bsd/licensee/journal_source.html
     * MedlineTA, PubDate;Volume(Issue):Pagination, ELocationID
     *
     *  @param object $article - PubmedArticle/MedlineCitation/Article
     *                           for one article.
     *
     *  @return void           - Updates fields directly.
     *
     * TODO: Users probably want something different.  Check with Bob.
     */
    private function loadCite($article) {
        $journal     = $article->Journal[0];
        $jrnlIssue   = $journal->JournalIssue[0];
        $titleAbbrev = $journal->Title;

        // Publication date can be MedlineDate or Year/Month/Day
        $pubDate     = $jrnlIssue->PubDate[0];
        $medlineDate = $pubDate->MedlineDate;
        if (!$medlineDate) {
            $date = $pubDate->Year;
            if ($pubDate->Month) {
                $date = $date . " " . $pubDate->Month;
                if ($pubDate->Day) {
                    $date = $date . " " . $pubDate->Day;
                }
            }
            else if ($pubDate->Season) {
                $date = $date. " " . $pubDate->Season;
            }
        }
        else
            $date = $medlineDate;

        // Volume, issue, pages
        $volume      = $jrnlIssue->Volume;
        $issue       = $jrnlIssue->Issue;
        if ($issue) {
            $issue = '(' . $issue . ')';
        }
        $pages = $article->Pagination->MedlinePGN;
        if ($pages) {
            $pages = ';' . $pages;
        }

        // Put it all together.  Some fields may be null
        $cite = $titleAbbrev . ', ' . $date . ';' . $volume . $issue . $pages;

        // Save what we've got
        $this->published_date = $date;
        $this->brf_citation   = $cite;
    }

    /**
     * Load the authors array from a Pubmed XML record.
     *
     *  @param object $authorList - AuthorList Pubmed XML element node.
     *
     *  @return array of Authors  - Array of author objects.
     */
    protected function loadAuthors($authorList) {

        // A very few articles have no authors in the record
        if (is_null($authorList))
            return null;

        // Array of arrays
        $arrayList = array();

        foreach ($authorList->Author as $author) {
            // authorId is null unless/until record is/was stored
            $oneAuth = new EbmsArticleAuthor(null, $author->CollectiveName,
               $author->LastName, $author->ForeName, $author->Initials);

            // If no error, append this author to the list of authors
            // Note: order is significant, first author, second, etc.
            if ($oneAuth->msg)
                $this->addError($oneAuth->msg, WATCHDOG_WARNING);
            else
                $arrayList[] = $oneAuth;
        }

        return $arrayList;
    }

    /**
     * Construct an abstract from AbstractText elements.
     *
     * Makes Label attributes explicit.
     *
     *  @param object $abstract - PubmedArticle/MedlineCitation/Article
     *                            for one article.
     *
     *  @return string|null     - Abstract as a single string.
     *                            Null if no abstract present.
     */
    public function loadAbstract($abstract) {
        if (!$abstract)
            return null;

        $abs = '';
        foreach ($abstract->AbstractText as $text) {
            // If we have some text already, put spacing after it
            if ($abs)
                $abs .= ' ';

            // Prepend any label
            $attrs = $text->attributes();
            if ($attrs['LABEL'])
                $abs = $attrs['LABEL'] . ': ';

            // And copy the text
            $abs .= $text;
        }

        return $abs;
    }

    /**
     * Create a text string with the contents of the object for debugging.
     */
    public function __toString() {
        // For debugging in text or in browser
        // $term = "\n";
        $term = "<br />\n";

        $text  = "$term------------------------------------------------$term";
        $text .= "    article_id: " . $this->article_id . $term;
        $text .= "        source: " . $this->source . $term;
        $text .= "     source_id: " . $this->source_id . $term;
        $text .= "source_jrnl_id: " . $this->source_jrnl_id . $term;
        $text .= " source_status: " . $this->source_status . $term;
        $text .= " article_title: " . $this->article_title . $term;
        $text .= "    jrnl_title: " . $this->jrnl_title . $term;
        $text .= "brf_jrnl_title: " . $this->brf_jrnl_title . $term;
        $text .= "   import_date: " . $this->import_date . $term;
        $text .= "   imported_by: " . $this->imported_by . $term;
        $text .= "   update_date: " . $this->update_date . $term;
        $text .= "      abstract: " . $this->abstract . $term;
        $text .= "published_date: " . $this->published_date . $term;
        $text .= "  brf_citation: " . $this->brf_citation . $term;
        $text .= "       Authors:$term";

        // Authors
        if ($this->authors) {
            foreach ($this->authors as $auth) {
                $text .= "          " . $auth->author_id . ': ';
                if ($collectName)
                    $text .= $collectName;
                else
                    $text .= $auth->last_name . ', ' .
                    $auth->forename . " (" . $auth->initials . ")";
                $text .= $term;
            }
        }
        else
            $text .= "          No authors$term";

        // Just indicate whether this is present or not
        if ($this->source_data)
            $text .= "   source_data: is populated" . $term;
        else
            $text .= "   source_data: is NULL" . $term;

        // Show the URL or filename for the PDF is present
        $text .= "  Full file ID: " . $this->full_text_id . $term;

        if (is_null($this->errorMsgs))
            $text .= "     errorMsgs: None" . $term;
        else {
            $text .= "     errorMsgs:" . $term;
            foreach ($this->errorMsgs as $msg) {
                $text .= "       $msg" . $term;
            }
        }

        $text .= "$term------------------------------------------------$term";

        return $text;
    }

    /**
     * Display a node and (optionally) its children
     * For debugging.
     */
    public function showNode($node, $recurse=False) {
        $dbg = $node->getname() . ":\n";
        if ($node->count() > 0) {
            $dbg .= "has " . $node->count() . "children\n";
            if ($recurse) {
                foreach ($node->children() as $child)
                    showNode($child);
            }
        }
        else
            // No children, show the text
            $dbg .= $node . "\n";
        return $dbg;
    }

    /**
     * Store new or updated article in the database.
     *
     * The determination of whether to store it as new or as an update
     * depends on whether $this->article_id has a legal value.  If so,
     * it's an update.  Else it's a new insert.
     *
     * Stores bibliographic data, source xml, authors.
     * Caller must separately make the association with a Summary Topic.
     *
     *  @param string $storeDate - Date to assign to all dates,
     *                             'YYYY-MM-DD HH:MM:SS'.
     *                Else use now().  Specifying the same date
     *                for a batch avoids having a different datetime on
     *                each record in the batch, though that may be better
     *                than allowing random dates.
     *
     *  @param int $importUserId - Person importing a new record.
     *                Maybe never used, but it supports EbmsImport
     *                functionality.
     *
     *  @return void.
     *
     *  @throws Exception on database error.
     */
    public function store($storeDate=null, $importUserId=null) {

        // TODO: Check authorization

        // Get a uniform date if needed
        if (is_null($storeDate))
            $storeDate = db_query('SELECT NOW()')->fetchField();

        // Is this a new record or an update?
        $newRec = true;
        if ($this->article_id)
            $newRec = false;

        // Here are the fields we'll store
        //   article_id and dates handled separately for new
        //   and replace records
        $fields = array(
            'source'         => $this->source,
            'source_id'      => $this->source_id,
            'source_jrnl_id' => $this->source_jrnl_id,
            'source_status'  => $this->source_status,
            'article_title'  => $this->article_title,
            'jrnl_title'     => $this->jrnl_title,
            'brf_jrnl_title' => $this->brf_jrnl_title,
            'abstract'       => $this->abstract,
            'published_date' => $this->published_date,
            'brf_citation'   => $this->brf_citation,
            'source_data'    => $this->source_data,
            'full_text_id'   => $this->full_text_id
        );

        // Wrap this in a transaction
        $txn = db_transaction();
        try {

            // If we're inserting a new record with store
            if ($newRec) {

                // Identify the importing user
                if (!$importUserId)
                    $importUserId = Util::getUserId($importUserId);

                // Insert a new record
                $query = db_insert('ebms_article');
                $fields['import_date'] = $storeDate;
                $fields['imported_by'] = $importUserId;
                $fields['update_date'] = null;
                $query->fields($fields);
                $article_id = $query->execute();

                // Make id available
                $this->article_id = $article_id;
            }
            else {
                // Update existing record with new update_date
                $query = db_update('ebms_article');
                $fields['update_date'] = $storeDate;
                $query->fields($fields);
                $query->condition('article_id', $this->article_id);
                $query->execute();

                // Delete any author associations.  We'll replace them
                $this->unlinkAuthors();
            }

            // Store the authors in the record and associate them with article
            $this->storeAuthors();
        } catch (\Exception $e) {
            $txn->rollback();
            throw $e;
        }

        // $txn goes out of scope here and transaction commits
    }


    /**
     * Store authors in the ebms_article_author and ebms_article_author_cite
     * tables.  Updates in memory object to include author IDs.
     *
     * Authorization checking is not required since the only way to get to
     * this function is through an authorized function.
     *
     * Helper function for store().
     *
     * Higher level store() performs transaction management.
     */
    private function storeAuthors() {

        // Is there anything to store?
        if (!$this->authors)
            return;

        // Author order in the article is important
        $citeOrder = 1;

        // Process each author in the array
        foreach ($this->authors as $author) {

            // If we don't have an author ID from the database
            if (!$author->author_id) {

                // Search for exact match on precisely what we have
                // Inexact can't be used.  No way to know if they
                //   are the same person or not.
                // Generic searches will still find the author (and noise too)
                $query = db_select('ebms_article_author', 'a');
                $query->fields('a', array('author_id'));

                // Search for last and collective name - both, just in case
                $query->condition('a.last_name', $author->last_name);
                $query->condition('a.collective_name',
                                   $author->collective_name);
                if ($author->forename)
                    $query->condition('a.forename', $author->forename);
                else
                    $query->isNull('a.forename');
                if ($author->initials)
                    $query->condition('a.initials', $author->initials);
                else
                    $query->isNull('a.initials');
                $result = $query->execute();
                if (count($result) > 0) {
                    // This author is known already
                    $author->author_id = $result->fetchField();
                }
                if (!$author->author_id) {

                    // Still no author_id. Create an author record
                    $query = db_insert('ebms_article_author')
                     ->fields(array(
                        'last_name'       => $author->last_name,
                        'forename'        => $author->forename,
                        'initials'        => $author->initials,
                        'collective_name' => $author->collective_name));

                    // Execute query, getting back the last insert ID
                    $author->author_id = $query->execute();

                    // Test for success
                    if (!$author->author_id)
                        $this->addError(
                                "EbmsArticle.storeAuthors: " .
                                "Could not create author record for " .
                                $author->collective_name . ':' .
                                $author->last_name . ', ' .
                                $author->fore_name . ' ' .
                                $author->initials,
                            WATCHDOG_ERROR);
                }
            }

            // Associate the author with the article
            $query = db_insert('ebms_article_author_cite')
             ->fields(array(
                'article_id' => $this->article_id,
                'author_id'  => $author->author_id,
                'cite_order' => $citeOrder));

            $query->execute();

            // Next author in citation order
            $citeOrder++;
        }
    }

    /**
     * Delete authors from the ebms_article_author_cite table and, if
     * there are no more cites, from the ebms_article_author table.
     *
     * Helper function for replace().
     *
     * Optimization note:
     *  We can replace faster if we compare new and old authors and only
     *  drop those that are in the old but not new, only add those that
     *  are in the new and not old.
     *  In many (most?) cases the author lists will be identical.  But
     *  optimization may be unimportant here.
     */
    private function unlinkAuthors() {

        // UNTESTED
        // First delete all associations between authors and article
        $delQry = db_delete('ebms_article_author_cite')
            ->condition('article_id', $this->article_id)
            ->execute();

        // TODO: Delete article_author if no cites left for him.
        //       Don't know if this is needed or not.
        // Alternative approaches:
        //   Delete author where id not in select author_id from cites
        //     This took 10 seconds in my test on verdi!
        //     Could do it once a day or week?
        //   Get author list for article and delete each one if no cites
        //     Probably faster for most articles.
        //     Not comprehensive as the other method is

        // XXX - ERROR CHECK?
    }

    /**
     * Get all topics associated with an article.
     *
     *  @return array of EbmsArticleTopic objects.
     *
     *  @throws Exception if database error.
     */
    public function getTopics() {

        return getTopics($this->article_id);
    }


    /**
     * Simple routine to just get topic IDs
     */

    /**
     * Associate an article with a summary topic.
     *
     *  @param int $topicId     - Unique ID of the topic
     *  @param int $userId      - Person who did it
     *  @param int $cycleId     - EBMS monthly processing cycle for association
     *  @param datetime $addDt  - Datetime of association.
     *                            When multiple articles processed in a batch
     *                            we use the same datetime for all.
     *  @param string $method   - 'P' = Program, 'H' = Human
     *
     *  @return bool            - True  = Success.
     *                            False = Article already has topic, no add
     *                              is allowed.
     *
     *  @throws Exception if key constraint or other DBMS error.
     */
    /* XXX NEEDS CHANGE, NOW DONE BY SETTING A STATE
    public function addTopic($topicId, $userId, $cycleId,
                             $addDt=null, $method='P') {

        // Get a uniform date if needed
        if (is_null($addDt))
            $addDt = db_query('SELECT NOW()')->fetchField();

        // Check if topic already present
        // Must always return something, may be 0
        $result = db_query(
            "SELECT COUNT(*)
               FROM {ebms_article_topic}
              WHERE article_id = :artId
                AND topic_id   = :topId",
            array(':artId' => $this->article_id,
                  ':topId' => $topicId));
        $count = $result->fetchField();

        if ($count > 0)
            return False;

        // Add the row
        $query = db_insert('ebms_article_topic');
        $query->fields(array(
                'article_id'       => $this->article_id,
                'topic_id'         => $topicId,
                'cycle_id'         => $cycleId,
                'user_id'          => $userId,
                'article_topic_dt' => $addDt,
                'method'           => $method));
        $query->execute();

        return True;
    }
    */
}

/**
 * Add a row to the state table for an article.
 *
 * This is implemented as a free standing function rather than an
 * EbmsArticle class member because it is likely to be needed when
 * no EbmsArticle object is otherwise required.
 *
 *  @param int $articleId    - Article whose state has changed.
 *  @param string $textId    - Name used in preference to ID.  The code is
 *                              less efficient but caller's code is more
 *                              self-documenting.
 *  @param int $topicId      - Topic identifier, now always required.
 *  @param int $boardId      - Board identifier, must match topic.
 *                              XXX - We can dispense with this now since
 *                                    board is now always set to the board
 *                                    associated with the topic.
 *  @param int $comment      - Optional user or program supplied comment.
 *  @param int $userId       - User setting the state.  If null use current.
 *  @param int $dateTime     - Date time of action.  If null, use now.
 *
 *  @return - int article_state_id.  This is the id of the new row
 *            in the ebms_article_state table, not the type table.
 *
 *  @throws Exception if invalid parameters  or lower level error.
 */
function setArticleState($articleId, $textId, $topicId,
        $boardId=null, $comment=null, $userId=null, $dateTime=null) {

    // Get everything we know about this state
    $stateType = db_select('ebms_article_state_type', 't')
        ->fields('t')
        ->condition('state_text_id', $textId)
        ->execute()
        ->fetchObject();

    // Must be one
    if (!$stateType)
        throw new \Exception(
                "setArticleState: Unknown state type text ID $textId");
    if (!$topicId)
        throw new \Exception(
                "setArticleState: Missing required topic ID");

    // Get defaul user from global if not passed (or 0)
    if (!$userId)
        $userId = Util::getUserId($userId);

    // Check topic/board consistency
    $topicBoardId = getTopicBoard($topicId);

    if ($boardId && $boardId != $topicBoardId)
        throw new \Exception("setArticleState: boardId=$boardId does not" .
                            " match topicId=$topicId");

    // Derive board from topic if needed
    if (!$boardId)
        $boardId = $topicBoardId;

    // Get datetime from database if needed
    if (is_null($dateTime))
        $dateTime = db_query('SELECT NOW()')->fetchField();

    // Prepare to insert the new row
    $insFields = array(
        'article_id'   => $articleId,
        'state_id'     => $stateType->state_id,
        'user_id'      => $userId,
        'status_dt'    => $dateTime,
        'active_status'=> 'A',
        'board_id'     => $boardId,
        'topic_id'     => $topicId,
        'current'      => 'Y'
    );
    $insQry = db_insert('ebms_article_state')
        ->fields($insFields);

    // Prepare to inactivate any rows with >= sequence number
    $checkInactivation = False;

    // Start a transaction
    $txn = db_transaction();

    // Create try block to catch any exceptions while in transaction
    $rowId = Null;
    try {
        // Find equal or higher sequenced active states needing inactivation
        if ($stateType->sequence) {
            $seqQry = db_select('ebms_article_state', 'state');
            $seqQry->join('ebms_article_state_type', 'stype',
                     'state.state_id = stype.state_id');
            $seqQry->fields('state', array('article_state_id'));
            $seqQry->condition('state.article_id', $articleId)
                   ->condition('stype.sequence', $stateType->sequence, '>=')
                   ->condition('state.active_status', 'A');

            // Add clauses for topic and board if needed
            if ($topicId)
                $seqQry->condition('topic_id', $topicId);
            if ($boardId)
                $seqQry->condition('board_id', $boardId);

            $result = $seqQry->execute();

            // Get all article_state_id for all rows
            $stateRowIds = $result->fetchCol(0);

            // If there were any equal or higher sequenced states in the table
            if (count($stateRowIds) > 0) {

                // Setup to inactivate all of the rows
                $inactiveUpd = db_update('ebms_article_state')
                  ->fields(array('active_status' => 'I'))
                  ->condition('article_state_id', $stateRowIds, 'IN');

                // Same clauses regarding topic and board
                if ($topicId)
                    $inactiveUpd->condition('topic_id', $topicId);
                if ($boardId)
                    $inactiveUpd->condition('board_id', $boardId);

                $inactiveUpd->execute();

                // Add a comment to each inactive row
                $inactiveComment =
                  "State inactivated by setting \"$textId\" at $dateTime";
                foreach ($stateRowIds as $updId)
                    addArticleStateComment($updId, $inactiveComment,
                                           $userId, $dateTime);
            }
        }

        // Remove the "current" flag from all existing rows for this
        //  article, this topic.
        // Note: This doesn't work if we allow states without topics,
        //       but a decision was made to implement this quickly using
        //       current assumptions.  We'll have to change it if and
        //       when the assumptions no longer obtain.
        $newVal = array();
        $newVal['current'] = 'N';
        $curQry = db_update('ebms_article_state');
        $curQry->fields($newVal);
        $curQry->condition('article_id', $articleId);
        $curQry->condition('topic_id', $topicId);
        $curQry->condition('current', 'Y');
        $curQry->execute();

        // Now insert row
        $rowId = $insQry->execute();

        // If there was a comment, insert it too
        if (!is_null($comment))
            addArticleStateComment($rowId, $comment, $userId, $dateTime);

    } catch (\Exception $e) {
        // Something didn't work, cancel all updates and re-throw
        $txn->rollback();
        throw $e;
    }

    // Success, transaction will commit when $txn goes out of scope
    return $rowId;
}


/**
 * Add a comment to a row in the state table.
 *
 *  @param int $articleStateId  Article state row ID to comment upon.
 *  @param string $comment      Comment text.
 *  @param int $userId          User making the comment, current user if null.
 *  @param int $commentDt       Datetime, current datetime if null.
 *
 *  @return int                 New auto_increment ID for comment row.
 *
 *  @throws Exception           If database or other serious error.
 */
function addArticleStateComment($articleStateId, $comment,
                                $userId=null, $commentDt=null) {

    // Resolve defaulted args
    if (!$userId)
        $userId = Util::getUserId($userId);
    if (is_null($commentDt))
        $commentDt = db_query('SELECT NOW()')->fetchField();

    $commentIns = db_insert('ebms_article_state_comment')
      ->fields(array(
        'article_state_id' => $articleStateId,
        'user_id'          => $userId,
        'comment_dt'       => $commentDt,
        'comment'          => $comment));

    $commentRowId = $commentIns->execute();

    return $commentRowId;
}


/**
 * Get the current state of an article where current is the one with
 * the highest active sequence number.
 *
 * WARNING:
 *  If passed topic ID is null and article has identical highest
 *  states in two different topics, this function returns the most
 *  recent state assigned, irrespective of topic.  Ditto for two
 *  boards.  In other words, the choice among the two topics or two
 *  boards is determinate, but not necessarily useful.
 *
 *  Is that really what you want?  If not, suggest a better approach, or
 *  always pass a topic ID.
 *
 *  @param int $article_id  Of article for which to get the current state.
 *  @param int $topic_id    If not null, only consider this topic.
 *  @param int $board_id    If not null, only consider this board.
 *                           Board ID is ignored if topic ID passed.  It
 *                           is implied by the topic ID.
 *
 *  @return array           Associative array of all info known about highest
 *                           sequenced state.
 *                           Returns null if no active entry in the state
 *                           table for this article.
 *
 *  @throws Exception       If bad parameters or database error.
 *
 * XXX - ISSUE
 *  What does the caller want when board and/or topic are null?
 *   1. Last state matching criteria?
 *   2. All last states matching criteria, e.g., 2 boards + 3 topics
 *
 * XXX - ISSUE
 *  Since this function was written we added a "current" column to the
 *  ebms_article_state table.  It greatly simplifies searching for the
 *  current article state.
 *
 *  However, due to issues regarding dirty data, we decided not to set
 *  the column value to 'Y' for any legacy data.  The function below
 *  should work for legacy data too (except where data is bad) and has
 *  therefore been retained.  A simpler, faster version could be written
 *  if we ignore legacy data and use the new column.
 */
function getCurrentState($article_id, $topic_id=null, $board_id=null) {

    $qry = db_select('ebms_article_state', 'state');
    $qry->join('ebms_article_state_type', 'type',
               'state.state_id = type.state_id');
    $qry->join('users', 'user', 'state.user_id = user.uid');
    $qry->leftJoin('ebms_board', 'board', 'state.board_id = board.board_id');
    $qry->leftJoin('ebms_topic', 'topic', 'state.topic_id = topic.topic_id');
    $qry->fields('state', array('article_state_id', 'state_id', 'user_id',
                                'board_id', 'topic_id', 'status_dt'));
    $qry->fields('type', array('state_name'));
    $qry->fields('user', array('name'));
    $qry->fields('board', array('board_name'));
    $qry->fields('topic', array('topic_name'));
    $qry->condition('state.article_id', $article_id)
        ->condition('state.active_status', 'A');

    // Limit by board or topic
    if ($topic_id)
        $qry->condition('state.topic_id', $topic_id);
    if ($board_id)
        $qry->condition('state.board_id', $board_id);

    // Get the highest sequence numbered state that qualifies
    // XXX status_dt should not be needed, but data was dirty in old DB
    $qry->orderBy('type.sequence', 'DESC');
    $qry->orderBy('state.status_dt', 'DESC');
    $qry->range(0, 1);

    /* XXX Here is what I am trying to achieve - Remove after debugging
 SELECT state.article_state_id, state.state_id, type.state_name,
        state.status_dt, state.user_id, type.completed
   FROM ebms_article_state state
   JOIN ebms_article_state_type type
     ON state.state_id = type.state_id
  WHERE state.article_id = ?
    AND state.topic_id = ?
    AND state.board_id = ?
    AND state.active_status = 'A'
  ORDER BY type.sequence DESC
  LIMIT 1
    */

    // Get it
    $result = $qry->execute()->fetchAssoc();

    // If we got any
    if ($result)
        return $result;

    // No hits - happens if wrong article, board or topic IDs passed
    return null;
}


/**
 * Container for status or tag comments.  Same class for both.
 */
class ArticleComment {
    public
        $comment_id,    // Row ID in status or tag comment table
        $references_id, // Row ID in ebms_article_state or _tag table
        $user_id,       // User creating the comment
        $user_name,     // His/her name
        $dt,            // Date comment recorded
        $text;          // Text of the comment

    /**
     * Produce HTML for one row of a table for this ArticleComment object.
     *
     *  @param string $class    Class attribute to add to the row.
     *                           See ArticleStatus->toHtml().
     *  @param int $cols        Number of columns in the table, used to
     *                           produce reasonable looking tables that
     *                           interleave status or tags + comments.
     *
     *  @return                 One <tr>...</tr> string.
     */
    public function toHtml($cols=1, $class='activeStat') {

        // Comments will spread across the table
        $spanAttr = ($cols < 1) ? '' : " colspan='$cols'";

        // Create compact display
        $html = <<<EOS
<tr class='$class'>
 <td$spanAttr>Comment: $this->comment_id State: $this->references_id
Date: $this->dt User: $this->user_name<br />$this->text</td>
</tr>
EOS;
        return $html;
 //<td>$this->comment_id / $this->references_id<br />$this->dt</td>
 //<td>$this->user_name<br />$this->user_id</td>
    }
}


/**
 * Container object for one state that an article has been in.
 */
class ArticleStatus {
    public
        $article_id,       // EBMS article id
        $article_state_id, // Row ID in ebms_article_state
        $dt,               // Date time status row was created
        $type_id,          // Row ID in ebms_article_state_type
        $type_text_id,     // Text state ID in ebms_article_state_type
        $type_name,        // Name of the state type
        $type_sequence,    // Sequence number in the type table
        $user_id,          // User assigning this state
        $user_name,        // His/her name
        $board_id,         // Editorial board ID
        $board_name,       // String form board name
        $topic_id,         // Summary topic ID
        $topic_name,       // String form topic name
        $active_status,    // 'A' = Active, 'I' = Inactive
        $current,          // 'Y' = This is the latest state, else 'N'
                           //  All legacy data is marked as 'N'
        $comments;         // Array of 0 or more EbmsComents

    /**
     * Produce HTML for one row of a table for this ArticleStatus object.
     *
     *  @return             One <tr>...</tr> string.
     */
    public function toHtml() {

        // Distinguish current, active, inactive rows
        if ($this->current == 'Y')
            $active = 'currentStat';
        else if ($this->active_status == 'A')
            $active = 'activeStat';
        else
            $active = 'inactiveStat';

        // Construct the row
        $html = <<<EOS
<tr class='$active'>
 <td>$this->dt<br />$this->article_id / $this->article_state_id</td>
 <td>$this->type_name / $this->type_text_id<br />$this->type_id / $this->type_sequence</td>
 <td>$this->user_name<br />$this->user_id</td>
 <td>$this->board_name<br />$this->board_id</td>
 <td>$this->topic_name<br />$this->topic_id</td>
</tr>
EOS;

        // If there are comments, add more rows
        if (count($this->comments) > 0) {
            foreach ($this->comments as $cmt)
                $html .= $cmt->toHtml(5, $active);
        }

        return $html;
    }
}


/**
 * Get all status history for an article.
 *
 *  @param int $articleId       Unique article ID
 *  @param int $boardId         If present, get status info that has no board
 *                               (i.e. all boards) or the specified board ID.
 *                               If null, and no topic specified, get all
 *                               status for all boards.
 *                               Default null.
 *  @param int $topicId         If present, get status info that has the
 *                               specified topic, or the specified or implied
 *                               board with no topic (i.e., all topics for
 *                               that board), or that has no board
 *                               information at all (i.e., all boards).
 *                               Default null.
 *  @param bool $incComments       True = include comments.
 *                               False = no comments.
 *                               Default = true.
 *  @param bool $inactive       True = get all history, including inactive.
 *                               False = only active status.
 *                               Default = false.
 *  @param string $order        Sort order for states, a string containing
 *                               a single letter code for each sort key:
 *                               'D' = Date
 *                               'S' = Sequence number in type table
 *                               'B' = Board name
 *                               'T' = Summary topic name
 *                               'U' = User name (firstname lastname)
 *                               'I' = Article state ID (state creation order)
 *                              Default = "DSBT".
 *
 *  @return array of ArticleStatus objects with all info.
 *
 *  @throws Exception if invalid board/topic combo, or other error.
 */
function getStatusHistory($articleId, $boardId=null, $topicId=null,
                          $incComments=true, $inactive=false, $order='DBST') {

    // Check board/topic congruence
    if ($topicId) {
        $topicBoardId = getTopicBoard($topicId);
        if (is_null($boardId))
            $boardId = $topicBoardId;
        else {
            if ($topicBoardId != $boardId)
                throw new \Exception(
           "getStatusHistory: mismatched boardId=$boardId, topicId=$topicId");
        }
    }

    // Join five tables
    $qry = db_select('ebms_article_state', 'state');
    $qry->join('ebms_article_state_type', 'stype',
               'state.state_id = stype.state_id');
    $qry->leftjoin('users', 'usr',
               'state.user_id = usr.uid');
    $qry->leftjoin('ebms_board', 'board',
               'state.board_id = board.board_id');
    $qry->leftjoin('ebms_topic', 'topic',
               'state.topic_id = topic.topic_id');
    $qry->fields('state', array('article_state_id', 'state_id', 'user_id',
             'status_dt', 'board_id', 'topic_id', 'active_status', 'current'))
        ->fields('stype', array('state_name', 'state_text_id', 'sequence'))
        ->fields('usr',   array('name'))
        ->fields('board', array('board_name'))
        ->fields('topic', array('topic_name'));

    $qry->condition('state.article_id', $articleId);
    if ($topicId) {
        // If there's a topic, we want anything with this topic, this board
        //  but no topic, or no board.
        // WHERE state.topic_id = $topicId
        //    OR (state.board_id = $boardId AND state.topic_id IS NULL)
        //    OR (state.board_id IS NULL)
        $qry->where(
             "    state.topic_id = $topicId
              OR (state.board_id = $boardId AND state.topic_id IS NULL
              OR (state.board_id IS NULL))");
    }
    else if (!is_null($boardId)) {
        // With no topic, get everything for this board, or that has
        //  no board specificity
        $qry->condition(
            db_or()->condition('state.board_id', $boardId)
                   ->isNull('state.board_id'));
    }
    // else No topic or board specified, just get everything

    // Eliminate inactive rows unless they are requested
    if (!$inactive)
        $qry->condition('state.active_status', 'A');

    // Construct desired sort/retrieval order
    for ($i=0; $i<strlen($order); $i++) {
        switch ($order[$i]) {
            case 'D': $fld = 'state.status_dt';        break;
            case 'S': $fld = 'stype.sequence';         break;
            case 'B': $fld = 'board.board_name';       break;
            case 'T': $fld = 'topic.topic_name';       break;
            case 'U': $fld = 'usr.name';               break;
            case 'I': $fld = 'state.article_state_id'; break;
            default:
                throw new \Exception(
                    "getStatusHistory: Invalid sort order token '$order[$i]'");
        }
        $qry->orderBy($fld);
    }

    // Get everything into an associative array keyed by date
    $stateData = $qry->execute()->fetchAllAssoc('article_state_id');

    // Build an array of well known objects
    $states = array();
    foreach ($stateData as $sd) {
        $st = new ArticleStatus();
        $st->article_id       = $articleId;
        $st->article_state_id = $sd->article_state_id;
        $st->dt               = $sd->status_dt;
        $st->type_id          = $sd->state_id;
        $st->type_text_id     = $sd->state_text_id;
        $st->type_name        = $sd->state_name;
        $st->type_sequence    = $sd->sequence;
        $st->user_id          = $sd->user_id;
        $st->user_name        = $sd->name;
        $st->board_id         = $sd->board_id;
        $st->board_name       = $sd->board_name;
        $st->topic_id         = $sd->topic_id;
        $st->topic_name       = $sd->topic_name;
        $st->active_status    = $sd->active_status;
        $st->current          = $sd->current;
        $st->comments         = array();

        // Save it, indexed by state row ID
        $states[$st->article_state_id] = $st;
    }

    // If comments are required, have to fetch them and add them in
    if (count($states) > 0 && $incComments) {

        // Construct string of article state IDs for IN statement
        $stateIdList = implode(',', array_keys($states));

        // Locate all of the comments for those
        $qry = db_select('ebms_article_state_comment', 'comment');
        $qry->leftjoin('users', 'usr', 'comment.user_id = usr.uid');
        $qry->fields('comment', array('article_state_id', 'comment_id',
                                      'user_id', 'comment_dt', 'comment'))
            ->fields('usr', array('uid', 'name'))
            ->where("comment.article_state_id IN ($stateIdList)")
            ->orderBy('comment.article_state_id', 'DESC')
            ->orderBy('comment.comment_dt', 'ASC');

        $comments = $qry->execute()->fetchAllAssoc('comment_id');

        // Construct a comment object and interpolate into the state data
        foreach ($comments as $cm) {
            // Construct fields
            $cmt = new ArticleComment();
            $cmt->comment_id    = $cm->comment_id;
            $cmt->references_id = $cm->article_state_id;
            $cmt->user_id       = $cm->user_id;
            $cmt->user_name     = $cm->name;
            $cmt->dt            = $cm->comment_dt;
            $cmt->text          = $cm->comment;

            // Append it to the correct status record comment array
            $states[$cm->article_state_id]->comments[] = $cmt;
        }
    }

    // Return the whole shebang
    // logw("The whole shebang follows");
    // logw(print_r($states, true));
    return $states;
}

/**
 * Convert the results of getCurrentState() to HTML for test/debug.
 *
 *  @param array $curStatus Return from getCurrentState();
 *
 *  @return string          HTML.
 */
function currentStatusToHtml($curStatus) {

    // Simple, dumb table for debugging
    $html = '
<table border="1">
 <tr><th>Field</th><th>Value</th></tr>
';
    if (is_array($curStatus)) {
        foreach ($curStatus as $key => $value)
            $html .= " <tr><td>$key</td><td>$value</td></tr>";
        $html .= " </tr>\n</table>\n";
        return $html;
    }
    return 'No status found for this article/board/topic combination';
}

/**
 * Convert the results of getStatusHistory() to HTML for test/debug.
 *
 *  @param array $history   Return from getStatusHistory();
 *
 *  @return string          HTML.
 */
function statusHistoryToHtml($history) {

    // Convert the array of objects to an array of strings
    $histHtmlArray = array();
    foreach ($history as $st)
        $histHtmlArray[] = $st->toHtml();

    // Convert the array to a single string
    $html = <<<EOS
<style type='text/css'>
#table { border: 5 px; }
 .inactiveStat { color: grey; }
 .activeStat   { color: blue; }
 .currentStat  { color: red; }
#testresults {
 width: 80%;
 margin-left: 10%;
 margin-right: 10%;
}
</style>

<div id='testresults'>
<h3>Status History</h3>
<p class="activeStat">Active records shown in this color.<br />
<span class="inactiveStat">Inactivated records are in this color.</span><br />
<span class="currentStat">The "current" state is in this color.</span></p>
<table class='debug' border='1'>
 <tr>
  <th>DateTime<br />ArtID/StateID</th>
  <th>TypeName<br />TypeID / TypeSeq</th>
  <th>UserName<br />UserID</th>
  <th>BoardName<br />BoardID</th>
  <th>TopicName<br />TopicID</th>
 </tr>
EOS;
    $html .= implode('', $histHtmlArray) . "</table>\n</div>\n";

    return $html;
}


/**
 * Container object for a descriptive tag attached to an article.
 */
class EbmsArticleTag {
    public
        $article_id,       // EBMS article id
        $article_tag_id,   // Row ID in the ebms_article_tag table
        $dt,               // Date time status row was created
        $type_id,          // Row ID in ebms_article_tag_type
        $type_text_id,     // Internal human readable name of tag type
        $type_name,        // External human readable name of tag type
        $user_id,          // User assigning this tag
        $user_name,        // His/her name
        $topic_id,         // Summary topic ID
        $topic_name,       // String form topic name
        $active_status,    // 'A' = Active, 'I' = Inactive
        $comments;         // Array of 0 or more EbmsComents

    /**
     * Produce HTML for one row of a table for this ArticleStatus object.
     *
     *  @return             One <tr>...</tr> string.
     */
    public function toHtml() {

        // Distinguish active from inactive rows
        $active = ($this->active_status == 'A') ? 'activeStat' : 'inactiveStat';

        // Construct the row
        $html = <<<EOS
<tr class='$active'>
 <td>$this->dt<br />$this->article_id / $this->article_tag_id</td>
 <td class='tagname'>$this->type_name<br />$this->type_text_id / $this->type_id</td>
 <td>$this->user_name<br />$this->user_id</td>
 <td>$this->topic_name<br />$this->topic_id</td>
</tr>
EOS;

        // If there are comments, add more rows
        if (count($this->comments) > 0) {
            foreach ($this->comments as $cmt)
                $html .= $cmt->toHtml(5, $active);
        }

        return $html;
    }
}


/**
 * Add a descriptive tag to an article.
 *
 *  @param string $tagIdStr Unique name of the tag to add.
 *  @param int $articleId   Unique ID of the article.
 *  @param int $topicId     Optional unique ID of a summary topic.
 *  @param string $comment  Optional comment to associate with the tag.
 *  @param int $userId      Optional user ID, default=current user.
 *
 *  @return                 Unique row ID of the descriptive tag.
 *
 *  @throws Exception       If serious parameter error.
 */
function addArticleTag($tagIdStr, $articleId, $topicId=null,
                       $comment=null, $userId=null) {

    // Check parms
    $result = db_query(
        'SELECT *
           FROM {ebms_article_tag_type}
          WHERE text_id = :text_id',
         array(':text_id' => $tagIdStr));

    // Must be a type
    $tagType = $result->fetchObject();
    if (!$tagType)
        throw new \Exception("addArticleTag: Unknown tag type $tagIdStr");

    // If topic ID is required, is it present?
    // If topic is illegal, the database will throw an exception
    if (is_null($topicId) && $tagType->topic_required == 'Y')
        throw new \Exception("addArticleTag: '$tagIdStr' requires topic ID");

    // Get default user from global if not passed
    if (!$userId)
        $userId = Util::getUserId($userId);
    $tagDt = db_query('SELECT NOW()')->fetchField();

    // Create the tag row in the database
    $qry = db_insert('ebms_article_tag');
    $qry->fields(array('article_id' => $articleId,
                       'tag_id'     => $tagType->tag_id,
                       'topic_id'   => $topicId,
                       'user_id'    => $userId,
                       'tag_dt'     => $tagDt));
    $articleTagId = $qry->execute();

    // If there's a comment, insert it
    if ($comment)
        addTagComment($articleTagId, $comment, $userId);

    return $articleTagId;
}


/**
 * Inactivate a tag.  Marks the tag so that it won't be selected
 * be default for most displays.
 *
 *  @param int $articleTagId    Row ID for the tag to inactivate.
 *                               We're looking for the row ID of the tag
 *                               record, not the ID of the tag type.  There
 *                               could be multiple rows with the same tag
 *                               type.
 *
 *  @throws exception           if bad row id.
 */
function inactivateArticleTag($articleTagId) {

    // Check tag exists and needs inactivation
    $aStatus = db_query('SELECT active_status
                           FROM {ebms_article_tag}
                          WHERE article_tag_id = :artTagId',
                        array(':artTagId' => $articleTagId))->fetchField();
    if (!$aStatus)
        throw new \Exception(
            "inactivateArticleTag(): can't find article_tag_id=$articleTagId");

    // Do it if we need to
    if ($aStatus == 'A') {
        $qry = db_update('ebms_article_tag');
        $qry->fields(array('active_status' => 'I'));
        $qry->condition('article_tag_id', $articleTagId);
        $qry->execute();
    }
}


/**
 * Append a comment to the list of zero more comments on this tag instance.
 *
 *  @param int $articleTagId    DB row ID for the tag to comment on.
 *  #param string $comment      Text of the comment.
 *  @param int $userId          User ID of person doing it.  Null=current user.
 *
 *  @return int                 DB row ID of the new comment.
 *
 *  @throws exception           if bad row id.
 */
function addTagComment($articleTagId, $comment, $userId=null) {

    // If no user, get current user
    if (!$userId)
        $userId = Util::getUserId($userId);
    $commentDt = db_query('SELECT NOW()')->fetchField();

    // Insert it
    $qry = db_insert('ebms_article_tag_comment');
    $qry->fields(array(
        'article_tag_id' => $articleTagId,
        'user_id'        => $userId,
        'comment_dt'     => $commentDt,
        'comment'        => $comment));

    $commentId = $qry->execute();

    if (!$commentId) {
        throw new \Exception(<<<"EOS"
addTagComment(): Unable to add comment to article_tagid=$articleTagId,
user=$userId, comment='$comment'
EOS
        );
    }

    return $commentId;
}


/**
 * Retrieve the complete tag history for an article.
 *
 *  @param int $articleId       Unique article ID
 *  @param int $topicId         If present, only get tag info that has the
 *                               specified topic ID.  Else get all tag info.
 *                               Default null.
 *  @param string $tagIdStr     Text identifier of tag.  If null, get all tags.
 *                               Default null.
 *  @param bool $incComments       True = include comments.
 *                               False = no comments.
 *                               Default = true.
 *  @param bool $inactive       True = get all history, including inactive.
 *                               False = only active status.
 *                               Default = false.
 *  @param string $order        Sort order for tags, a string containing
 *                               a single letter code for each sort key:
 *                               'D' = Date
 *                               'T' = Topic name
 *                               'N' = Tag name
 *                               'X' = Tag text ID
 *                               'U' = User name (firstname lastname)
 *                              Default = 'ND'.
 *
 *  @return array of ArticleStatus objects with all info.
 */
function getTagHistory($articleId, $topicId=null, $tagIdStr=null,
                       $incComments=true, $inactive=false, $order='TD') {

    // Create the query
    $qry = db_select('ebms_article_tag', 'tag');
    $qry->join('ebms_article_tag_type', 'ttype',
               'tag.tag_id = ttype.tag_id');
    $qry->leftjoin('users', 'usr',
                   'tag.user_id = usr.uid');
    $qry->leftjoin('ebms_topic', 'topic',
                   'tag.topic_id = topic.topic_id');
    $qry->fields('tag', array('article_tag_id', 'tag_id', 'topic_id',
                 'user_id', 'tag_dt', 'active_status'))
        ->fields('ttype', array('tag_name', 'text_id'))
        ->fields('topic', array('topic_name'))
        ->fields('usr', array('name'));

    // Selection critera
    $qry->condition('tag.article_id', $articleId);
    if ($topicId) {
        // Get everything for this topic, or with no topic identified
        // Otherwise, no topic restrictions at all
        $qry->condition(
            db_or()->condition('tag.topic_id', $topicId)
                   ->isNull('tag.topic_id'));
    }
    if (!$inactive)
        $qry->condition('tag.active_status', 'A');

    // Sort order
    for ($i=0; $i<strlen($order); $i++) {
        switch ($order[$i]) {
            case 'D': $fld = 'tag.tag_dt';       break;
            case 'T': $fld = 'topic.topic_name'; break;
            case 'N': $fld = 'ttype.tag_name';   break;
            case 'X': $fld = 'ttype.text_id';    break;
            case 'U': $fld = 'usr.name';         break;
            default:
                throw new \Exception(
                    "getTagHistory: Invalid sort order token '$order[$i]'");
        }
        $qry->orderBy($fld);
    }

    // Get it all
    $tagData = $qry->execute()->fetchAllAssoc('article_tag_id');

    // Build array of objects
    $tags = array();
    foreach ($tagData as $td) {
        $tt = new EbmsArticleTag();
        $tt->article_id     = $articleId;
        $tt->article_tag_id = $td->article_tag_id;
        $tt->dt             = $td->tag_dt;
        $tt->type_id        = $td->tag_id;
        $tt->type_text_id   = $td->text_id;
        $tt->type_name      = $td->tag_name;
        $tt->user_id        = $td->user_id;
        $tt->user_name      = $td->name;
        $tt->topic_id       = $td->topic_id;
        $tt->topic_name     = $td->topic_name;
        $tt->active_status  = $td->active_status;
        $comments           = array();

        // Save it, indexed by tag row ID
        $tags[$tt->article_tag_id] = $tt;
    }

    // Add comments if requested
    if (count($tags) > 0 && $incComments) {

        // Construct string of article state IDs for IN statement
        $tagIdList = implode(',', array_keys($tags));

        // Locate comments for all of them
        $qry = db_select('ebms_article_tag_comment', 'comment');
        $qry->leftjoin('users', 'usr', 'comment.user_id = usr.uid');
        $qry->fields('comment', array('article_tag_id', 'comment_id',
                                      'user_id', 'comment_dt', 'comment'))
            ->fields('usr', array('uid', 'name'))
            ->where("comment.article_tag_id IN ($tagIdList)")
            ->orderBy('comment.article_tag_id', 'DESC')
            ->orderBy('comment.comment_dt', 'ASC');

        $comments = $qry->execute()->fetchAllAssoc('comment_id');

        // Construct a comment object and interpolate into the state data
        foreach ($comments as $cm) {
            // Construct fields
            $cmt = new ArticleComment();
            $cmt->comment_id    = $cm->comment_id;
            $cmt->references_id = $cm->article_tag_id;
            $cmt->user_id       = $cm->user_id;
            $cmt->user_name     = $cm->name;
            $cmt->dt            = $cm->comment_dt;
            $cmt->text          = $cm->comment;

            // Interpolate into the correct status record comment array
            $tags[$cm->article_tag_id]->comments[] = $cmt;
        }
    }

    return $tags;
}

/**
 * Render a tag history array as HTML.
 *
 *  @param array $tagHistory    Return from getTagHistory().
 *
 *  @return string              HTML.
 */
function tagHistoryToHtml($tagHistory) {

    // Convert array of objects to array of strings
    $histHtmlArray = array();
    foreach ($tagHistory as $th)
        $histHtmlArray[] = $th->toHtml();

    // Convert the array to a single string
    $html = <<<EOS
<style type='text/css'>
#table { border: 2px; }
 .inactiveStat { color: grey; }
 .activeStat   { color: blue; }
#testresults {
 width: 80%;
 margin-left: 10%;
 margin-right: 10%;
}
</style>

<div id='testresults'>
<h3>Tag History</h3>
<p class="activeStat">Active records shown in this color.<br />
<span class="inactiveStat">Inactivated records are in this color.</span></p>
<table class='debug' border='3'>
 <tr>
  <th>DateTime<br />ArtID/TagID</th>
  <th>TypeName<br />TagTextID / TypeTypeId</th>
  <th>UserName<br />UserID</th>
  <th>TopicName<br />TopicID</th>
 </tr>
EOS;
    $html .= implode('', $histHtmlArray) . "</table>\n</div>\n";

    return $html;
}

/**
 * Generates a array of tag type names, indexed by their tag's text_id.
 *
 * @param bool $articleOnly only returns tags that do not require a topic
 *                          if true
 * @return mixed An array mapping tag text_ids to tag_names.
 */
function getTagTypeNames($articleOnly = true) {
    // query for the tag text IDs and tag names
    $query = db_select('ebms_article_tag_type', 'a')
        ->condition('a.active_status', 'A');
    $result = $query->fields('a', array('text_id', 'tag_name'))
        ->execute();

    // load the results into a text_id => tag_name array.
    $names = array();
    foreach ($result as $record) {
        $names[$record->text_id] = $record->tag_name;
    }

    return $names;
}

/**
 * Store the file ID in the Drupal file table for article full text.
 *
 *  @param int $articleId       Unique internal ID of the article.
 *  @param int $fullTextId      Unique fid of the full text.
 *
 *  @throws Exception           From database if error.
 */
function storeArticleFullTextId($articleId, $fullTextId) {

    $qry = db_update('ebms_article');
    $qry->fields(array('full_text_id'));
    $qry->condition('article_id', $this->article_id);
    $qry->execute();
}

/**
 * Get all topics associated with an article.
 *
 *  @param int $articleId   Unique internal ID of article.
 *
 *  @return array of EbmsArticleTopic objects.
 *
 *  @throws Exception if database error.
 */
function getTopics($articleId) {

    // Get the topic info from the database
    $result = db_query(
       'SELECT DISTINCT state.topic_id as topic_id,
               topic.topic_name as topic_name,
               topic.board_id as board_id,
               board.board_name as board_name
          FROM ebms_article_state state
          JOIN ebms_topic topic
            ON state.topic_id = topic.topic_id
          JOIN ebms_board board
            ON topic.board_id = board.board_id
         WHERE state.article_id = :artId
           AND state.active_status = :active
         ORDER BY board.board_name, topic.topic_name',
         array(':artId'=>$articleId, ':active'=>'A'));

    // Copy them into object(s) for return
    // Is it better to just return a "standard" object?
    $retTopics = array();
    foreach ($result as $row) {
        $at             = new EbmsArticleTopic;
        $at->article_id = $articleId;
        $at->topic_id   = $row->topic_id;
        $at->topic_name = $row->topic_name;
        $at->board_id   = $row->board_id;
        $at->board_name = $row->board_name;

        $retTopics[] = $at;
    }

    return $retTopics;
}


/**
 * Get the name of an editorial board from its internal ID.
 *
 *  @param int $boardId     Internal board ID.
 *
 *  @return string          Human readable name.
 *
 *  @throws Exception       If no board found.
 */
function getBoardNameById($boardId) {

    $boardName = db_query(
        'SELECT board_name FROM {ebms_board} WHERE board_id = :boardId',
        array(':boardId' => $boardId))->fetchField();
    if (is_null($boardName))
        throw new \Exception("getBoardNameById: no board with id=$boardId");

    return $boardName;
}


/**
 * Get the ID of an editorial board from its human readable name.
 *
 *  @param int $boardName   Internal board ID.
 *
 *  @return string          Human readable name.
 *
 *  @throws Exception       If no board found.
 */
function getBoardIdByName($boardName) {

    $boardName = db_query(
        'SELECT board_id FROM {ebms_board} WHERE board_name = :boardName',
        array(':boardName' => $boardName))->fetchField();
    if (is_null($boardName))
        throw new \Exception("getBoardNameById: no board with name=$boardName");

    return $boardName;
}


/**
 * Get the name of a state from its internal ID.
 *
 *  @param int $stateId     Internal state ID.
 *
 *  @return string          Human readable name.
 *
 *  @throws Exception       If no state found.
 */
function getStateNameById($stateId) {

    $stateName = db_query(
        'SELECT state_name
           FROM {ebms_article_state_type}
          WHERE state_id = :stateId',
        array(':stateId' => $stateId))->fetchField();

    if (is_null($stateName))
        throw new \Exception("getStateNameById: no state with id=$stateId");

    return $stateName;
}


/**
 * Get the ID of a state from its human readable name.
 *
 *  @param int $stateName   Internal state ID.
 *
 *  @return string          Human readable name.
 *
 *  @throws Exception       If no state found.
 */
function getStateIdByName($stateName) {

    $stateName = db_query(
        'SELECT state_id
           FROM {ebms_article_state_type}
          WHERE state_name = :stateName',
        array(':stateName' => $stateName))->fetchField();

    if (is_null($stateName))
        throw new \Exception("getStateIdByName: no state with name=$stateName");

    return $stateName;
}


/**
 * Get the name of a summary topic from its internal ID.
 *
 *  @param int $topicId     Internal topic ID.
 *
 *  @return string          Human readable name.
 *
 *  @throws Exception       If no topic found.
 */
function getTopicNameById($topicId) {

    $topicName = db_query(
        'SELECT topic_name FROM {ebms_topic} WHERE topic_id = :topicId',
        array(':topicId' => $topicId))->fetchField();

    if (is_null($topicName))
        throw new \Exception("getTopicNameById: no topic with id=$topicId");

    return $topicName;
}


/**
 * Get the ID of a summary topic from its human readable name.
 *
 *  @param int $topicName   Internal topic ID.
 *
 *  @return string          Human readable name.
 *
 *  @throws Exception       If no topic found.
 */
function getTopicIdByName($topicName) {

    $topicName = db_query(
        'SELECT topic_id FROM {ebms_topic} WHERE topic_name = :topicName',
        array(':topicName' => $topicName))->fetchField();

    if (is_null($topicName))
        throw new \Exception("getTopicIdByName: no topic with name=$topicName");

    return $topicName;
}

/**
 * Get the ID of a descriptive tag from its human readable name.
 *
 *  @param int $tagName   Internal tag ID.
 *
 *  @return string        Human readable name.
 *
 *  @throws Exception     If no tag found.
 */
function getTagIdByName($tagName) {

    $tagId = db_query(
        'SELECT tag_id FROM {ebms_article_tag_type} WHERE tag_name = :tagName',
        array(':tagName' => $tagName))->fetchField();

    if (is_null($tagId))
        throw new \Exception("getTagIdByName: no tag with name=$tagName");

    return $tagId;
}

/**
 * Get the text_id of a descriptive tag from its human readable name.
 *
 *  @param int $tagName   Internal tag ID.
 *
 *  @return string          Human readable name.
 *
 *  @throws Exception       If no tag found.
 */
function getTagTextIdByName($tagName) {

    $tagTextId = db_query(
        'SELECT text_id FROM {ebms_article_tag_type} WHERE tag_name = :tagName',
        array(':tagName' => $tagName))->fetchField();

    if (is_null($tagTextId))
        throw new \Exception("getTagTextIdByName: no tag with name=$tagName");

    return $tagTextId;
}


/**
 * Get the EBMS internal article ID by a source ID (usually Pubmed).
 *
 *  @param string $sourceIdStr  ID assigned by original article source.
 *  @param string $source       Only 'Pubmed' in initial EBMS.
 *
 *  @return int                 Internal EBMS ID.
 *                              Returns null if not found.  A typical cause
 *                               is that the source (e.g. Pubmed) has the
 *                               article but we never imported it.
 */
function getArticleIdBySourceId($sourceIdStr, $source='Pubmed') {

    // Search
    $articleId = db_query(
        'SELECT article_id
           FROM {ebms_article}
          WHERE source_id = :sourceId
            AND source = :source',
         array(':sourceId'=>$sourceIdStr, ':source'=>$source))->fetchField();

    // Return result, which may be null
    return $articleId;
}


/**
 * Verify that an internal EBMS article ID exists in the database.
 *
 *  @param int $articleId   Verify this number.
 *
 *  @return bool            True = ID found.  False = not found.
 */
function verifyArticleId($articleId) {

    // Search
    $idCount = db_query('
         SELECT COUNT(0)
           FROM {ebms_article}
          WHERE article_id = :artId',
        array(':artId' => $articleId))->fetchField();

    // Will be exactly one if found
    if ($idCount == 1)
        return true;
    if ($idCount > 1) {
        throw new \Exception(
       "verifyArticleId: $idCount articles with ID=$articleId. Can't happen!");
    }

    return false;
}


/**
 * Get the boardId for a topicId.  Utility function.
 *
 *  @param int $topicId     Topic for which to find the board.
 *
 *  @return int $boardId    Board owning this topic.
 *
 *  @throws Exception if database error or topic not found.
 */
function getTopicBoard ($topicId) {

    // Check the topic
    $result = db_query(
        'SELECT board_id
           FROM {ebms_topic}
          WHERE topic_id = :topId',
         array(':topId' => $topicId));
    $boardId = $result->fetchField();

    // Must be there or caller has a big problem
    if (!$boardId)
        throw new \Exception("getTopicBoard: No topic numbered '$topicId'");

    return $boardId;
}

/**
 * Retrieves the related packets for a given article id, optionally limiting
 * the results by a topic id.
 *
 * @param int $article_id   the article id to match.
 *
 * @param int $topic_id     the optional topic id.
 *
 * @param bool $find_all    if true, returns all packets, regardless of
 *                          inactivity
 *
 * @param bool $active      limits the results to active or inactive packets
 *
 * @param bool $include_dropped returns packets that have been dropped from the
 *                              article
 *
 * @return mixed $packet_ids    Array of packet ids matching the article and
 *                               topic, and limited to active or inactive.
 */
function getArticlePacketIds($article_id, $topic_id = NULL, $find_all = false,
        $active = true, $include_dropped = false) {
    $query = db_select('ebms_packet_article', 'a');
    $query->join('ebms_packet', 'p', 'p.packet_id = a.packet_id');

    if($topic_id != NULL){
        $query->condition('p.topic_id', $topic_id);
    }

    if(!$find_all){
        $flag = $active ? 'A' : 'I';
        $query->condition('p.active_status', $flag);
    }

    if(!$include_dropped){
        $query->condition('a.drop_flag', 0);
    }

    $packet_ids = $query
        ->condition('a.article_id', $article_id)
        ->fields('a', array('packet_id'))
        ->execute()
        ->fetchCol();

    return $packet_ids;
}

/**
 * Collects a variety of information about a given packet and organizes it into
 * a labeled array.
 *
 * @param int $packet_id        The id of the packet of interest.
 *
 * @return mixed $packet_info   An associative array of packet information.
 */
function getPacketInfo($packet_id) {
    $info = db_select('ebms_packet', 'p')->condition('p.packet_id', $packet_id)
        ->fields('p')->execute()->fetch();

    // also retrieve the reviewer ids
    $reviewers = db_select('ebms_packet_reviewer', 'r')
        ->condition('r.packet_id', $packet_id)
        ->fields('r', array('reviewer_id'))
        ->execute()
        ->fetchCol();

    $info->reviewers = $reviewers;

    return $info;
}

/**
 * Retrieves each board member's response for a topic of a particular article.
 *
 * @param int $article_id   the id of the article.
 *
 * @param int $topic_id     the topic id used to filter the reponses.
 *
 * @param bool $fetch_all   returns all responses, irrespective of active state.
 *
 * @param bool $active      returns only active or inactive reponses, in
 *                          conjunction with $fetch_all == false
 *
 * @return mixed $reponses  an array of DB records containing pertinent info.
 */
function getBoardMemberResponses($article_id, $topic_id, $fetch_all = false, $active = true) {
    $query = db_select('ebms_article_review', 'r');
    $query->join('ebms_review_disposition', 'd', 'r.review_id = d.review_id');
    $query->join('ebms_review_disposition_value', 'v', 'd.value_id = v.value_id');
    $query->join('ebms_packet', 'p', 'p.packet_id = r.packet_id');

    if (!$fetch_all) {
        $flag = $active ? 'A' : 'I';
        $query->condition('p.active_status', $flag);
    }

    return $query->fields('r')
            ->fields('p', array('packet_id', 'topic_id'))
            ->fields('d', array('value_id'))
            ->fields('v', array('value_name', 'instructions'))
            ->condition('r.article_id', $article_id)
            ->condition('p.topic_id', $topic_id)
            ->execute()
            ->fetchAll();
}

/**
 * Retrieves any board decisions that match the given article and topic ids
 *
 * @param int $article_id   The desired article's id.
 *
 * @param int $topic_id     The desired topic of the decision.
 *
 * @return mixed $decisionTree  An array of zero or more records from the database,
 *                              organized by article_status_id, with comments
 *                              attached if present.
 */
function getBoardDecisions($article_id, $topic_id) {
    $query = db_select('ebms_article_board_decision', 'd');
    $query->join('ebms_article_board_decision_value', 'v', 'd.decision_value_id = v.value_id');
    $query->join('ebms_article_state', 's', 's.article_state_id = d.article_state_id');

    $decisions =  $query->fields('s')
            ->fields('d', array('meeting_date', 'discussed'))
            ->fields('v', array('value_name'))
            ->condition('s.article_id', $article_id)
            ->condition('s.topic_id', $topic_id)
            ->execute()
            ->fetchAll();

    $decisionTree = array();

    foreach($decisions as $decision){
        $decisionTree[$decision->article_state_id][] = $decision;
    }

    $history = getStatusHistory($article_id, null, $topic_id);

    // grab the available comments for each article state
    foreach($decisionTree as $articleStateId => $decision){
        if(isset($history[$articleStateId]))
        {
            $decisionTree[$articleStateId]['comments'] = $history[$articleStateId]->comments;
        }
    }

    return $decisionTree;
}

/**
 * Retrieves any board decisions that match the given article state.
 *
 * @param int $article_state_id   The desired article state's id.
 *
 * @return mixed $decisions     An array of zero or more records from the database,
 *                              representing decisions afor the article state.
 */
function getBoardDecisionsByArticleState($article_state_id) {
    $query = db_select('ebms_article_board_decision', 'd');
    $query->join('ebms_article_board_decision_value', 'v', 'd.decision_value_id = v.value_id');
    $query->leftJoin('ebms_cycle', 'c', 'd.meeting_date = c.cycle_id');

    return $query
            ->fields('d', array('discussed'))
            ->fields('v', array('value_name'))
            ->fields('c', array('cycle_name'))
            ->condition('d.article_state_id', $article_state_id)
            ->execute()
            ->fetchAll();
}

/**
 * Retrieves all board member uids associated with a board decision.
 *
 * @param int $article_state_id   The decision's article state id.
 *
 * @return mixed $memberIds     An array of related member uids
 */
function getBoardDecisionMembers($article_state_id) {
    $query = db_select('ebms_article_board_decision_member', 'm');

    return $query
            ->fields('m', array('uid'))
            ->condition('m.article_state_id', $article_state_id)
            ->execute()
            ->fetchCol();
}


/**
 * Retrieves all matching review cycles for the given article and topic ids.
 *
 * @param int $article_id  The article id to match.
 * @param int $topic_id    The topic id to match
 * @return mixed $result   An array of rows matching the given filters.
 */
function getArticleReviewCycle($article_id, $topic_id = null){
    // get appropriate 'ready for initial review' rows
    $state_query = db_select('ebms_article_state_type', 't')
        ->condition('t.state_text_id', EbmsArticleStates::ReadyInitReview)
        ->fields('t', array('state_id'));

    $article_query = db_select('ebms_article_state', 'a')
        ->condition('a.active_status', 'A')
        ->condition('a.article_id', $article_id)
        ->fields('a');

    if ($topic_id != null)
        $article_query->condition ('a.topic_id', $topic_id);

    $ready_query = db_select($article_query, 's');
    $ready_query->join($state_query, 't', 's.state_id = t.state_id');
    $ready_query->fields('s');

    $cycle_query = db_select($ready_query, 'r');
    $cycle_query->join('ebms_cycle', 'c',
        'c.start_date IN (SELECT MIN(c1.start_date) FROM ebms_cycle c1 WHERE c1.start_date > r.status_dt)');
    $cycle_query->addExpression('DISTINCT c.cycle_name', 'cycle_name');

    return $cycle_query
        ->execute()
        ->fetchAll();
}

/**
 * Retrieves all possible values of review dispositions.
 *
 * @return mixed An array containing all rows of the review disposition table.
 */
function getReviewDispositions()
{
    return db_select('ebms_review_disposition_value', 'v')
        ->fields('v')
        ->execute()
        ->fetchAll();
}

/**
 * Retrieves a state type, identified by its text ID.
 *
 * @param string $state_text_id The text id of the intended state.
 *
 * @return mixed $sequence The state's matching details.
 */
function getStateTypebyTextId($state_text_id) {
    return db_select('ebms_article_state_type', 't')
            ->condition('state_text_id', $state_text_id)
            ->fields('t')
            ->execute()
            ->fetch();
}

/**
 * Retrieves all possible values of editorial board decisions.
 *
 * @return mixed An array containing all rows of the board decision value table.
 */
function getBoardDecisionValues() {
    return db_select('ebms_article_board_decision_value', 'v')
            ->fields('v')
            ->execute()
            ->fetchAll();
}

/**
 * Retrieves relevant information for an article's import.
 *
 * @param int $articleId The id of the article in question.
 *
 * @return mixed $return An associative array, branching by board_id, topic_id
 *      and active_state, with the leaves being arrays of database rows
 *      containing all available information related to the article's import.
 */
function getArticleImportData($articleId){
    $query = db_select('ebms_import_action', 'a');
    $query->join('ebms_import_batch', 'b',
        'a.import_batch_id = b.import_batch_id');
        $query->join('ebms_import_disposition', 'd',
            'a.disposition_id = d.disposition_id');

    $result = $query
        ->condition('a.article_id', $articleId)
        ->fields('a')
        ->fields('d', array(
            'text_id',
            'disposition_name',
            'description',
            'active_status'))
        ->fields('b', array(
            'topic_id',
            'source',
            'import_date',
            'cycle_id',
            'user_id',
            'article_count',
            'comment',))
        ->execute()
        ->fetchAll();

    $data = array();
    foreach($result as $record){
        $topicId = $record->topic_id;
        $boardId = 0;
        if(!$topicId) $topicId = 0;
        else{
            $boardId = getTopicBoard($topicId);
        }
        $data[$boardId][$topicId][$record->active_status][$record->import_batch_id][] = $record;
    }

    return $data;
}

/**
 * Retrieves all user's names that are associated with importing a given article.
 *
 * @param int $articleId The id of the article in question.
 *
 * @return mixed $userNames an array of user names found to have participated
 *      in importing this article.
 */
function getArticleImporters($articleId){
    $query = db_select('ebms_import_action', 'a');
    $query->join('ebms_import_batch', 'b',
        'a.import_batch_id = b.import_batch_id');
    $query->join('users', 'u', 'b.user_id = u.uid');

    $query->addExpression('DISTINCT u.name', 'user_name');

    return $query
        ->condition('a.article_id', $articleId)
        ->execute()
        ->fetchCol();
}

/**
 * Retrieves the numeric sequence number for a particular article state type.
 *
 * @param string $textId The state type's text id.
 *
 * @return int $sequence The state type's related sequence number.
 */
function getStateTypeSequence($textId){
    return db_select('ebms_article_state_type', 't')
        ->condition('t.state_text_id', $textId)
        ->fields('t', array('sequence'))
        ->execute()
        ->fetchField();
}

/**
 * Retrieves the article history, limited to states matching the given topic and
 * state type text id.
 *
 * @param type $articleId   The article to match.
 * @param type $topicId     The topic to match.
 * @param type $stateTextId The state type's text id to match.
 * @return array $filtered  The filtered history.
 */
function getStatusHistoryByType($articleId, $topicId, $stateTextId)
{
    $statii = getStatusHistory($articleId, null, $topicId);

    $filtered = array();
    foreach($statii as $articleStateId => $status)
    {
        if($status->type_text_id == $stateTextId)
            $filtered[$articleStateId] = $status;
    }

    return $filtered;
}

?>
