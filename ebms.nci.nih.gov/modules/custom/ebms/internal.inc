<?php

/**
 * @file
 *
 * Interface for managing internal EBMS articles.  Added as part of work on
 * OCEEBMS-509.
 */

require_once('EbmsImport.inc');
require_once('EbmsArticle.inc');

function pdq_ebms_internal_articles($form, &$form_state, $request_id) {
    $start = $tstart = date('Y-m-d', strtotime('-1 month'));
    $selected_tags = array();
    $comment = '';
    $end = $tend = date('Y-m-d');
    if (!empty($request_id)) {
        $json = db_select('ebms_report_request', 'r')
            ->fields('r', array('parameters'))
            ->condition('r.request_id', $request_id)
            ->execute()
            ->fetchField();
        $request = drupal_json_decode($json);
        foreach ($request['tags'] as $key => $val) {
            if (!empty($val))
                $selected_tags[] = $key;
        }
        $start = $request['import-date-start']['date'] ?? '';
        $end = $request['import-date-end']['date'] ?? '';
        $tstart = $request['tag-date-start']['date'] ?? '';
        $tend = $request['tag-date-end']['date'] ?? '';
        $comment = $request['comment'];
    }

    $label = 'Import Date';
    $name = 'import-date';
    $date_field = Ebms\Util::date_range_field($label, $name, $start, $end,
                                              '', false);
    $label = 'Tag Date';
    $name = 'tag-date';
    $tdate_field = Ebms\Util::date_range_field($label, $name, $tstart, $tend,
                                               '', false);
    $results = db_select('ebms_internal_tag', 't')
        ->fields('t', array('tag_id', 'tag_name'))
        ->condition('t.active_status', 'A')
        ->orderBy('t.tag_name')
        ->execute();
    $tag_options = array();
    foreach ($results as $result)
        $tag_options[$result->tag_id] = htmlspecialchars($result->tag_name);
    $query = db_select('ebms_article', 'a')->distinct();
    $query->join('ebms_internal_article_tag', 't',
                 't.article_id = a.article_id');
    if (!empty($selected_tags))
        $query->condition('t.tag_id', $selected_tags, 'IN');
    if (!empty($comment)) {
        $query->join('ebms_internal_article_comment', 'c',
                     'c.article_id = a.article_id');
        $query->condition('c.comment_text', "%${comment}%", 'LIKE');
    }
    Ebms\Util::add_date_range_to_query($query, $start, $end, 'a.import_date');
    Ebms\Util::add_date_range_to_query($query, $tstart, $tend, 't.tag_added');
    $query->fields('a', array('article_id', 'import_date'));
    $results = $query->orderBy('a.import_date', 'DESC')->execute();
    $queue = _internal_articles_table($results);
    return array(
        'left_nav' => Ebms\Util::build_left_nav('Internal Articles'),
        'form' => array(
            '#prefix' => '<div id="internal-articles-filter-form">',
            '#suffix' => '</div>',
            'title' => array(
                '#markup' => '<h2>Internal Articles</h2>',
            ),
            'tags' => array(
                '#type' => 'checkboxes',
                '#options' => $tag_options,
                '#default_value' => $selected_tags,
                '#title' => 'Tags',
            ),
            'import-date' => $date_field,
            'tag-date' => $tdate_field,
            'comment' => array(
                '#type' => 'textfield',
                '#title' => 'Comment',
                '#default_value' => $comment,
            ),
            'save' => array(
                '#type' => 'submit',
                '#value' => 'Submit',
            ),
            'cancel' => array(
                '#type' => 'submit',
                '#value' => 'Reset',
                '#submit' => array('pdq_ebms_internal_articles_submit'),
                '#limit_validation_errors' => array(),
            ),
        ),
        'articles' => array(
            'title' => array('#markup' => '<h2>Articles</h2>'),
            'queue' => $queue,
        ),
    );
}

function _internal_articles_table($results) {
    $headers = array(
        array('data' => '', 'class' => array('col-0')),
        array('data' => 'Article', 'class' => array('col-1')),
        array('data' => 'Imported', 'class' => array('col-2')),
        array('data' => 'Tags', 'class' => array('col-3')),
        array('data' => 'Comments', 'class' => array('col-4')),
    );
    $rows = array();
    $n = 1;
    $target_opts = array('attributes' => array('target' => '_blank'));
    $title_opts = array('attributes' => array('class' => 'cite-title'));
    foreach ($results as $result) {
        $article_id = $result->article_id;
        $article = new Ebms\EbmsArticle($article_id);
        $display = $article->display();
        if (empty($display->authors))
            $authors = '<b>[No authors]</b>';
        else if (count($display->authors) > 3) {
            $authors = array_slice($display->authors, 0, 3);
            $authors[] = 'et al.';
            $authors = implode(', ', $authors);
        }
        else
            $authors = implode(', ', $display->authors);
        $url = "citations/full/$article_id";
        $title = htmlspecialchars($display->title);
        $title = l($title, $url, $title_opts);
        $publication = htmlspecialchars($display->publication);
        $links = array(l('VIEW ABSTRACT', $display->pubmed_url, $target_opts));
        if (!empty($display->ft_url))
            $links[] = l('DOWNLOAD FULL TEXT', $display->ft_url, $target_opts);
        $links = implode(' &nbsp; ', $links);
        $ids = "PMID: {$display->pubmed_id} &nbsp; EBMS: {$article_id}";
        $citation = "$authors<br>$title<br>$publication<br>$ids<br>$links";
        $citation = ['data' => $citation, 'class' => ['internal-cite']];
        $tags = array();
        foreach ($article->getInternalTags() as $id => $tag)
            $tags[] = htmlspecialchars($tag);
        $tags = implode('<br>', $tags);
        $comments = array();
        foreach ($article->getInternalComments() as $id => $comment)
            $comments[] = '<b>' . htmlspecialchars($comment->name) . '</b>: ' .
                htmlspecialchars($comment->comment_text);
        $comments = implode('<br><br>', $comments);
        $imported = substr($result->import_date, 0, 10);
        $rows[] = array(
            'no_striping' => true,
            'data' => array($n++, $citation, $imported, $tags, $comments),
        );
    }
    return array(
        '#theme' => 'table',
        '#rows' => $rows,
        '#header' => $headers,
    );
}

function pdq_ebms_internal_articles_validate($form, &$form_state) {
    $values = $form_state['values'];
    if ($values['op'] !== 'Submit')
        return;
    $start = $values['import-date-start']['date'];
    $end = $values['import-date-start']['date'];
    if (Ebms\Util::fill_in_date($start) > Ebms\Util::fill_in_date($end))
        form_set_error('import-date-end', 'End of range cannot precede start.');
}

function pdq_ebms_internal_articles_submit($form, &$form_state) {
    $values = $form_state['values'];
    $parms = $form_state['query-parms'] ?? drupal_get_query_parameters();
    $opts = array('query' => $parms);
    $url = 'internal-articles';
    if ($values['op'] === 'Submit') {
        global $user;
        $now = strftime('%Y-%m-%d %H:%M:%S');
        $request_id = db_insert('ebms_report_request')
            ->fields(
                array(
                    'report_name' => 'Internal',
                    'requestor_id' => $user->uid,
                    'submitted' => $now,
                    'parameters' => drupal_json_encode($values),
                )
            )
            ->execute();
        $url .= "/$request_id";
    }
    drupal_goto($url, $opts);
}

/**
 * Entry point for internal tag management.
 *
 *  @param  string   $action   one of 'view', 'edit', or 'add'
 *  @param  int      $tag_id   optional integer for editing an existing tag
 *
 *  @return array    Drupal render or form array
 */
function pdq_ebms_internal_tags($action='view', $tag_id=0) {
    $form_id = 'pdq_ebms_internal_tag_form';
    switch ($action) {
        case 'edit':
            return drupal_get_form($form_id, $tag_id);
        case 'add':
            return drupal_get_form($form_id);
        case 'view':
        default:
            return pdq_ebms_show_internal_tags();
    }
}

/**
 * Build a table of all of the topics in the EBMS, with links for editing
 * the existing topics or creating a new one.
 *
 *  @return   array         Drupal render array for table of tags
 */
function pdq_ebms_show_internal_tags() {
    $query = db_select('ebms_internal_tag', 't');
    $query->fields('t');
    $query->orderBy('t.tag_name');
    $results = $query->execute();
    $rows = array();
    foreach ($results as $result) {
        $row = array(
            htmlspecialchars($result->tag_name),
            $result->active_status == 'A' ? 'Active' : 'Inactive',
            l('Edit', 'admin/internal-tags/edit/' . $result->tag_id),
        );
        $rows[] = $row;
    }
    $headers = array(
        'Tag Name',
        'Status',
        'Action',
    );
    $button_class = array('class' => array('button'));
    $attrs = array('attributes' => $button_class);
    return array(
        'description' => array(
            '#markup' =>
                '<p>' .
                'These tags are used to identify articles of interest to ' .
                'internal staff (articles which are not necessarily ' .
                'intended to be added to the board member review process).' .
                '</p>',
        ),
        'new-link' => array(
            '#markup' => l('Add New Tag', 'admin/internal-tags/add', $attrs) .
            '<br /><br />',
        ),
        'table' => array(
            '#theme' => 'table',
            '#rows' => $rows,
            '#header' => $headers,
        ),
    );
}

/**
 * Drupal form API callback which generates the form for editing an
 * existing EBMS internal tag or creating a new one.
 *
 *  @param  array   $form         Structured array containing the
 *                                elements and properties of the form
 *  @param  array   &$form_state  Modifiable structured array
 *                                containing the current values
 *                                of the form, as well as other
 *                                form state information
 *  @param  int     $tag_id       Primary key for the tag being
 *                                edited, if we are not creating a
 *                                new tag
 *
 *  @return array                 Array containing specification
 *                                of the fields to appear on the
 *                                form, as well as other markup
 */
function pdq_ebms_internal_tag_form($form, &$form_state, $tag_id=0) {

    // Override Drupal's default breadcrumb mechanism.
    drupal_set_breadcrumb(
        array(
            l('Home', 'home'),
            l('Internal Tags', 'admin/internal-tags'),
        )
    );

    // Initialize local variables to their defaults.
    $name = '';
    $status = 'A';

    // Catch any changes to the form from the AJAX trigger.
    if (isset($form_state['values'])) {
        $values = $form_state['values'];
        $name = $values['name'];
        $status = $values['status'];
    }

    // If we're editing an existing tag, collect the current values.
    elseif ($tag_id) {
        $query = db_select('ebms_internal_tag', 't');
        $query->fields('t');
        $query->condition('t.tag_id', $tag_id);
        $tag = $query->execute()->fetch();
        $name = $tag->tag_name;
        $status = $tag->active_status;
    }

    // Assemble and return the form render array.
    $title = $tag_id ? 'Edit Tag' : 'Add Tag';
    return array(
        'title' => array(
            '#markup' => "<h2>$title</h2>",
        ),
        'tag' => array(
            '#type' => 'hidden',
            '#value' => $tag_id,
        ),
        'name' => array(
            '#type' => 'textfield',
            '#title' => 'Tag Name',
            '#required' => true,
            '#default_value' => $name,
            '#maxlength' => 255,
        ),
        'status' => array(
            '#type' => 'radios',
            '#title' => 'Status',
            '#options' => array('A' => 'Active', 'I' => 'Inactive'),
            '#default_value' => $status,
            '#required' => true,
        ),
        'save' => array(
            '#type' => 'submit',
            '#value' => 'Save',
        ),
        'cancel' => array(
            '#type' => 'submit',
            '#value' => 'Cancel',
            '#submit' => array('pdq_ebms_internal_tag_form_submit'),
            '#limit_validation_errors' => array(),
        ),
    );
}

/**
 * Make sure a unique tag name and a valid status value.
 *
 *  @param  array   $form         Structured array containing the
 *                                elements and properties of the form
 *  @param  array   $form_state   Structured array containing the
 *                                current values of the form, as well
 *                                as other form state information
 */
function pdq_ebms_internal_tag_form_validate($form, &$form_state) {
    $values = $form_state['values'];
    if ($values['op'] != 'Save')
        return;
    if ($values['status'] != 'A' && $values['status'] != 'I')
        form_set_error('status', 'Invalid status value for tag');
    $name = trim($values['name']);
    if (empty(trim($name)))
        form_set_error('name', 'You must give the tag a name');
    else {
        $tag_id = db_select('ebms_internal_tag', 't')
            ->fields('t', array('tag_id'))
            ->condition('t.tag_name', $name)
            ->execute()
            ->fetchField();
        if ($tag_id && $tag_id != $values['tag'])
            form_set_error('name', 'Tag name is already in use');
    }
}

/**
 * Store the t information if the user has clicked on the 'Save' button.
 *
 *  @param  array   $form         Structured array containing the
 *                                elements and properties of the form
 *  @param  array   $form_state   Structured array containing the
 *                                current values of the form, as well
 *                                as other form state information
 */
function pdq_ebms_internal_tag_form_submit($form, &$form_state) {
    $values = $form_state['values'];
    if ($values['op'] == 'Save') {
        $tag_id = $values['tag'];
        $name = $values['name'];
        $status = $values['status'];
        $fields = array(
            'tag_name' => $name,
            'active_status' => $status,
        );
        if (!$tag_id)
            $topic = db_insert('ebms_internal_tag')
                ->fields($fields)
                ->execute();
        else
            db_update('ebms_internal_tag')
                ->fields($fields)
                ->condition('tag_id', $tag_id)
                ->execute();
        drupal_set_message('Tag ' . htmlspecialchars($name) . ' saved.');
    }
    drupal_goto('admin/internal-tags');
}

/**
 * Import internal articles.
 *
 * Implemented separately from the main import page, because the requirements
 * for this type of import are so radically different (for example, these
 * articles don't have a board topic, which is normally required, and we
 * aren't putting anything in the state table).
 *
 * It was a tossup whether this should go here or in import.inc. It had to
 * go somewhere, and import.inc was already plenty big.
 *
 *  @param  array   $form         Structured array containing the
 *                                elements and properties of the form
 *  @param  array   $form_state   Structured array containing the
 *                                current values of the form, as well
 *                                as other form state information
 */
function pdq_ebms_import_internal_articles($request_id = 0) {
    $form_id = 'pdq_ebms_import_internal_articles_form';
    return array(
        'left_nav' => Ebms\Util::build_left_nav('Internal Import'),
        drupal_get_form($form_id, $request_id),
    );
}

function pdq_ebms_import_internal_articles_form($form, &$form_state,
                                                $request_id = 0) {

    // Set initial form values.
    $pmids = $comment = '';
    $selected_tags = [];
    if ($request_id) {
        $result = db_select('ebms_import_request', 'r')
            ->fields('r', array('request_params', 'report_data'))
            ->condition('r.request_id', $request_id)
            ->execute()
            ->fetchObject();
        $report = $result->report_data;
        $params = drupal_json_decode($result->request_params);
        $pmids = $params['pmid'];
        $selected_tags = $params['tags'];
        $comment = $params['comment'];
    }

    // Assemble list of internal tags which can be selected.
    $results = db_select('ebms_internal_tag', 't')
        ->fields('t', array('tag_id', 'tag_name'))
        ->condition('t.active_status', 'A')
        ->orderBy('t.tag_name')
        ->execute();
    $tags = array();
    foreach ($results as $result)
        $tags[$result->tag_id] = htmlspecialchars($result->tag_name);
    $form = array(
        '#id' => 'internal-import-form',
        'title' => array('#markup' => '<h2>Import Internal Articles</h2>'),
        'pmid' => array(
            '#type' => 'textfield',
            '#title' => 'PMID(s)',
            '#default_value' => $pmids,
            '#required' => true,
            '#description' => t('Separate multiple PMIDs with commas ' .
                                'or spaces.'),
        ),
        'comment' => array(
            '#type' => 'textfield',
            '#title' => 'Comment',
            '#default_value' => $comment,
            '#description' => t('Optionally add a comment related to the ' .
                                'relevance of this article to PDQ work.'),
        ),
        'tags' => array(
            '#type' => 'checkboxes',
            '#title' => 'Internal Tag(s)',
            '#options' => $tags,
            '#default_value' => $selected_tags,
            '#required' => true,
            '#description' => t('Select one or more internal article tags. ' .
                                'Additional tags can be added on the ' .
                                '"Full Citation" page.'),
        ),
        'full-text' => array(
            '#title' => 'Full Text PDF File',
            '#type' => 'file',
            '#upload_validators' => ['file_validate_extensions' => ['pdf']],
            '#description' => t('Only allowed if a single article is being ' .
                                'imported. Can also be added later from ' .
                                'the "Full Citation" page (coming soon).'),
        ),
        'submit' => array(
            '#type' => 'submit',
            '#value' => 'Import',
        ),
        'cancel' => array(
            '#type' => 'submit',
            '#value' => 'Clear',
            // It's a little whacky, since Drupal already knows how
            // to derive the submit callback's name from the name
            // of the form, but if you don't explicitly set this
            // property, the #limit_validation_errors property is
            // ignored. Sound like a bug to you? You're not alone. :-)
            '#submit' => 'pdq_ebms_import_internal_articles_form_submit',
            '#limit_validation_errors' => array(),
        ),
    );

    // If we just handled an import request, show the statistics from that job.
    if ($request_id) {
        $batch = drupal_json_decode($report);
        $batch = (object) $batch;
        $form['report'] = Ebms\Util::import_report($batch, 'Statistics', true);
    }
    return $form;
}

/**
 * Ensure that the import request meets our criteria.
 *
 *  @param  array   $form         Structured array containing the
 *                                elements and properties of the form
 *  @param  array   $form_state   Structured array containing the
 *                                current values of the form, as well
 *                                as other form state information
 */
function pdq_ebms_import_internal_articles_form_validate($form, &$form_state) {
    $value = $form_state['values']['pmid'];
    $pmids = preg_split('/[\s,]+/', $value, null, PREG_SPLIT_NO_EMPTY);
    foreach ($pmids as $pmid) {
        if (!preg_match('/^\d{1,8}$/', $pmid)) {
            form_set_error('pmid', 'Invalid Pubmed ID format.');
            break;
        }
    }
    if (empty($pmids))
        form_set_error('pmid', 'At least one Pubmed ID must be specified.');
    $active_tags = db_select('ebms_internal_tag', 't')
        ->fields('t', array('tag_id'))
        ->condition('t.active_status', 'A')
        ->execute()
        ->fetchCol();
    $tags = array();
    foreach ($form_state['values']['tags'] as $key => $val) {
        if (!empty($val)) {
            if (!in_array($key, $active_tags))
                form_set_error('tags', "$key is not a valid internal tag ID.");
            else
                $tags[] = $key;
        }
    }
    if (empty($tags)) {
        $message = 'At least one internal article tag must be selected.';
        form_set_error('tags', $message);
    }
    $form_state['full-text-id'] = null;
    if (!empty($_FILES['files']['name']['full-text'])) {
        if (count($pmids) > 1)
            form_set_error('full-text', 'Full-text PDF upload can only be ' .
                'used for importing a single article');
        else {
            $name = $_FILES['files']['name']['full-text'];
            $path = $_FILES['files']['tmp_name']['full-text'];
            $extension_ok = Ebms\Util::has_valid_extension($name, 'pdf');
            $mime_type = Ebms\Util::get_mime_type($path);
            if (!$extension_ok || $mime_type !== 'application/pdf')
                form_set_error('full-text', 'Full text must be a PDF file');
            else {
                $validators = ['file_validate_extensions' => ['pdf']];
                $file = file_save_upload('full-text', $validators);
                if ($file) {
                    $file = file_move($file, 'public://');
                    $file->status |= FILE_STATUS_PERMANENT;
                    $file = file_save($file);
                    $form_state['full-text-id'] = $file->fid;
                }
                else
                    form_set_error('full-text', "Unable to upload $name");
            }
        }
    }
    $form_state['pmids'] = $pmids;
    $form_state['tags'] = $tags;
    $form_state['comment'] = trim($form_state['values']['comment']);
}

/**
 * Perform the requested import.
 *
 *  @param  array   $form         Structured array containing the
 *                                elements and properties of the form
 *  @param  array   $form_state   Structured array containing the
 *                                current values of the form, as well
 *                                as other form state information
 */
function pdq_ebms_import_internal_articles_form_submit($form, &$form_state) {

    // To cancel, we just route the user to a fresh form.
    if ($form_state['values']['op'] == 'Clear') {
        drupal_goto('import-internal-articles');
        return;
    }

    // Use a canned comment for the import request.
    $pmids = $form_state['pmids'];
    $import_comment = 'Imported for internal PDQ staff use.';
    try {
        $batch = Ebms\importArticlesFromNLM('live', $pmids, null, null,
                                            $import_comment, false, 'I');
    }
    catch (\Exception $e) {
        drupal_set_message($e);
        drupal_goto('import-internal-articles');
        return;
   }

    // If we succeeded, store the statistical information about the job,
    // and do some population of related tables as appropriate.
    if ($batch) {

        // Get a list of articles affected by the import.
        $articles = array();
        foreach (['imported', 'duplicate'] as $disposition) {
            foreach ($batch->action[$disposition] as $info) {
                list($pmid, $article_id) = $info;
                $message = "Imported PMID $pmid as EBMS $article_id";
                drupal_set_message($message);
                $articles[] = $article_id;
            }
        }

        // Record the location of the full-text PDF file (OCEEBMS-472).
        // Validation has already ensured that at most one article
        // has been imported (duplicate or not).
        $full_text_id = $form_state['full-text-id'];
        if (!empty($full_text_id)) {
            foreach ($articles as $article_id) {
                db_update('ebms_article')
                    ->fields(array('full_text_id' => $full_text_id))
                    ->condition('article_id', $article_id)
                    ->execute();
            }
        }

        // Store the internal tags with each article.
        $table = 'ebms_internal_article_tag';
        foreach($form_state['tags'] as $tag_id) {
            $fields = ['tag_id' => $tag_id];
            foreach ($articles as $article_id) {
                $fields['article_id'] = $article_id;
                db_insert($table)->fields($fields)->execute();
            }
        }

        // Store the comment with the articles, not the import batch.
        $comment = $form_state['comment'];
        if (!empty($comment)) {
            global $user;
            $now = strftime("%Y-%m-%d %H:%M:%S");
            $table = 'ebms_internal_article_comment';
            $fields = array(
                'user_id' => $user->uid,
                'comment_date' => $now,
                'comment_text' => $comment,
            );
            foreach ($articles as $article_id) {
                $fields['article_id'] = $article_id;
                db_insert($table)->fields($fields)->execute();
            }
        }

        // Save the statistical report information.
        $report_data = drupal_json_encode($batch);
        $request_params = drupal_json_encode($form_state['values']);
        $request_id = db_insert('ebms_import_request')
            ->fields(
                array(
                    'request_params' => $request_params,
                    'report_data' => $report_data,
                )
            )
            ->execute();
        $url = "import-internal-articles/$request_id";
    }

    // Draw the form again.
    drupal_goto($url);
}
