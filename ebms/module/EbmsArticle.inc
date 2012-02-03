<?php
/**
 * One article / topic association
 */
class EbmsArticleTopic {
    public
        $article_id,    // Unique ID of article
        $topic_id,      // ID of topic
        $topic_name,    // Human readable name
        $board_id,      // Board that owns this topic
        $board_name,    // Human readable name
        $cycle_id,      // Review cycle when assigned
        $cycle_name,    // Name of cycle, e.g., 'January 2012'
        $dt;            // Date of the association

    public function __toString() {
        // $term = "\n";
        $term = "<br />\n";
        $text  = "article_id: {$this->article_id}$term";
        $text .= "  topic_id: {$this->topic_id}$term";
        $text .= "topic_name: {$this->topic_name}$term";
        $text .= "  board_id: {$this->board_id}$term";
        $text .= "board_name: {$this->board_name}$term";
        $text .= "        dt: {$this->dt}$term";

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
     *  @param string $lastName     - Surname.
     *  @param string $foreName     - First name(s), may be null.
     *  @param string $inits        - First and optional more initials.
     */

    function __construct($aId, $lastName, $foreName, $inits) {

        // Translate any utf-8 characters using routine in this module
        require_once('common.inc');

        // Normalize and validate data
        $lastName = trim(Ebms\Util::utf8ToAscii($lastName));
        if (!$lastName)
            throw new Exception(
                    "EbmsArticleAuthor.__construct: Author missing last name");

        $foreName = trim(Ebms\Util::utf8ToAscii($foreName));
        if (!$foreName)
            $foreName = '';

        $initials = trim(Ebms\Util::utf8ToAscii($inits));
        if (!$initials)
            $initials = '';

        $this->author_id  = $aId;
        $this->last_name  = $lastName;
        $this->forename   = $foreName;
        $this->initials   = $inits;
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
     *  @param string $xmlStr       Serial XML.
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
            throw new Exception(
                    "EbmsArticle constructor: Must pass either " .
                    "one of articleId or xmlStr");

        // And not both
        if ($articleId && $xmlStr)
            throw new Exception(
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
                throw new Exception(
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

        // Only loads what's in the row, get other stuff lazily
        $row = db_query(
                "SELECT *
                   FROM {ebms_article}
                  WHERE article_id = :artId",
               array('artId' => $articleId))->fetchObject();

        // Caller should not be using an internal ID that doesn't exist
        if (count($row) == 0) {
            // Record error and throw exception
            $msg = 'EbmsArticle.__construct(id): ' .
                   "No database row found for article ID=$articleId";
            $this->addError($msg, true);
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
     * Clear all values from a Citation object.
     */
    public function clear() {
      $this->article_id     = NULL;
      $this->source         = NULL;
      $this->source_id      = NULL;
      $this->source_jrnl_id = NULL;
      $this->source_status  = NULL;
      $this->article_title  = NULL;
      $this->jrnl_title     = NULL;
      $this->brf_jrnl_title = NULL;
      $this->import_date    = NULL;
      $this->update_date    = NULL;
      $this->abstract       = NULL;
      $this->published_date = NULL;
      $this->brf_citation   = NULL;
      $this->authors        = NULL;
      $this->source_data    = NULL;
      $this->full_text_id   = NULL;
      $this->errorMsgs      = NULL;
    }

    /**
     * Simple getters get data that was loaded from the ebms_article row.
     */
    public function getId()           { return $this->article_id; }
    public function getSource()       { return $this->source; }
    public function getSourceId()     { return $this->source_id; }
    public function getSourceJrnlId() { return $this->source_jrnl_id; }
    public function getSourceStatus() { return $this->source_status; }
    public function getArticleTitle() { return $this->jrnl_article_title; }
    public function getJrnlTitle()    { return $this->jrnl_title; }
    public function getBrfJrnlTitle() { return $this->brf_jrnl_title; }
    public function getAbstract()     { return $this->abstract; }
    public function getPubDate()      { return $this->published_date; }
    public function getImportDate()   { return $this->import_date; }
    public function getUpdateDate()   { return $this->update_date; }

    /**
     * This one doesn't come from the database.
     */
    public function getErrorMsgs(){ return $this->errorMsgs; }

    /**
     * Retrieve authors.  Uses cache if available, else goes to the DB.
     *
     *  @param int|null $maxAuthors      - Max number to fetch, null=all.
     *                                     If negative, fetch from end.
     *
     *  @return array(EbmsArticleAuthor) - Retrieved authors as objects.
     *
     *  @throws Exception                - On database or other error.
     */
    public function getAuthors($maxAuthors = null) {

        // If no max given, get all (using an impossibly high max)
        if (!$maxAuthors)
            $maxAuthors = 99999;

        // If we don't already have them, get them
        if (!$this->authors) {
            // If no authors and no database article ID, something bad happened
            if (!$this->article_id)
                throw new Exception("EbmsArticle.getAuthors: " .
                  "Attempted to retrieve authors when no article ID present.");

            // Fetch all of them, otherwise caching defeats us if next
            //  caller wants more than we got.
            // This might get more than we need but we'll only optimize
            //  if experience shows the need
            $result = db_query("
             SELECT a.author_id, a.last_name, a.forename, a.initials
               FROM {ebms_article_author} a
               JOIN {ebms_article_author_cite} c
                 ON a.author_id = c.author_id
              WHERE c.article_id = :artId
              ORDER BY c.cite_order",
                  array(':artId' => $this->article_id))->fetchAll();

            // Copy each stdClass object to an Author object
            foreach ($result as $row) {
                $a = new EbmsArticleAuthor($row->author_id,
                        $row->last_name, $row->forename, $row->initials);
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
                throw new Exception(
                 "EbmsArticle.getSourceData: no source data, can't happen.");
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
     *  @param unsigned int $fullTextFileId - An ID key into the Drupal
     *                                         "file_managed" table.
     *
     *  @return void.
     *
     */
    public function setFullText($fullTextFileId) {

        // UNTESTED

        $this->full_text_id = $fullTextFileId;

        /* TODO:
         *  If there is an article_id, update the row in the ebms_article
         *    table to have the file ID.
         *  Else
         *    Just put the ID in the object and wait for a store() to save it.
         */
    }

    /**
     * Append an error message.
     *
     *  @param string $msg   - Error message.  Should include source func name.
     *  @param bool   $throw - True = also throw exception.
     */
    private function addError($msg, $throw=false) {
        if (is_null($this->errorMsgs))
            $this->errorMsgs = array();
        $this->errorMsgs[] = $msg;

        if ($throw)
            throw new Exception($msg);
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
            $errMsg = "Citation::loadPubmedXml: Wrong node passed " .
                      "Expected \"PubmedArticle\". Got: \"$pmaTag\"\n";
            // XXX Log them, then throw exception
            throw new Exception($errMsg);
        }
        // Find the top of the record
        $mc                   = $pma->MedlineCitation;
        $article              = $mc->Article;

        // There are other Pubmed data types besides "Article" but, for
        //   now at least, only Article is supported.
        if (!$article) {
            $msg = 'EbmsArticle.loadPubmedXml: ' .
                   'Record does not appear to be a Pubmed journal article.';
            $this->addError(msg);
            throw new Exception (msg);
        }

        $journal              = $mc->Article->Journal;
        $this->source         = 'Pubmed';
        $this->source_id      = $mc->PMID;
        $this->source_jrnl_id = $mc->MedlineJournalInfo->NlmUniqueID;
        $this->source_status  = $mc['Status'];
        $this->article_title  = $article->ArticleTitle;
        $this->jrnl_title     = $journal->Title;
        $this->brf_jrnl_title = $mc->MedlineJournalInfo->MedlineTA;
        $this->abstract       = $this->loadAbstract($article->Abstract[0]);
        $this->authors        = $this->loadAuthors($article->AuthorList[0]);

        // Published date and brief citation set here
        $this->loadCite($article);

        // import_date, update_date, and article_id all populated in the
        //   store() method
    }

    /**
     * Load object with contents of a Pubmed format XML string.
     *
     *  @param string $xmlStr - Serial XML in UTF-8 for element "PubmedArticle".
     *
     *  @return void.
     *
     *  @throws Exception     - If parser or xml error.
     *
     * TODO: All strings need to be utf-8 filtered to plain ASCII.
     * NOTE: One way is to invoke the filter to create a filtered copy of the
     *       entire XML record, then parse the copy.  Fast, simple, complete.
     */
    public function loadPubmedXmlStr($xmlStr) {
        // Manage errors ourselves
        libxml_use_internal_errors(true);

        // Clean slate
        $this->clear();

        // Parse xml
        $topElem = simplexml_load_string($xmlStr);
        if (!$topElem) {
            $errMsg="Citation::loadPubmedXmlStr: Failure parsing xml string:\n";
            foreach (libxml_get_errors() as $error) {
                $errMsg += "  " + $error + "\n";
                $this->addError('EbmsArticle.loadPubmedXmlString: ' . $errMsg);
            }
            // XXX Log them, then throw exception
            throw new Exception($errMsg);
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

        // Array of arrays
        $arrayList = array();

        foreach ($authorList->Author as $author) {
            // authorId is null unless/until record is/was stored
            $oneAuth = new EbmsArticleAuthor(null,
               $author->LastName, $author->ForeName, $author->Initials);

            // Append this author to the list of authors
            // Note: order is significant, first author, second, etc.
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
            return NULL;

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
        $text .= "   update_date: " . $this->update_date . $term;
        $text .= "      abstract: " . $this->abstract . $term;
        $text .= "published_date: " . $this->published_date . $term;
        $text .= "  brf_citation: " . $this->brf_citation . $term;
        $text .= "       Authors:$term";

        // Authors
        foreach ($this->authors as $auth) {
            $text .= "          " .
                $auth->author_id . ': ' . $auth->last_name . ', ' .
                $auth->forename . " (" . $auth->initials . ")$term";
        }

        // Just indicate whether this are present or not
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
     *  @param int $userId       - ID of authorized user.
     *  @param string $storeDate - Date to assign to all dates,
     *                             'YYYY-MM-DD HH:MM:SS'.
     *                Else use now().  Specifying the same date
     *                for a batch avoids having a different datetime on
     *                each record in the batch.
     *
     *  @return void.
     *
     * Throws:
     *   Exception on database error.
     */
    public function store($userId, $storeDate=null) {

        // TODO: Check authorization

        // Get a uniform date if needed
        if (is_null($storeDate))
            $storeDate = db_query('SELECT NOW()')->fetchField();

        // Is this a new record or an update?
        $newRec = true;
        if ($this->article_id) {
            $newRec = false;
            $this->update_date = $storeDate;
        }
        else {
            $this->import_date = $storeDate;
            $this->update_date = null;
        }

        // Here are the fields we'll store
        //   article_id handled separately for new and replace records
        $fields = array(
            'source'         => $this->source,
            'source_id'      => $this->source_id,
            'source_jrnl_id' => $this->source_jrnl_id,
            'source_status'  => $this->source_status,
            'article_title'  => $this->article_title,
            'jrnl_title'     => $this->jrnl_title,
            'brf_jrnl_title' => $this->brf_jrnl_title,
            'import_date'    => $this->import_date,
            'update_date'    => $this->update_date,
            'abstract'       => $this->abstract,
            'published_date' => $this->published_date,
            'brf_citation'   => $this->brf_citation,
            'source_data'    => $this->source_data,
            'full_text_id'   => $this->full_text_id
        );

        // TODO: Wrap this in a transaction?

        // If we're inserting a new record with store
        if ($newRec) {

            $query = db_insert('ebms_article');
            $query->fields($fields);
            $article_id = $query->execute();

            // Make id available
            $this->article_id = $article_id;
        }
        else {
            $query = db_update('ebms_article');
            $query->fields($fields);
            $query->condition('article_id', $this->article_id);
            $query->execute();

            // Delete any author associations.  We'll replace them
            $this->unlinkAuthors();
        }

        // Store the authors in the record and associate them with article
        $this->storeAuthors();

        // TODO: Commit transaction?
    }

    /**
     * Store authors in the ebms_article_author and ebms_article_author_cite
     * tables.  Updates in memory object to include author IDs.
     *
     * Authorization checking is not required since the only way to get to
     * this function is through an authorized function.
     *
     * Helper function for store() and replace().
     */
    private function storeAuthors() {

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
                $query->condition('a.last_name', $author->last_name);
                if ($author->forename)
                    $query->condition('a.forename', $author->forename);
                else
                    $query->condition('a.forename', '');
                if ($author->initials)
                    $query->condition('a.initials', $author->initials);
                else
                    $query->condition('a.initials', '');
                $result = $query->execute();
                if (count($result) > 0) {
                    // This author is known already
                    $author->author_id = $result->fetchField();
                }

                if (!$author->author_id) {

                    // Still no author_id. Create an author record
                    $query = db_insert('ebms_article_author')
                     ->fields(array(
                        'last_name' => $author->last_name,
                        'forename'  => $author->forename,
                        'initials'  => $author->initials));

                    // Execute query, getting back the last insert ID
                    $author->author_id = $query->execute();

                    // Test for success
                    if (!$author->author_id)
                        $this->addError(
                                "EbmsArticle.storeAuthors: " .
                                "Could not create author record for " .
                                $author->last_name . ', ' .
                                $author->fore_name . ' ' .
                                $author->initials);
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
     *  In many (most?) cases the author lists will be identical.
     */
    private function unlinkAuthors() {

        // UNTESTED

        // First delete all associations between authors and article
        $result = db_query(
            "DELETE {ebms_article_author_cite}
              WHERE article_id = :articleId",
            array('articleId'=>$this->article_id));

        // That may leave some authors with no citations
        // Delete them
        $result = db_query(
            "DELETE {ebms_article_author}
              WHERE TODO XXX"
            );


        // Get a list of all authors of the article
        $authorList = $this->getAuthors();

        // TODO: Incomplete - finish this and delete article_author if need
        // Delete all of their associations with the article
        // $query = db_update('ebms_article_author_cite', 'a')
        //  ->condition('a.article_id', $this->article_id);
        // $query->execute();

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

        // Get the topic info from the database
        $result = db_query(
            "SELECT at.topic_id as topic_id,
                    t.topic_name as topic_name,
                    t.board_id as board_id,
                    b.board_name as board_name,
                    at.cycle_id as cycle_id,
                    c.cycle_name as cycle_name,
                    at.article_topic_dt as dt
               FROM {ebms_article_topic} at
               JOIN {ebms_topic} t
                 ON at.topic_id = t.topic_id
               JOIN {ebms_board} b
                 ON t.board_id = b.board_id
               JOIN {ebms_cycle} c
                 ON at.cycle_id = c.cycle_id
              WHERE at.article_id = :artId
              ORDER BY b.board_name, t.topic_name",
             array('artId' => $this->article_id));

        // Copy them into object(s) for return
        $retTopics = array();
        foreach ($result as $row) {
            $at             = new EbmsArticleTopic;
            $at->article_id = $this->article_id;
            $at->topic_id   = $row->topic_id;
            $at->topic_name = $row->topic_name;
            $at->board_id   = $row->board_id;
            $at->board_name = $row->board_name;
            $at->cycle_id   = $row->cycle_id;
            $at->cycle_name = $row->cycle_name;
            $at->dt         = $row->dt;

            $retTopics[] = $at;
        }

        return $retTopics;
    }


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
    public function addTopic($topicId, $userId, $cycleId,
                             $addDt=null, $method='P') {

        // Get a uniform date if needed
        if (is_null($addDt))
            $addDt = db_query('SELECT NOW()')->fetchField();

        // Check if topic already present
        $result = db_query(
            "SELECT COUNT(*)
               FROM {ebms_article_topic}
              WHERE article_id = :artId
                AND topic_id   = :topId",
            array('artId' => $this->article_id,
                  'topId' => $topicId));
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
}
?>
