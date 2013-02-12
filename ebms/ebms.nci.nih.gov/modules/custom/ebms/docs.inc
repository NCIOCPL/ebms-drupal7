<?php

// $Id$

/**
 * @file
 *
 * Implementation of EBMS page for posting documents.
 */

/**
 * Callback registered for /docs pages.
 */
function pdq_ebms_docs($request=null, $doc_id=0) {
    $docs = new EbmsDocs($request, $doc_id);
    return $docs->run();
}

/**
 * Wrapper for logic to handle document posting/editing pages.  Keeps our
 * functions out of the global namespace with less ugly syntax than the
 * PHP namespace mechanism provides, and lets us pass around the current
 * state information more easily.
 */
class EbmsDocs {

    public
        $user,
        $request,
        $doc_id,
        $parms;
    public static $url = 'docs';

    /**
     * Passing settings in the URL instead of in $_SESSION in order
     * to avoid dependency on Javascript being enabled on the user's
     * browser.
     */
    public function __construct($request, $doc_id) {
        global $user;
        $this->user   = user_load($user->uid);
        $this->request = $request;
        $this->doc_id = $doc_id;
        $this->parms  = drupal_get_query_parameters();
    }

    /**
     * Show the doc table or form for adding/editing a single document.
     */
    public function run() {
        switch ($this->request) {
            case 'post':
                return drupal_get_form('pdq_ebms_post_doc_form', $this);
            case 'edit':
                return drupal_get_form('pdq_ebms_post_doc_form', $this,
                    $this->doc_id);
            case 'delete':
                return $this->del_doc();
            default:
                return $this->show_docs();
        }
    }

    /**
     * Marks an uploaded document as inactive, then re-routes
     * processing to show the uploaded document table.
     */
    function del_doc() {
        db_update('ebms_doc')
            ->fields(array('drop_flag' => 1))
            ->condition('doc_id', $this->doc_id)
            ->execute();
        drupal_goto(EbmsDocs::$url, array('query' => $this->parms));
    }

    /**
     * Builds the table displaying uploaded documents.  Table is paged
     * by default (this can be suppressed) and sortable by each of the
     * first three columns.
     */
    function show_docs() {

        // Only managers see these pages, so we can assume Javascript
        // is enabled.
        Ebms\Util::add_js();

        // Find out whether we're paging the results.
        $pager = true;
        if (isset($this->parms['pager']))
            if ($this->parms['pager'] == 'off')
                $pager = false;

        // Override Drupal's default breadcrumbs to match requirements.
        drupal_set_breadcrumb(
            array(
                l('Home', 'home'),
                'Post a Document',
            )
        );

        // Set up the table headers, identifying DB columns for sorting.
        $headers = array(
            array(
                'data' => 'File Name',
                'field' => 'f.filename',
            ),
            array(
                'data' => 'Uploaded By',
                'field' => 'u.name',
            ),
            array(
                'data' => 'Date Uploaded',
                'field' => 'd.when_posted',
            ),
            array(
                'data' => 'Actions',
                'class' => array('col-4'),
            ),
        );

        // Build and execute the database query to find the posted documents.
        $query = db_select('ebms_doc', 'd');
        if ($pager)
            $query = $query->extend('PagerDefault')->limit(10);
        $query = $query->extend('TableSort');
        $query->join('file_managed', 'f', 'f.fid = d.file_id');
        $query->join('users', 'u', 'u.uid = f.uid');
        $query->fields('f', array('filename'));
        $query->fields('d', array('when_posted', 'doc_id'));
        $query->fields('u', array('name'));
        $query->condition('d.drop_flag', 0);
        $query->orderByHeader($headers);
        $docs = $query->execute();

        // Collect information we'll use repeatedly in the loop.
        $divider = '<span class="action-divider">|</span>';
        $opts = array('query' => $this->parms);
        $main_url = EbmsDocs::$url;
        $del_func = 'javascript:ebmsscript.delete_doc';

        // Build the array of table rows.
        $rows = array();
        foreach ($docs as $d) {
            $edit_link = l('EDIT', "$main_url/edit/{$d->doc_id}", $opts);
            $del_url = url("$main_url/delete/{$d->doc_id}", $opts);
            $del_name = htmlspecialchars($d->filename);
            $del_name = str_replace("'", "\\'", $del_name);
            $del_url = "$del_func('$del_url','$del_name');";
            $del_link = '<a href="' . $del_url . '">DELETE</a>';
            $data = array(
                htmlspecialchars($d->filename),
                htmlspecialchars($d->name),
                date('m/d/Y', strtotime($d->when_posted)),
                "$edit_link $divider $del_link",
            );
            $rows[] = array(
                'data' => $data,
                'no_striping' => true,
            );
        }

        // Assemble the table.
        $add = l('Post Document', EbmsDocs::$url . '/post', $opts);
        $title = 'Documents';
        $id = 'documents-title';
        $table = array(
            'title' => array(
                '#type' => 'markup',
                '#markup' => "<h2 id='$id'>$title $add</h2>",
            ),
            'table' => array(
                '#attributes' => array('id' => 'documents-table'),
                '#theme' => 'table',
                '#rows' => $rows,
                '#header' => $headers,
            ),
            'delete-confirm' => array(
                '#type' => 'container',
                '#attributes' => array(
                    'id' => 'confirm-doc-delete',
                    'title' => 'Are you sure you want to delete?',
                ),
                'confirm-message' => array(
                    '#type' => 'markup',
                    '#markup' =>
                    '<p id="confirmation-filename"></p>',
                ),
            ),
        );

        // Add paging controls if appropriate.
        if ($pager)
            $table['pager'] = array('#theme' => 'pager');
        else {
            unset($opts['query']['pager']);
            $url = url(EbmsDocs::$url, $opts);
            $link = l('VIEW PAGES', EbmsDocs::$url, $opts);
            $table['pager'] = array(
                '#type' => 'markup',
                '#markup' => "<div id='pager-view-pages'>$link</div>",
            );
        }
        return $table;
    }
}

/**
 * Validation function to ensure that a document was selected for a file
 * upload field (Drupal's validation of required file upload fields is
 * broken).
 */
function pdq_ebms_doc_file_required($element, &$form_state) {
    if ($_FILES['files']['name']['filepath'] == '') {
        form_set_error('filepath', t('Please select a document'));
    }
}

/**
 * Object to collect information about a single posted EBMS document.
 */
class EbmsDoc {
    public
        $id,
        $name,
        $uri,
        $desc,
        $boards,
        $tags,
        $topics;

    /**
     * Collects information about this document from the related database
     * tables.
     */
    public function __construct($doc_id) {
        $this->id = $doc_id;
        $this->name = $this->uri = $this->desc = null;
        $this->boards = EbmsDoc::fetch_boards($doc_id);
        $this->tags = EbmsDoc::fetch_tags($doc_id);
        $this->topics = EbmsDoc::fetch_topics($doc_id);
        $query = db_select('ebms_doc', 'd');
        $query->join('file_managed', 'f', 'f.fid = d.file_id');
        $query->fields('d', array('description'));
        $query->fields('f', array('filename', 'uri'));
        $query->condition('d.doc_id', $doc_id);
        $results = $query->execute();
        foreach ($results as $result) {
            $this->name = $result->filename;
            $this->uri = $result->uri;
            $this->desc = $result->description;
        }
    }

    /**
     * Builds an array of the primary keys for the boards associated
     * with this document.
     */
    public static function fetch_boards($doc_id) {
        $boards = array();
        $results = db_select('ebms_doc_board', 'b')
            ->fields('b', array('board_id'))
            ->condition('b.doc_id', $doc_id)
            ->execute();
        foreach ($results as $result)
            $boards[] = $result->board_id;
        return $boards;
    }

    /**
     * Builds an array of the primary keys for the tags associated
     * with this document.
     */
    public static function fetch_tags($doc_id) {
        $tags = array();
        $results = db_select('ebms_doc_tag', 't')
            ->fields('t', array('tag_id'))
            ->condition('t.doc_id', $doc_id)
            ->execute();
        foreach ($results as $result)
            $tags[] = $result->tag_id;
        return $tags;
    }

    /**
     * Builds an array of the primary keys for the topics associated
     * with this document.
     */
    public static function fetch_topics($doc_id) {
        $topics = array();
        $results = db_select('ebms_doc_topic', 't')
            ->fields('t', array('topic_id'))
            ->condition('t.doc_id', $doc_id)
            ->execute();
        foreach ($results as $result)
            $topics[] = $result->topic_id;
        return $topics;
    }
}

/**
 * Callback to construct the form used to post a new document or edit
 * an already posted document.
 */
function pdq_ebms_post_doc_form($form, &$form_state, $control, $doc_id=0) {

    // Start out with values appropriate for a newly posted document.
    $checked_boards = array();
    $checked_tags = array();
    $summary_tag_checked = false;
    $summary_tag = 0;

    // Build the associative array of available tags.
    $tags = array();
    $results = db_select('ebms_tag', 't')
        ->fields('t', array('tag_id', 'tag_name'))
        ->orderBy('t.tag_name')
        ->execute();
    foreach ($results as $result) {
        $tags[$result->tag_id] = $result->tag_name;
        if ($result->tag_name == 'summary')
            $summary_tag = $result->tag_id;
    }

    // Do the same for the available boards.
    $boards = array();
    $results = db_select('ebms_board', 'b')
        ->fields('b', array('board_id', 'board_name'))
        ->orderBy('b.board_name')
        ->execute();
    foreach ($results as $result)
        $boards[$result->board_id] = $result->board_name;

    // If we are editing an existing document, collect its values.
    if ($doc_id) {
        $doc = new EbmsDoc($doc_id);
        $title = 'Edit Document - ' . htmlspecialchars($doc->name);
        $desc_default = $doc->desc;
        $boards_default = $checked_boards = $doc->boards;
        $tags_default = $doc->tags;
        $topics_default = $doc->topics;
        if (in_array($summary_tag, $doc->tags))
            $summary_tag_checked = true;
    }
    else {
        $title = 'Post Document';
        $boards_default = $tags_default = $topics_default = array();
        $desc_default = '';
    }

    // Override the Drupal breadcrumb mechanism to match requirements.
    drupal_set_breadcrumb(
        array(
            l('Home', 'home'),
            $title,
        )
    );

    // Don't lose what the user has already selected.
    if (isset($form_state['values'])) {
        //pdq_ebms_debug('DOC FORM WITH VALUES', $form_state['values']);
        foreach ($form_state['values']['boards'] as $id => $val)
            if ($val)
                $checked_boards[] = $val;
        foreach ($form_state['values']['tags'] as $id => $val)
            if ($val == $summary_tag)
                $summary_tag_checked = true;
        $query_parms = $form_state['values']['query-parms'];
    }
    else {
        $query_parms = drupal_json_encode($control->parms);
        // pdq_ebms_debug('DOC FORM NO VALUES', $form_state);
    }

    // Begin form assembly using common fields.
    $form = array(
        '#id' => 'post-doc-form',
        'doc' => array(
            '#type' => 'hidden',
            '#value' => $doc_id,
        ),
        'query-parms' => array(
            '#type' => 'hidden',
            '#value' => $query_parms ? $query_parms : '',
        ),
        'title' => array(
            '#type' => 'markup',
            '#markup' => '<h2>' . $title . '</h2>',
        ),
    );

    // If this is a newly-posted document, let the user find the file.
    if (!$doc_id) {
        $form['filepath'] = array(
            '#id' => 'filepath',
            '#type' => 'file',
            '#title' => 'CHOOSE FILE',
            '#prefix' => '<div id="choose-file">',
            '#suffix' => '</div>',
            '#title_display' => 'after',
            '#element_validate' => array('pdq_ebms_doc_file_required'),
        );
        $form['filename'] = array(
            '#type' => 'markup',
            '#markup' => '<div id="filename"></div>',
        );
    }

    // Fold in the Description field, used by new and existing documents.
    $form['desc'] = array(
        '#type' => 'textfield',
        '#title' => 'Description',
        '#default_value' => $desc_default,
    );

    // Add the Boards field, with Ajax hooks to control the rest of the form.
    $form['boards'] = array(
        '#type' => 'checkboxes',
        '#title' => 'boards',
        '#options' => $boards,
        '#default_value' => $boards_default,
        '#description' => 'You may select more than one board.',
        '#ajax' => array(
            'callback' => 'pdq_ebms_doc_board_ajax',
            'wrapper' => 'board-specific-fields',
        ),
    );

    // We have nested Ajax, because both the Tags and the Boards fields
    // control whether the Topics field is displayed.  This didn't work
    // when we tried it in the Drupal 6 prototype, but it's working in
    // Drupal 7, so we don't have to code the form (or Javascript for
    // the Ajax callbacks) by hand.
    $bsf = 'board-specific-fields';
    $tsf = 'tag-specific-fields';
    $form[$bsf] = array(
        '#prefix' => '<div id="' . $bsf . '">',
        '#suffix' => '</div>',
        'tags' => array(
            '#type' => 'checkboxes',
            '#title' => 'Tags',
            '#options' => $tags,
            '#default_value' => $tags_default,
            '#description' => 'You may select more than one tag.',
            '#ajax' => array(
                'callback' => 'pdq_ebms_doc_tag_ajax',
                'wrapper' => $tsf,
            ),
        ),
        $tsf => array(
            '#prefix' => '<div id="' . $tsf . '">',
            '#suffix' => '</div>',
        ),
    );

    // Add the submit button, with the appropriate label.
    $form['submit'] = array(
        '#type' => 'submit',
        '#value' => $doc_id ? 'Save Changes' : 'Upload File',
        '#submit' => array('pdq_ebms_post_doc_form_submit'),
    );

    // Topics are only selectable if at least one board has been
    // associated with the document and the 'summary' tag has been
    // checked.
    if ($summary_tag_checked && count($checked_boards) > 0) {
        $results = db_select('ebms_topic', 't')
            ->fields('t', array('topic_id', 'topic_name'))
            ->condition('t.board_id', $checked_boards, 'IN')
            ->condition('t.active_status', 'A')
            ->orderBy('t.topic_name')
            ->execute();
        $topics = array();
        foreach ($results as $result)
            $topics[$result->topic_id] = $result->topic_name;
        $form[$bsf][$tsf]['topics'] = array(
            '#type' => 'checkboxes',
            '#title' => 'Topics',
            '#description' => 'You may select more than one topic.',
            '#options' => $topics,
            '#default_value' => $topics_default,
        );
        //pdq_ebms_debug('AJAX TOPICS', $topics);
    }
    else
        unset($form[$bsf][$tsf]['topics']);
    return $form;
}

/**
 * Callback to get a fresh copy of the portion of the document posting
 * form which can change if the Boards field changes.
 */
function pdq_ebms_doc_board_ajax($form, $form_state) {
    //pdq_ebms_debug('DOC BOARD AJAX', array($form, $form_state));
    return $form['board-specific-fields'];
}

/**
 * Callback to get a fresh copy of the portion of the document posting
 * form which can change if the Tags field changes.
 */
function pdq_ebms_doc_tag_ajax($form, $form_state) {
    //pdq_ebms_debug('DOC TAG AJAX', array($form, $form_state));
    $subform = $form['board-specific-fields']['tag-specific-fields'];
    return $subform;
}

/**
 * Saves the information about the posted document in the EBMS
 * database tables.
 */
function pdq_ebms_post_doc_form_submit($form, &$form_state) {
    #pdq_ebms_debug('POST DOC FORM SUBMIT', $form_state);
    #drupal_set_message('ok');
    #return;
    // Collect state information.
    $values = $form_state['values'];
    $tags = $values['tags'];
    $boards = $values['boards'];
    $desc = $values['desc'];
    $topics = isset($values['topics']) ? $values['topics'] : array();
    $doc_id = $values['doc'];

    // If this is a new doc, save the file (allow any extensions for now).
    if (!$doc_id) {
        $validators = array('file_validate_extensions' => array());
        $file = file_save_upload('filepath', $validators);
        if ($file) {
            $now = strftime("%Y-%m-%d %H:%M:%S");
            $file = file_move($file, 'public://');
            $file->status |= FILE_STATUS_PERMANENT;
            $file = file_save($file);
            $doc_id = db_insert('ebms_doc')
                ->fields(
                    array(
                        'file_id' => $file->fid,
                        'when_posted' => $now,
                        'description' => $desc,
                    )
                )
                ->execute();
            drupal_set_message('Posted document ' . $file->filename . '.');
        }
        else {
            drupal_set_message('File not found', 'error');
            $doc_id = 0;
        }
    }

    else {

        // For existing documents, only the description column of the base
        // table can be changed.
        db_update('ebms_doc')
            ->fields(array('description' => $desc))
            ->condition('doc_id', $doc_id)
            ->execute();

        // We'll rebuild the lists of boards, tags, and topics.
        db_delete('ebms_doc_board')
            ->condition('doc_id', $doc_id)
            ->execute();
        db_delete('ebms_doc_tag')
            ->condition('doc_id', $doc_id)
            ->execute();
        db_delete('ebms_doc_topic')
            ->condition('doc_id', $doc_id)
            ->execute();
        drupal_set_message('Document successfully updated.');
    }

    // Populate the related tables.
    if ($doc_id) {
        foreach ($tags as $id => $value)
            if ($value)
                db_insert('ebms_doc_tag')
                    ->fields(
                        array(
                            'doc_id' => $doc_id,
                            'tag_id' => $value,
                        )
                    )
                    ->execute();
        foreach ($boards as $id => $value)
            if ($value)
                db_insert('ebms_doc_board')
                    ->fields(
                        array(
                            'doc_id' => $doc_id,
                            'board_id' => $value,
                        )
                    )
                    ->execute();
        foreach ($topics as $id => $value)
            if ($value)
                db_insert('ebms_doc_topic')
                    ->fields(
                        array(
                            'doc_id' => $doc_id,
                            'topic_id' => $value,
                        )
                    )
                    ->execute();
    }

    // Re-route processing to the page showing the posted documents.
    $parms = drupal_json_decode($values['query-parms']);
    drupal_goto(EbmsDocs::$url, array('query' => $parms));
}
