<?php

// $Id$

/**
 * @file
 *
 * Implementation of EBMS Summaries page.
 */

/**
 * Entry point used by the Drupal menu router for all of the EBMS summaries
 * pages.
 */
function pdq_ebms_summaries($request=null, $board_id=0, $subpage_id=0) {
    $summaries = new EbmsSummaries($request, $board_id, $subpage_id);
    return $summaries->run();
}

/**
 * Wrapper for logic to handle profile view/edit pages.  Keeps our
 * functions out of the global namespace with less ugly syntax than
 * the PHP namespace mechanism provides, and lets us pass around
 * the current state information more easily.
 */
class EbmsSummaries {

    public
        $request,
        $user,
        $role,
        $parms,
        $board_id,
        $subpage_id;
    public static $url = 'summaries';

    /**
     * Collect information we'll need for all the summaries pages.
     */
    public function __construct($request, $board_id, $subpage_id) {
        global $user;
        $this->user       = user_load($user->uid);
        $this->request    = $request;
        $this->board_id   = $board_id;
        $this->subpage_id = $subpage_id;
        $this->role       = user_access('administer') ? 'admin' : 'member';
        $this->parms      = drupal_get_query_parameters();
    }

    /**
     * Assemble the render array for the requested summaries page.
     * The landing page for summaries shows all of the user's boards
     * (or all boards, for roles not tied to a specific board).  From
     * the landing page the user can navigate to the page showing a
     * specific board, and all of the summary subpages created for
     * that board (as well as any summary supporting documents for
     * the board).  From the board page, the user can drill down
     * into a specific subpage where links to individidual summaries
     * on cancer.gov are shown, as well as summary documents posted
     * by board managers ("documents posted by NCI") and by board
     * members ("documents posted by board members").
     */
    public function run() {
        Ebms\Util::add_js();
        Ebms\Menu::$active = 'Summaries';
        switch ($this->request) {
            case 'board':
                return $this->show_board();
            case 'page':
                return $this->show_page();
            case 'member-post':
                return drupal_get_form('pdq_ebms_member_doc_form', $this);
            default:
                return $this->show_boards();
        }
    }

    /**
     * Displays one of the summary sub-pages created by a board
     * manager.  The page may have more than one summary.  An
     * example of such a page would be the Colorectal Summaries
     * page, with links to the Colon Cancer summary as well
     * as the Rectal Cancer summary.
     */
    private function show_page() {

        // Override the default Drupal breadcrumb system.
        $board_name = Ebms\Util::get_board_name($this->board_id);
        $page_name = db_select('ebms_summary_page', 'p')
            ->fields('p', array('page_name'))
            ->condition('p.page_id', $this->subpage_id)
            ->execute()
            ->fetchField();
        $page_name = htmlspecialchars($page_name);
        $url = EbmsSummaries::$url;
        $options = array('query' => $this->parms);
        drupal_set_breadcrumb(
            array(
                l('Summaries', $url),
                l($board_name, "$url/board/$this->board_id", $options),
                $page_name,
            )
        );

        // If the user checked the box to archive one of the documents,
        // update the row in the appropriate table to reflect the new
        // status.  This action (available only to managers) suppresses
        // the document from displays except to managers when the option
        // to include archived documents is turned on.  The 'archive'
        // parameter's value contains the code for the document's type
        // ('r' for 'returned' documents, uploaded by board members, or
        // 'p' for documents 'posted' by managers) and the document's
        // id, separated by a hyphen; e.g., archive=p-234.  Note how
        // the table we update depends on the document type.
        if (isset($this->parms['archive'])) {
            $now = strftime("%Y-%m-%d %H:%M:%S");
            list($doc_type, $doc_id) = explode('-', $this->parms['archive']);
            $doc_types = array('r' => 'returned', 'p' => 'posted');
            if (array_key_exists($doc_type, $doc_types)) {
                $table = 'ebms_summary_' . $doc_types[$doc_type] . '_doc';
                db_update($table)
                    ->fields(array('archived' => $now))
                    ->condition('doc_id', $doc_id)
                    ->condition('page_id', $this->subpage_id)
                    ->execute();
            }

            // Not enough to pull it out of our own set; table sorting
            // URLs will go back to the original query for parameters.
            unset($_GET['archive']);
            unset($this->parms['archive']);
        }

        // The manager can uncheck the 'archive' box to undo the
        // action described above for the 'archive' parameter.  This
        // will set the 'revive' parameter, which takes the same form
        // as the 'archive' parameter above.  We clear the 'archived'
        // column for the document.
        if (isset($this->parms['revive'])) {
            list($doc_type, $doc_id) = explode('-', $this->parms['revive']);
            $doc_types = array('r' => 'returned', 'p' => 'posted');
            if (array_key_exists($doc_type, $doc_types)) {
                $table = 'ebms_summary_' . $doc_types[$doc_type] . '_doc';
                db_update($table)
                    ->fields(array('archived' => null))
                    ->condition('doc_id', $doc_id)
                    ->condition('page_id', $this->subpage_id)
                    ->execute();
            }

            // See note above about clearing the 'archive' parameter.
            unset($_GET['revive']);
            unset($this->parms['revive']);
        }

        // Assemble and return the render array for the page.
        // The query parameter "se" controls which sections of the
        // subpage (s) are expanded (e): each section is represented
        // in the value of the parameter by a single character:
        //   c - Cancer.gov summaries section
        //   n - Section for documents uploaded by NCI
        //   m - Section for documents uploaded by board Members
        $expanded = isset($this->parms['se']) ? $this->parms['se'] : '';
        $url = "$url/page/$this->board_id/$this->subpage_id";
        return array(
            'header' => array(
                '#type' => 'markup',
                '#markup' => "<h2 class='low-key'>$page_name</h2>",
            ),
            'summaries' => $this->cg_summaries($url, $expanded),
            'nci-docs' => $this->nci_docs($url, $expanded),
            'member-docs' => $this->member_docs($url, $expanded),
        );
    }

    /**
     * Builds the portion of the page listing links to cancer.gov
     * summaries.
     *
     *   Passed:
     *
     *     $url       how we get back to this sub-page
     *
     *     $expanded  string containing one letter for each section
     *                of the sub-page which has been expanded:
     *                'c' for cancer.gov summaries; 'n' for
     *                summary documents posted by NCI; or 'm'
     *                for documents returned by board members.
     */
    private function cg_summaries($url, $expanded) {

        // Local convenience variables.
        $admin = $this->role == 'admin';
        $parms = $this->parms;

        // If the section is collapsed, just show its title, with a link
        // to expand it.
        if (strpos($expanded, 'c') === false) {
            $parms['se'] = $expanded . 'c';
            $options = array('query' => $parms, 'html' => true);
            $label = $this->collapsed('Cancer.gov Summaries');
            $link = l($label, $url, $options);
            return array(
                '#type' => 'container',
                '#attributes' => array(
                    'id' => 'cg-summaries-block',
                    'class' => array('collapsed', 'clearfix'),
                ),
                'section' => array(
                    '#prefix' => '<div class="float-left">',
                    '#suffix' => '</div>',
                    '#type' => 'markup',
                    '#markup' => "<h3 class='collapsed'>$link</h3>",
                ),
            );
        }

        // If we get here, the section is expanded.  Create the beginning
        // of the render array for the block.
        $parms['se'] = str_replace('c', '', $expanded);
        $options = array('query' => $parms, 'html' => true);
        $label = $this->expanded('Cancer.gov Summaries');
        $link = l($label, $url, $options);
        $block = array(
            'wrapper' => array(
                '#type' => 'container',
                '#attributes' => array(
                    'id' => 'cg-summaries-block',
                    'class' => array('expanded clearfix'),
                ),
                'left side' => array(
                    '#type' => 'container',
                    '#attributes' => array(
                        'class' => array('float-left'),
                    ),
                    'heading' => array(
                        '#type' => 'markup',
                        '#markup' => "<h3 class='expanded'>$link</h3>",
                    ),
                ),
            ),
        );

        // Fold in the links to the summaries on cancer.gov.
        $items = array();
        $results = db_select('ebms_summary_link', 'l')
            ->fields('l', array('link_url', 'link_label'))
            ->condition('l.page_id', $this->subpage_id)
            ->orderBy('l.link_label')
            ->execute();
        $options = array('attributes' => array('target' => '_blank'));
        foreach ($results as $link)
            $items[] = l($link->link_label, $link->link_url, $options);
        $block['wrapper']['left side']['links'] = array(
            '#theme' => 'item_list',
            '#items' => $items,
            '#attributes' => array('class' => array('no-bullets')),
        );

        // If the user is a manager, add a button to create a new link,
        // as well as the hidden form which will pop up if the user
        // clicks that link.
        if ($admin) {
            $href = 'javascript:ebmsscript.show_new_cg_summary_form();';
            $right_side = array(
                'wrapper' => array(
                    '#type' => 'container',
                    '#attributes' => array(
                        'class' => array('float-right', 'pad-top-10'),
                    ),
                    'button' => array(
                        '#type' => 'markup',
                        '#markup' =>
                        "<a href='$href' class='button'>Add New Link</a>",
                    ),
                ),
            );
            $block['wrapper']['right side'] = $right_side;
            $form = drupal_get_form('pdq_ebms_new_cg_summary_form', $this);
            $form['#prefix'] = '<div id="new-cg-summary-form" class="popup">';
            $form['#suffix'] = '</div>';
            $block['new link form'] = $form;
        }

        // The render array is complete; return it.
        return $block;
    }

    /**
     * Builds the portion of a Summaries sub-page showing documents
     * posted by an NCI manager.
     *
     *   Passed:
     *
     *     $url       how we get back to this sub-page
     *
     *     $expanded  string containing one letter for each section
     *                of the sub-page which has been expanded:
     *                'c' for cancer.gov summaries; 'n' for
     *                summary documents posted by NCI; or 'm'
     *                for documents returned by board members.
     */
    private function nci_docs($url, $expanded) {

        // Local convenience variables.
        $admin = $this->role == 'admin';
        $parms = $this->parms;

        // If the section is collapsed, just show its title, with a link
        // to expand it.
        if (strpos($expanded, 'n') === false) {
            $parms['se'] = $expanded . 'n';
            $options = array('query' => $parms, 'html' => true);
            $label = $this->collapsed('Documents Posted By NCI');
            $link = l($label, $url, $options);
            return array(
                '#type' => 'container',
                '#attributes' => array(
                    'id' => 'nci-docs-block',
                    'class' => array('collapsed', 'clearfix'),
                ),
                'section' => array(
                    '#prefix' => '<div class="float-left">',
                    '#suffix' => '</div>',
                    '#type' => 'markup',
                    '#markup' => "<h3 class='collapsed'>$link</h3>",
                ),
            );
        }

        // If we get here, the section is expanded.  Create the beginning
        // of the render array for the block.  We use two wrappers: one
        // to control spacing, one to wrap floats.
        $parms['se'] = str_replace('n', '', $expanded);
        $options = array('query' => $parms, 'html' => true);
        $label = $this->expanded('Documents Posted By NCI');
        $link = l($label, $url, $options);
        $block = array(
            'outer' => array(
                '#type' => 'container',
                '#attributes' => array(
                    'id' => 'nci-docs-block',
                    'class' => array('expanded clearfix'),
                ),
                'inner' => array(
                    '#type' => 'container',
                    '#attributes' => array(
                        'id' => 'nci-docs-block-inner',
                        'class' => array('clearfix'),
                    ),
                    'left side' => array(
                        '#type' => 'container',
                        '#attributes' => array(
                            'class' => array('float-left'),
                        ),
                        'heading' => array(
                            '#type' => 'markup',
                            '#markup' => "<h3 class='expanded'>$link</h3>",
                        ),
                    ),
                ),
            ),
        );

        // If the user is a manager, add a button to post another document.
        if ($admin) {
            $picklist = $this->get_nci_doc_picklist();
            if (count($picklist) > 0) {
                $href = 'javascript:ebmsscript.show_post_nci_doc_form();';
                $right_side = array(
                    'wrapper' => array(
                        '#type' => 'container',
                        '#attributes' => array(
                            'class' => array('float-right pad-top-10'),
                        ),
                        'button' => array(
                            '#type' => 'markup',
                            '#markup' =>
                            "<a href='$href' class='button'>Post Document</a>",
                        ),
                    ),
                );
                $block['outer']['inner']['right side'] = $right_side;
            }
        }

        // Create and add the table of posted documents.
        $block['outer']['table'] = $this->file_table('posted', $url);

        // If the user is a manager, add the hidden form that will pop
        // up if she clicks on the 'Post Document' button created above.
        if ($admin && count($picklist) > 0) {
            $form = drupal_get_form('pdq_ebms_nci_doc_form', $this,
                    $picklist);
            $id = 'post-summary-nd-form';
            $classes = 'popup doc-select-popup';
            $form['#prefix'] = "<div id='$id' class='$classes'>";
            $form['#suffix'] = '</div>';
            $block['post ncidoc form'] = $form;
        }
        return $block;
    }

    /**
     * Builds the portion of the page showing documents returned by board
     * members.
     *
     *   Passed:
     *
     *     $url       how we get back to this sub-page
     *
     *     $expanded  string containing one letter for each section
     *                of the sub-page which has been expanded:
     *                'c' for cancer.gov summaries; 'n' for
     *                summary documents posted by NCI; or 'm'
     *                for documents returned by board members.
     */
    private function member_docs($url, $expanded) {

        // If the section is collapsed, just show its title, with a link
        // to expand it.
        if (strpos($expanded, 'm') === false) {
            $parms = $this->parms;
            $parms['se'] = $expanded . 'm';
            $options = array('query' => $parms, 'html' => true);
            $label = $this->collapsed('Documents Posted by Board Members');
            $link = l($label, $url, $options);
            return array(
                '#type' => 'container',
                '#attributes' => array(
                    'id' => 'member-docs-block',
                    'class' => array('collapsed', 'clearfix'),
                ),
                'section' => array(
                    '#prefix' => '<div class="float-left">',
                    '#suffix' => '</div>',
                    '#type' => 'markup',
                    '#markup' => "<h3 class='collapsed'>$link</h3>",
                ),
            );
        }

        // If we get here, the section is expanded.  Create the beginning
        // of the render array for the block.  We use two wrappers: one
        // to control spacing, one to wrap floats.
        $admin = $this->role == 'admin';
        $parms['se'] = str_replace('m', '', $expanded);
        $options = array('query' => $parms, 'html' => true);
        $label = $this->expanded('Documents Posted By Board Members');
        $link = l($label, $url, $options);
        $block = array(
            'outer' => array(
                '#type' => 'container',
                '#attributes' => array(
                    'id' => 'member-docs-block',
                    'class' => array('expanded clearfix'),
                ),
                'inner' => array(
                    '#type' => 'container',
                    '#attributes' => array(
                        'id' => 'member-docs-block-inner',
                        'class' => array('clearfix'),
                    ),
                    'left side' => array(
                        '#type' => 'container',
                        '#attributes' => array(
                            'class' => array('float-left'),
                        ),
                        'heading' => array(
                            '#type' => 'markup',
                            '#markup' => "<h3 class='expanded'>$link</h3>",
                        ),
                    ),
                ),
            ),
        );

        // If the user is not a manager, add a button to post another document.
        if (!$admin) {
            $post_url = EbmsSummaries::$url . '/member-post/';
            $post_url .= $this->board_id . '/' . $this->subpage_id;
            $options = array(
                'query' => $this->parms,
                'attributes' => array('class' => array('button')),
            );
            $right_side = array(
                'wrapper' => array(
                    '#type' => 'container',
                    '#attributes' => array(
                        'class' => array('float-right pad-top-10'),
                    ),
                    'button' => array(
                        '#type' => 'markup',
                        '#markup' => l('Post Document', $post_url, $options),
                    ),
                ),
            );
            $block['outer']['inner']['right side'] = $right_side;
        }

        // Create and add the table of returned documents.
        $block['outer']['table'] = $this->file_table('returned', $url);
        if (!$admin) {
            $form = drupal_get_form('pdq_ebms_member_doc_form', $this);
            $id = 'file-upload-form-js';
            $classes = 'popup';
            $form['#prefix'] = "<div id='$id' class='$classes'>";
            $form['#suffix'] = '</div>';
            $form['filepath']['#title_display'] = 'after';
            $block['post memberdoc form'] = $form;
        }

        // The render array is complete; return it.
        return $block;
    }

    /**
     * Common function to build the table headers for both the "Documents
     * Posted by NCI" table and the "Documents Posted by Board Members"
     * table.
     *
     * Passed:
     *
     *   $code        'p' for documents posted by NCI, or 'r' for
     *                documents returned by board members
     *
     *   $url         URL string that knows how to get back to this
     *                summaries page (because it has the IDs for
     *                the board and sub-page embedded in it)
     */
    private function file_table_headers($code, $url) {

        // Start with the columns that everyone sees.
        $headers = array(
            array(
                'data' => 'File Name',
                'field' => 'd.description',
                'class' => array('col-1'),
            ),
            array(
                'data' => 'Notes',
                'field' => 't.notes',
                'class' => array('col-2'),
            ),
            array(
                'data' => 'Uploaded By',
                'field' => 'u.name',
                'class' => array('col-3'),
            ),
            array(
                'data' => 'Date',
                'field' => 'd.when_posted',
                'class' => array('col-4'),
            ),
        );

        // If the current user is a manager, add a fifth column with
        // a button toggle between showing and suppressing archived
        // documents.
        if ($this->role == 'admin') {
            $parms = $this->parms;
            $sa_parm = isset($parms['sa']) ? $parms['sa'] : '';
            if (strpos($sa_parm, $code) === false) {
                $parms['sa'] = $sa_parm . $code;
                $title = 'Show Archived Documents';
            }
            else {
                $parms['sa'] = str_replace($code, '', $sa_parm);
                $title = 'Don\'t Show Archived Documents';
            }
            $options = array(
                'query' => $parms,
                'attributes' => array('title' => $title),
            );
            $headers[] = array(
                'data' => l('Archive', $url, $options),
                'class' => array('col-5'),
            );
        }

        // Headers are complete; return them.
        return $headers;
    }

    /**
     * Common code for assembling a table for one of the summary pages
     *
     * Passed:
     *
     *   $doc_type    'supporting': table of board-specific supporting
     *                              documents on the Summaries landing
     *                              page; or
     *                'posted':     table of summary documents posted
     *                              by a board manager for a specific
     *                              summary sub-page; or
     *                'returned':   table of documents returned by
     *                              a board manager for a specific
     *                              summary sub-page
     *
     *   $url         URL string that knows how to get back to this
     *                summaries page (because it has the IDs for
     *                the board and sub-page embedded in it)
     */
    private function file_table($doc_type, $url) {

        // Construct the query we'll use to populate the table.
        $code = substr($doc_type, 0, 1);
        $sa_parm = isset($this->parms['sa']) ? $this->parms['sa'] : '';
        $query = db_select("ebms_summary_{$doc_type}_doc", 't');
        $query = $query->extend('TableSort');
        $query->join('ebms_doc', 'd', 'd.doc_id = t.doc_id');
        $query->join('file_managed', 'f', 'f.fid = d.file_id');
        $query->join('users', 'u', 'u.uid = f.uid');
        $query->fields('t', array('archived', 'notes', 'doc_id'));
        $query->fields('d', array('when_posted', 'description'));
        $query->fields('f', array('uri'));
        $query->fields('u', array('name'));
        if ($doc_type == 'supporting')
            $query->condition('t.board_id', $this->board_id);
        else
            $query->condition('t.page_id', $this->subpage_id);
        if ($this->role != 'admin' || strpos($sa_parm, $code) === false)
            $query->isNull('t.archived');
        $headers = $this->file_table_headers($code, $url);
        $query->orderByHeader($headers);
        if (false)
            dpq($query);
        $results = $query->execute();

        // Build the array of data rows for the table from the query results.
        $rows = array();
        foreach ($results as $doc) {
            $file_url = file_create_url($doc->uri);
            $link = l($doc->description, $file_url);
            $data = array(
                $link,
                htmlspecialchars($doc->notes),
                htmlspecialchars($doc->name),
                date('m/d/Y', strtotime($doc->when_posted)),
            );
            if ($this->role == 'admin') {
                $parms = $this->parms;
                $action = $doc->archived ? 'revive' : 'archive';
                $parms[$action] = $code . '-' . $doc->doc_id;
                pdq_ebms_debug('ARCHIVE BUTTON', array($code, $parms));
                $href = url($url, array('query' => $parms));
                $type = 'type="checkbox"';
                $onclick = 'onclick=\'location.href="' . $href . '"\'';
                $checked = $doc->archived ? ' checked="checked"' : '';
                $checkbox = "<input class='xyz' $type $onclick$checked />";
                $data[] = array(
                    'data' => $checkbox,
                    'class' => array('col-5'),
                );
            }
            $rows[] = array(
                'data' => $data,
                'no_striping' => true,
            );
        }

        // Assemble and return the render array for the table.
        return array(
            '#attributes' => array(
                'id' => "summary-{$doc_type}-docs-table",
                'class' => array($this->role, 'docs-table'),
            ),
            '#theme' => 'table',
            '#rows' => $rows,
            '#header' => $headers,
        );
    }

    /**
     * Show a menu of boards of which the user is a member.  Each board
     * has a link to show that board's Summaries page with the "Summaries"
     * section expanded, as well as a link to show the board's Summaries
     * page with the "Supporting Documents" section expanded.
     */
    private function show_boards() {

        // Override the default Drupal breadcrumb system.
        drupal_set_breadcrumb(array('Summaries'));

        // Make sure the user is associated with at least one board.
        if (count($this->user->boards) < 1)
            return '<p>Current user has no boards.</p>';

        // Get the list of all the boards to which this user belongs.
        $results = db_select('ebms_board', 'b')
            ->fields('b', array('board_id', 'board_name'))
            ->condition('b.board_id', $this->user->boards, 'IN')
            ->orderBy('b.board_name')
            ->execute();

        // Loop through the results of the query, adding the links to
        // jump to the page for a specific board, possibly with one
        // of the sections expanded.
        $base = EbmsSummaries::$url . '/board/';
        $boards = array();
        foreach ($results as $b) {
            $url = "$base$b->board_id";
            $options = array('html' => true, 'query' => array());
            $board_name = htmlspecialchars($b->board_name);
            $board_link = l($board_name, $url, $options);
            $options['query']['be'] = 's';
            $label = $this->collapsed('Summaries');
            $s_link = l($label, $url, $options);
            $options['query']['be'] = 'd';
            $label = $this->collapsed('Supporting Documents');
            $d_link = l($label, $url, $options);
            $boards[$b->board_id] = array(
                'board-link' => array(
                    '#type' => 'markup',
                    '#markup' => "<h2 class='low-key'>$board_link</h2>",
                ),
                'expand summaries' => array(
                    '#type' => 'markup',
                    '#markup' => "<h3 class='h3-first collapsed'>$s_link</h3>",
                ),
                'expand supporting docs' => array(
                    '#type' => 'markup',
                    '#markup' => "<h3 class='h3-last collapsed'>$d_link</h3>",
                ),
            );
        }

        // Assemble the render array for the page and return it.
        return array(
            'page' => array(
                '#type' => 'container',
                '#attributes' => array('id' => 'summaries'),
                'boards' => $boards,
            ),
        );
    }

    /**
     * Displays a page for a single board of which the user is a member,
     * with a section showing a menu of subpages for that board's
     * summaries, and a section listing supporting documents related
     * to all the summaries for this board.  Both sections are
     * collapsible (and are collapsed by default).
     */
    private function show_board() {

        // Override the default Drupal breadcrumb system.
        $board_name = htmlspecialchars(
            Ebms\Util::get_board_name($this->board_id));
        drupal_set_breadcrumb(
            array(
                l('Summaries', EbmsSummaries::$url),
                $board_name,
            )
        );

        // If the user checked the box to archive one of the documents,
        // update the row in the appropriate table to reflect the new
        // status.  This action (available only to managers) suppresses
        // the document from displays except to managers when the option
        // to include archived documents is turned on.  The 'archive'
        // parameter's value contains the code for the document's type
        // (we're only concerned with document type of 's' for 'supporting'
        // document in this case) and the document's id, separated by a
        // hyphen; e.g., archive=s-234.
        if (isset($this->parms['archive'])) {
            $now = strftime("%Y-%m-%d %H:%M:%S");
            list($doc_type, $doc_id) = explode('-', $this->parms['archive']);
            if ($doc_type == 's') {
                db_update('ebms_summary_supporting_doc')
                    ->fields(array('archived' => $now))
                    ->condition('doc_id', $doc_id)
                    ->execute();
            }

            // Not enough to pull it out of our own set; table sorting
            // URLs will go back to the original query for parameters.
            unset($_GET['archive']);
            unset($this->parms['archive']);
        }

        // The manager can uncheck the 'archive' box to undo the
        // action described above for the 'archive' parameter.  This
        // will set the 'revive' parameter, which takes the same form
        // as the 'archive' parameter above.  We clear the 'archived'
        // column for the document.
        if (isset($this->parms['revive'])) {
            list($doc_type, $doc_id) = explode('-', $this->parms['revive']);
            if ($doc_type == 's') {
                db_update('ebms_summary_supporting_doc')
                    ->fields(array('archived' => null))
                    ->condition('doc_id', $doc_id)
                    ->execute();
            }
            unset($_GET['revive']);
            unset($this->parms['revive']);
        }

        // Assemble the parts of the page.
        return array(
            'page' => array(
                '#type' => 'container',
                '#attributes' => array('id' => 'summaries'),
                'heading' => array(
                    '#type' => 'markup',
                    '#markup' => "<h2 class='low-key'>$board_name</h2>",
                ),
                'subpages' => $this->subpage_menu(),
                'supporting docs' => $this->supporting_docs(),
            ),
        );
    }

    /**
     * Assembles the menu of summaries subpages for a given board.
     * The menu will be collapsed by default.
     *
     *  @return  array     Drupal render array for the (possibly
     *                     collapsed) list of links to subpages
     *                     for a single board
     */
    private function subpage_menu() {

        // The 'be' parameter in the query request conveys which
        // sections of the Board (b) page are Expanded (e).  If
        // the parameter's value contains the letter 's' anywhere
        // then the menu of Subpages is expanded.  By default
        // it won't be, and we'll just show the header for the
        // Subpages section collapsed, with a link to expand it.
        $query = $this->parms;
        $expanded = isset($query['be']) ? $query['be'] : '';
        if (strpos($expanded, 's') === false) {
            $url = EbmsSummaries::$url . '/board/' . $this->board_id;
            $query['be'] = $expanded . 's';
            $options = array('query' => $query, 'html' => true);
            $label = $this->collapsed('Summaries');
            $link = l($label, $url, $options);
            return array(
                '#type' => 'container',
                '#attributes' => array(
                    'id' => 'subpage-menu-block',
                    'class' => array('collapsed', 'clearfix'),
                ),
                'section' => array(
                    '#prefix' => '<div class="float-left">',
                    '#suffix' => '</div>',
                    '#type' => 'markup',
                    '#markup' => "<h3 class='collapsed'>$link</h3>",
                ),
            );
        }

        // If we get here, the Subpage menu section has been expanded.
        // Find all of the summary subpages created for this board and
        // create a list of links to those pages.
        $results = db_select('ebms_summary_page', 'p')
            ->fields('p', array('page_id', 'page_name'))
            ->condition('p.board_id', $this->board_id)
            ->orderBy('p.page_name')
            ->execute();
        $pages = array();
        $options = array('query' => $this->parms);
        $url = EbmsSummaries::$url . '/page/' . $this->board_id . '/';
        foreach ($results as $page)
            $pages[] = l($page->page_name, $url . $page->page_id, $options);
        $url = EbmsSummaries::$url . '/board/' . $this->board_id;
        $query['be'] = str_replace('s', '', $expanded);
        $options = array('query' => $query, 'html' => true);
        $label = $this->expanded('Summaries');
        $link = l($label, $url, $options);

        // Put together the Drupal render array for the list of links
        // to summary subpages.
        $block = array(
            'wrapper' => array(
                '#type' => 'container',
                '#attributes' => array(
                    'id' => 'subpage-menu-block',
                    'class' => array('expanded clearfix'),
                ),
                'left side' => array(
                    '#type' => 'container',
                    '#attributes' => array(
                        'class' => array('float-left'),
                    ),
                    'heading' => array(
                        '#type' => 'markup',
                        '#markup' => "<h3 class='expanded'>$link</h3>",
                    ),
                    'links' => array(
                        '#theme' => 'item_list',
                        '#items' => $pages,
                        '#attributes' => array('class' => array('no-bullets')),
                    ),
                ),
            ),
        );

        // If the user is a board manager, allow her to create new subpages.
        if ($this->role == 'admin') {
            $href = 'javascript:ebmsscript.show_new_summaries_page_form();';
            $right_side = array(
                'wrapper' => array(
                    '#type' => 'container',
                    '#attributes' => array(
                        'class' => array('float-right pad-top-10'),
                    ),
                    'button' => array(
                        '#type' => 'markup',
                        '#markup' =>
                        "<a href='$href' class='button'>Add New Subpage</a>",
                    ),
                ),
            );
            $block['wrapper']['right side'] = $right_side;
            $form = drupal_get_form('pdq_ebms_new_summary_page_form', $this);
            $form['#prefix'] =
                '<div id="new-summary-page-form" class="popup">';
            $form['#suffix'] = '</div>';
            $block['new page form'] = $form;
        }

        // Return the Drupal render array to the caller.
        return $block;
    }

    /**
     * Assembles the list of supporting documents uploaded for a particular
     * board.  These are documents which are not specific to any of the
     * board's summary subpages.  The list will be collapsed by default.
     *
     *  @return  array     Drupal render array for the (possibly
     *                     collapsed) list of links to supporting
     *                     documents for a single board
     */
    private function supporting_docs() {

        // The 'be' parameter in the query request conveys which
        // sections of the Board (b) page are Expanded (e).  If
        // the parameter's value contains the letter 'd' anywhere
        // then the list of supporting documents is expanded.  By
        // default it won't be, and we'll just show the header for
        // the Supporting Documents  section collapsed, with a link
        // to expand it.
        $parms = $this->parms;
        $expanded = isset($parms['be']) ? $parms['be'] : '';
        $url = EbmsSummaries::$url . '/board/' . $this->board_id;
        if (strpos($expanded, 'd') === false) {
            $parms['be'] = $expanded . 'd';
            $options = array('query' => $parms, 'html' => true);
            $label = $this->collapsed('Supporting Documents');
            $link = l($label, $url, $options);
            return array(
                '#type' => 'container',
                '#attributes' => array(
                    'id' => 'supporting-docs-block',
                    'class' => array('float-clear', 'collapsed'),
                ),
                'section' => array(
                    '#prefix' => '<div class="float-left">',
                    '#suffix' => '</div>',
                    '#type' => 'markup',
                    '#markup' => "<h3 class='collapsed'>$link</h3>",
                ),
            );
        }

        // If we get here, the Supporting Documents section has been
        // expanded.  Assemble a table showing those documents.  We
        // start with a header line which has the title of the section
        // as a link for collapsing the section.
        $parms['be'] = str_replace('d', '', $expanded);
        $options = array('query' => $parms, 'html' => true);
        $label = $this->expanded('Supporting Documents');
        $link = l($label, $url, $options);
        $block = array(
            'wrapper' => array(
                '#type' => 'container',
                '#attributes' => array(
                    'id' => 'supporting-docs-block',
                    'class' => array('float-clear', 'expanded'),
                ),
                'left side' => array(
                    '#type' => 'container',
                    '#attributes' => array(
                        'class' => array('float-left'),
                    ),
                    'heading' => array(
                        '#type' => 'markup',
                        '#markup' => "<h3 class='expanded'>$link</h3>",
                    ),
                ),
            ),
        );

        // If the current user is a board manager, and there are any
        // documents posted to the system for the page's board, tagged
        // with both the 'summary' and the 'support' tags, then
        // add a button on the right side of the section header line
        // which the board manager can use to add more documents to
        // the section.
        $admin = $this->role == 'admin';
        if ($admin) {
            $picklist = $this->get_supporting_doc_picklist();
            if (count($picklist) > 0) {
                $href = 'javascript:ebmsscript.show_post_summaries_sd_form();';
                $right_side = array(
                    'wrapper' => array(
                        '#type' => 'container',
                        '#attributes' => array(
                            'class' => array('float-right pad-top-10'),
                        ),
                        'button' => array(
                            '#type' => 'markup',
                            '#markup' =>
                            "<a href='$href' class='button'>Post Document</a>",
                        ),
                    ),
                );
                $block['wrapper']['right side'] = $right_side;
            }
        }

        // The documents will be displayed in a table.  Start with the
        // table headers.
        $headers = array(
            array(
                'data' => 'File Name',
                'field' => 'd.description',
                'class' => array('col-1'),
            ),
            array(
                'data' => 'Notes',
                'field' => 's.notes',
                'class' => array('col-2'),
            ),
            array(
                'data' => 'Uploaded By',
                'field' => 'u.name',
                'class' => array('col-3'),
            ),
            array(
                'data' => 'Date',
                'field' => 'd.when_posted',
                'class' => array('col-4'),
            ),
        );

        // Board managers get a fifth column, allowing the user to
        // see supporting documents which have been archived, and
        // to control which supporting documents are archived.
        if ($admin) {
            $sa_parm = isset($parms['sa']) ? $parms['sa'] : '';
            $parms = $this->parms;
            if (strpos($sa_parm, 's') === false) {
                $parms['sa'] = $sa_parm . 's';
                $title = 'Show Archived Documents';
            }
            else {
                $parms['sa'] = str_replace('s', '', $sa_parm);
                $title = 'Don\'t Show Archived Documents';
            }
            $options = array(
                'query' => $parms,
                'attributes' => array('title' => $title),
            );
            $headers[] = array(
                'data' => l('Archive', $url, $options),
                'class' => array('col-5'),
            );
        }

        // Build the query to find the supporting documents for
        // this board's summary page.
        $query = db_select('ebms_summary_supporting_doc', 's');
        $query = $query->extend('TableSort');
        $query->join('ebms_doc', 'd', 'd.doc_id = s.doc_id');
        $query->join('file_managed', 'f', 'f.fid = d.file_id');
        $query->join('users', 'u', 'u.uid = f.uid');
        $query->fields('s', array('archived', 'notes', 'doc_id'));
        $query->fields('d', array('when_posted', 'description'));
        $query->fields('f', array('uri'));
        $query->fields('u', array('name'));
        $query->condition('s.board_id', $this->board_id);
        if (!$admin || strpos($sa_parm, 's') === false)
            $query->isNull('s.archived');
        $query->orderByHeader($headers);

        // Build up the rows for the table from the query results.
        $results = $query->execute();
        $rows = array();
        foreach ($results as $doc) {
            $url = file_create_url($doc->uri);
            $link = l($doc->description, $url);
            $data = array(
                $link,
                htmlspecialchars($doc->notes),
                htmlspecialchars($doc->name),
                date("m/d/Y", strtotime($doc->when_posted)),
            );
            if ($admin) {
                $parms = $this->parms;
                $action = $doc->archived ? 'revive' : 'archive';
                $parms[$action] = 's-' . $doc->doc_id;
                $url = EbmsSummaries::$url . '/board/' . $this->board_id;
                $url = url($url, array('query' => $parms));
                $type = 'type="checkbox"';
                $onclick = 'onclick=\'location.href="' . $url . '"\'';
                $checked = $doc->archived ? ' checked="checked"' : '';
                $checkbox = "<input $type $onclick$checked />";
                $data[] = array(
                    'data' => $checkbox,
                    'class' => array('col-5'),
                );
            }
            $rows[] = array(
                'data' => $data,
                'no_striping' => true,
            );
        }

        // Plug the table in the render array.
        $class = $admin ? 'admin' : 'non-admin';
        $block['table'] = array(
            '#attributes' => array(
                'id' => 'supp-docs-table',
                'class' => array($class),
            ),
            '#theme' => 'table',
            '#rows' => $rows,
            '#header' => $headers,
        );

        // If we have a button for adding more documents to the table,
        // we'll need the form that the button launches.  The form
        // is hidden at first, and Javascript brings it to life when
        // the button is clicked.
        if ($admin && count($picklist) > 0) {
            $form = drupal_get_form('pdq_ebms_post_summary_sd_form', $this,
                    $picklist);
            $id = 'post-summary-sd-form';
            $classes = 'popup doc-select-popup';
            $form['#prefix'] = "<div id='$id' class='$classes'>";
            $form['#suffix'] = '</div>';
            $block['post supporting doc form'] = $form;
        }

        // Return the Drupal render array to the caller.
        return $block;
    }

    /**
     * Finds all of the documents uploaded to the EBMS with the 'summary'
     * tag and with the 'support' tag, and which have been linked with
     * at the board for which we are building the current summary page,
     * and which have not already been assigned to the board's summary
     * page.
     */
    private function get_supporting_doc_picklist() {
        $subquery = db_select('ebms_summary_supporting_doc', 'd')
            ->fields('d', array('doc_id'))
            ->condition('d.board_id', $this->board_id);
        $query = db_select('ebms_doc', 'd');
        $query->join('ebms_doc_tag', 'dt1', 'dt1.doc_id = d.doc_id');
        $query->join('ebms_doc_tag', 'dt2', 'dt2.doc_id = d.doc_id');
        $query->join('ebms_tag', 't1', 't1.tag_id = dt1.tag_id');
        $query->join('ebms_tag', 't2', 't2.tag_id = dt2.tag_id');
        $query->join('ebms_doc_board', 'b', 'b.doc_id = d.doc_id');
        $query->fields('d', array('doc_id', 'description'));
        $query->condition('b.board_id', $this->board_id);
        $query->condition('t1.tag_name', 'summary');
        $query->condition('t2.tag_name', 'support');
        $query->condition('d.doc_id', $subquery, 'NOT IN');
        $query->orderBy('d.description');
        if (false)
            dpq($query);
        $results = $query->execute();
        $picklist = array();
        foreach ($results as $doc)
            $picklist[] = $doc;
        return $picklist;
    }

    /**
     * Finds all of the documents uploaded to the EBMS with the 'summary'
     * tag but not the 'support' tag, and which have been linked with
     * at least one of the topics connected with this summary subpage,
     * and which have not already been assigned to the current subpage.
     */
    private function get_nci_doc_picklist() {
        $subquery1 = db_select('ebms_summary_posted_doc', 'd')
            ->fields('d', array('doc_id'))
            ->condition('d.page_id', $this->subpage_id);
        $subquery2 = db_select('ebms_summary_page_topic', 't')
            ->fields('t', array('topic_id'))
            ->condition('t.page_id', $this->subpage_id);
        $query = db_select('ebms_doc', 'd');
        $query->join('ebms_doc_topic', 'topic', 'topic.doc_id = d.doc_id');
        $query->join('ebms_doc_tag', 'dt1', 'dt1.doc_id = d.doc_id');
        $query->join('ebms_tag', 't1', 't1.tag_id = dt1.tag_id');
        $query->leftJoin('ebms_doc_tag', 'dt2', 'dt2.doc_id = d.doc_id');
        $query->leftJoin('ebms_tag', 't2',
            't2.tag_id = dt2.tag_id AND t2.tag_name = \'support\'');
        $query->fields('d', array('doc_id', 'description'));
        $query->condition('t1.tag_name', 'summary');
        $query->condition('t2.tag_id', null);
        $query->condition('d.doc_id', $subquery1, 'NOT IN');
        $query->condition('topic.topic_id', $subquery2, 'IN');
        $query->distinct();
        $query->orderBy('d.description');
        $results = $query->execute();
        $picklist = array();
        foreach ($results as $doc)
            $picklist[] = $doc;
        return $picklist;
    }

    private static function collapsed($title) {
        return "<span>$title</span> &nbsp; +";
    }
    private static function expanded($title) {
        return "<span>$title</span> &nbsp; &ndash;";
    }
}

/**
 * Drupal form API callback which generates the form for creating a
 * new summary subpage for a specific board.
 *
 *  @param  array   $form         Structured array containing the
 *                                elements and properties of the form
 *  @param  array   &$form_state  Modifiable structured array
 *                                containing the current values
 *                                of the form, as well as other
 *                                form state information
 *  @param  array   $control      EbmsSummaries object carrying
 *                                information about the current
 *                                request
 *
 *  @return array                 Array containing specification
 *                                of the fields to appear on the
 *                                form, as well as other markup
 */
function pdq_ebms_new_summary_page_form($form, &$form_state, $control) {

    // Assemble a picklist of topics associated with the current board.
    $results = db_select('ebms_topic', 't')
        ->fields('t', array('topic_id', 'topic_name'))
        ->condition('t.board_id', $control->board_id)
        ->condition('t.active_status', 'A')
        ->orderBy('t.topic_name')
        ->execute();
    $topics = array();
    foreach ($results as $topic)
        $topics[$topic->topic_id] = htmlspecialchars($topic->topic_name);

    // Build the Drupal form array for the request.
    $form = array(
        'title' => array(
            '#type' => 'textfield',
        ),
        'topics' => array(
            '#type' => 'select',
            '#multiple' => true,
            '#options' => $topics,
            '#title' => 'Summary Topics',
            '#size' => min(count($topics), 15),
        ),
        'submit' => array(
            '#type' => 'submit',
            '#value' => 'Add Subpage',
        ),
        'board' => array(
            '#type' => 'hidden',
            '#value' => $control->board_id,
        ),
        'query-parms' => array(
            '#type' => 'hidden',
            '#value' => drupal_json_encode($control->parms),
        ),
    );
    return $form;
}

/**
 * Ensures that the request to create a new summaries subpage specifies
 * both a title for the subpage and at least one topic.
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
function pdq_ebms_new_summary_page_form_validate($form, &$form_state) {
    $title = trim($form_state['values']['title']);
    $topics = $form_state['values']['topics'];
    $selected_topics = array();
    foreach ($topics as $id => $val) {
        if ($val)
            $selected_topics[] = $id;
    }
    if (!$title || $title == 'Subpage Title')
        form_set_error('title', 'Title is required for new subpage.');
    if (count($selected_topics) < 1)
        form_set_error('topics', 'At least one topic must be selected.');
    $form_state['selected topics'] = $selected_topics;
}

/**
 * Create a new row in the table for summary subpages, as well as one
 * row for each topic associated with the subpage in a second table.
 * When we're done we construct a URL which we use to take the user
 * back to the page from which the form was launched.
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
function pdq_ebms_new_summary_page_form_submit($form, &$form_state) {

    $title = trim($form_state['values']['title']);
    $board = $form_state['values']['board'];
    $topics = $form_state['selected topics'];
    $page_id = db_insert('ebms_summary_page')
        ->fields(
            array(
                'board_id' => $board,
                'page_name' => $title,
            )
        )
        ->execute();
    foreach ($topics as $topic) {
        db_insert('ebms_summary_page_topic')
            ->fields(
                array(
                    'page_id' => $page_id,
                    'topic_id' => $topic,
                )
            )
            ->execute();
    }
    $parms = drupal_json_decode($form_state['values']['query-parms']);
    $options = array('query' => $parms);
    drupal_goto(EbmsSummaries::$url . '/board/' . $board, $options);
}

/**
 * Drupal form API callback which generates the form for creating a
 * new link to a PDQ summary on an EBMS summary subpage.
 *
 *  @param  array   $form         Structured array containing the
 *                                elements and properties of the form
 *  @param  array   &$form_state  Modifiable structured array
 *                                containing the current values
 *                                of the form, as well as other
 *                                form state information
 *  @param  array   $control      EbmsSummaries object carrying
 *                                information about the current
 *                                request
 *
 *  @return array                 Array containing specification
 *                                of the fields to appear on the
 *                                form, as well as other markup
 */
function pdq_ebms_new_cg_summary_form($form, &$form_state, $control) {
    return array(
        'url' => array(
            '#type' => 'textfield',
        ),
        'title' => array(
            '#type' => 'textfield',
        ),
        'board' => array(
            '#type' => 'hidden',
            '#value' => $control->board_id,
        ),
        'page' => array(
            '#type' => 'hidden',
            '#value' => $control->subpage_id,
        ),
        'submit' => array(
            '#type' => 'submit',
            '#value' => 'Add Link',
        ),
        'query-parms' => array(
            '#type' => 'hidden',
            '#value' => drupal_json_encode($control->parms),
        ),
    );
}

/**
 * Ensures that the request to create a new link to a PDQ summary
 * on cancer.gov specifies both a URL and a label.
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
function pdq_ebms_new_cg_summary_form_validate($form, &$form_state) {
    $url = trim($form_state['values']['url']);
    $title = trim($form_state['values']['title']);
    if (!$url)
        form_set_error('url', 'URL field is required.');
    if (!$title)
        form_set_error('url', 'Title field is required.');
}

/**
 * Create a new row in the table for links to PDQ summaries in cancer.gov.
 * When we're done we construct a URL which we use to take the user
 * back to the page from which the form was launched.
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
function pdq_ebms_new_cg_summary_form_submit($form, &$form_state) {
    $url = trim($form_state['values']['url']);
    $title = trim($form_state['values']['title']);
    $page = $form_state['values']['page'];
    $board = $form_state['values']['board'];
    db_insert('ebms_summary_link')
        ->fields(
            array(
                'page_id' => $page,
                'link_url' => $url,
                'link_label' => $title,
            )
        )
        ->execute();
    $parms = drupal_json_decode($form_state['values']['query-parms']);
    $options = array('query' => $parms);
    $url = EbmsSummaries::$url;
    $url = "$url/page/$board/$page";
    drupal_goto($url, $options);
}

/**
 * Drupal form API callback which generates the form for including
 * a document posted by the EBMS on the current subpage as a summmary
 * document provided by NCI which board members can download and possibly
 * edit and return with changes.
 *
 *  @param  array   $form         Structured array containing the
 *                                elements and properties of the form
 *  @param  array   &$form_state  Modifiable structured array
 *                                containing the current values
 *                                of the form, as well as other
 *                                form state information
 *  @param  array   $control      EbmsSummaries object carrying
 *                                information about the current
 *                                request
 *  @param  array   $docs         Results of the query to find the
 *                                documents eligible for inclusion
 *                                on the current subpage; this
 *                                query is performed in advance
 *                                because we need to know when
 *                                we're building the page on which
 *                                the link for this form would appear
 *                                whether any documents are eligible
 *                                at all
 *
 *  @return array                 Array containing specification
 *                                of the fields to appear on the
 *                                form, as well as other markup
 */
function pdq_ebms_nci_doc_form($form, &$form_state, $control, $docs) {

    // Build the picklist of documents eligible to be included on the page.
    $options = array(0 => 'SELECT DOCUMENT');
    foreach ($docs as $doc)
        $options[$doc->doc_id] = htmlspecialchars($doc->description);

    // Construct the structured array for the Drupal form API.
    $form = array(
        'doc' => array(
            '#type' => 'select',
            '#options' => $options,
        ),
        'notes' => array(
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
            '#type' => 'submit',
            '#value' => 'Post Document',
        ),
        'board' => array(
            '#type' => 'hidden',
            '#value' => $control->board_id,
        ),
        'page' => array(
            '#type' => 'hidden',
            '#value' => $control->subpage_id,
        ),
        'query-parms' => array(
            '#type' => 'hidden',
            '#value' => drupal_json_encode($control->parms),
        ),
    );
    return $form;
}

/**
 * Ensures that the request to include another summary document on
 * a summary subpage has selected the document to be included.
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
function pdq_ebms_nci_doc_form_validate($form, &$form_state) {
    $doc_id = $form_state['values']['doc'];
    if (!$doc_id)
        form_set_error('title', 'No document selected.');
}

/**
 * Create a new row in the table for summary documents included by NCI
 * on one of the Summary subpages for the current board.  When we're
 * done we construct a URL which we use to take the user back to the
 * page from which the form was launched.
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
function pdq_ebms_nci_doc_form_submit($form, &$form_state) {
    $doc_id = $form_state['values']['doc'];
    $notes = $form_state['values']['notes'];
    $board = $form_state['values']['board'];
    $page = $form_state['values']['page'];
    db_insert('ebms_summary_posted_doc')
        ->fields(
            array(
                'page_id' => $page,
                'doc_id' => $doc_id,
                'notes' => $notes,
            )
        )
        ->execute();
    $parms = drupal_json_decode($form_state['values']['query-parms']);
    $options = array('query' => $parms);
    $url = EbmsSummaries::$url;
    $url = "$url/page/$board/$page";
    drupal_goto($url, $options);
}

/**
 * Drupal form API callback which generates the form for including
 * a document posted by the EBMS as a supporting document on the Summaries
 * page for the current board.
 *
 *  @param  array   $form         Structured array containing the
 *                                elements and properties of the form
 *  @param  array   &$form_state  Modifiable structured array
 *                                containing the current values
 *                                of the form, as well as other
 *                                form state information
 *  @param  array   $control      EbmsSummaries object carrying
 *                                information about the current
 *                                request
 *  @param  array   $docs         Results of the query to find the
 *                                documents eligible for inclusion
 *                                on the current page; this
 *                                query is performed in advance
 *                                because we need to know when
 *                                we're building the page on which
 *                                the link for this form would appear
 *                                whether any documents are eligible
 *                                at all
 *
 *  @return array                 Array containing specification
 *                                of the fields to appear on the
 *                                form, as well as other markup
 */
function pdq_ebms_post_summary_sd_form($form, &$form_state, $control, $docs) {

    // Build the picklist of documents eligible to be included on the page.
    $options = array(0 => 'SELECT DOCUMENT');
    foreach ($docs as $doc)
        $options[$doc->doc_id] = htmlspecialchars($doc->description);

    // Construct the structured array for the Drupal form API.
    $form = array(
        'doc' => array(
            '#type' => 'select',
            '#options' => $options,
        ),
        'notes' => array(
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
            '#type' => 'submit',
            '#value' => 'Post Document',
        ),
        'board' => array(
            '#type' => 'hidden',
            '#value' => $control->board_id,
        ),
        'query-parms' => array(
            '#type' => 'hidden',
            '#value' => drupal_json_encode($control->parms),
        ),
    );
    return $form;
}

/**
 * Ensures that the request to include another supporting document on
 * the Summaries page for the current board has selected the document
 * to be included.
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
function pdq_ebms_post_summary_sd_form_validate($form, &$form_state) {
    $doc_id = $form_state['values']['doc'];
    if (!$doc_id)
        form_set_error('title', 'No document selected.');
}

/**
 * Create a new row in the table for supporting documents posted by
 * the board manager for the current board's Summaries page.  When we're
 * done we construct a URL which we use to take the user back to the
 * page from which the form was launched.
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
function pdq_ebms_post_summary_sd_form_submit($form, &$form_state) {
    $doc_id = $form_state['values']['doc'];
    $notes = $form_state['values']['notes'];
    $board = $form_state['values']['board'];
    db_insert('ebms_summary_supporting_doc')
        ->fields(
            array(
                'board_id' => $board,
                'doc_id' => $doc_id,
                'notes' => $notes,
            )
        )
        ->execute();
    $parms = drupal_json_decode($form_state['values']['query-parms']);
    $options = array('query' => $parms);
    drupal_goto(EbmsSummaries::$url . '/board/' . $board, $options);
}

/**
 * Drupal form API callback which generates the form used by a board
 * member for posting an edited summary document back to the current
 * Summaries subpage.
 *
 *  @param  array   $form         Structured array containing the
 *                                elements and properties of the form
 *  @param  array   &$form_state  Modifiable structured array
 *                                containing the current values
 *                                of the form, as well as other
 *                                form state information
 *  @param  array   $control      EbmsSummaries object carrying
 *                                information about the current
 *                                request
 *
 *  @return array                 Array containing specification
 *                                of the fields to appear on the
 *                                form, as well as other markup
 */
function pdq_ebms_member_doc_form($form, &$form_state, $control) {

    // Construct the structured array for the Drupal form API.
    return array(
        '#id' => 'member-doc-form',
        'title' => array(
            '#type' => 'markup',
            '#markup' => '<h2>Board Member Upload</h2>',
        ),
        'board' => array(
            '#type' => 'hidden',
            '#value' => $control->board_id,
        ),
        'page' => array(
            '#type' => 'hidden',
            '#value' => $control->subpage_id,
        ),
        'query-parms' => array(
            '#type' => 'hidden',
            '#value' => drupal_json_encode($control->parms),
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
        ),
    );
}

/**
 * Save the document posted by a board member for a summary subpage.
 * We add a row to the ebms_docs table after first installing the
 * file in Drupal's file_managed table.  When we're done we construct
 * a URL which will take the user back to the page from which the
 * upload form was launched.
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
function pdq_ebms_member_doc_form_submit($form, &$form_state) {

    // Collect state information.
    $page = $form_state['values']['page'];
    $board = $form_state['values']['board'];
    $filenotes = $form_state['values']['filenotes'];

    // Save the file (allow any extensions for now).
    $validators = array('file_validate_extensions' => array());
    $file = file_save_upload('filepath', $validators);
    if ($file) {
        $now = strftime("%Y-%m-%d %H:%M:%S");
        $file = file_move($file, 'public://');
        $file->status |= FILE_STATUS_PERMANENT;
        $file = file_save($file);
        $path_parts = pathinfo($file->filename);
        $filename = $path_parts['filename'];
        if (!$filenotes)
            $filenotes = null;
        $doc_id = db_insert('ebms_doc')
            ->fields(
                array(
                    'file_id' => $file->fid,
                    'when_posted' => $now,
                    'description' => $filename,
                )
            )
            ->execute();
        db_insert('ebms_summary_returned_doc')
            ->fields(
                array(
                    'doc_id' => $doc_id,
                    'page_id' => $page,
                    'notes' => $filenotes,
                )
            )
            ->execute();
    }
    else
        drupal_set_message('File not found', 'error');
    $parms = drupal_json_decode($form_state['values']['query-parms']);
    $options = array('query' => $parms);
    $url = EbmsSummaries::$url . '/page/' . $board . '/' . $page;
    drupal_goto($url, $options);
}

/**
 * Validation function to ensure that a document was selected for a file
 * upload field (Drupal's validation of required file upload fields is
 * broken).

 *  @param  array   $element      Structured array containing the
 *                                information about the current form
 *                                elements (ignored by this callback)
 *  @param  array   &$form_state  Modifiable structured array
 *                                containing the current values
 *                                of the form, as well as other
 *                                form state information (ignored
 *                                by this callback)
 */
function pdq_ebms_doc_file_required($element, &$form_state) {
    if ($_FILES['files']['name']['filepath'] == '') {
        form_set_error('filepath', t('Please select a document'));
    }
}
