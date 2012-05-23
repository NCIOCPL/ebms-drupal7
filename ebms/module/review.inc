<?php

// $Id$

/**
 * @file
 *
 * Implementation of EBMS Literature Review page.
 */

/**
 * Callback registered for /review pages.
 */
function pdq_ebms_review() {
    $review = new EbmsReview();
    return $review->run();
}

/**
 * Wrapper for logic to handle literature review pages.  Keeps our
 * functions out of the global namespace with less ugly syntax than
 * the PHP namespace mechanism provides, and lets us pass around
 * the current state information more easily.
 */
class EbmsReview {

    public
        $user,
        $packet_id,
        $packet_title,
        $article_id,
        $expanded,
        $sort_col,
        $sort_dir,
        $url;

    /**
     * Passing settings in the URL instead of in $_SESSION in order
     * to avoid dependency on Javascript being enabled on the user's
     * browser.
     */
    public function __construct() {
        global $user;
        $this->user         = $user;
        $this->packet_id    = Ebms\Util::get_request_value('packet');
        $this->article_id   = Ebms\Util::get_request_value('article');
        $this->expanded     = Ebms\Util::get_request_value('expanded');
        $this->sort_col     = Ebms\Util::get_request_value('sort-col', 'f');
        $this->sort_dir     = Ebms\Util::get_request_value('sort-dir', 'a');
        $this->action       = Ebms\Util::get_request_value('action');
        $this->packet_title = $this->fetch_packet_title($this->packet_id);
        $this->url          = 'review';
    }

    /**
     * Select the version of the page appropriate the user's role.
     */
    public function run() {
        if (in_array('board member', $this->user->roles)) {
            if ($this->action == 'reviewer-upload') {
                return $this->reviewer_upload_page();
            }
            return $this->choose_member_review_page();
        }
        return '<i>Stub for manager literature review page</i>';
    }

    /**
     * Pick the page for the board member/reviewer.
     */
    private function choose_member_review_page() {

        $this->add_js();
        if ($this->article_id) {
            if (!$this->already_reviewed())
                return $this->member_review();
            $this->article_id = 0;
        }
        if ($this->packet_id) {
            $articles = $this->reviewer_articles();
            if (count($articles) > 0)
                return $this->member_packet($articles);
            $this->packet_id = 0;
        }
        return $this->member_packets();
    }

    /**
     * Page used by a board member to review an article in his/her queue.
     */
    private function member_review() {

        // Set the bread crumbs.
        $options = array('query' => $this->query_parms());
        unset($options['query']['article']);
        drupal_set_breadcrumb(
            array(
                l('Literature', 'review'),
                l($this->packet_title, 'review', $options),
                "REVIEW",
            )
        );

        return drupal_get_form('pdq_ebms_board_member_review_form', $this);
    }

    /**
     * Member landing page for literature; display's all the packets
     * in the member's queue for review.
     */
    private function member_packets() {

        drupal_set_breadcrumb(array('Literature'));
        $packets = $this->reviewer_packets();
        $page = array(
            '#prefix' => '<div id="member-packets">',
            '#suffix' => '</div>',
        );
        $options = array('html' => true);
        $url = 'review';
        foreach ($packets as $p) {
            $options['query'] = array('packet' => $p->packet_id);
            $packet_link = l($p->packet_title, $url, $options);
            $options['query']['expanded'] = 's';
            $summaries_link = l('Summary Documents &nbsp; +', $url, $options);
            $options['query']['expanded'] = 'a';
            $articles_link = l('Articles &nbsp; +', $url, $options);
            $options['query']['expanded'] = 'u';
            $uploads_link = l('Reviewer Uploads &nbsp; +', $url, $options);
            $page["packet-{$p->packet_id}"] = array(
                'packet-title' => array(
                    '#type' => 'markup',
                    '#markup' => "<h2>$packet_link</h2>",
                ),
                'summary-docs' => array(
                    '#type' => 'markup',
                    '#markup' => "<h3 class='h3-first'>$summaries_link</h3>",
                ),
                'articles' => array(
                    '#type' => 'markup',
                    '#markup' => "<h3>$articles_link</h3>",
                ),
                'reviewer-uploads' => array(
                    '#type' => 'markup',
                    '#markup' => "<h3 class='h3-last'>$uploads_link</h3>",
                ),
            );
        }
        return $page;
    }

    /*
     * Display page for a single literature review packet.
     */
    private function member_packet($articles) {

        // Override Drupal's breadcrumbs.
        $title = htmlspecialchars($this->packet_title);
        drupal_set_breadcrumb(array(l('Literature', 'review'), $title));

        // Build the page content.
        $page = array(
            '#prefix' => '<div id="member-packet">',
            '#suffix' => '</div>',
            'packet-title' => array(
                '#type' => 'markup',
                '#markup' => "<h2>{$this->packet_title}</h2>"
            ),
        );

        // Fold in the three sections and we're done.
        $this->show_packet_summaries($page);
        $this->show_packet_articles($page, $articles);
        $this->show_reviewer_uploads($page);
        return $page;
    }

    /**
     * Show links to the summary documents attached to the current packet.
     * (if this section of the packet page is expanded).
     */
    private function show_packet_summaries(&$page) {

        // Create the options for expanding or collapsing the section.
        $options = array('html' => true, 'query' => $this->query_parms());

        // If the section is collapsed, just show the link to expand it.
        if (strpos($this->expanded, 's') === FALSE) {
            $options['query']['expanded'] = $this->expanded . 's';
            $link = l('Summary Documents &nbsp; +', 'review', $options);
            $page['summaries'] = array(
                '#type' => 'markup',
                '#markup' => "<h3>$link</h3><br />",
            );
        }

        // The section is expanded; fetch and list the summaries docs.
        else {
            $expanded = str_replace('s', '', $this->expanded);
            $options['query']['expanded'] = $expanded;
            $label = 'Summary Documents &nbsp <span>&ndash;</span>';
            $link = l($label, 'review', $options);
            $page['summaries']['heading'] = array(
                '#type' => 'markup',
                '#markup' => "<h3 class='expanded'>$link</h3>",
            );
            $summaries = $this->fetch_packet_summaries();
            $links = array();
            foreach ($summaries as $uri => $description)
                $links[] = l($description, file_create_url($uri));
            $page['summaries']['links'] = array(
                '#prefix' => '<div id="summary-links">',
                '#suffix' => '</div>',
                '#items' => $links,
                '#theme' => 'item_list',
            );
        }
    }

    /**
     * List the articles in the current packet which the board member still
     * needs to review (if this section of the packet page is expanded).
     * We've already fetched the articles because we had to make sure
     * there was actually a reason to display this page (which we don't
     * do if there are no more articles in the packet left for this
     * reviewer to look at).
     */
    private function show_packet_articles(&$page, $articles) {

        // Create the options for expanding or collapsing the section.
        $options = array('html' => true, 'query' => $this->query_parms());

        // If the section is collapsed, just show the link to expand it.
        if (strpos($this->expanded, 'a') === FALSE) {
            $options['query']['expanded'] = $this->expanded . 'a';
            $link = l('Articles &nbsp; +', 'review', $options);
            $page['articles'] = array(
                '#type' => 'markup',
                '#markup' => "<h3>$link</h3><br />",
            );
        }

        // The section is expanded; show the articles still in the queue.
        else {

            // XXX Link to real full text PDFs when they're ready.
            $ft_uri = 'public://full-text-article-stub.pdf';
            $target = array('attributes' => array('target' => '_blank'));
            $pubmed = 'http://www.ncbi.nlm.nih.gov/pubmed/';

            // Squeeze out article expansion for the "collapse" URL.
            $expanded = str_replace('a', '', $this->expanded);
            $options['query']['expanded'] = $expanded;
            $label = 'Articles &nbsp <span>&ndash;</span>';
            $collapse_link = l($label, 'review', $options);

            // Restore article expansion for the review links.
            $options['query']['expanded'] = $expanded . 'a';

            // Build the array of rows for the article table.
            $rows = array();
            foreach ($articles as $article) {
                $pm_id = $article->source_id;
                $pm_link = l('view abstract', $pubmed . $pm_id, $target);
                $ft_url = file_create_url($ft_uri);
                $ft_link = l('download full text', $ft_url, $target);
                $links = "$pm_link &nbsp; | &nbsp; $ft_link";
                $citation = Ebms\Util::get_full_citation($article->article_id);
                $citation = htmlspecialchars($citation);
                $options['query']['article'] = $article->article_id;
                $rows[] = array(
                    'data' => array(
                        array(
                            'data' => "$citation<br />$links",
                            'class' => array('c1'),
                        ),
                        array(
                            'data' => l('review', 'review', $options),
                            'class' => array('c2'),
                            'valign' => 'bottom',
                        ),
                    ),
                    'no_striping' => true,
                );
            }

            // Plug the "Articles" heading and table into the page.
            $page['articles'] = array(
                'heading' => array(
                    '#type' => 'markup',
                    '#markup' => "<h3 class='expanded'>$collapse_link</h3>",
                ),
                'table' => array(
                    '#prefix' => '<div id="articles">',
                    '#theme' => 'table',
                    '#rows' => $rows,
                    '#suffix' => '</div><br />',
                ),
            );
        }
    }

    /**
     * Display a table listing the files the user has uploaded for this
     * packet.
     */
    private function show_reviewer_uploads(&$page) {

        // Create the options for expanding or collapsing the section.
        $options = array('html' => true, 'query' => $this->query_parms());

        // If the section is collapsed, just show the link to expand it.
        if (strpos($this->expanded, 'u') === FALSE) {
            $options['query']['expanded'] = $this->expanded . 'u';
            $link = l('Reviewer Uploads &nbsp; +', 'review', $options);
            $page['uploads'] = array(
                '#type' => 'markup',
                '#markup' => "<h3 id='reviewer-uploads-h3'>$link</h3><br />",
            );
        }

        // The section is expanded; display it.
        else {

            // Open uploaded documents in a separate window or tab.
            $target = array('attributes' => array('target' => '_blank'));

            // Squeeze out section expansion for the "collapse" URL.
            $expanded = str_replace('u', '', $this->expanded);
            $options['query']['expanded'] = $expanded;
            $label = 'Reviewer Uploads &nbsp; <span>&ndash;</span>';
            $collapse_link = l($label, 'review', $options);

            // Restore the expanded option.
            $options['query']['expanded'] .= 'u';

            // Create new options array for document links.
            $doc_opts = array(
                'attributes' => array(
                    'target' => '_blank',
                    'title' => 'Click to download/open document',
                ),
            );

            // Start off the rows with the header row.
            $rows = array($this->reviewer_docs_column_headers());

            // Append the rows for the uploaded documents.
            $uploaded_docs = $this->fetch_reviewer_uploads();
            foreach ($uploaded_docs as $doc) {
                $url = file_create_url($doc->uri);
                $posted = date("y-m-d", strtotime($doc->when_posted));
                $rows[] = array(
                    'data' => array(
                        array(
                            'data' => l($doc->filename, $url, $doc_opts),
                            'class' => array('c1'),
                        ),
                        array(
                            'data' => htmlspecialchars($doc->name),
                            'class' => array('c2'),
                        ),
                        array(
                            'data' => $posted,
                            'class' => array('c3'),
                        ),
                    ),
                    'no_striping' => true,
                );
            }

            // Plug the "Reviewer Uploads" heading and table into the page.
            $options['query']['action'] = 'reviewer-upload';
            $page['uploads'] = array(
                '#prefix' => '<div id="reviewer-uploads">',
                '#suffix' => '</div>',
                'heading' => array(
                    '#type' => 'markup',
                    '#markup' => "<h3 class='expanded'>$collapse_link</h3>",
                ),
                'post link' => array(
                    '#type' => 'markup',
                    '#markup' => l('post document', 'review', $options),
                    '#prefix' => '<p id="reviewer-post-button">',
                    '#suffix' => '</p>',
                ),
                'table' => array(
                    '#theme' => 'table',
                    '#rows' => $rows,
                ),
            );

            // Add the file upload form we'll use if Javascript is enabled.
            $form = drupal_get_form('pdq_ebms_reviewer_upload_form', $this);
            $form['#prefix'] = '<div id="file-upload-form-js">';
            $form['#suffix'] = '</div>';
            $form['filepath']['#title_display'] = 'after';
            $page['js-file-upload-form'] = $form;
        }
    }

    /**
     * Create the page used by reviewers without Javascript for uploading
     * a document for the packets they review.
     */
    private function reviewer_upload_page() {
        $options = array('query' => $this->query_parms());
        drupal_set_breadcrumb(
            array(
                l('Literature', 'review'),
                l($this->packet_title, 'review', $options),
                "UPLOAD",
            )
        );
        return drupal_get_form('pdq_ebms_reviewer_upload_form', $this);
    }

    /**
     * Get the summary documents which have been attached to the current
     * packet.
     */
    private function fetch_packet_summaries() {
        $query = db_select('file_managed', 'f');
        $query->join('ebms_doc', 'd', 'd.file_id = f.fid');
        $query->join('ebms_packet_summary', 's', 's.doc_id = d.doc_id');
        $query->fields('f', array('uri'));
        $query->fields('d', array('description'));
        $query->condition('s.packet_id', $this->packet_id);
        $query->orderBy('d.description');
        $result = $query->execute();
        $summaries = array();
        foreach ($result as $record)
            $summaries[$record->uri] = $record->description;
        return $summaries;
    }

    /**
     * Get the documents the reviewer has posted back to this packet.
     */
    private function fetch_reviewer_uploads() {
        $sort_dir = $this->sort_dir == 'd' ? 'DESC' : 'ASC';
        $sort_col = 'f.filename';
        if ($this->sort_col == 'u')
            $sort_col = 'u.name';
        else if ($this->sort_col == 'p')
            $sort_col = 'd.when_posted';
        $query = db_select('file_managed', 'f');
        $query->join('ebms_reviewer_doc', 'd', 'd.file_id = f.fid');
        $query->join('users', 'u', 'u.uid = d.reviewer_id');
        $query->fields('f', array('uri', 'filename'));
        $query->fields('u', array('name'));
        $query->fields('d', array('when_posted'));
        $query->condition('d.packet_id', $this->packet_id);
        $query->condition('d.drop_flag', 0);
        $query->orderBy($sort_col, $sort_dir);
        $result = $query->execute();
        $rows = array();
        foreach ($result as $record)
            $rows[] = $record;
        return $rows;
    }

    /**
     * Create a URL for viewing the PubMed abstract for an article.
     */
    public static function pubmed_url($article_id) {
        $source_id = db_select('ebms_article', 'a')
            ->fields('a', array('source_id'))
            ->condition('a.article_id', $article_id)
            ->execute()
            ->fetchField();
        return 'http://www.ncbi.nlm.nih.gov/pubmed/' . $source_id;
    }

    /**
     * Create the top row for the table displaying documents the reviewer
     * has uploaded.  We're not using the Drupal "#header" technique
     * because we are not able to  take advantage of their styling, and
     * their mechanism for sorting columns makes the URLs too ugly.
     */
    private function reviewer_docs_column_headers() {
        $sort_cols = array(
            'f' => 'FILE NAME',
            'u' => 'UPLOADED BY',
            'p' => 'WHEN POSTED'
        );
        $options = array('query' => $this->query_parms());
        pdq_ebms_debug('col headers 1', $options);
        $i = 1;
        $headers = array();
        foreach ($sort_cols as $key => $label) {
            if ($this->sort_col == $key && $this->sort_dir == 'a')
                $options['query']['sort-dir'] = 'd';
            else
                $options['query']['sort-dir'] = 'a';
            $options['query']['sort-col'] = $key;
        pdq_ebms_debug('col headers 2', $options);
            $headers[] = array(
                'data' => l($label, 'review', $options),
                'class' => array('c' . $i++),
            );
        }
        return array(
            'data' => $headers,
            'no_striping' => true,
        );
    }

    /**
     * Don't want a board member bookmarking a review page and coming back
     * to an article she's already reviewed.
     */
    private function already_reviewed() {
        $count = db_select('ebms_article_review', 'r')
            ->condition('r.reviewer_id', $this->user->uid)
            ->condition('r.packet_id', $this->packet_id)
            ->condition('r.article_id', $this->article_id)
            ->countQuery()
            ->execute()
            ->fetchField();
        return $count > 0;
    }

    /**
     * Get the list of articles in this packet which still need to be
     * reviewed by the logged-in board member.
     */
    private function reviewer_articles() {
        $query = db_select('ebms_article', 'a');
        $query->fields('a',
            array('article_id', 'brf_citation', 'full_text_id',
                'source_id'));
        $query->join('ebms_packet_article', 'p',
            'p.article_id = a.article_id');
        $query->leftJoin('ebms_article_review', 'r',
            'r.article_id = a.article_id AND ' .
            'r.packet_id = p.packet_id AND ' .
            'r.reviewer_id = :uid', array(':uid' => $this->user->uid));
        $query->condition('p.packet_id', $this->packet_id);
        $query->isNull('r.article_id');
        $query->orderBy('a.import_date', 'desc');
        $result = $query->execute();
        $articles = array();
        foreach ($result as $article)
            $articles[] = $article;
        return $articles;
    }

    /**
     * Fetch all of the packets assigned to the board member, in which
     * at least one article is still waiting to be reviewed.
     */
    private function reviewer_packets() {
        $query = db_select('ebms_packet', 'p');
        $query->fields('p', array('packet_id', 'packet_title'));
        $query->join('ebms_packet_reviewer', 'm', 'p.packet_id = m.packet_id');
        $query->join('ebms_packet_article', 'a', 'a.packet_id = p.packet_id');
        $query->join('ebms_topic', 't', 't.topic_id = p.topic_id');
        $query->leftJoin('ebms_article_review', 'r',
            implode(' AND ',
                array(
                    'r.article_id = a.article_id',
                    'r.reviewer_id = m.reviewer_id',
                    'r.packet_id = p.packet_id'
                )
            )
        );
        $query->condition('m.reviewer_id', $this->user->uid);
        $query->isNull('r.article_id');
        $query->distinct();
        $query->orderBy('t.topic_name');
        $query->orderBy('p.created_at', 'desc');
        return $query->execute();
    }

    /*
     * Load the disposition options.
     */
    public static function disposition_options() {

        $query = db_select('ebms_review_disposition_value', 'v');
        $query->fields('v', array('value_id', 'value_name', 'instructions'));
        $query->orderBy('v.value_pos');
        $result = $query->execute();
        $options = array();
        foreach ($result as $record) {
            $display = htmlspecialchars($record->value_name);
            if ($record->instructions) {
                $instructions = htmlspecialchars($record->instructions);
                $display .= " <i>($instructions)</i>";
            }
            $options[$record->value_id] = $display;
        }
        return $options;
    }

    /*
     * Fetch the values for the reason(s) for exclusion.
     */
    public static function reason_options() {
        $query = db_select('ebms_review_rejection_value', 'v');
        $query->fields('v', array('value_id', 'value_name', 'extra_info'));
        $query->orderBy('v.value_pos');
        $result = $query->execute();
        $options = array();
        foreach ($result as $record) {
            $display = htmlspecialchars($record->value_name);
            if ($record->extra_info) {
                $instructions = htmlspecialchars($record->extra_info);
                $display .= " <i>($instructions)</i>";
            }
            $options[$record->value_id] = $display;
        }
        return $options;
    }

    /**
     * Retrieve the title for the current packet from the database.
     */
    private static function fetch_packet_title($packet_id) {
        if (!$packet_id)
            return null;
        return db_select('ebms_packet', 'p')
            ->fields('p', array('packet_title'))
            ->condition('p.packet_id', $packet_id)
            ->execute()
            ->fetchField();
    }

    /**
     * Build the URL for the LOE guidelines document used by the board
     * for which the current packet was created.
     */
    public function loeUrl() {
        $query = db_select('file_managed', 'f');
        $query->fields('f', array('uri'));
        $query->join('ebms_doc', 'd', 'd.file_id = f.fid');
        $query->join('ebms_board', 'b', 'b.loe_guidelines = d.doc_id');
        $query->join('ebms_topic', 't', 't.board_id = b.board_id');
        $query->join('ebms_packet', 'p', 'p.topic_id = t.topic_id');
        $query->condition('p.packet_id', $this->packet_id);
        $uri = $query->execute()->fetchField();
        return file_create_url($uri);
    }

    /*
     * Load the Javascript libraries we need.
     */
    private function add_js() {
        drupal_add_library('system', 'ui.dialog');
        drupal_add_js(Ebms\JS_DIR . '/ebms.js');
    }

    /**
     * Pack up the current settings for use in constructing URLs.
     */
    public function query_parms() {
        $parms = array();
        $names = array(
            'packet' => $this->packet_id,
            'article' => $this->article_id,
            'sort-col' => $this->sort_col,
            'sort-dir' => $this->sort_dir,
            'expanded' => $this->expanded,
        );
        foreach ($names as $name => $value)
            if ($value)
                $parms[$name] = $value;
        return $parms;
    }
}

/**
 * Verify that the minimum requirements for an article review have been
 * met:
 *
 *  - at least one disposition has been selected for the article
 *  - if "Warrants no changes to the summary" is selected, then
 *    no other dispositions are selected (should be impossible
 *    if Javascript is enabled)
 *  - if "Warrants no changes to the summary" is selected, then
 *    at least one reason is given for rejecting the summary
 *
 * Assumes that "Warrants no changes to the summary" is the first
 * option in the set of disposition checkboxes, so we can find it
 * by position rather than rely on the display string never changing.
 */
function pdq_ebms_board_member_review_form_validate($form, &$form_state) {

    // Look up the ID of the disposition for rejecting the article
    // (always in the first position).
    $no_change_option = db_select('ebms_review_disposition_value', 'v')
            ->fields('v', array('value_id'))
            ->condition('v.value_pos', 1)
            ->execute()
            ->fetchField();

    $disposition_count = $reason_count = 0;
    $rejected = false;
    $reasons = $form_state['values']['reasons'];
    $dispositions = $form_state['values']['dispositions'];
    foreach ($dispositions as $id => $val) {
        if ($val) {
            $disposition_count++;
            if ($val == $no_change_option)
                $rejected = true;
        }
    }
    if (!$disposition_count)
        form_set_error('dispositions',
            'At least one disposition must be selected.');

    if ($rejected) {
        if ($disposition_count > 1) {
            form_set_error('dispositions',
                'Contradictory dispositions selected.');
        }
        foreach ($reasons as $id => $val) {
            if ($val)
                $reason_count++;
        }
        if (!$reason_count)
            form_set_error('reasons',
                'At least one exclusion reason must be selected.');
    }
}

/**
 * Store the board member's feedback on this article.
 */
function pdq_ebms_store_review($form, &$form_state) {
    global $user;
    $packet_id = $form_state['values']['packet'];
    $article_id = $form_state['values']['article'];
    $expanded = $form_state['values']['expanded'];
    $sort_col = $form_state['values']['sort-col'];
    $sort_dir = $form_state['values']['sort-dir'];
    $comments = $form_state['values']['comments'];
    $loe = $form_state['values']['loe'];
    $now = strftime("%Y-%m-%d %H:%M:%S");
    $review_id = db_insert('ebms_article_review')
        ->fields(
            array(
                'packet_id'   => $packet_id,
                'article_id'  => $article_id,
                'reviewer_id' => $user->uid,
                'when_posted' => $now,
                'comments'    => $comments,
                'loe_info'    => $loe,
            )
        )
        ->execute();
    foreach ($form_state['values']['dispositions'] as $id => $val) {
        if ($val) {
            db_insert('ebms_review_disposition')
                ->fields(
                    array(
                        'review_id' => $review_id,
                        'value_id' => $id
                    )
                )
                ->execute();
        }
    }
    foreach ($form_state['values']['reasons'] as $id => $val) {
        if ($val) {
            db_insert('ebms_review_rejection_reason')
                ->fields(
                    array(
                        'review_id' => $review_id,
                        'value_id' => $id
                    )
                )
                ->execute();
        }
    }
    $options = array(
        'query' => array(
            'packet' => $packet_id,
            'expanded' => $expanded,
            'sort-col' => $sort_col,
            'sort-dir' => $sort_dir
        ),
    );
    drupal_goto('review', $options);
}

/**
 * Callback for generating the member article review form.  The name
 * of the callback function is used by Drupal as the form's ID.
 */
function pdq_ebms_board_member_review_form($form, &$form_state, $review) {

    // Make link for viewing LOE guidelines only if the board has them.
    $loeSuffix = '</div>';
    $loeUrl = $review->loeUrl();
    if ($loeUrl) {
        $attrs = array('attributes' => array('class' => 'left-indent'));
        $link = l('DOWNLOAD LOE GUIDELINES', $loeUrl, $attrs);
        $loeSuffix = "$link</div>";
    }

    // XXX Use real full text links when they're ready.
    $title = htmlspecialchars($review->packet_title);
    $attrs = array('attributes' => array('target' => '_blank'));
    $ft_url = file_create_url('public://full-text-article-stub.pdf');
    $pm_url = EbmsReview::pubmed_url($review->article_id);
    $ft_link = l('VIEW ABSTRACT', $ft_url, $attrs);
    $pm_link = l('DOWNLOAD FULL TEXT', $pm_url, $attrs);
    $article_id = $review->article_id;
    $cite = htmlspecialchars(Ebms\Util::get_full_citation($article_id));
    return array(
        '#id' => 'member-review',
        'packet-title' => array(
            '#type' => 'markup',
            '#markup' => "<h2>$title</h2>"
        ),
        'packet-subtitle' => array(
            '#type' => 'markup',
            '#markup' => '<h3 class="red">REVIEW</h3>'
        ),
        'article-citation-block' => array(
            '#type' => 'markup',
            '#prefix' => '<p id="article-citation">',
            '#suffix' => '</p>',
            '#markup' => "$cite <br /> $pm_link &nbsp; | &nbsp; $ft_link",
        ),
        'packet' => array(
            '#type' => 'hidden',
            '#value' => $review->packet_id,
        ),
        'article' => array(
            '#type' => 'hidden',
            '#value' => $review->article_id,
        ),
        'expanded' => array(
            '#type' => 'hidden',
            '#value' => $review->expanded,
        ),
        'sortcol' => array(
            '#type' => 'hidden',
            '#value' => $review->sort_col,
        ),
        'sortdir' => array(
            '#type' => 'hidden',
            '#value' => $review->sort_dir,
        ),
        'dispositions' => array(
            '#type' => 'checkboxes',
            '#title' => 'Disposition',
            '#description' =>
            'Indicate how the article might affect the summary.',
            '#options' => $review->disposition_options(),
        ),
        'reasons' => array(
            '#type' => 'checkboxes',
            '#title' => 'Reason(s) for Exclusion From PDQ Summary',
            '#description' =>
            'Please indicate which of the reasons below led to your ' .
            'decision to exclude the article. You may choose more than ' .
            'one reason.',
            '#options' => $review->reason_options(),
            '#prefix' => '<div id="reasons-wrapper">',
            '#suffix' => '</div>',
        ),
        'comments' => array(
            '#type' => 'textarea',
            '#resizable' => false,
            '#title' => 'Comments',
            '#description' => 'Detailed recommendations',
        ),
        'loe' => array(
            '#type' => 'textarea',
            '#resizable' => false,
            '#title' => 'Levels of Evidence Information',
            '#description' =>
            'Enter the appropriate level of evidence for this article',
            '#prefix' => '<div id="loe-wrapper">',
            '#suffix' => $loeSuffix,
        ),
        'submit' => array(
            '#type' => 'submit',
            '#value' => 'SUBMIT',
            '#submit' => array('pdq_ebms_store_review'),
            '#executes_submit_callback' => true,
        ),
        '#validate' => array('pdq_ebms_board_member_review_form_validate'),
    );
}

/**
 * Create the form which will pop up when the reviewer wants to
 * post a document back to the review packet.
 */
function pdq_ebms_reviewer_upload_form($form, &$form_state, $review) {

    // Build the render array for the form.
    return array(
        '#id' => 'reviewer-upload-form',
        'title' => array(
            '#type' => 'markup',
            '#markup' => '<h2>Reviewer Upload</h2>',
        ),
        'packet' => array(
            '#type' => 'hidden',
            '#value' => $review->packet_id,
        ),
        'expanded' => array(
            '#type' => 'hidden',
            '#value' => $review->expanded,
        ),
        'sort-col' => array(
            '#type' => 'hidden',
            '#value' => $review->sort_col,
        ),
        'sort-dir' => array(
            '#type' => 'hidden',
            '#value' => $review->sort_dir,
        ),
        'filepath' => array(
            '#id' => 'filepath',
            '#type' => 'file',
            '#title' => 'CHOOSE FILE',
            '#prefix' => '<div id="choose-file">',
            '#suffix' => '</div>',
            '#element_validate' => array('pdq_ebms_doc_file_required'),
        ),
        'filename' => array(
            '#type' => 'markup',
            '#markup' => '<div id="filename"></div>',
        ),
        'filenotes' => array(
            '#id' => 'filenotes',
            '#type' => 'textarea',
            '#resizable' => false,
            '#title' => 'ADD NOTES (OPTIONAL)',
            '#prefix' => '<div id="add-notes">',
            '#suffix' => '</div>',
            '#attributes' => array('maxlength' => 315),
        ),
        'charcount' => array(
            '#type' => 'markup',
            '#markup' => '<p id="charcount"></p>',
        ),
        'submit' => array(
            '#id' => 'upload-file',
            '#type' => 'submit',
            '#value' => 'UPLOAD FILE',
            '#submit' => array('pdq_ebms_save_reviewer_file'),
        ),
    );
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

/*
 * Store the file the reviewer posted.
 */
function pdq_ebms_save_reviewer_file($form, &$form_state) {

    // Collect state information.
    global $user;
    $packet_id = $form_state['values']['packet'];
    $expanded = $form_state['values']['expanded'];
    $sort_col = $form_state['values']['sort-col'];
    $sort_dir = $form_state['values']['sort-dir'];
    $filenotes = $form_state['values']['filenotes'];

    // Save the file (allow any extensions for now).
    $validators = array('file_validate_extensions' => array());
    $file = file_save_upload('filepath', $validators);
    if ($file) {
        $now = strftime("%Y-%m-%d %H:%M:%S");
        $file = file_move($file, 'public://');
        $file->status |= FILE_STATUS_PERMANENT;
        $file = file_save($file);
        if (!$filenotes)
            $filenotes = null;
        $docId = db_insert('ebms_reviewer_doc')
            ->fields(
                array(
                    'file_id' => $file->fid,
                    'reviewer_id' => $user->uid,
                    'packet_id' => $packet_id,
                    'when_posted' => $now,
                    'doc_title' => $file->filename,
                    'description' => $filenotes)
            )
            ->execute();
        drupal_set_message('Posted document ' . $file->filename . '.');
    }
    else
        drupal_set_message('File not found', 'error');
    $options = array(
        'query' => array(
            'packet' => $packet_id,
            'expanded' => $expanded,
            'sort-col' => $sort_col,
            'sort-dir' => $sort_dir
        ),
    );
    drupal_goto('review', $options);
}
