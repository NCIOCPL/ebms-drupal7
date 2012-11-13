<?PHP

namespace Ebms;

require_once('logw.inc'); // XXX DEBUG

/**
 * Call this function to create and return a Drupal db_select
 * query object that retrieves article_ids matching a search.
 *
 *  @param string $json     Search criteria in JSON serialization format.
 *
 *  @return object          Drupal db_select query object.
 */
function getSearchQuery($json) {

    // Create object
    $srch = new EbmsSearch($json);

    // Try all of the search criteria
    $srch->srchIds();
    $srch->srchTopicsBoards();
    $srch->srchAuthors();
    $srch->srchArticleTitle();
    $srch->srchFullTextRtrvd();
    $srch->srchStates();
    $srch->srchIncludeNotList();
    $srch->srchJrnl();
    $srch->srchPubDate();
    $srch->srchImportActions();
    $srch->srchArticleReviewer();
    $srch->srchReviewerResponse();
    $srch->srchComments();
    $srch->srchTags();
    $srch->srchInputDate();
    $srch->srchModifiedDate();

    // Sort them
    $srch->srchOrder();

    // XXX - Should we limit results?
    // $srch->qry->range(0,500);

// DEBUG
$jspec = print_r($srch->spec, true);
logw("Full decoded json spec:\n$jspec\n");
$query = print_r($srch->qry, true);
logw("Completed query:\n$query\n");


    return $srch->qry;
}

/**
 * Creates a Drupal db_select query object that returns article_ids
 * to match passed JSON encoded search criteria.
 */
class EbmsSearch {

    public
        $qry,           // The drupal object query
        $json,          // The JSON encoded query field specs
        $spec;          // $json decoded into an array

    private
        $topics,        // The topic IDs for this search, if any.
        $boards;        // The board IDs for this search, if any.

    function __construct($json) {
        // Convert any utf-8 to ASCII using the same algorithm used
        //  when we constructed index strings.
        $json = Util::utf8ToAscii($json);

        // Save the search spec
        $this->json = $json;
        $this->spec = json_decode($json, true);

        // Topics and not yet analyzed and collected
        $this->topics = array();
        $this->boards = array();

        // Create the base query
        $this->qry = db_select('ebms_article', 'art');
        $this->qry->fields('art', array('article_id'));
        $this->qry->distinct();
    }

    /**
     * Search unique IDs.
     */
    function srchIDs() {

        // Ignore conflicts.  We may change that someday.
        if ($this->spec['pmid']) {
            $this->qry->condition('art.source', 'Pubmed');
            $this->qry->condition('art.source_id', $this->spec['pmid']);
        }
        if ($this->spec['cms-id'])
            $this->qry->condition('art.article_id', $this->spec['cms-id']);
    }

    /**
     * Search for articles with specified topics assigned or boards
     * assigned.  Only need boards if no topics assigned.
     *
     * Always do this before any check that assumes the state table is
     * already joined.
     */
    function srchTopicsBoards() {

        // Gather up topics
        // All topics are in the json, vals > 0 are only ones specified
        foreach ($this->spec['topics'] as $key => $val) {
            if ($val)
                $this->topics[] = $key;
        }

        // Include articles with at least one state with one of the topics
        if (!empty($this->topics)) {
            $this->qry->join('ebms_article_state', 'state',
                             'state.article_id = art.article_id');
            // Spec might request AND or OR
            if ($this->spec['topic-logic'] == 'OR')
                $this->qry->condition('state.topic_id', $this->topics, 'IN');
            else {
                foreach ($this->topics as $topic)
                    $this->qry->condition('state.topic_id', $topic);
            }
        }

        // Board specs only exist in the json if specified by user
        // If board(s) specified with no topic
        // XXX Maybe I'm too restrictive here.
        //     Easy to change if so, just remove the else {}
        foreach ($this->spec['boards'] as $key => $val)
            $this->boards[] = $val;

        // Join to same article states as topics
        // Might be able to optimize it away if we have topics, but might not
        //  because some conditions only check board.
        if (empty($this->topics) && !empty($this->boards)) {
            $this->qry->join('ebms_article_state', 'state',
                             'state.article_id = art.article_id');
            $this->qry->condition('state.board_id', $this->boards, 'IN');
        }
    }

    /**
     * Expects semicolon separated authors
     *   e.g., "Smith AH; Jones B"
     * Expects last name [space] initials.
     *   e.g., "Smith AH"
     * Does no wildcard matching unless specified with percent signs.
     * If multiple authors, article must have all of them (A AND B).
     */
    function srchAuthors() {

        if ($this->spec['author']) {
            // Parse out multiple authors
            $authors = explode(';', $this->spec['author']);

            // Join tables separately, one pair per author
            $count = 1;
            foreach ($authors as $author) {
                // Normalize and parse author name
                $author = trim($author);
                $author = preg_replace( "/\s+/", " ", $author);
                $names = explode(' ', $author);
                $lastName = $names[0];
                if (array_key_exists(1, $names))
                    $inits = trim($names[1]);
                else
                    $inits = null;

                // Create aliases
                $citeAlias = "cite$count";
                $authAlias = "auth$count";

                // Join the tables
                $this->qry->join('ebms_article_author_cite', $citeAlias,
                                 "$citeAlias.article_id = art.article_id");
                $this->qry->join('ebms_article_author', $authAlias,
                                "$authAlias.author_id = $citeAlias.author_id");

                // Conditions depend on name contents
                if ($this->chkWildCards($lastName))
                    $this->qry->condition("$authAlias.last_name", $lastName,
                                          'LIKE');
                else
                    $this->qry->condition("$authAlias.last_name", $lastName);

                if ($inits) {
                    if ($this->chkWildCards($inits))
                        $this->qry->condition("$authAlias.initials", $inits,
                                              'LIKE');
                    else
                        $this->qry->condition("$authAlias.initials", $inits);
                }
                // Ready for next author, if any
                ++$count;
            }
        }
    }

    /**
     * Title of the article, wildcards allowed.
     */
    function srchArticleTitle() {

        if ($this->spec['title']) {
            $artTitle = $this->spec['title'];
            if ($this->chkWildCards($artTitle))
                $this->qry->condition('art.article_title', $artTitle, 'LIKE');
            else
                $this->qry->condition('art.article_title', $artTitle);
        }
    }

    /**
     * Full text of article was retrieved.  Non-zero = Yes.
     */
    function srchFullTextRtrvd() {
        if ($this->spec['advanced-options']['full-text-retrieved']) {
            // Need a new join on the state table, even if we already
            //  joined for topic or board purposes
            // This flag is not board/topic specific
            $this->qry->isNotNull('art.full_text_id');
        }
    }


    /**
     * Not listed journals are normally excluded but can be included
     * upon request.
     */
    function srchIncludeNotList() {
        // This is a negative condition.  Only do something if user did not
        //  ask to include articles from NOT listed journals
        if (!$this->spec['advanced-options']['not-list']) {

            // Create a subquery to find not listed articles
            // Don't think active/inactive state matters here?
            $subqry = db_select('ebms_article_state', 'notjrnstate');
            $subqry->fields('notjrnstate', array('article_id'));
            $subqry->join('ebms_article_state_type', 'notjrntype',
                          'notjrntype.state_id = notjrnstate.state_id');
            $subqry->condition('notjrntype.state_text_id',
                               'RejectJournalTitle');

            //
            if (!empty($this->boards))
                $subqry->condition('notjrnstate.board_id', $this->boards, 'IN');

            // Exclude article IDs if they are found by the subquery
            $this->qry->condition('art.article_id', $subqry, 'NOT IN');
        }
    }

    /**
     * Search article state comments.  Tag comments are not searched
     * (though users might want that in the future.)
     *
     * The user must use '%' wild cards appropriately.  Otherwise the search
     * will consider the passed search criterion as the complete and exact
     * value of the comment - which is not often helpful.
     *
     * However a user can search for a comment date without regard to
     * whether the text matches anything.
     *
     *  @param bool $anyMods    If true, use modified-start/end date
     *                          specs.  Else use 'tag-added...' dates.
     */
    function srchComments($anyMods=true) {

        $comment = null;
        if ($anyMods)
            $startYear = $this->spec['modified-start']['year'];
        else {
            $startYear = $this->spec['comment-added-start']['year'];
            $comment   = $this->spec['comments'];
        }

        // If we have either of them, we need to join states and comments
        if ($comment || $startYear) {
            $this->qry->join('ebms_article_state', 'comstate',
                     'comstate.article_id = art.article_id');
            $this->qry->join('ebms_article_state_comment', 'statecom',
                     'statecom.article_state_id = comstate.article_state_id');

            // Search comments
            if ($comment) {
                $op = '=';
                if ($this->chkWildCards($comment))
                    $op = 'LIKE';
                $this->qry->condition('statecom.comment', $comment, $op);
            }

            // Search comment dates
            if ($startYear) {

                // Search for comments within the modified range
                if ($anyMods)
                    $dateLimits = $this->chkDates(
                            $this->spec['modified-start'],
                            $this->spec['modified-end'],
                            $this->spec['use-modified-date-range']);

                // Or search for comments within the comment-added range
                else
                    $dateLimits = $this->chkDates(
                            $this->spec['comment-added-start'],
                            $this->spec['comment-added-end'],
                            $this->spec['use-comment-date-range']);

                // If we found a valid range
                if ($dateLimits) {
                    $this->qry->condition('statecom.comment_dt',
                                           $dateLimits[0], '>=');
                    $this->qry->condition('statecom.comment_dt',
                                           $dateLimits[1], '<');
                }
            }

            // Standard qualifications
            if (!empty($this->topics))
              $this->qry->condition("comstate.topic_id", $this->topics, 'IN');
            else if (!empty($this->boards))
              $this->qry->condition("comstate.board_id", $this->boards, 'IN');
        }
    }

    /**
     * Search for tagged articles.
     *
     * This can be called to search for tags in response to a tag
     * entry, or can be called by srchModifiedDates() to look for
     * articles that acquired a tag within the modified date range.
     *
     *  @param bool $anyMods    If true, use modified-start/end date
     *                          specs.  Else use 'tag-added...' dates.
     */
    function srchTags($anyMods=false) {

        $tagId = $this->spec['tag'];
        if ($anyMods)
            $startYear = $this->spec['modified-start']['year'];
        else
            $startYear = $this->spec['tag-added-start']['year'];

        // If we have either of them, we need to join tags
        if ($tagId || $startYear) {

            // Look in the table of active tags
            $this->qry->join('ebms_article_tag', 'tag',
                     'tag.article_id = art.article_id');
            $this->qry->condition('tag.active_status', 'A');

            // Looking for a specific tag?
            if ($tagId)
                $this->qry->condition('tag.tag_id', $tagId);

            // Looking for a date range?
            // Do it whether or not we're looking for a specific tag
            if ($startYear) {

                // Search for tags within the modified range
                if ($anyMods)
                    $dateLimits = $this->chkDates(
                            $this->spec['modified-start'],
                            $this->spec['modified-end'],
                            $this->spec['use-modified-date-range']);

                // Or search for tags within the tag-added range
                else
                    $dateLimits = $this->chkDates(
                            $this->spec['tag-added-start'],
                            $this->spec['tag-added-end'],
                            $this->spec['use-tag-date-range']);

                // If we found a valid range
                if ($dateLimits) {
                    $this->qry->condition('tag.tag_dt', $dateLimits[0], '>=');
                    $this->qry->condition('tag.tag_dt', $dateLimits[1], '<');
                }
            }

            // Tags can have topics, but not boards
            if (!empty($this->topics))
              $this->qry->condition("tag.topic_id", $this->topics, 'IN');
        }
    }

    /**
     * Search for requested states.
     *
     * This front end to chkStates figures out what states are requested
     * and passes the right parameters to chkStates to add them to the query.
     */
    function srchStates() {

        // All of these checks are looking for a value other than 0 in
        //  the passed search spec.

        // Passed board manager review
        if ($this->spec['advanced-options']['nci-reviewer-decision'])
            $this->chkStates(array('PassedBMReview'), '=');

        // Passed full text review
        // Once known as "passed literature surveillance committe"
        if ($this->spec['advanced-options']['nci-reviewer-decision'])
            $this->chkStates(array('PassedFullReview'), '=');

        // Has "For Your Information (FYI)" status
        // Cynthia said even inactive states qualify
        if ($this->spec['advanced-options']['fyi-citation'])
            $this->chkStates(array('FYI'), '=', false);

        // "Published", i.e. released from initial review to board managers
        if ($this->spec['advanced-options']['published-to-cite-ms'])
            $this->chkStates(array('Published'), '=');

        // Has ever had a summary topic added
        if ($this->spec['admin-options']['summary-topics-added'])
            $this->chkStates(array('ReadyInitReview'), '=');
    }

    /**
     * Find articles matching passed criteria for searching the
     * ebms_article_state table.
     *
     * This is a generic helper subroutine for srchStates().
     *
     *  @param array $textId        Array of values from the state type
     *                               table that will match rows in the state
     *                               table.
     *  @param string $op           Operator to apply to the state values,
     *                               One of "=", ">", ">=", etc., + "IN".
     *                               If relational operators are used (">"
     *                               etc., they are applied to state sequence
     *                               numbers.
     *  @param bool $activeOnly     True = Only examine active states.
     */
    function chkStates(array $textId, $op, $activeOnly=true) {

        // Relational operators using sequence number for comparison
        static $relationals = array('>', '>=', '<', '<=');

        // Function may be called multiple times.  Use integers to distinguish
        //  table aliases.
        static $tableCount = 0;

        ++$tableCount;
        $dataAlias = "state$tableCount";
        $typeAlias = "stateType$tableCount";

        // Join the state table to the query
        $this->qry->join('ebms_article_state', $dataAlias,
                   "$dataAlias.article_id = art.article_id");

        // Join the type table to the state table
        $this->qry->join('ebms_article_state_type', $typeAlias,
                   "$typeAlias.state_id = $dataAlias.state_id");

        // Conditions depend on the passed operator
        // Later ...
        if (in_array($op, $relationals))
            throw new \Exception(
                "EbmsSearch.chkStates: $op not supported yet - ask for it");

        else if ($op == '=')
            $this->qry->condition("$typeAlias.state_text_id", $textId[0]);
        else if ($op == 'IN')
            $this->qry->condition("$typeAlias.state_text_id", $textId, 'IN');
        else
            throw new \Exception(
                "EbmsSearch.chkStates: $op unknown - internal error");

        // The default is, only active states are checked
        if ($activeOnly)
            $this->qry->condition("$dataAlias.active_status", 'A');

        // If search is topic or board limited (implied by topic)
        // User may have AND'd topics, not OR'd them, but that will be
        //  checked elsewhere, so this is safe.
        if (!empty($this->topics))
            $this->qry->condition("$dataAlias.topic_id", $this->topics, 'IN');
        else if (!empty($this->boards))
            $this->qry->condition("$dataAlias.board_id", $this->boards, 'IN');
    }

    /**
     * Exclude any articles that have a current state we don't want.
     *
     * Currently, this means articles that were rejected because their
     * journal title was on a NOT list, or articles rejected in initial
     * review.
     *
     * Those are both negative conditions in the sense that we normally
     * make those exclusions.  The user must explicitly request that we
     * not make them.
     *
     * XXX UNIMPLEMENTED.  NO CHECK FOR non-rejected ARTICLES ONLY.
     * THERE ARE QUESTIONS ABOUT HOW TO DO IT.
     */
    /*
    XXX - MAY NOT WANT THIS.  WAIT TO HEAR FROM BOB AND DAN
    function srchExcludeStates()
    */
    /**
     * Journal searching.
     */

    function srchJrnl() {
        $jrnlTitle = $this->spec['journal'];
        if ($jrnlTitle) {
            if ($this->chkWildCards($jrnlTitle))
                $this->qry->condition('art.journal_title', $jrnlTitle, 'LIKE');
            else
                $this->qry->condition('art.journal_title', $jrnlTitle);
        }
    }

    /**
     * Published date.  Pubmed stores this as:
     *  <PubDate>
     *   <Year>2012</Year>
     *   <Month>Jan</Month>
     *   <Day>7</Day>
     *  <PubDate>
     *
     * This is stored an indexed in ebms_article as "2012 Jan 7".
     *
     * Not all dates have days.  Some do not have months.
     *
     * Variations for seasons are possible but are not in the search
     * form and are not searched here at this time.
     */
    function srchPubDate() {
        static $monthNames = array(
             '1'=>'Jan', '2'=>'Feb', '3'=>'Mar', '4'=>'Apr',
             '5'=>'May', '6'=>'Jun', '7'=>'Jul', '8'=>'Aug',
             '9'=>'Sep', '10'=>'Oct', '11'=>'Nov', '12'=>'Dec'
         );

        if ($this->spec['publication-year']) {
            $pubdate = $this->spec['publication-year'];

            // Only allow searching on month if year entered
            if ($this->spec['publication-month']) {
                $month = $monthNames[$this->spec['publication-month']];
                $pubdate .= ' ' . $month;
            }

            // Days are not searchable in the search form
            $pubdate .= '%';
            $this->qry->condition('art.published_date', $pubdate, 'LIKE');
        }
    }

    /**
     * Search the import tables.  Finds articles that have:
     *   Specific review cycle, and/or
     *   Have topics added during an import action.
     *
     * Putting these together is an optimization that avoids joining
     * the same tables twice.  Maybe that's a mistake, but it should
     * give higher performance when asking for articles with topics
     * added during a specific review cycle - which might be a common
     * usage.
     *
     * Note: This only works for articles imported in the new system
     *       unless we make a significant change to the conversion process.
     */
    function srchImportActions() {

        $cycleId  = $this->spec['review-cycle'];
        $topAdded = $this->spec['admin-options']['summary-topics-added'];

        if ($cycleId || $topAdded) {
            // We need the import action and disposition tables
            $this->qry->join('ebms_import_action', 'impact',
                         'impact.article_id = art.article_id');
            $this->qry->join('ebms_import_disposition', 'impdisp',
                         'impdisp.disposition_id = impact.disposition_id');

            // If cycle ID requested, also need the batch table
            if ($cycleId) {
                $this->qry->join('ebms_import_batch', 'impbatch',
                     'impbatch.import_batch_id = impact.import_batch_id');
                $this->qry->condition('impbatch.cycle_id', $cycleId);
            }

            // For topicsAdded search, we only need to find that action
            if ($topAdded)
                $this->qry->condition('impdisp.text_id', 'topicAdded');

            // Else we need both new imports and topic added articles
            else
                $this->qry->condition('impdisp.text_id',
                         array('imported','topicAdded'), 'IN');
        }
    }

    /**
     * Information about editorial board member relationships to articles.
     *
     * Only one board member can be selected for one search.
     */
    function srchArticleReviewer() {

        // Find articles which have appeared in a packet for this editorial
        //  board member for his/her review.  The board member may or may not
        //  have actually submitted a response and may or may not be regularly
        //  assigned to review this topic, but the articles passing this filter
        //  all appeared in packets assigned to this reviewer.
        // If the drop_flag was set, the article appeared in the reviewer's
        //  packet but was taken out again.  Don't count this as one of his
        //  articles UNLESS he's already reviewed it (a response exists.)
        //
        /*  This is what we're trying to do
            JOIN ebms_packet_article pcktart
              ON pcktart.article_id = art.article_id
            JOIN ebms_packet_reviewer pcktrvr
              ON pcktrvr.packet_id = pcktart.packet_id
           WHERE pcktrvr.reviewer_id = $memberId
             AND pcktart.drop_flag = 0
              OR EXISTS (
                SELECT ebms_article_review artrevw
                 WHERE artrevw.article_id = pcktart.article_id
                   AND artrevw.packet_id = pcktart.packet_id
                   AND artrevw.reviewer_id = $memberId
                )
       */

        $memberId = $this->spec['reviewer'];
        if ($memberId > 0) {
            // Subquery to include articles dropped from packets if a response
            //  was already received
            $subqry = db_select('ebms_article_review', 'artrevw');
            $subqry->fields('artrevw', array('article_id'));
            $subqry->condition('artrevw.article_id', 'pcktart.article_id');
            $subqry->condition('artrevw.packet_id', 'pcktart.packet_id');
            $subqry->condition('artrevw.reviewer_id', $memberId);

            // Articles in packets assigned to this reviewer
            $this->qry->join('ebms_packet_article', 'pcktart',
                             'pcktart.article_id = art.article_id');
            $this->qry->join('ebms_packet_reviewer', 'pcktrvr',
                             'pcktrvr.packet_id = pcktart.packet_id');
            $this->qry->condition('pcktrvr.reviewer_id', $memberId);

            // But not if article was dropped from packet and was not used
            $this->qry->condition('pcktart.drop_flag', 0);
            $this->qry->condition(
                db_or()
                    ->condition('pcktart.drop_flag', 0)
                    ->exists($subqry)
            );
        }
    }

    /**
     * At least one editorial board response exists with the expected
     * value for the article(s).
     *
     * This is implemented independently of the board reviewer.  If both
     * are specified, we don't require that the specified response came
     * from the specified reviewer, only that someone gave this response.
     *
     * Only one response can be selected for one search.
     */
    function srchReviewerResponse() {
        $response = $this->spec['response'];
        if ($response) {
            $this->qry->join('ebms_article_review', 'artrev',
                             'artrev.article_id = art.article_id');
            $this->qry->join('ebms_review_disposition', 'revdisp',
                             'revdisp.review_id = artrev.review_id');
            $this->qry->condition('revdisp.value_id', $response);
        }
    }

    /**
     * Find articles input within a date range.
     */
    function srchInputDate() {

        if ($this->spec['input-start']['year']) {
            $dateLimits = $this->chkDates(
                    $this->spec['input-start'],
                    $this->spec['input-end'],
                    $this->spec['use-input-date-range']);

            if ($dateLimits) {
                $this->qry->join('ebms_article_state', 'inpstate',
                     'inpstate.article_id = art.article_id');
                $this->qry->join('ebms_article_state_type', 'inptype',
                     'inptype.article_state_id = inpstate.article_state_id');

                // Looking for input states in the date range
                $this->qry->condition('inptype.state_text_id', 'Imported');
                $this->qry->condition('inpstate.status_dt',
                                       $dateLimits[0], '>=');
                $this->qry->condition('inpstate.status_dt',
                                       $dateLimits[1], '<');

                // Standard qualifiers
                if (!empty($this->topics))
                    $this->qry->condition("inpstate.topic_id",
                                           $this->topics, 'IN');
                else if (!empty($this->boards))
                    $this->qry->condition("inpstate.board_id",
                                           $this->boards, 'IN');
            }
        }
    }

    /**
     * Find articles with any modification within a date range.
     *
     * XXX - Since users said ANY modification we're not excluding
     *       inactive states.  Ask to be sure if that's right.
     */
    function srchModifiedDate() {

        if ($this->spec['modified-start']['year']) {
            $dateLimits = $this->chkDates(
                    $this->spec['modified-start'],
                    $this->spec['modified-end'],
                    $this->spec['use-modified-date-range']);

            if ($dateLimits) {
                $this->qry->join('ebms_article_state', 'modstate',
                                 'modstate.article_id = art.article_id');

                // Any mod will do within the date range, don't care what
                $this->qry->condition('modstate.status_dt',
                                       $dateLimits[0], '>=');
                $this->qry->condition('modstate.status_dt',
                                       $dateLimits[1], '<');

                if (!empty($this->topics))
                    $this->qry->condition("modstate.topic_id",
                                           $this->topics, 'IN');
                else if (!empty($this->boards))
                    $this->qry->condition("modstate.board_id",
                                           $this->boards, 'IN');

                // A tag entry is also considered a "modification"
                $this->srchTags(true);
            }
        }
    }

    /**
     * Find articles that had more than one summary topic added.
     *
     * XXX - The approach taken here only works for articles added
     * after our conversion.
     ***********
    function srchTopicsAdded() {

        if ($this->spec['admin-options']['summary-topics-added']) {

            // Inner subquery to find articles with > 1 summary topic added
            $inQry = db_select('ebms_article_state', 'topstate');
            $inQry->fields('topstate', array('article_id'));
            $inQry->join('ebms_article_state_type', 'toptype',
                          'toptype.state_id = topstate.state_id');

            // XXX LEFT OFF HERE - $inQry->condition

            $subqry =
              'SELECT s.article_id, count(*)
                FROM ebms_article_state s
                JOIN ebms_article_state_type t
                  ON s.state_id = t.state_id
               WHERE t.state_text_id = 'ReadyInitReview'
               GROUP BY s.article_id
               HAVING count(*) > 1';
        }
    }
    *******/


    /**
     * Convert start and end dates to SQL standard dates for searching.
     * There are tricks and twists here to provide usable defaults when
     * a user does not specify a complete date.
     *
     *  @param array $startDate     Three integers for year, month, day.
     *  @param array $endDate       Three more.
     *  @param bool $endSpecified   True = $endDate is real.  Else ignore it.
     *
     *  @return array               Tuple of SQL standard dates, YYYY-MM-DD.
     *                              Return NULL if no valid date passed or
     *                              end date is <= start date.
     *
     * Notes:
     *  Values of 0 mean there was no input for that field.
     *  Years are origin 1899.
     *    Year 0 = no input.
     *    Year 1 = 1900
     *    Year 113 = 2012
     *      etc.
     *  We provide defaults in many cases where values = 0.
     *  As I said, there tricks and twists here.
     */
    function chkDates($startDate, $endDate, $endSpecified) {

        // Convert everything to integers, original and modified
        $sYear  = $startYear  = intval($startDate['year']);
        $sMonth = $startMonth = intval($startDate['month']);
        $sDay   = $startDay   = intval($startDate['day']);
        $eYear  = $endYear    = intval($endDate['year']);
        $eMonth = $endMonth   = intval($endDate['month']);
        $eDay   = $endDay     = intval($endDate['day']);

        // Start year is absolutely required
        if ($sYear == 0)
            return null;

        // Set year relative to origin
        $startYear = $sYear + 1899;
        if ($eYear > 0)
            $endYear = $eYear + 1899;

        // No start month means start in Jan 1.
        if ($sMonth == 0) {
            $startMonth = 1;
            $startDay   = 1;
        }
        else if ($sDay == 0)
            $startDay = 1;

        // We've got the first return value
        $startSqlDate = sprintf("%04d-%02d-%02d",
                                 $startYear, $startMonth, $startDay);

        // Handle specified end date
        if ($endSpecified && $eYear > 0) {

            // Set the end dates from the passed data in the same way
            if ($eMonth == 0) {
                $endMonth = 1;
                $endDay   = 1;
            }
            else if ($eDay == 0)
                $endDay = 1;
        }

        // Else end dates were not specified, compute defaults
        else {
            $endYear = $startYear;
            if ($sMonth == 0) {
                // Searching for entire year
                $endYear  = $endYear + 1;
                $endMonth = 1;
                $endDay   = 1;
            }
            else if ($sDay == 0) {
                // Searching to end of start month
                $endMonth = $startMonth + 1;
                if ($eMonth > 12) {
                    $endYear  = $endYear + 1;
                    $endMonth = 1;
                };
                $endDay = 1;
            }
            else {
                // End is exactly 1 day after start, searching one day
                // Let PHP do the month/day/leapyear calculations for us
                $phpStartDate = new DateTime($startSqlDate);
                $phpEndDate   = $phpStartDate->add(new DateInterval("P1D"));
                $endYear      = $phpEndDate->format('Y');
                $endMonth     = $phpEndDate->format('m');
                $endDay       = $phpEndDate->format('j');
            }
        }

        // Ending date, as a return value
        $endSqlDate = sprintf("%04d-%02d-%02d",
                               $endYear, $endMonth, $endDay);

        // If the end date is <= start date, there's no point going on
        if (strcmp($endSqlDate, $startSqlDate) <= 0)
            return null;

        return(array($startSqlDate, $endSqlDate));
    }

    /**
     * Sorting results.
     *
     * Note: If we add an option to not sort it will speed up searches
     * that are otherwise limited to 500 or whatever - but will also
     * get a random 500 hits.
     */
    function srchOrder() {

        $sortField = $this->spec['sort-by'];

        switch ($sortField) {
            case 'PM ID#':
                // This could go awry with non-Pubmed data, but it won't
                //  do serious harm
                $this->qry->orderBy('art.source_id');
                break;
            case 'Title':
                $this->qry->orderBy('art.article_title');
                break;
            case 'Journal':
                $this->qry->orderBy('art.jrnl_title');
                break;
            case 'Author':
                // XXX TO DO
            case 'Publication Date':
                // XXX TO DO
            case 'CMS_ID':
            default:
                $this->qry->orderBy('art.article_id');
                break;
        }
    }

    /**
     * Check for presence of wildcards in a search value.
     * Supporting '%'.  May add '_' if requested.
     *
     *  @param string $srchString   User entered string.
     *
     *  @return boolean             True  = at least one wildcard found.
     */
    function chkWildCards($srchString) {
        // Distinguishes false from zero
        if (strpos($srchString, '%') !== false)
            return true;
        return false;
    }
}
