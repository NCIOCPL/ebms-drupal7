<?php

// $Id$

/**
 * @file
 *
 * Implementation of EBMS page for running system reports.
 */
function pdq_ebms_reports($report=null, $request_id=0) {
    register_shutdown_function('pdq_ebms_shutdown');
    $control = new EbmsReports($report, $request_id);
    return $control->run();
}

/**
 * Don't let PHP terminate the request with a blank screen (TIR #2231).
 */
function pdq_ebms_shutdown() {
    $e = error_get_last();
    if ($e !== null) {
        if (is_array($e) && isset($e['message']))
            $message = $e['message'];
        else
            $message = 'Report failure (timeout?)';
        watchdog('reports', $message, array(), WATCHDOG_ERROR);
        drupal_goto('reports/timeout');
    }
}

/*
 * Wrapping as much functionality within a class in order to simplify passing
 * around state information for processing the current request.  Also makes
 * it easier to avoid polluting the global namespace.
 */
class EbmsReports {

    // Class attributes.
    public static $url = 'reports';

    // Used internally by the manager_actions() class method.
    private static $_manager_actions = null;

    // Instance attributes.
    public $report, $request_id;

    /**
     * Passing settings in the URL instead of in $_SESSION in order
     * to avoid dependency on Javascript being enabled on the user's
     * browser.
     */
    public function __construct($report, $request_id) {
        $this->report = $report;
        $this->request_id = Ebms\Util::validate_int($request_id, 'reports');
    }

    /*
     * Routes the report request to the appropriate form.  Also handles
     * routing of URLs which are intended for a higher-level Reports
     * landing page.  For the documents report, handles any additional
     * actions embedded in the URL's request parameters.
     */
    public function run() {

        $parms = drupal_get_query_parameters();
        Ebms\Util::add_js();
        drupal_add_js(Ebms\JS_DIR . '/reports.js', 'file');
        Ebms\Menu::$active = 'Reports';
        drupal_add_css(Ebms\CSS_DIR . '/reports.css');
        switch ($this->report) {
            case 'board-members-last-login':
                return pdq_ebms_board_members_last_login();
            case 'meeting-dates':
            case 'not-list':
            case 'yes-list':
            case 'no-responses':
            case 'citation-followup':
            case 'boards-and-topics':
                return $this->not_yet_implemented();
            case 'responses-by-reviewer':
                return drupal_get_form('pdq_ebms_responses_by_reviewer',
                                       $this->request_id);
            case 'articles-without-responses':
                return drupal_get_form('pdq_ebms_articles_without_responses',
                                       $this->request_id);
            case 'membership':
                return drupal_get_form('pdq_ebms_membership_report_form',
                                       $this->request_id);
            case 'cites-status':
                return drupal_get_form('pdq_ebms_cites_by_status_form',
                                       $this->request_id);
            case 'citations':
                return drupal_get_form('pdq_ebms_citation_reports_form');
            case 'lit-review':
                return drupal_get_form('pdq_ebms_lit_review_report_form',
                    $this->request_id);
            case 'import':
                return drupal_get_form('pdq_ebms_import_reports_form',
                    $this->request_id);
            case 'documents':
                if (isset($parms['archive'])) {
                    $doc_id = Ebms\Util::validate_int($parms['archive'],
                              EbmsReports::$url . '/documents');
                    db_update('ebms_doc')
                        ->fields(array('drop_flag' => 1))
                        ->condition('doc_id', $doc_id)
                        ->execute();
                    unset($_GET['archive']);
                }
                if (isset($parms['edit'])) {
                    $doc_id = Ebms\Util::validate_int($parms['edit'],
                              EbmsReports::$url . '/documents');
                    unset($_GET['edit']);
                    return drupal_get_form('pdq_ebms_doc_edit_form', $doc_id,
                                           $this->request_id);
                }
                return drupal_get_form('pdq_ebms_documents_report_form',
                    $this->request_id);
            case 'board':
                return $this->landing_page('Board Management');
            case 'cite':
                return $this->landing_page('Citation Management');
            case 'failure':
            case 'timeout':
                drupal_set_message(
                    'Report request exceeded available resources.<br />' .
                    'Please use the Back button to return to your request ' .
                    'form and try refining the report filter criteria.',
                    'error');
                // Fall through.
            default:
                return $this->landing_page();
        }
    }

    /**
     * Put up custom stub pages for the reports which haven't yet been
     * implemented so the user doesn't lose the left navigation links
     * for reports.
     */
    private function not_yet_implemented() {
        $reports = array(
            'meeting-dates' => array('Board Management', 'Meeting Dates'),
            'not-list' => array('Citation Management', 'Not List'),
            'yes-list' => array('Citation Management', 'Yes List'),
            'no-responses' => array('Citation Management', 'No Responses'),
            'citation-followup' => array(
                'Citation Management',
                'Citation Follow-up'
            ),
            'boards-and-topics' => array(
                'Citation Management',
                'Board and Topic Lists',
            ),
        );
        $report = $this->report;
        if (!array_key_exists($report, $reports)) {
            drupal_set_message("Unrecognized report URL $report.", 'error');
            drupal_goto('reports');
        }
        list($group, $title) = $reports[$report];
        $words = explode(' ', $group);
        $url = 'reports/' . strtolower($words[0]);
        drupal_set_breadcrumb(
            array(
                l('Reports', 'reports'),
                l($group . ' Reports', $url),
                $title,
            )
        );
        $left_nav = EbmsReports::build_left_nav($group, $title);
        return array(
            '#type' => 'container',
            '#attributes' => array('id' => 'not-implemented'),
            'left-nav' => $left_nav,
            'stub' => array(
                '#type' => 'container',
                '#attributes' => array('id' => 'report-stub'),
                'title' => array(
                    '#type' => 'markup',
                    '#markup' => "<h2>$title</h2>",
                ),
                'explanation' => array(
                    '#type' => 'markup',
                    '#markup' =>
                    '<p>This report is planned for a future release.</p>',
                ),
            ),
        );
    }

    /**
     * Builds the Drupal render array for the Reports landing page.
     * There are possible variations, determining whether a section
     * of the left nav should be expanded.  The page consists of
     * a left navigation column, a title, a Reports landing page
     * image, and links to all of the available reports.  The
     * links below the image are never collapsed, unlike those
     * in the left nav column.
     *
     *  @param string  $open  Passed to build_left_nav() to indicate
     *                        a section of the left navigation
     *                        which should be expanded.
     */
    function landing_page($open=null) {
        $uri = Ebms\IMAGES_DIR . '/reports-image-730x242.jpg';
        drupal_set_breadcrumb(array('Reports'));
        $page = array(
            'left' => EbmsReports::build_left_nav($open),
            'right' => array(
                '#type' => 'container',
                '#attributes' => array('id' => 'reports-right'),
                'title' => array(
                    '#type' => 'markup',
                    '#markup' => '<h2>Reports</h2>',
                ),
                'image' => array(
                    '#theme' => 'image',
                    '#path' => $uri,
                    '#title' => 'EBMS Reports',
                    '#attributes' => array('id' => 'reports-image'),
                ),
                'links' => array(
                    '#type' => 'container',
                    '#attributes' => array('id' => 'reports-right-links'),
                ),
            ),
        );
        $links = EbmsReports::get_nav_links();
        foreach ($links as $parent) {
            list($label, $url, $children) = $parent;
            $items = array(
                array(
                    'data' => l($label, $url),
                    'class' => array('parent'),
                ),
            );
            foreach ($children as $child) {
                list($child_label, $child_url) = $child;
                $items[] = array(
                    'data' => l($child_label, $child_url),
                    'class' => array('child'),
                );
            }
            $page['right']['links'][$label] = array(
                '#theme' => 'item_list',
                '#items' => $items,
            );
        }
        return $page;
    }

    /*
     * Assemble the abstract display to be shown in the Citation column.
     * XXX To be rewritten when Alan has the API for getting the abstract
     *     elements ready.
     */
    static function format_citation($article_id, $counter=null) {
        $xml = db_select('ebms_article', 'a')
            ->fields('a', array('source_data'))
            ->condition('a.article_id', $article_id)
            ->execute()
            ->fetchField();
        $doc = new \SimpleXMLElement($xml);
        $title = $ta = $vol = $issue = $pgn = $year = $pmid = '';
        $article = $doc->MedlineCitation->Article;
        $title = $article->ArticleTitle;
        $pgn = htmlspecialchars($article->Pagination->MedlinePgn);
        $year = $article->Journal->JournalIssue->PubDate->Year;
        $issue = htmlspecialchars($article->Journal->JournalIssue->Issue);
        $vol = htmlspecialchars($article->Journal->JournalIssue->Volume);
        $authors = array();
        if (isset($doc->MedlineCitation->Article->AuthorList->Author)) {
            foreach ($doc->MedlineCitation->Article->AuthorList->Author
                as $author)
            {
                if (sizeof($authors) == 3) {
                    $authors[] = 'et al.';
                    break;
                }
                $name = null;
                if ($author->LastName) {
                    $name = $author->LastName;
                    if ($author->Initials)
                        $name .= ' ' . $author->Initials;
                }
                elseif ($author->CollectiveName)
                    $name = $author->CollectiveName;
                if ($name)
                    $authors[] = htmlspecialchars($name);
            }
        }
        if (count($authors) < 1)
            $authors = array('[No authors listed]');
        $ta = $doc->MedlineCitation->MedlineJournalInfo->MedlineTA;
        $ta = htmlspecialchars($ta);
        $pmid = $doc->MedlineCitation->PMID;
        $full_url = 'citations/full/' . $article_id;
        $title_opts = array(
            'attributes' => array(
                'class' => array('article-title'),
            )
        );
        $title_link = l($title, $full_url, $title_opts);
        $pieces = array('<span class="article-citation">');
        if ($counter)
            $pieces[] = '<span class="article-counter">' . $counter .
                '</span>';
        $pieces[] = implode(', ', $authors) . '<br />';
        $pieces[] = "$title_link<br />$ta $vol";
        if ($issue)
            $pieces[] = "($issue)";
        if ($pgn)
            $pieces[] = ": $pgn";
        if ($year)
            $pieces[] = ", $year";
        $pmurl = 'http://www.ncbi.nlm.nih.gov/pubmed/' . $pmid;
        $pmopts = array('attributes' => array('target' => '_blank'));
        $pieces[] = '.<br />PMID: ' . l($pmid, $pmurl, $pmopts);
        return implode($pieces);
    }

    /**
     * Medical librarians have different reports available than do
     * the rest of the users who can get to the reports.  This
     * method returns a dictionary array containing the links
     * to the reports available for the current user.  The links
     * are used in two places: the left navigation column, and
     * in the list of links below the Reports image on the landing
     * page.
     */
    static function get_nav_links() {

        // Get the URLs for webform-based reports.
        $hotel_report_node = db_select('node', 'n')
            ->fields('n', array('nid'))
            ->condition('n.type', 'webform')
            ->condition('n.title', 'Hotel Request')
            ->execute()
            ->fetchField();
        $reimbursement_report_node = db_select('node', 'n')
            ->fields('n', array('nid'))
            ->condition('n.type', 'webform')
            ->condition('n.title', 'Reimbursement Request')
            ->execute()
            ->fetchField();
        $hotel_report_url = $reimbursement_report_url = 'stub';
        if ($hotel_report_node)
            $hotel_report_url =
                "node/$hotel_report_node/webform-results/table";
        if ($reimbursement_report_node)
            $reimbursement_report_url =
                "node/$reimbursement_report_node/webform-results/table";
        global $user;
        if (in_array('medical librarian', $user->roles))
            $role = 'librarian';
        else
            $role = 'manager';
        $links = array(
            'librarian' => array(
                array(
                    'Board Management',
                    'reports/board',
                    array(
                        array('Documents', 'reports/documents'),
                        array('Meeting Dates', 'reports/meeting-dates'),
                    ),
                ),
                array(
                    'Citation Management',
                    'reports/cite',
                    array(
                        array('Citation Reports', 'reports/citations'),
                        array('Import Reports', 'reports/import'),
                        array('Not List', 'reports/not-list'),
                        array('Yes List', 'reports/yes-list'),
                    ),
                ),
            ),
            'manager' => array(
                array(
                    'Board Management',
                    'reports/board',
                    array(
                        array('Documents', 'reports/documents'),
                        array('Meeting Dates', 'reports/meeting-dates'),
                        array('Hotel Requests', $hotel_report_url),
                        array(
                            'Reimbursement Requests',
                            $reimbursement_report_url,
                        ),
                        array('Board Membership', 'reports/membership'),
                        array(
                            'Board Member Activity Report',
                            'reports/board-members-last-login',
                        ),
                    ),
                ),
                array(
                    'Citation Management',
                    'reports/cite',
                    array(
                        array('Import Reports', 'reports/import'),
                        array('Citation Reports', 'reports/citations'),
                        array('Citations By Status', 'reports/cites-status'),
                        array('Literature Reviews', 'reports/lit-review'),
                        array(
                            'Responses By Reviewer',
                            'reports/responses-by-reviewer',
                        ),
                        array(
                            'Articles Without Responses',
                            'reports/articles-without-responses',
                        ),
                        array(
                            'Citation Follow-up',
                            'reports/citation-followup',
                        ),
                        array(
                            'Board and Topic Lists',
                            'reports/boards-and-topics',
                        ),
                        array('Not List', 'reports/not-list'),
                        array('Yes List', 'reports/yes-list'),
                    ),
                ),
            ),
        );
        return $links[$role];
    }

    /**
     * Creates the render array for the left nav panel for the Reports
     * pages.
     *
     *  @param string $open    Name of the group in the navigation
     *                         column whose child links should be
     *                         shown; null if all groups are to be
     *                         collapsed
     *  @param string $active  Current action, highlighted by CSS
     *
     *  @return                Render array for left nav panel
     */
    public static function build_left_nav($open=null, $active=null) {

        $links = EbmsReports::get_nav_links();
        $items = array();
        foreach ($links as $parent) {
            list($label, $url, $children) = $parent;
            $link = l($label, $url);
            $class = array(
                'parent',
                $label == $active ? 'active' : 'inactive',
            );
            if ($open == $label)
                $class[] = 'open';
            $items[] = array(
                'data' => $link,
                'class' => $class,
            );
            if ($open == $label) {
                foreach ($children as $child) {
                    list($label, $url) = $child;
                    $link = l($label, $url);
                    $items[] = array(
                        'data' => $link,
                        'class' => array(
                            'child',
                            $label == $active ? 'active' : 'inactive',
                        ),
                    );
                }
            }
        }
        return array(
            '#type' => 'container',
            '#attributes' => array(
                'id' => 'left-nav',
                'class' => array('report-left-nav'),
            ),
            'nav' => array(
                '#theme' => 'item_list',
                '#items' => $items,
            ),
        );
    }

    /**
     * Builds the Drupal render array for the report on published
     * citations.  "Published" in this context means release for the
     * board manager's initial review (from the abstract).
     * One of several "Citations" reports, invoked by the function
     * pdq_ebms_citation_reports_form().
     *
     *  @param  int    $board   ID of board (if the user specified
     *                          one for the report)
     *  @param  int    $cycle   ID of the cycle used for filtering
     *                          the report
     *
     *  @return array           Drupal render array for the report
     */
    public static function cites_published($board, $cycle) {

        // Will be caught by validation.
        $cycle_name = Ebms\Util::get_cycle_name($cycle);
        if (!$cycle_name) {
            drupal_set_message('Cycle is required for this report', 'error');
            return array();
        }

        // Start the render array with the title and meta data.
        $d = date('m/d/Y');
        $report = array(
            '#type' => 'container',
            '#attributes' => array(
                'id' => 'report',
                'class' => array('cites-pub'),
            ),
            'title' => array(
                '#type' => 'markup',
                '#markup' => '<h2>Citations Published</h2>' .
                "<div class='meta'><span class='label'>Date:</span> $d</div>" .
                "<div class='meta'><span class='label'>Review cycle:</span> " .
                $cycle_name . '</div>',
            )
        );

        // Construct the report's query.
        $published = Ebms\Util::state_id_from_text_id('Published');
        $query = db_select('ebms_article_state', 's');
        $query->join('ebms_board', 'b', 'b.board_id = s.board_id');
        $query->join('ebms_article_topic', 'a',
            'a.article_id = s.article_id AND a.topic_id = s.topic_id');
        $query->condition('a.cycle_id', $cycle);
        $query->condition('s.state_id', $published);
        $query->fields('b', array('board_name'));
        $query->groupBy('b.board_name');
        if ($board)
            $query->condition('s.board_id', $board);

        // Branch off at this point for a separate version of the query
        // to get the total number of articles for each board (which we
        // can't get by summing the per-board/topic counts, because an
        // article can be represented in those counts more than once).
        $total_query = clone($query);
        $total_query->addExpression('COUNT(DISTINCT a.article_id)', 'n');
        $results = $total_query->execute();
        $totals = array();
        foreach ($results as $result)
            $totals[$result->board_name] = $result->n;

        // Resume with the rest of the original query, from which we
        // need more granularity.
        $query->join('ebms_topic', 't', 't.topic_id = s.topic_id');
        $query->fields('t', array('topic_name'));
        $query->groupBy('t.topic_name');
        $query->addExpression('COUNT(*)', 'n');
        $query->orderBy('b.board_name');
        $query->orderBy('t.topic_name');

        // Collect the query result in an array indexed by board.
        $results = $query->execute();
        $stats = array();
        foreach ($results as $result) {
            $board_name = $result->board_name;
            if (!array_key_exists($board_name, $stats))
                $stats[$board_name] = array();
            $stats[$board_name][$result->topic_name] = $result->n;
        }

        // Add a section for each board, showing counts for "publications"
        // by topic.
        foreach ($stats as $board_name => &$topic_array) {
            $rows = array();
            foreach ($topic_array as $topic_name => $count)
                $rows[] = array($topic_name, $count);
            $rows[] = array('TOTAL', $totals[$board_name]);
            $report[$board_name] = array(
                '#theme' => 'table',
                '#caption' => $board_name,
                '#header' => array('Topic Name', 'Articles Published'),
                '#rows' => $rows,
            );
        }
        return $report;
    }

    /**
     * Build a statistical report of the counts of articles imported
     * for each topic.
     * One of several "Citations" reports, invoked by the function
     * pdq_ebms_citation_reports_form().
     *
     *  @param  int    $board   ID of board (if the user specified
     *                          one for the report)
     *  @param  int    $cycle   ID of the cycle used for filtering
     *                          the report
     *
     *  @return array           Drupal render array for the report
     */
    public static function cites_imported($board, $cycle) {

        // Will be caught by validation function.
        $cycle_name = Ebms\Util::get_cycle_name($cycle);
        if (!$cycle_name) {
            drupal_set_message('Cycle is required for this report', 'error');
            return array();
        }

        // Start the render array with the title and meta data.
        $d = date('m/d/Y');
        $report = array(
            '#type' => 'container',
            '#attributes' => array(
                'id' => 'report',
                'class' => array('cites-imported'),
            ),
            'title' => array(
                '#type' => 'markup',
                '#markup' => '<h2>Citations Imported</h2>' .
                "<div class='meta'><span class='label'>Date:</span> $d</div>" .
                "<div class='meta'><span class='label'>Review cycle:</span> " .
                $cycle_name . '</div>',
            )
        );

        // Construct the report's query.
        $state_id = Ebms\Util::state_id_from_text_id('ReadyInitReview');
        $query = db_select('ebms_article_state', 's');
        $query->join('ebms_board', 'b', 'b.board_id = s.board_id');
        $query->join('ebms_article_topic', 'a',
            'a.article_id = s.article_id AND a.topic_id = s.topic_id');
        $query->condition('a.cycle_id', $cycle);
        $query->condition('s.state_id', $state_id);
        $query->fields('b', array('board_name'));
        $query->groupBy('b.board_name');
        if ($board)
            $query->condition('s.board_id', $board);

        // Branch off at this point for a separate version of the query
        // to get the total number of articles for each board (which we
        // can't get by summing the per-board/topic counts, because an
        // article can be represented in those counts more than once).
        $total_query = clone($query);
        $total_query->addExpression('COUNT(DISTINCT a.article_id)', 'n');
        $results = $total_query->execute();
        $totals = array();
        foreach ($results as $result)
            $totals[$result->board_name] = $result->n;

        // Resume with the rest of the original query, from which we
        // need more granularity.
        $query->join('ebms_topic', 't', 't.topic_id = s.topic_id');
        $query->fields('t', array('topic_name'));
        $query->groupBy('t.topic_name');
        $query->addExpression('COUNT(*)', 'n');
        $query->orderBy('b.board_name');
        $query->orderBy('t.topic_name');

        // Collect the query result in an array indexed by board.
        $results = $query->execute();
        $stats = array();
        foreach ($results as $result) {
            $board_name = $result->board_name;
            if (!array_key_exists($board_name, $stats))
                $stats[$board_name] = array();
            $stats[$board_name][$result->topic_name] = $result->n;
        }

        // Add a section for each board, showing counts for imports
        // by topic.
        foreach ($stats as $board_name => &$topic_array) {
            $rows = array();
            foreach ($topic_array as $topic_name => $count)
                $rows[] = array($topic_name, $count);
            $rows[] = array('TOTAL', $totals[$board_name]);
            $report[$board_name] = array(
                '#theme' => 'table',
                '#caption' => $board_name,
                '#header' => array('Topic Name', 'Articles Imported'),
                '#rows' => $rows,
            );
        }
        return $report;
    }

    /**
     * Produces a three-column report, in sections by board, showing
     * each topic for which any initial winnowing decisions were made
     * by the librarians, the number of articles rejected for the topic,
     * and the number accepted.
     * One of several "Citations" reports, invoked by the function
     * pdq_ebms_citation_reports_form().
     *
     *  @param  int    $board   ID of board (if the user specified
     *                          one for the report)
     *  @param  int    $cycle   ID of the cycle used for filtering
     *                          the report
     *
     *  @return array           Drupal render array for the report
     */
    public static function initial_decision($board, $cycle) {

        // Will be caught by validation function.
        $cycle_name = Ebms\Util::get_cycle_name($cycle);
        if (!$cycle_name) {
            drupal_set_message('Cycle is required for this report', 'error');
            return array();
        }

        // Start the render array with the title and meta data.
        $d = date('m/d/Y');
        $report = array(
            '#type' => 'container',
            '#attributes' => array(
                'id' => 'report',
                'class' => array('initial-decision'),
            ),
            'title' => array(
                '#type' => 'markup',
                '#markup' =>
                '<h2>Citations Rejected/Accepted for Publishing</h2>' .
                "<div class='meta'><span class='label'>Date:</span> $d</div>" .
                "<div class='meta'><span class='label'>Review cycle:</span> " .
                $cycle_name . '</div>',
            )
        );

        // Construct the report's query.
        $rejected = Ebms\Util::state_id_from_text_id('RejectInitReview');
        $accepted = Ebms\Util::state_id_from_text_id('PassedInitReview');
        $state_ids = array($rejected, $accepted);
        $query = db_select('ebms_article_state', 's');
        $query->join('ebms_board', 'b', 'b.board_id = s.board_id');
        $query->join('ebms_article_topic', 'a',
            'a.article_id = s.article_id AND a.topic_id = s.topic_id');
        $query->condition('a.cycle_id', $cycle);
        $query->condition('s.state_id', $state_ids, 'IN');
        $query->fields('b', array('board_name'));
        $query->fields('s', array('state_id'));
        $query->groupBy('b.board_name');
        $query->groupBy('s.state_id');
        if ($board)
            $query->condition('s.board_id', $board);

        // Branch off at this point for a separate version of the query
        // to get the total number of articles for each board (which we
        // can't get by summing the per-board/topic counts, because an
        // article can be represented in those counts more than once).
        $total_query = clone($query);
        $total_query->addExpression('COUNT(DISTINCT a.article_id)', 'n');
        $results = $total_query->execute();
        $totals = array();
        foreach ($results as $result) {
            if (!array_key_exists($result->board_name, $totals))
                $totals[$result->board_name] = array();
            $totals[$result->board_name][$result->state_id] = $result->n;
        }

        // Resume with the rest of the original query, from which we
        // need more granularity.
        $query->join('ebms_topic', 't', 't.topic_id = s.topic_id');
        $query->fields('t', array('topic_name'));
        $query->groupBy('t.topic_name');
        $query->addExpression('COUNT(*)', 'n');
        $query->orderBy('b.board_name');
        $query->orderBy('t.topic_name');

        // Collect the query result in an array indexed by board.
        $results = $query->execute();
        $stats = array();
        foreach ($results as $result) {
            $board_name = $result->board_name;
            $topic_name = $result->topic_name;
            $state_id = $result->state_id;
            if (!array_key_exists($board_name, $stats))
                $stats[$board_name] = array();
            if (!array_key_exists($topic_name, $stats[$board_name]))
                $stats[$board_name][$topic_name] = array(
                    $rejected => 0,
                    $accepted => 0,
                );
            $stats[$board_name][$topic_name][$state_id] = $result->n;
        }

        // Add a section for each board, showing counts for librarian
        // rejections and passes.
        foreach ($stats as $board_name => &$topic_array) {
            $rows = array();
            $total_rejected = $total_accepted = 0;
            if (isset($totals[$board_name][$rejected]))
                $total_rejected = $totals[$board_name][$rejected];
            if (isset($totals[$board_name][$accepted]))
                $total_accepted = $totals[$board_name][$accepted];
            foreach ($topic_array as $topic_name => &$n)
                $rows[] = array($topic_name, $n[$rejected], $n[$accepted]);
            $rows[] = array('TOTAL', $total_rejected, $total_accepted);
            $report[$board_name] = array(
                '#theme' => 'table',
                '#caption' => $board_name,
                '#header' => array('Topic Name', 'Rejected', 'Accepted'),
                '#rows' => $rows,
            );
        }
        return $report;
    }

    /**
     * Build a report showing all of the article-topic combinations
     * rejected by the board manager in her first pass.
     * One of several "Citations" reports, invoked by the function
     * pdq_ebms_citation_reports_form().
     *
     *  @param  int    $board   ID of board (if the user specified
     *                          one for the report)
     *  @param  int    $cycle   ID of the cycle used for filtering
     *                          the report
     *  @param  array  $start   Drupal form date field for the beginning
     *                          of a filtering date range
     *  @param  array  $end     Drupal form date field for the end
     *                          of a filtering date range
     *
     *  @return array           Drupal render array for the report
     */
    public static function manager_rejection($board, $cycle, $start, $end) {

        // Build up the markup for the report's title.
        $title = '<h2>Citations Not Selected for Full Text Retrieval</h2>';
        $title .= EbmsReports::format_report_date();
        if ($board) {
            $board_name = Ebms\Util::get_board_name($board);
            $title .= EbmsReports::format_board($board_name);
        }
        if ($cycle)
            $title .= EbmsReports::format_cycle_name($cycle);
        else {
            $range = EbmsReports::format_date_range($start, $end);
            if (!$range) {
                drupal_set_message('Must specify cycle or date range',
                    'error');
                return array();
            }
            $title .= $range;
        }

        // Start the render array with the title.
        $report = array(
            '#type' => 'container',
            '#attributes' => array(
                'id' => 'report',
                'class' => array('manager-rejection'),
            ),
            'title' => array(
                '#type' => 'markup',
                '#markup' => $title,
            )
        );

        // Construct the query for the report.
        $state_id = Ebms\Util::state_id_from_text_id('RejectBMReview');
        $header = array(
            'EBMS ID',
            'PMID',
            array('data' => 'Topic(s)', 'class' => array('col-3')),
            array('data' => 'NCI Reviewer', 'class' => array('col-4')),
            'Comments',
        );
        $query = db_select('ebms_article_state', 's');
        $query->join('ebms_topic', 't', 't.topic_id = s.topic_id');
        $query->join('ebms_article', 'a', 'a.article_id = s.article_id');
        $query->join('users', 'u', 's.user_id = u.uid');
        $query->fields('a', array('article_id', 'source_id'));
        $query->fields('t', array('topic_name'));
        $query->fields('u', array('name'));
        $query->fields('s', array('article_state_id'));
        if ($board)
            $query->condition('s.board_id', $board);
        if ($cycle) {
            $query->join('ebms_article_topic', 'c',
                'c.article_id = s.article_id AND c.topic_id = s.topic_id');
            $query->condition('c.cycle_id', $cycle);
        }
        else {
            if (!Ebms\Util::add_date_range_to_query($query, $start, $end,
                    's.status_dt')) {
                drupal_set_message('Failure restricting query by range',
                    'error');
                return array();
            }
        }
        $query->condition('s.state_id', $state_id);
        $query->orderBy('a.article_id');
        $query->orderBy('t.topic_name');

        // Prep for links to NLM's PubMed.
        $pubmed_base = 'http://www.ncbi.nlm.nih.gov/entrez/query.fcgi?' .
            'cmd=Retrieve&db=PubMed&dopt=abstract&list_uids=';
        $pubmed_opts = array('attributes' => array('target' => '_blank'));

        // Walk through the query results, assembling the rows for the
        // report table.
        $results = $query->execute();
        $rows = array();
        $last_article_id = null;
        foreach ($results as $result) {
            $article_state_id = $result->article_state_id;
            $article_id = $result->article_id;
            $pmid = $result->source_id;
            $col3 = $result->topic_name;
            $col4 = $result->name;
            $comment_rows = db_select('ebms_article_state_comment', 'c')
                ->fields('c', array('comment'))
                ->condition('c.article_state_id', $article_state_id)
                ->orderBy('c.comment_id')
                ->execute();
            $comments = array();
            foreach ($comment_rows as $comment_row)
                $comments[] = $comment_row->comment;
            $col5 = implode('; ', $comments);
            if ($article_id != $last_article_id) {
                $col1 = l($article_id, 'citations/full/' . $article_id);
                $col2 = l($pmid, $pubmed_base . $pmid, $pubmed_opts);
                $last_article_id = $article_id;
            }
            else
                $col1 = $col2 = '&nbsp;';
            $rows[] = array(
                array('data' => $col1, 'class' => array('col-1')),
                array('data' => $col2, 'class' => array('col-2')),
                array('data' => $col3, 'class' => array('col-3')),
                array('data' => $col4, 'class' => array('col-4')),
                array('data' => $col5, 'class' => array('col-5')),
            );
        }
        $report['table'] = array(
            '#theme' => 'table',
            '#header' => $header,
            '#rows' => $rows,
        );
        return $report;
    }

    /**
     * Generates the HTML markup for the list of topics associated with
     * an article for a particular article state.  Used by the report
     * which compares the topics chosen by the librarians with those
     * approved by the board manager (see function below).
     *
     *  @param  int  $article_id  Primary key into the ebms_article table
     *  @param  int  $state_id    Primary key into the ebms_article_state_type
     *                            table
     *
     *  @return                   String containing markup for list of
     *                            topics
     */
    static function get_topics($article_id, $state_id) {
        $query = db_select('ebms_topic', 't');
        $query->join('ebms_article_state', 's', 's.topic_id = t.topic_id');
        $query->fields('t', array('topic_name'));
        $query->condition('s.article_id', $article_id);
        $query->condition('s.state_id', $state_id);
        $query->distinct();
        $query->orderBy('t.topic_name');
        $results = $query->execute();
        $topics = array();
        foreach ($results as $result)
            $topics[] = htmlspecialchars($result->topic_name);
        return implode('<br />', $topics);
    }

    /**
     * Build a report showing all of the articles for which the board
     * manager has added topics which weren't in the set of topics
     * approved for the article by the librarians.
     * One of several "Citations" reports, invoked by the function
     * pdq_ebms_citation_reports_form().  The date range represented
     * by the parameters is used to find matching states created by
     * the board managers.
     *
     *  @param  array  $start   Drupal form date field for the beginning
     *                          of a filtering date range
     *  @param  array  $end     Drupal form date field for the end
     *                          of a filtering date range
     *
     *  @return array           Drupal render array for the report
     */
    public static function topic_changes($start, $end) {


        // Build up the markup for the report's title.
        $title = '<h2>Citation Summary Topic Changes</h2>';
        $title .= EbmsReports::format_report_date();
        $title .= EbmsReports::format_date_range($start, $end);
        $report = array(
            '#type' => 'container',
            '#attributes' => array(
                'id' => 'report',
                'class' => array('topic-changes'),
            ),
            'title' => array(
                '#type' => 'markup',
                '#markup' => $title,
            )
        );

        // Construct the query for the report.
        $manager_approval = Ebms\Util::state_id_from_text_id('PassedBMReview');
        $librarian_approval =
            Ebms\Util::state_id_from_text_id('PassedInitReview');
        $header = array(
            array('data' => 'EBMS ID', 'class' => array('col-1')),
            array('data' => 'PMID', 'class' => array('col-2')),
            array(
                'data' => 'After Librarian Review',
                'class' => array('col-3'),
            ),
            array('data' => 'After NCI Review', 'class' => array('col-4')),
        );
        $query = db_select('ebms_article_state', 's1');
        $query->join('ebms_article', 'a', 'a.article_id = s1.article_id');
        $query->leftJoin('ebms_article_state', 's0',
            's1.article_id = s0.article_id AND s1.topic_id = s0.topic_id ' .
            'AND s0.state_id = ' . $librarian_approval);
        $query->condition('s1.state_id', $manager_approval);
        $query->isNull('s0.article_id');
        $query->fields('a', array('article_id', 'source_id'));
        $query->distinct();
        if (!Ebms\Util::add_date_range_to_query($query, $start, $end,
            's1.status_dt')) {
            drupal_set_message('Failure restricting by range', 'error');
            return array();
        }
        $query->orderBy('a.article_id');

        // Prep for links to NLM's PubMed.
        $pubmed_base = 'http://www.ncbi.nlm.nih.gov/entrez/query.fcgi?' .
            'cmd=Retrieve&db=PubMed&dopt=abstract&list_uids=';
        $pubmed_opts = array('attributes' => array('target' => '_blank'));

        // Walk through the query results, assembling the rows for the
        // report table.
        $results = $query->execute();
        $rows = array();
        foreach ($results as $result) {
            $article_id = $result->article_id;
            $pmid = $result->source_id;
            $col1 = l($article_id, 'citations/full/' . $article_id);
            $col2 = l($pmid, $pubmed_base . $pmid, $pubmed_opts);
            $col3 = EbmsReports::get_topics($article_id, $librarian_approval);
            $col4 = EbmsReports::get_topics($article_id, $manager_approval);
            $rows[] = array(
                array('data' => $col1, 'class' => array('col-1')),
                array('data' => $col2, 'class' => array('col-2')),
                array('data' => $col3, 'class' => array('col-3')),
                array('data' => $col4, 'class' => array('col-4')),
            );
        }
        $report['table'] = array(
            '#theme' => 'table',
            '#header' => $header,
            '#rows' => $rows,
        );
        return $report;
    }

    /**
     * Display the list of import jobs matching the report criteria.
     * This display is the interim display for the "Import Reports"
     * page.
     *
     *  @param  int    $board   Primary key for ebms_board table
     *  @param  int    $topic   Primary key for ebms_topic table
     *  @param  int    $cycle   Primary key for ebms_cycle table
     *  @param  array  $start   Drupal form date array for beginning
     *                          of date range for filtering
     *  @param  array  $end     Drupal form date array for end
     *                          of date range for filtering
     *  @param  bool   $range   Flag indicating whether both dates
     *                          ($start and $end) should be used
     *                          for filtering
     *
     *  @return array           Drupal render array for the list
     *                          of import batch jobs
     */
    public static function import_reports($board, $topic, $cycle, $start,
                                          $end, $range) {

        // Construct the query object for the report.
        $query = db_select('ebms_import_batch', 'j');
        $query->join('ebms_import_action', 'a',
            'a.import_batch_id = j.import_batch_id');
        $query->join('ebms_topic', 't', 't.topic_id = j.topic_id');
        $query->join('ebms_board', 'b', 'b.board_id = t.board_id');
        if ($board)
            $query->condition('b.board_id', $board);
        if ($topic)
            $query->condition('t.topic_id', $topic);
        if ($cycle)
            $query->condition('j.cycle_id', $cycle);
        elseif ($range) {
            if (!Ebms\Util::add_date_range_to_query($query, $start, $end,
                    'j.import_date')) {
                drupal_set_message('Failure restricting query by range',
                    'error');
                return array();
            }
        }
        else
            Ebms\Util::add_date_to_query($query, $start, 'j.import_date');
        $query->fields('j', array('import_batch_id', 'import_date'));
        $query->fields('t', array('topic_name'));
        $query->fields('b', array('board_name'));
        $query->addExpression('COUNT(DISTINCT a.source_id)', 'citations');
        $query->groupBy('j.import_batch_id');
        $query->groupBy('j.import_date');
        $query->groupBy('t.topic_name');
        $query->groupBy('b.board_name');
        $query->orderBy('j.import_date');

        // Execute the query and assemble the list of items for the display.
        $results = $query->execute();
        $items = array();
        foreach ($results as $result) {
            $date = date('m/d/Y', strtotime($result->import_date));
            $board_name = $result->board_name;
            $topic_name = $result->topic_name;
            $count = $result->citations . ' citation';
            if ($result->citations != 1)
                $count .= 's';
            $line = "$date | $board_name | $topic_name | $count";
            $items[] = l($line, 'reports/import/' . $result->import_batch_id);
        }

        // Assemble and return the render array for the list.
        return array(
            '#type' => 'container',
            '#attributes' => array(
                'id' => 'report',
                'class' => array('import-reports'),
            ),
            'title' => array(
                '#type' => 'markup',
                '#markup' => '<h2>Import Reports (' . count($items) . ')</h2>',
            ),
            'reports' => array(
                '#theme' => 'item_list',
                '#items' => $items,
                '#attributes' => array('class' => array('no-bullets')),
            ),
        );
    }

    /**
     * Builds the Drupal render array for a report on an article import
     * job.  This method is almost trivial, since we're using the
     * common logic built for showing the same information immediately
     * after the job was run.
     *
     *  @param  int  $id      Primary key into the EBMS table used to
     *                        record import batches
     *
     *  @return array         Drupal render array for the report.
     */
    public static function import_job($id) {
        require_once('EbmsImport.inc');
        $batch = new Ebms\ImportBatch($id);
        $title = 'Import Batch ' . $id;
        return Ebms\Util::import_report($batch, $title);
        return;
        return array(
            '#type' => 'container',
            '#attributes' => array(
                'id' => 'report',
                'class' => array('import-batch-report'),
            ),
            'title' => array(
                '#type' => 'markup',
                '#markup' => '<h2>Import Batch ' . $id . '</h2>',
            ),
        );
    }

    /**
     * Builds up the Drupal render array for a report on documents
     * posted to the EBMS.
     *
     *  @param  int  $request_id   Primary key into the EBMS table
     *                             holding the parameters for the
     *                             report request.
     *
     *  @return array              Drupal render array for the
     *                             report.
     */
    public static function docs($request_id) {

        // Collect the report parameters.
        $board = $tag = $keyword = $inhouse = $member = $archived =
            $range = $uploader_board = null;
        $start = $end = Ebms\Util::empty_date();
        if ($request_id) {
            $params = EbmsReports::fetch_report_request($request_id);
            $board = $params['board'];
            $tag = $params['tag'];
            $keyword = $params['keyword'];
            $inhouse = $params['inhouse'];
            $member = $params['member'];
            $uploader_board = $params['uploader-board'];
            $archived = $params['archived'];
            $start = $params['upload-start'];
            $range = $params['use-upload-date-range'];
            if ($range)
                $end = $params['upload-end'];
            //pdq_ebms_debug('DOCS PARAMS', $params);
        }

        // Find out whether we're paging the results.
        $pager_limit = 10;
        $pager = true;
        $parms = drupal_get_query_parameters();
        if (isset($parms['pager']))
            if ($parms['pager'] == 'off')
                $pager = false;

        // Create the table column headers, specifying the column which
        // can be used for sorting.
        $headers = array(
            array(
                'data' => 'File Name',
                'field' => 'f.filename',
                'class' => array('col-1'),
            ),
            array(
                'data' => 'Uploaded By',
                'class' => array('col-2'),
            ),
            array(
                'data' => 'Boards',
                'class' => array('col-3'),
            ),
            array(
                'data' => 'Tags',
                'class' => array('col-4'),
            ),
            array(
                'data' => 'Date Uploaded',
                'class' => array('col-5'),
            ),
            array(
                'data' => 'Actions',
                'class' => array('col-6'),
            ),
        );

        // Construct the query for the report.
        $query = db_select('ebms_doc', 'd');
        $query = $query->extend('TableSort');
        if ($pager)
            $query = $query->extend('PagerDefault')->limit($pager_limit);
        $query->join('file_managed', 'f', 'f.fid = d.file_id');
        $query->join('users', 'u', 'u.uid = f.uid');
        $query->fields('f', array('filename', 'uri'));
        $query->fields('d', array('when_posted', 'doc_id', 'drop_flag'));
        $query->fields('u', array('name'));
        if (!$archived)
            $query->condition('d.drop_flag', 0);
        if ($board) {
            $query->join('ebms_doc_board', 'b', 'b.doc_id = d.doc_id');
            $query->condition('b.board_id', $board);
        }

        // Added for TIR 2441.
        if ($uploader_board) {
            $role = db_select('role', 'r')
                ->fields('r', array('rid'))
                ->condition('r.name', 'board member')
                ->execute()
                ->fetchField();
            $query->join('ebms_board_member', 'm', 'm.user_id = u.uid');
            $query->condition('m.board_id', $uploader_board);
            $query->join('users_roles', 'r', 'r.uid = u.uid');
            $query->condition('r.rid', $role);
        }
        if ($tag) {
            $query->join('ebms_doc_tag', 't', 't.doc_id = d.doc_id');
            $query->condition('t.tag_id', $tag);
        }
        if ($range)
            Ebms\Util::add_date_range_to_query($query, $start, $end,
                'd.when_posted');
        else
            Ebms\Util::add_date_to_query($query, $start, 'd.when_posted');
        if ($inhouse)
            $query->condition('u.uid', $inhouse);
        if ($member)
            $query->condition('u.uid', $member);
        $query->orderByHeader($headers);
        $count_query = $query->countQuery();
        $total = $count_query->execute()->fetchField();

        // Collect information we'll use repeatedly in the loop.
        $divider = '<span class="action-divider">|</span>';
        $main_url = 'reports/documents';
        if ($request_id)
            $main_url .= "/$request_id";
        $edit_parms = drupal_get_query_parameters();
        $archive_parms = drupal_get_query_parameters();

        // Build the array of table rows.
        //pdq_ebms_debug('DOC REPORT QUERY', 'query: ' . $query);
        $docs = $query->execute();
        $rows = array();
        $target = array('attributes' => array('target' => '_blank'));
        foreach ($docs as $d) {
            $edit_parms['edit'] = $d->doc_id;
            $links = l('EDIT', $main_url, array('query' => $edit_parms));

            // Robin decided she didn't like the original requirement
            // for this link.  See TIR 2450.
            $links = l('EDIT', 'docs/edit/' . $d->doc_id);
            if (!$d->drop_flag) {
                $archive_parms['archive'] = $d->doc_id;
                $archive_link = l('ARCHIVE', $main_url,
                                array('query' => $archive_parms));
                $links .= " $divider $archive_link";
            }
            $bquery = db_select('ebms_board', 'b');
            $bquery->join('ebms_doc_board', 'd', 'd.board_id = b.board_id');
            $bquery->condition('d.doc_id', $d->doc_id);
            $bquery->fields('b', array('board_name'));
            $bquery->orderBy('b.board_name');
            $results = $bquery->execute();
            $boards = array();
            foreach ($results as $result)
                $boards[] = $result->board_name;
            $boards = implode('; ', $boards);
            $tquery = db_select('ebms_tag', 't');
            $tquery->join('ebms_doc_tag', 'd', 't.tag_id = d.tag_id');
            $tquery->condition('d.doc_id', $d->doc_id);
            $tquery->fields('t', array('tag_name'));
            $tquery->orderBy('t.tag_name');
            $results = $tquery->execute();
            $tags = array();
            foreach ($results as $result)
                $tags[] = $result->tag_name;
            $tags = implode('; ', $tags);
            $file_url = file_create_url($d->uri);
            $data = array(
                array(
                    'data' => l($d->filename, $file_url, $target),
                    'class' => array('col-1'),
                ),
                array(
                    'data' => htmlspecialchars($d->name),
                    'class' => array('col-2'),
                ),
                array(
                    'data' => htmlspecialchars($boards),
                    'class' => array('col-3'),
                ),
                array(
                    'data' => htmlspecialchars($tags),
                    'class' => array('col-4'),
                ),
                array(
                    'data' => date('m/d/Y', strtotime($d->when_posted)),
                    'class' => array('col-5'),
                ),
                array(
                    'data' => $links,
                    'class' => array('col-6'),
                ),
            );
            $rows[] = array(
                'data' => $data,
                'no_striping' => true,
            );
        }

        // Assemble the table.
        $id = 'doc-report-title';
        $table = array(
            'title' => array(
                '#type' => 'markup',
                '#markup' => "<h2 id='$id'>Documents ($total)</h2>",
            ),
            'table' => array(
                '#attributes' => array('id' => 'doc-report'),
                '#theme' => 'table',
                '#rows' => $rows,
                '#header' => $headers,
            ),
        );
        if ($pager)
            $table['pager'] = array('#theme' => 'pager');
        elseif ($total > $pager_limit) {
            unset($parms['pager']);
            $opts = array('query' => $parms);
            $url = EbmsReports::$url . '/documents';
            if ($request_id)
                $url .= '/' . $request_id;
            $link = l('VIEW PAGES', $url, $opts);
            $table['pager'] = array(
                '#type' => 'markup',
                '#markup' => "<div id='pager-view-pages'>$link</div>",
            );
        }
        return $table;
    }


    /**
     * Create the Drupal render array for a report on articles whose
     * current status is approval or rejection by the NCI reviewer
     * based on the articles' abstractions
     *
     * This report is one of eight different reports on articles by
     * status.  Called by the cites_by_status() method below.
     *
     *  @param  array  $params   Values specified by the user
     *                           requesting the report, to be
     *                           used for selecting the articles
     *                           to appear on that report.
     *
     *  @return array            Drupal render array for the report
     *                           table and its title.
     */
    static function cites_by_status_rd($params) {

        // Create the table column headers, specifying the column which
        // can be used for sorting.
        $headers = array(
            '',
            array(
                'data' => 'Citation',
                'field' => 'a.last_name',
                'class' => array('col-1'),
            ),
        );
        if (!$params['disposition'])
            $headers[] = 'Disposition';
        $headers[] = 'Comments';
        $headers[] = 'Review Cycle';
        $headers[] = 'Date Reviewed';
        $headers[] = 'Reviewer';

        // Construct the query for the report.
        $passed = Ebms\Util::state_id_from_text_id('PassedBMReview');
        $rejected = Ebms\Util::state_id_from_text_id('RejectBMReview');
        $query = db_select('ebms_article_state', 's');
        $state_fields = array('article_id', 'article_state_id', 'status_dt');
        $query = $query->extend('TableSort');
        $query->join('ebms_article_topic', 't',
                't.article_id = s.article_id AND t.topic_id = s.topic_id');
        $query->join('users', 'u', 'u.uid = s.user_id');
        $query->leftJoin('ebms_article_author_cite', 'c',
            'c.article_id = s.article_id AND c.cite_order = 1');
        $query->leftJoin('ebms_article_author', 'a',
            'a.author_id = c.author_id');
        $query->condition('s.board_id', $params['board']);
        if ($params['topic'])
            $query->condition('s.topic_id', $params['topic']);
        $query->condition('s.current', 'Y');
        if (count($params['cycles']))
            $query->condition('t.cycle_id', $params['cycles'], 'IN');
        $query->join('ebms_cycle', 'c2', 'c2.cycle_id = t.cycle_id');
        $query->fields('c2', array('cycle_name'));
        switch ($params['disposition']) {
        case 'yes':
            $query->condition('s.state_id', $passed);
            break;
        case 'no':
            $query->condition('s.state_id', $rejected);
            break;
        default:
            $query->condition('s.state_id', array($passed, $rejected), 'IN');
            $state_fields[] = 'state_id';
            break;
        }
        $query->fields('s', $state_fields);
        $query->fields('u', array('name'));
        $query->orderByHeader($headers);
        $query->distinct();

        // Execute the query and assemble the rows for the report table.
        $results = $query->execute();
        $counter = 0;
        $rows = array();
        foreach ($results as $result) {
            $article_id = $result->article_id;
            $article_state_id = $result->article_state_id;
            $status_dt = strtotime($result->status_dt);
            $reviewer = htmlspecialchars($result->name);
            $citation = EbmsReports::format_citation($article_id);
            $comment_rows = db_select('ebms_article_state_comment', 'c')
                ->fields('c', array('comment'))
                ->condition('c.article_state_id', $article_state_id)
                ->orderBy('c.comment_dt')
                ->execute();
            $comments = array();
            foreach ($comment_rows as $comment_row)
                $comments[] = htmlspecialchars($comment_row->comment);
            $comments = implode('<br />', $comments);
            $row = array(
                array(
                    'data' => ++$counter,
                    'class' => array('col-0'),
                ),
                array(
                    'data' => $citation,
                    'class' => array('col-1'),
                ),
            );
            if (!$params['disposition'])
                $row[] = $result->state_id == $passed ? 'Yes' : 'No';
            $row[] = $comments;
            $row[] = $result->cycle_name;
            $row[] = date('m/d/Y', $status_dt);
            $row[] = $reviewer;
            $rows[] = array(
                'data' => $row,
                'no_striping' => true,
            );
        }

        // Assemble the table.
        $id = 'report-title';
        $title = 'Citations by Status';
        $table = array(
            'title' => array(
                '#type' => 'markup',
                '#markup' => "<h2 id='$id'>$title ($counter)</h2>",
            ),
            'table' => array(
                '#attributes' => array('id' => 'cites-by-status-report'),
                '#theme' => 'table',
                '#rows' => $rows,
                '#header' => $headers,
            ),
        );
        return $table;
    }

    /**
     * Create the Drupal render array for a report on articles whose
     * current status is approval or rejection (or in some cases,
     * marked as an "FYI" with no further action required) based on
     * review of the full text of the articles by NCI.  The label
     * used for this status is the somewhat anachronistic "Literature
     * Surveillance Committee Decision"; even though the use of
     * committees for this review step has long been abandoned, the
     * users insisted on retaining this language.
     *
     * This report is one of eight different reports on articles by
     * status.  Called by the cites_by_status() method below.
     *
     *  @param  array  $params   Values specified by the user
     *                           requesting the report, to be
     *                           used for selecting the articles
     *                           to appear on that report.
     *
     *  @return array            Drupal render array for the report
     *                           table and its title.
     */
    static function cites_by_status_cd($params) {

        // Create the table column headers, specifying the column which
        // can be used for sorting.
        $headers = array(
            '',
            array(
                'data' => 'Citation',
                'field' => 'a.last_name',
                'class' => array('col-1'),
            ),
        );
        if (!$params['disposition'])
            $headers[] = 'Disposition';
        $headers[] = 'Comments';
        $headers[] = 'Review Cycle';
        $headers[] = 'Date Reviewed';

        // Construct the query for the report.
        $passed = Ebms\Util::state_id_from_text_id('PassedFullReview');
        $rejected = Ebms\Util::state_id_from_text_id('RejectFullReview');
        $fyi = Ebms\Util::state_id_from_text_id('FYI');
        $query = db_select('ebms_article_state', 's');
        $state_fields = array('article_id', 'article_state_id', 'status_dt');
        $query = $query->extend('TableSort');
        $query->join('ebms_article_topic', 't',
                't.article_id = s.article_id AND t.topic_id = s.topic_id');
        $query->leftJoin('ebms_article_author_cite', 'c',
            'c.article_id = s.article_id AND c.cite_order = 1');
        $query->leftJoin('ebms_article_author', 'a',
            'a.author_id = c.author_id');
        $query->condition('s.board_id', $params['board']);
        if ($params['topic'])
            $query->condition('s.topic_id', $params['topic']);
        $query->condition('s.current', 'Y');
        if (count($params['cycles']))
            $query->condition('t.cycle_id', $params['cycles'], 'IN');
        $query->join('ebms_cycle', 'c2', 'c2.cycle_id = t.cycle_id');
        $query->fields('c2', array('cycle_name'));
        switch ($params['disposition']) {
        case 'yes':
            $query->condition('s.state_id', $passed);
            break;
        case 'no':
            $query->condition('s.state_id', $rejected);
            break;
        case 'fyi':
            $query->condition('s.state_id', $fyi);
            break;
        default:
            $query->condition('s.state_id',
                array($passed, $rejected, $fyi), 'IN');
            $state_fields[] = 'state_id';
            break;
        }
        $query->fields('s', $state_fields);
        $query->orderByHeader($headers);
        $query->distinct();

        // Execute the query and assemble the rows for the report table.
        $results = $query->execute();
        $counter = 0;
        $rows = array();
        foreach ($results as $result) {
            $article_id = $result->article_id;
            $article_state_id = $result->article_state_id;
            $status_dt = strtotime($result->status_dt);
            $citation = EbmsReports::format_citation($article_id);
            $comment_rows = db_select('ebms_article_state_comment', 'c')
                ->fields('c', array('comment'))
                ->condition('c.article_state_id', $article_state_id)
                ->orderBy('c.comment_dt')
                ->execute();
            $comments = array();
            foreach ($comment_rows as $comment_row)
                $comments[] = htmlspecialchars($comment_row->comment);
            $comments = implode('<br />', $comments);
            $row = array(
                array(
                    'data' => ++$counter,
                    'class' => array('col-0'),
                ),
                array(
                    'data' => $citation,
                    'class' => array('col-1'),
                ),
            );
            if (!$params['disposition']) {
                switch ($result->state_id) {
                    case $passed:
                        $disposition = 'Yes';
                        break;
                    case $rejected:
                        $disposition = 'No';
                        break;
                    default:
                        $disposition = 'FYI';
                        break;
                }
                $row[] = $disposition;
            }
            $row[] = $comments;
            $row[] = $result->cycle_name;
            $row[] = date('m/d/Y', $status_dt);
            $rows[] = array(
                'data' => $row,
                'no_striping' => true,
            );
        }

        // Assemble the table.
        $id = 'report-title';
        $title = 'Citations by Status';
        $table = array(
            'title' => array(
                '#type' => 'markup',
                '#markup' => "<h2 id='$id'>$title ($counter)</h2>",
            ),
            'table' => array(
                '#attributes' => array('id' => 'cites-by-status-report'),
                '#theme' => 'table',
                '#rows' => $rows,
                '#header' => $headers,
            ),
        );
        return $table;
    }

    /**
     * Create the Drupal render array for a report on articles whose
     * current status is that the full text for the articles has been
     * retrieved, or that the full text is needed but unobtainable.
     * In a meeting with Robin, she indicated that for this report,
     * if the user chooses "No" for the disposition, the report should
     * show the articles for which the full text is not available at
     * all; otherwise, the report should show articles for which
     * full text has been retrieved and stored in the EBMS (even if
     * the user did not make a selection from the Disposition drop-
     * down box).
     *
     * This report is one of eight different reports on articles by
     * status.  Called by the cites_by_status() method below.
     *
     *  @param  array  $params   Values specified by the user
     *                           requesting the report, to be
     *                           used for selecting the articles
     *                           to appear on that report.
     *
     *  @return array            Drupal render array for the report
     *                           table and its title.
     */
    static function cites_by_status_ftr($params) {

        // Create the table column headers, specifying the column which
        // can be used for sorting.
        $headers = array(
            '',
            array(
                'data' => 'Citation',
                'field' => 'aa.last_name',
                'class' => array('col-1'),
            ),
        );

        // Construct the query for the report.
        $passed_bm_review = Ebms\Util::state_id_from_text_id('PassedBMReview');
        $query = db_select('ebms_article_state', 's');
        $query = $query->extend('TableSort');
        $query->condition('s.current', 'Y');
        $query->condition('s.state_id', $passed_bm_review);
        $query->condition('s.board_id', $params['board']);
        $query->fields('s', array('article_id', 'status_dt'));
        $query->join('ebms_article_topic', 't',
            't.article_id = s.article_id AND t.topic_id = s.topic_id');
        if ($params['topic'])
            $query->condition('s.topic_id', $params['topic']);
        if ($params['disposition'] == 'no') {
            $query->join('ebms_ft_unavailable', 'u',
                'u.article_id = s.article_id');
            $query->fields('u', array('comment'));
            $headers[] = 'Comments';
            $headers[] = 'Review Cycle';
        }
        else {
            $query->join('ebms_article', 'a', 'a.article_id = s.article_id');
            $query->join('file_managed', 'f', 'f.fid = a.full_text_id');
            $query->fields('f', array('filename', 'timestamp'));
            $headers[] = 'Filename';
            $headers[] = 'Review Cycle';
            $headers[] = 'Date Uploaded';
        }
        if (count($params['cycles']))
            $query->condition('t.cycle_id', $params['cycles'], 'IN');
        $query->join('ebms_cycle', 'c2', 'c2.cycle_id = t.cycle_id');
        $query->fields('c2', array('cycle_name'));
        $query->leftJoin('ebms_article_author_cite', 'c',
            'c.article_id = s.article_id AND c.cite_order = 1');
        $query->leftJoin('ebms_article_author', 'aa',
            'aa.author_id = c.author_id');

        $query->orderByHeader($headers);
        $query->distinct();

        // Execute the query and assemble the rows for the report table.
        $results = $query->execute();
        $counter = 0;
        $rows = array();
        foreach ($results as $result) {
            $article_id = $result->article_id;
            $status_dt = strtotime($result->status_dt);
            $citation = EbmsReports::format_citation($article_id);
            $row = array(
                array(
                    'data' => ++$counter,
                    'class' => array('col-0'),
                ),
                array(
                    'data' => $citation,
                    'class' => array('col-1'),
                ),
            );
            if ($params['disposition'] == 'no')
                $row[] = htmlspecialchars($result->comment);
            else
                $row[] = htmlspecialchars($result->filename);
            $row[] = $result->cycle_name;
            if ($params['disposition'] != 'no')
                $row[] = date('m/d/Y', $result->timestamp);
            $rows[] = array(
                'data' => $row,
                'no_striping' => true,
            );
        }

        // Assemble the table.
        $id = 'report-title';
        $title = 'Citations by Status';
        $table = array(
            'title' => array(
                '#type' => 'markup',
                '#markup' => "<h2 id='$id'>$title ($counter)</h2>",
            ),
            'table' => array(
                '#attributes' => array('id' => 'cites-by-status-report'),
                '#theme' => 'table',
                '#rows' => $rows,
                '#header' => $headers,
            ),
        );
        return $table;
    }

    /**
     * Create the Drupal render array for a report on articles which
     * have been assigned to packets for review for specific topics,
     * but for which no board member has returned a response for that
     * topic.
     *
     * This report is one of eight different reports on articles by
     * status.  Called by the cites_by_status() method below.
     *
     *  @param  array  $params   Values specified by the user
     *                           requesting the report, to be
     *                           used for selecting the articles
     *                           to appear on that report.
     *
     *  @return array            Drupal render array for the report
     *                           table and its title.
     */
    static function cites_by_status_afr($params) {

        // Create the table column headers, specifying the column which
        // can be used for sorting.
        $headers = array(
            '',
            array(
                'data' => 'Citation',
                'field' => 'aa.last_name',
                'class' => array('col-1'),
            ),
            'Reviewers',
            'Date Assigned',
            'Review Cycle',
        );

        // Construct the query for the report.
        $passed_full_review =
            Ebms\Util::state_id_from_text_id('PassedFullReview');
        $query = db_select('ebms_article_state', 's');
        $query = $query->extend('TableSort');
        $query->join('ebms_article_topic', 't',
            't.article_id = s.article_id AND t.topic_id = s.topic_id');
        $query->condition('s.current', 'Y');
        $query->condition('s.state_id', $passed_full_review);
        $query->condition('s.board_id', $params['board']);
        $query->fields('s', array('article_id', 'status_dt'));
        $query->join('ebms_cycle', 'c2', 'c2.cycle_id = t.cycle_id');
        $query->fields('c2', array('cycle_name'));
        if ($params['topic'])
            $query->condition('s.topic_id', $params['topic']);
        if (count($params['cycles']))
            $query->condition('t.cycle_id', $params['cycles'], 'IN');
        $query->leftJoin('ebms_article_author_cite', 'c',
            'c.article_id = s.article_id AND c.cite_order = 1');
        $query->leftJoin('ebms_article_author', 'aa',
            'aa.author_id = c.author_id');
        $query->join('ebms_packet_article', 'a',
            'a.article_id = s.article_id');
        $query->condition('a.drop_flag', 0);
        $query->join('ebms_packet', 'p',
            'p.packet_id = a.packet_id AND p.topic_id = s.topic_id');
        $query->condition('p.active_status', 'A');
        $query->leftJoin('ebms_article_review', 'r',
            'r.packet_id = p.packet_id AND r.article_id = a.article_id');
        $query->isNull('r.article_id');
        $query->fields('p', array('packet_id', 'created_at'));
        $query->orderByHeader($headers);
        $query->distinct();

        // Execute the query and collect the results in an array
        // indexed by article, each element of the array containing
        // the review cycle for the article and an array of packet
        // information.
        $results = $query->execute();
        $articles = array();
        foreach ($results as $result) {
            $article_id = $result->article_id;
            if (!array_key_exists($article_id, $articles))
                $articles[$article_id] = array(
                    'cycle' => $result->cycle_name,
                    'packets' => array(),
                );
            $articles[$article_id]['packets'][] = array(
                $result->packet_id,
                date('m/d/y', strtotime($result->created_at)),
            );
        }

        // Assemble the rows for the report table.
        $counter = 0;
        $rows = array();
        foreach ($articles as $article_id => $article_info) {
            $have_citation = false;
            $cycle = $article_info['cycle'];
            $packets = $article_info['packets'];
            $rowspan = count($packets);
            foreach ($packets as $packet) {
                list($packet_id, $date_assigned) = $packet;
                $query = db_select('ebms_packet_reviewer', 'r');
                $query->join('users', 'u', 'u.uid = r.reviewer_id');
                $query->condition('r.packet_id', $packet_id);
                $query->fields('u', array('name'));
                $query->orderBy('u.name');
                $reviewer_rows = $query->execute();
                $reviewers = array();
                foreach ($reviewer_rows as $reviewer_row)
                    $reviewers[] = htmlspecialchars($reviewer_row->name);
                $col2 = array(
                    'data' => implode('<br />', $reviewers),
                    'class' => array('need-left-border'),
                );
                $col3 = $date_assigned;
                if (!$have_citation) {
                    $have_citation = true;
                    $citation = EbmsReports::format_citation($article_id);
                    $col0 = array(
                        'data' => ++$counter,
                        'class' => array('col-0'),
                    );
                    $col1 = array(
                        'data' => $citation,
                        'class' => array('col-1'),
                    );
                    $col4 = array(
                        'data' => $cycle,
                    );
                    if ($rowspan > 1)
                        $col0['rowspan'] = $col1['rowspan'] =
                            $col4['rowspan'] = $rowspan;
                    $row = array($col0, $col1, $col2, $col3, $col4);
                }
                else
                    $row = array($col2, $col3);
                $rows[] = array(
                    'data' => $row,
                    'no_striping' => true,
                );
            }
        }

        // Assemble the table.
        $id = 'report-title';
        $title = 'Citations by Status';
        $table = array(
            'title' => array(
                '#type' => 'markup',
                '#markup' => "<h2 id='$id'>$title ($counter)</h2>",
            ),
            'table' => array(
                '#attributes' => array('id' => 'cites-by-status-report'),
                '#theme' => 'table',
                '#rows' => $rows,
                '#header' => $headers,
            ),
        );
        return $table;
    }

    /**
     * Create the Drupal render array for a report on articles whose
     * current state is "passed full text review" and which have been
     * assigned to packets for review by board members, and for which
     * at least one board member has returned a response.  The report
     * can be narrowed by specifying which response disposition to
     * filter by.
     *
     * This report is one of eight different reports on articles by
     * status.  Called by the cites_by_status() method below.
     *
     *  @param  array  $params   Values specified by the user
     *                           requesting the report, to be
     *                           used for selecting the articles
     *                           to appear on that report.
     *
     *  @return array            Drupal render array for the report
     *                           table and its title.
     */
    static function cites_by_status_bmr($params) {

        // Create the table column headers, specifying the column which
        // can be used for sorting.
        $headers = array(
            '',
            array(
                'data' => 'Citation',
                'field' => 'aa.last_name',
                'class' => array('col-1'),
            ),
            'Reviewer',
            'Disposition',
            'Comments',
            'LOE',
            'Exclusion Reason',
            'Date Review Submitted',
            'Review Cycle',
        );


        // Construct the query for the report.
        $passed_full_review =
            Ebms\Util::state_id_from_text_id('PassedFullReview');
        $query = db_select('ebms_article_state', 's');
        $query = $query->extend('TableSort');
        $query->condition('s.current', 'Y');
        $query->condition('s.state_id', $passed_full_review);
        $query->fields('s', array('article_id', 'status_dt'));
        if ($params['topic'])
            $query->condition('s.topic_id', $params['topic']);
        $query->join('ebms_article_topic', 't',
            't.article_id = s.article_id AND t.topic_id = s.topic_id');
        $query->join('ebms_cycle', 'c2', 'c2.cycle_id = t.cycle_id');
        $query->fields('c2', array('cycle_name'));
        $query->condition('s.board_id', $params['board']);
        if (count($params['cycles']))
            $query->condition('t.cycle_id', $params['cycles'], 'IN');
        $query->leftJoin('ebms_article_author_cite', 'c',
            'c.article_id = s.article_id AND c.cite_order = 1');
        $query->leftJoin('ebms_article_author', 'aa',
            'aa.author_id = c.author_id');
        $query->join('ebms_packet_article', 'a',
            'a.article_id = s.article_id');
        $query->join('ebms_packet', 'p',
            'p.packet_id = a.packet_id AND p.topic_id = s.topic_id');
        $query->join('ebms_article_review', 'r',
            'r.packet_id = p.packet_id AND r.article_id = a.article_id');
        $query->fields('r', array('review_id'));
        if ($params['disposition']) {
            $query->join('ebms_review_disposition', 'd',
                'd.review_id = r.review_id');
            $query->condition('d.value_id', $params['disposition']);
        }
        $query->orderByHeader($headers);
        $query->distinct();

        // Execute the query and collect the results in an array
        // indexed by article, each element of the array containing
        // the review cycle for the article and an array of review
        // IDs.
        $results = $query->execute();
        $articles = array();
        foreach ($results as $result) {
            $article_id = $result->article_id;
            if (!array_key_exists($article_id, $articles))
                $articles[$article_id] = array(
                    'cycle' => $result->cycle_name,
                    'reviews' => array(),
                );
            $articles[$article_id]['reviews'][] = $result->review_id;
        }

        // Assemble the rows for the report table.
        $counter = 0;
        $rows = array();
        foreach ($articles as $article_id => $article_info) {
            $have_citation = false;
            $cycle = $article_info['cycle'];
            $reviews = $article_info['reviews'];
            $rowspan = count($reviews);
            foreach ($reviews as $review_id) {
                $query = db_select('ebms_article_review', 'r');
                $query->join('users', 'u', 'u.uid = r.reviewer_id');
                $query->condition('r.review_id', $review_id);
                $query->fields('r',
                    array('when_posted', 'comments', 'loe_info'));
                $query->fields('u', array('name'));
                $review_results = $query->execute();
                foreach ($review_results as $r) {
                    $col2 = array(
                        'data' => htmlspecialchars($r->name),
                        'class' => array('need-left-border'),
                    );
                    $col4 = htmlspecialchars($r->comments);
                    $col5 = htmlspecialchars($r->loe_info);
                    $col7 = date('m/d/Y', strtotime($r->when_posted));
                }
                $query = db_select('ebms_review_disposition', 'd');
                $query->join('ebms_review_disposition_value', 'v',
                    'v.value_id = d.value_id');
                $query->condition('d.review_id', $review_id);
                $query->fields('v', array('value_name'));
                $dispositions = array();
                $disp_results = $query->execute();
                foreach ($disp_results as $r)
                    $dispositions[] = htmlspecialchars($r->value_name);
                $col3 = implode('<br />', $dispositions);
                $query = db_select('ebms_review_rejection_reason', 'r');
                $query->join('ebms_review_rejection_value', 'v',
                    'v.value_id = r.value_id');
                $query->condition('r.review_id', $review_id);
                $query->fields('v', array('value_name'));
                $reasons = array();
                $reason_results = $query->execute();
                foreach ($reason_results as $r)
                    $reasons[] = htmlspecialchars($r->value_name);
                $col6 = implode('<br />', $reasons);
                if (!$have_citation) {
                    $have_citation = true;
                    $citation = EbmsReports::format_citation($article_id);
                    $col0 = array(
                        'data' => ++$counter,
                        'class' => array('col-0'),
                    );
                    $col1 = array(
                        'data' => $citation,
                        'class' => array('col-1'),
                    );
                    $col8 = array(
                        'data' => $cycle,
                    );
                    if ($rowspan > 1)
                        $col0['rowspan'] = $col1['rowspan'] =
                            $col8['rowspan'] = $rowspan;
                    $row = array($col0, $col1, $col2, $col3, $col4,
                           $col5, $col6, $col7, $col8);
                }
                else
                    $row = array($col2, $col3, $col4, $col5, $col6, $col7);
                $rows[] = array(
                    'data' => $row,
                    'no_striping' => true,
                );
            }
        }

        // Assemble the table.
        $id = 'report-title';
        $title = 'Citations by Status';
        $table = array(
            'title' => array(
                '#type' => 'markup',
                '#markup' => "<h2 id='$id'>$title ($counter)</h2>",
            ),
            'table' => array(
                '#attributes' => array('id' => 'cites-by-status-report'),
                '#theme' => 'table',
                '#rows' => $rows,
                '#header' => $headers,
            ),
        );
        return $table;
    }

    /**
     * Create the Drupal render array for a report on articles for
     * which a board member has taken an action after having looked
     * at the responses provided by her board members in their review
     * of the article assigned to their packets.
     *
     * This report is one of eight different reports on articles by
     * status.  Called by the cites_by_status() method below.
     *
     *  @param  array  $params   Values specified by the user
     *                           requesting the report, to be
     *                           used for selecting the articles
     *                           to appear on that report.
     *
     *  @return array            Drupal render array for the report
     *                           table and its title.
     */
    static function cites_by_status_bma($params) {
        $manager_actions = EbmsReports::manager_actions();

        // Create the table column headers, specifying the column which
        // can be used for sorting.
        $headers = array(
            '',
            array(
                'data' => 'Citation',
                'field' => 'a.last_name',
                'class' => array('col-1'),
            ),
        );
        if (!$params['disposition'])
            $headers[] = 'Disposition';
        if (!$params['topic'])
            $headers[] = array(
                'data' => 'Summary Topic',
                'field' => 'tn.topic_name',
            );
        $headers[] = 'Review Cycle';
        $headers[] = 'Date Assigned';
        $headers[] = 'Comments';

        // Construct the query for the report.
        $query = db_select('ebms_article_state', 's');
        $query = $query->extend('TableSort');
        $query->join('ebms_article_topic', 't',
            't.article_id = s.article_id AND t.topic_id = s.topic_id');
        $query->join('ebms_cycle', 'c2', 'c2.cycle_id = t.cycle_id');
        $query->fields('c2', array('cycle_name'));
        $query->leftJoin('ebms_article_author_cite', 'c',
            'c.article_id = s.article_id AND c.cite_order = 1');
        $query->leftJoin('ebms_article_author', 'a',
            'a.author_id = c.author_id');
        $query->condition('s.board_id', $params['board']);
        if ($params['topic'])
            $query->condition('s.topic_id', $params['topic']);
        $query->condition('s.current', 'Y');
        if ($params['disposition'])
            $query->condition('s.state_id', $params['disposition']);
        else
            $query->condition('s.state_id',
                array_keys($manager_actions), 'IN');
        if (count($params['cycles']))
            $query->condition('t.cycle_id', $params['cycles'], 'IN');
        $query->fields('s', array('article_id', 'article_state_id',
                                  'state_id', 'status_dt'));
        $query->join('ebms_topic', 'tn', 'tn.topic_id = t.topic_id');
        $query->fields('tn', array('topic_name'));
        $query->orderByHeader($headers);
        $query->distinct();

        // Execute the query and assemble the rows for the report table.
        $results = $query->execute();
        $counter = 0;
        $rows = array();
        foreach ($results as $result) {
            $article_id = $result->article_id;
            $citation = EbmsReports::format_citation($article_id);
            $row = array(
                array(
                    'data' => ++$counter,
                    'class' => array('col-0'),
                ),
                array(
                    'data' => $citation,
                    'class' => array('col-1'),
                ),
            );
            if (!$params['disposition']) {
                $disp = $manager_actions[$result->state_id];
                $row[] = htmlspecialchars($disp);
            }
            $article_state_id = $result->article_state_id;
            $comment_rows = db_select('ebms_article_state_comment', 'c')
                ->fields('c', array('comment'))
                ->condition('c.article_state_id', $article_state_id)
                ->orderBy('c.comment_dt')
                ->execute();
            $comments = array();
            foreach ($comment_rows as $comment_row)
                $comments[] = htmlspecialchars($comment_row->comment);
            if (!$params['topic'])
                $row[] = htmlspecialchars($result->topic_name);
            $row[] = $result->cycle_name;
            $status_dt = strtotime($result->status_dt);
            $row[] = date('m/d/Y', $status_dt);
            $row[] = implode('<br />', $comments);
            $rows[] = array(
                'data' => $row,
                'no_striping' => true,
            );
        }

        // Assemble the table.
        $id = 'report-title';
        $title = 'Citations by Status';
        $table = array(
            'title' => array(
                '#type' => 'markup',
                '#markup' => "<h2 id='$id'>$title ($counter)</h2>",
            ),
            'table' => array(
                '#attributes' => array('id' => 'cites-by-status-report'),
                '#theme' => 'table',
                '#rows' => $rows,
                '#header' => $headers,
            ),
        );
        return $table;
    }


    /**
     * Create the Drupal render array for a report on articles which
     * have been put on a meeting agenda for discussion by the PDQ
     * board members.  Robin said we should leave off state rows
     * from the legacy system for this report, since we don't have
     * any information about the meetings on whose agendas the
     * articles were to be discussed.
     *
     * This report is one of eight different reports on articles by
     * status.  Called by the cites_by_status() method below.
     *
     *  @param  array  $params   Values specified by the user
     *                           requesting the report, to be
     *                           used for selecting the articles
     *                           to appear on that report.
     *
     *  @return array            Drupal render array for the report
     *                           table and its title.
     */
    static function cites_by_status_oa($params) {

        // Create the table column headers, specifying the column which
        // can be used for sorting.
        $headers = array(
            '',
            array(
                'data' => 'Citation',
                'field' => 'a.last_name',
                'class' => array('col-1'),
            ),
            'Meeting Date (Category)',
        );
        if (!$params['topic'])
            $headers[] = array(
                'data' => 'Summary Topic',
                'field' => 'tn.topic_name',
            );
        $headers[] = 'Review Cycle';
        $headers[] = 'Comments';

        // Start off with an empty report table.
        $counter = 0;
        $rows = array();

        // We construct the list of eligible meetings first.
        // pdq_ebms_debug('ON AGENDA PARAMS', $params);
        $query = new EbmsEventQuery();
        $query->restrict_by_date($params['meeting-date']);
        $query->restrict_by_type($params['meeting-type']);
        $results = $query->execute();

        // If there are no eligible meetings, the report will be empty.
        if (isset($results['node'])) {

            // Construct the query for the report.
            $meeting_ids = array_keys($results['node']);
            //pdq_ebms_debug('MEETING IDS', $meeting_ids);
            $state_id = Ebms\Util::state_id_from_text_id('OnAgenda');
            $query = db_select('ebms_article_state', 's');
            $query = $query->extend('TableSort');
            $query->join('ebms_article_topic', 't',
                't.article_id = s.article_id AND t.topic_id = s.topic_id');
            $query->join('ebms_cycle', 'c2', 'c2.cycle_id = t.cycle_id');
            $query->fields('c2', array('cycle_name'));
            $query->join('ebms_agenda_meeting', 'm',
                'm.article_state_id = s.article_state_id');
            $query->condition('m.nid', $meeting_ids, 'IN');
            $query->leftJoin('ebms_article_author_cite', 'c',
                'c.article_id = s.article_id AND c.cite_order = 1');
            $query->leftJoin('ebms_article_author', 'a',
                'a.author_id = c.author_id');
            $query->condition('s.board_id', $params['board']);
            if ($params['topic'])
                $query->condition('s.topic_id', $params['topic']);
            $query->condition('s.current', 'Y');
            $query->condition('s.state_id', $state_id);
            if (count($params['cycles']))
                $query->condition('t.cycle_id', $params['cycles'], 'IN');
            $query->fields('s',
                array('article_id', 'article_state_id', 'status_dt'));
            $query->fields('m', array('nid'));
            $query->join('ebms_topic', 'tn', 'tn.topic_id = t.topic_id');
            $query->fields('tn', array('topic_name'));
            $query->orderByHeader($headers);
            $query->distinct();

            // Execute the query and assemble the rows for the report table.
            $results = $query->execute();
            foreach ($results as $result) {
                $article_id = $result->article_id;
                $citation = EbmsReports::format_citation($article_id);
                $article_state_id = $result->article_state_id;
                $comment_rows = db_select('ebms_article_state_comment', 'c')
                    ->fields('c', array('comment'))
                    ->condition('c.article_state_id', $article_state_id)
                    ->orderBy('c.comment_dt')
                    ->execute();
                $comments = array();
                foreach ($comment_rows as $comment_row)
                    $comments[] = htmlspecialchars($comment_row->comment);
                $col2 = 'N/A';
                $meetings = entity_load('node', array($result->nid));
                foreach ($meetings as $meeting) {
                    $event_date = field_get_items('node', $meeting,
                                  'field_datespan');
                    $start_date = date('m/d/Y', $event_date[0]['value']);

                    // Change request (TIR 2446): pick up the meeting
                    // type from the new event category field, not from
                    // whether boards or subgroups have been linked to
                    // the event.
                    $meeting_type = 'no category';
                    $event_types = field_get_items('node', $meeting,
                              'field_event_category');
                    if (is_array($event_types)) {
                        $event_type = $event_types[0]['value'];
                        $info = field_info_field('field_event_category');
                        $allowed = $info['settings']['allowed_values'];
                        if (array_key_exists($event_type, $allowed))
                            $meeting_type = $allowed[$event_type];
                    }
                    $col2 = "$start_date ($meeting_type)";
                }
                if (!$params['topic'])
                    $col3 = htmlspecialchars($result->topic_name);
                $col4 = $result->cycle_name;
                $col5 = implode('<br />', $comments);
                $row = array(
                    array(
                        'data' => ++$counter,
                        'class' => array('col-0'),
                    ),
                    array(
                        'data' => $citation,
                        'class' => array('col-1'),
                    ),
                    $col2,
                );
                if (!$params['topic'])
                    $row[] = $col3;
                $row[] = $col4;
                $row[] = $col5;
                $rows[] = array(
                    'data' => $row,
                    'no_striping' => true,
                );
            }
        }

        // Assemble and return the table.
        $id = 'report-title';
        $title = 'Citations by Status';
        $table = array(
            'title' => array(
                '#type' => 'markup',
                '#markup' => "<h2 id='$id'>$title ($counter)</h2>",
            ),
            'table' => array(
                '#attributes' => array('id' => 'cites-by-status-report'),
                '#theme' => 'table',
                '#rows' => $rows,
                '#header' => $headers,
            ),
        );
        return $table;
    }

    /**
     * Create the Drupal render array for a report on articles for
     * which a final editorial board decision has been made.  This
     * status is the end of the line for EBMS processing.  One of
     * eight different reports on articles by status.  Called by
     * the cites_by_status() method below.
     *
     *  @param  array  $params   Values specified by the user
     *                           requesting the report, to be
     *                           used for selecting the articles
     *                           to appear on that report.
     *
     *  @return array            Drupal render array for the report
     *                           table and its title.
     */
    static function cites_by_status_ebd($params) {

        // Create the table column headers, specifying the column which
        // can be used for sorting.
        $headers = array(
            '',
            array(
                'data' => 'Citation',
                'field' => 'a.last_name',
                'class' => array('col-1'),
            ),
            'Disposition',
            'Comments',
            'Review Cycle',
            'Date Assigned',
            'Discussed By',
        );

        // Tricky code needed to narrow by meeting date or type.  This is
        // because there are no meetings directly connected to the "editorial
        // board decision" state rows.  Instead, we have to find the latest
        // "on agenda" row for the same article/topic and check the meeting
        // linked to *that* row!  TIR 2446.
        $restrict_by_meeting = false;
        if ($params['meeting-type'] || $params['meeting-date']['year']) {
            $on_agenda_decision = Ebms\Util::state_id_from_text_id('OnAgenda');
            $restrict_by_meeting = true;
            $query = new EbmsEventQuery();
            $query->restrict_by_date($params['meeting-date']);
            $query->restrict_by_type($params['meeting-type']);
            $meeting_results = $query->execute();
            $meeting_ids = array();
            if (isset($meeting_results['node']))
                $meeting_ids = array_keys($meeting_results['node']);
            $subquery = db_select('ebms_article_state', 's');
            $subquery->addExpression('MAX(article_state_id)', 'art_state_id');
            $subquery->fields('s', array('article_id', 'topic_id'));
            $subquery->condition('active_status', 'A');
            $subquery->condition('state_id', $on_agenda_decision);
            $subquery->groupBy('s.article_id');
            $subquery->groupBy('s.topic_id');
        }

        // Construct the query for the report.
        $final_board_decision =
            Ebms\Util::state_id_from_text_id('FinalBoardDecision');
        $query = db_select('ebms_article_state', 's');
        $query = $query->extend('TableSort');
        $query->join('ebms_article_topic', 't',
            't.article_id = s.article_id AND t.topic_id = s.topic_id');
        $query->leftJoin('ebms_article_author_cite', 'c',
            'c.article_id = s.article_id AND c.cite_order = 1');
        $query->leftJoin('ebms_article_author', 'a',
            'a.author_id = c.author_id');
        $query->condition('s.board_id', $params['board']);
        if ($params['topic'])
            $query->condition('s.topic_id', $params['topic']);
        $query->condition('s.current', 'Y');
        $query->condition('s.state_id', $final_board_decision);
        if ($params['disposition']) {
            $query->join('ebms_article_board_decision', 'd',
                'd.article_state_id = s.article_state_id');
            $query->condition('d.decision_value_id', $params['disposition']);
        }
        if (count($params['cycles']))
            $query->condition('t.cycle_id', $params['cycles'], 'IN');
        $query->fields('s',
            array('article_id', 'article_state_id', 'status_dt'));
        $query->join('ebms_cycle', 'c2', 'c2.cycle_id = t.cycle_id');
        $query->fields('c2', array('cycle_name'));
        if ($restrict_by_meeting) {
            if (empty($meeting_ids))
                $meeting_ids[] = -1; // WHERE ... IN needs at least 1 value.
            $query->join($subquery, 'oa',
                'oa.article_id = s.article_id AND oa.topic_id = s.topic_id');
            $query->join('ebms_agenda_meeting', 'm',
                'm.article_state_id = oa.art_state_id');
            $query->condition('m.nid', $meeting_ids, 'IN');
        }
        $query->orderByHeader($headers);
        $query->distinct();

        // Execute the query and assemble the rows for the report table.
        $results = $query->execute();
        $counter = 0;
        $rows = array();
        foreach ($results as $result) {
            $article_id = $result->article_id;
            $citation = EbmsReports::format_citation($article_id);
            $article_state_id = $result->article_state_id;
            $query = db_select('ebms_article_board_decision_value', 'v');
            $query->fields('v', array('value_name'));
            $query->join('ebms_article_board_decision', 'd',
                'd.decision_value_id = v.value_id');
            $query->condition('d.article_state_id', $article_state_id);
            $decision_rows = $query->execute();
            $decisions = array();
            foreach ($decision_rows as $decision_row)
                $decisions[] = htmlspecialchars($decision_row->value_name);
            $col2 = implode('<br />', $decisions);
            $comment_rows = db_select('ebms_article_state_comment', 'c')
                ->fields('c', array('comment'))
                ->condition('c.article_state_id', $article_state_id)
                ->orderBy('c.comment_dt')
                ->execute();
            $comments = array();
            foreach ($comment_rows as $comment_row)
                $comments[] = htmlspecialchars($comment_row->comment);
            $col3 = implode('<br />', $comments);
            $col4 = $result->cycle_name;
            $col5 = date('m/d/Y', strtotime($result->status_dt));
            $query = db_select('users', 'u');
            $query->fields('u', array('name'));
            $query->join('ebms_article_board_decision_member', 'm',
                'm.uid = u.uid');
            $query->condition('m.article_state_id', $article_state_id);
            $query->orderBy('u.name');
            $member_rows = $query->execute();
            $members = array();
            foreach ($member_rows as $member_row)
                $members[] = htmlspecialchars($member_row->name);
            $col6 = implode('<br />', $members);
            $row = array(
                array(
                    'data' => ++$counter,
                    'class' => array('col-0'),
                ),
                array(
                    'data' => $citation,
                    'class' => array('col-1'),
                ),
                $col2,
                $col3,
                $col4,
                $col5,
                $col6,
            );
            $rows[] = array(
                'data' => $row,
                'no_striping' => true,
            );
        }

        // Assemble and return the table.
        $id = 'report-title';
        $title = 'Citations by Status';
        $table = array(
            'title' => array(
                '#type' => 'markup',
                '#markup' => "<h2 id='$id'>$title ($counter)</h2>",
            ),
            'table' => array(
                '#attributes' => array('id' => 'cites-by-status-report'),
                '#theme' => 'table',
                '#rows' => $rows,
                '#header' => $headers,
            ),
        );
        return $table;
    }

    /**
     * Retrieves the request values for one of the eight "Citations
     * by Status" reports and routes the request to the appropriate
     * method for generating the report.
     *
     *  @param  int  $request_id    Primary key into the ebms_report_request
     *                              table.
     *
     *  @return array               Drupal render array for the report
     *                              portion of the page.
     */
    public static function cites_by_status($request_id) {
        $params = EbmsReports::fetch_report_request($request_id);
        switch ($params['status']) {
            case 'reviewer decision':
                return EbmsReports::cites_by_status_rd($params);
            case 'full text retrieved':
                return EbmsReports::cites_by_status_ftr($params);
            case 'committee decision':
                return EbmsReports::cites_by_status_cd($params);
            case 'assigned for review':
                return EbmsReports::cites_by_status_afr($params);
            case 'board member responses':
                return EbmsReports::cites_by_status_bmr($params);
            case 'board manager action':
                return EbmsReports::cites_by_status_bma($params);
            case 'on agenda':
                return EbmsReports::cites_by_status_oa($params);
            case 'editorial board decision':
                return EbmsReports::cites_by_status_ebd($params);
            default:
                $name = htmlspecialchars($params['status']);
                drupal_set_message("Unsupported report request \"$name.\"",
                    'error');
                return array();
        }
    }

    /**
     * Build the Drupal render array for the "Literature Reviews" report.
     *
     *  @param  int  $request_id   Primary key for the EBMS table used
     *                             to store the paramaters for report
     *                             requests
     *
     *  @return array              Drupal render array for the report
     */
    public static function reviews($request_id) {

        // Create the table column headers, specifying the column which
        // can be used for sorting.
        $headers = array(
            array(
                'data' => '',
                'class' => array('col-0'),
            ),
            array(
                'data' => 'CITATION',
                'field' => 'a.last_name',
                'class' => array('col-1'),
            ),
            array(
                'data' => 'CYCLE',
                'class' => array('col-2'),
            ),
            array(
                'data' => 'REVIEWER&nbsp;|&nbsp;DISPOSITION',
                'class' => array('col-3'),
            ),
            array(
                'data' => 'COMMENTS',
                'class' => array('col-4'),
            ),
        );

        // Retrieve the filtering values from the database.
        $params = EbmsReports::fetch_report_request($request_id);
        //pdq_ebms_debug('REVIEWS TABLE PARAMS', $params);
        $board = $params['board'];
        $topic = $params['topic'];
        $cycle = $params['cycle'];
        $packet = $params['packet'];
        $reviewer = $params['reviewer'];
        $rrange = $params['rrange'];
        $rstart = $params['rstart'];
        $rend = $params['rend'];
        $prange = $params['prange'];
        $pstart = $params['pstart'];
        $pend = $params['pend'];

        // Build the database query for the report.
        $query = db_select('ebms_article_review', 'r');
        $query = $query->extend('TableSort');
        $query->join('users', 'u', 'u.uid = r.reviewer_id');
        $query->join('ebms_packet', 'p', 'p.packet_id = r.packet_id');
        $query->join('ebms_topic', 't', 't.topic_id = p.topic_id');
        $query->join('ebms_article_topic', 't2',
            't2.article_id = r.article_id AND t2.topic_id = p.topic_id');
        $query->join('ebms_cycle', 'c2', 'c2.cycle_id = t2.cycle_id');
        //$query->fields('c2', array('cycle_name'));
        $query->fields('c2', array('start_date'));
        $query->leftJoin('ebms_article_author_cite', 'c',
            'c.article_id = r.article_id AND c.cite_order = 1');
        $query->leftJoin('ebms_article_author', 'a',
            'a.author_id = c.author_id');
        $query->fields('r', array('article_id', 'review_id'));
        $query->fields('p', array('created_at'));
        $query->fields('u', array('name'));
        $query->condition('t.board_id', $board);
        if ($topic)
            $query->condition('p.topic_id', $topic);
        if ($cycle)
            $query->condition('t2.cycle_id', $cycle);
        if ($packet)
            $query->condition('p.packet_id', $packet);
        if ($reviewer)
            $query->condition('r.reviewer_id', $reviewer);
        if ($rrange)
            Ebms\Util::add_date_range_to_query($query, $rstart, $rend,
                'r.when_posted');
        else
            Ebms\Util::add_date_to_query($query, $rstart, 'r.when_posted');
        if ($prange)
            Ebms\Util::add_date_range_to_query($query, $pstart, $pend,
                'p.created_at');
        else
            Ebms\Util::add_date_to_query($query, $pstart, 'p.created_at');
        $query->distinct();
        $query->orderByHeader($headers);

        // Execute the database query and assemble an array indexed by
        // article ID, each element of the array containing a list of
        // packets for which the article was reviewed, with enough
        // information about those reviews that we can build the report
        // table.
        $results = $query->execute();
        $articles = array();
        foreach ($results as $result) {
            $article_id = $result->article_id;
            $review_id = $result->review_id;
            //$packet = date('Y/m', strtotime($result->created_at));
            //$packet = $result->cycle_name;
            $packet = date('Y/m', strtotime($result->start_date));
            $reviewer = $result->name;
            if (!array_key_exists($article_id, $articles))
                $articles[$article_id] = array();
            if (!array_key_exists($packet, $articles[$article_id]))
                $articles[$article_id][$packet] = array();
            if (!array_key_exists($reviewer, $articles[$article_id][$packet]))
                $articles[$article_id][$packet][$reviewer] = array();
            $articles[$article_id][$packet][$reviewer][] = $review_id;
        }

        // Look up the ID of the disposition for rejecting the article
        // (always in the first position).
        $no_change_option = db_select('ebms_review_disposition_value', 'v')
            ->fields('v', array('value_id'))
            ->condition('v.value_pos', 1)
            ->execute()
            ->fetchField();

        // Assemble the table rows for the report.  Moderately complicated
        // by the need to use HTML rowspan markup to keep information about
        // the same article aligned properly, and information about the same
        // packet aligned properly as well.  Each article can have multiple
        // packet cycle values, and each packet cycle value can have multiple
        // reviews.  Everything has to match visually.
        $counter = 0;
        $rows = array();
        foreach ($articles as $article_id => $packets) {
            $citation = EbmsReports::format_citation($article_id);
            $query = db_select('ebms_article_review', 'r');
            $article_rowspan = 0;
            ksort($packets);
            $packet_rows = array();

            // Construct an array indexed by the cycle string for each
            // packet for which we have reviews for this article.
            // Each element of the array contains a nested array of
            // information about each review returned for the article
            // and packet combination.
            foreach ($packets as $packet => $reviews) {
                list($year, $month) = explode('/', $packet);
                $p = "$month/$year";
                $packet_rows[$p] = array();
                $packet_rowspan = 0;
                ksort($reviews);
                foreach ($reviews as $reviewer => $review_id) {
                    $comment_rows = db_select('ebms_article_review', 'r')
                        ->fields('r', array('comments'))
                        ->condition('r.review_id', $review_id)
                        ->execute();
                    $comments = array();
                    foreach ($comment_rows as $r) {
                        if ($r->comments)
                            $comments[] = htmlspecialchars($r->comments);
                    }
                    $comments = implode('<br />', $comments);
                    $ok = $rejected = false;
                    $disp_rows = db_select('ebms_review_disposition', 'd')
                        ->fields('d', array('value_id'))
                        ->condition('d.review_id', $review_id)
                        ->execute();
                    foreach ($disp_rows as $disp_row) {
                        $disposition = $disp_row->value_id;
                        if ($disposition == $no_change_option)
                            $rejected = true;
                        else
                            $ok = true;
                    }
                    $disp = $ok ? 'Y' : ($rejected ? 'N' : '');
                    $packet_rows[$p][] = array($reviewer, $disp, $comments);
                    ++$packet_rowspan;
                    ++$article_rowspan;
                }
            }

            // We only display the article abstract once, no matter how
            // many reviews we have.
            $have_abstract_row = false;

            foreach ($packet_rows as $packet => $reviews) {

                // We only display the cycle date once, no matter how many
                // reviews we show for packets created in that cycle.
                $have_packet_row = false;

                // Create a row for each board member's review.
                foreach ($reviews as $review) {
                    $row = array();
                    list($reviewer, $disp, $comments) = $review;
                    if (!$have_abstract_row) {
                        $have_abstract_row = true;
                        $col = array(
                            'data' => ++$counter,
                            'class' => array('col-0'),
                        );
                        if ($article_rowspan > 1)
                            $col['rowspan'] = $article_rowspan;
                        $row[] = $col;
                        $col = array(
                            'data' => $citation,
                            'class' => array('col-1'),
                        );
                        if ($article_rowspan > 1)
                            $col['rowspan'] = $article_rowspan;
                        $row[] = $col;
                        if (!$have_packet_row) {
                            $have_packet_row = true;
                            $col = array(
                                'data' => $packet,
                                'class' => array('col-2'),
                            );
                            if ($packet_rowspan > 1)
                                $col['rowspan'] = $packet_rowspan;
                            $row[] = $col;
                        }
                    }
                    $img = '';
                    if ($disp) {
                        if ($disp == 'Y')
                            $png = 'EBMS_Pass_Icon_Active.png';
                        else
                            $png = 'EBMS_Selection_X_Icon.png';
                        $url = Ebms\IMAGES_DIR . '/' . $png;
                        $img = '<img src="' . $url . '" alt="" />';
                    }
                    $data = '<span class="reviewer-name">' .
                        htmlspecialchars($reviewer) . '</span> ' . $img;
                    $row[] = array(
                        'data' => $data,
                        'class' => array('col-3'),
                    );
                    $row[] = array(
                        'data' => $comments,
                        'class' => array('col-4'),
                    );
                    $rows[] = array(
                        'data' => $row,
                        'no_striping' => true,
                    );
                }
            }
        }

        // Assemble the table.
        $id = 'review-report-title';
        $title = 'Literature Review Reports';
        $table = array(
            'title' => array(
                '#type' => 'markup',
                '#markup' => "<h2 id='$id'>$title ($counter)</h2>",
            ),
            'table' => array(
                '#attributes' => array('id' => 'review-report'),
                '#theme' => 'table',
                '#rows' => $rows,
                '#header' => $headers,
            ),
        );
        return $table;
    }

    /**
     * Report on board (and optionally subgroup) membership.
     *
     *  @param  int $request_id  Primary key of row in database table
     *                           holding the paramaters specified for this
     *                           report request.
     *
     *  @return array            Drupal render array for the report
     *                           table and its title.
     */
    static function membership($request_id) {
        $params = EbmsReports::fetch_report_request($request_id);
        //pdq_ebms_debug('MEMBERSHIP', $params);
        $board_id = $params['board'];
        $query = db_select('users', 'u');
        $query->join('ebms_board_member', 'm', 'm.user_id = u.uid');
        $query->join('users_roles', 'ur', 'ur.uid = u.uid');
        $query->join('role', 'r', 'r.rid = ur.rid');
        $query->condition('r.name', 'board member');
        $query->condition('m.board_id', $board_id);
        $query->fields('u', array('name', 'status'));
        //if (!$params['include-inactive'])
        $query->condition('u.status', 1);
        $query->orderBy('u.name');
        $results = $query->execute();
        $items = array();
        foreach ($results as $result) {
            $name = htmlspecialchars($result->name);
            if (!$result->status)
                $name .= ' (inactive)';
            $items[] = $name;
        }
        $report = array(
            'board-members' => array(
                '#theme' => 'item_list',
                '#title' => Ebms\Util::get_board_name($board_id),
                '#items' => $items,
            ),
        );
        if ($params['show-subgroups']) {
            $subgroups = db_select('ebms_subgroup', 's')
                ->fields('s', array('sg_id', 'sg_name'))
                ->condition('s.board_id', $board_id)
                ->orderBy('s.sg_name')
                ->execute();
            foreach($subgroups as $subgroup) {
                $query = db_select('users', 'u');
                $query->join('ebms_subgroup_member', 'm', 'm.user_id = u.uid');
                $query->condition('m.sg_id', $subgroup->sg_id);
                $query->condition('u.status', 1);
                $query->orderBy('u.name');
                $query->fields('u', array('name', 'status'));
                $items = array();
                $results = $query->execute();
                foreach ($results as $result) {
                    $name = $result->name;
                    if (!$result->status)
                        $name .= ' (*)';
                    $items[] = htmlspecialchars($name);
                }
                if (empty($items))
                    $items[] = '(No active members of subgroup)';
                $report['sg-' . $subgroup->sg_id] = array(
                    '#theme' => 'item_list',
                    '#title' => htmlspecialchars($subgroup->sg_name),
                    '#items' => $items,
                );
            }
        }
        return $report;
    }

    /**
     * The processing model has the submit callback handler for
     * the report request form save the user's filtering criteria
     * for a report in the database and then invoke the report
     * URL indicating which row in the database table has the
     * report criteria.  The values are serialized using json
     * encoding.
     *
     *  @param  array   $params    The user's criteria for the
     *                             report
     *  @param  string  $name      The name of the requested report
     *
     *  @return int                Primary key into the EBMS table
     *                             which stores the report requests
     */
    public static function save_report_request($params, $name) {
        global $user;
        $now = strftime('%Y-%m-%d %H:%M:%S');
        return db_insert('ebms_report_request')
            ->fields(
                array(
                    'report_name' => $name,
                    'requestor_id' => $user->uid,
                    'submitted' => $now,
                    'parameters' => drupal_json_encode($params),
                )
            )
            ->execute();
    }

    /**
     * Retrieves the array of report request parameters from the
     * database.  Decodes the string used for serializing the
     * value.
     *
     *  @param  int   $request_id   Primary key into the EBMS table
     *                              which stores the report requests
     *
     *  @return array               Report request parameters
     */
    public static function fetch_report_request($request_id) {
        $json = db_select('ebms_report_request', 'r')
            ->fields('r', array('parameters'))
            ->condition('r.request_id', $request_id)
            ->execute()
            ->fetchField();
        return drupal_json_decode($json);
    }

    /**
     * Returns the array of article states representing actions
     * taken by the board manager after she has looked over the
     * reviews of an article sent back by her board members.
     * The technique used here is to find the sequence number
     * of a state which is known to be in this group of states
     * (and which we are assured will always be in this group)
     * and then find all of the states which share this same
     * sequence number.  For further information on article
     * states and sequence numbers see the documentation of the
     * ebms_article_state and ebms_article_state_type tables
     * in ebms.sql.
     *
     * Because we can need to use this array in more than one place
     * in a single page request, we cache the results of the
     * database queries in a static class property, and only
     * execute the queries once per page request.
     *
     *  @return array         Dictionary of article state types,
     *                        with primary keys into the
     *                        ebms_article_state_type table as
     *                        the array indexes, and the display
     *                        string for the state as the element
     *                        values
     */
    public static function manager_actions() {
        if (!EbmsReports::$_manager_actions) {
            $full_end_sequence = db_select('ebms_article_state_type', 't')
                ->condition('t.state_text_id', 'FullEnd')
                ->fields('t', array('sequence'))
                ->execute()
                ->fetchField();
            $results = db_select('ebms_article_state_type', 't')
                ->fields('t', array('state_id', 'state_name'))
                ->condition('t.sequence', $full_end_sequence)
                ->execute();
            EbmsReports::$_manager_actions = array();
            foreach ($results as $r)
                EbmsReports::$_manager_actions[$r->state_id] = $r->state_name;
        }
        return EbmsReports::$_manager_actions;
    }

    /**
     * Common routine to generate the HTML markup for the report date
     * block for the meta-data at the top of one of the citation reports.
     */
    static function format_report_date() {
        $d = date('m/d/Y');
        return '<div class="meta"><span class="label">Report Date:</span> ' .
            $d . '</div>';
    }

    /**
     * Common routine to generate the HTML markup for the board name
     * block for the meta-data at the top of one of the citation reports.
     */
    static function format_board($board_name) {
        return '<div class="meta"><span class="label">Board:</span> '.
            $board_name . '</div>';
    }

    /**
     * Common routine to generate the HTML markup for the cycle name
     * block for the meta-data at the top of one of the citation reports.
     */
    static function format_cycle_name($cycle) {
        $cycle_name = Ebms\Util::get_cycle_name($cycle);
        return '<div class="meta"><span class="label">Review Cycle:</span> ' .
            $cycle_name . '</div>';
    }

    /**
     * Common routine to format a Drupal form date field in the pattern
     * MM/DD/YYYY.
     *
     *  @param  array  $date    Drupal form date field with separate
     *                          elements for 'year', 'month', and 'day'
     *
     *  @return string          Date string as MM/DD/YYYY
     */
    static function format_date($date) {
        $y = $date['year'];
        $m = $date['month'];
        $d = $date['day'];
        if (!$y || !$m || !$d)
            return '';
        $t = mktime(0, 0, 0, $m, $d, $y);
        return date('m/d/Y', $t);
    }

    /**
     * Common routine to generate the HTML markup for a report's date
     * range block for the meta-data at the top of one of the citation
     * reports.
     *
     *  @param  array  $start   Drupal form date field for the beginning
     *                          of a date range
     *  @param  array  $end     Drupal form date field for the end of a
     *                          date range
     *
     *  @return string          HTML markup for date range block
     */
    static function format_date_range($start, $end) {
        $s = EbmsReports::format_date($start);
        $e = EbmsReports::format_date($end);
        if (!$s || !$e)
            return '';
        return '<div class="meta"><span class="label">Date Range:</span> ' .
            $s . ' - ' . $e . '</div>';
    }
}

/**
 * Extension of the core Drupal EntityFieldQuery class.  Used for
 * filtering meeting events by date and/or type.  By default the
 * query represented by the object will find all nodes with type
 * ebms_event, but there are methods for narrowing down the results.
 */
class EbmsEventQuery extends EntityFieldQuery {

    /**
     * Set some defaults for the query object.
     */
    public function __construct() {
        $this->entityCondition('entity_type', 'node')
            ->propertyCondition('status', 1)
            ->entityCondition('bundle', 'ebms_event');
    }

    /**
     * If the user has asked to filter meetings by date, apply the
     * appropriate field condition.  It is acceptable for the user
     * to have specified more general components of the date (for
     * example, a year and month without a day, or just a year) but
     * not the other way around (can't have a day without both a
     * month and year specified, for example).  The form validation
     * routine will have enforced this assumption.
     *
     *  @param  array   $date   Structured array representing the
     *                          separate values for year, month,
     *                          and day components of a Drupal
     *                          stock date form field.
     *
     *  @return                 None
     */
    public function restrict_by_date($date) {
        $y = $date['year'];
        $m = $date['month'];
        $d = $date['day'];
        if ($d) {
            $start = mktime(0, 0, 0, $m, $d, $y);
            $end = mktime(0, 0, -1, $m, $d + 1, $y);
        }
        elseif ($m) {
            $start = mktime(0, 0, 0, $m, 1, $y);
            $end = mktime(0, 0, -1, $m + 1, 1, $y);
        }
        elseif ($y) {
            $start = mktime(0, 0, 0, 1, 1, $y);
            $end = mktime(0, 0, -1, 1, 1, $y + 1);
        }
        else
            return;
        $field = 'field_datespan';
        $this->fieldCondition($field, 'value', array($start, $end), 'BETWEEN');
    }

    /**
     * If the user wants to filter meetings by whether they were board-
     * specific or subgroup-specific, apply the appropriate conditions
     * by detecting whether a board (or subgroup) has been specified
     * for the event.  There is an event type field in the event node,
     * but it's used for something else (designating whether a meeting
     * is face-to-face, or teleconferenced).
     *
     * 2013-03-11 (TIR 2446): use the new event_category field,
     * instead of checking for boards or subgroups linked to the
     * event.  Note that this may result in contradictory results
     * (if the user selects an event category value which conflicts
     * with the type of group (board or subgroup) linked to the
     * event.
     *
     *  @param  string  $type    If a type has been specified, it will
     *                           be "board" or "subgroup."
     *
     *  @return                 None
     */
    public function restrict_by_type($type) {
        if ($type)
            $this->fieldCondition('field_event_category', 'value', $type, '=');
    }
}

/**
 * Drupal form API callback which generates the form for all five
 * of the "Citation Reports" available for the librarians.
 *
 *  @param  array   $form         Structured array containing the
 *                                elements and properties of the form
 *  @param  array   &$form_state  Modifiable structured array
 *                                containing the current values
 *                                of the form, as well as other
 *                                form state information
 *
 *  @return array                 Array containing specification
 *                                of the fields to appear on the
 *                                form, as well as other markup
 */
function pdq_ebms_citation_reports_form($form, &$form_state) {

    // Override the breadcrumbs which Drupal would automatically
    // set based on the menu callback, in order to comply with
    // the customized requirements of the EBMS design.
    drupal_set_breadcrumb(
        array(
            l('Reports', 'reports'),
            l('Citation Management Reports', 'reports/citation'),
            'Citation Reports',
        )
    );

    // Initialize defaults for the form field's values.
    $board = $cycle = $report = 0;
    $start_date = $end_date = array('year' => 0, 'month' => 0, 'day' => 0);

    // Check for overrides of the defaults from the form's current state.
    $values = array();
    if (isset($form_state['values'])) {
        $values = $form_state['values'];
        if (isset($values['board']))
            $board = $values['board'];
        if (isset($values['cycle']))
            $cycle = $values['cycle'];
        if (isset($values['report']))
            $report = $values['report'];
        if (isset($values['start-date']))
            $start_date = $values['start-date'];
        if (isset($values['end-date']))
            $end_date = $values['end-date'];
    }

    // Build picklist arrays.
    $boards = array(0 => ' ') + Ebms\Util::boards();
    $cycles = array(0 => ' ');
    $reports = array(0 => ' ') + drupal_map_assoc(
        array(
            'Citations Published',
            'Citations Rejected/Accepted for Publishing',
            'Citations Imported',
            'Citations Not Selected for Full Text Retrieval',
            'Citation Summary Topic Changes',
        )
    );
    $results = db_select('ebms_cycle', 'c')
        ->fields('c', array('cycle_id', 'cycle_name'))
        ->orderBy('c.start_date', 'DESC')
        ->execute();
    foreach ($results as $result)
        $cycles[$result->cycle_id] = $result->cycle_name;

    // Begin the form structure, including the left nav penel.
    // Leave a placeholder for the fields controlled dynamically
    // by an ajax callback invoked when the user changes the
    // Report field's selection.
    $left_nav = EbmsReports::build_left_nav('Citation Management',
                                            'Citation Reports');
    $form['top'] = array(
        '#type' => 'container',
        '#attributes' => array('id' => 'reports-top'),
        'left-nav' => $left_nav,
        'report-form' => array(
            '#type' => 'container',
            '#attributes' => array('id' => 'report-form'),
            'title' => array(
                '#type' => 'markup',
                '#markup' => '<h2>Citation Reports</h2>',
            ),
            'report' => array(
                '#type' => 'select',
                '#title' => 'Reports',
                '#title_display' => 'after',
                '#options' => $reports,
                '#default_value' => $report,
                '#attributes' => array('class' => array('fixed-width')),
                '#ajax' => array(
                    'callback' => 'pdq_ebms_report_ajax',
                    'wrapper' => 'ajax-fields',
                ),
           ),
            'ajax fields' => array(
                '#type' => 'container',
                '#attributes' => array('id' => 'ajax-fields'),
            ),
            'buttons' => array(
                '#type' => 'container',
                '#attributes' => array('id' => 'report-buttons'),
                'reset' => array(
                    '#type' => 'submit',
                    '#value' => 'Reset',
                ),
                'submit' => array(
                    '#type' => 'submit',
                    '#value' => 'Submit',
                ),
            ),
        ),
    );

    // Only one of the five citation reports needs the Board and
    // Cycle fields.
    if (strcmp($report, 'Citation Summary Topic Changes')) {
        $form['top']['report-form']['ajax fields']['board'] = array(
            '#type' => 'select',
            '#title' => 'Editorial Board',
            '#title_display' => 'after',
            '#options' => $boards,
            '#default_value' => $board,
            '#attributes' => array('class' => array('fixed-width')),
        );
        $form['top']['report-form']['ajax fields']['cycle'] = array(
            '#type' => 'select',
            '#title' => 'Review Cycle',
            '#title_display' => 'after',
            '#options' => $cycles,
            '#default_value' => $cycle,
            '#attributes' => array('class' => array('fixed-width')),
        );
    }

    // Two of the five reports allow the user to specify a date range
    // for filtering.
    if (!strcmp($report, 'Citation Summary Topic Changes') ||
        !strcmp($report, 'Citations Not Selected for Full Text Retrieval')) {
        $form['top']['report-form']['ajax fields']['start-date'] = array(
            '#type' => 'date',
            '#title' => 'START DATE',
            '#title_display' => 'after',
            '#default_value' => $start_date,
            '#pre_render' => array('pdq_ebms_fix_date'),
            '#attributes' => array('class' => array('right-aligned-date')),
            '#element_validate' => array('pdq_ebms_date_validate'),
        );
        $form['top']['report-form']['ajax fields']['end-date'] = array(
            '#type' => 'date',
            '#title' => 'END DATE',
            '#title_display' => 'after',
            '#default_value' => $end_date,
            '#pre_render' => array('pdq_ebms_fix_date'),
            '#attributes' => array('class' => array('right-aligned-date')),
            '#element_validate' => array('pdq_ebms_date_validate'),
        );
    }

    // If we actually have a request for one of the reports, invoke
    // the appropriate class method to build the render array for the
    // report's display, and plug that into the form structure.
    if ($report && $values['op'] == 'Submit') {
        switch ($report) {
            case 'Citations Published':
                $form['report'] =
                    EbmsReports::cites_published($board, $cycle);
                break;
            case 'Citations Rejected/Accepted for Publishing':
                $form['report'] =
                    EbmsReports::initial_decision($board, $cycle);
                break;
            case 'Citations Imported':
                $form['report'] =
                    EbmsReports::cites_imported($board, $cycle);
                break;
            case 'Citations Not Selected for Full Text Retrieval':
                $form['report'] =
                    EbmsReports::manager_rejection($board, $cycle,
                        $start_date, $end_date);
                break;
            case 'Citation Summary Topic Changes':
                $form['report'] =
                    EbmsReports::topic_changes($start_date, $end_date);
                break;
        }
    }
    return $form;
}

/**
 * The librarians' five citation reports have different requirements,
 * depending on which report was selected.  Flag any violations of
 * these requirements.
 *
 *  @param  array   $form         Structured array containing the
 *                                elements and properties of the form
 *  @param  array   &$form_state  Modifiable structured array
 *                                containing the current values
 *                                of the form, as well as other
 *                                form state information
 *
 *  @return     none
 */
function pdq_ebms_citation_reports_form_validate($form, &$form_state) {

    // Collect the values we need for validation.
    $values = $form_state['values'];
    $report = $values['report'];
    $cycle = isset($values['cycle']) ? $values['cycle'] : 0;

    // No matter what else is wrong, a report selection is required.
    if (!$report)
        form_set_error('report', 'No report selected');
    else {
        switch ($report) {

            // Three of the reports only require a review cycle selection.
            case 'Citations Published':
            case 'Citations Rejected/Accepted for Publishing':
            case 'Citations Imported':
                if (!$cycle)
                    form_set_error('cycle', 'Please select a review cycle');
                break;

            // For the report on articles rejected by the board manager
            // in her first review pass, make sure the user has not
            // specified both date(s) and a review cycle.  Also,
            // ensure that both dates are complete if a date range is
            // specified.
            case 'Citations Not Selected for Full Text Retrieval':
                $start = $values['start-date'];
                $end = $values['end-date'];
                $have_start = $have_end = false;
                $start_complete = $end_complete = true;
                foreach ($start as $val)
                    if ($val)
                        $have_start = true;
                    else
                        $start_complete = false;
                foreach ($end as $val)
                    if ($val)
                        $have_end = true;
                    else
                        $end_complete = false;
                if ($cycle) {
                    if ($have_start)
                        form_set_error('start-date',
                            'Cannot specify cycle and date range');
                    elseif ($have_end)
                        form_set_error('end-date',
                            'Cannot specify cycle and date range');
                }
                elseif (!$start_complete || !$end_complete)
                    form_set_error('cycle',
                        'Must specify cycle or complete date range');
                break;

            // For the report on articles for which the board manager
            // have added topics not included by the librarian,
            // we must have a fully specified date range.
            case 'Citation Summary Topic Changes':
                $s = $values['start-date'];
                $e = $values['end-date'];
                if (!$s['year'] || !$s['month'] || !$s['day'])
                    form_set_error('start-date',
                        'A complete date range is required');
                elseif (!$e['year'] || !$e['month'] || !$e['day'])
                    form_set_error('end-date',
                        'A complete date range is required');
                break;
        }
    }
}

/**
 * Not much to do here, as all the work, including building the
 * report, is handled in the report form's callback.
 *
 *  @param  array   $form         Structured array containing the
 *                                elements and properties of the form
 *  @param  array   &$form_state  Modifiable structured array
 *                                containing the current values
 *                                of the form, as well as other
 *                                form state information
 *
 *  @return     none
 */
function pdq_ebms_citation_reports_form_submit($form, &$form_state) {
    $form_state['rebuild'] = true;
}

/**
 * Drupal form API callback which generates the form for the
 * report on previous article import jobs.  The report processing
 * works in several steps.  First we display the form, then the
 * user selects filter criteria and submits her request.  We
 * then use those criteria to find the matching jobs and present
 * them in a list.  Then the user selects one of the jobs and
 * we build and display the report showing what happend during
 * the import job.  All of these steps involved this function.
 *
 *  @param  array   $form         Structured array containing the
 *                                elements and properties of the form
 *  @param  array   &$form_state  Modifiable structured array
 *                                containing the current values
 *                                of the form, as well as other
 *                                form state information
 *  @param  int     $job          Primary key into the EBMS table
 *                                for import batch jobs, appended
 *                                to the URL for the links attached
 *                                to each of the jobs listed in the
 *                                intermediate step described above
 *
 *  @return array                 Array containing specification
 *                                of the fields to appear on the
 *                                form, as well as other markup
 */
function pdq_ebms_import_reports_form($form, &$form_state, $job=0) {

    // Override the breadcrumbs which Drupal would automatically
    // set based on the menu callback, in order to comply with
    // the customized requirements of the EBMS design.
    drupal_set_breadcrumb(
        array(
            l('Reports', 'reports'),
            l('Citation Management Reports', 'reports/citation'),
            'Import Reports',
        )
    );

    // Initialize defaults for the form field's values.
    $board = $cycle = $topic = 0;
    $op = '';
    $range = false;
    $start = $end = array('year' => 0, 'month' => 0, 'day' => 0);

    // Check for overrides of the defaults from the form's current state.
    if (isset($form_state['values'])) {
        $values = $form_state['values'];
        $board = $values['board'];
        $topic = $values['topic'];
        $cycle = $values['cycle'];
        $start = $values['import-start'];
        $end = $values['import-end'];
        $range = $values['use-import-date-range'];
        $op = $values['op'];
    }

    // Build picklist arrays.
    $boards = array(0 => ' ') + Ebms\Util::boards();
    $topics = array(0 => ' ');
    $results = db_select('ebms_topic', 't')
        ->fields('t', array('topic_id', 'topic_name'))
        ->orderBy('t.topic_name')
        ->execute();
    foreach ($results as $result)
        $topics[$result->topic_id] = $result->topic_name;
    $cycles = array(0 => ' ');
    $results = db_select('ebms_cycle', 'c')
        ->fields('c', array('cycle_id', 'cycle_name'))
        ->orderBy('c.start_date', 'DESC')
        ->execute();
    foreach ($results as $result)
        $cycles[$result->cycle_id] = $result->cycle_name;

    // Begin the form structure, including the left nav penel.
    $left_nav = EbmsReports::build_left_nav('Citation Management',
                                            'Import Reports');
    $form['top'] = array(
        '#type' => 'container',
        '#attributes' => array('id' => 'fulltext-top'),
        'left-nav' => $left_nav,
        'report-form' => array(
            '#type' => 'container',
            '#attributes' => array('id' => 'report-form'),
            'title' => array(
                '#type' => 'markup',
                '#markup' => '<h2>Import Reports</h2>',
            ),
            'board' => array(
                '#type' => 'select',
                '#title' => 'Editorial Board',
                '#title_display' => 'after',
                '#options' => $boards,
                '#default_value' => $board,
                '#attributes' => array('class' => array('fixed-width')),
            ),
            'topic' => array(
                '#type' => 'select',
                '#title' => 'Summary Topic',
                '#title_display' => 'after',
                '#options' => $topics,
                '#default_value' => $topic,
                '#attributes' => array('class' => array('fixed-width')),
            ),
            'cycle' => array(
                '#type' => 'select',
                '#title' => 'Review Cycle',
                '#title_display' => 'after',
                '#options' => $cycles,
                '#default_value' => $cycle,
                '#attributes' => array('class' => array('fixed-width')),
            ),
            'import-date-line' => array(
                '#type' => 'container',
                '#attributes' => array('class' => array('date-with-range')),
                'import-start' => array(
                    '#type' => 'date',
                    '#element_validate' => array('pdq_ebms_date_validate'),
                    '#default_value' => $start,
                    '#pre_render' => array('pdq_ebms_fix_date'),
                ),
                'use-import-date-range' => array(
                    '#type' => 'checkbox',
                    '#title' => 'RANGE',
                    '#prefix' => '<div class="box-on-right">',
                    '#suffix' => '</div>',
                    '#default_value' => $range,
                ),
            ),
            'import-date-label' => array(
                '#type' => 'markup',
                '#markup' => '<div class="date-label">Import Date</div>',
            ),
            'import-end' => array(
                '#type' => 'date',
                '#element_validate' => array('pdq_ebms_date_validate'),
                '#default_value' => $start,
                '#pre_render' => array('pdq_ebms_fix_date'),
                '#states' => array(
                    'invisible' => array(
                        '#edit-use-input-date-range' => array(
                            'checked' => false
                        ),
                    ),
                ),
            ),
            'buttons' => array(
                '#type' => 'container',
                '#attributes' => array('id' => 'report-buttons'),
                'reset' => array(
                    '#type' => 'submit',
                    '#value' => 'Reset',
                ),
                'submit' => array(
                    '#type' => 'submit',
                    '#value' => 'Submit',
                ),
            ),
        ),
    );

    // If the user has selected a specific import job, show what
    // happened during that job.
    if ($job)
        $form['report'] = EbmsReports::import_job($job);

    // If the user has given us the criteria for selecting import
    // jobs, build and display the list of matching jobs.
    elseif ($op == 'Submit')
        $form['report'] = EbmsReports::import_reports($board, $topic,
                          $cycle, $start, $end, $range);
    return $form;
}

/**
 * If the user clicks the "Reset" button, clear the form by routing
 * back to the URL for the import reports.  Otherwise let the form
 * callback do the work of displaying either a list of import jobs
 * or the report for a single job.
 *
 *  @param  array   $form         Structured array containing the
 *                                elements and properties of the form
 *  @param  array   &$form_state  Modifiable structured array
 *                                containing the current values
 *                                of the form, as well as other
 *                                form state information
 *
 *  @return     none
 */
function pdq_ebms_import_reports_form_submit($form, &$form_state) {
    if ($form_state['values']['op'] == 'Reset')
        drupal_goto('reports/import');
    $form_state['rebuild'] = true;
    return;
}

/**
 * Drupal form API callback which generates the form for the
 * report on non-article documents posted to the system.  We store
 * the report request parameters in a database table and then
 * launch the form with a modified URL which incorporates the
 * ID of the database row where the request parameters was saved.
 *
 *  @param  array   $form         Structured array containing the
 *                                elements and properties of the form
 *  @param  array   &$form_state  Modifiable structured array
 *                                containing the current values
 *                                of the form, as well as other
 *                                form state information
 *  @param  int     $request_id   Primary key into the EBMS table
 *                                for report request parameters
 *
 *  @return array                 Array containing specification
 *                                of the fields to appear on the
 *                                form, as well as other markup
 */
function pdq_ebms_documents_report_form($form, &$form_state, $request_id) {

    // Override the breadcrumbs which Drupal would automatically
    // set based on the menu callback, in order to comply with
    // the customized requirements of the EBMS design.
    drupal_set_breadcrumb(
        array(
            l('Reports', 'reports'),
            l('Board Management Reports', 'reports/board'),
            'Documents',
        )
    );

    // Initialize defaults for the form field's values.
    $board = $tag = $inhouse = $member = $uploader_board = 0;
    $keyword = '';
    $range = $archived = false;
    $start = $end = array('year' => 0, 'month' => 0, 'day' => 0);

    // Check for overrides of the defaults by request parameters
    // stored in the database.
    if ($request_id) {
        $params = EbmsReports::fetch_report_request($request_id);
        $board = $params['board'];
        $tag = $params['tag'];
        $keyword = $params['keyword'];
        $inhouse = $params['inhouse'];
        $member = $params['member'];
        $uploader_board = $params['uploader-board'];
        $archived = $params['archived'];
        $start = $params['upload-start'];
        $range = $params['use-upload-date-range'];
        if ($range)
            $end = $params['upload-end'];
    }

    // Build picklist arrays for boards, tags, roles, etc.
    $boards = Ebms\Util::boards();
    $uploader_boards = array(0 => 'BOARD MEMBERS (BY BOARD)') +
        $boards;
    $boards = array(0 => ' ') + $boards;
    $tags = array(0 => ' ');
    $results = db_select('ebms_tag', 't')
        ->fields('t', array('tag_id', 'tag_name'))
        ->orderBy('t.tag_name')
        ->execute();
    foreach ($results as $result)
        $tags[$result->tag_id] = $result->tag_name;
    $board_member_role = db_select('role', 'r')
        ->fields('r', array('rid'))
        ->condition('r.name', 'board member')
        ->execute()
        ->fetchField();
    $results = db_select('users_roles', 'u')
        ->fields('u', array('uid'))
        ->condition('u.rid', $board_member_role)
        ->execute();
    $board_member_ids = array();
    foreach ($results as $result)
        $board_member_ids[$result->uid] = $result->uid;
    $inhouse_users = array(0 => 'STAFF');
    $board_members = array(0 => 'BOARD MEMBERS');
    $results = db_select('users', 'u')
        ->fields('u', array('uid', 'name'))
        ->condition('u.uid', 0, '>')
        ->condition('u.status', 1)
        ->orderBy('u.name')
        ->execute();
    foreach ($results as $result)
        if (array_key_exists($result->uid, $board_member_ids))
            $board_members[$result->uid] = $result->name;
        else
            $inhouse_users[$result->uid] = $result->name;

    // Begin the form structure, including the left nav penel.
    $left_nav = EbmsReports::build_left_nav('Board Management',
                                            'Documents');
    $form['top'] = array(
        '#type' => 'container',
        '#attributes' => array('id' => 'documents-top'),
        'left-nav' => $left_nav,
        'report-form' => array(
            '#type' => 'container',
            '#attributes' => array('id' => 'report-form'),
            'title' => array(
                '#type' => 'markup',
                '#markup' => '<h2>Documents</h2>',
            ),
            'board' => array(
                '#type' => 'select',
                '#title' => 'Editorial Board',
                '#title_display' => 'after',
                '#options' => $boards,
                '#default_value' => $board,
                '#attributes' => array('class' => array('fixed-width')),
            ),
            'tag' => array(
                '#type' => 'select',
                '#title' => 'Tag',
                '#title_display' => 'after',
                '#options' => $tags,
                '#default_value' => $tag,
                '#attributes' => array('class' => array('fixed-width')),
            ),
            'keyword' => array(
                '#type' => 'textfield',
                '#title' => 'Keyword',
                '#title_display' => 'after',
                '#default_value' => $keyword,
                '#attributes' => array('class' => array('fixed-width')),
            ),
            'inhouse' => array(
                '#type' => 'select',
                '#title' => 'Uploaded By',
                '#title_display' => 'after',
                '#options' => $inhouse_users,
                '#default_value' => $inhouse,
                '#attributes' => array('class' => array('fixed-width')),
            ),
            'member' => array(
                '#type' => 'select',
                '#title' => 'Board Members',
                '#title_display' => 'none',
                '#options' => $board_members,
                '#default_value' => $member,
                '#attributes' => array('class' => array('fixed-width')),
            ),
            'uploader-board' => array(
                '#type' => 'select',
                '#title' => 'Uploader Board',
                '#title_display' => 'none',
                '#options' => $uploader_boards,
                '#default_value' => $uploader_board,
                '#attributes' => array('class' => array('fixed-width')),
            ),
            'upload-date-line' => array(
                '#type' => 'container',
                '#attributes' => array('class' => array('date-with-range')),
                'upload-start' => array(
                    '#type' => 'date',
                    '#element_validate' => array('pdq_ebms_date_validate'),
                    '#default_value' => $start,
                    '#pre_render' => array('pdq_ebms_fix_date'),
                ),
                'use-upload-date-range' => array(
                    '#type' => 'checkbox',
                    '#title' => 'RANGE',
                    '#prefix' => '<div class="box-on-right">',
                    '#suffix' => '</div>',
                    '#default_value' => $range,
                ),
            ),
            'upload-date-label' => array(
                '#type' => 'markup',
                '#markup' => '<div class="date-label">Date Uploaded</div>',
            ),
            'upload-end' => array(
                '#type' => 'date',
                '#element_validate' => array('pdq_ebms_date_validate'),
                '#default_value' => $end,
                '#pre_render' => array('pdq_ebms_fix_date'),
                '#states' => array(
                    'invisible' => array(
                        '#edit-use-upload-date-range' => array(
                            'checked' => false
                        ),
                    ),
                ),
            ),
            'archived' => array(
                '#type' => 'checkbox',
                '#title' => 'ARCHIVED',
                '#default_value' => $archived,
            ),
            'buttons' => array(
                '#type' => 'container',
                '#attributes' => array('id' => 'report-buttons'),
                'reset' => array(
                    '#type' => 'submit',
                    '#value' => 'Reset',
                ),
                'filter' => array(
                    '#type' => 'submit',
                    '#value' => 'Filter',
                ),
            ),
        ),
    );

    // Even before the user has given us any filter criteria, we are
    // required to show a report showing all posted documents.
    $form['report'] = EbmsReports::docs($request_id);
    return $form;
}

/**
 * If the user clicks the "Reset" button, clear the form by routing
 * back to the URL for the import reports.  Otherwise store the report
 * request parameters in the database and send the user to the URL
 * which knows where those parameters were stored.
 *
 *  @param  array   $form         Structured array containing the
 *                                elements and properties of the form
 *  @param  array   &$form_state  Modifiable structured array
 *                                containing the current values
 *                                of the form, as well as other
 *                                form state information
 *
 *  @return     none
 */
function pdq_ebms_documents_report_form_submit($form, &$form_state) {
    if ($form_state['values']['op'] == 'Reset')
        drupal_goto('reports/documents');
    $values = $form_state['values'];
    $request_id = EbmsReports::save_report_request($values, 'Documents');
    drupal_goto('reports/documents/' . $request_id);
}

/**
 * Drupal form API callback which generates the form for the
 * report on board/subgroup membership.  We store the report
 * request parameters in a database table and then launch the
 * form with a modified URL which incorporates the ID of the
 * database row where the request parameters was saved.
 *
 *  @param  array   $form         Structured array containing the
 *                                elements and properties of the form
 *  @param  array   &$form_state  Modifiable structured array
 *                                containing the current values
 *                                of the form, as well as other
 *                                form state information
 *  @param  int     $request_id   Primary key into the EBMS table
 *                                for report request parameters
 *
 *  @return array                 Array containing specification
 *                                of the fields to appear on the
 *                                form, as well as other markup
 */
function pdq_ebms_membership_report_form($form, &$form_state, $request_id) {

    // Override the breadcrumbs which Drupal would automatically
    // set based on the menu callback, in order to comply with
    // the customized requirements of the EBMS design.
    drupal_set_breadcrumb(
        array(
            l('Reports', 'reports'),
            l('Board Management Reports', 'reports/board'),
            'Board Membership',
        )
    );

    // Initialize defaults for the form field's values.
    $board = 0;
    //$include_inactive = false;
    $show_subgroups = false;

    // Check for overrides of the defaults by request parameters
    // stored in the database.
    if ($request_id) {
        $params = EbmsReports::fetch_report_request($request_id);
        $board = $params['board'];
        //$include_inactive = $params['include-inactive'];
        $show_subgroups = $params['show-subgroups'];
    }

    // Build picklist array for boards.
    $boards = array(0 => ' ') + Ebms\Util::boards();

    // Begin the form structure, including the left nav penel.
    $left_nav = EbmsReports::build_left_nav('Board Management',
                                            'Board Membership');
    $form['top'] = array(
        '#type' => 'container',
        '#attributes' => array('id' => 'membership-top'),
        'left-nav' => $left_nav,
        'report-form' => array(
            '#type' => 'container',
            '#attributes' => array('id' => 'report-form'),
            'title' => array(
                '#type' => 'markup',
                '#markup' => '<h2>Board Membership</h2>',
            ),
            'board' => array(
                '#type' => 'select',
                '#title' => 'Editorial Board',
                '#title_display' => 'after',
                '#options' => $boards,
                '#default_value' => $board,
                '#attributes' => array('class' => array('fixed-width')),
            ),
            /*
            'include-inactive' => array(
                '#type' => 'checkbox',
                '#title' => 'INCLUDE INACTIVE USERS',
                '#default_value' => $include_inactive,
            ),*/
            'show-subgroups' => array(
                '#type' => 'checkbox',
                '#title' => 'SHOW SUBGROUPS',
                '#default_value' => $show_subgroups,
            ),
            'buttons' => array(
                '#type' => 'container',
                '#attributes' => array('id' => 'report-buttons'),
                'filter' => array(
                    '#type' => 'submit',
                    '#value' => 'Report',
                ),
            ),
        ),
    );

    // Append the report if we have a request.
    if ($request_id)
        $form['report'] = EbmsReports::membership($request_id);
    return $form;
}

/**
 * Make sure the user has selected a board, which is the only
 * field required for the membership report.
 *
 *  @param  array   $form         Structured array containing the
 *                                elements and properties of the form
 *  @param  array   &$form_state  Modifiable structured array
 *                                containing the current values
 *                                of the form, as well as other
 *                                form state information
 *
 *  @return     none
 */
function pdq_ebms_membership_report_form_validate($form, &$form_state) {

    // Collect the values we need for validation.
    $values = $form_state['values'];

    // Make sure a board has been selected;
    if (!$values['board'])
        form_set_error('board', 'You must select a board for this report.');
}

/**
 * Store the report request parameters in the database and send the
 * user to the URL which knows where those parameters were stored.
 *
 *  @param  array   $form         Structured array containing the
 *                                elements and properties of the form
 *  @param  array   &$form_state  Modifiable structured array
 *                                containing the current values
 *                                of the form, as well as other
 *                                form state information
 *
 *  @return     none
 */
function pdq_ebms_membership_report_form_submit($form, &$form_state) {
    $values = $form_state['values'];
    $request_id = EbmsReports::save_report_request($values, 'Membership');
    drupal_goto('reports/membership/' . $request_id);
}

/**
 * Drupal form API callback which generates the form for replacing
 * the file uploaded for a document posted in the EBMS.  This
 * form is invoked from links provided on the Documents report.
 *
 *  @param  array   $form         Structured array containing the
 *                                elements and properties of the form
 *  @param  array   &$form_state  Modifiable structured array
 *                                containing the current values
 *                                of the form, as well as other
 *                                form state information
 *  @param  int     $doc_id       Primary key for the document we are
 *                                editing
 *  @param  int     $report_id    ID of the report from which we got
 *                                to this page (so the user can return
 *                                via the breadcrumb links)
 *
 *  @return array                 Array containing specification
 *                                of the fields to appear on the
 *                                form, as well as other markup
 */
function pdq_ebms_doc_edit_form($form, &$form_state, $doc_id, $report_id) {

    // Fetch the name of the file from the database
    $query = db_select('ebms_doc', 'd');
    $query->join('file_managed', 'f', 'f.fid = d.file_id');
    $query->fields('f', array('filename'));
    $query->condition('d.doc_id', $doc_id);
    //pdq_ebms_debug('DOC EDIT FORM QUERY', array('q: ' . $query,
    //        $query->arguments()));
    $file_name = htmlspecialchars($query->execute()->fetchField());

    // Set up the custom breadcrumbs, including a link back to the
    // report from which we came, with all the parameters controlling
    // things like report sorting and paging.
    $parms = drupal_get_query_parameters();
    $parent_url = 'reports/documents';
    if ($report_id)
        $parent_url .= "/$report_id";
    drupal_set_breadcrumb(
        array(
            l('Reports', 'reports'),
            l('Board Management Reports', 'reports/board'),
            l('Documents', $parent_url, array('query' => $parms)),
            $file_name,
        )
    );

    // Send back the structured array specifying the form's fields.
    return array(
        'parent-url' => array(
            '#type' => 'hidden',
            '#value' => $parent_url,
         ),
        'parent-parms' => array(
            '#type' => 'hidden',
            '#value' => drupal_json_encode($parms),
         ),
        'title' => array(
            '#type' => 'markup',
            '#markup' => "<h2>Replace $file_name</h2>",
        ),
        'file' => array(
            '#type' => 'file',
            '#title' => 'Replacement File',
            '#element_validate' => array('pdq_ebms_doc_file_required'),
        ),
        'doc_id' => array(
            '#type' => 'hidden',
            '#value' => $doc_id,
        ),
        'submit' => array(
            '#type' => 'submit',
            '#value' => 'Submit',
        ),
        'cancel' => array(
            '#type' => 'submit',
            '#value' => 'Cancel',
        ),
    );
}

/**
 * Make sure a file of an acceptable type is being saved.
 */
function pdq_ebms_doc_edit_form_validate($form, &$form_state) {

    // Find out whether the doc has been tagged as a summary.
    $doc_id = $form_state['values']['doc_id'];
    $query = db_select('ebms_doc', 'd');
    $query->join('ebms_doc_tag', 'dt', 'dt.doc_id = d.doc_id');
    $query->join('ebms_tag', 't', 't.tag_id = dt.tag_id');
    $query->condition('t.tag_name', 'summary');
    $query->condition('d.doc_id', $doc_id);
    $count = $query->countQuery()->execute()->fetchField();
    $is_summary = $count > 0;

    // Get some information about the file just uploaded.
    $field_name = 'file';
    $files = $_FILES['files'];
    $filename = $files['name'][$field_name];
    $path = $files['tmp_name'][$field_name];
    if (!$filename) {
        form_set_error($field_name, 'You must select a file to upload.');
        return;
    }
    $period = strpos($filename, '.');
    if ($period === false || $period < 1) {
        form_set_error($field_name,
            'Unable to determine filename extension for ' .
            htmlspecialchars($filename));
        return;
    }
    $pieces = explode('.', $filename);
    $extension = strtolower(array_pop($pieces));

    // More stringent checking for summary documents.
    if ($is_summary) {
        $acceptable = array(
            'pdf' => 'application/pdf',
            'rtf' => 'text/rtf',
            'doc' => 'application/msword',
            'docx' => 'application/msword'
        );
        if (!array_key_exists($extension, $acceptable)) {
            $message = 'Summary document must be one of pdf, rtf, doc, docx.';
            form_set_error($field_name, $message);
            return;
        }
        $mime_type = Ebms\Util::get_mime_type($path);
        $expected = $acceptable[$extension];
        if ($mime_type != $expected) {
            $filename = htmlspecialchars($filename);
            form_set_error($field_name, "$filename is not of type $expected.");
        }
        return;
    }

    // Just check file extension for everything else.
    $ok = explode(' ', Ebms\DOC_TYPES);
    if (!in_array($extension, $ok)) {
        $extensions = implode(', ', $ok);
        form_set_error($field_name, "Document must be one of $extensions.");
    }
}

/**
 * Swap in the replacement file for the document being edited.
 * Post a message indicating success or failure, as appropriate.
 * Send the user back to the report from which she launched this form.
 * XXX Consider possibly marking the file being replaced for garbage
 * collection, if we can determine safely that it's not being used
 * anywhere else.
 *
 *  @param  array   $form         Structured array containing the
 *                                elements and properties of the form
 *  @param  array   &$form_state  Modifiable structured array
 *                                containing the current values
 *                                of the form, as well as other
 *                                form state information
 *
 *  @return     none
 */
function pdq_ebms_doc_edit_form_submit($form, &$form_state) {
    if ($form_state['values']['op'] == 'Submit') {
        $doc_id = $form_state['values']['doc_id'];
        $validators = array('file_validate_extensions' => array());
        $file = file_save_upload('file', $validators);
        if ($file) {
            $file = file_move($file, 'public://');
            $file->status |= FILE_STATUS_PERMANENT;
            $file = file_save($file);
            db_update('ebms_doc')
                ->fields(array('file_id' => $file->fid))
                ->condition('doc_id', $doc_id)
                ->execute();
            drupal_set_message('Posted document ' . $file->filename . '.');
        }
        else
            drupal_set_message('File not found', 'error');
    }
    $url = $form_state['values']['parent-url'];
    $parms = drupal_json_decode($form_state['values']['parent-parms']);
    drupal_goto($url, array('query' => $parms));
}

/**
 * Drupal form API callback which generates the form for the
 * report on board member reviews for articles in packets
 * assigned to them.
 *
 *  @param  array   $form         Structured array containing the
 *                                elements and properties of the form
 *  @param  array   &$form_state  Modifiable structured array
 *                                containing the current values
 *                                of the form, as well as other
 *                                form state information
 *  @param  int     $request_id   Primary key into the EBMS table
 *                                for report request parameters
 *
 *  @return array                 Array containing specification
 *                                of the fields to appear on the
 *                                form, as well as other markup
 */
function pdq_ebms_lit_review_report_form($form, &$form_state, $request_id=0) {

    // Override the breadcrumbs which Drupal would automatically
    // set based on the menu callback, in order to comply with
    // the customized requirements of the EBMS design.
    drupal_set_breadcrumb(
        array(
            l('Reports', 'reports'),
            l('Citation Management Reports', 'reports/cite'),
            'Literature Reviews',
        )
    );

    // Initialize defaults for the form field's values.
    $board = $topic = $reviewer = $cycle = $rrange = $prange = $packet = 0;
    $rstart = $rend = $pstart = $pend = Ebms\Util::empty_date();

    // Check for overrides of the defaults by request parameters
    // stored in the database.
    if ($request_id) {
        $values = EbmsReports::fetch_report_request($request_id);
        $board = $values['board'];
        $topic = $values['topic'];
        $reviewer = $values['reviewer'];
        $cycle = $values['cycle'];
        $packet = $values['packet'];
        $rrange = $values['rrange'];
        $prange = $values['prange'];
        $rstart = $values['rstart'];
        $pstart = $values['pstart'];
        $rend = $values['rend'];
        $pend = $values['pend'];
    }

    // Build picklist arrays (except topics, handled below).
    $boards = array(0 => ' ') + Ebms\Util::boards();
    $cycles = array(0 => ' ') + Ebms\Util::cycles(false);
    $packets = array(0 => ' ');
    $reviewers = array(0 => ' ');
    $board_member_role = db_select('role', 'r')
        ->fields('r', array('rid'))
        ->condition('r.name', 'board member')
        ->execute()
        ->fetchField();
    $query = db_select('users', 'u');
    $query->join('users_roles', 'r', 'r.uid = u.uid');
    $query->condition('r.rid', $board_member_role);
    $query->fields('u', array('uid', 'name'));
    $results = $query->execute();
    foreach ($results as $result)
        $reviewers[$result->uid] = $result->name;

    // See if the board field has changed value since the last request
    // was stored in the database (needed in case of an ajax callback).
    if (isset($form_state['values'])) {
        $values = $form_state['values'];
        $board = $values['board'];
        $topic = $values['topic'];
    }

    // Use the board choice to fill the topic picklist with the topics
    // connected with the chosen board.
    // OCEEBMS-70: same for packets.
    $topics = array(0 => ' ');
    if ($board) {
         $results = db_select('ebms_topic', 't')
            ->fields('t', array('topic_id', 'topic_name'))
            ->condition('t.board_id', $board)
            ->orderBy('t.topic_name')
            ->execute();
        foreach ($results as $result)
            $topics[$result->topic_id] = $result->topic_name;
        if (!array_key_exists($topic, $topics))
            $topic = 0;
        $query = db_select('ebms_packet', 'p')
            ->fields('p', array('packet_id', 'packet_title'));
        if ($topic)
            $query->condition('p.topic_id', $topic);
        else {
            $query->join('ebms_topic', 't', 't.topic_id = p.topic_id');
            $query->condition('t.board_id', $board);
        }
        $query->orderBy('p.packet_title');
        //pdq_ebms_debug('PACKET QUERY', $query);
        $results = $query->execute();
        foreach ($results as $result)
            $packets[$result->packet_id] = $result->packet_title;
        if (!array_key_exists($packet, $packets))
            $packet = 0;
    }

    // Begin the form structure, including the left nav penel.
    $left_nav = EbmsReports::build_left_nav('Citation Management',
                                            'Literature Reviews');
    $form['top'] = array(
        '#type' => 'container',
        '#attributes' => array('id' => 'lit-review-report-top'),
        'left-nav' => $left_nav,
        'report-form' => array(
            '#type' => 'container',
            '#attributes' => array('id' => 'report-form'),
            'title' => array(
                '#type' => 'markup',
                '#markup' => '<h2>Literature Reviews</h2>',
            ),
            'board' => array(
                '#type' => 'select',
                '#title' => 'Editorial Board *',
                '#title_display' => 'after',
                '#options' => $boards,
                '#default_value' => $board,
                '#attributes' => array('class' => array('fixed-width')),
                '#ajax' => array(
                    'callback' => 'pdq_ebms_report_ajax',
                    'wrapper' => 'board-dependent',
                ),
            ),
            'ajax fields' => array(
                '#type' => 'container',
                '#attributes' => array('id' => 'board-dependent'),
                'topic' => array(
                    '#type' => 'select',
                    '#title' => 'Summary Topic',
                    '#title_display' => 'after',
                    '#options' => $topics,
                    '#default_value' => $topic,
                    '#attributes' => array('class' => array('fixed-width')),
                    '#ajax' => array(
                        'callback' => 'pdq_ebms_report_topic_ajax',
                        'wrapper' => 'topic-dependent',
                    ),
                ),
                'reviewer' => array(
                    '#type' => 'select',
                    '#title' => 'Reviewer',
                    '#title_display' => 'after',
                    '#options' => $reviewers,
                    '#default_value' => $reviewer,
                    '#attributes' => array('class' => array('fixed-width')),
                ),
                'cycle' => array(
                    '#type' => 'select',
                    '#title' => 'Review Cycle',
                    '#title_display' => 'after',
                    '#options' => $cycles,
                    '#default_value' => $cycle,
                    '#attributes' => array('class' => array('fixed-width')),
                ),
                'topic dependent' => array(
                    '#type' => 'container',
                    '#attributes' => array('id' => 'topic-dependent'),
                    'packet' => array(
                        '#type' => 'select',
                        '#title' => 'Packet Name',
                        '#title_display' => 'after',
                        '#options' => $packets,
                        '#default_value' => $packet,
                        '#attributes' => array(
                            'class' => array('fixed-width'),
                        ),
                    ),
                ),
            ),
            'review-date-line' => array(
                '#type' => 'container',
                '#attributes' => array('class' => array('date-with-range')),
                'rstart' => array(
                    '#type' => 'date',
                    '#element_validate' => array('pdq_ebms_date_validate'),
                    '#default_value' => $rstart,
                    '#pre_render' => array('pdq_ebms_fix_date'),
                ),
                'rrange' => array(
                    '#type' => 'checkbox',
                    '#title' => 'RANGE',
                    '#prefix' => '<div class="box-on-right">',
                    '#suffix' => '</div>',
                    '#default_value' => $rrange,
                ),
            ),
            'review-date-label' => array(
                '#type' => 'markup',
                '#markup' =>
                '<div class="date-label">Date Review Submitted</div>',
            ),
            'rend' => array(
                '#type' => 'date',
                '#element_validate' => array('pdq_ebms_date_validate'),
                '#default_value' => $rend,
                '#pre_render' => array('pdq_ebms_fix_date'),
                '#states' => array(
                    'invisible' => array(
                        '#edit-rrange' => array('checked' => false),
                    ),
                ),
            ),
            'packet-date-line' => array(
                '#type' => 'container',
                '#attributes' => array('class' => array('date-with-range')),
                'pstart' => array(
                    '#type' => 'date',
                    '#element_validate' => array('pdq_ebms_date_validate'),
                    '#default_value' => $pstart,
                    '#pre_render' => array('pdq_ebms_fix_date'),
                ),
                'prange' => array(
                    '#type' => 'checkbox',
                    '#title' => 'RANGE',
                    '#prefix' => '<div class="box-on-right">',
                    '#suffix' => '</div>',
                    '#default_value' => $prange,
                ),
            ),
            'packet-date-label' => array(
                '#type' => 'markup',
                '#markup' =>
                '<div class="date-label">Date Packet Created</div>',
            ),
            'pend' => array(
                '#type' => 'date',
                '#element_validate' => array('pdq_ebms_date_validate'),
                '#default_value' => $pend,
                '#pre_render' => array('pdq_ebms_fix_date'),
                '#states' => array(
                    'invisible' => array(
                        '#edit-prange' => array('checked' => false),
                    ),
                ),
            ),
            'buttons' => array(
                '#type' => 'container',
                '#attributes' => array('id' => 'report-buttons'),
                'reset' => array(
                    '#type' => 'submit',
                    '#value' => 'Reset',
                ),
                'filter' => array(
                    '#type' => 'submit',
                    '#value' => 'Submit',
                ),
            ),
        ),
    );

    // If the user has given us report filter parameters, build and
    // show the report below the form.
    if ($request_id)
        $form['report'] = EbmsReports::reviews($request_id);
    return $form;
}

/**
 * Only the Editorial Board field is required for this report (though
 * specifying only a board runs a high risk of timing out the request,
 * and would provide a report which wouldn't be very useful, so it's
 * unlikely to be invoked that way).
 *
 * 2012-12-31: added code to ensure that date ranges don't specify
 * a 'from' date which is later than the 'to' date (TIR #2226).
 *
 *  @param  array   $form         Structured array containing the
 *                                elements and properties of the form
 *  @param  array   &$form_state  Modifiable structured array
 *                                containing the current values
 *                                of the form, as well as other
 *                                form state information
 *
 *  @return     none
 */
function pdq_ebms_lit_review_report_form_validate($form, &$form_state) {
    $values = $form_state['values'];
    if ($values['op'] == 'Submit') {
        if (!$values['board'])
            form_set_error('board', 'No board selected.');
        if ($values['prange']) {
            if (!$values['pend']['day'] || !$values['pstart']['day'])
                form_set_error('pstart',
                    'Complete from and to dates required for date range.');
            else {
                $from = Ebms\Util::date_field_to_timestamp($values['pstart']);
                $to = Ebms\Util::date_field_to_timestamp($values['pend']);
                if ($from > $to)
                    form_set_error('pstart', 'Invalid date range.');
            }
        }
        if ($values['rrange']) {
            if (!$values['rend']['day'] || !$values['rstart']['day'])
                form_set_error('rstart',
                    'Complete from and to dates required for date range.');
            else {
                $from = Ebms\Util::date_field_to_timestamp($values['rstart']);
                $to = Ebms\Util::date_field_to_timestamp($values['rend']);
                if ($from > $to)
                    form_set_error('rstart', 'Invalid date range');
            }
        }
    }
}

/**
 * If the user clicks the "Reset" button, clear the form by routing
 * back to the URL for the literature review reports.  Otherwise store
 * the report request parameters in the database and send the user to
 * the URL which knows where those parameters were stored.
 *
 *  @param  array   $form         Structured array containing the
 *                                elements and properties of the form
 *  @param  array   &$form_state  Modifiable structured array
 *                                containing the current values
 *                                of the form, as well as other
 *                                form state information
 *
 *  @return     none
 */
function pdq_ebms_lit_review_report_form_submit($form, &$form_state) {
    if ($form_state['values']['op'] == 'Reset')
        drupal_goto('reports/lit-review');
    if ($form_state['values']['op'] == 'Submit') {
        $values = $form_state['values'];
        $request_id = EbmsReports::save_report_request($values,
                                                       'Literature Reviews');
        drupal_goto('reports/lit-review/' . $request_id);
    }
}

/**
 * Drupal form API callback which generates the form for the eight
 * reports on articles based on what the current status of the articles
 * is.  Each status has different filtering criteria available,
 * different logic for selecting the articles, and different columns
 * for the various kinds of information required by the report for
 * that particular status.  Routing to the correct report logic
 * is handled by the EbmsReports class method cites_by_status().
 *
 *  @param  array   $form         Structured array containing the
 *                                elements and properties of the form
 *  @param  array   &$form_state  Modifiable structured array
 *                                containing the current values
 *                                of the form, as well as other
 *                                form state information
 *  @param  int     $request_id   Primary key into the EBMS table
 *                                for report request parameters
 *
 *  @return array                 Array containing specification
 *                                of the fields to appear on the
 *                                form, as well as other markup
 */
function pdq_ebms_cites_by_status_form($form, &$form_state, $request_id) {

    // Override the breadcrumbs which Drupal would automatically
    // set based on the menu callback, in order to comply with
    // the customized requirements of the EBMS design.
    drupal_set_breadcrumb(
        array(
            l('Reports', 'reports'),
            l('Citation Management Reports', 'reports/cite'),
            'Citations By Status',
        )
    );

    // Initialize defaults for the form field's values.
    $board = $topic = $status = $meeting_type = $disposition = 0;
    $selected_cycles = array();
    $meeting_date = Ebms\Util::empty_date();

    // Check for overrides of the defaults by request parameters
    // stored in the database.
    if ($request_id) {
        $values = EbmsReports::fetch_report_request($request_id);
        $board = $values['board'];
        $topic = $values['topic'];
        $status = $values['status'];
        if (isset($values['disposition']))
            $disposition = $values['disposition'];
        $selected_cycles = $values['cycles'];
        if (isset($values['meeting-type']))
            $meeting_type = $values['meeting-type'];
        if (isset($values['meeting-date']))
            $meeting_date = $values['meeting-date'];
    }

    // Create the picklists for cycles, boards, meeting types,
    // and article states.
    $cycles = Ebms\Util::cycles();
    $boards = array(0 => ' ') + Ebms\Util::boards();
    $info = field_info_field('field_event_category');
    $meeting_types = array(0 => ' ') + $info['settings']['allowed_values'];
    $statuses = array(
        0 => '',
        'reviewer decision' => 'Reviewer Decision',
        'full text retrieved' => 'Full Text Retrieved',
        'committee decision' => 'Literature Surveillance Committee Decision',
        'assigned for review' => 'Assigned For Review',
        'board member responses' => 'Board Member Responses',
        'board manager action' => 'Board Manager Action',
        'on agenda' => 'On Agenda',
        'editorial board decision' => 'Editorial Board Decision',
    );

    // The "Disposition" picklist, if shown at all, has a composition
    // which is determined by the user's selection in the "Status"
    // field.  Assemble the possible Disposition picklists value sets.
    $yes_no = $yes_no_fyi = array(0 => ' ', 'yes' => 'Yes', 'no' => 'No');
    $yes_no_fyi['fyi'] = 'Flag for FYI';
    $manager_actions = array(0 => ' ') + EbmsReports::manager_actions();
    $review_dispositions = array(0 => ' ');
    $results = db_select('ebms_review_disposition_value', 'v')
        ->fields('v', array('value_id', 'value_name'))
        ->orderBy('v.value_pos')
        ->execute();
    foreach ($results as $result)
        $review_dispositions[$result->value_id] = $result->value_name;
    $board_decisions = array(0 => ' ');
    $results = db_select('ebms_article_board_decision_value', 'v')
        ->fields('v', array('value_id', 'value_name'))
        ->orderBy('v.value_name')
        ->execute();
    foreach ($results as $result)
        $board_decisions[$result->value_id] = $result->value_name;

    // Now plug those picklist arrays into the lookup dictionary
    // used by the ajax callback to dynamically change the picklist
    // available for the Disposition field when the users changes
    // the selection in the Status field.  For two of the eight
    // possible state ("Status") values there is no selection
    // for Disposition needed, and the Disposition field is suppressed.
    $all_dispositions = array(
        'reviewer decision' => $yes_no,
        'full text retrieved' => $yes_no,
        'committee decision' => $yes_no_fyi,
        'board member responses' => $review_dispositions,
        'board manager action' => $manager_actions,
        'editorial board decision' => $board_decisions,
    );

    // Get the values of interest for ajax callbacks.
    if (isset($form_state['values'])) {
        $board = $form_state['values']['board'];
        $status = $form_state['values']['status'];
        $topic = $form_state['values']['topic'];
        $disposition = $form_state['values']['disposition'];
    }

    // Use the board choice to fill the topic picklist with the topics
    // connected with the chosen board.
    $topics = array(0 => ' ');
    if ($board) {
        $results = db_select('ebms_topic', 't')
            ->fields('t', array('topic_id', 'topic_name'))
            ->condition('t.board_id', $board)
            ->orderBy('t.topic_name')
            ->execute();
        foreach ($results as $result)
            $topics[$result->topic_id] = $result->topic_name;

        // Make sure we unselect any leftover topic selection which
        // isn't valid with a change in the composition of the Summary
        // Topic picklist.
        if ($topic && !array_key_exists($topic, $topics))
            $topic = $form_state['values']['topic'] = 0;
    }

    // Just as we used the board value to populate the Summary
    // Topic picklist, we use the current Status value to
    // plug in the picklist for the Disposition field.
    $dispositions = array(0 => '');
    if (array_key_exists($status, $all_dispositions))
        $dispositions = $all_dispositions[$status];

    // Unselect any leftover Disposition selection which is no longer
    // valid after the Status select has changed.
    if ($disposition && !array_key_exists($disposition, $dispositions))
        $disposition = $form_state['values']['disposition'] = 0;

    // Begin the form structure, including the left nav penel.
    $left_nav = EbmsReports::build_left_nav('Citation Management',
                                            'Citations By Status');
    $form['top'] = array(
        '#type' => 'container',
        '#attributes' => array('id' => 'cites-by-status-report-top'),
        'left-nav' => $left_nav,
        'report-form' => array(
            '#type' => 'container',
            '#attributes' => array('id' => 'report-form'),
            'title' => array(
                '#type' => 'markup',
                '#markup' => '<h2>Citations By Status</h2>',
            ),
            'board' => array(
                '#type' => 'select',
                '#title' => 'Editorial Board *',
                '#title_display' => 'after',
                '#options' => $boards,
                '#default_value' => $board,
                '#attributes' => array('class' => array('fixed-width')),
                '#ajax' => array(
                    'callback' => 'pdq_ebms_report_ajax',
                    'wrapper' => 'board-dependent',
                ),
            ),
            'ajax fields' => array(
                '#type' => 'container',
                '#attributes' => array('id' => 'board-dependent'),
                'topic' => array(
                    '#type' => 'select',
                    '#title' => 'Summary Topic',
                    '#title_display' => 'after',
                    '#options' => $topics,
                    '#default_value' => $topic,
                    '#attributes' => array('class' => array('fixed-width')),
                ),
            ),
            'status' => array(
                '#type' => 'select',
                '#title' => 'Status *',
                '#title_display' => 'after',
                '#options' => $statuses,
                '#default_value' => $status,
                '#attributes' => array('class' => array('fixed-width')),
                '#ajax' => array(
                    'callback' => 'pdq_ebms_cit_status_report_ajax',
                    'wrapper' => 'status-dependent',
                ),
            ),

            'status-dep' => array(
                '#type' => 'container',
                '#attributes' => array('id' => 'status-dependent'),
            ),
        ),
    );

    // Six of the Status field selections call for display of the
    // Disposition picklist field.
    if (strcmp($status, '0') && strcmp($status, 'assigned for review') &&
        strcmp($status, 'on agenda'))
        $form['top']['report-form']['status-dep']['disposition'] = array(
            '#type' => 'select',
            '#title' => 'Disposition',
            '#title_display' => 'after',
            '#options' => $dispositions,
            '#default_value' => $disposition,
            '#attributes' => array(
                'class' => array('fixed-width')
            ),
        );

    // The Review Cycles field is always present, but it's sandwiched
    // between fields which depend on what has been selected for the
    // Status field, and the Drupal field states mechanism is broken,
    // so this field always gets redrawn by the ajax callback for
    // the change of Status.
    $form['top']['report-form']['status-dep']['cycles'] = array(
        '#type' => 'select',
        '#multiple' => true,
        '#size' => 6,
        '#title' => 'Review Cycle(s)',
        '#title_display' => 'after',
        '#options' => $cycles,
        '#default_value' => $selected_cycles,
        '#attributes' => array('class' => array('fixed-width')),
    );

    // The last two Status choices call for two additional fields,
    // related to board meetings.
    if (!strcmp($status, 'on agenda') ||
        !strcmp($status, 'editorial board decision')) {
        $form['top']['report-form']['status-dep']['meeting-type'] = array(
            '#type' => 'select',
            '#title' => 'Meeting Category',
            '#title_display' => 'after',
            '#options' => $meeting_types,
            '#default_value' => $meeting_type,
            '#attributes' => array('class' => array('fixed-width')),
        );
        $form['top']['report-form']['status-dep']['meeting-date'] = array(
            '#type' => 'date',
            '#title' => 'Meeting Date',
            '#title_display' => 'after',
            '#default_value' => $meeting_date,
            '#pre_render' => array('pdq_ebms_fix_date'),
            '#attributes' => array('class' => array('right-aligned-date')),
            '#element_validate' => array('pdq_ebms_date_validate'),
        );
    }

    // Add the buttons to the form to complete it.
    $form['top']['report-form']['buttons'] = array(
        '#type' => 'container',
        '#attributes' => array('id' => 'report-buttons'),
        'reset' => array(
            '#type' => 'submit',
            '#value' => 'Reset',
        ),
        'filter' => array(
            '#type' => 'submit',
            '#value' => 'Submit',
        ),
    );

    // If the user has given us report filter parameters, build and
    // show the report below the form.
    if ($request_id)
        $form['report'] = EbmsReports::cites_by_status($request_id);
    return $form;
}

/**
 * The Editorial Board and Status fields are required for this
 * form, everything else is optional, no matter which of the eight
 * reports is requested.
 *
 *  @param  array   $form         Structured array containing the
 *                                elements and properties of the form
 *  @param  array   &$form_state  Modifiable structured array
 *                                containing the current values
 *                                of the form, as well as other
 *                                form state information
 *
 *  @return     none
 */
function pdq_ebms_cites_by_status_form_validate($form, &$form_state) {
    $values = $form_state['values'];
    if ($values['op'] == 'Submit') {
        if (!$values['board'])
            form_set_error('board', 'No board selected.');
        if (!$values['status'])
            form_set_error('status', 'No status selected.');
    }
}

/**
 * If the user clicks the "Reset" button, clear the form by routing
 * back to the URL for the Citations by Status reports.  Otherwise store
 * the report request parameters in the database and send the user to
 * the URL which knows where those parameters were stored.
 *
 *  @param  array   $form         Structured array containing the
 *                                elements and properties of the form
 *  @param  array   &$form_state  Modifiable structured array
 *                                containing the current values
 *                                of the form, as well as other
 *                                form state information
 *
 *  @return     none
 */
function pdq_ebms_cites_by_status_form_submit($form, &$form_state) {
    if ($form_state['values']['op'] == 'Reset')
        drupal_goto('reports/cites-status');
    if ($form_state['values']['op'] == 'Submit') {
        $values = $form_state['values'];
        $request_id = EbmsReports::save_report_request($values,
                                                       'Citations by Status');
        drupal_goto('reports/cites-status/' . $request_id);
    }
}

/**
 * Modify the stock Drupal date field to conform to the requirements
 * of the EBMS design.  These requirements include a label at the
 * top of each picklist for year, month, or day indicating that
 * a value has not (yet) been selected.  Also, a more limited year
 * range is used than the one shown on the stock Drupal date field,
 * and the most recent years are at the top, since they are the
 * most frequently selected.
 *
 *  @param  array   $element    Structured array controlling the
 *                              values available for the components
 *                              of a date on a Drupal form
 *
 *  @return array               Modified copy reflecting EBMS
 *                              requirements
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

/**
 * We have a more lenient approach to date validation than is used
 * by the stock Drupal date field.  Our requirements ask us to accept
 * partially-specified dates, provide no less granular components are
 * omitted when more granular components are specified.  For example,
 * it is not permitted to specify a month and day, but fail to
 * specify the year, whereas it is OK to give the year without
 * specifying the month or day.
 */
function pdq_ebms_date_validate($element, &$form_state, $form) {
    $y = $element['#value']['year'];
    $m = $element['#value']['month'];
    $d = $element['#value']['day'];
    if ($d) {
        if (!$y || !$m)
            form_error($element, 'Day cannot be specified without full date');
        elseif (!checkdate($m, $d, $y))
            form_error($element, 'The specified date is invalid');
    }
    elseif ($m && !$y)
        form_error($element, 'Month cannot be specified without year');
}

/**
 * Validation function to ensure that a document was selected for a file
 * upload field (Drupal's validation of required file upload fields is
 * broken).  Make sure we don't make this check if the Cancel button
 * has been pressed (TIR #2237).
 */
function pdq_ebms_doc_file_required($element, &$form_state) {
    if ($form_state['values']['op'] == 'Submit') {
        if ($_FILES['files']['name']['file'] == '') {
            form_set_error('file', t('Please select a document'));
        }
    }
}

/**
 * Return the portion of a form dynamically controlled by values
 * selected elsewhere on the form.  Used in common by several of
 * the report forms.
 */
function pdq_ebms_report_ajax($form, $form_state) {
    return $form['top']['report-form']['ajax fields'];
}

/**
 * Return the portion of a form dynamically controlled by values
 * selected elsewhere on the form.  Used in common by several of
 * the report forms.
 */
function pdq_ebms_report_topic_ajax($form, $form_state) {
    return $form['top']['report-form']['ajax fields']['topic dependent'];
}

/**
 * Return the refreshed portion of the Citations by Status request
 * form when the value of the Status field has changed.
 */
function pdq_ebms_cit_status_report_ajax($form, $form_state) {
    return $form['top']['report-form']['status-dep'];
}

/**
 * Drupal form API callback which generates the form for the
 * report on packet responses by reviewer.
 *
 *  @param  array   $form         Structured array containing the
 *                                elements and properties of the form
 *  @param  array   &$form_state  Modifiable structured array
 *                                containing the current values
 *                                of the form, as well as other
 *                                form state information
 *  @param  int     $request_id   Primary key into the EBMS table
 *                                for report request parameters
 *
 *  @return array                 Array containing specification
 *                                of the fields to appear on the
 *                                form, as well as other markup
 */
function pdq_ebms_responses_by_reviewer($form, &$form_state, $request_id=0) {

    // Override the breadcrumbs which Drupal would automatically
    // set based on the menu callback, in order to comply with
    // the customized requirements of the EBMS design.
    drupal_set_breadcrumb(
        array(
            l('Reports', 'reports'),
            l('Citation Management Reports', 'reports/cite'),
            'Responses By Reviewer',
        )
    );

    // Initialize defaults for the form field's values.
    $board = 0;
    $selected_topics = $selected_reviewers = array();
    $start = $end = '';

    // Pull request parameters from the database if appropriate.
    if ($request_id) {
        $values = EbmsReports::fetch_report_request($request_id);
        $board = $values['board'];
        $selected_topics = $values['topics'];
        $selected_reviewers = $values['reviewers'];
        $start = $values['start-date'];
        $end = $values['end-date'];
    }

    // Get the values of interest for ajax callbacks.
    if (isset($form_state['values'])) {
        $board = $form_state['values']['board'];
        $selected_topics = $form_state['values']['topics'];
        $selected_reviewers = $form_state['values']['reviewers'];
    }

    // How tall should the picklists allowing multiple selections be?
    $size = 6;

    // Create the picklists.
    $topics = $reviewers = array();
    $boards = array(0 => ' ') + Ebms\Util::boards();

    // If a board has been selected, populate the board-dependent picklists.
    if ($board) {
        $results = db_select('ebms_topic', 't')
            ->fields('t', array('topic_id', 'topic_name'))
            ->condition('t.board_id', $board)
            ->orderBy('t.topic_name')
            ->execute();
        $topics = array();
        foreach ($results as $result)
            $topics[$result->topic_id] = $result->topic_name;
        $query = db_select('users', 'u')
            ->fields('u', array('uid', 'name'));
        $query->join('ebms_article_review', 'r', 'r.reviewer_id = u.uid');
        $query->join('ebms_packet', 'p', 'p.packet_id = r.packet_id');
        $query->join('ebms_topic', 't', 't.topic_id = p.topic_id');
        $query->condition('t.board_id', $board);
        $query->orderBy('u.name');
        $query->distinct();
        $results = $query->execute();
        $reviewers = array();
        foreach ($results as $result)
            $reviewers[$result->uid] = htmlspecialchars($result->name);
        foreach ($selected_topics as $key => $value)
            if (!array_key_exists($value, $topics))
                unset($selected_topics[$key]);
        foreach ($selected_reviewers as $key => $value)
            if (!array_key_exists($value, $reviewers))
                unset($selected_reviewers[$key]);
     }

    // Assemble the request form.
    $form = array(
        '#type' => 'container',
        '#attributes' => array('id' => 'report-form'),
        'title' => array('#markup' => '<h2>Responses By Reviewer</h2>'),
        'board' => array(
            '#type' => 'select',
            '#options' => $boards,
            '#required' => true,
            '#default_value' => $board,
            '#title' => 'Editorial Board',
            '#attributes' => array('class' => array('fixed-width')),
            '#ajax' => array(
                'callback' => 'pdq_ebms_report_ajax',
                'wrapper' => 'board-dependent',
            ),
        ),
        'ajax fields' => array(
            '#type' => 'container',
            '#attributes' => array('id' => 'board-dependent'),
            'topics' => array(
                '#type' => 'select',
                '#multiple' => true,
                '#options' => $topics,
                '#default_value' => $selected_topics,
                '#title' => 'Topic(s)',
                '#size' => $size,
                '#attributes' => array('class' => array('fixed-width')),
            ),
            'reviewers' => array(
                '#type' => 'select',
                '#multiple' => true,
                '#options' => $reviewers,
                '#default_value' => $selected_reviewers,
                '#title' => 'Reviewer(s)',
                '#size' => $size,
                '#attributes' => array('class' => array('fixed-width')),
            ),
        ),
        'start-date' => array(
            '#type' => 'date_popup',
            '#title' => 'Start Date',
            '#date_format' => 'Y-m-d',
            '#default_value' => $start,
            '#date_label_position' => 'within',
            '#attributes' => array(
                'title' =>
                 'Assigned date (defaults to no start date)',
            ),
        ),
        'end-date' => array(
            '#type' => 'date_popup',
            '#title' => 'End Date',
            '#date_format' => 'Y-m-d',
            '#default_value' => $end,
            '#date_label_position' => 'within',
            '#attributes' => array(
                'title' =>
                 'Assigned date (defaults to no end date)',
            ),
        ),
        'reset' => array(
            '#type' => 'submit',
            '#value' => 'Reset',
        ),
        'submit' => array(
            '#type' => 'submit',
            '#value' => 'Submit',
        ),
    );

    // Assemble the page where the form lives, including the left nav penel.
    $left_nav = EbmsReports::build_left_nav('Citation Management',
                                            'Responses By Reviewer');
    $page['top'] = array(
        '#type' => 'container',
        '#attributes' => array('id' => 'cites-by-status-report-top'),
        'left-nav' => $left_nav,
        'report-form' => $form,
    );

    // If we have a submitted report request, add the report under the form.
    if ($request_id) {
        $query = db_select('ebms_packet', 'p')
            ->fields('p', array('packet_id', 'packet_title', 'created_at'));
        $query = $query->extend('TableSort');
        $query->leftJoin('ebms_packet_reviewer', 'r',
            'r.packet_id = p.packet_id');
        $query->leftJoin('users', 'u', 'u.uid = r.reviewer_id');
        $query->fields('u', array('uid', 'name'));
        if (count($values['topics']))
            $query->condition('p.topic_id', $values['topics'], 'IN');
        else {
            $query->join('ebms_topic', 't', 't.topic_id = p.topic_id');
            $query->condition('t.board_id', $board);
        }
        if (count($values['reviewers']))
            $query->condition('r.reviewer_id', $values['reviewers'], 'IN');
        if ($values['start-date'])
            $query->condition('p.created_at', $values['start-date'], '>=');
        if ($values['end-date']) {
            $end = $values['end-date'] . ' 23:59:59';
            $query->condition('p.created_at', $end, '<=');
        }
        //pdq_ebms_debug('QUERY VALUES', $values);
        $headers = array(
            array(
                'data' => 'Packet Name',
                'field' => 'p.packet_title',
            ),
            array(
                'data' => 'Created',
                'field' => 'p.created_at',
                'sort' => 'desc',
            ),
            array(
                'data' => 'Reviewer',
                'field' => 'u.name',
            ),
            'Assigned',
            'Completed',
            'Not Completed',
        );
        $query->orderByHeader($headers);
        $rows = array();
        $results = $query->execute();
        $total_assigned = $total_completed = $total_not_completed = 0;
        foreach ($results as $result) {
            $query = db_select('ebms_packet_article', 'a')
                ->condition('a.packet_id', $result->packet_id)
                ->fields('a', array('drop_flag'));
            $query->leftJoin('ebms_article_review', 'r',
                'r.packet_id = a.packet_id AND ' .
                'r.article_id = a.article_id AND ' .
                'r.reviewer_id = ' . $result->uid);
            $query->fields('r', array('review_id'));
            $articles = $query->execute();
            $assigned = $completed = $not_completed = 0;
            foreach ($articles as $article) {
                ++$assigned;
                if ($article->review_id)
                    ++$completed;
                elseif (!$article->drop_flag)
                    ++$not_completed;
            }
            $row = array();
            $row[] = htmlspecialchars($result->packet_title);
            $row[] = substr($result->created_at, 0, 10);
            $row[] = htmlspecialchars($result->name);
            $row[] = $assigned;
            $row[] = $completed;
            $row[] = $not_completed;
            $rows[] = $row;
            $total_assigned += $assigned;
            $total_completed += $completed;
            $total_not_completed += $not_completed;
        }
        $rows[] = array(
            '<b>TOTALS</b>',
            '',
            '',
            "<b>$total_assigned</b>",
            "<b>$total_completed</b>",
            "<b>$total_not_completed</b>",
        );

        // Assemble the table.
        $id = 'report-title';
        $title = 'Responses';
        $page['report'] = array(
            'title' => array(
                '#markup' => "<h2 id='$id'>$title</h2>",
            ),
            'table' => array(
                '#attributes' => array('id' => 'responses-by-reviewer-report'),
                '#theme' => 'table',
                '#rows' => $rows,
                '#header' => $headers,
            ),
        );
//         $page['report'] = array(
//             '#attributes' => array('id' => 'responses-by-reviewer-report'),
//             '#theme' => 'table',
//             '#rows' => $rows,
//             '#header' => $headers,
//         );
    }
    return $page;
}

/**
 * Store the report request parameters in the database and send the user
 * to the URL which knows where those parameters were stored.
 *
 *  @param  array   $form         Structured array containing the
 *                                elements and properties of the form
 *  @param  array   &$form_state  Modifiable structured array
 *                                containing the current values
 *                                of the form, as well as other
 *                                form state information
 *
 *  @return     none
 */
function pdq_ebms_responses_by_reviewer_submit($form, &$form_state) {
    if ($form_state['values']['op'] == 'Reset')
        drupal_goto('reports/responses-by-reviewer');
    $values = $form_state['values'];
    $report = 'Responses by Reviewer';
    $request_id = EbmsReports::save_report_request($values, $report);
    drupal_goto('reports/responses-by-reviewer/' . $request_id);
}

/**
 * Drupal form API callback which generates the form for the
 * report on articles which have not yet been reviewed by
 * any of the board members assigned to it for a given topic.
 *
 *  @param  array   $form         Structured array containing the
 *                                elements and properties of the form
 *  @param  array   &$form_state  Modifiable structured array
 *                                containing the current values
 *                                of the form, as well as other
 *                                form state information
 *  @param  int     $request_id   Primary key into the EBMS table
 *                                for report request parameters
 *
 *  @return array                 Array containing specification
 *                                of the fields to appear on the
 *                                form, as well as other markup
 */
function pdq_ebms_articles_without_responses($form, &$form_state,
                                             $request_id=0) {

    // Override the breadcrumbs which Drupal would automatically
    // set based on the menu callback, in order to comply with
    // the customized requirements of the EBMS design.
    drupal_set_breadcrumb(
        array(
            l('Reports', 'reports'),
            l('Citation Management Reports', 'reports/cite'),
            'Articles Without Responses',
        )
    );

    // Initialize defaults for the form field's values.
    $board = 0;
    $selected_topics = $selected_cycles = array();
    $order_by = 'author';
    $start = $end = '';

    // Pull request parameters from the database if appropriate.
    if ($request_id) {
        $values = EbmsReports::fetch_report_request($request_id);
        $board = $values['board'];
        $selected_topics = $values['topics'];
        $selected_cycles = $values['cycles'];
        $start = $values['start-date'];
        $end = $values['end-date'];
        $order_by = $values['order-by'];
    }

    // Get the values of interest for ajax callbacks.
    if (isset($form_state['values'])) {
        $board = $form_state['values']['board'];
        $selected_topics = $form_state['values']['topics'];
        $selected_cycles = $form_state['values']['cycles'];
    }

    // How tall should the picklists allowing multiple selections be?
    $size = 6;

    // Create the picklists.
    $topics = array();
    $boards = array(0 => ' ') + Ebms\Util::boards();
    $cycles = Ebms\Util::cycles();

    // If a board has been selected, populate the board-dependent picklists.
    if ($board) {
        $results = db_select('ebms_topic', 't')
            ->fields('t', array('topic_id', 'topic_name'))
            ->condition('t.board_id', $board)
            ->orderBy('t.topic_name')
            ->execute();
        foreach ($results as $result)
            $topics[$result->topic_id] = $result->topic_name;
        foreach ($selected_topics as $key => $value)
            if (!array_key_exists($value, $topics))
                unset($selected_topics[$key]);
     }

    // Assemble the request form.
    $form = array(
        '#type' => 'container',
        '#attributes' => array('id' => 'report-form'),
        'title' => array('#markup' => '<h2>Articles Without Responses</h2>'),
        'board' => array(
            '#type' => 'select',
            '#options' => $boards,
            '#required' => true,
            '#default_value' => $board,
            '#title' => 'Editorial Board',
            '#attributes' => array('class' => array('fixed-width')),
            '#ajax' => array(
                'callback' => 'pdq_ebms_report_ajax',
                'wrapper' => 'board-dependent',
            ),
        ),
        'ajax fields' => array(
            '#type' => 'container',
            '#attributes' => array('id' => 'board-dependent'),
            'topics' => array(
                '#type' => 'select',
                '#multiple' => true,
                '#options' => $topics,
                '#default_value' => $selected_topics,
                '#title' => 'Topic(s)',
                '#size' => $size,
                '#attributes' => array('class' => array('fixed-width')),
            ),
        ),
        'cycles' => array(
            '#type' => 'select',
            '#multiple' => true,
            '#options' => $cycles,
            '#default_value' => $selected_cycles,
            '#title' => 'Cycle(s)',
            '#size' => $size,
            '#attributes' => array('class' => array('fixed-width')),
        ),
        'start-date' => array(
            '#type' => 'date_popup',
            '#title' => 'Start Date',
            '#date_format' => 'Y-m-d',
            '#default_value' => $start,
            '#date_label_position' => 'within',
            '#attributes' => array(
                'title' =>
                 'Assigned date (defaults to no start date)',
            ),
        ),
        'end-date' => array(
            '#type' => 'date_popup',
            '#title' => 'End Date',
            '#date_format' => 'Y-m-d',
            '#default_value' => $end,
            '#date_label_position' => 'within',
            '#attributes' => array(
                'title' =>
                 'Assigned date (defaults to no end date)',
            ),
        ),
        'order-by' => array(
            '#type' => 'radios',
            '#title' => 'Order By',
            '#options' => array(
                'author' => 'First Article Author',
                'topic' => 'Topic',
            ),
            '#default_value' => $order_by,
        ),
        'reset' => array(
            '#type' => 'submit',
            '#value' => 'Reset',
        ),
        'submit' => array(
            '#type' => 'submit',
            '#value' => 'Submit',
        ),
    );

    // Assemble the page where the form lives, including the left nav penel.
    $left_nav = EbmsReports::build_left_nav('Citation Management',
                                            'Articles Without Responses');
    $page['top'] = array(
        '#type' => 'container',
        '#attributes' => array('id' => 'cites-by-status-report-top'),
        'left-nav' => $left_nav,
        'report-form' => $form,
    );

    // If we have a submitted report request, add the report under the form.
    if ($request_id) {
        $query = db_select('ebms_article', 'a');
        //$query = $query->extend('TableSort');
        $query->join('ebms_packet_article', 'pa',
            'pa.article_id = a.article_id');
        $query->join('ebms_packet', 'p', 'p.packet_id = pa.packet_id');
        $query->join('ebms_topic', 't', 't.topic_id = p.topic_id');
        if (count($values['topics']))
            $query->condition('p.topic_id', $values['topics'], 'IN');
        else
            $query->condition('t.board_id', $board);
        $query->leftJoin('ebms_article_review', 'r',
            'r.article_id = a.article_id AND p.packet_id = r.packet_id');
        $query->isNull('r.review_id');
        $query->fields('a', array('article_id'));
        $query->fields('p', array('packet_id', 'created_at'));
        $query->fields('t', array('topic_id', 'topic_name'));
        if (count($values['cycles'])) {
            $query->join('ebms_article_topic', 'at',
                'at.article_id = a.article_id AND at.topic_id = p.topic_id');
            $query->condition('at.cycle_id', $values['cycles'], 'IN');
        }
        if ($values['start-date'])
            $query->condition('p.created_at', $values['start-date'], '>=');
        if ($values['end-date']) {
            $end = $values['end-date'] . ' 23:59:59';
            $query->condition('p.created_at', $end, '<=');
        }
        $query->leftJoin('ebms_article_author_cite', 'ac',
                         'a.article_id = ac.article_id AND ac.cite_order = 1');
        $query->leftJoin('ebms_article_author', 'aa',
                         'aa.author_id = ac.author_id');
        $query->addExpression('IFNULL(aa.last_name, aa.collective_name)',
                              'author_sort');
        if ($order_by == 'topic')
            $query->orderBy('t.topic_name');
        $query->orderBy('aa.last_name');
        $query->orderBy('aa.initials');
        $query->orderBy('aa.collective_name');
        if ($order_by != 'topic')
            $query->orderBy('t.topic_name');
        //$query->orderBy('sort_auth.last_name');
        //$query->orderBy('sort_auth.initials');
        //$query->orderBy('sort_auth.collective_name');

        //pdq_ebms_debug('QUERY VALUES', $values);
        $headers = array(
            array(
                'data' => 'Article',
                //'field' => 'author_sort', //'a.article_title',
                //'sort' => 'asc',
            ),
            array(
                'data' => 'Topic',
                //'field' => 't.topic_name',
                //'sort' => 'asc',
            ),
            'Reviewer',
            'Assigned',
        );
        //$query->orderByHeader($headers);
        $rows = array();
        $results = $query->execute();

        // Find the sequence number for the highest state for which
        // we want to include an article/topic combination on the report.
        $sequence = db_select('ebms_article_state_type', 't')
            ->fields('t', array('sequence'))
            ->condition('t.state_text_id', 'PassedFullReview')
            ->execute()
            ->fetchField();

        // Assemble the rows in the report.
        foreach ($results as $result) {

            // Winnow out rows for article/state combinations which have
            // moved on past the review state.
            $query = db_select('ebms_article_state', 's')
                ->condition('s.article_id', $result->article_id)
                ->condition('s.topic_id', $result->topic_id)
                ->condition('s.active_status', 'A');
            $query->join('ebms_article_state_type', 't',
                't.state_id = s.state_id');
            $query->condition('t.sequence', $sequence, '>');
            $count = $query->countQuery()->execute()->fetchField();
            if ($count > 0)
                continue;

            // Collect the list of reviewers assigned to the article.
            $query = db_select('users', 'u')
                ->fields('u', array('name'));
            $query->join('ebms_packet_reviewer', 'r', 'r.reviewer_id = u.uid');
            $query->condition('r.packet_id', $result->packet_id);
            $query->orderBy('u.name');
            $reviewer_results = $query->execute();
            $reviewers = array();
            foreach ($reviewer_results as $reviewer_result)
                $reviewers[] = htmlspecialchars($reviewer_result->name);

            // Assemble the report row for this article/topic combo.
            $row = array();
            $row[] = EbmsReports::format_citation($result->article_id);
            $row[] = htmlspecialchars($result->topic_name);
            $row[] = implode('<br><br>', $reviewers);
            $row[] = date('Y-m-d', strtotime($result->created_at));
            $rows[] = $row;
        }

        // Assemble the table.
        $id = 'report-title';
        $title = 'Articles';
        $page['report'] = array(
            'title' => array(
                '#markup' => "<h2 id='$id'>$title</h2>",
            ),
            'table' => array(
                '#attributes' => array('id' => 'unreviewed-articles-report'),
                '#theme' => 'table',
                '#rows' => $rows,
                '#header' => $headers,
            ),
        );
    }
    return $page;
}

/**
 * Store the report request parameters in the database and send the user
 * to the URL which knows where those parameters were stored.
 *
 *  @param  array   $form         Structured array containing the
 *                                elements and properties of the form
 *  @param  array   &$form_state  Modifiable structured array
 *                                containing the current values
 *                                of the form, as well as other
 *                                form state information
 *
 *  @return     none
 */
function pdq_ebms_articles_without_responses_submit($form, &$form_state) {
    if ($form_state['values']['op'] == 'Reset')
        drupal_goto('reports/articles-without-responses');
    $values = $form_state['values'];
    $report = 'Articles Without Responses';
    $request_id = EbmsReports::save_report_request($values, $report);
    drupal_goto('reports/articles-without-responses/' . $request_id);
}

/*
 * Generates an Excel report of the last time each active board
 * member logged in.  JIRA::OCEEBMS-133.
 */
function pdq_ebms_board_members_last_login() {
    $xllib = libraries_get_path('PHPExcel');
    require_once $xllib . '/Classes/PHPExcel.php';
    require_once $xllib . '/Classes/PHPExcel/IOFactory.php';
    $excel = new PHPExcel();
    $excel->setActiveSheetIndex(0);
    $sheet = $excel->getActiveSheet();
    $sheet->setTitle('Board Members');
    $sheet->getColumnDimension('A')->setAutoSize(true);
    $sheet->getColumnDimension('B')->setAutoSize(true);
    $sheet->getColumnDimension('C')->setAutoSize(true);
    $sheet->setCellValue('A1', 'Name');
    $sheet->setCellValue('B1', 'Board');
    $sheet->setCellValue('C1', 'Last Login');
    //$sheet->getStyle('A1:C1')->getFont()->setBold(true);
    $sheet->getStyle('A1:C1')->applyFromArray(
        array(
            'fill' => array(
                'type' => PHPExcel_Style_Fill::FILL_SOLID,
                'color' => array('rgb' => '0101DF'),
            ),
            'font' => array(
                'bold' => true,
                'color' => array(
                    'argb' => PHPExcel_Style_Color::COLOR_WHITE,
                ),
                //'size' => 15,
                //'name' => 'Verdana',
            ),
            'alignment' => array(
                'horizontal' => PHPExcel_Style_Alignment::HORIZONTAL_CENTER,
            ),
        )
    );
    $row = 2;
    $query = db_select('users', 'u')
        ->fields('u', array('name', 'login'))
        ->condition('u.status', 1);
    $query->join('users_roles', 'ur', 'ur.uid = u.uid');
    $query->join('role', 'r', 'r.rid = ur.rid');
    $query->condition('r.name', 'board member');
    $query->addExpression('FROM_UNIXTIME(u.login, \'%Y-%m-%d\')', 'last');
    $query->leftJoin('ebms_board_member', 'm', 'm.user_id = u.uid');
    $query->leftJoin('ebms_board', 'b', 'b.board_id = m.board_id');
    $query->fields('b', array('board_name'));
    $query->orderBy('u.name');
    $query->orderBy('b.board_name');
    $results = $query->execute();
    $name = 'board-members-last-login-' . strftime('%Y%m%d%H%M%S') . '.xls';
    foreach ($results as $result) {
        $last_login = $result->login ? $result->last : '';
        $board = $result->board_name ? $result->board_name : '';
        $sheet->setCellValue("A$row", $result->name);
        $sheet->setCellValue("B$row", $result->board_name);
        $sheet->setCellValue("C$row", $last_login);
        ++$row;
    }
    $sheet->setSelectedCell('A2');
    $writer = PHPExcel_IOFactory::createWriter($excel, 'Excel5');
    drupal_add_http_header('Content-Type', 'application/vnd.ms-excel; charset=utf-8');
    drupal_add_http_header('Cache-control', 'must-revalidate, post-check=0, pre-check=0');
    drupal_add_http_header('Content-disposition', 'attachment;filename=' . $name);
    drupal_add_http_header('Content-transfer-encoding', 'binary');
    $writer->save('php://output');
    exit(0);
    return '<p>DONE!!</p>';


    echo pack('ss', 0x0A, 0x0);
    exit(0);
}

/*
 * Fallback, in case we can't get CBIIT to install PHPExcel in the upper
 * tiers.  This version creates a spreadsheet using the Excel 2 format,
 * which is so ancient that current versions of Excel won't let you edit
 * or save the file (you have to transfer the cell data to a new workbook
 * using the clipboard).
 */
function xpdq_ebms_board_members_last_login() {
    $name = 'board-members-last-login-' . strftime('%Y%m%d%H%M%S') . '.xls';
    //drupal_add_http_header('Content-type', 'application/force-download');
    //drupal_add_http_header('Content-type', 'application/octet-stream', true);
    //drupal_add_http_header('Content-type', 'application/download', true);
    drupal_add_http_header('Content-Type', 'application/vnd.ms-excel; charset=utf-8');
    drupal_add_http_header('Cache-control', 'must-revalidate, post-check=0, pre-check=0');
    drupal_add_http_header('Content-disposition', 'attachment;filename=' . $name);
    drupal_add_http_header('Content-transfer-encoding', 'binary');

    echo pack('ssssss', 0x809, 0x8, 0x0, 0x10, 0x0, 0x0);
    echo pdq_ebms_xls_write_string(0, 0, 'Name');
    echo pdq_ebms_xls_write_string(0, 1, 'Board');
    echo pdq_ebms_xls_write_string(0, 2, 'Last Login');
    $row = 1;
    $query = db_select('users', 'u')
        ->fields('u', array('name', 'login'))
        ->condition('u.status', 1);
    $query->join('users_roles', 'ur', 'ur.uid = u.uid');
    $query->join('role', 'r', 'r.rid = ur.rid');
    $query->condition('r.name', 'board member');
    $query->addExpression('FROM_UNIXTIME(u.login, \'%Y-%m-%d\')', 'last');
    $query->leftJoin('ebms_board_member', 'm', 'm.user_id = u.uid');
    $query->leftJoin('ebms_board', 'b', 'b.board_id = m.board_id');
    $query->fields('b', array('board_name'));
    $query->orderBy('u.name');
    $query->orderBy('b.board_name');
    $results = $query->execute();
    foreach ($results as $result) {
        $last_login = $result->login ? $result->last : '';
        $board = $result->board_name ? $result->board_name : '';
        echo pdq_ebms_xls_write_string($row, 0, $result->name);
        echo pdq_ebms_xls_write_string($row, 1, $result->board_name);
        echo pdq_ebms_xls_write_string($row, 2, $last_login);
        ++$row;
    }
    echo pack('ss', 0x0A, 0x0);
    exit(0);
}

/*
 * Helper function for fallback approach above.
 */
function pdq_ebms_xls_write_string($row, $col, $string) {
    $len = strlen($string);
    $s = pack('ssssss', 0x204, 8 + $len, $row, $col, 0x0, $len);
    return $s . $string;
}
