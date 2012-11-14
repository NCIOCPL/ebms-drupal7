<?php

// $Id$

/**
 * @file
 *
 * Implementation of EBMS page for uploading full-text PDFs.
 */
function pdq_ebms_fulltext() {
    $control = new EbmsFullText();
    return $control->run();
}

/*
 * Wrapping as much functionality within a class in order to simplify passing
 * around state information for processing the current request.  Also makes
 * it easier to avoid polluting the global namespace.
 */
class EbmsFullText {

    public static $url = 'citations/fulltext';

    /*
     * Show the articles which need full text uploaded.
     */
    public function run() {

        Ebms\Util::add_js();
        drupal_set_breadcrumb(array('Citation Management'));
        drupal_add_css(Ebms\CSS_DIR . '/fulltext.css');
        Ebms\Menu::$active = 'Citation Management';
        return drupal_get_form('pdq_ebms_fulltext_form');
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
        $pieces[] = ' &nbsp; EBMS ID: ' . l($article_id,
                    'citations/full/' . $article_id);

        // XXX Add real link when Dan's finished his current story.
        $pieces[] = '<br /><br />' . l('View Abstract', 'stub');
        return implode($pieces);
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
    public static function build_left_nav($active=null) {
        $links = array(
            array('Search Database', 'citations/search'),
            array('Import Citations', 'citations/import'),
            array('Not List Maintenance', 'citations/not-list'),
        );
        $items = array();
        foreach ($links as $link_info) {
            list($label, $url) = $link_info;
            $link = l($label, $url);
            $class = $label == $active ? 'active' : 'inactive';
            $items[] = array(
                'data' => $link,
                'class' => array($class),
            );
        }
        return array(
            '#type' => 'container',
            '#attributes' => array('id' => 'left-nav'),
            'nav' => array(
                '#theme' => 'item_list',
                '#items' => $items,
            ),
        );
    }
}

function pdq_ebms_fulltext_form_submit($form, &$form_state) {
    if ($form_state['values']['op'] != 'Reset')
        $form_state['rebuild'] = true;
    if ($form_state['values']['op'] != 'Submit')
        return;
    $validators = array('file_validate_extensions' => array());
    foreach ($_FILES['files']['name'] as $key => $value) {
        list($prefix, $article_id) = explode('-', $key);
        if ($value) {
            $file = file_save_upload($key, $validators);
            if ($file) {
                $file = file_move($file, 'public://');
                $file->status |= FILE_STATUS_PERMANENT;
                $file = file_save($file);
                db_update('ebms_article')
                    ->fields(array('full_text_id' => $file->fid))
                    ->condition('article_id', $article_id)
                    ->execute();
                drupal_set_message('Posted document ' . $file->filename .
                                   ' for article ' . $article_id . '.');
            }
            else
                drupal_set_message('Unable to upload ' . $value, 'error');
        }
        elseif ($form_state['values']['unavailable-' . $article_id]) {
            global $user;
            $now = strftime("%Y-%m-%d %H:%M:%S");
            try {
                db_insert('ebms_ft_unavailable')
                    ->fields(
                        array(
                            'article_id' => $article_id,
                            'flagged' => $now,
                            'flagged_by' => $user->uid,
                        )
                    )
                    ->execute();
                drupal_set_message('Marked article ' . $article_id .
                    ' as unavailable');
            }
            catch (Exception $e) {
                // Shouldn't happen if this code is written correctly.
                drupal_set_message('Unavailable flag already set for article '
                    . $article_id, 'error');
            }
        }
    }
    pdq_ebms_debug('FULLTEXT FORM SUBMIT',
                   array('VALUES' => $form_state['values'],
                         'FILES' => $_FILES,
                         )
                   );
}

function pdq_ebms_fulltext_form($form, &$form_state) {
    $board = $cycle = 0;
    if (isset($form_state['values'])) {
        $board = $form_state['values']['board'];
        $cycle = $form_state['values']['cycle'];
    }
    $boards = array(0 => ' ') + Ebms\Util::boards();
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
        'left-nav' => EbmsFullText::build_left_nav(),
        'filter-fields' => array(
            '#type' => 'container',
            '#attributes' => array('id' => 'fulltext-filter'),
            'title' => array(
                '#type' => 'markup',
                '#markup' => '<h2>Citation Management</h2>',
            ),
            'board' => array(
                '#type' => 'select',
                '#title' => 'Editorial Board',
                '#title_display' => 'after',
                '#options' => $boards,
                '#default_value' => $board,
            ),
            'cycle' => array(
                '#type' => 'select',
                '#title' => 'Review Cycle',
                '#title_display' => 'after',
                '#options' => $cycles,
                '#default_value' => $cycle,
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
    $passed_bm_review = db_select('ebms_article_state_type', 't')
        ->fields('t', array('state_id'))
        ->condition('t.state_text_id', 'PassedBMReview')
        ->execute()
        ->fetchField();
    $query = db_select('ebms_article', 'a');
    $query->fields('a', array('article_id'));
    $query->join('ebms_article_state', 's', 'a.article_id = s.article_id');
    $query->leftJoin('ebms_ft_unavailable', 'u',
        'u.article_id = a.article_id');
    $query->condition('s.current', 'Y');
    $query->isNull('a.full_text_id');
    $query->isNull('u.article_id');
    $query->distinct();
    $query->condition('s.state_id', $passed_bm_review);
    if ($board)
        $query->condition('s.board_id', $board);
    if ($cycle) {
        $cycle_query = db_select('ebms_cycle', 'c');
        $cycle_query->fields('c', array('start_date'));
        $cycle_query->addExpression(
            'DATE_ADD(c.start_date, INTERVAL 1 MONTH)',
            'end_date'
        );
        $cycle_query->condition(
            'c.cycle_id',
            $cycle
        );
        $cycle_start_date = $cycle_end_date = null;
        $results = $cycle_query->execute();
        foreach ($results as $result) {
            $cycle_start_date = $result->start_date;
            $cycle_end_date = $result->end_date;
        }
        if ($cycle_start_date)
            $query->condition(
                's.status_dt',
                array($cycle_start_date, $cycle_end_date),
                'BETWEEN'
            );
    }
    else
        $query->condition('s.status_dt', Ebms\CONVERSION_DATE, '>');
    $query->range(0, 100);
    $query->orderBy('s.status_dt');
    $article_ids = array();
    $results = $query->execute();
    foreach ($results as $result)
        $article_ids[] = $result->article_id;
    $n = count($article_ids);
    $form['bottom'] = array(
        'title' => array(
            '#type' => 'markup',
            '#markup' =>
            "<h2>Abstracts Requiring Full Text Retrieval ($n)</h2>",
        ),
    );
    if ($n > 0) {
        $form['bottom']['submit-1'] = array(
            '#type' => 'submit',
            '#value' => 'Submit',
        );
        $form['bottom']['articles'] = array(
            '#type' => 'container',
            '#attributes' => array('id' => 'fulltext-articles'),
            //'#tree' => true,
        );
        $form['bottom']['submit-2'] = array(
            '#type' => 'submit',
            '#value' => 'Submit',
        );
//         'table' => array(
//             '#prefix' => '<div id="fulltext-table">',
//             '#suffix' => '</div>',
//             '#tree' => true,
//             '#theme' => 'table',
//             '#header' => array('CITATIONS', 'BOARDS', 'FULL TEXT', ''),
//             '#rows' => array(),
//         ),
//    );
        $counter = 1;
        $items = array();
        foreach ($article_ids as $article_id) {
            $citation = EbmsFullText::format_citation($article_id, $counter);
            $results = db_select('ebms_article_state', 's')
                ->fields('s', array('board_id'))
                ->condition('s.current', 'Y')
                ->condition('s.state_id', $passed_bm_review)
                ->condition('s.article_id', $article_id)
                ->distinct()
                ->execute();
            $article_boards = array();
            foreach ($results as $result) {
                if (array_key_exists($result->board_id, $boards))
                    $article_boards[] = $boards[$result->board_id];
            }
            sort($article_boards);
            $form['bottom']['articles'][] = array(
                'article' => array(
                    '#type' => 'markup',
                    '#markup' => '<div class="citation">' .
                    '<div class="article-counter">' . $counter . '</div>' .
                    $citation . '</div>',
                ),
                'boards' => array(
                    '#type' => 'markup',
                    '#markup' => '<p class="boards">Boards: ' .
                    implode('; ', $article_boards) . '</p>',
                ),
                'fields' => array(
                    '#prefix' => '<div class="article-fields">',
                    '#suffix' => '</div>',
                    'pdf-' . $article_id => array(
                        '#type' => 'file',
                        '#id' => 'pdf-' . $article_id,
                    ),
                    'unavailable-' . $article_id => array(
                        '#type' => 'checkbox',
                        '#id' => 'unavailable-' . $article_id,
                        '#title' => 'Unable to Upload',
                    ),
                ),
            );
            ++$counter;
        }
    }
    else
        $form['nothing'] = array(
            '#type' => 'markup',
            '#markup' =>
            '<p id="nothing">' .
            'No articles found requiring full text retrieval.</p>',
        );
    return $form;
}

//     $data = array(array('A', 'B'), array('C', 'D'), array('E', 'F'));
//     $counter = 1;
//     foreach ($data as $datum) {
//         $file_field = array(
//             '#type' => 'file',
//             '#id' => 'pdf-' . $counter,
//             '#size' => 10,
//         );
//         $checkbox = array(
//             '#type' => 'checkbox',
//             '#id' => 'unable-' . $counter++,
//             '#title' => 'Unable to retrieve file',
//         );
//         $form['bottom']['table'][] = array(
//             'file' => &$file_field,
//             'unable' => &$checkbox,
//         );
//         $form['bottom']['table']['#rows'][] = array(
//             'no_striping' => true,
//             'data' => array(
//                 array(
//                     'data' => $datum[0],
//                     'class' => array('col-1'),
//                 ),
//                 array(
//                     'data' => $datum[1],
//                     'class' => array('col-2'),
//                 ),
//                 array(
//                     'data' => &$file_field,
//                     'class' => array('col-3'),
//                 ),
//                 array(
//                     'data' => &$checkbox,
//                     'class' => array('col-4'),
//                 ),
//             ),
//         );
//         unset($file_field);
//         unset($checkbox);
//     }

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
