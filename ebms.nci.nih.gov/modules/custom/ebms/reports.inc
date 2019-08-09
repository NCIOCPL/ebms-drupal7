<?php

/**
 * @file
 *
 * Implementation of EBMS page for running system reports.
 */
function pdq_ebms_reports($report=null, $request_id=0) {
    //register_shutdown_function('pdq_ebms_shutdown');
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

    /**
     * Version of the "articles without responses" report tailored for
     * board members (OCEEBMS-207).
     */
    private function awr_member() {
        $report = articles_without_responses_report($this->request_id, true);
        $title = "Articles Without Responses ($report->total)";
        $table = array(
            'title' => array('#markup' => "<h1>$title</h1>"),
            'table' => array(
                '#prefix' => '<div id="awr">',
                '#suffix' => '</div>',
                '#theme' => 'table',
                '#rows' => $report->rows,
                '#header' => $report->headers,
            ),
        );
        $css = Ebms\CSS_DIR . '/stripped-report.css';
        $page = array(
            'language' => 'en',
            'head' => '<meta charset="utf-8">',
            'title' => 'Articles Without Responses',
            'scripts' => '',
            'favicon' => '',
            'css' => "<link rel='stylesheet' href='/$css'>",
            'message' => array(),
            'footer_scripts' => '',
            'body' => drupal_render($table),
        );
        $html = theme('stripped', array('page' => $page));
        drupal_add_http_header('Content-Type', 'text/html; charset=utf-8');
        drupal_send_headers();
        print $html;
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
            case 'recent-activity':
                return drupal_get_form('pdq_ebms_recent_activity_form',
                                       $this->request_id);
            case 'awr-member':
                return $this->awr_member();
            case 'board-members-last-login':
                return pdq_ebms_board_members_last_login();
            case 'meeting-dates':
            case 'no-responses':
            //case 'citation-followup': OCEEBMS-414
            case 'boards-and-topics':
                return $this->not_yet_implemented();
            case 'missing-from-nlm':
                return drupal_get_form('pdq_ebms_missing_from_nlm',
                                       $this->request_id);
            case 'responses-by-reviewer':
                return drupal_get_form('pdq_ebms_responses_by_reviewer',
                                       $this->request_id);
            case 'articles-without-responses':
                return drupal_get_form('pdq_ebms_articles_without_responses',
                                       $this->request_id);
            case 'membership':
                return drupal_get_form('pdq_ebms_membership_report_form',
                                       $this->request_id);
            /* removed by OCEEBMS-412 case 'login-attempts':
                return drupal_get_form('pdq_ebms_login_attempts_form',
                                       $this->request_id);
            */
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
                // archive code here obsoleted by OCEEBMS-292?
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
            /* removed by OCEEBMS-473
            'not-list' => array('Citation Management', 'Not List'),
            'yes-list' => array('Citation Management', 'Yes List'),
            'no-responses' => array('Citation Management', 'No Responses'),
            */
            /* removed by OCEEBMS-414
            'citation-followup' => array(
                'Citation Management',
                'Citation Follow-up'
            ), */
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
     *
     * OCEEBMS-417: unfortunately, this is what happens when we get
     * requirements piecemeal. The users have asked that we implement
     * different formatting for the Articles Without Responses report.
     */
    static function format_citation($article_id, $counter=null,
        $member_version=false, $articles_without_responses_report=false,
        $topic_id=null
    ) {
        $xml = db_select('ebms_article', 'a')
            ->fields('a', array('source_data'))
            ->condition('a.article_id', $article_id)
            ->execute()
            ->fetchField();
        $doc = new \SimpleXMLElement($xml);
        $article = $doc->MedlineCitation->Article;
        $title = Ebms\Util::get_text($article->ArticleTitle);
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
        if ($member_version || $articles_without_responses_report) {
            $uri = Ebms\Util::get_ft_uri($article_id);
            if ($uri)
                $title = l($title, file_create_url($uri));
        }
        else {
            $title_opts = array(
                'attributes' => array(
                    'class' => array('article-title'),
                )
            );
            $full_url = 'citations/full/' . $article_id;
            $title = l($title, $full_url, $title_opts);
        }
        $pieces = array('<span class="article-citation">');
        if ($counter)
            $pieces[] = '<span class="article-counter">' . $counter .
                '</span>';
        $pieces[] = implode(', ', $authors) . '<br />';
        $pieces[] = "$title<br />$ta $vol";
        if ($issue)
            $pieces[] = "($issue)";
        if ($pgn)
            $pieces[] = ": $pgn";
        if ($year)
            $pieces[] = ", $year";
        $pmurl = Ebms\Util::pubmed_url($pmid);
        $pmopts = array('attributes' => array('target' => '_blank'));
        $pieces[] = '.<br />PMID: ' . l($pmid, $pmurl, $pmopts);
        if (!$member_version && $articles_without_responses_report) {
            $link = l($article_id, "citations/full/$article_id");
            $pieces[] = " &nbsp; EBMS ID: $link";
        }
        if (!empty($topic_id)) {
            $tags = Ebms\Util::get_topic_tags($article_id, $topic_id);
            if (in_array('High priority', $tags)) {
                $pieces[] = '<br><br><span class="hi-priority">' .
                            'HIGH PRIORITY</span>';
            }
        }
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
                        array('Import Report', 'reports/import'),
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
                        /* removed by OCEEBMS-412
                        array(
                            'Board Member Login Attempts',
                            'reports/login-attempts'
                        ),
                        */
                        array(
                            'Board Member Activity Report',
                            'reports/board-members-last-login',
                        ),
                        array(
                            'Recent Activity Report',
                            'reports/recent-activity',
                        ),
                    ),
                ),
                array(
                    'Citation Management',
                    'reports/cite',
                    array(
                        array('Import Report', 'reports/import'),
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
                        /* removed by OCEEBMS-414
                        array(
                            'Citation Follow-up',
                            'reports/citation-followup',
                        ), */
                        array(
                            'Board and Topic Lists',
                            'reports/boards-and-topics',
                        ),
                        array('Invalid Pubmed IDs', 'reports/missing-from-nlm'),
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
        $state_ids = array(
            Ebms\Util::state_id_from_text_id('ReadyInitReview'),
            Ebms\Util::state_id_from_text_id('RejectJournalTitle'),
        );
        $query = db_select('ebms_article_state', 's');
        $query->join('ebms_board', 'b', 'b.board_id = s.board_id');
        $query->join('ebms_article_topic', 'a',
            'a.article_id = s.article_id AND a.topic_id = s.topic_id');
        $query->condition('a.cycle_id', $cycle);
        $query->condition('s.state_id', $state_ids, 'IN');
        $query->fields('b', array('board_name'));
        $query->groupBy('b.board_name');
        if ($board)
            $query->condition('s.board_id', $board);

        // Branch off at this point for a separate version of the query
        // to get the total number of articles for each board (which we
        // can't get by summing the per-board/topic counts, because an
        // article can be represented in those counts more than once).
        $total_query = clone($query);
        $total_query->addExpression('COUNT(DISTINCT s.article_id)', 'n');
        $results = $total_query->execute();
        $totals = array();
        foreach ($results as $result)
            $totals[$result->board_name] = $result->n;

        // Resume with the rest of the original query, from which we
        // need more granularity.
        $query->join('ebms_topic', 't', 't.topic_id = s.topic_id');
        $query->fields('t', array('topic_name'));
        $query->groupBy('t.topic_name');
        $query->addExpression('COUNT(DISTINCT s.article_id)', 'n');
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

        // Primary keys for the states we're looking for.
        $rejected = Ebms\Util::state_id_from_text_id('RejectInitReview');
        $accepted = Ebms\Util::state_id_from_text_id('PassedInitReview');

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

        // New requirement (OCEEBMS-204): If no board is selected,
        // add a total count of unique rejected articles at the top
        // of the report.
        if (!$board) {
            $on = 't.article_id = s.article_id AND t.topic_id = s.topic_id';
            $query = db_select('ebms_article_state', 's');
            $query->join('ebms_article_topic', 't', $on);
            $query->condition('s.state_id', $rejected);
            $query->condition('t.cycle_id', $cycle);
            $query->addExpression('COUNT(DISTINCT s.article_id)', 'total');
            $total_rejected = $query->execute()->fetchField();
            $label = '<span class="label">Total Articles Rejected:</span>';
            $markup = "<div class='meta'>$label $total_rejected</div>";
            $report['grant total rejected'] = array('#markup' => $markup);
        }


        // Construct the report's query.
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
        $pubmed_base = 'https://www.ncbi.nlm.nih.gov/entrez/query.fcgi?' .
            'cmd=Retrieve&db=PubMed&dopt=abstract&list_uids=';
        $new_tab = array('attributes' => array('target' => '_blank'));

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
                $col1 = l($article_id, 'citations/full/' . $article_id,
                          $new_tab);
                $col2 = l($pmid, $pubmed_base . $pmid, $new_tab);
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
        $pubmed_base = 'https://www.ncbi.nlm.nih.gov/entrez/query.fcgi?' .
            'cmd=Retrieve&db=PubMed&dopt=abstract&list_uids=';
        $new_tab = array('attributes' => array('target' => '_blank'));

        // Walk through the query results, assembling the rows for the
        // report table.
        $results = $query->execute();
        $rows = array();
        foreach ($results as $result) {
            $article_id = $result->article_id;
            $pmid = $result->source_id;
            $col1 = l($article_id, 'citations/full/' . $article_id, $new_tab);
            $col2 = l($pmid, $pubmed_base . $pmid, $new_tab);
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
     * This display is the interim display for the "Import Report"
     * page.
     *
     *  @param  int    $request_id  ID of request stored in DB
     *
     *  @return array                Drupal render array for the list
     *                               of import batch jobs
     */
    public static function import_reports($request_id) {

        // Fetch the report request's parameters.
        $params = EbmsReports::fetch_report_request($request_id);
        $board = $params['board'];
        $topic = $params['topic'];
        $cycle = $params['cycle'];
        $start = $params['import-start']['date'];
        $end = $params['import-end']['date'];

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
        elseif ($start || $end) {
            if (!Ebms\Util::add_date_range_to_query($query, $start, $end,
                    'j.import_date')) {
                drupal_set_message('Failure restricting query by range',
                    'error');
                return array();
            }
        }
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
            $opts = array('query' => array('job' => $result->import_batch_id));
            $items[] = l($line, "reports/import/$request_id", $opts);
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
                '#markup' => '<h2>Import Report (' . count($items) . ')</h2>',
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
        return Ebms\Util::import_report($batch, $title, 7);
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
     * OCEEBMS-292: the table now has to be built up by hand as
     * raw markup, because we need to intersperse more form fields.
     *
     *  @param  int  $request_id   Primary key into the EBMS table
     *                             holding the parameters for the
     *                             report request.
     *
     *  @return array              Drupal render array for the
     *                             report.
     */
    public static function docs($request_id) {

        // This is the render array we'll return to be inserted into
        // the report form, using the Drupal Form API.
        $table = array();

        // Collect the report parameters.
        $board = $tag = $inhouse = $member = $archived =
            $range = $uploader_board = null;
        $start = $end = Ebms\Util::empty_date();
        $topics = array();
        $sort = 'f.filename';
        if ($request_id) {
            $params = EbmsReports::fetch_report_request($request_id);
            $board = $params['board'];
            foreach ($params['topic'] as $id => $val)
                if ($val)
                    $topics[] = $id;
            $tag = $params['tag'];
            $inhouse = $params['inhouse'];
            $member = $params['member'];
            $uploader_board = $params['uploader-board'];
            $archived = $params['archived'];
            $start = $params['upload-start']['date'];
            $end = $params['upload-end']['date'];
            $sort = $params['sort'];
            $range = $start || $end;
        }
        if (!$sort)
            $sort = 'f.filename';

        // Find out whether we're paging the results.
        $pager_limit = 10;
        $pager = true;
        $parms = drupal_get_query_parameters();
        if (isset($parms['pager']))
            if ($parms['pager'] == 'off')
                $pager = false;

        // Construct the query for the report.
        $query = db_select('ebms_doc', 'd');
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

        // Added for OCEEBMS-302.
        if (!empty($topics)) {
            $query->join('ebms_doc_topic', 't', 't.doc_id = d.doc_id');
            $query->condition('t.topic_id', $topics, 'IN');
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
        $query->orderBy($sort);
        $count_query = $query->countQuery();
        $total = $count_query->execute()->fetchField();

        // Collect information we'll use repeatedly in the loop.
        $divider = '<span class="action-divider">|</span>';
        $main_url = 'reports/documents';
        if ($request_id)
            $main_url .= "/$request_id";
        $edit_parms = drupal_get_query_parameters();

        // Build the array of table rows.
        $docs = $query->execute();
        $rows = array();
        $target = array('attributes' => array('target' => '_blank'));
        foreach ($docs as $d) {
            $edit_parms['edit'] = $d->doc_id;
            $links = l('EDIT', $main_url, array('query' => $edit_parms));

            // Robin decided she didn't like the original requirement
            // for this link.  See TIR 2450.
            $links = l('EDIT', 'docs/edit/' . $d->doc_id);
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
            $file_link = l($d->filename, $file_url, $target);
            $rows[$d->doc_id] = array(
                '#prefix' => '<tr>
 <td class="col-1">',
                '#suffix' => ' ' . $file_link . '</td>
 <td class="col-2">' . htmlspecialchars($d->name) . '</td>
 <td class="col-3">' . htmlspecialchars($boards) . '</td>
 <td class="col-4">' . htmlspecialchars($tags) . '</td>
 <td class="col-5">' . date('m/d/Y', strtotime($d->when_posted)) . '</td>
 <td class="col-6">' . $links . '</td>
</tr>',
                'bulk-archive-' . $d->doc_id => array(
                    '#type' => 'checkbox',
                    '#title' => "\xc2\xa0",
                ),
            );
        }

        // Assemble the table.
        $title = array(
            '#markup' => "<h2 id='doc-report-title'>Documents ($total)</h2>",
        );
        $table = array(
            'title' => $title,
            'archive' => array(
                '#type' => 'submit',
                '#value' => 'Archive',
                '#attributes' => array('title' => 'Archive checked documents'),
                '#suffix' =>
                '<p id="archive-instructions">' .
                '<em>Click VIEW ALL before selecting documents to archive.</em></p>',
            ),
        );
        if ($pager)
            $table['pager-top'] = array('#theme' => 'pager');
        elseif ($total > $pager_limit) {
            unset($parms['pager']);
            $opts = array('query' => $parms);
            $url = EbmsReports::$url . '/documents';
            if ($request_id)
                $url .= '/' . $request_id;
            $link = l('VIEW PAGES', $url, $opts);
            $table['pager-top'] = array(
                '#type' => 'markup',
                '#markup' => "<div id='pager-view-pages'>$link</div>",
            );
        }
        $table['head'] = array('#markup' =>
'<table id="doc-report" class="sticky-enabled">
 <thead>
  <tr>
   <th class="col-1">File Name</th>
   <th class="col-2">Uploaded By</th>
   <th class="col-3">Boards</th>
   <th class="col-4">Tags</th>
   <th class="col-5">Date Uploaded</th>
   <th class="col-6">Actions</th>
  </tr>
 </thead>
 <tbody>
');
        $table['rows'] = $rows;
        $table['tail'] = array('#markup' =>
' </tbody>
</table>
');
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
        $table['archive-bottom'] = array(
            '#type' => 'submit',
            '#value' => 'Archive',
            '#attributes' => array('title' => 'Archive checked documents'),
        );
        return $table;
    }


    /**
     * Create the Drupal render array for a report on articles whose
     * current status is approval or rejection by the NCI reviewer
     * based on the articles' abstracts
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
                'sort' => 'asc',
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
        Ebms\Util::add_status_date_range_to_query($query, $params);

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
                'sort' => 'asc',
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
        Ebms\Util::add_status_date_range_to_query($query, $params);

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
                'sort' => 'asc',
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
        $query->join('ebms_cycle', 'c2', 'c2.cycle_id = t.cycle_id');
        $query->fields('c2', array('cycle_name'));
        $query->leftJoin('ebms_article_author_cite', 'c',
            'c.article_id = s.article_id AND c.cite_order = 1');
        $query->leftJoin('ebms_article_author', 'aa',
            'aa.author_id = c.author_id');

        $query->orderByHeader($headers);
        $query->distinct();
        Ebms\Util::add_status_date_range_to_query($query, $params);

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
                'sort' => 'asc',
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
        Ebms\Util::add_status_date_range_to_query($query, $params);

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
                'sort' => 'asc',
            ),
            'Reviewer',
            'Disposition',
            'Comments',
            'Date Review Submitted',
            'Summary Topic',
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
        $query->join('ebms_topic', 'tn', 'tn.topic_id = t.topic_id');
        $query->condition('s.board_id', $params['board']);
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
        $query->fields('tn', array('topic_name'));
        if ($params['disposition']) {
            $query->join('ebms_review_disposition', 'd',
                'd.review_id = r.review_id');
            $query->condition('d.value_id', $params['disposition']);
        }
        $query->orderByHeader($headers);
        $query->distinct();
        Ebms\Util::add_status_date_range_to_query($query, $params);

        // Execute the query and collect the results in an array
        // indexed by article, each element of the array containing
        // a sequence of review ID/topic name arrays.
        $results = $query->execute();
        $articles = array();
        foreach ($results as $result) {
            $article_id = $result->article_id;
            if (!array_key_exists($article_id, $articles))
                $articles[$article_id] = array();
            $articles[$article_id][] = array(
                $result->review_id,
                $result->topic_name,
            );
        }

        // Assemble the rows for the report table.
        $counter = 0;
        $rows = array();
        foreach ($articles as $article_id => $reviews) {
            $have_citation = false;
            $rowspan = count($reviews);
            foreach ($reviews as $review) {
                list($review_id, $topic) = $review;
                $query = db_select('ebms_article_review', 'r');
                $query->join('users', 'u', 'u.uid = r.reviewer_id');
                $query->condition('r.review_id', $review_id);
                $query->fields('r',
                    array('when_posted', 'comments'));
                $query->fields('u', array('name'));
                $review_results = $query->execute();
                foreach ($review_results as $r) {
                    $col2 = array(
                        'data' => htmlspecialchars($r->name),
                        'class' => array('need-left-border'),
                    );
                    $col4 = htmlspecialchars($r->comments);
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
                $col8 = htmlspecialchars($topic);
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
                    if ($rowspan > 1)
                        $col0['rowspan'] = $col1['rowspan'] = $rowspan;
                    $row = array($col0, $col1, $col2, $col3, $col4, $col7,
                                 $col8);
                }
                else
                    $row = array($col2, $col3, $col4, $col7, $col8);
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
                'sort' => 'asc',
            ),
        );
        if (count($params['disposition']) != 1)
            $headers[] = 'Disposition';
        if (!$params['topic'])
            $headers[] = array(
                'data' => 'Summary Topic',
                'field' => 'tn.topic_name',
            );
        $headers[] = 'Review Cycle';
        $headers[] = 'Date Assigned';
        $headers[] = 'Comments';
        $headers[] = 'Merits Discussion'; // OCEEBMS-201

        // Used in the loop below for OCEEBMS-201 support.
        $merits_discussion_id = db_select('ebms_review_disposition_value', 'v')
            ->fields('v', array('value_id'))
            ->condition('v.value_name', 'Merits discussion')
            ->execute()->fetchField();

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
        if (!empty($params['disposition']))
            $query->condition('s.state_id', $params['disposition']);
        else
            $query->condition('s.state_id',
                              array_keys($manager_actions), 'IN');
        $query->fields('s', array('article_id', 'article_state_id',
                                  'state_id', 'status_dt'));
        $query->join('ebms_topic', 'tn', 'tn.topic_id = t.topic_id');
        $query->fields('tn', array('topic_name', 'topic_id'));
        $query->orderByHeader($headers);
        $query->distinct();
        Ebms\Util::add_status_date_range_to_query($query, $params);

        // Execute the query and assemble the rows for the report table.
        $results = $query->execute();
        $counter = 0;
        $rows = array();
        foreach ($results as $result) {
            $article_id = $result->article_id;
            $topic_id = $result->topic_id;
            $citation = EbmsReports::format_citation($article_id, null,
                                                     false, false, $topic_id);
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
            if (count($params['disposition']) != 1) {
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

            // New column added (OCEEBMS-201).
            $query = db_select('users', 'u');
            $query->fields('u', array('name'));
            $query->join('ebms_article_review', 'r', 'r.reviewer_id = u.uid');
            $query->join('ebms_review_disposition', 'd',
                         'd.review_id = r.review_id');
            $query->join('ebms_packet', 'p', 'p.packet_id = r.packet_id');
            $query->condition('p.topic_id', $topic_id);
            $query->condition('r.article_id', $article_id);
            $query->condition('d.value_id', $merits_discussion_id);
            $row[] = implode('<br>', $query->execute()->fetchCol());
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
                'sort' => 'asc',
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
        $query = new Ebms\EventQuery();
        $query->restrict_by_date($params);
        $query->restrict_by_type($params['meeting-type']);
        $results = $query->execute();

        // If there are no eligible meetings, the report will be empty.
        if (isset($results['node'])) {

            // Construct the query for the report.
            $meeting_ids = array_keys($results['node']);
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
            $query->fields('s',
                array('article_id', 'article_state_id', 'status_dt'));
            $query->fields('m', array('nid'));
            $query->join('ebms_topic', 'tn', 'tn.topic_id = t.topic_id');
            $query->fields('tn', array('topic_name'));
            $query->orderByHeader($headers);
            $query->distinct();
            Ebms\Util::add_status_date_range_to_query($query, $params);

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
                'sort' => 'asc',
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
        $have_date = (!empty($params['date-start']['date']) ||
                      !empty($params['date-end']['date']));
        $restrict_by_meeting = false;
        if ($have_date || $params['meeting-type']) {
            $restrict_by_meeting = true;
            $on_agenda_decision = Ebms\Util::state_id_from_text_id('OnAgenda');
            $query = new Ebms\EventQuery();
            $query->restrict_by_date($params);
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
        if (!empty($params['disposition'])) {
            $query->join('ebms_article_board_decision', 'd',
                'd.article_state_id = s.article_state_id');
            $query->condition('d.decision_value_id', $params['disposition']);
        }
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
        Ebms\Util::add_status_date_range_to_query($query, $params);

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
                'data' => 'PACKET CREATED',
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
            array(
                'data' => 'EXCLUSION REASON',
                'class' => array('col-5'),
            ),
        );

        // Retrieve the filtering values from the database.
        $params = EbmsReports::fetch_report_request($request_id);
        $board = $params['board'];
        $topic = $params['topic'];
        $cycle = $params['cycle'];
        $packet = $params['packet'];
        $reviewer = $params['reviewer'];
        $rstart = $params['review-date-start']['date'];
        $rend = $params['review-date-end']['date'];
        $pstart = $params['packet-date-start']['date'];
        $pend = $params['packet-date-end']['date'];

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
        $query->fields('p', array('created_at', 'topic_id'));
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
        if ($rstart || $rend)
            Ebms\Util::add_date_range_to_query($query, $rstart, $rend,
                'r.when_posted');
        if ($pstart || $pend)
            Ebms\Util::add_date_range_to_query($query, $pstart, $pend,
                'p.created_at');
        $query->distinct();
        $query->orderByHeader($headers);

        // Execute the database query and assemble an array indexed by
        // article ID, each element of the array containing a list of
        // packets for which the article was reviewed, with enough
        // information about those reviews that we can build the report
        // table.
        $results = $query->execute();
        $high_priority = array();
        $checked_tags = array();
        $articles = array();
        foreach ($results as $result) {
            $article_id = $result->article_id;
            if (!in_array($article_id, $high_priority)) {
                $topic_id = $result->topic_id;
                $article_topic_key = "${article_id}_${topic_id}";
                if (!in_array($article_topic_key, $checked_tags)) {
                    $tags = Ebms\Util::get_topic_tags($article_id, $topic_id);
                    if (in_array('High priority', $tags))
                        $high_priority[] = $article_id;
                    else
                        $checked_tags[] = $article_topic_key;
                }
            }
            $review_id = $result->review_id;
            //$packet = date('Y/m', strtotime($result->created_at));
            //$packet = $result->cycle_name;
            // $packet = date('Y/m', strtotime($result->start_date));
            $packet = date('Y-m-d', strtotime($result->created_at));
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
            if (in_array($article_id, $high_priority)) {
                $citation .= '<br><br><span class="hi-priority">';
                $citation .= 'HIGH PRIORITY</span>';
            }
            $article_rowspan = 0;
            ksort($packets);
            $packet_rows = array();

            // Construct an array indexed by the cycle string for each
            // packet for which we have reviews for this article.
            // Each element of the array contains a nested array of
            // information about each review returned for the article
            // and packet combination.
            foreach ($packets as $packet => $reviews) {
                $p = $packet;
                $packet_rows[$p] = array();
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
                    $query = db_select('ebms_review_rejection_value', 'v');
                    $query->join('ebms_review_rejection_reason', 'r',
                                 'r.value_id = v.value_id');
                    $query->fields('v', array('value_name'));
                    $query->condition('r.review_id', $review_id);
                    $reasons = $query->execute()->fetchCol();
                    $packet_rows[$p][] = array($reviewer, $disp, $comments,
                                               $reasons);
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
                    list($reviewer, $disp, $comments, $reasons) = $review;
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
                    }
                    if (!$have_packet_row) {
                        $have_packet_row = true;
                        $col = array(
                            'data' => str_replace('-', '&#8209;', $packet),
                            'class' => array('col-2'),
                        );
                        if (count($reviews) > 1)
                            $col['rowspan'] = count($reviews);
                        $row[] = $col;
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
                    $row[] = array(
                        'data' => implode('<br>', $reasons),
                        'class' => array('col-5'),
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
        $title = 'Literature Review Report';
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
        $request = drupal_json_decode($json);
        return $request;
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
     * Common routine to format a possibly incomplete date in the pattern
     * MM/DD/YYYY.
     *
     *  @param  string  $date   ISO date string (possibly incomplete)
     *  @param  boolean $end    Flag indicating whether this date
     *                          represents the end point for a data
     *                          range, in which case any missing
     *                          parts of the date are to be filled in
     *                          with the latest possible values
     *
     *  @return string          Date string as MM/DD/YYYY
     */
    static function format_date($date, $end=false) {
        if (!$date)
            return $end ? '[no ending date]' : '[no starting date]';
        $date = Ebms\Util::fill_in_date($date, $end);
        $date = Ebms\Util::date_string_to_array($date);
        $y = $date['year'];
        $m = $date['month'];
        $d = $date['day'];
        $t = mktime(0, 0, 0, $m, $d, $y);
        return date('m/d/Y', $t);
    }

    /**
     * Common routine to generate the HTML markup for a report's date
     * range block for the meta-data at the top of one of the citation
     * reports.
     *
     *  @param  string  $start   ISO date string (possibly incomplete)
     *  @param  string  $end     ISO date string (possibly incomplete)
     *
     *  @return string           HTML markup for date range block
     */
    static function format_date_range($start, $end) {
        $s = self::format_date($start);
        $e = self::format_date($end, true);
        return '<div class="meta"><span class="label">Date Range:</span> ' .
            $s . ' - ' . $e . '</div>';
    }

    /**
     * Get the list of Pubmed IDs in the EBMS. Be default, we just want
     * the trials which have not been rejected for all associated topics,
     * but if false is passed for the $active parameter, we check all
     * of the Pubmed IDs.
     *
     * 2016-02-03: Robin changed her mind, and now wants more restrictions
     * for the set of "active" articles.
     * https://tracker.nci.nih.gov/browse/OCEEBMS-270#comment-530146
     */
    static function pmids_in_ebms($active_only=true) {
        $query = "SELECT CONVERT(source_id, UNSIGNED INTEGER) AS pmid
  FROM {ebms_article}
 WHERE source = 'Pubmed'";
        if ($active_only)
            $query .= "
   AND article_id IN (
         SELECT DISTINCT s.article_id
           FROM {ebms_article_state} s
           JOIN ebms_article_state_type t
             ON s.state_id = t.state_id
LEFT OUTER JOIN {ebms_article_board_decision} d
             ON d.article_state_id = s.article_state_id
LEFT OUTER JOIN {ebms_article_board_decision_value} v
             ON v.value_id = d.decision_value_id
          WHERE t.state_text_id NOT IN ('RejectJournalTitle',
                                        'RejectInitReview','RejectBMReview',
                                        'RejectFullReview', 'FullEnd')
            AND s.current = 'Y'
            AND (v.value_name IS NULL OR
                 v.value_name NOT IN ('Not cited', 'Cited (citation only)',
                                      'Cited (legacy)', 'Text approved'))
   )";
        $query .= "
 ORDER BY 1";
        return db_query($query)->fetchCol();
    }

    /**
     * Check a batch of PMIDs to see which ones NLM still has.
     */
    static function slice_still_at_nlm($pmids) {
        $max = count($pmids);
        $default = 'https://eutils.ncbi.nlm.nih.gov/entrez/eutils/esearch.fcgi';
        $base = variable_get('pubmed_esearch_url', $default);
        $parms = "db=pubmed&retmax=$max&term=" . implode(',', $pmids) . '[UID]';
        $ch = curl_init();
        curl_setopt($ch, CURLOPT_URL, $base);
        curl_setopt($ch, CURLOPT_POST, true);
        curl_setopt($ch, CURLOPT_POSTFIELDS, $parms);
        curl_setopt($ch, CURLOPT_RETURNTRANSFER, TRUE);
        curl_setopt($ch, CURLOPT_SSL_VERIFYPEER, false);
        curl_setopt($ch, CURLOPT_SSL_VERIFYHOST, false);
        $results = curl_exec($ch);
        curl_close($ch);

        // Check for problems.
        $error = array();
        if (preg_match('#<ERROR>(.*)</ERROR>#Us', $results, $error))
            throw new \Exception('PMID ERROR: ' . $error[1]);
        if (!preg_match('#<IdList#', $results))
            throw new \Exception('PMID FAILURE: ' . $results);


        // Extract the PMIDs into an array.
        $pattern = '#<Id>(\d+)</Id>#';
        $offset = 0;
        $pmids = array();
        while (true) {
            $matches = array();
            $found = preg_match($pattern, $results, $matches,
                     PREG_OFFSET_CAPTURE, $offset);
            if ($found == 1) {
                $pmids[] = trim($matches[1][0]);
                $offset = $matches[1][1] + strlen($matches[1][0]);
            }
            else
                break;
        }
        return $pmids;
    }

    /**
     * Find out which of the Pubmed IDs NLM admits to still having.
     * Do this in batches to reduce the chances of running out of
     * memory, pausing briefly between requests to avoid throttling
     * at NLM.
     */
    static function still_at_nlm($pmids, $batch_size, $sleep) {
        $offset = 0;
        $verified = array();
        while ($offset < count($pmids)) {
            $slice = array_slice($pmids, $offset, $batch_size);
            $at_nlm = EbmsReports::slice_still_at_nlm($slice);
            $verified = array_merge($verified, $at_nlm);
            $offset += $batch_size;
            if ($sleep > 0)
                sleep($sleep);
        }
        return $verified;
    }

    /**
     * Find out which EBMS articles fetched from Pubmed have been
     * lost by NLM.
     *
     *  @param  boolean  $active_only  if true (the default) only
     *                                 check articles which have at
     *                                 least one topic which has not
     *                                 been rejected
     *  @param  int      $batch_size   how many PMIDs should we check
     *                                 in a single request (to avoid
     *                                 exhausting available memory)?
     *                                 default is 10000
     *  @param  int      $sleep_secs   how long (in seconds) should
     *                                 we delay between requests
     *                                 (we don't want NLM to cut us
     *                                 off because we're flooding
     *                                 their server)
     *  @return object                 object containing two properties:
     *                   checked       the number of articles checked
     *                   missing       the list of PMIDs lost
     */
    public static function lost_by_nlm($active_only=true,
                                       $batch_size=10000,
                                       $sleep_secs=2) {
        $pmids = EbmsReports::pmids_in_ebms($active_only);
        $confirmed = EbmsReports::still_at_nlm($pmids, $batch_size,
                                              $sleep_secs);
        $report = new stdClass();
        $report->checked = count($pmids);
        $report->missing = array_diff($pmids, $confirmed);
        return $report;
    }

    /**
     * Determine which boards are associated with this message.
     *  @param message array   entity wrapper for message
     *
     * return          array   board IDs
     */
    public static function boards_for_message(\EntityDrupalWrapper $message) {
        $ids = $message->field_boards->value();
        $subgroup_ids = $message->field_subgroups->value();
        if (!empty($subgroup_ids)) {
            $query = db_select('ebms_subgroup', 's')
                ->condition('s.sg_id', $subgroup_ids, 'IN')
                ->fields('s', array('board_id'));
            if (!empty($ids))
                $query->condition('s.board_id', $ids, 'NOT IN');
            $more_ids = $query->execute()->fetchCol();
            if (!empty($more_ids))
                $ids = array_merge($ids, $more_ids);
        }
        $group_ids = $message->field_ad_hoc_groups->value();
        if (!empty($group_ids)) {
            $query = db_select('ebms_ad_hoc_group_board', 'b')
                ->condition('b.group_id', $group_ids, 'IN')
                ->fields('b', array('board_id'));
            if (!empty($ids))
                $query->condition('b.board_id', $ids, 'NOT IN');
            $more_ids = $query->execute()->fetchCol();
            if (!empty($more_ids))
                $ids = array_merge($ids, $more_ids);
        }
        return $ids;
    }

    /**
     * Find out whether this is an in-person or WebEx event.
     *  @param message array   entity wrapper for message
     *
     * return          string  "In person" or "WebEx"
     */
    public static function event_description(\EntityDrupalWrapper $wrapper) {
        $event_id = $wrapper->field_event_id->value();
        $events = entity_load('node', array($event_id));
        foreach ($events as $event) {
            $event_date = field_get_items('node', $event, 'field_datespan');
            $event_type = field_get_items('node', $event, 'field_event_type');
            $event_type = $event_type[0]['value'];
            if ($event_type == 'remote')
                $event_type = 'WebEx';
            elseif ($event_type == 'in_person')
                $event_type = 'In person';
            $start_date = date('Y-m-d', $event_date[0]['value']);
            $title = htmlspecialchars($event->title);
            return "$title - $start_date - $event_type";
        }
        return 'EVENT NOT FOUND';
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
    $board = \Ebms\Util::get_user_default_board(0);
    $cycle = $report = 0;
    $start_date = $end_date = '';

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
        if (isset($values['date-start'])) {
            $start_date = $values['date-start']['date'];
            if ($start_date)
                $start_date = Ebms\Util::fill_in_date($start_date);
        }
        if (isset($values['date-end'])) {
            $end_date = $values['date-end']['date'];
            if ($end_date)
                $end_date = Ebms\Util::fill_in_date($end_date, true);
        }
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
        $date_field = Ebms\Util::date_range_field('Decision Date', 'date',
                                                  $start_date, $end_date);
        $form['top']['report-form']['ajax fields']['date'] = $date_field;
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
                $start = $values['date-start']['date'];
                $end = $values['date-end']['date'];
                if ($cycle) {
                    $msg = 'Cannot specify both cycle and dates.';
                    if ($start)
                        form_set_error('date-start', $msg);
                    elseif ($end)
                        form_set_error('end-date', $msg);
                }
                elseif (!$start && !$end)
                    form_set_error('cycle', 'Must specify cycle or dates.');
                break;

            // For the report on articles for which the board manager
            // have added topics not included by the librarian,
            // we must have a fully specified date range.
            case 'Citation Summary Topic Changes':
                $start = $values['date-start']['date'];
                $end = $values['date-end']['date'];
                if (!$start && !$end)
                    form_set_error('date-start', 'A date is required.');
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
function pdq_ebms_import_reports_form($form, &$form_state, $request_id=0) {

    // Override the breadcrumbs which Drupal would automatically
    // set based on the menu callback, in order to comply with
    // the customized requirements of the EBMS design.
    drupal_set_breadcrumb(
        array(
            l('Reports', 'reports'),
            l('Citation Management Reports', 'reports/citation'),
            'Import Report',
        )
    );

    // Initialize defaults for the form field's values.
    $board = \Ebms\Util::get_user_default_board(0);
    $cycle = $topic = 0;
    $op = $start = $end = '';

    // Check for overrides of the form's defaults if we have a request.
    if ($request_id) {
        $params = EbmsReports::fetch_report_request($request_id);
        $board = $params['board'];
        $topic = $params['topic'];
        $cycle = $params['cycle'];
        $start = $params['import-start']['date'];
        $end = $params['import-end']['date'];
        $op = $params['op'];
        if ($start)
            $start = Ebms\Util::fill_in_date($start);
        if ($end)
            $end = Ebms\Util::fill_in_date($end);
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
                                            'Import Report');
    $import_date = Ebms\Util::date_range_field('Import Date', 'import',
                                               $start, $end);
    $form['top'] = array(
        '#type' => 'container',
        '#attributes' => array('id' => 'fulltext-top'),
        'left-nav' => $left_nav,
        'report-form' => array(
            '#type' => 'container',
            '#attributes' => array('id' => 'report-form'),
            'title' => array(
                '#type' => 'markup',
                '#markup' => '<h2>Import Report</h2>',
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
            'import-date' => $import_date,
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
    $parms = drupal_get_query_parameters();
    if (!empty($parms['job']))
        $form['report'] = EbmsReports::import_job($parms['job']);

    // If the user has given us the criteria for selecting import
    // jobs, build and display the list of matching jobs.
    elseif ($request_id)
        $form['report'] = EbmsReports::import_reports($request_id);
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
    $values = $form_state['values'];
    $request_id = EbmsReports::save_report_request($values, 'Imports');
    drupal_goto('reports/import/' . $request_id);
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

    // See https://tracker.nci.nih.gov/browse/OCEEBMS-292.
    if (isset($form_state['confirm_archive']))
        return pdq_ebms_doc_archive($form, $form_state, $request_id);

    // Initialize defaults for the form field's values.
    $board = 0; // \Ebms\Util::get_user_default_board(0); OCEEBMS-294
    $tag = $inhouse = $member = $uploader_board = 0;
    $topic = array();
    $start = $end = '';
    $archived = false;
    $sort = 'f.filename';

    // Check for overrides of the defaults by request parameters
    // stored in the database.
    if ($request_id) {
        $params = EbmsReports::fetch_report_request($request_id);
        $board = $params['board'];
        foreach ($params['topic'] as $id => $val)
            if ($val)
                $topic[] = $id;
        $tag = $params['tag'];
        $inhouse = $params['inhouse'];
        $member = $params['member'];
        $uploader_board = $params['uploader-board'];
        $archived = $params['archived'];
        $start = $params['upload-start']['date'];
        $end = $params['upload-end']['date'];
        if ($start)
            $start = Ebms\Util::fill_in_date($start);
        if ($end)
            $end = Ebms\Util::fill_in_date($end);
        $sort = $params['sort'];
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

    // Get the values of interest for ajax callbacks.
    if (isset($form_state['values'])) {
        $board = $form_state['values']['board'];
        $topic = $form_state['values']['topic'];
    }

    // Use the board choice to determine which topic checkboxes will be drawn.
    $topics = array();
    if ($board) {
        $results = db_select('ebms_topic', 't')
            ->fields('t', array('topic_id', 'topic_name'))
            ->condition('t.board_id', $board)
            ->condition('t.active_status', 'A')
            ->orderBy('t.topic_name')
            ->execute();
        foreach ($results as $result)
            $topics[$result->topic_id] = $result->topic_name;

        // Make sure we unselect any leftover topic selection which
        // isn't valid with a change in the composition of the Summary
        // Topic picklist.
        $new_topic = array();
        foreach ($topic as $t)
            if (array_key_exists($t, $topics))
                $new_topic[] = $t;
        $topic = $form_state['values']['topic'] = $new_topic;
    }

    // Begin the form structure, including the left nav penel.
    $left_nav = EbmsReports::build_left_nav('Board Management',
                                            'Documents');
    $date_field = Ebms\Util::date_range_field('Date Uploaded', 'upload',
                                              $start, $end);
    $form['request-id'] = array(
        '#type' => 'hidden',
        '#value' => $request_id,
    );
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
                '#ajax' => array(
                    'callback' => 'pdq_ebms_report_ajax',
                    'wrapper' => 'board-dependent',
                ),
            ),
            'ajax fields' => array(
                '#type' => 'fieldset',
                '#attributes' => array(
                    'id' => 'board-dependent',
                    'class' => array('clearfix'),
                ),
                '#title' => 'SUMMARY TOPIC',
                '#collapsible' => true,
                '#collapsed' => count($topics) < 1,
                'topic' => array(
                    '#type' => 'checkboxes',
                    '#title' => '',
                    // '#title_display' => 'after',
                    '#options' => $topics,
                    '#default_value' => $topic,
                    //'#attributes' => array('class' => array('fixed-width')),
                ),
            ),
            'tag' => array(
                '#type' => 'select',
                '#title' => 'Tag',
                '#title_display' => 'after',
                '#options' => $tags,
                '#default_value' => $tag,
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
            '$upload-date' => $date_field,
            'archived' => array(
                '#type' => 'checkbox',
                '#title' => 'ARCHIVED',
                '#default_value' => $archived,
            ),
            'sort' => array(
                '#type' => 'select',
                '#title' => 'Order By',
                '#title_display' => 'after',
                '#options' => array(
                    'f.filename' => 'File Name',
                    'u.name' => 'Uploaded By',
                    'd.when_posted' => 'Date Uploaded',
                ),
                '#default_value' => $sort,
                '#attributes' => array('class' => array('fixed-width')),
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
 * Make sure at least one document has been selected for archiving.
 * Part of OCEEBMS-292.
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
function pdq_ebms_documents_report_form_validate($form, &$form_state) {
    if ($form_state['triggering_element']['#value'] == t('Archive')) {
        $docs_to_confirm = array();
        $prefix = 'bulk-archive-';
        $len = strlen($prefix);
        foreach ($form_state['values'] as $key => $value) {
            if (substr($key, 0, $len) == $prefix) {
                if ($value)
                    $docs_to_confirm[] = substr($key, $len);
            }
        }
        if (empty($docs_to_confirm))
            form_set_error('', 'No documents selected for archiving.');
        else
            $form_state['docs-to-confirm'] = $docs_to_confirm;
    }
}

/**
 * If the user clicks the "Reset" button, clear the form by routing
 * back to the URL for the import report.  Otherwise store the report
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

    // OCEEBMS-292: support bulk archiving of documents.
    if (!empty($form_state['values']['archive_ids']))
        return pdq_ebms_doc_archive_submit($form, $form_state);
    if ($form_state['triggering_element']['#value'] == t('Archive')) {
        $form_state['rebuild'] = true;
        $form_state['confirm_archive'] = true;
        return;
    }

    if ($form_state['values']['op'] == 'Reset')
        drupal_goto('reports/documents');
    if ($form_state['values']['op'] != 'Filter')
        return;
    $values = $form_state['values'];
    $request_id = EbmsReports::save_report_request($values, 'Documents');
    drupal_goto('reports/documents/' . $request_id);
}

/**
 * Put up a confirmation page asking if the user really wants to
 * archive the selected documents.
 * See https://tracker.nci.nih.gov/browse/OCEEBMS-292.
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
function pdq_ebms_doc_archive($form, &$form_state, $report_id) {
    $form['archive_ids'] = array(
        '#type' => 'value',
        '#value' => $form_state['docs-to-confirm'],
    );
    $form['report_id'] = array(
        '#type' => 'value',
        '#value' => $report_id,
    );
    $query = db_select('ebms_doc', 'd');
    $query->join('file_managed', 'f', 'f.fid = d.file_id');
    $query->condition('d.doc_id', $form_state['docs-to-confirm'], 'IN');
    $query->fields('f', array('filename', 'uri'));
    $query->orderBy('f.filename');
    $results = $query->execute();
    $docs = array();
    foreach ($results as $result)
        $docs[] = htmlspecialchars($result->filename);
    $these = count($docs) == 1 ? 'this document' : 'these documents';
    $form['info'] = array(
        '#markup' => theme_item_list(
            array(
                'items' => $docs,
                'title' => "Do you want to archive $these?",
                'type' => 'ul',
                'attributes' => array('id' => 'docs-to-archive'),
            )
        ),
    );

    // Assemble the confirmation form.
    $path = 'reports/documents';
    if ($report_id)
        $path .= "/$report_id";
    $path = array(
        'path' => $path,
        'query' => array(
            'pager' => 'off',
            'items_per_page' => 'All',
        ),
    );
    return confirm_form(
        $form,
        'Archive selected documents?',
        $path,
        '<p><em>This action cannot be undone.</em></p>'
    );
}

/**
 * Set the drop_flag column for the rows in the ebms_doc table
 * corresponding to the documents marked for archiving.
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
function pdq_ebms_doc_archive_submit($form, &$form_state) {
    $count = $first_id = 0;
    $bail_url = EbmsReports::$url . '/documents';
    foreach ($form_state['values']['archive_ids'] as $doc_id) {
        $int_id = Ebms\Util::validate_int($doc_id, $bail_url);
        db_update('ebms_doc')
            ->fields(array('drop_flag' => 1))
            ->condition('doc_id', $int_id)
            ->execute();
        $count++;
        if (!$first_id)
            $first_id = $doc_id;
    }
    if ($count == 1) {
        $query = db_select('ebms_doc', 'd');
        $query->join('file_managed', 'f', 'f.fid = d.file_id');
        $query->condition('d.doc_id', $first_id);
        $query->fields('f', array('filename', 'uri'));
        $name = $query->execute()->fetchField();
        drupal_set_message(htmlspecialchars($name) . ' archived.');
    }
    else
        drupal_set_message("$count documents archived.");
    $target = 'reports/documents';
    if (!empty($form_state['values']['report_id']))
        $target .= '/' . $form_state['values']['report_id'];
    drupal_goto(
        $target,
        array(
            'query' => array(
                'pager' => 'off',
                'items_per_page' => 'All',
            ),
        )
    );
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
    $board = \Ebms\Util::get_user_default_board(0);
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
 * Drupal form API callback which generates the form for the
 * report on member login attempts.  Used to help identify board
 * members having login problems.
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
function pdq_ebms_login_attempts_form($form, &$form_state, $request_id=0) {

    // Override the breadcrumbs which Drupal would automatically
    // set based on the menu callback, in order to comply with
    // the customized requirements of the EBMS design.
    drupal_set_breadcrumb(
        array(
            l('Reports', 'reports'),
            l('Board Management Reports', 'reports/login-attempts'),
            'Board Member Login Attempts',
        )
    );

    // Get some data from previous submission of the form, if any
    // Initialize defaults for the form field's values.
    $start = $end = '';
    $show_details = 'none';

    // If user pressed Submit button, his parameters are saved in the DB
    // with a request_id and this form re-invoked.  Execute the request.
    if ($request_id) {
        $values = EbmsReports::fetch_report_request($request_id);
        $start = $values['date-start']['date'];
        $end = $values['date-end']['date'];
        $show_details = $values['show-details'];
        if ($start)
            $start = Ebms\Util::fill_in_date($start);
        if ($end)
            $end = Ebms\Util::fill_in_date($end, true);
    }
    else {
        // Find the minimum and maximum dates
        $qry = db_select('watchdog', 'w');
        $qry->addExpression('min(w.timestamp)', 'mindatetime');
        $qry->addExpression('max(w.timestamp)', 'maxdatetime');
        $result = $qry->execute()->fetchAll();

        // Create defaults.  Max is one day (86400 seconds)
        //  more than current date
        if (!$start)
            $start = date('Y-m-d', $result[0]->mindatetime);
        if (!$end)
            // Adjust end to one day after first second of day
            $end = date('Y-m-d', $result[0]->maxdatetime);
            //$end = date('Y-m-d', intval($result[0]->maxdatetime/86400)+86400);
    }

    // Assemble the login-attempts report form
    $form = array(
        '#type' => 'container',
        '#attributes' => array('id' => 'report-form'),
        'title' => array('#markup' =>
            '<h2>Board Member Login Attempts</h2>'),
        'date' => Ebms\Util::date_range_field(
                            'Login attempt dates (inclusive)', 'date',
                                                   $start, $end, '', false),
        'show-details' => array(
            '#type' => 'radios',
            '#title' => 'Report options',
            '#options' => array(
                'none' => 'Statistics Only',
                'failed' =>
        'Board Members With Failed Login Attempts (Excludes NIH Board Members)',
                'all' =>
        'All Board Member Login Attempts (Includes NIH Board Members)',
            ),
            '#default_value' => $show_details,
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

    // If request_id passed, user already pressed submit on previous go-round
    // Call the function to build the report and append it to the form.
    if ($request_id) {
        $form = pdq_ebms_login_attempts_build_report($form, $start, $end,
                                                     $show_details);
    }

    // Assemble the page where the form lives, including the left nav penel.
    $left_nav = EbmsReports::build_left_nav('Board Management',
                                            'Login Attempts');
    $page['top'] = array(
        '#type' => 'container',
        '#attributes' => array('id' => 'cites-by-status-report-top'),
        'left-nav' => $left_nav,
        'report-form' => $form,
    );

    return $page;
}

/**
 * Validation routine for the login-attempts form submission
 *  @param  array   $form         Structured array containing the
 *                                elements and properties of the form
 *  @param  array   &$form_state  Modifiable structured array
 *                                containing the current values
 *                                of the form, as well as other
 *                                form state information
 *
 *  @return none
 */
function pdq_ebms_login_attempts_form_validate($form, &$form_state) {

    $values = $form_state['values'];

    // Validate dates
    $startStr = $values['date-start']['date'];
    if (!$startStr) {
        form_set_error('date-start', 'Starting date is required');
        return;
    }
    $endStr = $values['date-end']['date'];
    if (!$endStr) {
        form_set_error('date-end', 'Ending date is required');
        return;
    }

    // Avoid simple user error
    if ($endStr < $startStr) {
        form_set_error('date-start','Start date must be on or before end date');
        return;
    }
}

/**
 * Helper class for producing the login_attempts report.
 *
 * Each object represents one user (Board Member as of this writing)
 * or one time period (months as of this writing.)
 *
 * The user or time identifier is maintained outside the object
 * and the object is externally associated with the identifier.
 */
class LoginAttempts {
    public
        $uid,           // User unique ID, not available for all logins
        $name,          // User name
        $successCount,  // Number of successful logins
        $lastSuccess,   // Timestamp of last success
        $failureCount,  // Number of login failures
        $lastFailure;   // Timestamp

    /**
     * Constructor.
     *
     * Pass:
     *  @param string  $name    User name, required
     *  @param int     $uid     May be null if user logged in w/wrong name
     */
    public function __construct($name, $uid=NULL) {

        $this->name         = $name;
        $this->uid          = $uid;
        $this->successCount = 0;
        $this->lastSuccess  = 0;
        $this->failureCount = 0;
        $this->lastFailure  = 0;
    }

    /**
     * Add another datapoint for this object.
     *
     * Pass:
     *  @param boolean $success     True = record success, else failure.
     *  @param int     $timestamp   UNIX timestamp.
     */
    public function update($success, $timestamp) {
        if ($success) {
            $this->successCount++;
            if ($lastSuccess > $this->timestamp)
                $this->lastSuccess = $timestamp;
        }
        else {
            $this->failureCount++;
            if ($lastFailure > $this->timestamp)
                $this->lastFailure = $timestamp;
        }
    }

    /**
     * Utility function to find name and id from watchdog list.
     *
     *  @param string $vars         Watchdog "variables" column
     *  @param string $varName      JSON variable name to find, e.g., '%user'
     *  @param boolean $roleName    Only search for this user role, e.g.
     *                              'board member'
     *
     *  @return stdClass object
     *    ->name                User name, from users table if found there
     *    ->uid                 uid from users table, if found there
     *
     *    If name not found, returns passed name + Null uid.
     */
    const WRONG_USER_TYPE = 'WrongUserType';

    static function loginNameConvert($vars, $varName,
                                     $roleName='board member',
                                     $authModule='nci_edir') {

        // Dig the name out of the watchdog variables
        // This works because we know the structure of the message col
        //   contains '%user' for failed login attempts
        $struct  = unserialize($vars);
        $chkName = $struct[$varName];

        $obj = new stdClass;

        // Get uid and canonical name, if possible
        $qry = db_select('users', 'u')
            ->fields('u', array('uid', 'name'))
            ->condition('u.name', $chkName);

        $row = $qry->execute()->fetchObject();

        // No hit
        if (!$row) {
            $obj->name = $chkName;
            $obj->uid  = NULL;
            return $obj;
        }
        else {
            // Got one hit.  Two users with one name are not allowed in Drupal
            //  return canonical name and uid
            $obj->name = $row->name;
            $obj->uid  = $row->uid;

            // Remove hit if wrong role or auth module
            if ($roleName || $authModule) {

                $qry = db_select('users', 'u')
                     ->fields('u', array('uid'))
                     ->condition('u.uid', $obj->uid);

                // Limit to specific role
                if ($roleName) {
                    $qry->join('users_roles', 'ur', 'u.uid = ur.uid');
                    $qry->join('role', 'r', 'ur.rid = r.rid');
                    $qry->condition('r.name', $roleName);
                }

                if ($authModule) {
                    $qry->join('authmap', 'am', 'u.uid = am.uid');
                    $qry->condition('am.module', $authModule);
                }

                $result = $qry->execute()->rowCount();

                if ($result == 0)
                    $obj->name = LoginAttempts::WRONG_USER_TYPE;
            }
        }
        return $obj;
    }
}

/**
 * Like all other report submit functions:
 *   If:   "Reset" button, go back to repaint form.
 *   Else: Save the parameters and re-invoke form with passed parms
 *         to build and append the report.
 *
 *  @param  array   $form         Structured array containing the
 *                                elements and properties of the form
 *  @param  array   &$form_state  Modifiable structured array
 *                                containing the current values
 *                                of the form, as well as other
 *                                form state information
 *
 *  @return none
 */
function pdq_ebms_login_attempts_form_submit($form, &$form_state) {
    if ($form_state['values']['op'] == 'Reset')
        drupal_goto('reports/login-attempts');
    if ($form_state['values']['op'] == 'Submit') {
        $values = $form_state['values'];
        $request_id = EbmsReports::save_report_request($values,
                                                       'Literature Reviews');
        drupal_goto('reports/login-attempts/' . $request_id);
    }
}

/**
 * Build the login attempts report(s).  There may or may not be a "details"
 * report included.
 *
 *  @param array $form      Structured array containing the
 *                            elements and properties of the form.
 *  @param int $start       UNIX timestamp for earliest date of report.
 *  @param int $end         UNIX timestamp for latest date of report.
 *  @param string $details  What details to include.  See form.
 *
 *  @return                 Updated form including the report(s).
 */
function pdq_ebms_login_attempts_build_report($form, $startDt, $endDt,
                                              $details) {

    // Conversions to UNIX timestamps
    $start = strtotime($startDt);
    $end   = strtotime($endDt);

    // -------------------
    // Gather information
    // -------------------

    // Total board members
    $qry = db_select('users', 'u');
    $qry->join('users_roles', 'ur', 'u.uid = ur.uid');
    $qry->join('role', 'r', 'ur.rid = r.rid');
    $qry->condition('r.name', 'board member');
    $qry->condition('u.status', 1);
    $bmCount = $qry->countQuery()->execute()->fetchField();

    // Board member edir users
    $qry = db_select('users', 'u');
    $qry->join('users_roles', 'ur', 'u.uid = ur.uid');
    $qry->join('authmap', 'a', 'u.uid = a.uid');
    $qry->join('role', 'r', 'ur.rid = r.rid');
    $qry->fields('users', array('uid'));
    $qry->condition('r.name', 'board member');
    $qry->condition('a.module', 'nci_edir');
    $qry->condition('u.status', 1);
    $qry->condition('a.module', 'nci_edir');
    $qry->condition('u.status', 1);
    $bmEdirCount = $qry->countQuery()->execute()->fetchField();

    // Assemble all report elements here
    //   $report = array of reports
    //     $report['status']  = Table of current counts of relevant user info.
    //     $report['monthly'] = Table of successes and failures by month.
    //     $report['details'] = Table of info by user.
    // At the end, we return the info with:
    //   $form['report'] = $report
    $report = array();

    // -------------------
    // Current counts report
    // -------------------
    $report['status'] = array(
        'title' => array('#markup' =>
            '<br /><h3>Current board member and login info</h3>'),
        'table' => array(
            '#theme' => 'table',
            '#header' => array('Board Members', 'Board Member EDIR users'),

            '#rows'   => array(array($bmCount, $bmEdirCount)),
        ),
    );

    // -------------------
    // Monthly report
    // -------------------

    $monthRows = array();
    $successTotal     = 0;
    $failBdMbrTotal   = 0;
    $failUnknownTotal = 0;

    // Find the start and end of the first month.
    // $thisMonth may be earlier than $start.
    // That's intentional, need first of the month to find boundaries.
    $thisMonth  = strtotime(date('Y-m-d', $start));
    $beginMonth = strtotime(date('Y-m-01', $start));
    $nextMonth  = strtotime('+1 month', $beginMonth);

    // Select data for each month
    while (true) {
        if ($nextMonth > $end)
            $nextMonth = $end;

        $successCount = db_query("
            SELECT COUNT(*)
              FROM watchdog w
              JOIN users_roles ur
                ON ur.uid = w.uid
              JOIN role r
                ON r.rid = ur.rid
              JOIN authmap am
                ON am.uid = w.uid
             WHERE w.timestamp >= :this
               AND w.timestamp < :next
               AND w.message = 'Session opened for %name.'
               AND r.name = 'board member'
               AND am.module = 'nci_edir'",
            array(':this'=>$thisMonth, ':next'=>$nextMonth))->fetchField();

        // Failure count is harder.  We only want board members using edir,
        //  but if the login didn't succeed, there's no link to the user
        //  record and no easy way to tell who the user is.
        // We'll do it the hard way, matching each person by name
        $results = db_query("
            SELECT variables, COUNT(*) AS cnt
              FROM watchdog
             WHERE message = 'Login attempt failed for %user.'
               AND timestamp BETWEEN :start AND :end
             GROUP BY variables",
               array(':start'=>$thisMonth, ':end'=>$nextMonth))->fetchAll();

        // For each failure group, try to find a uid and check his role
        $failBdMbrCount   = 0;
        $failUnknownCount = 0;
        foreach ($results as $row) {
            $usrInfo = LoginAttempts::loginNameConvert($row->variables,
                                    '%user', 'board member', 'nci_edir');
            // Don't include users known not to be board members or edir users
            if ($usrInfo->name == LoginAttempts::WRONG_USER_TYPE)
                continue;

            // Anything missing that filter is either a board member, or
            //  an unmatched name
            if ($usrInfo->uid) {
                $failBdMbrCount += $row->cnt;
                $failBdMbrTotal += $row->cnt;
            }
            else {
                $failUnknownCount += $row->cnt;
                $failUnknownTotal += $row->cnt;
            }
        }
        $successTotal += $successCount;

        // Add row for the month to the report
        $row = array(date('Y M', $thisMonth), $successCount,
                    $failBdMbrCount, $failUnknownCount);
        $monthRows[] = $row;

        // Next month
        if ($nextMonth >= $end)
            break;
        $thisMonth = $nextMonth;
        $nextMonth = strtotime('+1 month', $thisMonth);
    }
    // Add totals
    $monthRows[] = array('<b>TOTALS</b>',
                        "<b>$successTotal</b>",
                        "<b>$failBdMbrTotal</b>",
                        "<b>$failUnknownTotal</b>");

    $report['monthly'] = array(
        'title' => array('#markup' =>
            '<br /><h3>Monthly login attempt breakdown</h3>'),
        'table' => array(
            '#theme' => 'table',
            '#header' => array('Month', 'Successes',
                               'Failed Members', 'Failed Unknown'),
            '#rows'   => $monthRows,
        ),
    );

    // Are we done?
    if ($details == 'none') {
        $form['report'] = $report;
        return $form;
    }

    // -------------------
    // Detail report
    // -------------------

    // Accumulate data here
    $successes = 0;
    $failures  = 0;

    // Get all login failures in our timeframe
    $results = db_query("
            SELECT variables, COUNT(*) AS cnt, max(timestamp) AS maxts
              FROM watchdog
             WHERE message = 'Login attempt failed for %user.'
               AND timestamp BETWEEN :start AND :end
             GROUP BY variables
             ORDER BY timestamp",
               array(':start'=>$start, ':end'=>$end))->fetchAll();

    // An array of of LoginAttempts objects, keyed by user name
    $loginAttempts = array();

    // An index into that array by user ID, for those for which we know it
    $loginAttemptsByUid = array();

    // Parse the results and fill array and its index
    foreach ($results as $row) {
        $usrInfo = LoginAttempts::loginNameConvert($row->variables, '%user',
                                                   'board member', 'nci_edir');

        // Don't include people known not to be board members or edir users
        if ($usrInfo->name == LoginAttempts::WRONG_USER_TYPE)
            continue;

        // Create an object for it
        $lat = new LoginAttempts($usrInfo->name, $usrInfo->uid);
        $lat->failureCount = $row->cnt;
        $lat->lastFailure  = $row->maxts;

        // Save it by the user name.  Can't use uid because failed login
        //  attempts cannot always identify a uid.
        $loginAttempts[$usrInfo->name] = $lat;

        if ($lat->uid)
            $loginAttemptsByUid[$lat->uid] = $lat->name;

        // Cumulate
        $failures += $lat->failureCount;
    }

    $results = db_query("
        SELECT w.uid, u.name, COUNT(*) AS cnt, max(w.timestamp) AS maxts
          FROM watchdog w
          JOIN users u
            ON w.uid = u.uid
          JOIN users_roles ur
            ON w.uid = ur.uid
          JOIN role r
            ON ur.rid = r.rid
         WHERE w.message = 'Session opened for %name.'
           AND w.timestamp BETWEEN :start AND :end
           AND r.name = 'board member'
         GROUP BY w.uid",
           array(':start'=>$start, ':end'=>$end))->fetchAll();

    foreach ($results as $row) {

        // Update the matching object, if there is one
        if (array_key_exists($row->uid, $loginAttemptsByUid)) {
            $lat = $loginAttempts[$loginAttemptsByUid[$row->uid]];
        }
        // If only processing failures and no matching object, forget row
        elseif ($details == 'failed') {
            continue;
        }
        // Else processing all member logins, create row for one w/no failure
        else {
            $lat = new LoginAttempts($row->name, $row->uid);
        }

        $lat->successCount = $row->cnt;
        $lat->lastSuccess  = $row->maxts;

        // Cumulate
        $successes += $lat->successCount;

        // Insert or replace object in the array
        $loginAttempts[$lat->name] = $lat;
    }

    // Sort rows by name, case insensitive
    // Uses method that works without requiring PHP 5.4
    uksort($loginAttempts, 'strcasecmp');

    $detailRows = array();
    foreach ($loginAttempts as $lat) {

        // Differentiate known board members from unknown names
        if ($lat->uid)
            $lat->name .= '*';

        // If no date available, ensure empty cell
        if ($lat->lastSuccess > 0)
            $lastSuccess = date('M d, Y', $lat->lastSuccess);
        else
            $lastSuccess = '';
        if ($lat->lastFailure > 0)
            $lastFailure = date('M d, Y', $lat->lastFailure);
        else
            $lastFailure = '';

        // Create a row for this user
        $row = array($lat->name,
                $lat->successCount, $lastSuccess,
                $lat->failureCount, $lastFailure);

        // Append it to our list
        $detailRows[] = $row;
    }

    // Add the counts
    $detailRows[] = array('<b>TOTALS</b>',
                          "<b>$successes</b>", '',
                          "<b>$failures</b>", '');

    // Add the details to the report
    $report['details'] = array(
        'title' => array('#markup' =>
            '<br /><h3>Detailed login attempt breakdown</h3>'),
        'table' => array(
            '#theme' => 'table',
            '#header' => array('Member Name*', 'Successes', 'Last Success',
                               'Failures', 'Last Failure'),
            '#rows'   => $detailRows,
        ),
    );

    $form['report'] = $report;
    return $form;
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
    $word_xml = 'vnd.openxmlformats-officedocument.wordprocessingml.document';
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

        // Workaround for shifting MS word mime types (OCEEBMS-383).
        if ($mime_type == "application/$word_xml")
            $mime_type = 'application/msword';

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
    $board = \Ebms\Util::get_user_default_board(0);
    $topic = $reviewer = $cycle = $rrange = $prange = $packet = 0;
    $rstart = $rend = $pstart = $pend = '';

    // Check for overrides of the defaults by request parameters
    // stored in the database.
    if ($request_id) {
        $values = EbmsReports::fetch_report_request($request_id);
        $board = $values['board'];
        $topic = $values['topic'];
        $reviewer = $values['reviewer'];
        $cycle = $values['cycle'];
        $packet = $values['packet'];
        $rstart = $values['review-date-start']['date'];
        $pstart = $values['packet-date-start']['date'];
        $rend = $values['review-date-end']['date'];
        $pend = $values['packet-date-end']['date'];
        if ($pstart)
            $pstart = Ebms\Util::fill_in_date($pstart, true);
        if ($rstart)
            $rstart = Ebms\Util::fill_in_date($rstart, true);
        if ($pend)
            $pend = Ebms\Util::fill_in_date($pend, true);
        if ($rend)
            $rend = Ebms\Util::fill_in_date($rend, true);
    }

    // OCEEBMS-200: pre-populate lit reviews report
    else {
        $parms = drupal_get_query_parameters();
        if (!empty($parms['packet'])) {
            $packet = $parms['packet'];
            $query = db_select('ebms_packet', 'p');
            $query->join('ebms_topic', 't', 't.topic_id = p.topic_id');
            $query->fields('p', array('topic_id', 'created_at'));
            $query->fields('t', array('board_id'));
            $query->condition('p.packet_id', $packet);
            $packet_info = $query->execute()->fetchObject();
            $board = $packet_info->board_id;
            $topic = $packet_info->topic_id;
            $created = $packet_info->created_at;
            $pstart = $created;
        }
    }

    // Initialize picklist arrays (except topics and reviewers, handled below).
    $boards = array(0 => ' ') + Ebms\Util::boards();
    $cycles = array(0 => ' ') + Ebms\Util::cycles(false);
    $packets = array(0 => ' ');

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
    $reviewers = array(0 => ' ');
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
        $results = $query->execute();
        foreach ($results as $result)
            $packets[$result->packet_id] = $result->packet_title;
        if (!array_key_exists($packet, $packets))
            $packet = 0;
        $board_member_role = db_select('role', 'r')
            ->fields('r', array('rid'))
            ->condition('r.name', 'board member')
            ->execute()
            ->fetchField();
        $query = db_select('users', 'u');
        $query->join('users_roles', 'r', 'r.uid = u.uid');
        $query->condition('r.rid', $board_member_role);
        $query->fields('u', array('uid', 'name'));
        $query->join('ebms_board_member', 'm', 'm.user_id = u.uid');
        $query->condition('m.board_id', $board);
        $query->orderBy('u.name');
        $results = $query->execute();
        foreach ($results as $result)
            $reviewers[$result->uid] = $result->name;
    }

    // Begin the form structure, including the left nav penel.
    $left_nav = EbmsReports::build_left_nav('Citation Management',
                                            'Literature Reviews');
    $rdate_field = Ebms\Util::date_range_field('Date Review Submitted',
                                               'review-date', $rstart, $rend);
    $pdate_field = Ebms\Util::date_range_field('Date Packet Created',
                                               'packet-date', $pstart, $pend);
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
            'review-date' => $rdate_field,
            'packet-date' => $pdate_field,
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
        $pstart = $values['packet-date-start']['date'];
        $rstart = $values['review-date-start']['date'];
        $pend = $values['packet-date-end']['date'];
        $rend = $values['review-date-end']['date'];
        if ($pend)
            $pend = Ebms\Util::fill_in_date($pend, true);
        if ($rend)
            $rend = Ebms\Util::fill_in_date($rend, true);
        if ($pstart && $pend && $pstart >= $pend)
            form_set_error('packet-date-start', 'Invalid date range.');
        if ($rstart && $rend && $rstart >= $rend)
            form_set_error('review-date-start', 'Invalid date range.');
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
    $board = \Ebms\Util::get_user_default_board(0);
    $topic = $status = $meeting_type = $disposition = 0;
    $date_start = $date_end = $status_date_start = $status_date_end = '';

    // Check for overrides of the defaults by request parameters
    // stored in the database.
    if ($request_id) {
        $values = EbmsReports::fetch_report_request($request_id);
        $board = $values['board'];
        $topic = $values['topic'];
        $status = $values['status'];
        if (isset($values['disposition']))
            $disposition = $values['disposition'];
        if (isset($values['meeting-type']))
            $meeting_type = $values['meeting-type'];
        if (!empty($values['date-start']['date']))
            $date_start =
                Ebms\Util::fill_in_date($values['date-start']['date']);
        if (!empty($values['date-end']['date']))
            $date_end = Ebms\Util::fill_in_date($values['sdate-end']['date'],
                                                true);
        if (!empty($values['sdate-start']['date']))
            $status_date_start =
                Ebms\Util::fill_in_date($values['sdate-start']['date']);
        if (!empty($values['sdate-end']['date']))
            $status_date_end =
                Ebms\Util::fill_in_date($values['sdate-end']['date'], true);
    }

    // Create the picklists for boards, meeting types, and article states.
    $boards = array(0 => ' ') + Ebms\Util::boards();
    $info = field_info_field('field_event_category');
    $meeting_types = array(0 => ' ') + $info['settings']['allowed_values'];
    $statuses = array(
        0 => '',
        'reviewer decision' => 'Abstract Decision',
        'full text retrieved' => 'Full Text Retrieved',
        'committee decision' => 'Full-Text Decision',
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
    $manager_actions = EbmsReports::manager_actions();
    $review_dispositions = array(0 => ' ');
    $results = db_select('ebms_review_disposition_value', 'v')
        ->fields('v', array('value_id', 'value_name'))
        ->orderBy('v.value_pos')
        ->execute();
    foreach ($results as $result)
        $review_dispositions[$result->value_id] = $result->value_name;
    $board_decisions = array(); // (0 => ' ');
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
        if (!empty($form_state['values']['disposition']))
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
    // Remember which statuses accept multiple dispositions on the form.
    $md_statuses = array('board manager action', 'editorial board decision');
    if (in_array($status, $md_statuses)) {
        if (!is_array($disposition))
            $disposition = $form_state['values']['disposition'] = array();
    }
    elseif (is_array($disposition))
        $disposition = $form_state['values']['disposition'] = 0;
    elseif ($disposition && !array_key_exists($disposition, $dispositions))
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
            '#multiple' => in_array($status, $md_statuses),
            '#attributes' => array(
                'class' => array('fixed-width')
            ),
        );

    // The Review Cycles field is always present, but it's sandwiched
    // between fields which depend on what has been selected for the
    // Status field, and the Drupal field states mechanism is broken,
    // so this field always gets redrawn by the ajax callback for
    // the change of Status.
    // 2019-08-08 (OCEEBMS-459): cycles replaced by status date range.
    $status_date_field = Ebms\Util::date_range_field('Status Date', 'sdate',
                         $status_date_start, $status_date_end);
    $form['top']['report-form']['status-dep']['sdate'] = $status_date_field;

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
        $date_field = Ebms\Util::date_range_field('Meeting Date', 'date',
                                                  $date_start, $date_end);
        $form['top']['report-form']['status-dep']['date'] = $date_field;
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
    if (!empty($values['op'])) {
        if ($values['op'] == 'Submit') {
            if (!$values['board'])
                form_set_error('board', 'No board selected.');
            if (!$values['status'])
                form_set_error('status', 'No status selected.');
        }
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
    $board = \Ebms\Util::get_user_default_board(0);
    $selected_topics = $selected_reviewers = array();
    $start = $end = '';

    // Pull request parameters from the database if appropriate.
    if ($request_id) {
        $values = EbmsReports::fetch_report_request($request_id);
        $board = $values['board'];
        $selected_topics = $values['topics'];
        $selected_reviewers = $values['reviewers'];
        $start = $values['date-start']['date'];
        $end = $values['date-end']['date'];
        if ($start)
            $start = Ebms\Util::fill_in_date($start);
        if ($end)
            $end = Ebms\Util::fill_in_date($end, true);
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
        $query->join('ebms_packet_reviewer', 'r', 'r.reviewer_id = u.uid');
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
        'dates' => Ebms\Util::date_range_field('DATE ASSIGNED', 'date',
                                               $start, $end, '', false),
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
        '#attributes' => array('id' => 'responses-by-reviewer-report-top'),
        'left-nav' => $left_nav,
        'report-form' => $form,
    );

    // If we have a submitted report request, add the report under the form.
    if ($request_id) {
        $query = db_select('ebms_packet', 'p')
            ->fields('p', array('packet_id', 'packet_title', 'created_at'));
        $query = $query->extend('TableSort');
        $query->join('ebms_packet_reviewer', 'r', 'r.packet_id = p.packet_id');
        $query->join('users', 'u', 'u.uid = r.reviewer_id');
        $query->fields('u', array('uid', 'name'));
        if (count($values['topics']))
            $query->condition('p.topic_id', $values['topics'], 'IN');
        else {
            $query->join('ebms_topic', 't', 't.topic_id = p.topic_id');
            $query->condition('t.board_id', $board);
        }
        if (count($values['reviewers']))
            $query->condition('r.reviewer_id', $values['reviewers'], 'IN');
        if ($values['date-start']['date']) {
            $start = Ebms\Util::fill_in_date($values['date-start']['date']);
            $query->condition('p.created_at', $start, '>=');
        }
        if ($values['date-end']['date']) {
            $end = Ebms\Util::fill_in_date($values['date-end']['date'], true);
            $query->condition('p.created_at', $end, '<=');
        }
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
    $board = \Ebms\Util::get_user_default_board(0);
    $selected_topics = /* $selected_cycles = */ array();
    $order_by = 'author';
    $start = $end = '';
    $hi_priority = false;

    // Pull request parameters from the database if appropriate.
    if ($request_id) {
        $values = EbmsReports::fetch_report_request($request_id);
        $board = $values['board'];
        $selected_topics = $values['topics'];
        // $selected_cycles = $values['cycles'];
        $start = $values['date-start']['date'];
        $end = $values['date-end']['date'];
        if ($start)
            $start = Ebms\Util::fill_in_date($start);
        if ($end)
            $end = Ebms\Util::fill_in_date($end, true);
        $order_by = $values['order-by'];
        $hi_priority = $values['options']['hi-priority'];
    }

    // Get the values of interest for ajax callbacks.
    if (isset($form_state['values'])) {
        $board = $form_state['values']['board'];
        $selected_topics = $form_state['values']['topics'];
        /* $selected_cycles = $form_state['values']['cycles']; */
    }

    // How tall should the picklists allowing multiple selections be?
    $size = 6;

    // Create the picklists.
    $topics = array();
    $boards = array(0 => ' ') + Ebms\Util::boards();
    // $cycles = Ebms\Util::cycles();

    // If a board has been selected, populate the board-dependent picklists.
    if ($board) {
        $results = db_select('ebms_topic', 't')
            ->fields('t', array('topic_id', 'topic_name'))
            ->condition('t.board_id', $board)
            ->condition('t.active_status', 'A')
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
        // Suppress filtering this report by cycle (OCEEBMS-362)
        /*
        'cycles' => array(
            '#type' => 'select',
            '#multiple' => true,
            '#options' => $cycles,
            '#default_value' => $selected_cycles,
            '#title' => 'Cycle(s)',
            '#size' => $size,
            '#attributes' => array('class' => array('fixed-width')),
        ),
        */
        'date' => Ebms\Util::date_range_field('Assigned Date', 'date',
                                              $start, $end, '', false),
        'options' => array(
            '#type' => 'checkboxes',
            '#title' => 'Options',
            '#attributes' => array('class' => array('fixed-width')),
            '#options' => array(
                'hi-priority' => 'Only High-Priority Article/Topics',
            ),
            '#default_value' => $hi_priority ? ['hi-priority'] : [],
        ),
        'order-by' => array(
            '#type' => 'radios',
            '#title' => 'Order By',
            '#options' => array(
                'author' => 'First Article Author',
                'topic' => 'Topic',
                'core-journals' => 'Journal (Core Journals First)',
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
        $report = articles_without_responses_report($request_id);
        $id = 'report-title';
        $title = "Articles ($report->total)";
        $opts = array(
            'attributes' => array(
                'target' => '_blank',
                'class' => 'button member-version',
                'style' => 'position: relative; top: -15px; float: right',
            ),
        );
        $link = l('Member Version', "reports/awr-member/$request_id", $opts);
        $page['report'] = array(
            'title' => array(
                '#markup' =>
                "<h2 style='position: relative' id='$id'>$title $link</h2>",
            ),
            'table' => array(
                '#attributes' => array('id' => 'unreviewed-articles-report'),
                '#theme' => 'table',
                '#rows' => $report->rows,
                '#header' => $report->headers,
            ),
        );
    }
    return $page;
}

/**
 * This has been hoisted out so that it can be used by the version shown
 * to board managers as well as the stripped-down version which can be
 * copied to the clipboard and pasted into a Word document.
 */
function articles_without_responses_report($request_id, $member_version=false) {

    // Our return value will be an object.
    $report = new stdClass();
    $report->headers = array('Article', 'Topic', 'Reviewer', 'Assigned');

    // Fetch the settings for this request, use them to find the report data.
    $values = EbmsReports::fetch_report_request($request_id);
    $query = db_select('ebms_article', 'a');
    $query->join('ebms_packet_article', 'pa', 'pa.article_id = a.article_id');
    $query->join('ebms_packet', 'p', 'p.packet_id = pa.packet_id');
    $query->join('ebms_topic', 't', 't.topic_id = p.topic_id');
    $query->join('ebms_packet_reviewer', 'rwr', 'rwr.packet_id = p.packet_id');
    $query->fields('a', array('article_id'));
    $query->fields('p', array('packet_id', 'created_at'));
    $query->fields('t', array('topic_id', 'topic_name'));

    // Show only high-priority article/topics if so requested (OCEEBMS-476).
    if (!empty($values['options']['hi-priority'])) {
        $query->join('ebms_article_tag', 'eat',
                     'eat.article_id = a.article_id AND ' .
                     'eat.topic_id = t.topic_id');
        $query->join('ebms_article_tag_type', 'eatt',
                     'eatt.tag_id = eat.tag_id');
        $query->condition('eat.active_status', 'A');
        $query->condition('eatt.text_id', 'high_priority');
    }

    // Skip articles subsequently dropped from packet (OCEEBMS-357).
    $query->condition('pa.drop_flag', 0);

    // Filter on topics, if any specified, otherwise by the board.
    if (count($values['topics']))
        $query->condition('p.topic_id', $values['topics'], 'IN');
    else
        $query->condition('t.board_id', $values['board']);

    // Exclude articles which have at least one review for the packet.
    $query->leftJoin('ebms_article_review', 'r',
                     'r.article_id = a.article_id AND ' .
                     'p.packet_id = r.packet_id');
    $query->isNull('r.review_id');

    // Filter by date if asked to do so.
    if ($values['date-start']['date']) {
        $start = Ebms\Util::fill_in_date($values['date-start']['date']);
        $query->condition('p.created_at', $start, '>=');
    }
    if ($values['date-end']['date']) {
        $end = Ebms\Util::fill_in_date($values['date-end']['date'], true);
        $query->condition('p.created_at', $end, '<=');
    }

    // Get the first author's (last or collective) name (for sorting).
    $query->leftJoin('ebms_article_author_cite', 'ac',
                     'a.article_id = ac.article_id AND ac.cite_order = 1');
    $query->leftJoin('ebms_article_author', 'aa',
                     'aa.author_id = ac.author_id');
    $query->addExpression('IFNULL(aa.last_name, aa.collective_name)',
                          'author_sort');

    // Apply the requested sorting.
    if ($values['order-by'] == 'topic')
        $query->orderBy('t.topic_name');
    elseif ($values['order-by'] == 'core-journals') {
        $query->leftJoin('ebms_core_journal', 'cj',
                         'cj.source = a.source AND ' .
                         'cj.source_jrnl_id = a.source_jrnl_id');
        $query->addExpression('ISNULL(cj.source_jrnl_id)', 'core');
        $query->orderBy('core');
        $query->orderBy('a.jrnl_title');
    }
    $query->orderBy('aa.last_name');
    $query->orderBy('aa.initials');
    $query->orderBy('aa.collective_name');
    if ($values['order-by'] != 'topic')
        $query->orderBy('t.topic_name');

    // Might not be necessary (see isNull() above), but doesn't hurt.
    $query->distinct();

    // Snag a count of articles in the report.
    // $report->total = $query->countQuery()->execute()->fetchField();
    // No longer using the original query's total. Even though the
    // title of the report implies that we're showing the number of
    // articles which haven't got any reviews, Robin wants the displayed
    // total to avoid including the articles which somehow reached a
    // later stage without any reviews.

    // Find the sequence number for the highest state for which
    // we want to include an article/topic combination on the report.
    $sequence = db_select('ebms_article_state_type', 't')
        ->fields('t', array('sequence'))
        ->condition('t.state_text_id', 'PassedFullReview')
        ->execute()
        ->fetchField();

    // Assemble the rows in the report.
    $report->rows = array();
    $results = $query->execute();
    $reviewer_break = $member_version ? '<br>' : '<br><br>';
    foreach ($results as $result) {

        // Winnow out rows for article/state combinations which have
        // moved on past the review state.
        $query = db_select('ebms_article_state', 's')
            ->condition('s.article_id', $result->article_id)
            ->condition('s.topic_id', $result->topic_id)
            ->condition('s.active_status', 'A');
        $query->join('ebms_article_state_type', 't', 't.state_id = s.state_id');
        $query->condition('t.sequence', $sequence, '>');
        $count = $query->countQuery()->execute()->fetchField();
        if ($count > 0)
            continue;

        // Filter out FYI articles (OCEEBMS-338).
        $query = db_select('ebms_article_state', 's')
            ->condition('s.article_id', $result->article_id)
            ->condition('s.topic_id', $result->topic_id)
            ->condition('s.active_status', 'A');
        $query->join('ebms_article_state_type', 't', 't.state_id = s.state_id');
        $query->condition('t.state_text_id', 'FYI');
        $count = $query->countQuery()->execute()->fetchField();
        if ($count > 0)
            continue;

        // Collect the list of reviewers assigned to the article.
        $query = db_select('users', 'u')->fields('u', array('name'));
        $query->join('ebms_packet_reviewer', 'r', 'r.reviewer_id = u.uid');
        $query->condition('r.packet_id', $result->packet_id);
        $query->orderBy('u.name');
        $reviewer_results = $query->execute();
        $reviewers = array();
        foreach ($reviewer_results as $reviewer_result)
            $reviewers[] = htmlspecialchars($reviewer_result->name);

        // Assemble the report row for this article/topic combo.
        $data = array(
            EbmsReports::format_citation($result->article_id, null,
                $member_version, true, $result->topic_id),
            htmlspecialchars($result->topic_name),
            implode($reviewer_break, $reviewers),
            date('Y-m-d', strtotime($result->created_at)),
        );
        $report->rows[] = array('data' => $data, 'no_striping' => true);
    }

    // Snag a count of articles in the report.
    $report->total = count($report->rows);

    // Return the assembled object.
    return $report;
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
    $query = db_select('users', 'u')
        ->fields('u', array('name', 'login'))
        ->condition('u.status', 1);
    $query->join('users_roles', 'ur', 'ur.uid = u.uid');
    $query->join('role', 'r', 'r.rid = ur.rid');
    $query->condition('r.name', 'board member');
    $query->addExpression('FROM_UNIXTIME(u.login, \'%Y-%m-%d\')', 'last');
    $query->leftJoin('ebms_board_member', 'm', 'm.user_id = u.uid');
    $query->leftJoin('ebms_board', 'b', 'b.board_id = m.board_id');
    $query->leftJoin('authmap', 'a', 'a.uid = u.uid');
    $query->addExpression('IFNULL(b.board_name, \'ZZZ\')', 'board');
    $query->fields('a', array('authname'));
    $query->orderBy('board');
    $query->orderBy('u.name');
    $results = $query->execute();
    $current_board = null;
    $sheet_id = 0;
    foreach ($results as $result) {
        $last_login = $result->login ? $result->last : '';
        $board = $result->board;
        if ($board == 'Integrative, Alternative, and Complementary Therapies')
            $board = 'IACT';
        elseif ($board == 'ZZZ')
            $board = 'NO BOARDS';
        $name = $result->name;
        $authname = $result->authname ? $result->authname : '';
        if ($board != $current_board) {
            $current_board = $board;
            if ($sheet_id)
                $excel->createSheet();
            $excel->setActiveSheetIndex($sheet_id++);
            $sheet = $excel->getActiveSheet();
            $sheet->setTitle($board);
            $sheet->getColumnDimension('A')->setAutoSize(true);
            $sheet->getColumnDimension('B')->setAutoSize(true);
            $sheet->getColumnDimension('C')->setAutoSize(true);
            $sheet->setCellValue('A1', 'Name');
            $sheet->setCellValue('B1', 'Authname');
            $sheet->setCellValue('C1', 'Last Login');
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
                    ),
                    'alignment' => array(
                        'horizontal' =>
                        PHPExcel_Style_Alignment::HORIZONTAL_CENTER,
                    ),
                )
            );
            $row = 2;
        }
        $sheet->setCellValue("A$row", $name);
        $sheet->setCellValue("B$row", $authname);
        $sheet->setCellValue("C$row", $last_login);
        ++$row;
        $sheet->setSelectedCell('A2');
    }
    $excel->setActiveSheetIndex(0);
    $name = 'board-members-last-login-' . strftime('%Y%m%d%H%M%S') . '.xls';
    $writer = PHPExcel_IOFactory::createWriter($excel, 'Excel5');
    drupal_add_http_header('Content-Type',
                           'application/vnd.ms-excel; charset=utf-8');
    drupal_add_http_header('Cache-control',
                           'must-revalidate, post-check=0, pre-check=0');
    drupal_add_http_header('Content-disposition',
                           'attachment;filename=' . $name);
    drupal_add_http_header('Content-transfer-encoding', 'binary');
    $writer->save('php://output');
    exit(0);
}

function pdq_ebms_missing_from_nlm($form, &$form_state, $request_id) {

    // Override the breadcrumbs which Drupal would automatically
    // set based on the menu callback, in order to comply with
    // the customized requirements of the EBMS design.
    drupal_set_breadcrumb(
        array(
            l('Reports', 'reports'),
            l('Citation Management Reports', 'reports/cite'),
            'Invalid Pubmed IDs',
        )
    );

    // Initialize defaults for the form field's values.
    $check_all = false;
    $batch = 10000;
    $delay = 2;

    // Pull request parameters from the database if appropriate.
    if ($request_id) {
        $values = EbmsReports::fetch_report_request($request_id);
        $check_all = $values['check-all'];
        $batch = $values['batch-size'];
        $delay = $values['delay'];
    }

    // Assemble the request form.
    $form = array(
        '#type' => 'container',
        '#attributes' => array('id' => 'report-form'),
        'title' => array('#markup' => '<h2>Invalid Pubmed IDs</h2>'),
        'check-all' => array(
            '#title' => 'Check All PMIDs?',
            '#description' => 'Default is to only check active articles.' .
            ' (Checking all IDs takes a very long time, and may time out.)',
            '#type' => 'checkbox',
            '#default_value' => $check_all,
        ),
        'batch-size' => array(
            '#title' => 'Batch Size',
            '#description' => 'How many IDs to check at one time.',
            '#attributes' => array('class' => array('fixed-width')),
            '#default_value' => $batch,
            '#type' => 'textfield',
        ),
        'delay' => array(
            '#title' => 'Delay',
            '#description' => 'Number of seconds to wait between requests.',
            '#attributes' => array('class' => array('fixed-width')),
            '#default_value' => $delay,
            '#type' => 'textfield',
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
    );

    // Assemble the page where the form lives, including the left nav penel.
    $left_nav = EbmsReports::build_left_nav('Citation Management',
                                            'Invalid Pubmed IDs');
    $page['top'] = array(
        '#type' => 'container',
        '#attributes' => array('id' => 'invalid-pubmed-ids-report-top'),
        'left-nav' => $left_nav,
        'report-form' => $form,
    );

    // If we have a submitted report request, add the report under the form.
    if ($request_id) {
        try {
            $report = EbmsReports::lost_by_nlm(!$check_all, $batch, $delay);
        }
        catch (Exception $e) {
            watchdog_exception('reports', $e);
            drupal_set_message('Unable to retrieve PMID statuses from NLM.',
                'error');
            return $page;
        }
        $checked = $report->checked;
        $missing = $report->missing;
        $count = count($missing);
        if ($check_all)
            $checked = "Checked All $checked IDs";
        else
            $checked = "Checked $checked Active Articles";
        $title = "$count Invalid PMIDs ($checked)";
        $id = 'missing-pmids-title';
        $pmids = array();
        foreach ($missing as $pmid) {
            $ebms_id = db_select('ebms_article', 'a')
                ->fields('a', array('article_id'))
                ->condition('source', 'Pubmed')
                ->condition('source_id', $pmid)
                ->execute()
                ->fetchField();
            if ($ebms_id) {
                $link = l("EBMS ID $ebms_id", "citations/full/$ebms_id");
                $pmids[] = "$pmid ($link)";
            }
            else
                $pmids[] = $pmid;
        }
        $page['report'] = array(
            'title' => array(
                '#markup' =>
                '<h2 style="clear: both; position: relative" ' .
                "id='$id'>$title</h2>",
            ),
            'list' => array(
                '#attributes' => array('id' => 'missing-pmids-report'),
                '#theme' => 'item_list',
                '#items' => $pmids,
            ),
        );
    }
    return $page;
}

function pdq_ebms_missing_from_nlm_submit($form, &$form_state) {
    if ($form_state['values']['op'] == 'Reset')
        drupal_goto('reports/missing-from-nlm');
    $values = $form_state['values'];
    $report = 'Invalid Pubmed IDs';
    $request_id = EbmsReports::save_report_request($values, $report);
    drupal_goto('reports/missing-from-nlm/' . $request_id);
}

function pdq_ebms_recent_activity_form($form, &$form_state, $request_id) {
    // Override the breadcrumbs which Drupal would automatically
    // set based on the menu callback, in order to comply with
    // the customized requirements of the EBMS design.
    drupal_set_breadcrumb(
        array(
            l('Reports', 'reports'),
            l('Board Management Reports', 'reports/board'),
            'Recent Activity',
        )
    );
    $start = $end = '';
    $start = date('Y-m-d', strtotime('-1 year +1 day'));
    $end = date('Y-m-d');
    $boards = Ebms\Util::boards();
    $board = \Ebms\Util::get_user_default_board(0);
    $types = array(
        'Literature',
        'Document',
        'Meeting'
    );

    // Assemble the request form.
    $date_range = Ebms\Util::date_range_field('Date Range', 'date',
                  $start, $end, '', false);
    $date_range['#required'] = true;
    $form = array(
        '#type' => 'container',
        '#attributes' => array('id' => 'report-form'),
        'title' => array('#markup' => '<h2>Recent Activity Report</h2>'),
        'board' => array(
            '#type' => 'checkboxes',
            '#options' => $boards,
            '#required' => true,
            '#default_value' => array($board),
            '#title' => 'Editorial Board',
        ),
        'type' => array(
            '#type' => 'checkboxes',
            '#attributes' => array('class' => array('fixed-width')),
            '#options' => drupal_map_assoc($types),
            '#default_value' => array('Literature', 'Document'),
            '#title' => 'Activity Type',
            '#required' => true,
        ),
        'options' => array(
            '#type' => 'checkboxes',
            '#title' => 'Options',
            '#attributes' => array('class' => array('fixed-width')),
            '#options' => array(
                'group' => 'Group activity by type under each board',
            ),
        ),
        $date_range,
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
    );

    // Assemble the page where the form lives, including the left nav penel.
    $left_nav = EbmsReports::build_left_nav('Board Management',
                                            'Recent Activity');
    $page['top'] = array(
        '#type' => 'container',
        '#attributes' => array('id' => 'recent-activity-report-top'),
        'left-nav' => $left_nav,
        'report-form' => $form,
    );

    return $page;

}

/**
 * Must have at least one board and one activity type, as well as
 * a date range. The form definition takes care of everything but
 * the date range.
 *
 * Requirements change: have to validate sort now, too.
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
function pdq_ebms_recent_activity_form_validate($form, &$form_state) {
    $parts = array('start', 'end');
    $values = $form_state['values'];
    foreach ($parts as $part) {
        if (empty($values["date-$part"]['date']))
            form_set_error("date-$part", "Date  $part required");
        elseif (!Ebms\Util::chkISOdate($values["date-$part"]['date']))
            form_set_error("date-$part", "Malformed date $part");
    }
    $types = $values['type'];
}

/**
 * If the user clicks the "Reset" button, clear the form by routing
 * back to the URL for the report form.  Otherwise create the report
 * directly (without the normal Drupal trimmings, so that it's easier
 * to copy and paste into a word processor).
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
function pdq_ebms_recent_activity_form_submit($form, &$form_state) {
    $values = $form_state['values'];
    if ($values['op'] == 'Reset')
        drupal_goto('reports/recent-activity');
    $start = strtotime($values['date-start']['date']);
    $end = strtotime($values['date-end']['date'] . ' 23:59:59');
    $boards = array();
    $events = array();
    foreach ($values['board'] as $key => $val)
        if ($val) {
            $boards[$val] = Ebms\Util::get_board_name($val);
            $events[$val] = array();
        }
    $types = array();
    $need_messages = false;
    foreach ($values['type'] as $key => $val)
        if ($val) {
            $types[] = $val;
            if ($val != 'Literature')
                $need_messages = true;
        }
    if ($need_messages) {
        $query = new EntityFieldQuery();
        $query->entityCondition('entity_type', 'message')
            ->propertyCondition('timestamp', $start, '>=')
            ->propertyCondition('timestamp', $end, '<');
        $results = $query->execute();
        if (isset($results['message'])) {
            $message_ids = array_keys($results['message']);
            $messages = entity_load('message', $message_ids);
        }
    }
    if (in_array('Literature', $types)) {
        /* Robin changed her mind.
           https://tracker.nci.nih.gov/browse/OCEEBMS-439#comment-626331
        $query = db_select('ebms_packet', 'p');
        $query->join('ebms_topic', 't', 't.topic_id = p.topic_id');
        $query->condition('t.board_id', array_keys($boards), 'IN');
        $query->fields('p', array('created_at', 'packet_title'));
        $query->fields('t', array('board_id', 'topic_name'));
        $query->condition('p.created_at', date('Y-m-d', $start), '>=');
        $query->condition('p.created_at', date('Y-m-d', $end), '<');
        $results = $query->execute();
        foreach ($results as $packet) {
            $title = htmlspecialchars($packet->packet_title);
            $datetime = $packet->created_at;
            $date = substr($datetime, 0, 10);
            $topic = $packet->topic_name;
            $description = "<b>Packet created $date</b> ($title)";
            $event = array($datetime, $description, 'Literature', $topic);
            $events[$packet->board_id][] = $event;
        }
        */
        $query = db_select('ebms_article_review', 'r');
        $query->join('ebms_packet', 'p', 'p.packet_id = r.packet_id');
        $query->join('ebms_topic', 't', 't.topic_id = p.topic_id');
        $query->join('users', 'u', 'u.uid = r.reviewer_id');
        $query->condition('t.board_id', array_keys($boards), 'IN');
        $query->condition('r.when_posted', date('Y-m-d', $start), '>=');
        $query->condition('r.when_posted', date('Y-m-d', $end), '<');
        $query->fields('r', array('review_id', 'article_id', 'when_posted',
                'comments', 'loe_info'));
        $query->fields('t', array('board_id', 'topic_name'));
        $query->fields('u', array('name'));
        $query->fields('p', array('packet_title'));
        $results = $query->execute();
        foreach ($results as $review) {
            $reviewer = htmlspecialchars($review->name);
            $comments = htmlspecialchars($review->comments);
            $loe = htmlspecialchars($review->loe_info);
            $datetime = $review->when_posted;
            $date = substr($datetime, 0, 10);
            $topic = htmlspecialchars($review->topic_name);
            $packet = htmlspecialchars($review->packet_title);
            $query = db_select('ebms_review_disposition_value', 'v');
            $query->join('ebms_review_disposition', 'd',
                'd.value_id = v.value_id');
            $query->condition('d.review_id', $review->review_id);
            $query->orderBy('v.value_pos');
            $query->fields('v', array('value_name'));
            $options = $query->execute()->fetchCol();
            $options = htmlspecialchars(implode('; ', $options));
            $query = db_select('ebms_review_rejection_value', 'v');
            $query->join('ebms_review_rejection_reason', 'r',
                'r.value_id = v.value_id');
            $query->condition('r.review_id', $review->review_id);
            $query->orderBy('v.value_pos');
            $query->fields('v', array('value_name'));
            $reasons = $query->execute()->fetchCol();
            $citation = EbmsReports::format_citation($review->article_id,
                        null, true);
            $description = "<b>Review posted $date</b> by $reviewer<br>" .
                "[Packet: $packet]<br>" .
                $citation . '<br>Disposition: ' . $options;
            if (!empty($reasons)) {
                $reasons = htmlspecialchars(implode('; ', $reasons));
                $description .= "<br>Exclusion Reasons:  $reasons";
            }
            if (!empty($comments))
                $description .= "<br>Comments: $comments";
            if (!empty($loe))
                $description .= "<br>LOE Info: $loe";
            $event = array($datetime, $description, 'Literature', $topic);
            $events[$review->board_id][] = $event;
        }
    }
    if (in_array('Document', $types)) {
        $actions = array('posted', 'returned');
        foreach ($actions as $action) {
            $query = db_select("ebms_summary_{$action}_doc", 's');
            $query->join('ebms_doc', 'd', 'd.doc_id = s.doc_id');
            $query->join('ebms_summary_page', 'p', 'p.page_id = s.page_id');
            $query->join('file_managed', 'f', 'f.fid = d.file_id');
            $query->join('users', 'u', 'u.uid = f.uid');
            $query->condition('d.when_posted', date('Y-m-d', $start), '>=');
            $query->condition('d.when_posted', date('Y-m-d H:i:s', $end), '<=');
            $query->condition('p.board_id', array_keys($boards), 'IN');
            $query->fields('p', array('board_id'));
            $query->fields('u', array('name'));
            $query->fields('f', array('fid', 'filename', 'uri'));
            $query->fields('s', array('notes'));
            $query->fields('d', array('when_posted'));
            $results = $query->execute();
            $attributes = array('attributes' => array('target' => '_blank'));
            foreach ($results as $summary) {
                $poster = htmlspecialchars($summary->name);
                $filename = $summary->filename;
                $date = $summary->when_posted;
                $notes = htmlspecialchars($summary->notes);
                $url = file_create_url($summary->uri);
                $link = l($filename, $url, $attributes);
                $event = "<b>Document posted $date</b> by $poster: $link";
                if (!empty($notes))
                    $event .= " ($notes)";
                $event = array($date, $event, 'Document');
                $events[$summary->board_id][] = $event;
            }
        }
    }
    if (in_array('Meeting', $types)) {
        foreach ($messages as $mid => $message) {
            $when = date('Y-m-d H:i:s', $message->timestamp);
            $wrapper = entity_metadata_wrapper('message', $message);
            $message_boards = EbmsReports::boards_for_message($wrapper);
            $event = null;
            switch ($message->type) {
                case 'activity_new_event':
                    $d = EbmsReports::event_description($wrapper);
                    $event = "<b>Meeting added to calendar $when</b> ($d)";
                    break;
                case 'activity_event_type':
                    $d = EbmsReports::event_description($wrapper);
                    $event = "<b>Meeting type changed $when</b> ($d)";
                    break;
                case 'activity_changed_event':
                    $d = EbmsReports::event_description($wrapper);
                    $event = "<b>Meeting date/time changed $when</b> ($d)";
                    break;
                case 'activity_agenda_published':
                    $d = EbmsReports::event_description($wrapper);
                    $event = "<b>Meeting agenda published $when</b> ($d)";
                    break;

            }
            if (!empty($event)) {
                $event = array($when, $event, 'Meeting');
                foreach ($message_boards as $board_id) {
                    if (array_key_exists($board_id, $events))
                        $events[$board_id][] = $event;
                }
            }
        }
    }
    $body = array();
    $body[] = '<h1>Recent Activity (' . date('Y-m-d', $start) . ' through ' .
        date('Y-m-d', $end) . ')</h1>';
    asort($boards);
    foreach ($boards as $board_id => $board_name) {
        $body[] = "<br><h2>Recent Activity for the $board_name Board</h2>";
        $board_events = $events[$board_id];
        usort($board_events,
            function($a, $b) { return strcmp($b[0], $a[0]); });
        if (!empty($values['options']['group'])) {
            $type_events = array(
                'Literature' => array(),
                'Document' => array(),
                'Meeting' => array()
            );
            foreach ($board_events as $event)
                $type_events[$event[2]][] = $event;
            foreach ($type_events as $type => $event_list) {
                if (!empty($event_list)) {
                    $body[] = "<br><h3>$type Activity</h3><hr>";
                    foreach ($event_list as $event) {
                        $description = $event[1];
                        $body[] = "<p>$description</p>";
                    }
                }
            }
        }
        else {
            foreach ($board_events as $event) {
                $description = $event[1];
                $body[] = "<p>$description</p>";
            }
        }
        if (empty($board_events))
            $body[] = '<p>No activity to report for this board.</p>';
    }
    $css = Ebms\CSS_DIR . '/stripped-report.css';
    $page = array(
        'language' => 'en',
        'head' => '<meta charset="utf-8">',
        'title' => 'Recent Activity',
        'scripts' => '',
        'favicon' => '',
        'css' => "<link rel='stylesheet' href='/$css'>",
        'message' => array(),
        'footer_scripts' => '',
        'body' => implode("\n", $body),
    );
    $html = theme('stripped', array('page' => $page));
    drupal_add_http_header('Content-Type', 'text/html; charset=utf-8');
    drupal_send_headers();
    print $html;
    drupal_exit();
}
