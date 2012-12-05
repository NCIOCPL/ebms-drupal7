<?php

// $Id$

/**
 * @file
 *
 * Implementation of EBMS pages for citation management
 */

/**
 * Callback registered for /citations pages.
 */
function pdq_ebms_search($action=null, $id=null) {
    Ebms\Menu::$active = 'Citation Management';
    switch ($action) {
        case 'search':
        case 'refine':
            $search = new EbmsSearch($action, $id);
            return $search->run();
        default:
            drupal_set_title('Citation Management');
            return '<i>Stub for \'Citations\' page</i>';
    }
}

class EbmsSearch {

    public
        $request,
        $parms,
        $search_id;
    public static $url = 'citations/search';
    public function __construct($action, $id) {
        $this->request = $action;
        $this->search_id = $id;
        $this->parms = drupal_get_query_parameters();
    }
    public function run() {
        drupal_add_css(Ebms\CSS_DIR . '/search.css');
        drupal_set_breadcrumb(
            array(
                l('Citation Management', 'citations'),
                'Search Database',
            )
        );
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

    function show_search_results() {
        require_once('EbmsSearch.inc');
        $json = $this->fetch_search_spec($this->search_id, false);
        $spec = drupal_json_decode($json);
        $query = Ebms\getSearchQuery($json);
        $counter = $query->countQuery();
        $count = $counter->execute()->fetchField();
        $per_page = $spec['items-per-page'];
        $pager = true;
        $show_pages_link = '';
        if (isset($this->parms['pager']) && $this->parms['pager'] == 'off') {
            $pager = false;
            if ($per_page != 'View All' && $per_page < $count) {
                $url = EbmsSearch::$url . '/' . $this->search_id;
                $show_pages_link = l('VIEW PAGES', $url);
            }
        }
        if (!$pager || $per_page == 'View All')
            $per_page = $count;
        if ($count >= $per_page)
            $query = $query->extend('PagerDefault')->limit($per_page);
        $results = $query->execute();
        $items = array();
        $page = 0;
        if (isset($this->parms['page']))
            $page = $this->parms['page'];
        $pos = 1 + $page * $per_page;
        foreach ($results as $result)
            $items[] = $this->list_article($result->article_id, $pos++);
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
    function list_article($article_id, $position) {
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
                if (sizeof($authors) == 8) {
                    $authors[] = 'et al.';
                    break;
                }
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
        $ta = $doc->MedlineCitation->MedlineJournalInfo->MedlineTA;
        $ta = htmlspecialchars($ta);
        $pmid = $doc->MedlineCitation->PMID;
        $full_url = 'citations/full/' . $article_id;
        $full_opts = array('query' => array('search' => $this->search_id));
        $title_opts = array(
            'query' => array('search' => $this->search_id),
            'attributes' => array(
                'class' => array('article-title'),
            )
        );
        $title_link = l($title, $full_url, $title_opts);
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
        $edit_link = l('EDIT', $full_url, $full_opts);
        $pub_link = l('PUBLISH', $full_url, $full_opts);
        $ft_link = l('DOWNLOAD FULL TEXT', $full_url, $full_opts);
        $pieces[] = "<br />PMID: $pmid &nbsp; CMS ID: $article_id<br /><br />";
        $pieces[] = '<a href="http://www.ncbi.nlm.nih.gov/pubmed/' .
            $pmid . '" target="_blank">VIEW ABSTRACT</a> | ';
        $pieces[] = $ft_link . ' | ';
        $pieces[] = $edit_link . ' | ';
        $pieces[] = l('REVIEW', 'citations/review') . ' | ';
        $pieces[] = $pub_link . '</span></br /><br />&nbsp;';
        return implode($pieces);
    }

    public static function defaults() {
        return array(
            'boards' => array(),
            'topic-logic' => null,
            'topics' => array(),
            'pmid' => '',
            'cms-id' => '',
            'author' => '',
            'title' => '',
            'advanced-options' => array(
                'fyi-citation' => 0,
                'journal-exact-match' => 0,
            ),
            'journal' => '',
            'publication-year' => 0,
            'publication-month' => 0,
            'review-cycle' => 0,
            'nci-reviewer-decision' => null,
            'full-text-retrieved' => null,
            'committee-decision' => null,
            'reviewer' => 0,
            'response' => 0,
            'decision' => 0,
            'comments' => '',
            'comment-added-start' => $no_date,
            'comment-added-end' => $no_date,
            'tag-added-start' => $no_date,
            'tag-added-end' => $no_date,
            'input-start' => $no_date,
            'input-end' => $no_date,
            'use-comment-date-range' => 0,
            'use-tag-date-range' => 0,
            'use-comment-date-range' => 0,
            'tag' => 0,
        );
    }
}

class EbmsSearchSpec {
    public $no_date = array('year' => 0, 'month' => 0, 'day' => 0);
    public
        $boards = array(),
        $topics = array(),
        $advanced_options = array(),
        $admin_options = array(),
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
        $reviewer = 0,
        $response = 0,
        $decision = 0,
        $tag = 0,
        $comment_added_start = array('year' => 0, 'month' => 0, 'day' => 0),
        $comment_added_end = array('year' => 0, 'month' => 0, 'day' => 0),
        $tag_added_start = array('year' => 0, 'month' => 0, 'day' => 0),
        $tag_added_end = array('year' => 0, 'month' => 0, 'day' => 0),
        $input_start = array('year' => 0, 'month' => 0, 'day' => 0),
        $input_end = array('year' => 0, 'month' => 0, 'day' => 0),
        $modified_start = array('year' => 0, 'month' => 0, 'day' => 0),
        $modified_end = array('year' => 0, 'month' => 0, 'day' => 0),
        $use_comment_date_range = 0,
        $use_tag_date_range = 0,
        $use_input_date_range = 0,
        $use_modified_date_range = 0,
        $sort_by = 'CMS ID#',
        $format = 'Brief',
        $items_per_page = 10;
    public function __construct($search_id) {
        if ($search_id) {
            $spec = EbmsSearch::fetch_search_spec($search_id);
            foreach ($spec['boards'] as $id => $val)
                if ($val)
                    $this->boards[] = $id;
            foreach ($spec['topics'] as $id => $val)
                if ($val)
                    $this->topics[] = $id;
            foreach ($spec['advanced-options'] as $id => $val)
                if ($val)
                    $this->advanced_options[] = $id;
            foreach ($spec['admin-options'] as $id => $val)
                if ($val)
                    $this->admin_options[] = $id;
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
            $this->comment_added_start = $spec['comment-added-start'];
            $this->comment_added_end = $spec['comment-added-end'];
            $this->tag_added_start = $spec['tag-added-start'];
            $this->tag_added_end = $spec['tag-added-end'];
            $this->input_start = $spec['input-start'];
            $this->input_end = $spec['input-end'];
            $this->modified_start = $spec['modified-start'];
            $this->modified_end = $spec['modified-end'];
            $this->use_comment_date_range = $spec['use-comment-date-range'];
            $this->use_tag_date_range = $spec['use-tag-date-range'];
            $this->use_input_date_range = $spec['use-input-date-range'];
            $this->use_modified_date_range = $spec['use-mod-date-range'];
            $this->sort_by = $spec['sort-by'];
            $this->format = $spec['format'];
            $this->items_per_page = $spec['items-per-page'];
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

    // Initial assumption.
    $have_value = $have_board = false;

    // Use shorthand for checking values.
    $values = $form_state['values'];
    pdq_ebms_debug('SEARCH FORM VALIDATE', $values);

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

    // Check the fields that can have a single value.
    if (!$have_value) {
        $simple_fields = array('pmid', 'cms-id', 'author', 'title', 'journal',
                         'comments', 'publication-year', 'review-cycle',
                         'reviewer', 'response', 'decision', 'tag');
        foreach ($simple_fields as $field) {
            if ($values[$field]) {
                $have_value = true;
                break;
            }
        }
    }

    // Flag options to check.
    $flag_fields = array(
        'admin' => array('summary-topics-added'),
        'advanced' => array(
            'fyi-citation',
            'nci-reviewer-decision',
            'full-text-retrieved',
            'committee-decision',
            'published-to-cite-ms',
        ),
    );
    if (!$have_value) {
        foreach ($flag_fields as $group => $names) {
            $set = $values[$group . '-options'];
            foreach ($names as $name) {
                if ($set[$name]) {
                    $have_value = true;
                    break 2;
                }
            }
        }
    }

    // Check the date fields.
    $date_fields = array(
        'comment-added' => 'use-comment-date-range',
        'tag-added' => 'use-tag-date-range',
        'input' => 'use-input-date-range',
        'modified' => 'use-mod-date-range',
    );
    $message = 'Date range must be specified with complete dates.';
    foreach ($date_fields as $field => $flag_field) {
        $start_name = $field . '-start';
        $start_date = $values[$start_name];
        if ($start_date['year'])
            $have_value = true;
        if ($values[$flag_field]) {
            $end_name = $field . '-end';
            $end_date = $values[$end_name];
            if (!$start_date['day'])
                form_set_error($start_name, $message);
            elseif (!$end_date['day'])
                form_set_error($start_name, $message);
        }
    }

    // Complain if we didn't get any search criteria.
    if (!$have_value)
        form_set_error('pmid',
            'At least one (non-board) search criterion is required.');
}

function pdq_ebms_search_db_form($form, &$form_state, $search_id) {
    $defaults = new EbmsSearchSpec($search_id);
    $checked_boards = $defaults->boards;
    $topics = array();
    if (isset($form_state['values'])) {
        $checked_boards = array();
        foreach ($form_state['values']['boards'] as $id => $val)
            if ($val)
                $checked_boards[] = $val;
    }
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
    $results = db_select('ebms_article_tag_type', 't')
        ->fields('t', array('tag_id', 'tag_name'))
        ->orderBy('t.tag_name')
        ->execute();
    $tags = array(0 => ' ');
    foreach ($results as $result)
        $tags[$result->tag_id] = $result->tag_name;
    $results = db_select('ebms_article_board_decision_value', 'v')
        ->fields('v', array('value_id', 'value_name'))
        ->orderBy('v.value_name')
        ->execute();
    $decisions = array(0 => ' ');
    foreach ($results as $result)
        $decisions[$result->value_id] = $result->value_name;
    $results = db_select('ebms_review_disposition_value', 'v')
        ->fields('v', array('value_id', 'value_name'))
        ->orderBy('v.value_name')
        ->execute();
    $responses = array(0 => ' ');
    foreach ($results as $result)
        $responses[$result->value_id] = $result->value_name;
    $query = db_select('users', 'u');
    $query->join('ebms_packet_reviewer', 'r', 'r.reviewer_id = u.uid');
    $query->fields('u', array('uid', 'name'));
    $query->orderBy('u.name');
    $results = $query->execute();
    $reviewers = array(0 => ' ');
    foreach ($results as $result)
        $reviewers[$result->uid] = $result->name;
    $pub_months = array(0 => ' ');
    for ($i = 1; $i <= 12; ++$i)
        $pub_months[$i] = date('F', mktime(0, 0, 0, $i));
    $pub_years = array(0 => ' ');
    $last_year = 2013;
    for ($i = 1964; $i <= $last_year; ++$i)
        $pub_years[$i] = $i;
    $results = db_select('ebms_cycle', 'c')
        ->fields('c', array('cycle_id', 'cycle_name'))
        ->orderBy('c.start_date', 'desc')
        ->execute();
    $review_cycles = array(0 => ' ');
    foreach ($results as $result)
        $review_cycles[$result->cycle_id] = $result->cycle_name;
    return array(
        '#id' => 'search-db-form',
        'page title' => array(
            '#type' => 'markup',
            '#markup' => '<h2>Search Database</h2>',
        ),
        'boards' => array(
            '#type' => 'select',
            '#multiple' => true,
            '#default_value' => $defaults->boards,
            '#title' => 'EDITORIAL BOARD',
            '#description' => 'To select more than one, hold down the CTRL ' .
                              'or COMMAND (for Mac users) key',
            '#options' => Ebms\Util::boards(),
            '#size' => 6,
            '#ajax' => array(
                'callback' => 'pdq_ebms_db_search_board_ajax',
                'wrapper' => 'topics-boxes',
            ),
        ),
        'topics box' => array(
            '#type' => 'fieldset',
            '#title' => 'SUMMARY TOPIC',
            '#collapsible' => true,
            '#collapsed' => true,
            'topic-logic' => array(
                '#type' => 'radios',
                '#title' => 'SEARCH CONNECTOR',
                '#default_value' => $defaults->topic_connector,
                '#description' => 'If you do not select "AND" or "OR," ' .
                                  'the default search will be "OR"',
                '#options' => drupal_map_assoc(array('AND', 'OR')),
            ),
            'topics' => array(
                '#type' => 'checkboxes',
                '#options' => $topics,
                '#default_value' => $defaults->topics,
                '#prefix' => '<div id="topics-boxes">',
                '#suffix' => '</div>',
            ),
        ),
        'pmid' => array(
            '#type' => 'textfield',
            '#default_value' => $defaults->pmid,
            '#title' => 'PMID',
        ),
        'cms-id' => array(
            '#type' => 'textfield',
            '#title' => 'CMS ID',
            '#default_value' => $defaults->cms_id,
        ),
        'author' => array(
            '#type' => 'textfield',
            '#title' => 'AUTHOR',
            '#default_value' => $defaults->author,
        ),
        'title' => array(
            '#type' => 'textfield',
            '#title' => 'TITLE',
            '#default_value' => $defaults->title,
        ),
        'advanced search' => array(
            '#type' => 'fieldset',
            '#title' => 'ADVANCED SEARCH',
            '#collapsible' => true,
            '#collapsed' => true,
            'advanced-options' => array(
                '#type' => 'checkboxes',
                '#title' => 'ADVANCED OPTIONS',
                '#options' => array(
                    'fyi-citation' => 'FYI CITATION',
                    'nci-reviewer-decision' => 'NCI REVIEWER DECISION',
                    'full-text-retrieved' => 'FULL TEXT RETRIEVED',
                    'committee-decision' => 'COMMITTEE DECISION',
                    'published-to-cite-ms' => 'PUBLISHED TO CITE MS',
                    'core-journals' => 'CORE JOURNALS',
                    'not-list' => 'INCLUDE NOT LIST JOURNALS',
                ),
                '#default_value' => $defaults->advanced_options,
            ),
            'journal' => array(
                '#type' => 'textfield',
                '#title' => 'JOURNAL',
                '#default_value' => $defaults->journal,
            ),
            'publication-year' => array(
                '#type' => 'select',
                '#title' => 'PUBLICATION YEAR/MONTH',
                '#options' => $pub_years,
                '#default_value' => $defaults->publication_year,
            ),
            'publication-month' => array(
                '#type' => 'select',
                '#options' => $pub_months,
                '#default_value' => $defaults->publication_month,
            ),
            'review-cycle' => array(
                '#type' => 'select',
                '#title' => 'REVIEW CYCLE',
                '#options' => $review_cycles,
                '#default_value' => $defaults->review_cycle,
            ),
            'reviewer' => array(
                '#type' => 'select',
                '#title' => 'EDITORIAL BOARD REVIEWER',
                '#options' => $reviewers,
                '#default_value' => $defaults->reviewer,
            ),
            'response' => array(
                '#type' => 'select',
                '#title' => 'EDITORIAL BOARD RESPONSE',
                '#options' => $responses,
                '#default_value' => $defaults->response,
            ),
            'decision' => array(
                '#type' => 'select',
                '#title' => 'EDITORIAL BOARD DECISION',
                '#options' => $decisions,
                '#default_value' => $defaults->decision,
            ),
            'comments' => array(
                '#type' => 'textfield',
                '#title' => 'COMMENTS',
                '#default_value' => $defaults->comments,
            ),
            'comment-added-start' => array(
                '#type' => 'date',
                '#title' => 'DATE COMMENT ADDED',
                '#element_validate' => array('pdq_ebms_date_validate'),
                '#default_value' => $defaults->comment_added_start,
                '#pre_render' => array('pdq_ebms_fix_date'),
            ),
            'use-comment-date-range' => array(
                '#type' => 'checkbox',
                '#title' => 'RANGE',
                '#prefix' => '<div class="use-range-checkbox">',
                '#suffix' => '</div>',
                '#default_value' => $defaults->use_comment_date_range,
            ),
            'comment-added-end' => array(
                '#type' => 'date',
                '#element_validate' => array('pdq_ebms_date_validate'),
                '#default_value' => $defaults->comment_added_end,
                '#pre_render' => array('pdq_ebms_fix_date'),
                '#states' => array(
                    'visible' => array(
                        '#edit-use-comment-date-range' => array(
                            'checked' => true
                        ),
                    ),
                ),
            ),
            'tag' => array(
                '#type' => 'select',
                '#title' => 'TAG',
                '#options' => $tags,
                '#default_value' => $defaults->tag,
            ),
            'tag-added-start' => array(
                '#type' => 'date',
                '#element_validate' => array('pdq_ebms_date_validate'),
                '#title' => 'DATE TAG ADDED',
                '#default_value' => $defaults->tag_added_start,
                '#pre_render' => array('pdq_ebms_fix_date'),
            ),
            'use-tag-date-range' => array(
                '#type' => 'checkbox',
                '#title' => 'RANGE',
                '#prefix' => '<div class="use-range-checkbox">',
                '#suffix' => '</div>',
                '#default_value' => $defaults->use_tag_date_range,
            ),
            'tag-added-end' => array(
                '#type' => 'date',
                '#element_validate' => array('pdq_ebms_date_validate'),
                '#default_value' => $defaults->tag_added_end,
                '#pre_render' => array('pdq_ebms_fix_date'),
                '#states' => array(
                    'visible' => array(
                        '#edit-use-tag-date-range' => array(
                            'checked' => true
                        ),
                    ),
                ),
            ),
        ),
        'administrator search' => array(
            '#type' => 'fieldset',
            '#title' => 'ADMINISTRATOR SEARCH',
            '#collapsible' => true,
            '#collapsed' => true,
            'admin-options' => array(
                '#type' => 'checkboxes',
                '#title' => 'OPTIONS',
                '#options' => array(
                    'summary-topics-added' => 'SUMMARY TOPICS ADDED',
                    'rejected' => 'INCLUDE REJECTED ARTICLES',
                ),
                '#default_value' => $defaults->admin_options,
            ),
            'input-start' => array(
                '#type' => 'date',
                '#element_validate' => array('pdq_ebms_date_validate'),
                '#title' => 'INPUT DATE',
                '#default_value' => $defaults->input_start,
                '#pre_render' => array('pdq_ebms_fix_date'),
            ),
            'use-input-date-range' => array(
                '#type' => 'checkbox',
                '#title' => 'RANGE',
                '#prefix' => '<div class="use-range-checkbox">',
                '#suffix' => '</div>',
                '#default_value' => $defaults->use_input_date_range,
            ),
            'input-end' => array(
                '#type' => 'date',
                '#element_validate' => array('pdq_ebms_date_validate'),
                '#default_value' => $defaults->input_end,
                '#pre_render' => array('pdq_ebms_fix_date'),
                '#states' => array(
                    'visible' => array(
                        '#edit-use-input-date-range' => array(
                            'checked' => true
                        ),
                    ),
                ),
             ),
            'modified-start' => array(
                '#type' => 'date',
                '#element_validate' => array('pdq_ebms_date_validate'),
                '#title' => 'MODIFIED DATE',
                '#default_value' => $defaults->modified_start,
                '#pre_render' => array('pdq_ebms_fix_date'),
            ),
            'use-mod-date-range' => array(
                '#type' => 'checkbox',
                '#title' => 'RANGE',
                '#prefix' => '<div class="use-range-checkbox">',
                '#suffix' => '</div>',
                '#default_value' => $defaults->use_modified_date_range,
            ),
            'modified-end' => array(
                '#type' => 'date',
                '#element_validate' => array('pdq_ebms_date_validate'),
                '#default_value' => $defaults->modified_end,
                '#pre_render' => array('pdq_ebms_fix_date'),
                '#states' => array(
                    'visible' => array(
                        '#edit-use-mod-date-range' => array(
                            'checked' => true
                        ),
                    ),
                ),
             ),
        ),
        'display options' => array(
            '#type' => 'fieldset',
            '#title' => 'DISPLAY OPTIONS',
            '#collapsible' => true,
            '#collapsed' => true,
            'sort-by' => array(
                '#type' => 'select',
                '#title' => 'SORT BY',
                '#options' => drupal_map_assoc(
                    array(
                        'CMS ID#',
                        'PM ID#',
                        //'Author',
                        'Title',
                        'Journal',
                        'Publication Date',
                    )
                ),
                '#default_value' => $defaults->sort_by,
            ),
            'format' => array(
                '#type' => 'select',
                '#title' => 'FORMAT',
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
            'items-per-page' => array(
                '#type' => 'select',
                '#title' => 'PER PAGE',
                '#options' => drupal_map_assoc(
                    array('10', '25', '50', 'View All')
                ),
                '#default_value' => $defaults->items_per_page,
            ),
        ),
        'submit' => array(
            '#type' => 'submit',
            '#value' => 'SUBMIT',
        )
    );
}

function pdq_ebms_db_search_board_ajax($form, $form_state) {
    return $form['topics box']['topics'];
}

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
    if ($y && $m && $d && !checkdate($m, $d, $y))
        form_error($element, 'The specified date is invalid.');
    elseif ($m && !$y)
        form_error($element, 'Month cannot be specified without year.');
    elseif ($d && !$m)
        form_error($element,
            'Day cannot be specified without month and year.');
}
