<?PHP

// $Id$

namespace Ebms;

// require_once('logw.inc'); // XXX DEBUG

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
    $srch->srchStates();
    $srch->srchTopicsBoards();
    $srch->srchAuthors();
    $srch->srchArticleTitle();
    $srch->srchFullTextRtrvd();
    $srch->srchJrnl();
    $srch->srchPubDate();
    $srch->srchImportActions();
    $srch->srchCycle();
    $srch->srchArticleReviewer();
    $srch->srchReviewerResponse();
    $srch->srchComments();
    $srch->srchTags();
    $srch->srchInputDate();
    $srch->srchModifiedDate();
    $srch->srchDecision();

    // Sort them
    $srch->srchOrder();

    // XXX - Should we limit results?
    // $srch->qry->range(0,500);

// DEBUG
// $jspec = print_r($srch->spec, true);
// logw("Full decoded json spec:\n$jspec\n");
// logw("SQL for query:\n{$srch->qry}\n");


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
        $boards,        // The board IDs for this search, if any.
        $needBoards,    // True=Need to qualify results by board.
        $earlyStates,   // These are the statuses the librarians control.
        $laterStates,   // Everything after published.
        $stateMap,      // Maps state_text_id strings to state_id integers.
        $fyi;           // FYI gets special handling (doesn't have to be
                        // in an active status row to count).

    function __construct($json) {
        // Convert any utf-8 to ASCII using the same algorithm used
        //  when we constructed index strings.
        $json = Util::utf8ToAscii($json);

        // Save the search spec
        $this->json = $json;
        $this->spec = json_decode($json, true);

        // Topics and boards not yet analyzed and collected
        $this->topics = array();
        $this->boards = array();

        // Create the base query
        $this->qry = db_select('ebms_article', 'art');
        $this->qry->fields('art', array('article_id'));
        $this->qry->distinct();

        // Gather up topics
        // All topics are in the json, vals > 0 are only ones specified
        // Moved this into the constructor because more than one method
        // relies on $this->topics and $this->boards and we don't want
        // to depend on the sequence of the method invocation to ensure
        // that the population of the arrays is done.
        foreach ($this->spec['topics'] as $key => $val) {
            if ($val)
                $this->topics[] = $key;
        }

        // Board specs only exist in the json if specified by user
        // This code doesn't search boards, it just creates a list of them
        foreach ($this->spec['boards'] as $key => $val)
            $this->boards[] = $val;

        // Boards may or may not be needed
        $this->needBoards = $this->needBoardSrch();

        // Load the map of state_text_id values to state_id integers.
        $results = db_select('ebms_article_state_type', 't')
            ->fields('t', array('state_id', 'state_text_id'))
            ->execute();
        $this->stateMap = array();
        foreach ($results as $result)
            $this->stateMap[$result->state_text_id] = $result->state_id;
        $this->fyi = $this->stateMap['FYI'];

        // Collect the set of states which are acceptable for inclusion
        // in the result set.  By default the users only want to see
        // articles which have the Published state, but sometimes they'll
        // relax the criteria to also include articles which were rejected
        // (by the librarian) or excluded (by having been published in a
        // journal from which the board in question doesn't generally
        // want to see any of the articles) or which just haven't gotten
        // around to the initial review step.  If the user sets the flag
        // for "Include Unpublished Articles" we don't exclude anything
        // in the early filtering step.  In that case we leave the
        // earlyStates array empty.
        if ($this->spec['unpublished'])
            $this->earlyStates = array();
        else {
            $this->earlyStates = array($this->stateMap['Published']);
            if ($this->spec['not-listed'])
                $this->earlyStates[] = $this->stateMap['RejectJournalTitle'];
            if ($this->spec['rejected'])
                $this->earlyStates[] = $this->stateMap['RejectInitReview'];
        }

        // Now collect the states which reflect activity after the
        // 'Published' state.  Unlike the early states, where any
        // one will do to let an article into the set of result
        // candidates, each of these is checked and required
        // separately.  The two decision fields checked below carry
        // the state text ID for a specific accept or reject decision
        // state.  The third field is just a boolean represented by
        // by a checkbox on the UI.  Note that the form for the
        // search request has the state_text_id strings as the
        // values for the "Yes" and "No" radio buttons for the
        // first two fields here, so we don't have to know what
        // those values are.
        $this->laterStates = array();
        if ($this->spec['nci-reviewer-decision'])
            $this->laterStates[] =
                $this->stateMap[$this->spec['nci-reviewer-decision']];
        if ($this->spec['committee-decision'])
            $this->laterStates[] =
                $this->stateMap[$this->spec['committee-decision']];
        if ($this->spec['fyi-citation'])
            $this->laterStates[] = $this->fyi;
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
     * The default behavior returns all articles which have reached the
     * Published state.  When the user identifies states later than Published,
     * we narrow the search to include only articles which have all of these
     * states, and we don't have to bother checking for the Published state
     * (because the presence of any of the later states guarantees that the
     * Published state will be present as well).
     *
     * If no states later than Published are specified, but the user
     * checks any of the boxes to include articles which have states
     * earlier than Published, even if they don't also have the Published
     * state, we broaden the net to add those articles, but still including
     * the ones that do have the Published state.  In other words,
     * specifying later states narrows the search, and specifying only
     * earlier states broadens the search.
     *
     * The states we're looking for are collected in the constructor.
     */
    function srchStates() {

        // Use a counter to distinguish table aliases from each other.
        $delta = 1;

        // Look for the later states first.  The reason for doing it this
        // way is that the downstream (that is, after Published) status
        // values guarantee that the Published state is also present
        // (which means we don't have to bother checking the early
        // states if we filter on the later ones).
        if (!empty($this->laterStates)) {

            // Each state mentioned must be present.
            foreach ($this->laterStates as $stateId) {

                // Match up the state with the topics named.
                if (!empty($this->topics)) {

                    // Handle the cases where a single join will suffice.
                    $logic = $this->spec['topic-logic'];
                    if ($logic == 'OR' || count($this->topics) == 1) {
                        $alias = 'state_alias_' . $delta++;
                        $this->qry->join('ebms_article_state', $alias,
                            "$alias.article_id = art.article_id");
                        $this->qry->condition("$alias.state_id", $stateId);
                        if ($stateId != $this->fyi)
                            $this->qry->condition("$alias.active_status", 'A');
                        if (count($this->topics) == 1)
                            $this->qry->condition("$alias.topic_id",
                                $this->topics[0]);
                        else
                            $this->qry->condition("$alias.topic_id",
                                $this->topics, 'IN');
                    }

                    // We're AND-ing more than one topic; separate joins.
                    else {
                        foreach ($this->topics as $topicId) {
                            $alias = 'state_alias_' . $delta++;
                            $this->qry->join('ebms_article_state', $alias,
                                "$alias.article_id = art.article_id");
                            $this->qry->condition("$alias.state_id", $stateId);
                            $this->qry->condition("$alias.topic_id", $topicId);
                            if ($stateId != $this->fyi)
                                $this->qry->condition("$alias.active_status",
                                    'A');
                        }
                    }
                }

                // Need to add board qualifications?
                if ($this->needBoards) {

                    // Boards are always ANDed.
                    foreach ($this->boards as $boardId) {
                        $alias = 'state_alias_' . $delta++;
                        $this->qry->join('ebms_article_state', $alias,
                            "$alias.article_id = art.article_id");
                        $this->qry->condition("$alias.state_id", $stateId);
                        $this->qry->condition("$alias.board_id", $boardId);
                        if ($stateId != $this->fyi)
                            $this->qry->condition("$alias.active_status", 'A');
                    }
                }

                // Simple case: no topics or boards specified.
                else {
                    $alias = 'state_alias_' . $delta++;
                    $this->qry->join('ebms_article_state', $alias,
                        "$alias.article_id = art.article_id");
                    $this->qry->condition("$alias.state_id", $stateId);
                    if ($stateId != $this->fyi)
                        $this->qry->condition("$alias.active_status", 'A');
                }
            }
        }

        // Only have to check for Published and earlier states if no
        // later states were asked for.
        elseif (!empty($this->earlyStates)) {

            // Decide whether the state condition is an IN or =
            $states = $this->earlyStates;
            if (count($this->earlyStates) > 1) {
                $states = $states[0];
                $op = '=';
            }
            else
                $op = 'IN';

            // Again, we try the most granular (topics) first.
            if (!empty($this->topics)) {

                // Can we get away with just a single join?
                $logic = $this->spec['topic-logic'];
                if ($logic == 'OR' || count($this->topics) == 1) {
                    $alias = 'state_alias_' . $delta++;
                    $this->qry->join('ebms_article_state', $alias,
                        "$alias.article_id = art.article_id");
                    $this->qry->condition("$alias.state_id", $states, $op);
                    if (count($this->topics) == 1)
                        $this->qry->condition("$alias.topic_id",
                            $this->topics[0]);
                    else
                        $this->qry->condition("$alias.topic_id",
                            $this->topics, 'IN');
                    $this->qry->condition("$alias.active_status", 'A');
                }

                // User wants multiple topics all to be represented
                else {
                    foreach ($this->topics as $topicId) {
                        $alias = 'state_alias_' . $delta++;
                        $this->qry->join('ebms_article_state', $alias,
                            "$alias.article_id = art.article_id");
                        $this->qry->condition("$alias.state_id", $states, $op);
                        $this->qry->condition("$alias.topic_id", $topicId);
                        $this->qry->condition("$alias.active_status", 'A');
                    }
                }
            }

            // Boards needed?
            if ($this->needBoards) {

                // Again, boards are always checked in separate joins.
                foreach ($this->boards as $boardId) {
                    $alias = 'state_alias_' . $delta++;
                    $this->qry->join('ebms_article_state', $alias,
                        "$alias.article_id = art.article_id");
                    $this->qry->condition("$alias.state_id", $states, $op);
                    $this->qry->condition("$alias.board_id", $boardId);
                    $this->qry->condition("$alias.active_status", 'A');
                }
            }

            // Test for the state(s) without regard to topic or board.
            else {
                $alias = 'state_alias_' . $delta++;
                $this->qry->join('ebms_article_state', $alias,
                    "$alias.article_id = art.article_id");
                $this->qry->condition("$alias.state_id", $states, $op);
                $this->qry->condition("$alias.active_status", 'A');
            }
        }
    }

    /**
     * Search for articles with specified topics assigned or boards
     * assigned.  Only need boards if no topics assigned.
     *
     * We rarely have to do anything here, because all of the state
     * checks take care of looking for boards and topics requested,
     * and the same is true for a specified cycle.
     *
     * Note that we don't have to check the active_status column
     * of the state rows, because the only way for a status row
     * to become inactive is because another row for the same
     * topic (and, by implication, board) is added at an earlier
     * processing sequence, and that newer row will be active.
     * Since the filtering we're doing here doesn't care what
     * state the rows represent, that new row will satisfy the
     * query (and will be active).
     */
    function srchTopicsBoards() {

        // Make sure our table aliases are distinct.
        $delta = 1;

        // Make sure this hasn't been taken care of already.
        if (empty($this->earlyStates) && empty($this->laterStates)) {
            if (!$this->spec['review-cycle']) {

                // Check topics first.
                if (!empty($this->topics)) {

                    // Can we handle this is with a single join?
                    $logic = $this->spec['topic-logic'];
                    if ($logic == 'OR' || count($this->topics) == 1) {
                        $alias = 'topic_alias_' . $delta++;
                        $this->qry->join('ebms_article_state', $alias,
                            "$alias.article_id = art.article_id");
                        if (count($this->topics) == 1)
                            $this->qry->condition("$alias.topic_id",
                                $this->topics[0]);
                        else
                            $this->qry->condition("$alias.topic_id",
                                $this->topics, 'IN');
                    }

                    // User wants each topic to be present in each article.
                    else {
                        foreach ($this->topics as $topicId) {
                            $alias = 'topic_alias_' . $delta++;
                            $this->qry->join('ebms_article_state', $alias,
                                "$alias.article_id = art.article_id");
                            $this->qry->condition("$alias.topic_id", $topicId);
                        }
                    }
                }

                // No topics specified.  Any boards?
                if ($this->needBoards) {

                    // Separate join required for each board.
                    foreach ($this->boards as $boardId) {
                        $alias = 'topic_alias_' . $delta++;
                        $this->qry->join('ebms_article_state', $alias,
                            "$alias.article_id = art.article_id");
                        $this->qry->condition("$alias.board_id", $boardId);
                    }
                }
            }
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
     * Filter on whether or not the full text has been retrieved and stored
     * in the EBMS.  Looking for articles with full text retrieved will not
     * pick up any articles converted from the legacy system (except for
     * any that the users have gone back and plugged in after the fact).
     * Similarly, if the users check "NO" for this field, they'll get
     * all the legacy articles, regardless of whether the full text was
     * obtained in the old system.  There's no provision in the user
     * interface for finding only articles for which the full text cannot
     * be obtained.
     */
    function srchFullTextRtrvd() {
        switch ($this->spec['full-text-retrieved']) {
            case 'yes':
                $this->qry->isNotNull('art.full_text_id');
                break;
            case 'no':
                $this->qry->isNull('art.full_text_id');
                break;
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
     */
    function srchComments() {

        $comment = $this->spec['comments'];

        // See if user wants to search for comment-added date
        $startYear = $this->spec['comment-added-start']['year'];

        // But if we're searching any mod date, don't need to repeat here
        if ($this->spec['modified-start']['year'])
            $startYear = null;

        // If we have either of them, we need to join states and comments
        // This could be optimized if modified-start also, but it's not
        //  worth the trouble.  It should be a rare search.
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
     */
    function srchTags() {

        $tagId     = $this->spec['tag'];
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
     * Journal searching.
     */
    function srchJrnl() {
        $jrnlTitle = $this->spec['journal'];
        if ($jrnlTitle) {
            if ($this->chkWildCards($jrnlTitle))
                $this->qry->condition('art.jrnl_title', $jrnlTitle, 'LIKE');
            else
                $this->qry->condition('art.jrnl_title', $jrnlTitle);
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
     *
     * 2013-01-16 (TIRs 2197 and 2300).  In spite of the users' protests
     * that they never expected us to support searching by cycle in the
     * legacy data, they filed multiple TIRs complaining that searching
     * by cycle was broken because they didn't see any search results
     * for the legacy data.  So we have created and populated a new
     * table for the purpose of searching and filtering by review cycle.
     * Search by review cycle has been split off into its own method,
     * to use that table, so the srchImportActions method now handles
     * only searching for articles which had topics added during import
     * jobs.
     */
    function srchImportActions() {

        $topAdded = $this->spec['summary-topics-added'];

        if ($topAdded) {

            // We need the import action and disposition tables
            $this->qry->join('ebms_import_action', 'impact',
                         'impact.article_id = art.article_id');
            $this->qry->join('ebms_import_disposition', 'impdisp',
                         'impdisp.disposition_id = impact.disposition_id');

            // Narrow down to the specific import action we want.
            $this->qry->condition('impdisp.text_id', 'topicAdded');
        }
    }

    /**
     * Restrict the results to those which have been assigned to
     * a specific cycle.  Using the new ebms_article_topic table,
     * created for just this purpose.
     */
    function srchCycle() {

        // Don't do anything unless a specific cycle was chosen.
        $cycleId  = $this->spec['review-cycle'];
        if ($cycleId) {

            // Are we narrowing by topics?
            if (count($this->topics) > 0) {
                $logic = $this->spec['topic-logic'];

                // Handle the cases where a single join will suffice.
                if ($logic == 'OR' || count($this->topics) == 1) {
                    $this->qry->join('ebms_article_topic', 'art_top_cycle',
                        'art_top_cycle.article_id = art.article_id');
                    $this->qry->condition('art_top_cycle.cycle_id', $cycleId);
                    if (count($this->topics) == 1)
                        $this->qry->condition('art_top_cycle.topic_id',
                            $this->topics[0]);
                    else
                        $this->qry->condition('art_top_cycle.topic_id',
                            $this->topics, 'IN');
                }

                // We're AND-ing more than one topic; separate join for each.
                else {
                    $i = 1;
                    foreach ($this->topics as $topicId) {
                        $alias = "cycle_$i";
                        $this->qry->join('ebms_article_topic', $alias,
                            "$alias.article_id = art.article_id");
                        $this->qry->condition("$alias.cycle_id", $cycleId);
                        $this->qry->condition("$alias.topic_id", $topicId);
                        ++$i;
                    }
                }
            }

            // How about boards?  (Boards always use AND, never OR.)
            elseif (count($this->boards) > 0) {
                $i = 1;
                foreach ($this->boards as $boardId) {
                    $alias1 = "cycle_$i";
                    $alias2 = "board_$i";
                    $this->qry->join('ebms_article_topic', $alias1,
                        "$alias1.article_id = art.article_id");
                    $this->qry->join('ebms_topic', $alias2,
                        "$alias2.topic_id = $alias1.topic_id");
                    $this->qry->condition("$alias1.cycle_id", $cycleId);
                    $this->qry->condition("$alias2.board_id", $boardId);
                }
            }

            // Easy case: no topics or boards specified.
            else {
                $this->qry->join('ebms_article_topic', 'art_top_cycle',
                    'art_top_cycle.article_id = art.article_id');
                $this->qry->condition('art_top_cycle.cycle_id', $cycleId);
            }
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
                // We already specified the article table
                $this->qry->condition('art.import_date', $dateLimits[0], '>=');
                $this->qry->condition('art.import_date', $dateLimits[1], '<');
            }
        }
    }


    /**
     * Find articles with any modification within a date range.
     *
     * Note: This finds ANY modifications, including inactive states and tags.
     *       This seems to me to be the right thing to do since the users
     *       asked for any mods, and a modification to a state or tag that
     *       is now inactive had to be active when the mod was made.
     *
     *       But, we can change it if needed.
     */
    function srchModifiedDate() {

        if ($this->spec['modified-start']['year']) {
            $dateLimits = $this->chkDates(
                    $this->spec['modified-start'],
                    $this->spec['modified-end'],
                    $this->spec['use-mod-date-range']);

            if ($dateLimits) {
                // Build a subquery to find articles that have rows
                //   for states, state comments, tags, tag comments

                // 1. State changes
                $stateQry = db_select('ebms_article_state', 'mstat');
                $stateQry->addField('mstat', 'article_id');
                $stateQry->condition('mstat.status_dt', $dateLimits[0], '>=');
                $stateQry->condition('mstat.status_dt', $dateLimits[1], '<');

                // Have to qualify by topic or board if needed
                $this->modDateQuals($stateQry, 'mstat.topic_id',
                                    'mstat.board_id');

                // 2. State comment changes
                $scmtQry = db_select('ebms_article_state_comment', 'mcmt');
                $scmtQry->join('ebms_article_state', 'mst',
                               'mcmt.article_state_id = mst.article_state_id');
                $scmtQry->addField('mst', 'article_id');
                $scmtQry->condition('mcmt.comment_dt', $dateLimits[0], '>=');
                $scmtQry->condition('mcmt.comment_dt', $dateLimits[1], '<');
                $this->modDateQuals($scmtQry, 'mst.topic_id', 'mst.board_id');

                // 3. Tag additions
                $tagQry = db_select('ebms_article_tag', 'mtag');
                $tagQry->addField('mtag', 'article_id');
                $tagQry->condition('mtag.tag_dt', $dateLimits[0], '>=');
                $tagQry->condition('mtag.tag_dt', $dateLimits[1], '<');
                $this->modDateQuals($tagQry, 'mtag.topic_id');

                // 4. Tag comment additions
                $tcmtQry = db_select('ebms_article_tag_comment', 'tcmt');
                $tcmtQry->join('ebms_article_tag', 'ctag',
                               'tcmt.article_tag_id = ctag.article_tag_id');
                $tcmtQry->addField('ctag', 'article_id');
                $tcmtQry->condition('tcmt.comment_dt', $dateLimits[0], '>=');
                $tcmtQry->condition('tcmt.comment_dt', $dateLimits[1], '<');
                $this->modDateQuals($tcmtQry, 'ctag.topic_id');

                // We want the union of these queries,
                //  i.e. any article_id produced by any one of the queries
                $stateQry->union($scmtQry, 'UNION DISTINCT');
                $tagQry->union($tcmtQry, 'UNION DISTINCT');
                $stateQry->union($tagQry, 'UNION DISTINCT');

                // Only pick article_ids from the rest of the search that
                //  are found by these queries.
                $this->qry->condition('art.article_id', $stateQry, 'IN');
            }
        }
    }

    /**
     * Restrict modified date queries by topic and board.  If there are
     * multiple topics or boards, we always OR them, never AND.  User asked
     * for ANY modifications, which implies OR.
     *
     * Subroutine of srchModifiedDates().
     *
     *  @param object ref $qry          Reference to query object to update.
     *  @param string $topicDateField   Query alias for finding topic dates.
     *  @param string $boardDateField   Same for board, null if none.
     *                                  (Tag records are not board qualified.)
     */
    function modDateQuals(&$qry, $topicDateField, $boardDateField=null) {

        $topicCount = count($this->topics);
        if ($topicCount > 0) {
            // Apply optimization for one topic vs. multiple
            if ($topicCount > 1)
                $qry->condition($topicDateField, $this->topics, 'IN');
            else
                $qry->condition($topicDateField, $this->topics[0], '=');
        }
        else if (!is_null($boardDateField)) {
            $boardCount = count($this->boards);
            if ($topicCount > 0) {
                if ($boardCount > 1)
                    $qry->condition($boardDateField, $this->boards, 'IN');
                else
                    $qry->condition($boardDateField, $this->boards[0], '=');
            }
        }
    }

    /**
     * Search for articles which have a specific decision made by the
     * editorial boards, toward the end of the processing of articles.
     */
    function srchDecision() {

        // Don't do anything unless the user specified a decision.
        if ($this->spec['decision']) {

            // Join with the state table.
            $this->qry->join('ebms_article_state', 'decision_state',
                'decision_state.article_id = art.article_id');

            // Don't bother with states that are no longer relevant.
            $this->qry->condition('decision_state.active_status', 'A');

            // Narrow down to the decision the user specified.
            $this->qry->join('ebms_article_board_decision', 'state_decision',
                'state_decision.article_state_id = ' .
                'decision_state.article_state_id');
            $this->qry->condition('state_decision.decision_value_id',
                $this->spec['decision']);
        }
    }

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
        //     $startYear = $sYear + 1899;
        // if ($eYear > 0)
        //     $endYear = $eYear + 1899;

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
                $phpStartDate = new \DateTime($startSqlDate);
                $phpEndDate   = $phpStartDate->add(new \DateInterval("P1D"));
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
                $this->qry->orderBy('CAST(art.source_id AS UNSIGNED)', 'DESC');
                break;
            case 'Title':
                $this->qry->orderBy('art.article_title');
                break;
            case 'Journal':
                $this->qry->orderBy('art.jrnl_title');
                break;
            case 'Author':
                // Use first author name or collective name
                $this->qry->leftJoin('ebms_article_author_cite',
                    'sort_auth_cite',
                    'art.article_id = sort_auth_cite.article_id
                                  AND sort_auth_cite.cite_order = 1');
                $this->qry->leftJoin('ebms_article_author', 'sort_auth',
                    'sort_auth.author_id = sort_auth_cite.author_id');
                $this->qry->orderBy('sort_auth.last_name');
                $this->qry->orderBy('sort_auth.initials');
                $this->qry->orderBy('sort_auth.collective_name');
                break;
            case 'Publication Date':
                // XXX TO DO
            case 'CMS_ID':
            default:
                $this->qry->orderBy('art.article_id', 'DESC');
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

    /**
     * Check to see if we need to consider boards in a search.
     * We don't if no board was specified or if the single selected
     * board is automatically implied automatically implied by the topic(s).
     *
     * ASSUMPTIONS:
     *   $this->boards and $this->topics have already been set.
     *
     *  @return boolean    True = Must check boards.  Else False.
     */
    function needBoardSrch() {

        if (count($this->boards) == 0)
            // No boards to check
            return False;

        // At least one board found
        if (count($this->topics) == 0)
            // No topics to imply boards.  Must check the specified board(s)
            return True;

        // At least one board + one topic found
        if (count($this->boards) == 1)
            // Checking the topic(s) implies the one board
            return False;

        // Multiple boards specified.  Assume the user really needed them.
        return True;
    }
}
