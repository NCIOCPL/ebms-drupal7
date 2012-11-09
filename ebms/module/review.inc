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
function pdq_ebms_review($action=null, $packet_id=0, $article_id=0) {
    $review = new EbmsReview($action, $packet_id, $article_id);
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
        $action,
        $reviewed,
        $pager;
    public static $url = 'review';

    /**
     * Passing settings in the URL instead of in $_SESSION in order
     * to avoid dependency on Javascript being enabled on the user's
     * browser.
     */
    public function __construct($action, $packet_id, $article_id) {
        global $user;
        $this->user         = $user;
        $this->packet_id    = Ebms\Util::get_request_value('packet',
                              $packet_id);
        $this->article_id   = Ebms\Util::get_request_value('article',
                              $article_id);
        $this->expanded     = Ebms\Util::get_request_value('expanded');
        $this->sort_col     = Ebms\Util::get_request_value('sort-col', 'f');
        $this->sort_dir     = Ebms\Util::get_request_value('sort-dir', 'a');
        $this->action = $action;
        $this->reviewed     = Ebms\Util::get_request_value('reviewed');
        $this->pager        = Ebms\Util::get_request_value('pager');
        $this->packet_title = EbmsPacket::fetch_title($this->packet_id);
    }

    /**
     * Select the version of the page appropriate the user's role.
     */
    public function run() {
        Ebms\Util::add_js();
        drupal_add_css(Ebms\CSS_DIR . '/review.css');
        Ebms\Menu::$active = 'Literature';
        if (in_array('board member', $this->user->roles)) {
            if ($this->action == 'reviewer-upload') {
                return $this->reviewer_upload_page();
            }
            return $this->choose_member_review_page();
        }
        return $this->choose_manager_review_page();
        // XXX STUB
        return '<i>Stub for manager literature review page</i>';
    }

    /**
     * Pick the page for the board manager.
     */
    private function choose_manager_review_page() {
        if (false)
            pdq_ebms_debug('SWITCHER', $this->action);
        if ($this->action == 'packets')
            return $this->manager_packets();
        if ($this->action == 'reviews')
            return $this->manager_reviews();
        if ($this->action == 'add')
            return drupal_get_form('pdq_ebms_edit_packet_form');
        if ($this->action == 'edit')
            return drupal_get_form('pdq_ebms_edit_packet_form',
                $this->packet_id);
        if ($this->action == 'delete')
            return $this->delete_packet();
        return $this->manager_literature_landing_page();
    }

    /*
     * Marks the current packet as inactive so it won't show up
     * in users' queues, then redirects to the page which shows
     * the board manager's packets.
     */
    private function delete_packet() {
        $title = EbmsPacket::fetch_title($this->packet_id);
        $title = str_replace(' [Legacy]', '', $title);
        $updated = db_update('ebms_packet')
            ->fields(array('active_status' => 'I'))
            ->condition('packet_id', $this->packet_id)
            ->execute();
        if ($updated == 1)
            drupal_set_message("Packet '$title' has been deleted.");
        else
            drupal_set_message("Failure deleting packet '$title'.", "error");
        $opts = array('query' => drupal_get_query_parameters());
        drupal_goto(EbmsReview::$url . '/packets', $opts);
    }

    /**
     * Show the page a board manager uses to choose between managing
     * packets and reviewing board member feedback.
     */
    private function manager_literature_landing_page() {
        drupal_set_breadcrumb(array('Literature'));
        return array(
            'packets' => array(
                '#type' => 'markup',
                '#markup' => l('Literature Surveillance Packets',
                           EbmsReview::$url . '/packets'),
                '#prefix' => '<h2>',
                '#suffix' => '</h2>',
            ),
            'reviews' => array(
                '#type' => 'markup',
                '#markup' => l('Literature Surveillance Reviews',
                           EbmsReview::$url . '/reviews'),
                '#prefix' => '<h2 class="end-of-short-page">',
                '#suffix' => '</h2>',
            ),
        );
    }

    /**
     * Builds a fly-out menu option to switch between viewing
     * all packets (the default) or only those which have reviews
     * attached.
     */
    private function manager_packets_view_toggle($options) {
        if ($this->reviewed) {
            unset($options['query']['reviewed']);
            $label = 'All';
        }
        else {
            $options['query']['reviewed'] = true;
            $label = 'Reviewed';
        }
        $down_arrow = array(
            '#type' => 'markup',
            '#prefix' => 'View <span class="view-toggle-arrow">',
            '#markup' => Ebms\DOWN_ARROW,
            '#suffix' => '</span>',
        );
        $render_array = array(
            '#type' => 'container',
            '#attributes' => array('id' => 'manager-packets-view-toggle'),
            'under' => array(
                '#type' => 'container',
                '#attributes' => array('id' => 'view-toggle-under'),
                'over' => array(
                    '#type' => 'container',
                    '#attributes' => array('id' => 'view-toggle-over'),
                    'view' => array(
                        '#type' => 'markup',
                        'down-arrow' => $down_arrow,
                    ),
                    'link' => array(
                        '#type' => 'markup',
                        '#markup' =>
                        l($label, EbmsReview::$url . '/packets', $options),
                    ),
                ),
                'down-arrow' => $down_arrow,
            ),
        );
        return render($render_array);
    }

    /**
     * Show the packets associated with the manager who's logged in.
     * Another Drupal bug: the render array for a page callback is
     * not handled correctly when used as a page callback if the
     * top-level '#type' attribute is 'container'; there are no
     * plans to back-port any fixes for this bug to Drupal 7.
     * See http://drupal.org/node/1300290#comment-5093234.
     */
    private function manager_packets() {
        drupal_set_breadcrumb(
            array(
                l('Literature', EbmsReview::$url),
                'Literature Surveillance Packets',
            )
        );
        $headers = array(
            array(
                'data' => 'Packet Name',
                'field' => 'p.packet_title',
            ),
            array(
                'data' => 'Board',
                'field' => 'b.board_name',
            ),
            array(
                'data' => 'Uploaded By',
                'field' => 'u.name',
            ),
            array(
                'data' => 'Actions',
                'class' => array('col-4'),
            ),
        );
        $query = db_select('ebms_packet', 'p');
        if ($this->pager != 'off')
            $query = $query->extend('PagerDefault')->limit(10);
        $query = $query->extend('TableSort');
        $query->join('ebms_topic', 't', 't.topic_id = p.topic_id');
        $query->join('ebms_board', 'b', 'b.board_id = t.board_id');
        $query->join('ebms_board_member', 'm', 'm.board_id = b.board_id');
        $query->join('users', 'u', 'u.uid = p.created_by');
        if ($this->reviewed) {
            $query->join('ebms_article_review', 'r',
                'r.packet_id = p.packet_id');
            $query->distinct();
        }
        $query->fields('p', array('packet_id', 'packet_title'));
        $query->fields('b', array('board_name'));
        $query->fields('u', array('name'));
        $query->condition('m.user_id', $this->user->uid);
        $query->condition('p.active_status', 'A');
        $query->orderByHeader($headers);
        $packets = $query->execute();
        $divider = '<span class="action-divider">|</span>';
        $opts = array('query' => drupal_get_query_parameters());
        $main_url = EbmsReview::$url;
        $del_func = 'javascript:ebmsscript.delete_packet';
        $rows = array();
        foreach ($packets as $p) {
            #$options = array('query' => array('packet' => $p->packet_id));
            #$options['query']['action'] = 'edit';
            $edit_url = "$main_url/edit/{$p->packet_id}";
            $edit_link = l('EDIT', $edit_url, $opts);
            $del_parm = url("$main_url/delete/{$p->packet_id}", $opts);
            $del_url = "$del_func('$del_parm');";
            $del_link = '<a href="' . $del_url . '">DELETE</a>';
            $title = str_replace(' [Legacy]', '', $p->packet_title);
            $title_link = l(htmlspecialchars($title), $edit_url, $opts);
            $data = array(
                $title_link,
                htmlspecialchars($p->board_name),
                htmlspecialchars($p->name),
                "$edit_link $divider $del_link",
            );
            $rows[] = array(
                'data' => $data,
                'no_striping' => true,
            );
        }
        $view = $this->manager_packets_view_toggle($opts);
        $add = l('Add New', EbmsReview::$url . '/add', $opts);
        $title = 'Literature Surveillance Packets';
        $id = 'manager-packets-page-title';
        $table = array(
            'title' => array(
                '#type' => 'markup',
                '#markup' => "<h2 id='$id'>$title $add $view</h2>",
            ),
            'table' => array(
                '#attributes' => array('id' => 'manager-packets-table'),
                '#theme' => 'table',
                '#rows' => $rows,
                '#header' => $headers,
            ),
            'delete-confirm' => array(
                '#type' => 'container',
                '#attributes' => array(
                    'id' => 'confirm-packet-delete',
                    'title' => 'Delete Packet?',
                ),
                'confirm-message' => array(
                    '#type' => 'markup',
                    '#markup' =>
                    '<p>Are you sure you want to delete this packet?</p>',
                ),
            ),
        );
        if ($this->pager != 'off')
            $table['pager'] = array('#theme' => 'pager');
        else {
            unset($opts['query']['pager']);
            $url = url(EbmsReview::$url, $opts);
            $link = l('VIEW PAGES', EbmsReview::$url . '/packets', $opts);
            $table['pager'] = array(
                '#type' => 'markup',
                '#markup' => "<div id='pager-view-pages'>$link</div>",
            );
        }
        return $table;
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
            array('Reviewed Packets', 'review/reviews'),
            array('View / Edit Packets', 'review/packets'),
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

    /**
     * Show the reviews for this board manager's packets.  Switches to
     * the appropriate page type, if the user has drilled down to a
     * specific packet or article.
     */
    private function manager_reviews() {
        $opts = array('query' => drupal_get_query_parameters());
        if ($this->article_id)
            return $this->show_reviewed_article($opts);
        if ($this->packet_id)
            return $this->show_reviewed_packet($opts);
        drupal_set_breadcrumb(
            array(
                l('Literature', EbmsReview::$url),
                'Reviewed Packets',
            )
        );
        $left_nav = $this->build_left_nav('Reviewed Packets');
        $left_nav['board filter box'] = $this->board_filter_box($opts);
        return array(
            'left-nav' => $left_nav,
            'packets' => $this->reviewed_packet_list($opts),
        );
    }

    /*
     * Creates a set of checkboxes for identifying boards for which the
     * reviews should be displayed, rather than using the list of boards
     * of which the user is a member.
     */
    private function board_filter_box($opts) {
        $checked_boards = array();
        if (isset($opts['query']['boards']))
            $checked_boards = explode(',', $opts['query']['boards']);
        $items = array();
        $boards = Ebms\Util::boards();
        $url = EbmsReview::$url . '/reviews/' . $this->packet_id;
        $type = 'type="checkbox"';
        foreach ($boards as $id => $name) {
            if (in_array($id, $checked_boards)) {
                $checked = ' checked="checked"';
                $new_boards = array_diff($checked_boards, (array)$id);
            }
            else {
                $checked = '';
                $new_boards = array_merge($checked_boards, (array)$id);
            }
            if (count($new_boards) < 1)
                unset($opts['query']['boards']);
            else
                $opts['query']['boards'] = implode(',', $new_boards);
            $href = url($url, $opts);
            $onclick = 'onclick=\'location.href="' . $href . '"\'';
            $cb_id = "edit-boards-$id";
            $checkbox = "<input id='$cb_id' $type $onclick$checked />";
            $label = "<label class='board-name' for='$cb_id'>$name</label>";
            $item = "<div>$checkbox $label</div>";
            $items[] = $item;
        }
        $url = Ebms\IMAGES_DIR . '/left-nav-box-bottom.png';
        $img = "<img class='ebms-left-nav-block-bottom' src='$url' alt='' />";
        return array(
            '#theme' => 'item_list',
            '#title' => 'Filter by Board',
            '#items' => $items,
            '#attributes' => array('class' => 'ebms_left_nav_block'),
            '#prefix' => '<div class="more-actions">',
            '#suffix' => '</div>',
        );
    }

    /*
     * Builds the Drupal render array for the list of all the packets
     * connected with the user's board(s).  If the user has explicitly
     * identified boards for which she wants to see review, show
     * packets for those boards instead.
     */
    private function reviewed_packet_list($opts) {

        // See if the user has identified one or more boards.
        $boards = array();
        if (isset($opts['query']['boards']))
            $boards = explode(',', $opts['query']['boards']);

        // If not, find the boards of she's a member.
        if (count($boards) < 1) {
            $results = db_select('ebms_board_member', 'm')
                ->fields('m', array('board_id'))
                ->condition('m.user_id', $this->user->uid)
                ->execute();
            foreach ($results as $result)
                $boards[] = $result->board_id;
        }

        // If we don't have any boards, then there's nothing more to do.
        if (count($boards) < 1) {
            drupal_set_message('You are not linked to any boards');
            return array(
                '#type' => 'container',
                '#attributes' => array('id' => 'main-right-panel'),
                'page title' => array(
                    '#type' => 'markup',
                    '#markup' => '<h2>Reviewed Packets</h2>',
                ),
            );
        }

        // Set up the table headers, specifying sorting options.
        $headers = array(
            array(
                'data' => 'Packet Name',
                'field' => 'p.packet_title',
            ),
            array(
                'data' => 'Reviewer(s)',
            ),
            array(
                'data' => 'Updated',
                'field' => 'updated',
            ),
        );

        // Build the query to show the packets with reviews.
        $query = db_select('ebms_packet', 'p');
        if ($this->pager != 'off')
            $query = $query->extend('PagerDefault')->limit(10);
        $query = $query->extend('TableSort');
        $query->join('ebms_topic', 't', 't.topic_id = p.topic_id');
        $query->join('ebms_board', 'b', 'b.board_id = t.board_id');
        $query->join('ebms_article_review', 'r', 'r.packet_id = p.packet_id');
        $query->fields('p', array('packet_id', 'packet_title'));
        $query->addExpression('MAX(r.when_posted)', 'updated');
        $query->condition('b.board_id', $boards, 'IN');
        $query->condition('p.active_status', 'A');
        $query->orderByHeader($headers);
        $query->groupBy('p.packet_id');
        $packets = $query->execute();

        // Assemble the rows for the table.
        $rows = array();
        $opts = array('query' => drupal_get_query_parameters());
        foreach ($packets as $p) {
            $title = str_replace(' [Legacy]', '', $p->packet_title);
            $query = db_select('users', 'u');
            $query->fields('u', array('name'));
            $query->join('ebms_packet_reviewer', 'r', 'r.reviewer_id = u.uid');
            $query->condition('r.packet_id', $p->packet_id);
            $query->orderBy('u.name');
            $results = $query->execute();
            $reviewers = array();
            foreach ($results as $result)
                $reviewers[] = htmlspecialchars($result->name);
            $url = EbmsReview::$url . '/reviews/' . $p->packet_id;
            $link = l($title, $url); #, $opts);
            $data = array(
                $link,
                implode('<br />', $reviewers),
                date("m/d/Y", strtotime($p->updated)),
            );
            $rows[] = array(
                'data' => $data,
                'no_striping' => true,
            );
        }

        // Assemble the render array for the table and return it.
        $title = 'Literature Surveillance Packets';
        $id = 'manager-packets-page-title';
        if ($this->pager != 'off')
            $pager = array('#theme' => 'pager');
        else {
            unset($opts['query']['pager']);
            $link = l('VIEW PAGES', EbmsReview::$url . '/reviews', $opts);
            $pager = array(
                '#type' => 'markup',
                '#markup' => "<div id='pager-view-pages'>$link</div>",
            );
        }
        $table = array(
            '#type' => 'container',
            '#attributes' => array('id' => 'main-right-panel'),
            'title' => array(
                '#type' => 'markup',
                '#markup' => '<h2>Reviewed Packets</h2>',
            ),
            'report button' => array(
                '#type' => 'markup',
                '#markup' => '<a class="button" href="stub">Create Report</a>',
            ),
            'top pager' => $pager,
            'table' => array(
                '#attributes' => array('id' => 'manager-packets-table'),
                '#theme' => 'table',
                '#rows' => $rows,
                '#header' => $headers,
            ),
            'bottom pager' => $pager,
        );
        return $table;
    }

    /*
     * Assembles the full citation display for a single article.
     * XXX to be rewritten when Alan's API for getting the citation
     * elements is complete.
     */
    private function get_reviewed_citation($article_id, $add_link=true) {
        $xml = db_select('ebms_article', 'a')
            ->fields('a', array('source_data'))
            ->condition('a.article_id', $article_id)
            ->execute()
            ->fetchField();
        $doc = new \SimpleXMLElement($xml);
        $title = $ta = $vol = $issue = $pgn = $year = $pmid = '';
        $article = $doc->MedlineCitation->Article;
        $title = htmlspecialchars($article->ArticleTitle);
        if ($add_link) {
            $url = EbmsReview::$url . '/reviews/' . $this->packet_id . '/' .
                $article_id;
            $title = l($title, $url);
        }
        $pgn = htmlspecialchars($article->Pagination->MedlinePgn);
        $year = $article->Journal->JournalIssue->PubDate->Year;
        $issue = htmlspecialchars($article->Journal->JournalIssue->Issue);
        $vol = htmlspecialchars($article->Journal->JournalIssue->Volume);
        $authors = array();
        foreach ($doc->MedlineCitation->Article->AuthorList->Author as $author)
        {
            if (sizeof($authors) == 3) {
                $authors[] = 'et al.';
                break;
            }
            $name = $author->LastName;
            if ($author->Initials)
                $name .= ' ' . $author->Initials;
            $authors[] = htmlspecialchars($name);
        }
        $ta = $doc->MedlineCitation->MedlineJournalInfo->MedlineTA;
        $ta = htmlspecialchars($ta);
        $pmid = $doc->MedlineCitation->PMID;
        $pieces = array(implode(',', $authors), '<br /><em>', $title,
                  '</em><br />', $ta, ' ', $vol);
        if ($issue)
            $pieces[] = "($issue)";
        if ($pgn)
            $pieces[] = ": $pgn";
        if ($year)
            $pieces[] = ", $year";
        $pieces[] = ".<br />PMID: $pmid";
        return implode($pieces);
    }

    /*
     */
    private function show_reviewed_packet($opts) {
        if (isset($opts['query']['archive'])) {
            $article_id = $opts['query']['archive'];
            if ($article_id) {
                $now = strftime("%Y-%m-%d %H:%M:%S");
                db_update('ebms_packet_article')
                    ->fields(array('archived' => $now))
                    ->condition('article_id', $article_id)
                    ->condition('packet_id', $this->packet_id)
                    ->execute();
            }

            // Not enough to pull it out of our own set; table sorting
            // URLs will go back to the original query for parameters.
            unset($_GET['archive']);
            unset($opts['query']['archive']);
        }
        if (isset($opts['query']['revive'])) {
            $article_id = $opts['query']['revive'];
            if ($article_id) {
                db_update('ebms_packet_article')
                    ->fields(array('archived' => null))
                    ->condition('article_id', $article_id)
                    ->condition('packet_id', $this->packet_id)
                    ->execute();
            }

            // See note above about clearing the 'archive' parameter.
            unset($_GET['revive']);
            unset($opts['query']['revive']);
        }

        $url = EbmsReview::$url . '/reviews/' . $this->packet_id;
        drupal_set_breadcrumb(
            array(
                l('Literature', EbmsReview::$url),
                l('Reviewed Packets', EbmsReview::$url . '/reviews'),
                $this->packet_title,
            )
        );
        if ($this->pager != 'off')
            $pager = array('#theme' => 'pager');
        else {
            unset($opts['query']['pager']);
            $link = l('VIEW PAGES', EbmsReview::$url . '/reviews', $opts);
            $pager = array(
                '#type' => 'markup',
                '#markup' => "<div id='pager-view-pages'>$link</div>",
            );
        }
        $archive_opts = $opts;
        if (isset($archive_opts['query']['sa'])) {
            $archive_title = 'Don\'t Show Archived Articles';
            unset($archive_opts['query']['sa']);
            $sa = true;
        }
        else {
            $archive_title = 'Show Archived Articles';
            $archive_opts['query']['sa'] = 1;
            $sa = false;
        }
        $archive_opts['attributes'] = array('title' => $archive_title);
        $headers = array(
            array(
                'data' => 'Article',
                'field' => 'a.article_title',
            ),
            array(
                'data' => 'Reviewer&nbsp; &nbsp; | &nbsp; &nbsp;Disposition',
                'class' => array('col-2'),
            ),
            array(
                'data' => 'Updated',
                'field' => 'updated',
            ),
            array(
                'data' => l('Archive', $url, $archive_opts),
                'class' => array('col-4'),
            ),
        );
        $warrants_no_changes = db_select('ebms_review_disposition_value', 'v')
            ->fields('v', array('value_id'))
            ->condition('v.value_name', 'Warrants no changes to the summary')
            ->execute()
            ->fetchField();
        $query = db_select('ebms_packet_article', 'p');
        $query = $query->extend('TableSort');
        $query->join('ebms_article', 'a', 'a.article_id = p.article_id');
        $query->join('ebms_article_review', 'r',
            'r.article_id = a.article_id');
        $query->join('ebms_review_disposition', 'd',
            'd.review_id = r.review_id');
        $query->fields('a', array('article_id', 'article_title'));
        $query->fields('p', array('archived'));
        $query->addExpression('MAX(r.when_posted)', 'updated');
        $query->condition('p.packet_id', $this->packet_id);
        if (!$sa)
            $query->isNull('p.archived');
        $query->groupBy('a.article_id');
        $query->orderByHeader($headers);
        $articles = $query->execute();
        $rows = array();
        foreach ($articles as $article) {
            $citation = $this->get_reviewed_citation($article->article_id);
            $query = db_select('users', 'u');
            $query->fields('u', array('name'));
            $query->join('ebms_article_review', 'r', 'r.reviewer_id = u.uid');
            $query->fields('r', array('review_id'));
            $query->join('ebms_review_disposition', 'd',
                'd.review_id = r.review_id');
            $query->condition('r.packet_id', $this->packet_id);
            $query->condition('r.article_id', $article->article_id);
            $query->orderBy('u.name');
            $query->distinct();
            $results = $query->execute();
            $reviewers = array();
            foreach ($results as $result) {
                $name = htmlspecialchars($result->name);
                $dispositions = db_select('ebms_review_disposition', 'd')
                    ->fields('d', array('value_id'))
                    ->condition('d.review_id', $result->review_id)
                    ->execute();
                $png = 'EBMS_Pass_Icon_Active.png';
                $thumbs_down = false;
                foreach ($dispositions as $disposition)
                    if ($disposition->value_id == $warrants_no_changes)
                        $png = 'EBMS_Selection_X_Icon.png';
                $image_url = Ebms\IMAGES_DIR . '/' . $png;
                $img = '<img src="' . $image_url . '" alt="" />';
                $reviewers[] = '<div class="reviewer">' .
                    '<span class="reviewer-name">' . $name . '</span> ' .
                    $img .
                    #'<span class="review-disposition">' . $image . '</span>' .
                    '</div>';
            }
            $article_opts = $opts;
            $action = $article->archived ? 'revive' : 'archive';
            $article_opts['query'][$action] = $article->article_id;
            $href = url($url, $article_opts);
            $type = 'type="checkbox"';
            $onclick = 'onclick=\'location.href="' . $href . '"\'';
            $checked = $article->archived ? ' checked="checked"' : '';
            $checkbox = "<input $type $onclick$checked />";
            $links = '<div class="links">' .
                l('View Abstract', 'stub') . ' | ' .
                l('Download Full Text', 'stub') . '</div>';
            $data = array(
                $citation . $links,
                implode('<br />', $reviewers),
                date("m/d/Y", strtotime($article->updated)),
                array(
                    'data' => $checkbox,
                    'class' => array('col-4'),
                ),
            );
            $rows[] = array(
                'data' => $data,
                'no_striping' => true,
            );
        }
        return array(
            'title' => array(
                '#type' => 'markup',
                '#markup' => '<h2>' . htmlspecialchars($this->packet_title) .
                '</h2>',
            ),
            'report button' => array(
                '#type' => 'markup',
                '#markup' => '<a class="button" href="stub">Create Report</a>',
                '#prefix' => '<div id="report-button">',
                '#suffix' => '</div>',
            ),
            'top pager' => $pager,
            'table' => array(
                '#attributes' => array('id' => 'manager-packet-table'),
                '#theme' => 'table',
                '#rows' => $rows,
                '#header' => $headers,
            ),
            'bottom pager' => $pager,
        );
    }

    private function show_reviewed_article($opts) {
        #$results = db_select();
        drupal_set_breadcrumb(
            array(
                l('Literature', EbmsReview::$url),
                l('Reviewed Packets', EbmsReview::$url . '/reviews'),
                l($this->packet_title, EbmsReview::$url . '/reviews/' .
                    $this->packet_id),
            )
        );
        $reviews = array();
        $query = db_select('ebms_article_review', 'r');
        $query->join('users', 'u', 'u.uid = r.reviewer_id');
        $query->fields('r', array('review_id', 'when_posted', 'comments',
                'loe_info'));
        $query->fields('u', array('name'));
        $query->condition('r.packet_id', $this->packet_id);
        $query->condition('r.article_id', $this->article_id);
        $query->orderBy('r.when_posted', 'DESC');
        $results = $query->execute();
        foreach ($results as $result) {
            $reviewed = date("m/d/Y | g:i A", strtotime($result->when_posted));
            $markup = array('<div class="review">');
            #$markup[] = '<p class="align-right">' . $reviewed . '</p>';
            $markup[] = '<h3>Reviewer<span class="float-right">' .
                $reviewed . '</span></h3>';
            $markup[] = '<p>' . htmlspecialchars($result->name) . '</p>';
            $markup[] = '<h3>Options</h3>';
            $query = db_select('ebms_review_disposition_value', 'v');
            $query->join('ebms_review_disposition', 'd',
                'd.value_id = v.value_id');
            $query->fields('v', array('value_name'));
            $query->orderBy('v.value_pos');
            $query->distinct();
            $query->condition('d.review_id', $result->review_id);
            $dispositions = $query->execute();
            foreach ($dispositions as $d)
                $markup[] = '<p>' . htmlspecialchars($d->value_name) . '</p>';
            $markup[] = '<h3>Comments</h3>';
            if ($result->comments) {
                $markup[] = '<p>' . htmlspecialchars($result->comments) .
                    '</p>';
            }
            $markup[] = '<h3>LOE Info</h3>';
            if ($result->loe_info) {
                $markup[] = '<p>' . htmlspecialchars($result->loe_info) .
                    '</p>';
            }
            $markup[] = '</div>';
            $reviews[] = array(
                '#type' => 'markup',
                '#markup' => implode('', $markup),
            );
        }
        return array(
            'title' => array(
                '#type' => 'markup',
                '#markup' => '<h2>' . $this->packet_title . '</h2>',
            ),
            'left' => array(
                '#type' => 'container',
                '#attributes' => array('id' => 'reviewed-citation-left'),
                'label' => array(
                    '#type' => 'markup',
                    '#markup' => '<h3>Full Citation</h3>',
                ),
                'citation' => array(
                    '#type' => 'markup',
                    '#markup' => $this->get_reviewed_citation(
                        $this->article_id,
                        false
                    ),
                    '#prefix' => '<div id="citation">',
                    '#suffix' => '</div>',
                ),
                'view abstract' => array(
                    '#type' => 'markup',
                    '#markup' => l('View Abstract', 'stub'),
                    '#prefix' => '<div id="view-abstract-link">',
                    '#suffix' => '</div>',
                ),
                'download full text' => array(
                    '#type' => 'markup',
                    '#markup' => l('Download Full Text', 'stub'),
                    '#prefix' => '<div id="download-full-text-link">',
                    '#suffix' => '</div>',
                ),
            ),
            'right' => array(
                '#type' => 'container',
                '#attributes' => array('id' => 'article-reviews-right'),
                'reviews' => $reviews,
                /*array(
                    array(
                        '#type' => 'markup',
                        '#markup' => '<h3>Reviewer</h3><p>Ilana Cass</p>' .
                        '<h3>Options</h3><p>Merits revision of the text' .
                        '</p><p>Discuss at next meeting' .
                        '</p><h3>Comments</h3><p>Quo usque tandem ' .
                        'abutere Catalina patientia nostra?  Quamdiu ' .
                        'etiam furor iste nos eludet?  Quem ad finem ' .
                        'efranata iactabit audacia?</p>',
                        '#prefix' => '<div class="review">',
                        '#suffix' => '</div>',
                    ),
                    array(
                        '#type' => 'markup',
                        '#markup' => '<h3>Reviewer</h3>',
                        '#prefix' => '<div class="review">',
                        '#suffix' => '</div>',
                    ),
                    array(
                        '#type' => 'markup',
                        '#markup' => '<h3>Reviewer</h3>',
                        '#prefix' => '<div class="review">',
                        '#suffix' => '</div>',
                    ),
                    array(
                        '#type' => 'markup',
                        '#markup' => '<h3>Reviewer</h3>',
                        '#prefix' => '<div class="review">',
                        '#suffix' => '</div>',
                    ),
                    array(
                        '#type' => 'markup',
                        '#markup' => '<h3>Reviewer</h3>',
                        '#prefix' => '<div class="review">',
                        '#suffix' => '</div>',
                    ),
                ),*/
            ),
        );
    }

    /**
     * Pick the page for the board member/reviewer.
     */
    private function choose_member_review_page() {

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
                l('Literature', EbmsReview::$url),
                l($this->packet_title, EbmsReview::$url, $options),
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
        $url = EbmsReview::$url;
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
        drupal_set_breadcrumb(
            array(
                l('Literature', EbmsReview::$url),
                $title,
            )
        );

        // Build the page content.
        $page = array(
            '#prefix' => '<div id="member-packet">',
            '#suffix' => '</div>',
            'packet-title' => array(
                '#type' => 'markup',
                '#markup' => "<h2>{$this->packet_title}</h2>",
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
        if (strpos($this->expanded, 's') === false) {
            $options['query']['expanded'] = $this->expanded . 's';
            $link = l('Summary Documents &nbsp; +', EbmsReview::$url,
                    $options);
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
            $link = l($label, EbmsReview::$url, $options);
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
        if (strpos($this->expanded, 'a') === false) {
            $options['query']['expanded'] = $this->expanded . 'a';
            $link = l('Articles &nbsp; +', EbmsReview::$url, $options);
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
            $collapse_link = l($label, EbmsReview::$url, $options);

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
                $options['query']['article'] = $article->article_id;
                $rows[] = array(
                    'data' => array(
                        array(
                            'data' => "$citation<br />$links",
                            'class' => array('c1'),
                        ),
                        array(
                            'data' => l('Review', EbmsReview::$url, $options),
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
        if (strpos($this->expanded, 'u') === false) {
            $options['query']['expanded'] = $this->expanded . 'u';
            $link = l('Reviewer Uploads &nbsp; +', EbmsReview::$url, $options);
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
            $collapse_link = l($label, EbmsReview::$url, $options);

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
                $posted = date("m/d/Y", strtotime($doc->when_posted));
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
            $page['uploads'] = array(
                '#prefix' => '<div id="reviewer-uploads">',
                '#suffix' => '</div>',
                'heading' => array(
                    '#type' => 'markup',
                    '#markup' => "<h3 class='expanded'>$collapse_link</h3>",
                ),
                'post link' => array(
                    '#type' => 'markup',
                    '#markup' => l('post document',
                               EbmsReview::$url . '/reviewer-upload',
                               $options),
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
                l('Literature', EbmsReview::$url),
                l($this->packet_title, EbmsReview::$url, $options),
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
     * has uploaded.  We're not using the Drupal "header" technique
     * because we are not able to take advantage of their styling, and
     * their mechanism for sorting columns makes the URLs too ugly.
     */
    private function reviewer_docs_column_headers() {
        $sort_cols = array(
            'f' => 'FILE NAME',
            'u' => 'UPLOADED BY',
            'p' => 'WHEN POSTED'
        );
        $options = array('query' => $this->query_parms());
        $i = 1;
        $headers = array();
        foreach ($sort_cols as $key => $label) {
            if ($this->sort_col == $key && $this->sort_dir == 'a')
                $options['query']['sort-dir'] = 'd';
            else
                $options['query']['sort-dir'] = 'a';
            $options['query']['sort-col'] = $key;
            $headers[] = array(
                'data' => l($label, EbmsReview::$url, $options),
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
                    'r.packet_id = p.packet_id',
                )
            )
        );
        $query->condition('m.reviewer_id', $this->user->uid);
        $query->condition('p.active_status', 'A');
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

    /**
     * XXX Replace with Alan's version when API is ready.
     */
    private static function brief_citation($article_id) {
        return db_select('ebms_article', 'a')
            ->fields('a', array('brf_citation'))
            ->condition('a.article_id', $article_id)
            ->execute()
            ->fetchField();
    }

    /**
     * Get the topics handled by a particular board, and which have
     * articles which could be assigned to a review packet.  Can't
     * use Drupal's DB API, because it isn't sophisticated enough
     * to handle nested correlated queries.
     */
    public static function topics_for_board($board_id, $state_id) {
        $query = "
SELECT DISTINCT t.topic_id, t.topic_name
           FROM ebms_topic t
           JOIN ebms_article_state s
             ON t.topic_id = s.topic_id
          WHERE s.active_status = 'A'
            AND s.state_id = $state_id
            AND t.board_id = $board_id
            AND s.article_id NOT IN
   (SELECT DISTINCT article_id
               FROM ebms_packet_article
              WHERE drop_flag = 0
                AND packet_id IN
       (SELECT packet_id
          FROM ebms_packet
         WHERE topic_id = s.topic_id
           AND active_status = 'A'))";
        pdq_ebms_debug('TOPICS FOR BOARD QUERY', $query);
        $results = db_query($query);
        $topics = array();
        foreach ($results as $result)
            $topics[$result->topic_id] = $result->topic_name;
        return $topics;
    }

    /**
     * Find the articles which have passed full text review for the
     * selected topic and which have not already been assigned to
     * a packet.
     */
    public static function articles_for_topic(
        $topic_id, $state_id, $packet_id=0
    ) {
        if (!$topic_id)
            return array();
        if (false)
            pdq_ebms_debug('ARTICLES FOR TOPIC', array($topic, $state_id));
        $subquery = db_select('ebms_packet', 'p')
            ->fields('p', array('packet_id'))
            ->condition('p.active_status', 'A')
            ->condition('p.topic_id', $topic_id);
        $subquery = db_select('ebms_packet_article', 'a')
            ->fields('a', array('article_id'))
            ->condition('a.packet_id', $subquery, 'IN')
            ->condition('a.drop_flag', 0)
            ->distinct();
        $query = db_select('ebms_article_state', 's')
            ->fields('s', array('article_id'))
            ->condition('s.topic_id', $topic_id)
            ->condition('s.active_status', 'A')
            ->condition('s.state_id', $state_id)
            ->condition('s.article_id', $subquery, 'NOT IN')
            ->distinct();
        if ($packet_id) {
            $union = db_select('ebms_packet_article', 'pa')
                ->fields('pa', array('article_id'))
                ->condition('pa.packet_id', $packet_id)
                ->condition('pa.drop_flag', 0);
            $query->union($union);
        }
        if (false)
            dpq($query);
        if (false)
            pdq_ebms_debug(
                'ARTICLE QUERY', array(
                    'query' => $query->__toString(),
                    'topic' => $topic_id,
                    'passed full text review status' => $state_id,
                )
            );
        $results = $query->execute();
        $articles = array();
        foreach ($results as $result) {
            $id = $result->article_id;
            $articles[$id] = EbmsReview::brief_citation($id);
        }
        natsort($articles);
        return $articles;
    }

    /**
     * List all of the uploaded documents which have been tagged as
     * summary documents and associated with the packet's topic.
     * Include documents already linked to the packet, even if
     * the settings for those documents have been modified in such
     * a way that they would not otherwise be picked up by the
     * rest of the query's logic.
     */
    public static function summaries_for_topic($topic_id, $packet_id=0) {
        $query = db_select('ebms_doc', 'd');
        $query->fields('d', array('doc_id', 'description'));
        $query->join('ebms_doc_topic', 't', 't.doc_id = d.doc_id');
        $query->join('ebms_doc_tag', 'dt', 'dt.doc_id = d.doc_id');
        $query->join('ebms_tag', 'tg', 'tg.tag_id = dt.tag_id');
        $query->condition('tg.tag_name', 'summary');
        $query->condition('t.topic_id', $topic_id);
        if ($packet_id) {
            $union = db_select('ebms_doc', 'd2');
            $union->fields('d2', array('doc_id', 'description'));
            $union->join('ebms_packet_summary', 's', 's.doc_id = d2.doc_id');
            $union->condition('s.packet_id', $packet_id);
            $query->union($union);
        }
        $results = $query->execute();
        $docs = array();
        foreach ($results as $result)
            $docs[$result->doc_id] = htmlspecialchars($result->description);
        natsort($docs);
        return $docs;
    }

    /**
     * Build the hash of board members who can be assigned to the
     * packet being created or modified.  The user can toggle between
     * displaying all members of the packet's board, or only those
     * board members who are associated with the packet's specific
     * topic.  Note that some topics have no default reviewers, so
     * when creating a packet for such topics the board manager
     * will always have to ask for the complete board.  We don't
     * display all the board members by default even in such as
     * case, because we don't want to give the impression that all
     * the board members have an association with the selected
     * topic.  If we're editing an existing packet, we include
     * reviewers who are currently assigned to the packet, even
     * if they wouldn't be picked up by the rest of the query's
     * logic.
     */
    public static function reviewers_for_topic(
        $topic_id, $board_id=0, $packet_id=0
    ) {
        if (false)
            pdq_ebms_debug("REVIEWERS FOR TOPIC", array(
                               "TOPIC ID" => $topic_id,
                               "BOARD ID" => $board_id,
                               "PACKET ID" => $packet_id));
        if (!$topic_id && !$board_id)
            return array();
        $query = db_select('users', 'u');
        $query->fields('u', array('uid', 'name'));
        if ($board_id) {
            $query->join('ebms_board_member', 'm', 'm.user_id = u.uid');
            $query->join('users_roles', 'ur', 'ur.uid = u.uid');
            $query->join('role', 'r', 'r.rid = ur.rid');
            $query->condition('m.board_id', $board_id);
            $query->condition('r.name', 'board member');
        }
        else {
            $query->join('ebms_topic_reviewer', 'r', 'r.user_id = u.uid');
            $query->condition('r.topic_id', $topic_id);
        }
        $query->condition('u.status', 1);
        if ($packet_id) {
            $union = db_select('users', 'u2');
            $union->fields('u2', array('uid', 'name'));
            $union->join('ebms_packet_reviewer', 'pr',
                'pr.reviewer_id = u2.uid');
            $union->condition('pr.packet_id', $packet_id);
            $query->union($union);
        }
        if (false)
            pdq_ebms_debug("DPQ", dpq($query, true));
        $results = $query->execute();
        $reviewers = array();
        foreach ($results as $result)
            $reviewers[$result->uid] = $result->name;
        natsort($reviewers);
        if (false)
            pdq_ebms_debug("REVIEWERS", $reviewers);
        return $reviewers;
    }
}

/**
 * EBMS packet called up for editing.
 */
class EbmsPacket {
    public
        $id,
        $title,
        $reviewers,
        $articles,
        $summaries;
    public function __construct($packet_id) {
        $this->id = $packet_id;
        $this->reviewers = EbmsPacket::fetch_reviewers($packet_id);
        $this->summaries = EbmsPacket::fetch_summaries($packet_id);
        $this->articles = EbmsPacket::fetch_articles($packet_id);
        $query = db_select('ebms_packet', 'p');
        $query->join('ebms_topic', 't', 't.topic_id = p.topic_id');
        $query->fields('p', array('packet_title', 'topic_id'));
        $query->fields('t', array('board_id'));
        $query->condition('p.packet_id', $packet_id);
        $results = $query->execute();
        foreach ($results as $result) {
            $this->title = $result->packet_title;
            $this->topic_id = $result->topic_id;
            $this->board_id = $result->board_id;
        }
    }
    /**
     * Retrieve the title for the current packet from the database.
     */
    public static function fetch_title($packet_id) {
        if (!$packet_id)
            return null;
        return db_select('ebms_packet', 'p')
            ->fields('p', array('packet_title'))
            ->condition('p.packet_id', $packet_id)
            ->execute()
            ->fetchField();
    }
    public static function fetch_reviewers($packet_id) {
        $results = db_select('ebms_packet_reviewer', 'r')
            ->fields('r', array('reviewer_id'))
            ->condition('r.packet_id', $packet_id)
            ->execute();
        foreach ($results as $result)
            $reviewers[] = $result->reviewer_id;
        return $reviewers;
    }
    public static function fetch_articles($packet_id) {
        $results = db_select('ebms_packet_article', 'a')
            ->fields('a', array('article_id'))
            ->condition('a.packet_id', $packet_id)
            ->condition('a.drop_flag', 0)
            ->execute();
        foreach ($results as $result)
            $articles[] = $result->article_id;
        return $articles;
    }
    public static function fetch_summaries($packet_id) {
        $results = db_select('ebms_packet_summary', 's')
            ->fields('s', array('doc_id'))
            ->condition('s.packet_id', $packet_id)
            ->execute();
        $summaries = array();
        foreach ($results as $result)
            $summaries[] = $result->doc_id;
        return $summaries;
    }
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
    $cite = Ebms\Util::get_full_citation($article_id);
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
    $sort_col = $form_state['values']['sortcol'];
    $sort_dir = $form_state['values']['sortdir'];
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
                        'value_id' => $id,
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
                        'value_id' => $id,
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
            'sort-dir' => $sort_dir,
        ),
    );
    drupal_goto(EbmsReview::$url, $options);
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
        $doc_id = db_insert('ebms_reviewer_doc')
            ->fields(
                array(
                    'file_id' => $file->fid,
                    'reviewer_id' => $user->uid,
                    'packet_id' => $packet_id,
                    'when_posted' => $now,
                    'doc_title' => $file->filename,
                    'description' => $filenotes,
                )
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
            'sort-dir' => $sort_dir,
        ),
    );
    drupal_goto(EbmsReview::$url, $options);
}

/**
 * Create the form used by a board manager to create a new literature
 * review packet.  We show the parts of the form incrementally, as
 * lower portions depend on the selections made in the upper portions.
 */
function pdq_ebms_edit_packet_form($form, &$form_state, $packet_id=0) {
    if (false)
        pdq_ebms_debug('EDIT PACKET FORM', array('FORM' => $form,
                'FORM STATE' => $form_state));

    // We'll use this more than once, so fetch and save it now.
    $state_id = Ebms\Util::get_state_id('Passed full text review');

    // If we're editing an existing packet, we already know board and topic.
    if ($packet_id) {
        $packet = new EbmsPacket($packet_id);
        $board_id = $packet->board_id;
        $topic_id = $packet->topic_id;
        $packet_header = 'Edit Packet - ' . htmlspecialchars($packet->title);
    }
    else {

        // See if the board and topic have already been selected.
        $board_id = $topic_id = 0;
        if (!empty($form_state['values']['board']))
            $board_id = $form_state['values']['board'];
        if (!empty($form_state['values']['topic']))
            $topic_id = $form_state['values']['topic'];
        $packet_header = 'Add New Literature Surveillance Packet';

        // Make sure we don't have a stale topic ID (i.e., board changed).
        $topics = EbmsReview::topics_for_board($board_id, $state_id);
        if (!array_key_exists($topic_id, $topics))
            $topic_id = null;

        // Collapse topic field if no topics available; otherwise expand.
        if (count($topics)) {
            $topic_title = 'Topic';
            $topic_desc = 'You may select only one topic.';
        }
        else {
            $topic_title =
                'None of this board\'s topics have articles ready for ' .
                'packets';
            $topic_desc = null;
        }
    }

    // Override the Drupal breadcrumb mechanism to match requirements.
    drupal_set_breadcrumb(
        array(
            l('Literature', EbmsReview::$url),
            $packet_header,
        )
    );

    // Check the state of the toggle for showing all board members.
    $all_reviewers = false;
    if (!empty($form_state['values']['all-reviewers']))
        $all_reviewers = $form_state['values']['all-reviewers'];
    if ($all_reviewers)
        $all_reviewers_title = 'Show Only Topic Reviewers &ndash;';
    else
        $all_reviewers_title = 'Show All Board Members +';

    // Initialize default arrays (replaced below as appropriate).
    $article_defaults = $summary_defaults = $reviewer_defaults = array();

    // Set up the fields which depend on having a topic selected.
    if ($topic_id) {
        $articles_title = 'Articles';
        $summaries_title = 'Summary Documents (Optional)';
        $reviewers_title = 'Reviewers';
        $all_reviewers_class = 'all-reviewers-visible';
        $articles = EbmsReview::articles_for_topic($topic_id, $state_id,
                    $packet_id);
        $summaries = EbmsReview::summaries_for_topic($topic_id, $packet_id);
        $reviewers = EbmsReview::reviewers_for_topic($topic_id,
                     $all_reviewers ? $board_id : null, $packet_id);
        $articles_desc =
            'Selected articles will be included in the packet.  ' .
            'You may unselect any articles that you do not want ' .
            'to be included.';
        $summaries_desc = 'You may select more than one summary document.';
        $reviewers_desc =
            'Selected reviewers will receive this packet to review.  ' .
            'You may unselect these reviewers and/or ' .
            'select additional Board members to review this packet.';

        // Are we editing an existing packet?
        if ($packet_id) {
            $article_defaults = $packet->articles;
            $reviewer_defaults = $packet->reviewers;
            $summary_defaults = $packet->summaries;
            $articles_desc =
                'You may select any articles that you want to add to this ' .
                'packet or unselect any articles that you want to remove ' .
                'from this packet.';
            $reviewers_desc =
                'You may select any reviewers that you want to add to this ' .
                'packet or unselect any reviewers that you want to remove ' .
                'from this packet.';
        }
        else
            $article_defaults = array_keys($articles);
    }
    else {

        // Don't have a topic yet; collapse dependent fields.
        $all_reviewers_class = 'all-reviewers-hidden';
        $articles_title = 'Articles';
        $summaries_title = 'Summary Documents (Optional)';
        $reviewers_title = 'Reviewers';
        $articles_desc = $summaries_desc = $reviewers_desc = '';
        $articles = $summaries = $reviewers = $article_defaults =
            $summary_defaults = $reviewer_defaults = array();
        $all_reviewers = false;
    }

    // Assemble the common portions of the form (both editing and creation).
    $form_id = 'pdq-ebms-edit-packet-form';
    $heading_field = array(
        '#type' => 'markup',
        '#markup' => "<h2>$packet_header</h2>",
    );
    $packet_id_field = array(
        '#type' => 'hidden',
        '#value' => $packet_id,
    );
    $articles_field = array(
        '#type' => 'checkboxes',
        '#title' => $articles_title,
        '#description' => $articles_desc,
        '#options' => $articles,
        '#default_value' => $article_defaults,
    );
    $summaries_field = array(
        '#type' => 'checkboxes',
        '#title' => $summaries_title,
        '#description' => $summaries_desc,
        '#options' => $summaries,
        '#default_value' => $summary_defaults,
    );
    $reviewers_div = array(
        '#prefix' => '<div id="reviewers">',
        '#suffix' => '</div>',
        'reviewers' => array(
            '#type' => 'checkboxes',
            '#title' => $reviewers_title,
            '#description' => $reviewers_desc,
            '#options' => $reviewers,
            '#default_value' => $reviewer_defaults,
        ),
        'all-reviewers' => array(
            '#prefix' => "<div class='$all_reviewers_class'>",
            '#suffix' => '</div>',
            '#type' => 'checkbox',
            '#title' => $all_reviewers_title,
            '#ajax' => array(
                'callback' => 'pdq_ebms_packet_reviewers_ajax',
                'wrapper' => 'reviewers',
            ),
        ),
    );
    $submit_field = array(
        '#type' => 'submit',
        '#value' => $packet_id ? 'Save Changes' : 'Upload Packet',
    );

    // Put up the form for an existing packet.
    if ($packet_id)
        return array(
            '#id' => $form_id,
            'heading' => $heading_field,
            'packet' => $packet_id_field,
            'board' => array(
                '#type' => 'hidden',
                '#value' => $packet->board_id,
            ),
            'board-specific-fields' => array(
                'topic' => array(
                    '#type' => 'hidden',
                    '#value' => $packet->topic_id,
                ),
                'topic-specific-fields' => array(
                    'articles' => $articles_field,
                    'summaries' => $summaries_field,
                    'reviewers-div' => $reviewers_div,
                ),
            ),
            'submit' => $submit_field,
        );

    // Build the render array for the form for a new packet.  Note that
    // there is a Drupal bug which causes our default checking of the
    // articles to be ignored (http://drupal.org/node/1100170).
    $bsf = 'board-specific-fields';
    $tsf = 'topic-specific-fields';
    $form = array(
        '#id' => 'pdq-ebms-edit-packet-form',
        'packet' => $packet_id_field,
        'board' => array(
            '#type' => 'radios',
            '#title' => 'Board',
            '#description' => 'You may select only one board.',
            '#options' => Ebms\Util::boards(),
            '#ajax' => array(
                'callback' => 'pdq_ebms_packet_board_ajax',
                'wrapper' => $bsf,
            ),
        ),
        $bsf => array(
            '#prefix' => "<div id='$bsf'>",
            '#suffix' => '</div>',
        ),
    );

    if ($board_id) {
        $form[$bsf]['topic'] = array(
            '#type' => 'radios',
            '#title' => $topic_title,
            '#description' => $topic_desc,
            '#options' => $topics,
            '#ajax' => array(
                'callback' => 'pdq_ebms_packet_topic_ajax',
                'wrapper' => $tsf,
            ),
        );
        $form[$bsf][$tsf] = array(
            '#prefix' => "<div id='$tsf'>",
            '#suffix' => '</div>',
        );
        if ($topic_id) {
            $form[$bsf][$tsf]['articles'] = $articles_field;
            $form[$bsf][$tsf]['summaries'] = $summaries_field;
            $form[$bsf][$tsf]['reviewers-div'] = $reviewers_div;
            $form[$bsf][$tsf]['name'] = array(
                '#type' => 'textfield',
                '#title' => 'Packet Name',
            );
            $form[$bsf][$tsf]['submit'] = $submit_field;
        }
    }
    return $form;
}

/**
 * Verify that the required information is present for a new or
 * modified packet.  We do this here instead of by using the
 * 'required' property because the users don't like the red asterisks
 * which Drupal injects into the form's markup.
 */
function pdq_ebms_edit_packet_form_validate($form, &$form_state) {
    $packet_id = $form_state['values']['packet'];
    if (!$packet_id) {
        $name = trim($form_state['values']['name']);
        if (!$form_state['values']['board'])
            form_set_error('board', 'Board selection is required.');
        if (!$form_state['values']['topic'])
            form_set_error('topic', 'Topic selection is required.');
        if (!$name)
            form_set_error('name', 'Packet must be given a name.');
    }
    $article_count = $reviewer_count = 0;
    $articles = $form_state['values']['articles'];
    foreach ($articles as $id => $val)
        if ($val)
            $article_count++;
    $reviewers = $form_state['values']['reviewers'];
    foreach ($reviewers as $id => $val)
        if ($val)
            $reviewer_count++;
    if (!$article_count)
        form_set_error('articles', 'At least one article must be selected.');
    if (!$reviewer_count)
        form_set_error('reviewers', 'At least one reviewer must be selected.');
        $form_state['redirect'] = array(
            EbmsReview::$url . '/packets',
            'query' => drupal_get_query_parameters(),
        );
}

/**
 * Store the new or modified packet, along with the articles assigned
 * for review, the board members assigned to perform the review, and
 * the summary documents attached to the packet.
 */
function pdq_ebms_edit_packet_form_submit($form, &$form_state) {

    // Collect all of the form's values.
    $packet_id = $form_state['values']['packet'];
    $articles = array();
    $reviewers = array();
    $summaries = array();
    foreach ($form_state['values']['articles'] as $id => $value)
        if ($value)
            $articles[$value] = $value;
    foreach ($form_state['values']['reviewers'] as $id => $value)
        if ($value)
            $reviewers[$value] = $value;
    foreach ($form_state['values']['summaries'] as $id => $value)
        if ($value)
            $summaries[$value] = $value;

    // If this is a new packet, create a row (and packet ID) for it.
    if (!$packet_id) {
        global $user;
        $articles_to_add = $articles;
        $topic_id = $form_state['values']['topic'];
        $name = $form_state['values']['name'];
        $now = date('Y-m-d H:i:s');
        $packet_id = db_insert('ebms_packet')
            ->fields(
                array(
                    'topic_id' => $topic_id,
                    'created_by' => $user->uid,
                    'created_at' => $now,
                    'packet_title' => $name,
                )
            )
            ->execute();
    }
    else {

        // Which articles are in the packet already?
        $rows = db_select('ebms_packet_article', 'a')
            ->fields('a', array('article_id', 'drop_flag'))
            ->condition('a.packet_id', $packet_id)
            ->execute();
        $in_packet = array();
        foreach ($rows as $row)
            $in_packet[$row->article_id] = $row->drop_flag ? true : false;

        // Add the articles the manager wants in the packet now.
        $articles_to_add = array();
        foreach ($articles as $article_id) {
            if (array_key_exists($article_id, $in_packet)) {

                // If the article was there, then suppressed, unsuppress it.
                if ($in_packet[$article_id])
                    db_update('ebms_packet_article')
                        ->fields(array('drop_flag' => 0))
                        ->condition('article_id', $article_id)
                        ->execute();
            }
            else

                // Wasn't ever in the packet; we'll add it below.
                $articles_to_add[] = $article_id;
        }

        // Mark articles the manager no longer wants in the packet.
        // Doing it this way avoids trouble with articles that have
        // already been reviewed.
        foreach ($in_packet as $article_id => $dropped) {
            if (!$dropped)
                if (!array_key_exists($article_id, $articles))
                    db_update('ebms_packet_article')
                        ->fields(array('drop_flag' => 1))
                        ->condition('article_id', $article_id)
                        ->execute();
        }

        // We'll rebuild the list of reviewers; doesn't affect existing
        // reviews of articles in the packet.
        db_delete('ebms_packet_reviewer')
            ->condition('packet_id', $packet_id)
            ->execute();

        // Same approach for summary documents.
        db_delete('ebms_packet_summary')
            ->condition('packet_id', $packet_id)
            ->execute();
    }

    // Add the articles which aren't in the table already.
    foreach ($articles_to_add as $article_id) {
        db_insert('ebms_packet_article')
            ->fields(
                array(
                    'packet_id' => $packet_id,
                    'article_id' => $article_id,
                )
            )
            ->execute();
    }

    // ... and the reviewers:
    foreach ($reviewers as $reviewer_id) {
        db_insert('ebms_packet_reviewer')
            ->fields(
                array(
                    'packet_id' => $packet_id,
                    'reviewer_id' => $reviewer_id,
                )
            )
            ->execute();
    }

    // ... and the summaries:
    foreach ($summaries as $doc_id) {
        db_insert('ebms_packet_summary')
            ->fields(
                array(
                    'packet_id' => $packet_id,
                    'doc_id' => $doc_id,
                )
            )
            ->execute();
    }
}

/**
 * Get the updated slice of the packet creation form which changes
 * depending on which board is selected.
 */
function pdq_ebms_packet_board_ajax($form, $form_state) {
    return $form['board-specific-fields'];
}

/**
 * Get the updated slice of the packet creation form which changes
 * whenever a different topic is selected.  We have to manipulate
 * the data structure to get the articles selected by default, in
 * order to work around a Drupal bug which prevents this from
 * happening automatically based on our default values above (see
 * http://drupal.org/node/1100170).
 */
function pdq_ebms_packet_topic_ajax($form, $form_state) {
    $subform = $form['board-specific-fields']['topic-specific-fields'];
    foreach ($subform['articles'] as $k => $v) {
        if (isset($v['#type']) and $v['#type'] == 'checkbox') {
            $subform['articles'][$k]['#checked'] = 'checked';
        }
    }
    return $subform;
}

/**
 * Get the updated slice of the packet editing form which shows the
 * board members which can be assigned to review the articles in this
 * packet.  This toggles back and forth betweeh the members of the
 * board who are directly associated with the packet's topic and the
 * complete set of all members of the packet's board.
 */
function pdq_ebms_packet_reviewers_ajax($form, $form_state) {
    return $form['board-specific-fields']['topic-specific-fields']
        ['reviewers-div'];
}
function pdq_ebms_board_filter_form($form, &$form_state, $opts) {
    $checked_boards = array();
    if (isset($opts['query']['boards']))
        $checked_boards = explode(',', $opts['query']['boards']);
    $items = array();
    $boards = Ebms\Util::boards();
        foreach ($boards as $id => $name) {
            $href = '';
            $type = 'type="checkbox"';
            $onclick = 'onclick=\'location.href="' . $href . '"\'';
            $onclick = '';
            $checked = '';
            if (in_array($id, $checked_boards))
                $checked = ' checked="checked"';
            $checkbox = "<input $type $onclick$checked />";
            $items[] = "$checkbox <span class='board-name'>$name</span>";
        }
    $boards = Ebms\Util::boards();
    return array(
        'boards' => array(
            '#type' => 'checkboxes',
            '#title' => 'Filter by Board',
            '#options' => $boards,
        ),
    );
}
