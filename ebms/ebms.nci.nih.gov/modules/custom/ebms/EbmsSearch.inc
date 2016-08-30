<?PHP

namespace Ebms;

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
    // Some of the later criteria filters depend on work done in previous
    //  filters.  Some will check the results of earlier filters and short
    //  circuit tests that are already done as side effects of earlier
    //  filters.
    // Do not change the order of filters without considering this issue.
    $srch->srchIDs();
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
    $srch->srchOnAgenda();
    $srch->srchDecision();

    // Sort them
    $srch->srchOrder();

    // XXX - Should we limit results to speed searching?
    // Users preferred unrestricted searching, accepting the performance
    //  penalty for difficult cases.
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
        $delta,         // Used to create unique table aliases
        $stateFilters,  // Boolean: did we use states in the query?
        $topics,        // The topic IDs for this search, if any.
        $boards,        // The board IDs for this search, if any.
        $needBoards,    // True=Need to qualify results by board.
        $earlyStates,   // These are the statuses the librarians control.
        $laterStates,   // Everything after published.
        $stateMap,      // Maps state_text_id strings to state_id integers.
        $fyi;           // FYI gets special handling (doesn't have to be
                        // in an active status row to count).

    function __construct($json) {

        // Make sure our table aliases are distinct.
        $this->delta = 1;

        // Remember if we've done filtering by state.
        $this->stateFilters = false;

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
        if (!empty($this->spec['unpublished']))
            $this->earlyStates = array();
        else {
            $this->earlyStates = array($this->stateMap['Published']);
            if (!empty($this->spec['not-listed']))
                $this->earlyStates[] = $this->stateMap['RejectJournalTitle'];
            if (!empty($this->spec['rejected']))
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
        if (!empty($this->spec['nci-reviewer-decision']))
            $this->laterStates[] =
                $this->stateMap[$this->spec['nci-reviewer-decision']];
        if (!empty($this->spec['committee-decision']))
            $this->laterStates[] =
                $this->stateMap[$this->spec['committee-decision']];
        if (!empty($this->spec['fyi-citation']))
            $this->laterStates[] = $this->fyi;

        // If any "OnAgenda" criteria are specified, ebms_article_state
        //  rows must also match the OnAgenda article state
        if (!empty($this->spec['meeting-category']) ||
            !empty($this->spec['meeting-start']) &&
            !empty($this->spec['meeting-start']['date']) ||
            !empty($this->spec['meeting-end']) &&
            !empty($this->spec['meeting-end']['date'])) {
            $this->laterStates[] = $this->stateMap['OnAgenda'];
        }
    }

    /**
     * Search unique IDs.
     */
    function srchIDs() {

        // Ignore conflicts.  We may change that someday.
        $pmid = trim($this->spec['pmid']);
        $ebms_id = '';
        if (!empty($this->spec['cms-id']))
            $ebms_id = trim($this->spec['cms-id']);
        if ($pmid) {
            $this->qry->condition('art.source', 'Pubmed');
            $this->qry->condition('art.source_id', $pmid);
        }
        if ($ebms_id)
            $this->qry->condition('art.article_id', $ebms_id);
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
     * state, we broaden the net to add those articles, but still include
     * the ones that do have the Published state.  In other words,
     * specifying later states narrows the search, and specifying only
     * earlier states broadens the search.
     *
     * The states we're looking for are collected in the constructor.
     *
     * 2014-08-01 (OCEEBMS-203): we've added options for filtering
     * down to just articles whose current states are early states.
     * See details at the top of this method.
     */
    function srchStates() {

        // Librarians can now limit the search results to include only
        //  articles which have been rejected at an early stage, either
        //  by the NOT list, or by the librarian initial review, or
        //  articles that have not yet been reviewed.
        if (!empty($this->spec['only-not-listed']))
            $this->matchEarlyStates('RejectJournalTitle');
        elseif (!empty($this->spec['only-rejected']))
            $this->matchEarlyStates('RejectInitReview');
        elseif (!empty($this->spec['only-unpublished']))
            $this->matchEarlyStates();

        // If the matchEarlyStates() method set stateFilters, we're done here.
        if ($this->stateFilters)
            return;

        // Look for the later states first.  The reason for doing it this
        // way is that the downstream (that is, after Published) status
        // values guarantee that the Published state is also present
        // (which means we don't have to bother checking the early
        // states if we filter on the later ones).
        if (!empty($this->laterStates)) {

            // Each state mentioned must be present.
            foreach ($this->laterStates as $stateId) {
                $stateFilter = $stateId == $this->fyi ? 'any' : 'active';
                $this->matchStates($stateId, $stateFilter);
            }
        }

        // Only have to check for Published and earlier states if no
        // later states were asked for.
        elseif (!empty($this->earlyStates)) {

            // Optimize for the case of a single state.
            $states = $this->earlyStates;
            if (count($this->earlyStates) == 1)
                $states = $states[0];

            // For these we check all the states at once.
            $this->matchStates($states, 'active');
        }
    }

    /**
     * New method to support OCEEBMS-203.
     *
     * Called by srchStates().
     *
     * The librarians can now search for only articles which have
     * early states. If a state name is passed, we match only
     * articles with that state. Otherwise, we match any state
     * which is earlier than 'Published'.  If topics or boards
     * were selected, we handle narrowing by those criteria, too.
     *
     * The principal difference between what we're doing here
     * and state filtering we do elsewhere, is that we're only
     * accepting article states which are the current state for
     * the article.
     */
    function matchEarlyStates($state_name='') {

        // Are we looking for one state or multiple?
        if ($state_name) {
            $states = $this->stateMap[$state_name];
        }
        else {
            $pub_sequence = db_select('ebms_article_state_type', 't')
                ->fields('t', array('sequence'))
                ->condition('t.state_text_id', 'Published')
                ->execute()
                ->fetchField();
            $states = db_select('ebms_article_state_type', 't')
                ->fields('t', array('state_id'))
                ->condition('t.sequence', $pub_sequence, '<')
                ->execute()
                ->fetchCol();
        }

        // Add the state condition to the query.
        $this->matchStates($states, 'current');
    }

    /**
     * Common code to check for specified article states. Called
     * separately for later (post-published) states, where all of the
     * states specified must be present, earlier (pre-published) states,
     * where the presence any of the states specified cause an article
     * to be included, and a special set of new searches, which find
     * articles with an early state as the current (not just active)
     * state.
     *
     *  @param  mixed  $states        Either an array of state IDs, or
     *                                an integer for a single state ID
     *
     *  @param  string $state_filter  One of three values:
     *                                 'current': only match current states
     *                                 'active':  match any state row
     *                                            which is still valid
     *                                 'any':     no state status filtering
     *
     *  @return  none
     */
    function matchStates($states, $state_filter) {

        // Try the most granular (topics) first.
        if (!empty($this->topics)) {

            // "OR" means we can do all the topics in one join.
            if ($this->spec['topic-logic'] == 'OR') {
                $alias = $this->addStateJoin($states, $state_filter);
                $this->qry->condition("$alias.topic_id", $this->topics, 'IN');
            }

            // Otherwise, user wants multiple topics all to be represented
            else {
                foreach ($this->topics as $topicId) {
                    $alias = $this->addStateJoin($states, $state_filter);
                    $this->qry->condition("$alias.topic_id", $this->topicId);
                }
            }
        }

        // Boards needed?
        if ($this->needBoards) {

            // Boards are always checked in separate joins.
            foreach ($this->boards as $boardId) {
                $alias = $this->addStateJoin($states, $state_filter);
                $this->qry->condition("$alias.board_id", $boardId);
            }
        }

        // Test for the state(s) without regard to topic or board,
        // IFF we didn't create any joins earlier in this method.
        if (!$this->stateFilters)
            $this->addStateJoin($states, $state_filter);
    }

    /**
     * Add a join to the ebms_article_state table to match one
     * or more states.  Can also add a join to restrict the state
     * rows to those which belong to a specific cycle.  See notes
     * on matchStates() for an explanation of the parameters.  As
     * noted there, $states can be an array or a single integer.
     * The condition() method for the query object automatically
     * uses the correct operator: 'IN' for arrays, '=' otherwise.
     *
     *  @return  string     Alias for the joined state table
     */
    function addStateJoin($states, $state_filter) {

        $alias = 'state_alias_' . $this->delta;
        $this->qry->join('ebms_article_state', $alias,
            "$alias.article_id = art.article_id");
        $this->qry->condition("$alias.state_id", $states);
        if ($state_filter == 'current')
            $this->qry->condition("$alias.current", 'Y');
        elseif ($state_filter == 'active')
            $this->qry->condition("$alias.active_status", 'A');
        if (!empty($this->spec['review-cycle'])) {
            $cycle_id = $this->spec['review-cycle'];
            $cycle_alias = 'cycle_alias_' . $this->delta;
            $this->qry->join('ebms_article_topic', $cycle_alias,
                "$cycle_alias.article_id = $alias.article_id AND " .
                "$cycle_alias.topic_id = $alias.topic_id");
            $this->qry->condition("$cycle_alias.cycle_id", $cycle_id);
        }

        // Remember that we've filtered by state.
        $this->stateFilters = true;

        // Increment here so the two aliases created above have the same
        // number.
        $this->delta++;

        // Alias needed by the caller for possible further joins.
        return $alias;
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

        // Make sure this hasn't been taken care of already.
        if (!$this->stateFilters && empty($this->spec['review-cycle'])) {

            // Check topics first.
            if (!empty($this->topics)) {

                // Can we handle this is with a single join?
                $logic = $this->spec['topic-logic'];
                if ($logic == 'OR' || count($this->topics) == 1) {
                    $alias = 'topic_alias_' . $this->delta++;
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
                        $alias = 'topic_alias_' . $this->delta++;
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
                    $alias = 'topic_alias_' . $this->delta++;
                    $this->qry->join('ebms_article_state', $alias,
                                     "$alias.article_id = art.article_id");
                    $this->qry->condition("$alias.board_id", $boardId);
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
                $on = "$citeAlias.article_id = art.article_id";

                // OCEEBMS-295
                if (!empty($this->spec['author-pos'])) {
                    switch ($this->spec['author-pos']) {
                        case 'first':
                            $on .= " AND $citeAlias.cite_order = 1";
                            break;
                        case 'last':
                            $on .= " AND $citeAlias.cite_order = (
                                SELECT MAX(cite_order)
                                  FROM ebms_article_author_cite
                                 WHERE article_id = $citeAlias.article_id)";
                            break;
                    }
                }
                $this->qry->join('ebms_article_author_cite', $citeAlias, $on);
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

        // Board members don't have access to these fields.
        if (empty($this->spec['is-board-member'])) {

            $comment = $this->spec['comments'];

            // See if user wants to search for comment-added date
            $dateStart = $this->spec['comment-added-start']['date'];
            $dateEnd = $this->spec['comment-added-end']['date'];

            // But if we're searching any mod date, don't need to repeat here
            if ($this->spec['modified-start']['date'] ||
                $this->spec['modified-end']['date'])
                $dateStart = $dateEnd = '';

            // If we have either of them, we need to join states and comments
            // This could be optimized if modified-start also, but it's not
            //  worth the trouble.  It should be a rare search.
            if ($comment || $dateStart || $dateEnd) {
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
                if ($dateStart || $dateEnd) {
                    $range = $this->chkDates($dateStart, $dateEnd);

                    // If we found a valid range
                    if ($range) {
                        if ($range[0])
                            $this->qry->condition('statecom.comment_dt',
                                $range[0], '>=');
                        if ($range[1])
                            $this->qry->condition('statecom.comment_dt',
                                $range[1], '<=');
                    }
                }

                // Standard qualifications
                if (!empty($this->topics))
                    $this->qry->condition("comstate.topic_id", $this->topics,
                        'IN');
                else if (!empty($this->boards))
                    $this->qry->condition("comstate.board_id", $this->boards,
                        'IN');
            }
        }
    }

    /**
     * Search for tagged articles.
     */
    function srchTags() {

        // Board members don't have access to these fields.
        if (empty($this->spec['is-board-member'])) {


            $tagId     = $this->spec['tag'];
            $dateStart = $this->spec['tag-added-start']['date'];
            $dateEnd   = $this->spec['tag-added-end']['date'];

            // If we have any of these, we need to join tags
            if ($tagId || $dateStart || $dateEnd) {

                // Look in the table of active tags
                $this->qry->join('ebms_article_tag', 'tag',
                    'tag.article_id = art.article_id');
                $this->qry->condition('tag.active_status', 'A');

                // Looking for a specific tag?
                if ($tagId)
                    $this->qry->condition('tag.tag_id', $tagId);

                // Looking for a date range?
                // Do it whether or not we're looking for a specific tag
                if ($dateStart || $dateEnd) {

                    // Search for tags within the modified range
                    $range = $this->chkDates($dateStart, $dateEnd);

                    // If we found a valid range
                    if ($range) {
                        if ($range[0])
                            $this->qry->condition('tag.tag_dt', $range[0],
                                '>=');
                        if ($range[1])
                            $this->qry->condition('tag.tag_dt', $range[1],
                                '<=');
                    }
                }

                // Tags can have topics, but not boards
                if (!empty($this->topics))
                    $this->qry->condition("tag.topic_id", $this->topics, 'IN');
            }
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

        // Board members don't have access to the core-journals field.
        if (empty($this->spec['is-board-member'])) {
            switch ($this->spec['core-journals']) {
                case 'yes':
                    $this->qry->join('ebms_core_journal', 'core',
                        'core.source = art.source AND ' .
                        'core.source_jrnl_id = art.source_jrnl_id');
                    break;
                case 'no':
                    $this->qry->leftJoin('ebms_core_journal', 'core',
                        'core.source = art.source AND ' .
                        'core.source_jrnl_id = art.source_jrnl_id');
                    $this->qry->isNull('core.source_jrnl_id');
                    break;
            }
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

        // Board members don't have access to this field.
        if (empty($this->spec['is-board-member'])) {

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
    }

    /**
     * Restrict the results to those which have been assigned to
     * a specific cycle.  Using the new ebms_article_topic table,
     * created for just this purpose.
     */
    function srchCycle() {

        // Board members don't have access to these fields.
        if (!empty($this->spec['is-board-member']))
            return;

        // If we filtered by states, this has already been taken care of.
        if ($this->stateFilters)
            return;

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

        // Board members don't have access to this field.
        if (empty($this->spec['is-board-member'])) {

            $memberId = $this->spec['reviewer'];
            if ($memberId > 0) {
                // Subquery to include articles dropped from packets if a
                //  response was already received
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

        // Board members don't have access to this field.
        if (empty($this->spec['is-board-member'])) {

            $response = $this->spec['response'];
            if ($response) {
                $this->qry->join('ebms_article_review', 'artrev',
                    'artrev.article_id = art.article_id');
                $this->qry->join('ebms_review_disposition', 'revdisp',
                    'revdisp.review_id = artrev.review_id');
                $this->qry->condition('revdisp.value_id', $response);
            }
        }
    }

    /**
     * Find articles input within a date range.
     */
    function srchInputDate() {

        // Board members don't have access to these fields.
        if (empty($this->spec['is-board-member'])) {

            $dateStart = $this->spec['input-start']['date'];
            $dateEnd = $this->spec['input-end']['date'];
            if ($dateStart || $dateEnd) {
                $range = $this->chkDates($dateStart, $dateEnd);

                if ($range) {
                    // We already specified the article table
                    if ($range[0])
                        $this->qry->condition('art.import_date',
                            $range[0], '>=');
                    if ($range[1])
                        $this->qry->condition('art.import_date',
                            $range[1], '<=');
                }
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

        // Board members don't have access to these fields.
        if (!empty($this->spec['is-board-member']))
            return;

        $dateStart = $this->spec['modified-start']['date'];
        $dateEnd = $this->spec['modified-end']['date'];
        if ($dateStart || $dateEnd) {
            $range = $this->chkDates($dateStart, $dateEnd);

            if ($range) {
                // Build a subquery to find articles that have rows
                //   for states, state comments, tags, tag comments

                // 1. State changes
                $stateQry = db_select('ebms_article_state', 'mstat');
                $stateQry->addField('mstat', 'article_id');
                if ($range[0])
                    $stateQry->condition('mstat.status_dt', $range[0], '>=');
                if ($range[1])
                    $stateQry->condition('mstat.status_dt', $range[1], '<=');

                // Have to qualify by topic or board if needed
                $this->modDateQuals($stateQry, 'mstat.topic_id',
                                    'mstat.board_id');

                // 2. State comment changes
                $scmtQry = db_select('ebms_article_state_comment', 'mcmt');
                $scmtQry->join('ebms_article_state', 'mst',
                               'mcmt.article_state_id = mst.article_state_id');
                $scmtQry->addField('mst', 'article_id');
                if ($range[0])
                    $scmtQry->condition('mcmt.comment_dt', $range[0], '>=');
                if ($range[1])
                    $scmtQry->condition('mcmt.comment_dt', $range[1], '<=');
                $this->modDateQuals($scmtQry, 'mst.topic_id', 'mst.board_id');

                // 3. Tag additions
                $tagQry = db_select('ebms_article_tag', 'mtag');
                $tagQry->addField('mtag', 'article_id');
                if ($range[0])
                    $tagQry->condition('mtag.tag_dt', $range[0], '>=');
                if ($range[1])
                    $tagQry->condition('mtag.tag_dt', $range[1], '<=');
                $this->modDateQuals($tagQry, 'mtag.topic_id');

                // 4. Tag comment additions
                $tcmtQry = db_select('ebms_article_tag_comment', 'tcmt');
                $tcmtQry->join('ebms_article_tag', 'ctag',
                               'tcmt.article_tag_id = ctag.article_tag_id');
                $tcmtQry->addField('ctag', 'article_id');
                if ($range[0])
                    $tcmtQry->condition('tcmt.comment_dt', $range[0], '>=');
                if ($range[1])
                    $tcmtQry->condition('tcmt.comment_dt', $range[1], '<=');
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
     * Filter search results to only those that are listed as on an agenda
     * for a meeting matching specified dates and meeting type.
     *
     * If no meeting type or dates are specified, no filtering is done.  A
     * user must specify at least one of those parameters to invoke OnAgenda
     * filtering.
     *
     * This is one of the more complicated filters.
     */
    function srchOnAgenda() {

        // Board members don't have access to these fields.
        if (!empty($this->spec['is-board-member']))
            return;

        // Any one of the following means article must have OnAgenda state
        $meet_type  = $this->spec['meeting-category'];
        $meet_start = $this->spec['meeting-start']['date'];
        $meet_end   = $this->spec['meeting-end']['date'];

        if ($meet_type || $meet_start || $meet_end) {

            // Create a parameter list to construct an EventQuery object
            $params = array();
            $params['date-start']['date'] = $meet_start;
            $params['date-end']['date']   = $meet_end;

            // Construct the object
            $meetEventQry = new \Ebms\EventQuery();

            // We already know at least one of these obtains
            if ($meet_type)
                $meetEventQry->restrict_by_type($meet_type);
            if ($meet_start || $meet_end)
                $meetEventQry->restrict_by_date($params);

            // Execute the search query to get node ids for all meeting events
            $results = $meetEventQry->execute();
            $meeting_nids = array();

            // IN condition blows up if there are no meeting_nids
            // Prevent that by installing an impossible value
            if (isset($results['node']))
                $meeting_nids = array_keys($results['node']);
            else
                $meeting_nids[] = -1;

            // Get all the article_state_ids from the table that correlates
            //  meeting node ids with article_state_ids
            $this->qry->join('ebms_article_state', 'meet_state',
                             "meet_state.article_id = art.article_id");

            $this->qry->join('ebms_agenda_meeting', 'agenda',
                "agenda.article_state_id = meet_state.article_state_id");

            $this->qry->condition("agenda.nid", $meeting_nids, 'IN');
        }
    }

    /**
     * Search for articles which have a specific decision made by the
     * editorial boards, toward the end of the processing of articles.
     */
    function srchDecision() {

        // Don't do anything unless the user specified a decision.
        if (!empty($this->spec['decision'])) {

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
     *  @param string $startDate    ISO format, possibly incomplete
     *  @param string $endDate      ISO format, possibly incomplete
     *
     *  @return array               Tuple of date/time strings
     *                              Return NULL if no valid date passed or
     *                              end date is < start date.
     *
     * Notes:
     *  Users are allowed to omit trailing portions of the date strings.
     *  For example:
     *   '2012'
     *   '2014-03'
     *  We provide appropriate expansions for these cases.
     */
    function chkDates($startDate, $endDate) {

        // Make sure we have at least one (possibly partial) date.
        if (!$startDate && !$endDate)
            return null;

        // Initial assumptions.
        $startSqlDate = $endSqlDate = '';

        // Fill out partial dates.
        if ($startDate)
            $startSqlDate = Util::fill_in_date($startDate);
        if ($endDate)
            $endSqlDate = Util::fill_in_date($endDate, true);

        // If the end date is < start date, there's no point going on
        if ($startSqlDate && $endSqlDate)
            if (strcmp($endSqlDate, $startSqlDate) < 0)
                return null;
        return array($startSqlDate, $endSqlDate);
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
            case 'PMID':
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
            case 'Core Journals':
                $this->qry->leftJoin('ebms_core_journal', 'cj',
                                     'cj.source = art.source AND ' .
                                     'cj.source_jrnl_id = art.source_jrnl_id');
                $this->qry->addExpression('ISNULL(cj.source_jrnl_id)', 'core');
                $this->qry->orderBy('core');
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
     * board is automatically implied by the topic(s).
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
