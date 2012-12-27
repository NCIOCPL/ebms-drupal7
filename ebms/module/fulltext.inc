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

        // Link to NLM's abstract for the article.
        $url = 'http://www.ncbi.nlm.nih.gov/pubmed/' . $pmid;
        $target = array('attributes' => array('target' => '_blank'));
        $pieces[] = '<br /><br />' . l('View Abstract', $url, $target);
        return implode($pieces);
    }
}

/**
 * Drupal form API callback which generates the form used to filter the
 * queue of articles awaiting upload of full text.  The queue itself
 * is also included below the form fields.
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
function pdq_ebms_fulltext_form($form, &$form_state) {

    // Build the picklists for PDQ boards and review cycles.
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

    // Specify the fields to be used for filtering the queue.
    $form['top'] = array(
        '#type' => 'container',
        '#attributes' => array('id' => 'fulltext-top'),
        'left-nav' => Ebms\Util::build_left_nav(),
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

    // Construct the query for finding the articles which belong in
    // the queue (no more than 100 at a time).
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

    // Run the query and build an array of the article IDs.
    $article_ids = array();
    $results = $query->execute();
    foreach ($results as $result)
        $article_ids[] = $result->article_id;

    // Add the title of the queue below the filtering form fields.
    $n = count($article_ids);
    $form['bottom'] = array(
        'title' => array(
            '#type' => 'markup',
            '#markup' =>
            "<h2>Abstracts Requiring Full Text Retrieval ($n)</h2>",
        ),
    );

    // Build a list of articles in the queue if there are any
    if ($n > 0) {

        // Put some general action buttons at the top of the queue.
        $form['bottom']['submit-1'] = array(
            '#type' => 'submit',
            '#value' => 'Submit',
        );
        $form['bottom']['articles'] = array(
            '#type' => 'container',
            '#attributes' => array('id' => 'fulltext-articles'),
        );
        $form['bottom']['submit-2'] = array(
            '#type' => 'submit',
            '#value' => 'Submit',
        );
        $counter = 1;

        // Assemble the list items.
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

    // If there are no articles in the queue, say so.
    else
        $form['nothing'] = array(
            '#type' => 'markup',
            '#markup' =>
            '<p id="nothing">' .
            'No articles found requiring full text retrieval.</p>',
        );
    return $form;
}

/**
 * Store the uploaded files for full text using Drupal's managed files
 * mechanism, and mark the ones for which the user says she was unable
 * to obtain the full text.  When we're done we rebuild the form, and
 * the queue is redrawn, leaving off the ones we just took care of.
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
}
