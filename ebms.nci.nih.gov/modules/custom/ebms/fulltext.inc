<?php

/**
 * @file
 *
 * Implementation of EBMS page for uploading full-text PDFs.
 */
function pdq_ebms_fulltext($request_id=null) {
    $control = new EbmsFullText();
    return $control->run($request_id);
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
    public function run($request_id) {

        Ebms\Util::add_js();
        drupal_set_breadcrumb(array('Citation Management'));
        drupal_add_css(Ebms\CSS_DIR . '/fulltext.css');
        Ebms\Menu::$active = 'Citation Management';
        return drupal_get_form('pdq_ebms_fulltext_form', $request_id);
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
        //$title = htmlspecialchars($article->ArticleTitle);
        // Changed by TIR 2458.
        $title = l($article->ArticleTitle, 'citations/full/' . $article_id);
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
        $pieces[] = ' &nbsp; EBMS ID: ' . $article_id;

        // Show related articles (OCEEBMS-358).
        $related = Ebms\Util::get_related_articles($article_id);
        if ($related)
            $pieces[]= "<br>$related";

        // Link to NLM's abstract for the article.
        $url = Ebms\Util::pubmed_url($pmid);
        $target = array('attributes' => array('target' => '_blank'));
        $pieces[] = '<br /><br />' . l('VIEW ABSTRACT', $url, $target);
        return implode($pieces);
    }
}

/**
 * Drupal form API callback which generates the form used to filter the
 * queue of articles awaiting upload of full text.  The queue itself
 * is also included below the form fields.  See the description of
 * the submit callback below for more details.
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
function pdq_ebms_fulltext_form($form, &$form_state, $request_id) {

    // Build the picklists for PDQ boards and review cycles.
    $board = $cycle = $topic = 0;
    $errors = array();
    $parms = drupal_get_query_parameters();
    $page = 0;
    if (isset($parms['page']))
        $page = $parms['page'];
    if (isset($form_state['values'])) {
        $values = $form_state['values'];
        $board = $values['board'];
        $cycle = $values['cycle'];
        $topic = $values['topic'];
    }
    elseif ($request_id) {
        $json = db_select('ebms_search', 's')
            ->fields('s', array('search_spec'))
            ->condition('s.search_id', $request_id)
            ->execute()
            ->fetchField();
        $request = drupal_json_decode($json);
        $board = $request['board'];
        $cycle = $request['cycle'];
        $topic = $request['topic'];
        $errors = $request['errors'];
    }

    $boards = array(0 => ' ') + Ebms\Util::boards();
    $cycles = array(0 => ' ');
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
        'left-nav' => Ebms\Util::build_left_nav('Full Text Retrieval'),
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
                '#ajax' => array(
                    'callback' => '_fulltext_form_board_select_ajax',
                    'wrapper' => 'topic-select-div',
                ),
            ),
            'topic' => array(
                '#type' => 'select',
                '#title' => 'Summary Topic',
                '#title_display' => 'after',
                '#options' => $topics,
                '#default_value' => $topic,
                '#prefix' => '<div id="topic-select-div">',
                '#suffix' => '</div>',
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
    $query->condition('s.current', 'Y');
    $query->condition('s.state_id', $passed_bm_review);
    $query->isNull('a.full_text_id');
    $query->distinct();
    if ($board)
        $query->condition('s.board_id', $board);
    if ($topic)
        $query->condition('s.topic_id', $topic);
    if ($cycle) {
        $query->join('ebms_article_topic', 't',
            't.article_id = s.article_id AND t.topic_id = s.topic_id');
        $query->condition('t.cycle_id', $cycle);
    }
    else
        $query->condition('s.status_dt', Ebms\CONVERSION_DATE, '>');
    $query->leftJoin('ebms_ft_unavailable', 'u',
        'u.article_id = a.article_id');
    $query->isNull('u.article_id');
    $counter = $query->countQuery();
    $count = $counter->execute()->fetchField();
    $query->orderBy('s.status_dt');

    // Run the query and build an array of the article IDs.
    $per_page = 10;
    $pos = 1 + $page * $per_page;
    $pager = false;
    if ($count > $per_page) {
        $query = $query->extend('PagerDefault')->limit($per_page);
        $pager = true;
    }
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
            "<h2>Abstracts Requiring Full Text Retrieval ($count)</h2>",
        ),
    );

    // Build a list of articles in the queue if there are any
    if ($n > 0) {

        // Put some general action buttons at the top of the queue.
        if ($pager)
            $form['bottom']['pager-top'] = array('#theme' => 'pager');
        $form['bottom']['page'] = array(
            '#type' => 'hidden',
            '#value' => $page
        );
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
        $counter = $pos;

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
            $pdf_key = "pdf-$article_id";
            $file_class = array('form-file');
            if (in_array($pdf_key, $errors))
                $file_class[] = 'error';
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
                        '#attributes' => array('class' => $file_class),
                    ),
                    /*
                    'unavailable-' . $article_id => array(
                        '#type' => 'checkbox',
                        '#id' => 'unavailable-' . $article_id,
                        '#title' => 'Unable to Upload',
                    ),
                */
                ),
            );
            ++$counter;
        }
        if ($pager)
            $form['bottom']['pager-bottom'] = array('#theme' => 'pager');
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
 * Dynamically update the topic picklist when the selected board changes.
 */
function _fulltext_form_board_select_ajax($form, $form_state) {
    return $form['top']['filter-fields']['topic'];
}

/**
 * Store the uploaded files for full text using Drupal's managed files
 * mechanism, and mark the ones for which the user says she was unable
 * to obtain the full text.  When we're done we rebuild the form, and
 * the queue is redrawn, leaving off the ones we just took care of.
 *
 * 2013-02-20: Extensively rewritten to handle TIR 2389.  We now
 * need to ensure that only PDF files are uploaded for the articles'
 * full text documents; otherwise the printing routines fail in an
 * unacceptable way.  We can't use Drupal's built-in file type checking,
 * partly because it's only checking the file name's extension, and
 * mainly because when one of the files fails this test, the entire
 * form submission is put by Drupal in a state which prevents our
 * code from refreshing the queue, eliminating the articles for which
 * the file upload succeeded.  In order to ensure that we are always
 * able to construct the queue afresh, and at the same time preserve
 * the user's filtering values, we store the current request in the
 * search table.  We include, along with those filtering values,
 * the field IDs for failed file uploads, so we can mark them with
 * the styling which identifies them as requiring attention.  Instead
 * of calling form_set_error(), we use drupal_set_message().  At the
 * bottom of the submit() callback we navigate directly to the full
 * text upload queue page with the ID of the row we just put in the
 * search table.  The only time we don't use the search table is when
 * the user clicks the Reset button.
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

    // Find out which button was pushed.
    $values = $form_state['values'];
    $op = $values['op'];
    if ($op == 'Reset')
        drupal_goto(EbmsFullText::$url);

    // Collect the values we'll remember when we re-build the queue.
    global $user;
    $now = strftime("%Y-%m-%d %H:%M:%S");
    $board = $values['board'];
    $cycle = $values['cycle'];
    $topic = $values['topic'];
    $page = $values['page'];
    $request = array(
        'type' => 'fulltext queue',
        'board' => $board,
        'cycle' => $cycle,
        'topic' => $topic,
        'errors' => array(),
    );

    // Only process the fields for the Submit button (not the Filter button).
    if ($op == 'Submit') {
        $files = $_FILES['files'];

        // Suppress the default checking; we'll do our own.
        $validators = array('file_validate_extensions' => array());
        foreach ($files['name'] as $key => $value) {
            list($prefix, $article_id) = explode('-', $key);

            // Do we have a file for this article?
            if ($value) {

                // Is it the right type?
                $extension_ok = Ebms\Util::has_valid_extension($value, 'pdf');
                $mtype = Ebms\Util::get_mime_type($files['tmp_name'][$key]);
                if (!$extension_ok || $mtype !== 'application/pdf') {
                    drupal_set_message('<i>' . htmlspecialchars($value) .
                        '</i> not uploaded (not a PDF file).', 'error');
                    $request['errors'][] = $key;
                    continue;
                }

                // Yes: hook it up with the article table.
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

            // No file; how about the "unavailable" flag?
            /*
            elseif ($form_state['values']['unavailable-' . $article_id]) {
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
                        ' as unavailable.');
                }
                catch (Exception $e) {
                    // Shouldn't happen if this code is written correctly.
                    drupal_set_message('Unavailable flag already set for ' .
                        'article ' . $article_id . '.', 'error');
                }
            }
            */
        }
    }

    // This is the only way we can communicate with our form-building
    // callback in a way that meets all the requirements.
    $request_id = db_insert('ebms_search')
        ->fields(
            array(
                'when_searched' => $now,
                'searched_by' => $user->uid,
                'search_spec' => drupal_json_encode($request),
            )
        )
        ->execute();
    $url = EbmsFullText::$url . '/' . $request_id;
    $opts = array();
    if ($page)
        $opts['query'] = array('page' => $page);
    drupal_goto($url, $opts);
}
