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

        Ebms\Util::add_js();
        drupal_add_js(Ebms\JS_DIR . '/reports.js', 'file');
        Ebms\Menu::$active = 'Reports';
        drupal_add_css(Ebms\CSS_DIR . '/reports.css');
        switch ($this->report) {
            case 'run':
                return $this->create_report();
            case 'citations':
                return drupal_get_form('pdq_ebms_citation_reports_form');
            case 'import':
                return drupal_get_form('pdq_ebms_import_reports_form');
            case 'board':
                return $this->landing_page('Board Management');
            case 'cite':
                return $this->landing_page('Citation Management');
            default:
                return $this->landing_page();
        }
    }

    function create_report() {
        $json = db_select('ebms_report_request', 'r')
            ->fields('r', array('parameters'))
            ->condition('r.request_id', $this->request_id)
            ->execute()
            ->fetchField();
        return '<p>' . htmlspecialchars($json) . '</p>';
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
    public static function format_citation($article_id, $counter) {
        $xml = db_select('ebms_article', 'a')
            ->fields('a', array('source_data'))
            ->condition('a.article_id', $article_id)
            ->execute()
            ->fetchField();
        $doc = new \SimpleXMLElement($xml);
        $title = $ta = $vol = $issue = $pgn = $year = $pmid = '';
        $article = $doc->MedlineCitation->Article;
        $title = htmlspecialchars($article->ArticleTitle);
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
        $pieces = array(
            '<span class="article-citation">',
            //'<span class="article-counter">' . $counter . '</span>',
            '<span class="authors">',
            implode(', ', $authors), '</span><br /><em>',
            $title, '</em><br /> ',
            $ta, ' ', $vol,
        );
        if ($issue)
            $pieces[] = "($issue)";
        if ($pgn)
            $pieces[] = ": $pgn";
        if ($year)
            $pieces[] = ", $year";
        $pieces[] = '.<br />PMID: ' . $pmid;

        // XXX Add real link when Dan's finished his current story.
        $pieces[] = '<br /><br />' . l('View Abstract', 'stub');
        return implode($pieces);
    }

    public static function get_nav_links() {
        global $user;
        if (in_array('medical librarian', $user->roles))
            $role = 'librarian';
        else
            $role = 'manager';
        $role = 'librarian'; // XXX debugging
        $links = array(
            'librarian' => array(
                array(
                    'Board Management',
                    'reports/board',
                    array(
                        array('Documents', 'stub'),
                        array('Meeting Dates', 'stub'),
                        array('Hotel Requests', 'stub'),
                        array('Reimbursement', 'stub'),
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
                        array('Documents', 'stub'),
                        array('Meeting Dates', 'stub'),
                        array('Hotel Requests', 'stub'),
                        array('Reimbursement', 'stub'),
                    ),
                ),
                array(
                    'Citation Management',
                    'reports/cite',
                    array(
                        array('Citations By Status', 'stub'),
                        array('Literature Reviews', 'stub'),
                        array('No Responses', 'stub'),
                        array('Citation Follow-up', 'stub'),
                        array('Literature Report', 'stub'),
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
        pdq_ebms_debug("CITES PUBLISHED", array($boards, $board, $cycle));
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
        //pdq_ebms_debug('CITES PUBLISHED QUERY', 'query: ' . $query);
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
        //pdq_ebms_debug('CITES PUBLISHED QUERY', 'query: ' . $query);
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
        //pdq_ebms_debug('CITES PUBLISHED QUERY', 'query: ' . $query);
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
        //pdq_ebms_debug('INITIAL DECISION QUERY', 'query: ' . $query);
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
}

function pdq_ebms_citation_reports_form_submit($form, &$form_state) {
    $form_state['rebuild'] = true;
    return;
    $spec = drupal_json_encode($form_state['values']);
    pdq_ebms_debug('ENCODED SPEC', $spec);
    global $user;
    $now = strftime("%Y-%m-%d %H:%M:%S");
    $request_id = db_insert('ebms_report_request')
        ->fields(
            array(
                'report_name' => $form_state['values']['report'],
                'requestor_id' => $user->uid,
                'submitted' => $now,
                'parameters' => $spec,
            )
        )
        ->execute();
    drupal_goto(
        EbmsReports::$url . '/run/' . $request_id,
        array('target' => '_blank')
    );
}

function pdq_ebms_report_ajax($form, $form_state) {
    return $form['top']['report-form']['ajax fields'];
}
function pdq_ebms_citation_reports_form_validate($form, &$form_state) {
    $report = $form_state['values']['report'];
    $cycle = $form_state['values']['cycle'];
    if (!$report)
        form_set_error('report', 'No report selected');
    else {
        switch ($report) {
            case 'Citations Published':
            case 'Citations Rejected/Accepted for Publishing':
                if (!$cycle)
                    form_set_error('cycle', 'Please select a review cycle');
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
        $board = $values['board'];
        $cycle = $values['cycle'];
        $report = $values['report'];
    }
    pdq_ebms_debug('CITES REPORT', array($cycle, $values));
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
    pdq_ebms_debug("REPORT NAME", $report);
    /*
    if (!strcmp($report, 'Citation Summary Topic Changes') ||
        !strcmp($report, 'Citations Not Selected for Full Text Retrieval')) {
        pdq_ebms_debug("REPORT NAME TEST", 'Yes!');
        $form['top']['report-form']['ajax fields']['start-date'] = array(
            '#type' => 'date',
            '#title' => 'Start Date',
            '#title_display' => 'after',
            '#default_value' => $start_date,
            '#attributes' => array('class' => array('fixed-width')),
        );
        $form['top']['report-form']['ajax fields']['end-date'] = array(
            '#type' => 'date',
            '#title' => 'Start Date',
            '#title_display' => 'after',
            '#default_value' => $start_date,
            '#attributes' => array('class' => array('fixed-width')),
        );
    }*/
    pdq_ebms_debug("SWITCH STATEMENT",
        array($report, $boards, $board, $cycle));
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
        }
    }
    return $form;
}

function pdq_ebms_import_reports_form($form, &$form_state) {
    $no_date = array('year' => 0, 'month' => 0, 'day' => 0);
    drupal_set_breadcrumb(
        array(
            l('Reports', 'reports'),
            l('Citation Management Reports', 'reports/citation'),
            'Import Reports',
        )
    );
    $left_nav = EbmsReports::build_left_nav('Citation Management',
                                            'Import Reports');
    $board = $cycle = $report = 0;
    if (isset($form_state['values'])) {
        $board = $form_state['values']['board'];
        $cycle = $form_state['values']['cycle'];
    }
    $boards = array(0 => ' ') + Ebms\Util::boards();
    $cycles = array(0 => ' ');
    $reports = array(0 => ' ') + drupal_map_assoc(
        array(
            'Citations Published',
            'Citations Rejected/Accepted for Publishing',
            'Citations Imported from Text Files',
            'Citations Originally Imported from Text Files',
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
        '#attributes' => array('id' => 'fulltext-top'),
        'left-nav' => $left_nav,
        'report-form' => array(
            '#type' => 'container',
            '#attributes' => array('id' => 'report-form'),
            'title' => array(
                '#type' => 'markup',
                '#markup' => '<h2>Import Reports</h2>',
            ),
            'reports' => array(
                '#type' => 'select',
                '#title' => 'Reports',
                '#title_display' => 'after',
                '#options' => $reports,
                '#default_value' => $report,
                '#attributes' => array('class' => array('fixed-width')),
            ),
            'board' => array(
                '#type' => 'select',
                '#title' => 'Editorial Board',
                '#title_display' => 'after',
                '#options' => $boards,
                '#default_value' => $board,
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
                    //'#title' => 'Import Date',
                    '#default_value' => $no_date,
                    '#pre_render' => array('pdq_ebms_fix_date'),
                ),
                'use-import-date-range' => array(
                    '#type' => 'checkbox',
                    //'#title' => 'RANGE <span class="cb"> </span>',
                    '#title' => 'RANGE',
                    //'#title_display' => 'before',
                    '#prefix' => '<div class="box-on-right">',
                    '#suffix' => '</div>',
                    //'#default_value' => $defaults->use_input_date_range,
                ),
            ),
                'import-date-label' => array(
                    '#type' => 'markup',
                    '#markup' => '<div class="date-label">Import Date</div>',
                ),
            'import-end' => array(
                '#type' => 'date',
                '#element_validate' => array('pdq_ebms_date_validate'),
                '#default_value' => $no_date,
                '#pre_render' => array('pdq_ebms_fix_date'),
                '#states' => array(
                    'visible' => array(
                        '#edit-use-input-date-range' => array(
                            'checked' => true
                        ),
                    ),
                ),
             ),
            'buttons' => array(
                '#type' => 'container',
                '#attributes' => array('id' => 'filter-buttons'),
                'reset' => array(
                    '#type' => 'submit',
                    '#value' => 'Reset',
                ),
                'submit' => array(
                    '#type' => 'submit',
                    '#value' => 'Filter',
                ),
            ),
        ),
    );
    return $form;
}

function pdq_ebms_fix_date($element) {
    pdq_ebms_debug('FIX DATE', $element);
    //$element['year']['#weight'] = 3;
    //$element['month']['#weight'] = 1;
    //$element['day']['#weight'] = 2;
    array_unshift($element['year']['#options'], 'YEAR');
    array_unshift($element['month']['#options'], 'MONTH');
    array_unshift($element['day']['#options'], 'DAY');
    return $element;
}
function pdq_ebms_date_validate($element, &$form_state, $form) {
    $y = $element['#value']['year'];
    $m = $element['#value']['month'];
    $d = $element['#value']['day'];
    if ($y && $m && $d && !checkdate($m, $d, $y))
        form_error($element, 'The specified date is invalid');
}
