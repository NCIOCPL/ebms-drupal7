<?php

// $Id$

define('VIEW_ALL', 'View All');     // User choice for all on one page

/**
 * @file
 *
 * Implementation of EBMS pages for searching the database for specific
 * articles.  This story got implemented in the wake of a management
 * decision that the development for the system had to be completed
 * by Halloween, drastically collapsing the amount of time available
 * for the remaining components.  As a result, some significant shortcuts
 * were taken.  In particular:
 *
 *    o We're using Drupal form field appearance/layout, rather than
 *      customizing the form layout to conform to the designer's
 *      mockups.
 *
 *    o The "Citation Actions" box has not been created.
 *
 *    o Tagging articles with stars for bulk processing of a subset
 *      of a search result set has not been implemented.
 *
 *    o Changing of display options on the results page is not supported.
 *
 *    o A count of summary topics selected is not displayed when the
 *      topics block is collapsed.
 *
 *    o Only one display format implemented (following the layout given
 *      in the design mockups).
 *
 *    o Sorting of the results set by author is unsupported.
 */

/**
 * Callback registered for the /citations/search page.
 */
function pdq_ebms_search($id=null) {
    Ebms\Menu::$active = 'Citation Management';
    drupal_add_css(Ebms\CSS_DIR . '/search.css');
    drupal_set_breadcrumb(
        array(
            l('Citation Management', 'citations'),
            'Search Database',
        )
    );
    $search = new EbmsSearch($id);
    return $search->run();
}

/**
 * Wrapper for logic to handle article searching.  Keeps our functions
 * out of the global namespace with less ugly syntax than the PHP
 * namespace mechanism provides, and lets us pass around the current
 * state information more easily.
 */
class EbmsSearch {

    // Instance data.
    public
        $parms,
        $search_id;

    // Class data.
    public static $url = 'citations/search';

    /**
     * The $id parameter will be non-null if we are executing or refining
     * an existing search request.  Otherwise, we're just displaying a
     * new search request form.
     */
    public function __construct($id) {
        $this->search_id = $id;
        $this->parms = drupal_get_query_parameters();
    }

    /**
     * Three possibilities here.
     *
     *   1. We're being asked to execute a search and display the results.
     *      This is the case when the invoking URL is in the format:
     *      .../citations/search/<decimal-number>.  The number at the end
     *      of the URL is the ID for the row in the database table where
     *      the parameters for the search are stored.
     *
     *   2. We're being asked to let the user refine a previous search.
     *      For this case we get .../citations/search/R<decimal-number>.
     *      We use that number to pull the search parameters out so we
     *      can use them to seed the form with the defaults from that
     *      search.
     *
     *   3. If there is no argument embedded in the URL after ../search
     *      then we just put up a blank form with no default values
     *      selected.
     */
    public function run() {
        drupal_add_js(Ebms\JS_DIR . '/search.js', 'file');
        $search_id = null;
        if ($this->search_id) {
            if (substr($this->search_id, 0, 1) == 'R')
                $search_id = substr($this->search_id, 1);
            else
                return $this->show_search_results();
        }
        $left_nav = Ebms\Util::build_left_nav('Search Database');
        return array(
            'left-nav' => $left_nav,
            'search form' => drupal_get_form('pdq_ebms_search_db_form',
                           $search_id),
        );
    }

    /**
     * Pull the json-encoded search parameters from its row in the
     * ebms_search table.
     *
     *  @param int    $search_id   Primary key for ebms_search table
     *  @param bool   $decode      If true, return the parameters as
     *                             a PHP dictionary array; otherwise,
     *                             return them as a json-encoded string,
     *                             which is how they are stored in the
     *                             database table.
     */
    public static function fetch_search_spec($search_id, $decode=true) {
        $json = db_select('ebms_search', 's')
            ->fields('s', array('search_spec'))
            ->condition('s.search_id', $search_id)
            ->execute()
            ->fetchField();
        if ($decode)
            return drupal_json_decode($json);
        return $json;
    }

    /**
     * Ask the search API to find out which articles match the user's
     * search criteria, and display the results.
     */
    function show_search_results() {

        // Ask the search API to construct a database query for the search.
        require_once('EbmsSearch.inc');
        $json = $this->fetch_search_spec($this->search_id, false);
        $spec = drupal_json_decode($json);
        $query = Ebms\getSearchQuery($json);

        // Figure out whether we're going to need paging.
        $counter = $query->countQuery();
        $count = $counter->execute()->fetchField();
        $per_page = $spec['items-per-page'];
        $pager = true;
        $show_pages_link = '';
        if (isset($this->parms['pager']) && $this->parms['pager'] == 'off') {
            $pager = false;
            if ($per_page != VIEW_ALL && $per_page < $count) {
                $url = EbmsSearch::$url . '/' . $this->search_id;
                $show_pages_link = l('VIEW PAGES', $url);
            }
        }
        if (!$pager || $per_page == VIEW_ALL)
            $per_page = $count;
        if ($count > $per_page)
            $query = $query->extend('PagerDefault')->limit($per_page);

        // Execute the query and build up the rows for the results set.
        $results = $query->execute();
        $items = array();
        $page = 0;
        if (isset($this->parms['page']))
            $page = $this->parms['page'];
        $pos = 1 + $page * $per_page;
        foreach ($results as $result)
            $items[] = $this->list_article($result->article_id, $pos++);

        // Assemble the Drupal render array for the page.
        $refine_url = url(EbmsSearch::$url . '/R' . $this->search_id);
        $list = array(
            '#type' => 'container',
            '#attributes' => array('id' => 'search-results'),
            'page title' => array(
                '#type' => 'markup',
                '#markup' => '<h2>Search Results (' . $count . ')</h2>',
            ),
            'refine' => array(
                '#type' => 'markup',
                '#markup' => '<div class="float-left"><a href="' .
                $refine_url . '" class="button">REFINE SEARCH</a></div>',
            ),
        );
        $prefix = '<div id="articles-div">';
        if ($count > $per_page)
            $list['pager-top'] = array('#theme' => 'pager');
        elseif (!empty($show_pages_link))
            $list['pager-top'] = array(
                '#type' => 'markup',
                '#markup' =>
                "<div id='pager-view-pages'>$show_pages_link</div>",
            );
        else
            $prefix = '<div id="articles-div" class="extra-top-margin">';
        $list['items'] = array(
            '#prefix' => $prefix,
            '#suffix' => '</div>',
            '#theme' => 'item_list',
            '#items' => $items,
            '#attributes' => array('id' => 'article-list'),
        );

        if ($count > $per_page)
            $list['pager-bottom'] = array('#theme' => 'pager');
        elseif (!empty($show_pages_link))
            $list['pager-bottom'] = array(
                '#type' => 'markup',
                '#markup' =>
                "<div id='pager-view-pages'>$show_pages_link</div>",
            );

        $left = Ebms\Util::build_left_nav('Search Database');
        return array($left, $list);
    }

    /**
     * Assemble the markup for a single article in the search results set.
     *
     *  @param  int     $article_id  Primary key into the ebms_article table
     *  @param  int     $position    Integer showing where the article fits
     *                               in the result set; displayed to the left
     *                               the article.
     *
     * XXX May want to replace some of the parsing code below when Alan's
     *     API for getting citation elements is in place.
     */
    function list_article($article_id, $position) {

        // Get the cached state ID for 'Ready for Initial Review'.
        $cached = &drupal_static(__FUNCTION__);
        if (!isset($cached)) {
            $state_id = db_select('ebms_article_state_type', 't')
                ->condition('t.state_text_id', 'ReadyInitReview')
                ->fields('t', array('state_id'))
                ->execute()
                ->fetchField();
            $cached = array('ready_for_initial_review' => $state_id);
        }
        $ready_for_initial_review = $cached['ready_for_initial_review'];

        // Pull NLM's XML document for the article from the database
        $xml = db_select('ebms_article', 'a')
            ->fields('a', array('source_data'))
            ->condition('a.article_id', $article_id)
            ->execute()
            ->fetchField();

        // See if the article was converted from the legacy system.
        $legacy_id = db_select('ebms_legacy_article_id', 'i')
            ->fields('i', array('legacy_id'))
            ->condition('i.article_id', $article_id)
            ->execute()
            ->fetchField();

        // Extract the elements that we need from the XML document.
        $doc = new \SimpleXMLElement($xml);
        $title = $ta = $vol = $issue = $pgn = $year = $pmid = '';
        $article = $doc->MedlineCitation->Article;
        $title = $article->ArticleTitle;
        $pgn = htmlspecialchars($article->Pagination->MedlinePgn);
        $year = $article->Journal->JournalIssue->PubDate->Year;
        $issue = htmlspecialchars($article->Journal->JournalIssue->Issue);
        $vol = htmlspecialchars($article->Journal->JournalIssue->Volume);
        $ta = $doc->MedlineCitation->MedlineJournalInfo->MedlineTA;
        $ta = htmlspecialchars($ta);
        $pmid = $doc->MedlineCitation->PMID;

        // Build up a sequence of author names
        $authors = array();
        if (isset($doc->MedlineCitation->Article->AuthorList->Author)) {
            foreach ($doc->MedlineCitation->Article->AuthorList->Author
                as $author)
            {
                $name = $author->CollectiveName;
                if (!$name) {
                    $name = $author->LastName;
                    if ($author->Initials)
                        $name .= ' ' . $author->Initials;
                }
                $authors[] = htmlspecialchars($name);
            }
        }
        if (count($authors) < 1)
            $authors = array('[No authors listed]');

        // Find out which topics were assigned at import (TIR 2209).
        // No, make that all topics (TIR 2433).
        $query = db_select('ebms_article_state', 's');
        $query->join('ebms_topic', 't', 't.topic_id = s.topic_id');
        $query->condition('s.article_id', $article_id);
        $query->condition('s.active_status', 'A');
        //$query->condition('s.state_id', $ready_for_initial_review);
        $query->fields('t', array('topic_name'));
        $query->distinct();
        $query->orderBy('t.topic_name');
        $results = $query->execute();
        $topics = array();
        foreach ($results as $result)
            $topics[] = htmlspecialchars($result->topic_name);
        $topics = implode('; ', $topics);

        // Create the links we'll need.
        $full_url = 'citations/full/' . $article_id;
        $full_opts = array('query' => array('search' => $this->search_id));
        $title_opts = array(
            'query' => array('search' => $this->search_id),
            'attributes' => array(
                'class' => array('article-title'),
            )
        );
        $title_link = l($title, $full_url, $title_opts);

        // Assemble the markup pieces into an array.
        $pieces = array(
            '<span class="article-number">' . $position . '</span>',
            '<span class="article-citation">',
            implode('; ', $authors) . '<br />',
            $title_link,
            '<br />',
            $ta, ' ', $vol,
        );
        if ($issue)
            $pieces[] = "($issue)";
        if ($pgn)
            $pieces[] = ": $pgn";
        if ($year)
            $pieces[] = ", $year";
        $pieces[] = "<br />PMID: $pmid &nbsp; EBMS ID: $article_id";
        if ($legacy_id)
            $pieces[] = " &nbsp; Legacy CMS ID: $legacy_id";
        if ($topics)
            $pieces[] = "<br />Topics: $topics";
        $pieces[] = '<br /><br />';
        $pieces[] = '<a href="http://www.ncbi.nlm.nih.gov/pubmed/' .
            $pmid . '" target="_blank">VIEW ABSTRACT</a>';
        $ft_uri = Ebms\Util::get_ft_uri($article_id);
        if ($ft_uri) {
            $ft_url = file_create_url($ft_uri);
            $target = array('attributes' => array('target' => '_blank'));
            $ft_link = l('DOWNLOAD FULL TEXT', $ft_url, $target);
            $pieces[] = ' &nbsp;|&nbsp; ' . $ft_link;
        }
        $pieces[] = '</span></br /><br />&nbsp;';

        // Collapse and return the markup.
        return implode($pieces);
    }
}

/**
 * Separate class for the user's parameters in an article search.
 */
class EbmsSearchSpec {

    // Default values for when no search parameters have been specified.
    public $no_date = array('year' => 0, 'month' => 0, 'day' => 0);
    public
        $boards = array(),
        $topics = array(),
        $topic_connector = 'OR',
        $pmid = '',
        $cms_id = '',
        $author = '',
        $title = '',
        $journal = '',
        $comments = '',
        $publication_year = 0,
        $publication_month = 0,
        $review_cycle = 0,
        $fyi = 0,
        $nci_reviewer_decision = '',
        $full_text_retrieved = '',
        $committee_decision = '',
        $core_journals = '',
        $unpublished = 1,
        $only_unpublished = 0,
        $not_listed = 1,
        $only_not_listed = 0,
        $rejected = 1,
        $only_rejected = 0,
        $summary_topics_added = 0,
        $reviewer = 0,
        $response = 0,
        $decision = 0,
        $tag = 0,
        $comment_added_start = '',
        $comment_added_end = '',
        $tag_added_start = '',
        $tag_added_end = '',
        $input_start = '',
        $input_end = '',
        $modified_start = '',
        $modified_end = '',
        $sort_by = 'CMS ID#',
        $format = 'Brief',
        $items_per_page = 10;

    /**
     * Override the default search parameter values if a search ID
     * is specified, using the values stored in the database.
     */
    public function __construct($search_id) {
        if ($search_id) {
            $spec = EbmsSearch::fetch_search_spec($search_id);
            foreach ($spec['boards'] as $id => $val)
                if ($val)
                    $this->boards[] = $id;
            foreach ($spec['topics'] as $id => $val)
                if ($val)
                    $this->topics[] = $id;
            $this->fyi = $spec['fyi-citation'];
            $this->nci_reviewer_decision = $spec['nci-reviewer-decision'];
            $this->full_text_retrieved = $spec['full-text-retrieved'];
            $this->committee_decision = $spec['committee-decision'];
            $this->core_journals = $spec['core-journals'];
            $this->unpublished = $spec['unpublished'];
            $this->not_listed = $spec['not-listed'];
            $this->rejected = $spec['rejected'];
            $this->only_unpublished = $spec['only-unpublished'];
            $this->only_not_listed = $spec['only-not-listed'];
            $this->only_rejected = $spec['only-rejected'];
            $this->summary_topics_added = $spec['summary-topics-added'];
            $this->topic_connector = $spec['topic-logic'];
            $this->pmid = $spec['pmid'];
            $this->cms_id = $spec['cms-id'];
            $this->author = $spec['author'];
            $this->title = $spec['title'];
            $this->journal = $spec['journal'];
            $this->comments = $spec['comments'];
            $this->publication_year = $spec['publication-year'];
            $this->publication_month = $spec['publication-month'];
            $this->review_cycle = $spec['review-cycle'];
            $this->reviewer = $spec['reviewer'];
            $this->response = $spec['response'];
            $this->decision = $spec['decision'];
            $this->tag = $spec['tag'];
            $this->comment_added_start = $spec['comment-added-start']['date'];
            $this->comment_added_end = $spec['comment-added-end']['date'];
            $this->tag_added_start = $spec['tag-added-start']['date'];
            $this->tag_added_end = $spec['tag-added-end']['date'];
            $this->input_start = $spec['input-start']['date'];
            $this->input_end = $spec['input-end']['date'];
            $this->modified_start = $spec['modified-start']['date'];
            $this->modified_end = $spec['modified-end']['date'];
            $this->sort_by = $spec['sort-by'];
            //$this->format = $spec['format'];
            $this->items_per_page = $spec['items-per-page'];
            if ($this->comment_added_start)
                $this->comment_added_start =
                    Ebms\Util::fill_in_date($this->comment_added_start);
            if ($this->comment_added_end)
                $this->comment_added_end =
                    Ebms\Util::fill_in_date($this->comment_added_end, true);
            if ($this->tag_added_start)
                $this->tag_added_start =
                    Ebms\Util::fill_in_date($this->tag_added_start);
            if ($this->tag_added_end)
                $this->tag_added_end =
                    Ebms\Util::fill_in_date($this->tag_added_end, true);
            if ($this->input_start)
                $this->input_start =
                    Ebms\Util::fill_in_date($this->input_start);
            if ($this->input_end)
                $this->input_end =
                    Ebms\Util::fill_in_date($this->input_end, true);
            if ($this->modified_start)
                $this->modified_start =
                    Ebms\Util::fill_in_date($this->modified_start);
            if ($this->modified_end)
                $this->modified_end =
                    Ebms\Util::fill_in_date($this->modified_end, true);
        }
    }
}

/**
 * Prevent the user from submitting a search request without having
 * specified any search criteria.  Specifying a board (or multiple
 * boards) isn't sufficient.  Also, make sure date ranges are fully
 * specified.
 */
function pdq_ebms_search_db_form_validate($form, &$form_state) {

    // Use shorthand for checking values.
    $values = $form_state['values'];

    // Don't do anything for AJAX calls.
    if (!isset($values['op']))
        return;

    // If the user hits the Reset button, draw a fresh form.
    if ($form_state['values']['op'] == 'Reset')
        drupal_goto(EbmsSearch::$url);

    // Initial assumption.
    $have_value = $have_board = false;

    // Any of the multi-select fields have a value selected?
    $multi_fields = array('boards', 'topics');
    foreach ($multi_fields as $field) {
        foreach ($values[$field] as $id => $val) {
            if ($val) {
                if ($field == 'boards')
                    $have_board = true;
                else
                    $have_value = true;
            }
        }
    }

    // Check the fields that can have a single value.  We don't bother
    // with the ones that widen the net rather than narrow it (for
    // example, 'unpublished' or 'not-listed' or 'rejected').
    if (!$have_value) {
        $simple_fields = array(
            'pmid', 'cms-id', 'author', 'title', 'journal',
            'comments', 'publication-year', 'review-cycle',
            'reviewer', 'response', 'decision', 'tag',
            'fyi-citation', 'nci-reviewer-decision',
            'committee-decision', 'full-text-retrieved',
            // 'core-journals', XXX not implemeted yet
            'summary-topics-added',
            'only-unpublished', 'only-rejected', 'only-not-listed',
        );
        foreach ($simple_fields as $field) {
            if ($values[$field]) {
                $have_value = true;
                break;
            }
        }
    }

    // Check the date fields.
    $date_fields = array('comment-added', 'tag-added', 'input', 'modified');
    $message = 'Start date cannot be earlier than end date.';
    foreach ($date_fields as $date_field) {
        $start_name ="$date_field-start";
        $start_date = $values[$start_name]['date'];
        $end_name = "$date_field-end";
        $end_date = $values[$end_name]['date'];
        if ($start_date || $end_date)
            $have_value = true;
        if ($start_date && $end_date) {
            $end_date = Ebms\Util::fill_in_date($end_date);
            if (strcmp($start_date, $end_date) > 0)
                form_set_error($start_name, $message);
        }
    }

    // Complain if we didn't get any search criteria.
    if (!$have_value)
        form_set_error('pmid',
            'At least one (non-board) search criterion is required.');

    // If requested that display options be saved
    if (!empty($values['save-as-default'])) {
        // Convert string to integer representation
        $per_page = $values['items-per-page'];
        if ($per_page == VIEW_ALL)
            $per_page = 0;

        global $user;
        $items = array(
            'dft_cite_search_sort_key' => $values['sort-by'],
            'dft_cite_search_items_per_page' => $per_page);
        Ebms\Util::set_custom_user_field_values($items, $user->uid);
    }
}

/**
 * Make sure the ending date for a date range is later than the
 * starting date.
 */
function pdq_ebms_check_date_range($field_name, $start, $end) {
    $s = sprintf('%04d-%02d-%02d', $start['year'], $start['month'],
         $start['day']);
    $e = sprintf('%04d-%02d-%02d', $end['year'], $end['month'], $end['day']);
    if (strcmp($s, $e) >= 0)
        form_set_error($field_name,
            'Date range end must be later than start date.');
}

/**
 * Drupal callback for constructing the search request form.
 */
function pdq_ebms_search_db_form($form, &$form_state, $search_id) {

    // Get the default values, possibly drawn from a previous search.
    $defaults = new EbmsSearchSpec($search_id);

    // For new searches only, override with any user specific defaults
    if (!$search_id) {
        // Sorting
        $dft = Ebms\Util::get_custom_user_field_value(
                'dft_cite_search_sort_key');
        if (!is_null($dft))
            $defaults->sort_by = $dft;

        // Items per page, 0 requests all citations
        $dft = Ebms\Util::get_custom_user_field_value(
                    'dft_cite_search_items_per_page');
        if (!is_null($dft))
            if ($dft == 0)
                $defaults->items_per_page = VIEW_ALL;
            else
                $defaults->items_per_page = $dft;
    }

    // Find out which board are already selected.
    $checked_boards = $defaults->boards;
    if (isset($form_state['values'])) {
        $checked_boards = array();
        foreach ($form_state['values']['boards'] as $id => $val)
            if ($val)
                $checked_boards[] = $val;
    }

    /* Users changed their minds and decided against this.
     * I left it in but commented out in case they change again.
     * (AHM) 2014-09-16
    // If none selected, establish a default
    // "else" forces emptying of topics if user de-selected all boards
    // No reason for user to do that but behave properly if they do
    else if (empty($checked_boards)) {
        $checked_boards = array();
        $dftBoard = \Ebms\Util::get_user_default_board();
        if ($dftBoard)
            $checked_boards[] = $dftBoard;
        $defaults->boards = $checked_boards;
    }
    */

    // Build the topics picklist, based on which boards are selected.
    $topics = array();
    if (count($checked_boards) > 0) {
        $results = db_select('ebms_topic', 't')
            ->fields('t', array('topic_id', 'topic_name'))
            ->condition('t.board_id', $checked_boards, 'IN')
            ->condition('t.active_status', 'A')
            ->orderBy('t.topic_name')
            ->execute();
        foreach ($results as $result)
            $topics[$result->topic_id] = $result->topic_name;
    }

    // Create a custom legend for the summmary topics box.
    $selected_topics_count = count($defaults->topics);
    if ($selected_topics_count)
        $topselcnt = " &nbsp; ($selected_topics_count)";
    else
        $topselcnt = '';

    // Build the tag picklist.
    $results = db_select('ebms_article_tag_type', 't')
        ->fields('t', array('tag_id', 'tag_name'))
        ->orderBy('t.tag_name')
        ->execute();
    $tags = array(0 => ' ');
    foreach ($results as $result)
        $tags[$result->tag_id] = $result->tag_name;

    // Build the picklist for board decisions
    $results = db_select('ebms_article_board_decision_value', 'v')
        ->fields('v', array('value_id', 'value_name'))
        ->orderBy('v.value_name')
        ->execute();
    $decisions = array(0 => ' ');
    foreach ($results as $result)
        $decisions[$result->value_id] = $result->value_name;

    // Build the picklist for board member reviewer feedback.
    $results = db_select('ebms_review_disposition_value', 'v')
        ->fields('v', array('value_id', 'value_name'))
        ->orderBy('v.value_name')
        ->execute();
    $responses = array(0 => ' ');
    foreach ($results as $result)
        $responses[$result->value_id] = $result->value_name;

    // Build the picklist for reviewers.
    $query = db_select('users', 'u');
    $query->join('ebms_packet_reviewer', 'r', 'r.reviewer_id = u.uid');
    $query->fields('u', array('uid', 'name'));
    $query->orderBy('u.name');
    $results = $query->execute();
    $reviewers = array(0 => ' ');
    foreach ($results as $result)
        $reviewers[$result->uid] = $result->name;

    // Build picklists for the publishing year and month.
    $pub_months = array(0 => 'MONTH');
    for ($i = 1; $i <= 12; ++$i)
        $pub_months[$i] = date('F', mktime(0, 0, 0, $i));
    $query = db_select('ebms_article', 'a');
    $query->addExpression('MIN(a.published_date)', 'first_date');
    $query->condition('a.published_date', '0000', '>');
    $first_date = $query->execute()->fetchField();
    $query = db_select('ebms_article', 'a');
    $query->addExpression('MAX(a.published_date)', 'last_date');
    $last_date = $query->execute()->fetchField();
    $first_year = substr($first_date, 0, 4);
    $last_year = substr($last_date, 0, 4);
    $pub_years = array(0 => 'YEAR');
    for ($i = $last_year; $i >= $first_year; --$i)
        $pub_years[$i] = $i;

    // Build the review cycle picklist.
    $results = db_select('ebms_cycle', 'c')
        ->fields('c', array('cycle_id', 'cycle_name'))
        ->orderBy('c.start_date', 'DESC')
        ->execute();
    $review_cycles = array(0 => ' ');
    foreach ($results as $result)
        $review_cycles[$result->cycle_id] = $result->cycle_name;

    // Assemble the pieces for the search request form.
    return array(
        '#id' => 'search-db-form',
        'page title' => array(
            '#type' => 'markup',
            '#markup' => '<h2>Search Database</h2>',
        ),
        'multi-select-instructions' => array(
            '#type' => 'markup',
            '#markup' => '<p class="field-desc">To select ' .
            'more than one, hold down the CTRL ' .
            'or COMMAND (for Mac users) key</p>',
        ),
        'boards' => array(
            '#type' => 'select',
            '#multiple' => true,
            '#default_value' => $defaults->boards,
            '#title' => 'EDITORIAL BOARD',
            '#title_display' => 'after',
            '#attributes' => array('class' => array('fixed-width')),
            '#options' => Ebms\Util::boards(),
            '#size' => 6,
            '#ajax' => array(
                'callback' => 'pdq_ebms_db_search_board_ajax',
                'wrapper' => 'topics-boxes',
            ),
            '#prefix' => '<div class="clearfix">',
            '#suffix' => '</div>',
        ),

        // This part collapses, and is populated dynamically.
        'topics box' => array(
            '#type' => 'fieldset',
            '#title' => "SUMMARY TOPIC<span id='topselcnt'>$topselcnt</span>",
            '#collapsible' => true,
            '#collapsed' => count($defaults->topics) < 1,
            '#attributes' => array('class' => array('clearfix')),
            'topic-logic' => array(
                '#type' => 'radios',
                '#title' => '',
                '#default_value' => $defaults->topic_connector,
                '#options' => drupal_map_assoc(array('AND', 'OR')),
                '#prefix' => '<div class="clearfix left-radios right-boxes">',
                '#suffix' => '<span class="field-desc">If you do not select ' .
                '"AND" or "OR," the default search will be "OR"</span></div>',
            ),
            'topics' => array(
                '#type' => 'checkboxes',
                '#title' => '',
                '#options' => $topics,
                '#default_value' => $defaults->topics,
                '#attributes' => array(
                    'onchange' => 'ebmsscript.count_search_topics();',
                ),
                '#prefix' => '<div id="topics-boxes" class="clearfix">',
                '#suffix' => '</div>',
            ),
        ),

        // A few straightforward fields.
        'common-fields' => array(
            '#type' => 'container',
            '#attributes' => array('class' => array('clearfix')),
            'pmid' => array(
                '#type' => 'textfield',
                '#default_value' => $defaults->pmid,
                '#title' => 'PMID',
                '#title_display' => 'after',
                '#attributes' => array('class' => array('fixed-width')),
            ),
            'cms-id' => array(
                '#type' => 'textfield',
                '#title' => 'EBMS ID',
                '#title_display' => 'after',
                '#attributes' => array('class' => array('fixed-width')),
                '#default_value' => $defaults->cms_id,
            ),
            'author' => array(
                '#type' => 'textfield',
                '#title' => 'AUTHOR',
                '#title_display' => 'after',
                '#attributes' => array('class' => array('fixed-width')),
                '#default_value' => $defaults->author,
            ),
            'title' => array(
                '#type' => 'textfield',
                '#title' => 'TITLE',
                '#title_display' => 'after',
                '#attributes' => array('class' => array('fixed-width')),
                '#default_value' => $defaults->title,
            ),
            'publication-date' => array(
                '#type' => 'container',
                '#attributes' => array(
                    'class' => array('year-and-month'),
                ),
                'publication-year' => array(
                    '#type' => 'select',
                    '#title' => 'PUBLICATION YEAR',
                    '#title_display' => 'none',
                    '#options' => $pub_years,
                    '#default_value' => $defaults->publication_year,
                ),
                'publication-month' => array(
                    '#type' => 'select',
                    '#title' => 'PUBLICATION MONTH',
                    '#title_display' => 'none',
                    '#options' => $pub_months,
                    '#default_value' => $defaults->publication_month,
                ),
                'publication-date-label' => array(
                    '#type' => 'markup',
                    '#markup' => '<span>PUBLICATION DATE</span>',
                ),
            ),
        ),

        // Another collapsible portion of the form.
        'advanced search' => array(
            '#type' => 'fieldset',
            '#title' => 'ADVANCED SEARCH',
            '#attributes' => array('class' => array('clearfix')),
            '#collapsible' => true,
            '#collapsed' => true,
            'journal' => array(
                '#type' => 'textfield',
                '#title' => 'JOURNAL',
                '#title_display' => 'after',
                '#attributes' => array('class' => array('fixed-width')),
                '#default_value' => $defaults->journal,
            ),
            'review-cycle' => array(
                '#type' => 'select',
                '#title' => 'REVIEW CYCLE',
                '#title_display' => 'after',
                '#attributes' => array('class' => array('fixed-width')),
                '#options' => $review_cycles,
                '#default_value' => $defaults->review_cycle,
            ),
            'fyi-citation' => array(
                '#type' => 'checkbox',
                '#title' => 'FYI CITATION',
                '#prefix' => '<div class="checkbox-line">',
                '#suffix' => '</div>',
                '#default_value' => $defaults->fyi,
            ),
            'nci-reviewer-decision' => array(
                '#type' => 'radios',
                '#title' => 'NCI REVIEWER DECISION',
                '#options' => array(
                    'RejectBMReview' => 'NO',
                    'PassedBMReview' => 'YES',
                ),
                '#title_display' => 'after',
                '#prefix' => '<div class="yes-no-boxes clearfix">',
                '#suffix' => '</div>',
                '#default_value' => $defaults->nci_reviewer_decision,
            ),
            'full-text-retrieved' => array(
                '#type' => 'radios',
                '#title' => 'FULL TEXT RETRIEVED',
                '#options' => array(
                    'no' => 'NO',
                    'yes' => 'YES',
                ),
                '#title_display' => 'after',
                '#prefix' => '<div class="yes-no-boxes clearfix">',
                '#suffix' => '</div>',
                '#default_value' => $defaults->full_text_retrieved,
            ),
            'committee-decision' => array(
                '#type' => 'radios',
                '#title' => 'COMMITTEE DECISION',
                '#options' => array(
                    'RejectFullReview' => 'NO',
                    'PassedFullReview' => 'YES',
                ),
                '#title_display' => 'after',
                '#prefix' => '<div class="yes-no-boxes clearfix">',
                '#suffix' => '</div>',
                '#default_value' => $defaults->committee_decision,
            ),
            'reviewer' => array(
                '#type' => 'select',
                '#title' => 'EDITORIAL BOARD REVIEWER',
                '#title_display' => 'after',
                '#attributes' => array('class' => array('fixed-width')),
                '#options' => $reviewers,
                '#default_value' => $defaults->reviewer,
            ),
            'response' => array(
                '#type' => 'select',
                '#title' => 'EDITORIAL BOARD RESPONSE',
                '#title_display' => 'after',
                '#attributes' => array('class' => array('fixed-width')),
                '#options' => $responses,
                '#default_value' => $defaults->response,
            ),
            'decision' => array(
                '#type' => 'select',
                '#title' => 'EDITORIAL BOARD DECISION',
                '#title_display' => 'after',
                '#attributes' => array('class' => array('fixed-width')),
                '#options' => $decisions,
                '#default_value' => $defaults->decision,
            ),
            'comments' => array(
                '#type' => 'textfield',
                '#title' => 'COMMENT',
                '#title_display' => 'after',
                '#attributes' => array('class' => array('fixed-width')),
                '#default_value' => $defaults->comments,
            ),
            'comment-added' =>
            Ebms\Util::date_range_field('Date Comment Added', 'comment-added',
                                        $defaults->comment_added_start,
                                        $defaults->comment_added_end),
            'tag' => array(
                '#type' => 'select',
                '#title' => 'TAG',
                '#title_display' => 'after',
                '#attributes' => array('class' => array('fixed-width')),
                '#options' => $tags,
                '#default_value' => $defaults->tag,
            ),
            'tag-added' =>
            Ebms\Util::date_range_field('Date Tag Added', 'tag-added',
                                        $defaults->tag_added_start,
                                        $defaults->tag_added_end),
            'core-journals' => array(
                '#type' => 'radios',
                '#title' => 'CORE JOURNALS',
                '#options' => array(
                    'no' => 'NO',
                    'yes' => 'YES',
                ),
                '#title_display' => 'after',
                '#prefix' => '<div class="yes-no-boxes clearfix">',
                '#suffix' => '</div>',
                '#default_value' => $defaults->core_journals,
            ),
        ),

        // Yet another collapsible portion.
        'administrator search' => array(
            '#type' => 'fieldset',
            '#title' => 'ADMINISTRATOR SEARCH',
            '#collapsible' => true,
            '#collapsed' => true,
            '#attributes' => array('class' => array('clearfix')),
            'input' =>
            Ebms\Util::date_range_field('Input Date', 'input',
                                        $defaults->input_start,
                                        $defaults->input_end),
            'modified' =>
            Ebms\Util::date_range_field('Modified Date', 'modified',
                                        $defaults->modified_start,
                                        $defaults->modified_end),
            'unpublished' => array(
                '#type' => 'checkbox',
                '#title' => 'INCLUDE UNPUBLISHED',
                '#prefix' => '<div class="checkbox-line early-box">',
                '#suffix' => '</div>',
                '#default_value' => $defaults->unpublished,
            ),
            'only-unpublished' => array(
                '#type' => 'checkbox',
                '#title' => 'UNPUBLISHED ONLY',
                '#prefix' => '<div class="checkbox-line only-box">',
                '#suffix' => '</div>',
                '#default_value' => $defaults->only_unpublished,
            ),
            'not-listed' => array(
                '#type' => 'checkbox',
                '#title' => 'INCLUDE NOT LISTED',
                '#prefix' => '<div class="checkbox-line early-box">',
                '#suffix' => '</div>',
                '#default_value' => $defaults->not_listed,
            ),
            'only-not-listed' => array(
                '#type' => 'checkbox',
                '#title' => 'NOT LISTED ONLY',
                '#prefix' => '<div class="checkbox-line only-box">',
                '#suffix' => '</div>',
                '#default_value' => $defaults->only_not_listed,
            ),
            'rejected' => array(
                '#type' => 'checkbox',
                '#title' => 'INCLUDE REJECTED',
                '#prefix' => '<div class="checkbox-line early-box">',
                '#suffix' => '</div>',
                '#default_value' => $defaults->rejected,
            ),
            'only-rejected' => array(
                '#type' => 'checkbox',
                '#title' => 'REJECTED ONLY',
                '#prefix' => '<div class="checkbox-line only-box">',
                '#suffix' => '</div>',
                '#default_value' => $defaults->only_rejected,
            ),
            'summary-topics-added' => array(
                '#type' => 'checkbox',
                '#title' => 'SUMMARY TOPICS ADDED',
                '#prefix' => '<div class="checkbox-line">',
                '#suffix' => '</div>',
                '#default_value' => $defaults->summary_topics_added,
            ),
        ),

        // Final collapsible section.
        'display options' => array(
            '#type' => 'fieldset',
            '#title' => 'DISPLAY OPTIONS',
            '#collapsible' => true,
            '#collapsed' => true,
            '#attributes' => array('class' => array('clearfix')),
            'sort-by' => array(
                '#type' => 'select',
                '#title' => 'SORT BY',
                '#title_display' => 'after',
                '#attributes' => array('class' => array('fixed-width')),
                '#options' => drupal_map_assoc(
                    array(
                        'EBMS ID#',
                        'PM ID#',
                        'Title',
                        'Author',
                        'Journal',
                        'Core Journals',
                        //'Publication Date',
                    )
                ),
                '#default_value' => $defaults->sort_by,
            ),
            /* Don't show options we're not supporting (yet), per email
               thread 2013-01-14 with Laura & Ashleigh.
            'format' => array(
                '#type' => 'select',
                '#title' => 'FORMAT',
                '#title_display' => 'after',
                '#attributes' => array('class' => array('fixed-width')),
                '#options' => drupal_map_assoc(
                    array(
                        'Brief',
                        'Abstract',
                        'Citation History',
                        'Full Record',
                    )
                ),
                '#default_value' => $defaults->format,
            ),
            */
            'items-per-page' => array(
                '#type' => 'select',
                '#title' => 'PER PAGE',
                '#title_display' => 'after',
                '#attributes' => array('class' => array('fixed-width')),
                '#options' => drupal_map_assoc(
                    array('10', '25', '50', VIEW_ALL)
                ),
                '#default_value' => $defaults->items_per_page,
            ),
            'save-as-default' => array(
                '#type' => 'checkbox',
                '#title' => t('SAVE AS DEFAULT'),
                'checked' => '',
            ),
        ),

        'submit' => array(
            '#type' => 'submit',
            '#value' => 'Submit',
        ),

        // Added at Ashleigh's request (TIR 2293).
        'reset' => array(
            '#type' => 'submit',
            '#value' => 'Reset',
        ),
    );
}

/**
 * Callback to pull the dynamically build topic section of the search form.
 */
function pdq_ebms_db_search_board_ajax($form, $form_state) {
    return $form['topics box']['topics'];
}

/**
 * Store the search parameters and come back in with the ID of the
 * database row where we stored them.
 */
function pdq_ebms_search_db_form_submit($form, &$form_state) {
    $spec = drupal_json_encode($form_state['values']);
    global $user;
    $now = strftime("%Y-%m-%d %H:%M:%S");
    $search_id = db_insert('ebms_search')
        ->fields(
            array(
                'when_searched' => $now,
                'searched_by' => $user->uid,
                'search_spec' => $spec,
            )
        )
        ->execute();
    drupal_goto(EbmsSearch::$url . '/' . $search_id);
}

/**
 * Modify Drupal's stock date field.  For one thing, we need to insert
 * a value at the top of the picklists which mean the user hasn't selected
 * a value yet.  For another Drupal has too many years, and in the wrong
 * order.
 */
function pdq_ebms_fix_date($element) {
    $years = array(0 => 'YEAR');
    $today = getdate();
    for ($year = $today['year'] + 1; $year >= 2000; --$year)
        $years[$year] = $year;
    $element['year']['#options'] = $years;
    array_unshift($element['month']['#options'], 'MONTH');
    array_unshift($element['day']['#options'], 'DAY');
    return $element;
}
