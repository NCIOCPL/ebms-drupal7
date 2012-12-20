<?php

// $Id$

/**
 * @file
 *
 * Implementation of EBMS page for uploading full-text PDFs.
 */
function pdq_ebms_reports($report=null, $request_id=0) {
    $control = new EbmsReports($report, $request_id);
    return $control->run();
}

/*
 * Wrapping as much functionality within a class in order to simplify passing
 * around state information for processing the current request.  Also makes
 * it easier to avoid polluting the global namespace.
 */
class EbmsReports {

    public static $url = 'reports';
    private static $_manager_actions = null;
//  = array(
//         'FullEnd',
//         'NotForAgenda',
//         'AgendaNoPaprChg',
//         'AgendaFutureChg',
//         'AgendaBoardDiscuss',
//         'AgendaWrkGrpDiscuss',
//     );
    public $report, $request_id;

    /**
     * Passing settings in the URL instead of in $_SESSION in order
     * to avoid dependency on Javascript being enabled on the user's
     * browser.
     */
    public function __construct($report, $request_id) {
        $this->report = $report;
        $this->request_id = $request_id;
    }

    /*
     * Show the articles which need full text uploaded.
     */
    public function run() {

        $parms = drupal_get_query_parameters();
        Ebms\Util::add_js();
        drupal_add_js(Ebms\JS_DIR . '/reports.js', 'file');
        Ebms\Menu::$active = 'Reports';
        drupal_add_css(Ebms\CSS_DIR . '/reports.css');
        switch ($this->report) {
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
            case 'board':
                return $this->landing_page('Board Management');
            case 'cite':
                return $this->landing_page('Citation Management');
            case 'documents':
                if (isset($parms['archive'])) {
                    $doc_id = $parms['archive'];
                    db_update('ebms_doc')
                        ->fields(array('drop_flag' => 1))
                        ->condition('doc_id', $doc_id)
                        ->execute();
                    unset($_GET['archive']);
                }
                if (isset($parms['edit'])) {
                    $doc_id = $parms['edit'];
                    unset($_GET['edit']);
                    return drupal_get_form('pdq_ebms_doc_edit_form', $doc_id,
                                           $this->request_id);
                }
                return drupal_get_form('pdq_ebms_documents_report_form',
                    $this->request_id);
            default:
                return $this->landing_page();
        }
    }

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
    public static function format_citation($article_id, $counter=null) {
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

    public static function get_nav_links() {
        global $user;
        if (in_array('medical librarian', $user->roles))
            $role = 'librarian';
        else
            $role = 'manager';
        //$role = 'librarian'; // XXX debugging
        $links = array(
            'librarian' => array(
                array(
                    'Board Management',
                    'reports/board',
                    array(
                        array('Documents', 'reports/documents'),
                        array('Meeting Dates', 'stub'),
                        array('Hotel Requests', 'stub'),
                        array('Reimbursement Requests', 'stub'),
                    ),
                ),
                array(
                    'Citation Management',
                    'reports/cite',
                    array(
                        array('Citation Reports', 'reports/citations'),
                        array('Import Reports', 'reports/import'),
                        array('Not List', 'stub'),
                        array('Yes List', 'stub'),
                    ),
                ),
            ),
            'manager' => array(
                array(
                    'Board Management',
                    'reports/board',
                    array(
                        array('Documents', 'reports/documents'),
                        array('Meeting Dates', 'stub'),
                        array('Hotel Requests', 'stub'),
                        array('Reimbursement Requests', 'stub'),
                    ),
                ),
                array(
                    'Citation Management',
                    'reports/cite',
                    array(
                        array('Import Reports', 'reports/import'),
                        array('Citations By Status', 'reports/cites-status'),
                        array('Literature Reviews', 'reports/lit-review'),
                        array('No Responses', 'stub'),
                        array('Citation Follow-up', 'stub'),
                        array('Board and Topic Lists', 'stub'),
                    ),
                ),
            ),
        );
        return $links[$role];
    }

    /**
     * Creates the render array for the left nav panel for Citation
     * Management page.  Cloned from common.inc, where I had originally
     * placed this functionality, not realizing that the left nav block
     * was going to be different for every page.
     *
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

    /*
     * This version mimics the legacy system's behavior, showing rows
     * for topics for which no articles were published.
     */
    public static function cites_published_0($boards, $board, $cycle) {
        if ($board)
            $boards = array($board => $boards[$board]);
        $query = db_select('ebms_cycle', 'c')
            ->fields('c', array('cycle_name', 'start_date'))
            ->condition('c.cycle_id', $cycle);
        $query->addExpression(
            'DATE_ADD(c.start_date, INTERVAL 1 MONTH)',
            'end_date'
        );
        $results = $query->execute();
        $cycle_name = null;
        foreach ($results as $result) {
            $date_range = array($result->start_date, $result->end_date);
            $cycle_name = $result->cycle_name;
        }
        if (!$cycle_name) {
            drupal_set_message('Cycle is required for this report', 'error');
            return array();
        }
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
                "<div class='meta'><span class='label'>Date:</span>$d</div>" .
                "<div class='meta'><span class='label'>Review cycle:</span>" .
                $cycle_name . '</div>',
            )
        );
        $published = db_select('ebms_article_state_type', 't')
            ->fields('t', array('state_id'))
            ->condition('t.state_text_id', 'Published')
            ->execute()
            ->fetchField();
        $query = db_select('ebms_article_state', 's');
        $query->fields('s', array('topic_id'));
        $query->condition('s.board_id', array_keys($boards), 'IN');
        $query->condition('s.status_dt', $date_range, 'BETWEEN');
        $query->condition('s.state_id', $published);
        $query->groupBy('s.topic_id');
        $query->addExpression('COUNT(*)', 'n');
        $results = $query->execute();
        $stats = array();
        foreach ($results as $result)
            $stats[$result->topic_id] = $result->n;
        foreach ($boards as $board_id => $board_name) {
            if ($board_id) {
                $results = db_select('ebms_topic', 't')
                    ->fields(
                        't',
                        array('topic_id', 'topic_name', 'active_status')
                    )
                    ->condition('t.board_id', $board_id)
                    ->orderBy('t.topic_name')
                    ->execute();
                $rows = array();
                $total = 0;
                foreach ($results as $result) {
                    $id = $result->topic_id;
                    $name = $result->topic_name;
                    $status = $result->active_status;
                    $count = 0;
                    if (array_key_exists($id, $stats)) {
                        $count = $stats[$id];
                        $total += $count;
                    }
                    if ($count || $status == 'A')
                        $rows[] = array(
                            'data' => array($name, $count),
                            'no_striping' => false,
                        );
                }
                $report[$board_name] = array(
                    '#theme' => 'table',
                    '#caption' => "$board_name (Total $total)",
                    '#header' => array('Topic Name', 'Articles Published'),
                    '#rows' => $rows,
                );
            }
        }
        return $report;
    }
    public static function cites_published($boards, $board, $cycle) {
        $cycle_name = Ebms\Util::get_cycle_name($cycle);
        if (!$cycle_name) {
            drupal_set_message('Cycle is required for this report', 'error');
            return array();
        }
        if ($board)
            $boards = array($board => $boards[$board]);
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
        $published = db_select('ebms_article_state_type', 't')
            ->fields('t', array('state_id'))
            ->condition('t.state_text_id', 'Published')
            ->execute()
            ->fetchField();
        $query = db_select('ebms_article_state', 's');
        $query->join('ebms_board', 'b', 'b.board_id = s.board_id');
        $query->join('ebms_topic', 't', 't.topic_id = s.topic_id');
        $query->fields('b', array('board_name'));
        $query->fields('t', array('topic_name'));
        $query->condition('s.board_id', array_keys($boards), 'IN');
        if (!Ebms\Util::add_cycle_to_query($query, $cycle, 's.status_dt')) {
            drupal_set_message('Failure restricting query by cycle', 'error');
            return array();
        }
        $query->condition('s.state_id', $published);
        $query->groupBy('b.board_name');
        $query->groupBy('t.topic_name');
        $query->addExpression('COUNT(*)', 'n');
        $query->orderBy('b.board_name');
        $query->orderBy('t.topic_name');
        $results = $query->execute();
        $stats = array();
        foreach ($results as $result) {
            $board_name = $result->board_name;
            if (!array_key_exists($board_name, $stats))
                $stats[$board_name] = array();
            $stats[$board_name][$result->topic_name] = $result->n;
        }
        foreach ($stats as $board_name => &$topic_array) {
            $rows = array();
            $total = 0;
            foreach ($topic_array as $topic_name => $count) {
                $rows[] = array($topic_name, $count);
                $total += $count;
            }
            $rows[] = array('TOTAL', $total);
            $report[$board_name] = array(
                '#theme' => 'table',
                '#caption' => $board_name,
                '#header' => array('Topic Name', 'Articles Published'),
                '#rows' => $rows,
            );
        }
        return $report;
    }
    public static function cites_imported($boards, $board, $cycle) {
        $cycle_name = Ebms\Util::get_cycle_name($cycle);
        if (!$cycle_name) {
            drupal_set_message('Cycle is required for this report', 'error');
            return array();
        }
        if ($board)
            $boards = array($board => $boards[$board]);
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
        $published = db_select('ebms_article_state_type', 't')
            ->fields('t', array('state_id'))
            ->condition('t.state_text_id', 'ReadyInitReview')
            ->execute()
            ->fetchField();
        $query = db_select('ebms_article_state', 's');
        $query->join('ebms_board', 'b', 'b.board_id = s.board_id');
        $query->join('ebms_topic', 't', 't.topic_id = s.topic_id');
        $query->fields('b', array('board_name'));
        $query->fields('t', array('topic_name'));
        $query->condition('s.board_id', array_keys($boards), 'IN');
        if (!Ebms\Util::add_cycle_to_query($query, $cycle, 's.status_dt')) {
            drupal_set_message('Failure restricting query by cycle', 'error');
            return array();
        }
        $query->condition('s.state_id', $published);
        $query->groupBy('b.board_name');
        $query->groupBy('t.topic_name');
        $query->addExpression('COUNT(*)', 'n');
        $query->orderBy('b.board_name');
        $query->orderBy('t.topic_name');
        $results = $query->execute();
        $stats = array();
        foreach ($results as $result) {
            $board_name = $result->board_name;
            if (!array_key_exists($board_name, $stats))
                $stats[$board_name] = array();
            $stats[$board_name][$result->topic_name] = $result->n;
        }
        foreach ($stats as $board_name => &$topic_array) {
            $rows = array();
            $total = 0;
            foreach ($topic_array as $topic_name => $count) {
                $rows[] = array($topic_name, $count);
                $total += $count;
            }
            $rows[] = array('TOTAL', $total);
            $report[$board_name] = array(
                '#theme' => 'table',
                '#caption' => $board_name,
                '#header' => array('Topic Name', 'Articles Imported'),
                '#rows' => $rows,
            );
        }
        return $report;
    }
    public static function initial_decision($boards, $board, $cycle) {
        $cycle_name = Ebms\Util::get_cycle_name($cycle);
        if (!$cycle_name) {
            drupal_set_message('Cycle is required for this report', 'error');
            return array();
        }
        if ($board)
            $boards = array($board => $boards[$board]);
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
        $rejected = db_select('ebms_article_state_type', 't')
            ->fields('t', array('state_id'))
            ->condition('t.state_text_id', 'RejectInitReview')
            ->execute()
            ->fetchField();
        $accepted = db_select('ebms_article_state_type', 't')
            ->fields('t', array('state_id'))
            ->condition('t.state_text_id', 'PassedInitReview')
            ->execute()
            ->fetchField();
        $state_ids = array($rejected, $accepted);
        $query = db_select('ebms_article_state', 's');
        $query->join('ebms_board', 'b', 'b.board_id = s.board_id');
        $query->join('ebms_topic', 't', 't.topic_id = s.topic_id');
        $query->fields('b', array('board_name'));
        $query->fields('t', array('topic_name'));
        $query->fields('s', array('state_id'));
        $query->condition('s.board_id', array_keys($boards), 'IN');
        if (!Ebms\Util::add_cycle_to_query($query, $cycle, 's.status_dt')) {
            drupal_set_message('Failure restricting query by cycle', 'error');
            return array();
        }
        $query->condition('s.state_id', $state_ids, 'IN');
        $query->groupBy('b.board_name');
        $query->groupBy('t.topic_name');
        $query->groupBy('s.state_id');
        $query->addExpression('COUNT(*)', 'n');
        $query->orderBy('b.board_name');
        $query->orderBy('t.topic_name');
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
        foreach ($stats as $board_name => &$topic_array) {
            $rows = array();
            $total_rejected = $total_accepted = 0;
            foreach ($topic_array as $topic_name => &$n) {
                $rows[] = array($topic_name, $n[$rejected], $n[$accepted]);
                $total_rejected += $n[$rejected];
                $total_accepted += $n[$accepted];
            }
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
    static function format_report_date() {
        $d = date('m/d/Y');
        return '<div class="meta"><span class="label">Report Date:</span> ' .
            $d . '</div>';
    }
    static function format_board($board_name) {
        return '<div class="meta"><span class="label">Board:</span> '.
            $board_name . '</div>';
    }
    static function format_cycle_name($cycle) {
        $cycle_name = Ebms\Util::get_cycle_name($cycle);
        return '<div class="meta"><span class="label">Review Cycle:</span> ' .
            $cycle_name . '</div>';
    }
    static function format_date($date) {
        $y = $date['year'];
        $m = $date['month'];
        $d = $date['day'];
        if (!$y || !$m || !$d)
            return '';
        $t = mktime(0, 0, 0, $m, $d, $y);
        return date('m/d/Y', $t);
    }
    static function format_date_range($start, $end) {
        $s = EbmsReports::format_date($start);
        $e = EbmsReports::format_date($end);
        if (!$s || !$e)
            return '';
        return '<div class="meta"><span class="label">Date Range:</span> ' .
            $s . ' - ' . $e . '</div>';
    }
    public static function manager_rejection($board, $cycle, $start, $end)
    {
        $pubmed_base = 'http://www.ncbi.nlm.nih.gov/entrez/query.fcgi?' .
            'cmd=Retrieve&db=PubMed&dopt=abstract&list_uids=';
        $pubmed_opts = array('attributes' => array('target' => '_blank'));
        $title = '<h2>Citations Not Selected for Full Text Retrieval</h2>';
        $title .= EbmsReports::format_report_date();
        $boards = Ebms\Util::boards();
        if ($board) {
            $board_name = $boards[$board];
            $boards = array($board => $board_name);
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
        $state_id = db_select('ebms_article_state_type', 't')
            ->fields('t', array('state_id'))
            ->condition('t.state_text_id', 'RejectBMReview')
            ->execute()
            ->fetchField();
        $header = array(
            'EBMS ID',
            'PMID',
            array('data' => 'Topic(s)', 'class' => array('col-3')),
            array('data' => 'NCI Reviewer', 'class' => array('col-4')),
            'Comments',
        );
        $board_ids = array_keys($boards);
        $query = db_select('ebms_article_state', 's');
        $query->join('ebms_topic', 't', 't.topic_id = s.topic_id');
        $query->join('ebms_article', 'a', 'a.article_id = s.article_id');
        $query->join('users', 'u', 's.user_id = u.uid');
        $query->fields('a', array('article_id', 'source_id'));
        $query->fields('t', array('topic_name'));
        $query->fields('u', array('name'));
        $query->fields('s', array('article_state_id'));
        $query->condition('s.board_id', $board_ids, 'IN');
        if ($cycle) {
            if (!Ebms\Util::add_cycle_to_query($query, $cycle,
                    's.status_dt')) {
                drupal_set_message('Failure restricting query by cycle',
                    'error');
                return array();
            }
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
    public static function topic_changes($start, $end)
    {
        $pubmed_base = 'http://www.ncbi.nlm.nih.gov/entrez/query.fcgi?' .
            'cmd=Retrieve&db=PubMed&dopt=abstract&list_uids=';
        $pubmed_opts = array('attributes' => array('target' => '_blank'));
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
        $librarian_approval = db_select('ebms_article_state_type', 't')
            ->fields('t', array('state_id'))
            ->condition('t.state_text_id', 'PassedInitReview')
            ->execute()
            ->fetchField();
        $manager_approval = db_select('ebms_article_state_type', 't')
            ->fields('t', array('state_id'))
            ->condition('t.state_text_id', 'PassedBMReview')
            ->execute()
            ->fetchField();
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

    public static function import_reports(
        $board, $topic, $cycle, $start, $end, $range
    ) {
        $query = db_select('ebms_import_batch', 'j');
        $query->join('ebms_import_action', 'a',
            'a.import_batch_id = j.import_batch_id');
        $query->join('ebms_topic', 't', 't.topic_id = j.topic_id');
        $query->join('ebms_board', 'b', 'b.board_id = t.board_id');
        if ($board)
            $query->condition('b.board_id', $board);
        if ($topic)
            $query->condition('t.topic_id', $topic);
        if ($cycle) {
            if (!Ebms\Util::add_cycle_to_query($query, $cycle,
                    'j.import_date')) {
                drupal_set_message('Failure restricting query by cycle',
                    'error');
                return array();
            }
        }
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
        //pdq_ebms_debug('IMPORT REPORT QUERY', 'query: ' . $query);
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
    public static function import_job($id) {
        require_once('EbmsImport.inc');
        $batch = new Ebms\ImportBatch($id);
        //pdq_ebms_debug('IMPORT BATCH', $batch);
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

    //public static function docs($board, $tag, $keyword, $inhouse,
    //    $member, $archived, $range, $start, $end) {
    public static function docs($request_id) {

        $board = $tag = $keyword = $inhouse = $member = $archived =
            $range = null;
        $start = $end = Ebms\Util::empty_date();
        if ($request_id) {
            $params = EbmsReports::fetch_report_request($request_id);
            $board = $params['board'];
            $tag = $params['tag'];
            $keyword = $params['keyword'];
            $inhouse = $params['inhouse'];
            $member = $params['member'];
            $archived = $params['archived'];
            $start = $params['upload-start'];
            $range = $params['use-upload-date-range'];
            if ($range)
                $end = $params['upload-end'];
            pdq_ebms_debug('DOCS PARAMS', $params);
        }
        $pager = true;
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
        $query = db_select('ebms_doc', 'd');
        $query = $query->extend('TableSort');
        if ($pager)
            $query = $query->extend('PagerDefault')->limit(5);
        $query->join('file_managed', 'f', 'f.fid = d.file_id');
        $query->join('users', 'u', 'u.uid = f.uid');
        $query->fields('f', array('filename'));
        $query->fields('d', array('when_posted', 'doc_id', 'drop_flag'));
        $query->fields('u', array('name'));
        if (!$archived)
            $query->condition('d.drop_flag', 0);
        if ($board) {
            $query->join('ebms_doc_board', 'b', 'b.doc_id = d.doc_id');
            $query->condition('b.board_id', $board);
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
        $docs = $query->execute();

        // Collect information we'll use repeatedly in the loop.
        $divider = '<span class="action-divider">|</span>';
        $main_url = 'reports/documents';
        if ($request_id)
            $main_url .= "/$request_id";
        $edit_parms = drupal_get_query_parameters();

        // Build the array of table rows.
        $rows = array();
        foreach ($docs as $d) {
            $edit_parms['edit'] = $d->doc_id;
            $links = l('EDIT', $main_url, array('query' => $edit_parms));
            if (!$d->drop_flag) {
                $archive_link = l('ARCHIVE', "$main_url/a{$d->doc_id}");
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
            $data = array(
                array(
                    'data' => htmlspecialchars($d->filename),
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
        return $table;
    }

    static function cites_by_status_rd($params) {
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
            'Date Reviewed',
            'Reviewer',
        );
        $passed = db_select('ebms_article_state_type', 't')
            ->fields('t', array('state_id'))
            ->condition('t.state_text_id', 'PassedBMReview')
            ->execute()
            ->fetchField();
        $rejected = db_select('ebms_article_state_type', 't')
            ->fields('t', array('state_id'))
            ->condition('t.state_text_id', 'RejectBMReview')
            ->execute()
            ->fetchField();
        $query = db_select('ebms_article_state', 's');
        $query = $query->extend('TableSort');
        $query->join('ebms_topic', 't', 't.topic_id = s.topic_id');
        $query->join('users', 'u', 'u.uid = s.user_id');
        $query->leftJoin('ebms_article_author_cite', 'c',
            'c.article_id = s.article_id AND c.cite_order = 1');
        $query->leftJoin('ebms_article_author', 'a',
            'a.author_id = c.author_id');
        $query->condition('t.board_id', $params['board']);
        if ($params['topic'])
            $query->condition('s.topic_id', $params['topic']);
        $query->condition('s.current', 'Y');
        switch ($params['disposition']) {
        case 'yes':
            $query->condition('s.state_id', $passed);
            break;
        case 'no':
            $query->condition('s.state_id', $rejected);
            break;
        default:
            $query->condition('s.state_id', array($passed, $rejected), 'IN');
            break;
        }
        if ($params['cycle'])
            Ebms\Util::add_cycle_to_query($query, $params['cycle'],
                                          's.status_dt');
        $query->fields('s', array('article_id', 'topic_id', 'article_state_id',
                                  'state_id', 'status_dt'));
        $query->fields('u', array('name'));
        $query->orderByHeader($headers);
        $query->distinct();
        $results = $query->execute();
        $counter = 0;
        $rows = array();
        foreach ($results as $result) {
            $article_id = $result->article_id;
            $article_state_id = $result->article_state_id;
            $state_id = $result->state_id;
            $status_dt = strtotime($result->status_dt);
            $reviewer = htmlspecialchars($result->name);
            $disposition = $state_id == $passed ? 'Yes' : 'No';
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
                $disposition,
                $comments,
                date('Y/m', $status_dt),
                date('m/d/Y', $status_dt),
                $reviewer,
            );
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
    static function cites_by_status_cd($params) {
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
            'Date Reviewed',
        );
        $passed = db_select('ebms_article_state_type', 't')
            ->fields('t', array('state_id'))
            ->condition('t.state_text_id', 'PassedFullReview')
            ->execute()
            ->fetchField();
        $rejected = db_select('ebms_article_state_type', 't')
            ->fields('t', array('state_id'))
            ->condition('t.state_text_id', 'RejectFullReview')
            ->execute()
            ->fetchField();
        $fyi = db_select('ebms_article_state_type', 't')
            ->fields('t', array('state_id'))
            ->condition('t.state_text_id', 'FYI')
            ->execute()
            ->fetchField();
        $query = db_select('ebms_article_state', 's');
        $query = $query->extend('TableSort');
        $query->join('ebms_topic', 't', 't.topic_id = s.topic_id');
        $query->leftJoin('ebms_article_author_cite', 'c',
            'c.article_id = s.article_id AND c.cite_order = 1');
        $query->leftJoin('ebms_article_author', 'a',
            'a.author_id = c.author_id');
        $query->condition('t.board_id', $params['board']);
        if ($params['topic'])
            $query->condition('s.topic_id', $params['topic']);
        $query->condition('s.current', 'Y');
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
            break;
        }
        if ($params['cycle'])
            Ebms\Util::add_cycle_to_query($query, $params['cycle'],
                                          's.status_dt');
        $query->fields('s', array('article_id', 'article_state_id',
                                  'state_id', 'status_dt'));
        $query->orderByHeader($headers);
        $query->distinct();
        $results = $query->execute();
        $counter = 0;
        $rows = array();
        foreach ($results as $result) {
            $article_id = $result->article_id;
            $article_state_id = $result->article_state_id;
            $state_id = $result->state_id;
            $status_dt = strtotime($result->status_dt);
            switch ($state_id) {
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
                $disposition,
                $comments,
                date('Y/m', $status_dt),
                date('m/d/Y', $status_dt),
            );
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
    static function cites_by_status_ftr($params) {
        $headers = array(
            '',
            array(
                'data' => 'Citation',
                'field' => 'aa.last_name',
                'class' => array('col-1'),
            ),
        );
        $passed_bm_review = db_select('ebms_article_state_type', 't')
            ->fields('t', array('state_id'))
            ->condition('t.state_text_id', 'PassedBMReview')
            ->execute()
            ->fetchField();
        $query = db_select('ebms_article_state', 's');
        $query = $query->extend('TableSort');
        $query->condition('s.current', 'Y');
        $query->condition('s.state_id', $passed_bm_review);
        $query->fields('s', array('article_id', 'status_dt'));
        if ($params['topic'])
            $query->condition('s.topic_id', $params['topic']);
        $query->join('ebms_topic', 't', 't.topic_id = s.topic_id');
        $query->condition('t.board_id', $params['board']);
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
        if ($params['cycle'])
            Ebms\Util::add_cycle_to_query($query, $params['cycle'],
                                          's.status_dt');
        $query->leftJoin('ebms_article_author_cite', 'c',
            'c.article_id = s.article_id AND c.cite_order = 1');
        $query->leftJoin('ebms_article_author', 'aa',
            'aa.author_id = c.author_id');

        $query->orderByHeader($headers);
        $query->distinct();
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
            $row[] = date('Y/m', $status_dt);
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

    static function cites_by_status_afr($params) {
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
        $passed_full_review = db_select('ebms_article_state_type', 't')
            ->fields('t', array('state_id'))
            ->condition('t.state_text_id', 'PassedFullReview')
            ->execute()
            ->fetchField();
        $query = db_select('ebms_article_state', 's');
        $query = $query->extend('TableSort');
        $query->condition('s.current', 'Y');
        $query->condition('s.state_id', $passed_full_review);
        $query->fields('s', array('article_id', 'status_dt'));
        if ($params['topic'])
            $query->condition('s.topic_id', $params['topic']);
        $query->join('ebms_topic', 't', 't.topic_id = s.topic_id');
        $query->condition('t.board_id', $params['board']);
        if ($params['cycle'])
            Ebms\Util::add_cycle_to_query($query, $params['cycle'],
                                          's.status_dt');
        $query->leftJoin('ebms_article_author_cite', 'c',
            'c.article_id = s.article_id AND c.cite_order = 1');
        $query->leftJoin('ebms_article_author', 'aa',
            'aa.author_id = c.author_id');
        $query->join('ebms_packet_article', 'a',
            'a.article_id = s.article_id');
        $query->condition('a.drop_flag', 0);
        $query->join('ebms_packet', 'p',
            'p.packet_id = a.packet_id AND p.topic_id = s.topic_id');
        $query->leftJoin('ebms_article_review', 'r',
            'r.packet_id = p.packet_id AND r.article_id = a.article_id');
        $query->isNull('r.article_id');
        $query->fields('p', array('packet_id', 'created_at'));
        $query->orderByHeader($headers);
        $query->distinct();
        $results = $query->execute();
        $articles = array();
        foreach ($results as $result) {
            $article_id = $result->article_id;
            if (!array_key_exists($article_id, $articles))
                $articles[$article_id] = array(
                    'cycle' => date('Y/m', strtotime($result->status_dt)),
                    'packets' => array(),
                );
            $articles[$article_id]['packets'][] = array(
                $result->packet_id,
                date('m/d/y', strtotime($result->created_at)),
            );
        }
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

    static function cites_by_status_bmr($params) {
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

        $passed_full_review = db_select('ebms_article_state_type', 't')
            ->fields('t', array('state_id'))
            ->condition('t.state_text_id', 'PassedFullReview')
            ->execute()
            ->fetchField();
        $query = db_select('ebms_article_state', 's');
        $query = $query->extend('TableSort');
        $query->condition('s.current', 'Y');
        $query->condition('s.state_id', $passed_full_review);
        $query->fields('s', array('article_id', 'status_dt'));
        if ($params['topic'])
            $query->condition('s.topic_id', $params['topic']);
        $query->join('ebms_topic', 't', 't.topic_id = s.topic_id');
        $query->condition('t.board_id', $params['board']);
        if ($params['cycle'])
            Ebms\Util::add_cycle_to_query($query, $params['cycle'],
                                          's.status_dt');
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
        $results = $query->execute();
        $articles = array();
        foreach ($results as $result) {
            $article_id = $result->article_id;
            if (!array_key_exists($article_id, $articles))
                $articles[$article_id] = array(
                    'cycle' => date('Y/m', strtotime($result->status_dt)),
                    'reviews' => array(),
                );
            $articles[$article_id]['reviews'][] = $result->review_id;
        }
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
//         $query = db_select('ebms_review_disposition', 'd');
//         $query = $query->extend('TableSort');
//         $query->join('ebms_article_review', 'r', 'r.review_id = d.review_id');
//         $query->join('ebms_packet', 'p', 'p.packet_id = r.packet_id');
//         $query->join('ebms_topic', 't', 't.topic_id = p.topic_id');
//         $query->leftJoin('ebms_article_author_cite', 'c',
//             'c.article_id = r.article_id AND c.cite_order = 1');
//         $query->leftJoin('ebms_article_author', 'a',
//             'a.author_id = c.author_id');
//         $query->fields('r', array('review_id', 'when_posted', 'comments',
//                        'article_id'));
//         $query->fields('p', array('created_at'));
//         $query->fields('u', array('name'));
//         $query->condition('t.board_id', $params['board']);
//         if ($params['topic'])
//             $query->condition('t.topic_id', $params['topic']);
//         if ($params['disposition']) {
//             $no_change = db_select('ebms_review_disposition_value', 'v')
//                 ->fields('v', array('value_id'))
//                 ->condition('v.value_pos', 1)
//                 ->execute()
//                 ->fetchField();
//             switch ($params['disposition']) {
//             case 'yes':
//                 $query->condition('d.value_id', $no_change, '<>');
//                 break;
//             case 'no':
//                 $query->condition('d.value_id', $no_change);
//                 break;
//             }
//         }

//         // XXX Is cycle for packet creation or review? Robin: packet creation.
//         if ($params['cycle'])
//             Ebms\Util::add_cycle_to_query($query, $params['cycle'],
//                                           'p.created_at');

//         // XXX Need logic for meeting type/meeting date.  Robin: ignore.
//         $query->fields('r', array('article_id'));
//         //$query->orderBy('r.article_id');
//         //$query->orderBy('r.when_posted');
//         $query->orderByHeader($headers);
//         $query->distinct();
//         $results = $query->execute();
//         $articles = array();
//         foreach ($results as $result) {
//             $article_id = $result->article_id;
//             if (!array_key_exists($article_id, $articles))
//                 $articles[$article_id] = array();
//             $articles[$article_id] = $result;
//         }
//         $rows = array();
//         foreach ($articles as $article_id => $reviews) {
//             $citation_shown = false;
//             foreach ($reviews as $review) {
//                 $row = array();
//                 if (!$citation_shown) {
//                 }
//             }
//         }
//     }

    static function cites_by_status_bma($params) {
        $manager_actions = EbmsReports::manager_actions();
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
        // $headers[] = 'Review Cycle'; XXX wait for article/topic table.
        $headers[] = 'Date Assigned';
        $query = db_select('ebms_article_state', 's');
        $query = $query->extend('TableSort');
        $query->join('ebms_topic', 't', 't.topic_id = s.topic_id');
        $query->leftJoin('ebms_article_author_cite', 'c',
            'c.article_id = s.article_id AND c.cite_order = 1');
        $query->leftJoin('ebms_article_author', 'a',
            'a.author_id = c.author_id');
        $query->condition('t.board_id', $params['board']);
        if ($params['topic'])
            $query->condition('s.topic_id', $params['topic']);
        $query->condition('s.current', 'Y');
        if ($params['disposition'])
            $query->condition('s.state_id', $params['disposition']);
        else
            $query->condition('s.state_id',
                array_keys($manager_actions), 'IN');
        if ($params['cycle'])
            Ebms\Util::add_cycle_to_query($query, $params['cycle'],
                                          's.status_dt');
        $query->fields('s', array('article_id', 'article_state_id',
                                  'state_id', 'status_dt'));
        $query->orderByHeader($headers);
        $query->distinct();
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
            $row[] = implode('<br />', $comments);
            $status_dt = strtotime($result->status_dt);
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

    static function cites_by_status_ebd($params) {
        $headers = array(
            '',
            array(
                'data' => 'Citation',
                'field' => 'a.last_name',
                'class' => array('col-1'),
            ),
            'Disposition',
            'Comments',
            // 'Review Cycle', XXX wait for article/topic table
            'Date Assigned',
            // 'Discussed By', XXX find out where this comes from!
        );

        $final_board_decision = db_select('ebms_article_state_type', 't')
            ->fields('t', array('state_id'))
            ->condition('t.state_text_id', 'FinalBoardDecision')
            ->execute()
            ->fetchField();
        $query = db_select('ebms_article_state', 's');
        $query = $query->extend('TableSort');
        $query->join('ebms_topic', 't', 't.topic_id = s.topic_id');
        $query->leftJoin('ebms_article_author_cite', 'c',
            'c.article_id = s.article_id AND c.cite_order = 1');
        $query->leftJoin('ebms_article_author', 'a',
            'a.author_id = c.author_id');
        $query->condition('t.board_id', $params['board']);
        if ($params['topic'])
            $query->condition('s.topic_id', $params['topic']);
        $query->condition('s.current', 'Y');
        $query->condition('s.state_id', $final_board_decision);
        if ($params['disposition']) {
            $query->join('ebms_article_board_decision', 'd',
                'd.article_state_id = s.article_state_id');
            $query->condition('d.decision_value_id', $params['disposition']);
        }
        if ($params['cycle'])
            Ebms\Util::add_cycle_to_query($query, $params['cycle'],
                                          's.status_dt');
        $query->fields('s',
            array('article_id', 'article_state_id', 'status_dt'));
        $query->orderByHeader($headers);
        $query->distinct();
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
            $col4 = date('m/d/Y', strtotime($result->status_dt));
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
            );
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

    // XXX do the statuses need to be current?  Yes.
    public static function cites_by_status($request_id) {
        $params = EbmsReports::fetch_report_request($request_id);
        pdq_ebms_debug('CITE BY STATUS REPORT TABLE', $params);
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
            case 'editorial board decision':
                return EbmsReports::cites_by_status_ebd($params);
            case 'on agenda':
            default:
                return array();
        }
        $query = db_select('ebms_article_state', 's');
        $query->condition('s.board_id', $params['board']);
        if ($params['topic'])
            $query->condition('s.topic_id', $params['topic']);
            $citation = EbmsReports::format_citation($article_id);

    }

    public static function reviews($request_id) {
        $params = EbmsReports::fetch_report_request($request_id);
        pdq_ebms_debug('REVIEWS TABLE PARAMS', $params);
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
        $query = db_select('ebms_article_review', 'r');
        $query = $query->extend('TableSort');
        $query->join('users', 'u', 'u.uid = r.reviewer_id');
        $query->join('ebms_packet', 'p', 'p.packet_id = r.packet_id');
        $query->join('ebms_topic', 't', 't.topic_id = p.topic_id');
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
            Ebms\Util::add_cycle_to_query($query, $cycle, 'p.created_at');
        if ($packet)
            $query->condition('p.packet_title', $packet);
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
        //$query->range(0, 25);
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
                'data' => 'REVIEWER | DISPOSITION',
                'class' => array('col-3'),
            ),
            array(
                'data' => 'COMMENTS',
                'class' => array('col-4'),
            ),
        );
        $query->orderByHeader($headers);
        $results = $query->execute();
        $articles = array();
        foreach ($results as $result) {
            $article_id = $result->article_id;
            $review_id = $result->review_id;
            $packet = date('Y/m', strtotime($result->created_at));
            $reviewer = $result->name;
            if (!array_key_exists($article_id, $articles))
                $articles[$article_id] = array();
            if (!array_key_exists($packet, $articles[$article_id]))
                $articles[$article_id][$packet] = array();
            if (!array_key_exists($reviewer, $articles[$article_id][$packet]))
                $articles[$article_id][$packet][$reviewer] = array();
            $articles[$article_id][$packet][$reviewer][] = $review_id;
        }
        $counter = 0;
        $rows = array();

        // Look up the ID of the disposition for rejecting the article
        // (always in the first position).
        $no_change_option = db_select('ebms_review_disposition_value', 'v')
            ->fields('v', array('value_id'))
            ->condition('v.value_pos', 1)
            ->execute()
            ->fetchField();
        foreach ($articles as $article_id => $packets) {
            $citation = EbmsReports::format_citation($article_id);
            $query = db_select('ebms_article_review', 'r');
            $article_rowspan = 0;
            ksort($packets);
            $packet_rows = array();
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
            $have_abstract_row = false;
            //pdq_ebms_debug('PACKET ROWS', $packet_rows);
            foreach ($packet_rows as $packet => $reviews) {
                $have_packet_row = false;
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
    public static function fetch_report_request($request_id) {
        $json = db_select('ebms_report_request', 'r')
            ->fields('r', array('parameters'))
            ->condition('r.request_id', $request_id)
            ->execute()
            ->fetchField();
        return drupal_json_decode($json);
    }
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
}

function pdq_ebms_citation_reports_form_submit($form, &$form_state) {
    $form_state['rebuild'] = true;
    return;
}

function pdq_ebms_report_ajax($form, $form_state) {
    return $form['top']['report-form']['ajax fields'];
}
function pdq_ebms_cit_status_report_ajax($form, $form_state) {
    return $form['top']['report-form']['status-dep'];
}

function pdq_ebms_citation_reports_form_validate($form, &$form_state) {
    $values = $form_state['values'];
    $report = $values['report'];
    $cycle = isset($values['cycle']) ? $values['cycle'] : 0;
    if (!$report)
        form_set_error('report', 'No report selected');
    else {
        switch ($report) {
            case 'Citations Published':
            case 'Citations Rejected/Accepted for Publishing':
            case 'Citations Imported':
                if (!$cycle)
                    form_set_error('cycle', 'Please select a review cycle');
                break;
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

function pdq_ebms_citation_reports_form($form, &$form_state) {
    drupal_set_breadcrumb(
        array(
            l('Reports', 'reports'),
            l('Citation Management Reports', 'reports/citation'),
            'Citation Reports',
        )
    );
    $left_nav = EbmsReports::build_left_nav('Citation Management',
                                            'Citation Reports');
    $board = $cycle = $report = 0;
    $start_date = $end_date = array('year' => 0, 'month' => 0, 'day' => 0);
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
    if ($report && $values['op'] == 'Submit') {
        switch ($report) {
            case 'Citations Published':
                $form['report'] =
                    EbmsReports::cites_published($boards, $board, $cycle);
                break;
            case 'Citations Rejected/Accepted for Publishing':
                $form['report'] =
                    EbmsReports::initial_decision($boards, $board, $cycle);
                break;
            case 'Citations Imported':
                $form['report'] =
                    EbmsReports::cites_imported($boards, $board, $cycle);
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

function pdq_ebms_import_reports_form_submit($form, &$form_state) {
    if ($form_state['values']['op'] == 'Reset')
        drupal_goto('reports/import');
    $form_state['rebuild'] = true;
    return;
}

function pdq_ebms_import_reports_form($form, &$form_state, $job=0) {
    drupal_set_breadcrumb(
        array(
            l('Reports', 'reports'),
            l('Citation Management Reports', 'reports/citation'),
            'Import Reports',
        )
    );
    $left_nav = EbmsReports::build_left_nav('Citation Management',
                                            'Import Reports');
    $board = $cycle = $topic = 0;
    $op = '';
    $range = false;
    $start = $end = array('year' => 0, 'month' => 0, 'day' => 0);
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
    if ($job)
        $form['report'] = EbmsReports::import_job($job);
    elseif ($op == 'Submit')
        $form['report'] = EbmsReports::import_reports($board, $topic,
                          $cycle, $start, $end, $range);
    return $form;
}

function pdq_ebms_documents_report_form_submit($form, &$form_state) {
    if ($form_state['values']['op'] == 'Reset')
        drupal_goto('reports/documents');
    $values = $form_state['values'];
    global $user;
    $now = strftime('%Y-%m-%d %H:%M:%S');
    $request_id = EbmsReports::save_report_request($values, 'Documents');
    drupal_goto('reports/documents/' . $request_id);
}

function pdq_ebms_documents_report_form($form, &$form_state, $request_id) {
    drupal_set_breadcrumb(
        array(
            l('Reports', 'reports'),
            l('Board Managment Reports', 'reports/board'),
            'Documents',
        )
    );
    $board = $tag = $inhouse = $member = 0;
    $keyword = '';
    $range = $archived = false;
    $start = $end = array('year' => 0, 'month' => 0, 'day' => 0);
    if ($request_id) {
        $params = EbmsReports::fetch_report_request($request_id);
        $board = $params['board'];
        $tag = $params['tag'];
        $keyword = $params['keyword'];
        $inhouse = $params['inhouse'];
        $member = $params['member'];
        $archived = $params['archived'];
        $start = $params['upload-start'];
        $range = $params['use-upload-date-range'];
        if ($range)
            $end = $params['upload-end'];
    }
    $left_nav = EbmsReports::build_left_nav('Board Management',
                                            'Documents');
    $boards = array(0 => ' ') + Ebms\Util::boards();
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
    $inhouse_users = array(0 => 'IN-HOUSE STAFF');
    $board_members = array(0 => 'BOARD MEMBERS');
    $results = db_select('users', 'u')
        ->fields('u', array('uid', 'name'))
        ->condition('u.uid', 0, '>')
        ->orderBy('u.name')
        ->execute();
    foreach ($results as $result)
        if (array_key_exists($result->uid, $board_member_ids))
            $board_members[$result->uid] = $result->name;
        else
            $inhouse_users[$result->uid] = $result->name;
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
                '#options' => $board_members,
                '#default_value' => $member,
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
    $form['report'] = EbmsReports::docs($request_id);
    // $board, $tag, $keyword, $inhouse,
    //$member, $archived, $range, $start, $end);
    return $form;
}

function pdq_ebms_doc_edit_form($form, &$form_state, $doc_id, $report_id) {
    $query = db_select('ebms_doc', 'd');
    $query->join('file_managed', 'f', 'f.fid = d.file_id');
    $query->fields('f', array('filename'));
    $file_name = htmlspecialchars($query->execute()->fetchField());
    $parms = drupal_get_query_parameters();
    $parent_url = 'reports/documents';
    if ($report_id)
        $parent_url .= "/$report_id";
    drupal_set_breadcrumb(
        array(
            l('Reports', 'reports'),
            l('Board Managment Reports', 'reports/board'),
            l('Documents', $parent_url, array('query' => $parms)),
            $file_name,
        )
    );
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

function pdq_ebms_lit_review_report_form($form, &$form_state, $request_id=0) {
    drupal_set_breadcrumb(
        array(
            l('Reports', 'reports'),
            l('Citation Managment Reports', 'reports/cite'),
            'Literature Reviews',
        )
    );
    $left_nav = EbmsReports::build_left_nav('Citation Management',
                                            'Literature Reviews');
    $boards = array(0 => ' ') + Ebms\Util::boards();
    $topics = array(0 => ' ');
    $board = $topic = $reviewer = $cycle = $rrange = $prange = 0;
    $packet = $op = '';
    $cycles = array(0 => ' ') + Ebms\Util::cycles(true);
    $reviewers = array(0 => ' ');
    $rstart = $rend = $pstart = $pend = Ebms\Util::empty_date();
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
    if (isset($form_state['values'])) {
        $values = $form_state['values'];
        $op = $values['op'];
        $board = $values['board'];
    }
    if ($board) {
        $results = db_select('ebms_topic', 't')
            ->fields('t', array('topic_id', 'topic_name'))
            ->condition('t.board_id', $board)
            ->orderBy('t.topic_name')
            ->execute();
        foreach ($results as $result)
            $topics[$result->topic_id] = $result->topic_name;
    }
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
            'packet' => array(
                '#type' => 'textfield',
                '#title' => 'Packet Name',
                '#title_display' => 'after',
                '#default_value' => $packet,
                '#attributes' => array('class' => array('fixed-width')),
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
    if ($request_id)
        $form['report'] = EbmsReports::reviews($request_id);
    return $form;
}

function pdq_ebms_lit_review_report_form_submit($form, &$form_state) {
    pdq_ebms_debug('REVIEW REPORT FORM SUBMIT', $form_state['values']);
    if ($form_state['values']['op'] == 'Reset')
        drupal_goto('reports/lit-review');
    if ($form_state['values']['op'] == 'Submit') {
        $values = $form_state['values'];
        $request_id = EbmsReports::save_report_request($values,
                                                       'Literature Reviews');
        drupal_goto('reports/lit-review/' . $request_id);
    }
}

function pdq_ebms_cites_by_status_form($form, &$form_state, $request_id) {
    drupal_set_breadcrumb(
        array(
            l('Reports', 'reports'),
            l('Citation Managment Reports', 'reports/cite'),
            'Citations By Status',
        )
    );
    $left_nav = EbmsReports::build_left_nav('Citation Management',
                                            'Citations By Status');
    $board = $topic = $cycle = $status = $meeting_type = $disposition = 0;
    $meeting_date = Ebms\Util::empty_date();
    if ($request_id) {
        $values = EbmsReports::fetch_report_request($request_id);
        pdq_ebms_debug('BY STATUS FORM VALUES', array($request_id, $values));
        $board = $values['board'];
        $topic = $values['topic'];
        $status = $values['status'];
        if (isset($values['disposition']))
            $disposition = $values['disposition'];
        $cycle = $values['cycle'];
        if (isset($values['meeting-type']))
            $meeting_type = $values['meeting-type'];
        if (isset($values['meeting-date']))
            $meeting_date = $values['meeting-date'];
    }
    $meeting_types = array(
        0 => '',
        'subgroup' => 'Subgroup',
        'board' => 'Board',
    );
    $cycles = array(0 => ' ') + Ebms\Util::cycles();
    $boards = array(0 => ' ') + Ebms\Util::boards();
    $yes_no = $yes_no_fyi = array(0 => ' ', 'yes' => 'Yes', 'no' => 'No');
    $yes_no_fyi['fyi'] = 'Flag for FYI';
    $review_dispositions = array(0 => ' ');
    $results = db_select('ebms_review_disposition_value', 'v')
        ->fields('v', array('value_id', 'value_name'))
        ->orderBy('v.value_pos')
        ->execute();
    foreach ($results as $result)
        $review_dispositions[$result->value_id] = $result->value_name;

    $manager_actions = array(0 => ' ') + EbmsReports::manager_actions();
//     foreach (EbmsReports::$manager_actions as $text_id) {
//         $results = db_select('ebms_article_state_type', 't')
//             ->fields('t', array('state_id', 'state_name'))
//             ->condition('t.state_text_id', $text_id)
//             ->execute();
//         foreach ($results as $result)
//             $manager_actions[$result->state_id] = $result->state_name;
//     }
    $board_decisions = array(0 => ' ');
    $results = db_select('ebms_article_board_decision_value', 'v')
        ->fields('v', array('value_id', 'value_name'))
        ->orderBy('v.value_name')
        ->execute();
    foreach ($results as $result)
        $board_decisions[$result->value_id] = $result->value_name;
    $topics = array(0 => ' ');
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
    $all_dispositions = array(
        'reviewer decision' => $yes_no,
        'full text retrieved' => $yes_no,
        'committee decision' => $yes_no_fyi,
        'assigned for review' => array(0 => ''),
        'board member responses' => $review_dispositions,
        'board manager action' => $manager_actions,
        'editorial board decision' => $board_decisions,
    );

    if (isset($form_state['values'])) {
        $board = $form_state['values']['board'];
        $status = $form_state['values']['status'];
        $topic = $form_state['values']['topic'];
        $disposition = $form_state['values']['disposition'];
    }
    if ($board) {
        $results = db_select('ebms_topic', 't')
            ->fields('t', array('topic_id', 'topic_name'))
            ->condition('t.board_id', $board)
            ->orderBy('t.topic_name')
            ->execute();
        foreach ($results as $result)
            $topics[$result->topic_id] = $result->topic_name;
        if ($topic && !array_key_exists($topic, $topics))
            $topic = $form_state['values']['topic'] = 0;
    }
    $dispositions = array(0 => '');
    if (array_key_exists($status, $all_dispositions))
        $dispositions = $all_dispositions[$status];
    if ($disposition && !array_key_exists($disposition, $dispositions))
        $disposition = $form_state['values']['disposition'] = 0;
    pdq_ebms_debug('DISPOSITIONS', array($status, $dispositions));
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
    $form['top']['report-form']['status-dep']['cycle'] = array(
        '#type' => 'select',
        '#title' => 'Review Cycle',
        '#title_display' => 'after',
        '#options' => $cycles,
        '#default_value' => $cycle,
        '#attributes' => array('class' => array('fixed-width')),
    );
    if (!strcmp($status, 'on agenda') ||
        !strcmp($status, 'editorial board decision')) {
        $form['top']['report-form']['status-dep']['meeting-type'] = array(
            '#type' => 'select',
            '#title' => 'Meeting Type',
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
    pdq_ebms_debug('AJAX MYSTERY', array($form, $status));
    if ($request_id)
        $form['report'] = EbmsReports::cites_by_status($request_id);
    return $form;
}

function pdq_ebms_cites_by_status_form_validate($form, &$form_state) {
    $values = $form_state['values'];
    if (!$values['board'])
        form_set_error('board', 'No board selected.');
    if (!$values['status'])
        form_set_error('status', 'No status selected.');
}
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
 * broken).
 */
function pdq_ebms_doc_file_required($element, &$form_state) {
    if ($_FILES['files']['name']['file'] == '') {
        form_set_error('file', t('Please select a document'));
    }
}
