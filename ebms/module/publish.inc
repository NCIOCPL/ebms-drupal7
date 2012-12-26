<?php

// $Id$

/**
 * @file
 *
 * Implementation of EBMS page for publishing articles
 */
function pdq_ebms_publish($queue_id=null) {
    $control = new EbmsPublish($queue_id);
    return $control->run();
}

/*
 * Wrapping as much functionality within a class in order to simplify passing
 * around state information for processing the current request.  Also makes
 * it easier to avoid polluting the global namespace.
 */
class EbmsPublish {
    public $parms, $boards, $topics, $start, $queue_id;
    public static $url = 'citations/publish';

    /*
     * Load the settings for displaying the queue for articles
     * which are eligible for promotion to the "Published" state.
     * This state makes the articles eligible for inclusion in
     * the board manager's queue of article ready for her initial
     * review, based on the article abstract.  This intermediate
     * "Published" state allows the librarian to flag articles as
     * having passed her initial review over a stretch of time,
     * without having them trickle into the board manager's
     * review queue one at a time.  Instead the librarian can
     * release a batch of articles to the board manager's queue
     * all at once using this page.
     *
     * We record the current queue in the database so that
     * paging through the queue or re-sorting can happen without
     * forgetting which articles have been marked for publishing.
     * If the current request does not yet have a queue item
     * in the database, we create one and recursively load the page.
     * Any time a new filtering action is applied, we create
     * a new queue row in the database, discarding any selections.
     */
    public function __construct($queue_id) {

        // Remember when we started so we can tell how long this takes.
        $this->start = microtime(true);

        // No queue for this request; create one and reload.
        if (!$queue_id) {
            $filter_parms = array(
                'cycle' => 0,
                'board' => 0,
                'topic' => 0,
            );
            global $user;
            $now = strftime("%Y-%m-%d %H:%M:%S");
            $json = drupal_json_encode($filter_parms);
            $queue_id = db_insert('ebms_publish_queue')
                ->fields(
                    array(
                        'when_created' => $now,
                        'requested_by' => $user->uid,
                        'queue_filter' => $json,
                    )
                )
                ->execute();
            pdq_ebms_debug('CREATING NEW PUBLISH QUEUE',
                array($filter_parms, $json, $queue_id));
            drupal_goto(EbmsPublish::$url . '/' . $queue_id);
        }

        // Collect the state information for this request.
        $this->queue_id = $queue_id;
        $this->parms = drupal_get_query_parameters();
        pdq_ebms_debug('PUBLISH CONSTRUCTOR PARMS', $this->parms);
        $this->boards = Ebms\Util::boards();
        $this->topics = array();
        $results = db_select('ebms_topic', 't')
            ->fields('t', array('topic_id', 'topic_name'))
            ->orderBy('t.topic_name')
            ->execute();
        foreach ($results as $result)
            $this->topics[$result->topic_id] = $result->topic_name;
        $json = db_select('ebms_publish_queue', 'q')
            ->fields('q', array('queue_filter'))
            ->condition('q.queue_id', $queue_id)
            ->execute()
            ->fetchField();
        $this->filter_parms = drupal_json_decode($json);
        pdq_ebms_debug('PUBLISH QUEUE CONSTRUCTOR',
            array($this->filter_parms, $json, $queue_id));
    }

    /*
     * Build the page for the Publish queue.
     */
    public function run() {

        // If the user hit the Publish button, move all the selected
        // articles into the new state and recursively invoke the
        // page without the 'action' parameter, so the queue is
        // built without the newly promoted articles.
        Ebms\Util::add_js();
        if (isset($this->parms['action'])) {
            if ($this->parms['action'] == 'publish-checked') {
                $this->publish_checked();
                drupal_goto(EbmsPublish::$url);
                return;
            }
        }
        drupal_add_css(Ebms\CSS_DIR . '/publish.css');
        Ebms\Menu::$active = 'Citation Management';
        drupal_set_breadcrumb(
            array(
                l('Citation Management', 'citations'),
                'Publish Citations',
            )
        );
        $left_nav = Ebms\Util::build_left_nav('Publish Citations');
        $page = array(
            'top' => array(
                '#type' => 'container',
                '#attributes' => array('id' => 'publish-top'),
                'left-nav' => $left_nav,
                'formbox' => array(
                    '#prefix' => '<div id="publish-filter">',
                    '#suffix' => '</div>',
                    'title' => array(
                        '#type' => 'markup',
                        '#markup' => '<h2>Publish Citations</h2>',
                    ),
                    'form' => drupal_get_form(
                        'pdq_ebms_publish_filter_form',
                        $this
                    ),
                ),
            ),
            'bottom' => $this->get_queue(),
        );

        return $page;
    }

    /*
     * Move all of the articles which have been marked for publishing
     * into the new state.  Delegate as much functionality to the
     * common article APIs as possible.
     */
    function publish_checked() {
        require_once('EbmsArticle.inc');
        $query = db_select('ebms_article_state', 's');
        $query->join('ebms_publish_queue_flag', 'f',
            'f.article_state_id = s.article_state_id');
        $query->condition('f.queue_id', $this->queue_id);
        $query->fields(
            's',
            array('article_id', 'board_id', 'topic_id', 'article_state_id')
        );
        $results = $query->execute();
        $count = 0;
        
        // collect boards for recent activity
        $boards = array();
        
        foreach ($results as $result) {
            $new_state_id = Ebms\setArticleState(
                $result->article_id,
                'Published',
                $result->topic_id,
                $result->board_id
            );
            $count++;
            $boards[$result->board_id] = $result->board_id;
        }

        // create a message for each board found
        foreach ($boards as $board_id) {
            \Ebms\Util::activityArticlesPublished($board_id);
        }
    }
    /*
     * Workhorse for the page: builds and returns the render array
     * for the queue of articles eligible to be moved into the "Published"
     * state.
     */
    function get_queue() {

        // Find out which articles have been marked for publishing
        // from the current queue.
        $query = db_select('ebms_publish_queue_flag', 'f')
            ->condition('f.queue_id', $this->queue_id);
        $query = $query->countQuery();
        $boxes_checked = $query->execute()->fetchField();

        // Get the primary key for the state to be used in the main query.
        $passed_initial_review = db_select('ebms_article_state_type', 't')
            ->fields('t', array('state_id'))
            ->condition('t.state_text_id', 'PassedInitReview')
            ->execute()
            ->fetchField();

        // Create table headers; only the 'Citations' column is sortable.
        $headers = array(
            array(
                '',
                'class' => array('col-1'),
            ),
            array(
                'data' => 'Citations',
                'field' => 'a.last_name',
                'class' => array('col-2'),
            ),
            array(
                'data' => 'Board',
                'class' => array('col-3'),
            ),
            array(
                'data' => 'Summary Topic',
                'class' => array('col-4'),
            ),
            array(
                'data' => 'Publish',
                'class' => array('col-5'),
            ),
        );

        // Build the main query for the queue.
        $query = db_select('ebms_article_state', 's');

        // Sorting is by surname of the first author.
        $query = $query->extend('TableSort');
        $query->leftJoin('ebms_article_author_cite', 'c',
            'c.article_id = s.article_id AND c.cite_order = 1');
        $query->leftJoin('ebms_article_author', 'a',
            'a.author_id = c.author_id');
        $query->fields('s', array('article_id'));
        $query->distinct();
        $query->condition('s.current', 'Y');
        $query->condition('s.state_id', $passed_initial_review);
        if ($this->filter_parms['cycle']) {
            $cycle_query = db_select('ebms_cycle', 'c');
            $cycle_query->fields('c', array('start_date'));
            $cycle_query->addExpression(
                'DATE_ADD(c.start_date, INTERVAL 1 MONTH)',
                'end_date'
            );
            $cycle_query->condition(
                'c.cycle_id',
                $this->filter_parms['cycle']
            );
            $results = $cycle_query->execute();
            foreach ($results as $result) {
                $cycle_start_date = $result->start_date;
                $cycle_end_date = $result->end_date;
            }
            $query->condition(
                's.status_dt',
                array($cycle_start_date, $cycle_end_date),
                'BETWEEN'
            );
        }
        else
            $query->condition('s.status_dt', Ebms\CONVERSION_DATE, '>');
        if ($this->filter_parms['board'])
            $query->condition('s.board_id', $this->filter_parms['board']);
        if ($this->filter_parms['topic'])
            $query->condition('s.topic_id', $this->filter_parms['topic']);
        $query->orderByHeader($headers);

        // Find out how many articles match query (not just for this page).
        $counter = $query->countQuery();
        $count = $counter->execute()->fetchField();

        // Determine whether the queue is paged.
        $pager = true;
        $per_page = 10;
        $show_pages_link = '';
        if (isset($this->parms['pager']) && $this->parms['pager'] == 'off') {
            $pager = false;
            if ($per_page < $count) {
                $url = EbmsPublish::$url;
                $show_pages_link = l('VIEW PAGES', $url);
            }
            $per_page = $count;
        }
        if ($count >= $per_page)
            $query = $query->extend('PagerDefault')->limit($per_page);

        // Determine the position of the first article on the page.
        $page = 0;
        if (isset($this->parms['page']))
            $page = $this->parms['page'];
        $pos = 1 + $page * $per_page;

        // Get the IDs of the articles to be displayed on the current page.
        $results = $query->execute();
        $rows = array();

        // XXX It's theoretically possible that between the query we just
        // ran above and the queries we're about to run for each article
        // someone else may have snuck in a new state or inactived an
        // existing state in a way that will cause an article to appear
        // in the table with no topics for which the article needs to
        // be published.  Don't have time to worry about that very remote
        // possibility.
        foreach ($results as $result) {

            // Create the display for the Citation colum.
            $abstract = $this->format_abstract($result->article_id);

            // Determine which topics are eligible to be published for
            // this article.
            $query = db_select('ebms_article_state', 's');
            $query->fields(
                's',
                array(
                    'board_id',
                    'topic_id',
                    'article_state_id',
                )
            );
            $query->condition('s.current', 'Y');
            $query->condition('s.state_id', $passed_initial_review);
            $query->condition('s.article_id', $result->article_id);
            if ($this->filter_parms['board'])
                $query->condition('s.board_id', $this->filter_parms['board']);
            if ($this->filter_parms['topic'])
                $query->condition('s.topic_id', $this->filter_parms['topic']);
            if ($this->filter_parms['cycle'])
                $query->condition(
                    's.status_dt',
                    array($cycle_start_date, $cycle_end_date),
                    'BETWEEN'
                );
            $topics = $query->execute();
            $total_topics = 0;
            $boards = array();
            foreach ($topics as $topic) {
                if (!array_key_exists($topic->board_id, $boards))
                    $boards[$topic->board_id] = array();
                $boards[$topic->board_id][] = array(
                    $topic->topic_id,
                    $topic->article_state_id,
                );
                $total_topics++;
            }

            // Add rows for each of the boards having topics eligible
            // for promotion for this article.  Adjust the span of
            // the columns for the entire article so the board-specific
            // rows align with the article information.
            $first_article_row = true;
            foreach ($boards as $board_id => $topics) {
                $board_name = $this->boards[$board_id];
                $first_topic_for_board = true;

                // Add a row for each topic eligible for publication for
                // this article.  Same row span adjustment as with the
                // boards.
                foreach ($topics as $topic) {
                    list($topic_id, $article_state_id) = $topic;
                    $row = array();

                    // Plug in the article-specific information if we
                    // haven't already done it.
                    if ($first_article_row) {
                        $row[0] = array(
                            'data' => $pos++,
                            'class' => array('col-1'),
                        );
                        $row[1] = array(
                            'data' => $abstract,
                            'class' => array('col-2'),
                        );
                        if ($total_topics > 1) {
                            $row[0]['rowspan'] = $total_topics;
                            $row[1]['rowspan'] = $total_topics;
                        }
                    }

                    // Plug in the board name for this topic if it's not
                    // already there for another topic on the same board.
                    if ($first_topic_for_board) {
                        $col = array(
                            'data' => $board_name,
                            'class' => array('col-3'),
                        );
                        if (count($topics) > 1)
                            $col['rowspan'] = count($topics);
                        $row[] = $col;
                        $first_topic_for_board = false;
                    }

                    // Add the column for the topic's name.
                    $row[] = array(
                        'data' => $this->topics[$topic_id],
                        'class' => array('col-4'),
                    );

                    // Add the checkbox for flagging/unflagging this
                    // topic from the subset which will be published
                    // when the user hits the "Publish Checked" button.
                    $onclick = 'onclick=\'ebmsscript.publish_checkbox(this,' .
                        $this->queue_id . ',' . $article_state_id . ')\' ';
                    $type = 'type="checkbox"';
                    $checked = '';

                    // Shoud the box already be checked?
                    $flag_results = db_select('ebms_publish_queue_flag', 'f')
                        ->fields('f', array('queue_id'))
                        ->condition('f.queue_id', $this->queue_id)
                        ->condition('f.article_state_id', $article_state_id)
                        ->execute();
                    foreach ($flag_results as $ignore)
                        $checked = ' checked="checked"';
                    $checkbox = "<input $type $onclick$checked />";
                    $row[] = array(
                        'data' => $checkbox,
                        'class' => array('col-5'),
                    );

                    // Package the row for insertion into the render array.
                    $row = array(
                        'data' => $row,
                        'no_striping' => true,
                    );
                    if ($first_article_row)
                        $row['class'] = array('publishable-article');
                    $rows[] = $row;

                    // Turn off the flag which controls display of the
                    // article-specific information.
                    $first_article_row = false;
                }
            }
        }

        // Create action buttons to appear above and below queue table.
        $buttons = array();
        $href = "javascript:ebmsscript.publish_check_all({$this->queue_id})";
        $buttons['check-all-button'] = array(
            '#type' => 'markup',
            '#markup' => "<a onclick='$href' class='button'>Select All</a>",
        );
        $href = "javascript:ebmsscript.publish_clear_all({$this->queue_id})";
        $buttons['clear-all-button'] = array(
            '#type' => 'markup',
            '#markup' => "<a onclick='$href' class='button'>Deselect All</a>",
        );
        $href = url(
            EbmsPublish::$url . '/' . $this->queue_id,
            array(
                'query' => array(
                    'action' => 'publish-checked',
                ),
            )
        );
        $buttons['publish-button'] = array(
            '#type' => 'markup',
            '#markup' => "<a href='$href' class='button'>Batch Publish</a>",
        );

        // Add an indicator which is visible while Ajax is working.
        $buttons['working'] = array(
            '#type' => 'markup',
            '#markup' => '<span class="working">Working...</span>',
        );

        // Assemble the render array and return it.
        $queue = array(
            '#type' => 'container',
            '#attributes' => array('id' => 'publish-bottom'),
            'title' => array(
                '#type' => 'markup',
                '#markup' => "<h2>Unpublished Citations ($count)</h2>",
            ),
            'top-buttons' => array(
                '#type' => 'container',
                '#attributes' => array('id' => 'top-buttons'),
                'buttons' => $buttons,
            ),
        );
        if ($count > $per_page)
            $queue['pager-top'] = array('#theme' => 'pager');
        elseif (!empty($show_pages_link))
            $queue['pager-top'] = array(
                '#type' => 'markup',
                '#markup' =>
                "<div id='pager-view-pages'>$show_pages_link</div>",
            );
        $queue['table'] = array(
            '#theme' => 'table',
            '#attributes' => array('id' => 'pub-queue-table'),
            '#rows' => $rows,
            '#header' => $headers,
        );
        if ($count > $per_page)
            $queue['pager-bottom'] = array('#theme' => 'pager');
        elseif (!empty($show_pages_link))
            $queue['pager-bottom'] = array(
                '#type' => 'markup',
                '#markup' =>
                "<div id='pager-view-pages'>$show_pages_link</div>",
            );
        $queue['bottom-buttons'] = array(
            '#type' => 'container',
            '#attributes' => array('id' => 'bottom-buttons'),
            'buttons' => $buttons,
        );
        $finished = microtime(true);
        $elapsed = $finished - $this->start;
        $queue['elapsed'] = array(
            '#type' => 'markup',
            '#markup' => "\n<!-- elapsed: $elapsed -->\n",
        );
        return $queue;
    }

    /*
     * Assemble the abstract display to be shown in the Citation column.
     * XXX To be rewritten when Alan has the API for getting the abstract
     *     elements ready.
     */
    function format_abstract($article_id) {
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
            '<span class="article-citation"><span class="authors">',
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
        return implode($pieces);
    }
}

/*
 * Creates the render array for the Drupal form shown at the top of the
 * Publish page so the user can control what's visible in the queue.
 */
function pdq_ebms_publish_filter_form($form, &$form_state, $control) {
    $boards = array(0 => ' ') + $control->boards;
    $topics = array(0 => ' ') + $control->topics;
    $cycles = array(0 => ' ');
    $results = db_select('ebms_cycle', 'c')
        ->fields('c', array('cycle_id', 'cycle_name'))
        ->orderBy('c.start_date', 'DESC')
        ->execute();
    foreach ($results as $result)
        $cycles[$result->cycle_id] = $result->cycle_name;
    return array(
        'cycle' => array(
            '#type' => 'select',
            '#title' => 'Review Cycle',
            '#options' => $cycles,
            '#default_value' => $control->filter_parms['cycle'],
        ),
        'board' => array(
            '#type' => 'select',
            '#title' => 'Editorial Board',
            '#options' => $boards,
            '#default_value' => $control->filter_parms['board'],
        ),
        'topic' => array(
            '#type' => 'select',
            '#title' => 'Summary Topic',
            '#options' => $topics,
            '#default_value' => $control->filter_parms['topic'],
        ),
        'reset' => array(
            '#type' => 'submit',
            '#value' => 'Reset',
        ),
        'submit' => array(
            '#type' => 'submit',
            '#value' => 'Filter',
        ),
    );
}

/*
 * Menu callback invoked when user asks to discard any filtering for the
 * queue.  Implemented by redirecting to the base URL for the page with
 * all parameters from the existing queue stripped, causing a new queue
 * object to be created in the database.
 */
function pdq_ebms_publish_filter_reset() {
    drupal_goto(EbmsPublsh::$url);
}

/*
 * Handles Reset and Filter actions submitted from the queue filtering
 * form.
 */
function pdq_ebms_publish_filter_form_submit($form, &$form_state) {
    pdq_ebms_debug('PUBLISH FORM SUBMIT', $form_state['values']);
    if ($form_state['values']['op'] == 'Reset') {
        drupal_goto(EbmsPublish::$url);
        return;
    }
    global $user;
    $now = strftime("%Y-%m-%d %H:%M:%S");
    $filter_parms = array(
        'cycle' => $form_state['values']['cycle'],
        'board' => $form_state['values']['board'],
        'topic' => $form_state['values']['topic'],
    );
    $json = drupal_json_encode($filter_parms);
    $queue_id = db_insert('ebms_publish_queue')
        ->fields(
            array(
                'when_created' => $now,
                'requested_by' => $user->uid,
                'queue_filter' => $json,
            )
        )
        ->execute();
    pdq_ebms_debug("PUBLISH FILTER FORM SUBMIT", array($filter_parms, $json,
        $queue_id));
    drupal_goto(EbmsPublish::$url . '/' . $queue_id);
}

/*
 * Handler for all of the Ajax callbacks for this page:
 *
 *  - clear-all      request to unmark all articles in queue
 *  - check-all      request to mark all articles for publication
 *  - set            request to mark specific article/topic
 *  - clear          request to unmark specific article
 *
 * An error string is sent back to the Javascript caller.  This
 * string will be empty in the normal case.
 */
function pdq_ebms_publish_checkbox_ajax() {
    $error = '';
    $action = Ebms\Util::get_request_value('action');
    $queue_id = Ebms\Util::get_request_value('queue_id');
    $article_state_id = Ebms\Util::get_request_value('article_state_id');
    pdq_ebms_debug('PUBLISH CHECKBOX AJAX', $_GET);
    switch ($action) {
        case 'clear-all':
            db_delete('ebms_publish_queue_flag')
                ->condition('queue_id', $queue_id)
                ->execute();
            break;
        case 'check-all':
            // Clear things out first.
            db_delete('ebms_publish_queue_flag')
                ->condition('queue_id', $queue_id)
                ->execute();

            // Go get the filter parameters for the query.
            $json = db_select('ebms_publish_queue', 'q')
                ->fields('q', array('queue_filter'))
                ->condition('q.queue_id', $queue_id)
                ->execute()
                ->fetchField();
            $filter_parms = drupal_json_decode($json);
            $passed_initial_review = db_select('ebms_article_state_type', 't')
                ->fields('t', array('state_id'))
                ->condition('t.state_text_id', 'PassedInitReview')
                ->execute()
                ->fetchField();
            $query = db_select('ebms_article_state', 's');
            $query->fields('s', array('article_state_id'));
            $query->addExpression($queue_id, 'queue_id');
            $query->condition('s.current', 'Y');
            $query->condition('s.state_id', $passed_initial_review);
            if ($filter_parms['cycle']) {
                $cycle_query = db_select('ebms_cycle', 'c');
                $cycle_query->fields('c', array('start_date'));
                $cycle_query->addExpression(
                    'DATE_ADD(c.start_date, INTERVAL 1 MONTH)',
                    'end_date'
                );
                $cycle_query->condition('c.cycle_id', $filter_parms['cycle']);
                $results = $cycle_query->execute();
                foreach ($results as $result) {
                    $cycle_start_date = $result->start_date;
                    $cycle_end_date = $result->end_date;
                }
                $query->condition(
                    's.status_dt',
                    array($cycle_start_date, $cycle_end_date),
                    'BETWEEN'
                );
            }
            if ($filter_parms['board'])
                $query->condition('s.board_id', $filter_parms['board']);
            if ($filter_parms['topic'])
                $query->condition('s.topic_id', $filter_parms['topic']);

            // Using an INSERT ... FROM query is significantly more
            // efficient that looping through the article/topic
            // combinations inserting a row for each one at a time.
            $insert = db_insert('ebms_publish_queue_flag');
            $insert->from($query);
            $insert->execute();
            break;

        case 'set':
            try {
                db_insert('ebms_publish_queue_flag')
                    ->fields(
                        array(
                            'queue_id' => $queue_id,
                            'article_state_id' => $article_state_id,
                        )
                    )
                    ->execute();
            }
            catch (Exception $e) {
                // Shouldn't happen if this code is written correctly,
                // and the browser behaves itself. :-)
                $error = 'Checkbox already recorded';
            }
            break;

        case 'clear':

            // In contrast to the insert, the delete is safe to do more
            // than once.
            db_delete('ebms_publish_queue_flag')
                ->condition('queue_id', $queue_id)
                ->condition('article_state_id', $article_state_id)
                ->execute();
            break;
        default:
            $error = "Unsupported action '$action'";
            break;
    }
    drupal_json_output(array('error' => $error));
}
