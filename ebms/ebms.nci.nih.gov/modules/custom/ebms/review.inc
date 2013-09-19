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
        $pager,
        $on_behalf;
    public static $url = 'review';

    /**
     * Passing settings in the URL instead of in $_SESSION in order
     * to avoid dependency on Javascript being enabled on the user's
     * browser.
     */
    public function __construct($action, $packet_id, $article_id) {
        $this->packet_id    = Ebms\Util::get_request_value('packet',
                              $packet_id);
        $this->article_id   = Ebms\Util::get_request_value('article',
                              $article_id);
        $this->expanded     = Ebms\Util::get_request_value('expanded');
        $this->sort_col     = Ebms\Util::get_request_value('sort-col', 'f');
        $this->sort_dir     = Ebms\Util::get_request_value('sort-dir', 'a');
        $this->action       = $action;
        $this->reviewed     = Ebms\Util::get_request_value('reviewed');
        $this->pager        = Ebms\Util::get_request_value('pager');
        $this->on_behalf    = Ebms\Util::get_request_value('obo');
        $this->packet_title = EbmsPacket::fetch_title($this->packet_id);

        // determine user
        global $user;
        $this->user = $user;

        // if viewing page on behalf of another, load that account instead
        if ($this->on_behalf) {
            $account = user_load($this->on_behalf);
            if ($account !== false) {
                // use the account if it loaded
                $this->user = $account;
            } else {
                // otherwise, clear out on_behalf as well
                $this->on_behalf = '';
            }
        }
    }

    /**
     * Select the version of the page appropriate the user's role.
     */
    public function run() {
        Ebms\Util::add_js();
        drupal_add_css(Ebms\CSS_DIR . '/review.css');
        Ebms\Menu::$active = 'Literature';

        if (!$this->check_permissions())
            return MENU_ACCESS_DENIED;

        if (in_array('board member', $this->user->roles)) {
            if ($this->action == 'reviewer-upload') {
                return $this->reviewer_upload_page();
            }
            return $this->choose_member_review_page();
        }
        return $this->choose_manager_review_page();
    }

    /**
     * Pick the page for the board manager.
     */
    private function choose_manager_review_page() {
        switch ($this->action) {
            case 'add':
                return drupal_get_form('pdq_ebms_edit_packet_form');
            case 'edit':
                return drupal_get_form('pdq_ebms_edit_packet_form',
                        $this->packet_id);
            case 'delete':
                return $this->delete_packet();
            case 'record':
                return $this->record_responses();
            case 'packets':
                return $this->manager_packets();
            case 'reviews':
            default:
                return $this->manager_reviews();
        }
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
     * Shows the form to select a user and optionally a packet in order to enter
     * responses on behalf of a board member.
     *
     * @return array a Drupal render array containing the form and left nav.
     */
    private function record_responses() {
        // check permissions
        if (!user_access('record print responses')) {
            return MENU_ACCESS_DENIED;
        }

        // Override Drupal's default breadcrumbs.  TIR 2335 from Ashleigh
        // (marked Critical, no less).
        drupal_set_breadcrumb(
            array(
                l('Literature', EbmsReview::$url . '/reviews'),
                'Record Responses',
            )
        );

        return array(
            'left-nav' => $this->build_left_nav('Record Responses'),
            'right-panel' => array(
                'form' => drupal_get_form('pdq_ebms_record_responses_form'),
                '#prefix' => "<div id='main-right-panel'><h2>Record Responses</h2>",
                '#suffix' => "</div>",
            ),
        );
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
                l('Literature', EbmsReview::$url . '/reviews'),
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
                'data' => 'Created',
                'class' => array('col-5'),
                'field' => 'p.created_at',
                'sort' => 'desc',
            ),
            array(
                'data' => 'Actions',
                'class' => array('col-4'),
            ),
        );

        // See if the user has identified one or more boards.
        $opts = array('query' => drupal_get_query_parameters());
        $boards = array();
        if (isset($opts['query']['boards']))
            $boards = explode(',', $opts['query']['boards']);
        else
            $boards = $this->user->boards;

        // Build the query.
        $query = db_select('ebms_packet', 'p');
        if ($this->pager != 'off')
            $query = $query->extend('PagerDefault')->limit(10);
        $query = $query->extend('TableSort');
        $query->join('ebms_topic', 't', 't.topic_id = p.topic_id');
        $query->join('ebms_board', 'b', 'b.board_id = t.board_id');
        $query->join('users', 'u', 'u.uid = p.created_by');
        if ($this->reviewed) {
            $query->join('ebms_article_review', 'r',
                'r.packet_id = p.packet_id');
            $query->distinct();
        }
        $query->fields('p', array('packet_id', 'packet_title', 'created_at'));
        $query->fields('b', array('board_name'));
        $query->fields('u', array('name'));
        if (count($boards) == 1)
            $query->condition('b.board_id', reset($boards));
        elseif (count($boards) > 1)
            $query->condition('b.board_id', $boards, 'IN');
        //$query->condition('p.created_by', $this->user->uid);
        $query->condition('p.active_status', 'A');
        $query->orderByHeader($headers);
        $packets = $query->execute();
        $divider = '<span class="action-divider">|</span>';
        $main_url = EbmsReview::$url;
        $del_func = 'javascript:ebmsscript.delete_packet';
        $rows = array();
        foreach ($packets as $p) {
            $edit_url = "$main_url/edit/{$p->packet_id}";
            $edit_link = l('EDIT', $edit_url, $opts);
            $del_parm = url("$main_url/delete/{$p->packet_id}", $opts);
            $del_url = "$del_func('$del_parm');";
            $del_link = '<a href="' . $del_url . '">DELETE</a>';
            $title = str_replace(' [Legacy]', '', $p->packet_title);
            $title .= " [Packet #{$p->packet_id}]";
            $title_link = l(htmlspecialchars($title), $edit_url, $opts);
            $data = array(
                $title_link,
                htmlspecialchars($p->board_name),
                htmlspecialchars($p->name),
                date('m/d/Y', strtotime($p->created_at)),
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
        $left_nav = $this->build_left_nav('View / Edit Packets');
        $url = EbmsReview::$url . '/packets';
        if (count($this->user->boards) > 1)
            $left_nav['board filter box'] =
                $this->board_filter_box($opts, $url);
        if ($this->pager != 'off')
            $table['pager'] = array('#theme' => 'pager');
        else {
            unset($opts['query']['pager']);
            $link = l('VIEW PAGES', $url, $opts);
            $table['pager'] = array(
                '#type' => 'markup',
                '#markup' => "<div id='pager-view-pages'>$link</div>",
            );
        }
        return array(
            'left-nav' => $left_nav,
            'packets' => array(
                '#type' => 'container',
                '#attributes' => array('id' => 'main-right-panel'),
                'report' => $table,
            ),
        );
    }

    /**
     * Creates the render array for the left nav panel for Citation
     * Management page.  Cloned from common.inc, where I had originally
     * placed this functionality, not realizing that the left nav block
     * was going to be different for every page.
     *
     *  @param string $active  Current action, highlighted by CSS
     *
     *  @return array          Render array for left nav panel
     */
    public static function build_left_nav($active=null) {
        $links = array(
            array('Reviewed Packets', 'review/reviews'),
            array('Create Packet', 'review/add'),
            array('View / Edit Packets', 'review/packets'),
        );

        // add record responses if permitted
        if(user_access('record print responses'))
            $links[] = array('Record Responses', 'review/record');

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
                l('Literature', EbmsReview::$url . '/reviews'),
                'Reviewed Packets',
            )
        );
        $left_nav = $this->build_left_nav('Reviewed Packets');
        $url = EbmsReview::$url . '/reviews';
        if (count($this->user->boards) > 1)
            $left_nav['board filter box'] =
                $this->board_filter_box($opts, $url);
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
    private function board_filter_box($opts, $url) {
        $checked_boards = array();
        if (isset($opts['query']['boards']))
            $checked_boards = explode(',', $opts['query']['boards']);
        $items = array();
        $boards = Ebms\Util::boards();
        $type = 'type="checkbox"';
        foreach ($boards as $id => $name) {
            if (!in_array($id, $this->user->boards))
                continue;
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
            $label = "<label for='$cb_id'>$name</label>";
            $item = "<div class='review-board'>$checkbox $label</div>";
            $items[] = $item;
        }
        $url = Ebms\IMAGES_DIR . '/left-nav-box-bottom.png';
        return array(
            '#type' => 'markup',
            '#markup' => '<div class="more-actions left-nav-box">' .
            '<div class="left-nav-box-title">Filter by Board</div>' .
            implode('', $items) . '<br class="float-clear" /></div>',
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

        // If not, find the boards of which she's a member.
        if (count($boards) < 1)
            $boards = $this->user->boards;

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
                'sort' => 'desc',
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
            $title .= " [Packet #{$p->packet_id}]";
            $query = db_select('users', 'u');
            $query->fields('u', array('name'));
            $query->join('ebms_article_review', 'r', 'r.reviewer_id = u.uid');
            $query->condition('r.packet_id', $p->packet_id);
            $query->distinct();
            $query->orderBy('u.name');
            $results = $query->execute();
            $reviewers = array();
            foreach ($results as $result)
                $reviewers[] = '<div>' . htmlspecialchars($result->name) .
                '</div>';
            $url = EbmsReview::$url . '/reviews/' . $p->packet_id;
            $link = l($title, $url);
            $data = array(
                $link,
                array(
                    'data' => implode('', $reviewers),
                    'class' => array('reviewers'),
                ),
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
        $button_class = array('class' => array('button'));
        $attrs = array('attributes' => $button_class);
        $table = array(
            '#type' => 'container',
            '#attributes' => array('id' => 'main-right-panel'),
            'title' => array(
                '#type' => 'markup',
                '#markup' => '<h2>Reviewed Packets</h2>',
            ),
            'report button' => array(
                '#type' => 'markup',
                '#markup' => l('Create Report', 'reports/lit-review', $attrs),
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
    private function get_reviewed_citation($article_id, $narrow=true) {
        $xml = db_select('ebms_article', 'a')
            ->fields('a', array('source_data'))
            ->condition('a.article_id', $article_id)
            ->execute()
            ->fetchField();
        $doc = new \SimpleXMLElement($xml);
        $title = $ta = $vol = $issue = $pgn = $year = $pmid = '';
        $article = $doc->MedlineCitation->Article;
        $title = htmlspecialchars($article->ArticleTitle);
        $title = l($title, 'citations/full/' . $article_id);
        $pgn = htmlspecialchars($article->Pagination->MedlinePgn);
        $year = $article->Journal->JournalIssue->PubDate->Year;
        $issue = htmlspecialchars($article->Journal->JournalIssue->Issue);
        $vol = htmlspecialchars($article->Journal->JournalIssue->Volume);
        $authors = array();
        foreach ($doc->MedlineCitation->Article->AuthorList->Author as $a) {
            if (sizeof($authors) == 3) {
                $authors[] = 'et al.';
                break;
            }
            $name = $a->LastName;
            if ($a->Initials)
                $name .= ' ' . $a->Initials;
            $authors[] = htmlspecialchars($name);
        }
        $ta = $doc->MedlineCitation->MedlineJournalInfo->MedlineTA;
        $ta = htmlspecialchars($ta);
        $pmid = $doc->MedlineCitation->PMID;
        $pieces = array(implode(', ', $authors), '<br /><em>', $title,
                  '</em><br />', $ta, ' ', $vol);
        if ($issue)
            $pieces[] = "($issue)";
        if ($pgn)
            $pieces[] = ": $pgn";
        if ($year)
            $pieces[] = ", $year";
        $pieces[] = ".<br />PMID: $pmid";
        $separator = $narrow ? '<br />' : ' &nbsp; ';
        $pieces[] = "{$separator}EBMS ID: $article_id";

        // See if the article was converted from the legacy system.
        $legacy_id = db_select('ebms_legacy_article_id', 'i')
            ->fields('i', array('legacy_id'))
            ->condition('i.article_id', $article_id)
            ->execute()
            ->fetchField();
        if ($legacy_id)
            $pieces[] = "{$separator}Legacy CMS ID: $legacy_id";
        return implode($pieces);
    }

    /**
     * The user (a board manager) has selected a specific packet from the
     * list of reviewed packets displayed on her Literature landing page.
     * Create a table showing each article in the packet, the reviewers
     * who have weighed in on the articles, and a visual indication of
     * whether they gave an article a thumbs up (green check icon) or
     * down (red X icon).  We also show that last update to the article
     * activity for the packet, as well as a checkbox allowing the board
     * manager to suppress ("archive") the article from the packet (or to
     * turn off that suppression.
     */
    private function show_reviewed_packet($opts) {

        // We'll have at least one use for the current date/time.
        $now = strftime("%Y-%m-%d %H:%M:%S");

        // If the user asked to suppress on of the articles in the
        // packet, update the table to reflect this request.
        if (isset($opts['query']['archive'])) {
            $article_id = $opts['query']['archive'];
            if ($article_id) {
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

        // Undo the suppression of an article in a packet.
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

        // Manual breadcrumb setting to match requirements.
        $url = EbmsReview::$url . '/reviews/' . $this->packet_id;
        drupal_set_breadcrumb(
            array(
                l('Literature', EbmsReview::$url . '/reviews'),
                l('Reviewed Packets', EbmsReview::$url . '/reviews'),
                $this->packet_title,
            )
        );

        // Handle some of the other page state settings.
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

        // Set up sorting via the table column headers.
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

        // Get the ID for the disposition for which we display a red X icon.
        $warrants_no_changes = db_select('ebms_review_disposition_value', 'v')
            ->fields('v', array('value_id'))
            ->condition('v.value_name', 'Warrants no changes to the summary')
            ->execute()
            ->fetchField();

        // Construct the query to get the articles in the packet.
        $query = db_select('ebms_packet_article', 'p');
        $query = $query->extend('TableSort');
        $query->join('ebms_article', 'a', 'a.article_id = p.article_id');
        $query->join('ebms_article_review', 'r',
            'r.article_id = a.article_id');
        $query->join('ebms_review_disposition', 'd',
            'd.review_id = r.review_id');
        $query->leftJoin('file_managed', 'f', 'f.fid = a.full_text_id');
        $query->fields('a', array('article_id', 'article_title', 'source_id'));
        $query->fields('p', array('archived'));
        $query->fields('f', array('uri'));
        $query->addExpression('MAX(r.when_posted)', 'updated');
        $query->condition('p.packet_id', $this->packet_id);
        if (!$sa)
            $query->isNull('p.archived');
        $query->groupBy('a.article_id');
        $query->orderByHeader($headers);
        $articles = $query->execute();

        // Get a list of all the reviewers assigned to the packet.
        $packet_reviewers = array();
        $query = db_select('users', 'u');
        $query->fields('u', array('name'));
        $query->join('ebms_packet_reviewer', 'r', 'r.reviewer_id = u.uid');
        $query->condition('r.packet_id', $this->packet_id);
        $query->orderBy('u.name');
        $results = $query->execute();
        foreach ($results as $result)
            $packet_reviewers[] = $result->name;

        // Build up the sequence of table rows.
        $rows = array();
        $pubmed = 'http://www.ncbi.nlm.nih.gov/pubmed/';
        $target = array('attributes' => array('target' => '_blank'));
        foreach ($articles as $article) {

            // Index for walking through the $packet_reviewers list.
            $i = 0;

            // Collect information about what each reviewer said about the
            // article.
            $article_id = $article->article_id;
            $citation = $this->get_reviewed_citation($article_id, false);
            $query = db_select('users', 'u');
            $query->fields('u', array('name'));
            $query->join('ebms_article_review', 'r', 'r.reviewer_id = u.uid');
            $query->fields('r', array('review_id'));
            $query->join('ebms_review_disposition', 'd',
                'd.review_id = r.review_id');
            $query->condition('r.packet_id', $this->packet_id);
            $query->condition('r.article_id', $article_id);
            $query->orderBy('u.name');
            $query->distinct();
            $results = $query->execute();
            $reviewers = array();

            // Loop through the list of reviewers who have already reviewed
            // this article.
            foreach ($results as $result) {

                // Add any reviewers preceding this reviewer who haven't
                // yet reviewed the article.
                while ($i < count($packet_reviewers)) {
                    $other_name = $packet_reviewers[$i];
                    if ($other_name < $result->name) {
                        $reviewers[] = '<div class="reviewer">' .
                            '<span class="reviewer-name">' .
                            htmlspecialchars($other_name) . '</span></div>';
                    }
                    if ($other_name <= $result->name)
                        ++$i;
                    else
                        break;
                }

                // Now add the reviewer who actually posted this review.
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
                    $img . '</div>';
            }

            // Pick up any leftover board members who have not yet
            // done their review of the article.
            while ($i < count($packet_reviewers))
                $reviewers[] = '<div class="reviewer">' .
                    '<span class="reviewer-name">' .
                    htmlspecialchars($packet_reviewers[$i++]) .
                    '</span></div>';

            // Assemble the row.
            $article_opts = $opts;
            $action = $article->archived ? 'revive' : 'archive';
            $article_opts['query'][$action] = $article_id;
            $href = url($url, $article_opts);
            $type = 'type="checkbox"';
            $onclick = 'onclick=\'location.href="' . $href . '"\'';
            $checked = $article->archived ? ' checked="checked"' : '';
            $checkbox = "<input $type $onclick$checked />";
            $pm_link = $pubmed . $article->source_id;
            $links = '<div class="links">' .
                l('View Abstract', $pm_link, $target);
            if ($article->uri) {
                $f_url = file_create_url($article->uri);
                $links .= ' | ' . l('Download Full Text', $f_url, $target);
            }
            $a_url = EbmsReview::$url . '/reviews/' . $this->packet_id . '/' .
                $article_id;
            $links .= ' | ' . l('Show Details', $a_url) . '</div>';
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

        // Remember when the board manager last looked at this packet (but
        // only if it's one she created herself).
        db_update('ebms_packet')
            ->fields(array('last_seen' => $now))
            ->condition('packet_id', $this->packet_id)
            ->condition('created_by', $this->user->uid)
            ->execute();

        // Put together and return the render array for the table.
        $button_class = array('class' => array('button'));
        $attrs = array('attributes' => $button_class);
        $page = array(
            'title' => array(
                '#type' => 'markup',
                '#markup' => '<h2>' . htmlspecialchars($this->packet_title) .
                '</h2>',
            ),
            'report button' => array(
                '#type' => 'markup',
                '#markup' => l('Create Report', 'reports/lit-review', $attrs),
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
        $reviewer_files = $this->get_reviewer_files();
        if ($reviewer_files)
            $page['reviewer files'] = $reviewer_files;
        return $page;
    }

    /**
     * Create the render array for a table showing the files board
     * members have posted for a packet.  Used for the board manager's
     * page for looking at a reviewed packet (there's another similar
     * method used for showing the list of these files to the board
     * members who are reviewing the packet; see fetch_reviewer_uploads()).
     * Added for TIR 2274.
     */
    private function get_reviewer_files() {
        $query = db_select('file_managed', 'f');
        $query->join('ebms_reviewer_doc', 'd', 'd.file_id = f.fid');
        $query->join('users', 'u', 'u.uid = d.reviewer_id');
        $query->fields('f', array('uri', 'filename'));
        $query->fields('u', array('name'));
        $query->fields('d', array('when_posted', 'description'));
        $query->condition('d.packet_id', $this->packet_id);
        $query->condition('d.drop_flag', 0);
        $query->orderBy('d.when_posted', 'DESC');
        $result = $query->execute();
        $rows = array();
        foreach ($result as $record) {
            $url = file_create_url($record->uri);
            $uploaded = date("m/d/Y", strtotime($record->when_posted));
            $row = array(
                l($record->filename, $url),
                htmlspecialchars($record->description),
                htmlspecialchars($record->name),
                $uploaded,
            );
            $rows[] = array('data' => $row, 'no_striping' => true);
        }
        if (empty($rows))
            return null;
        $headers = array('Filename', 'Notes', 'Uploaded By', 'Date');
        return array(
            '#attributes' => array('id' => 'packet-reviewer-files'),
            '#theme' => 'table',
            '#prefix' => '<h3 class="pkt-rvwr-files">Reviewer Files</h3>',
            '#rows' => $rows,
            '#header' => $headers,
        );
    }

    /**
     * The board manager, wanting to see more information than the thumbs-up/
     * thumbs-down icons in the page showing the whole packet, has clicked
     * on the "SHOW DETAILS" link for one of the articles in the packet.
     * Show all the information we have about the reviews submitted by the
     * board members for the article.
     */
    private function show_reviewed_article($opts) {

        // Manual breadcrumb construction to match design/requirements.
        drupal_set_breadcrumb(
            array(
                l('Literature', EbmsReview::$url . '/reviews'),
                l('Reviewed Packets', EbmsReview::$url . '/reviews'),
                l($this->packet_title, EbmsReview::$url . '/reviews/' .
                    $this->packet_id),
            )
        );

        // Construct the query to get all the reviews.
        $query = db_select('ebms_article_review', 'r');
        $query->join('users', 'u', 'u.uid = r.reviewer_id');
        $query->fields('r', array('review_id', 'when_posted', 'comments',
                'loe_info'));
        $query->fields('u', array('name'));
        $query->condition('r.packet_id', $this->packet_id);
        $query->condition('r.article_id', $this->article_id);
        $query->orderBy('r.when_posted', 'DESC');
        $results = $query->execute();

        // Display each review.
        $reviews = array();
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

            // Enhancement request from TIR 2256: show exclusion reasons.
            $query = db_select('ebms_review_rejection_value', 'v');
            $query->join('ebms_review_rejection_reason', 'r',
                'r.value_id = v.value_id');
            $query->fields('v', array('value_name'));
            $query->condition('r.review_id', $result->review_id);
            $query->orderBy('v.value_pos');
            $query->distinct();
            $reasons = $query->execute();
            $lines = array();
            foreach ($reasons as $r)
                $lines[] = '<p>' . htmlspecialchars($r->value_name) . '</p>';
            if (count($lines)) {
                $markup[] = '<h3>Exclusion Reason(s)</h3>';
                $markup = array_merge($markup, $lines);
            }

            $markup[] = '<h3>Comments</h3>';
            if ($result->comments) {
                $markup[] = '<p>' . filter_xss($result->comments, array('br', 'i')) .
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

        // Assemble the render array for the page.
        $pubmed = 'http://www.ncbi.nlm.nih.gov/pubmed/';
        $target = array('attributes' => array('target' => '_blank'));
        $pmid = db_select('ebms_article', 'a')
            ->fields('a', array('source_id'))
            ->condition('a.article_id', $this->article_id)
            ->execute()
            ->fetchField();
        $pm_url = $pubmed . $pmid;
        $download_full_text = array();
        $ft_query = db_select('ebms_article', 'a');
        $ft_query->join('file_managed', 'f', 'f.fid = a.full_text_id');
        $ft_query->fields('f', array('uri'));
        $ft_query->condition('a.article_id', $this->article_id);
        $ft_uri = $ft_query->execute()->fetchField();
        if ($ft_uri) {
            $ft_url = file_create_url($ft_uri);
            $download_full_text = array(
                '#type' => 'markup',
                '#markup' => l('Download Full Text', $ft_url, $target),
                '#prefix' => '<div id="download-full-text-link">',
                '#suffix' => '</div>',
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
                        true
                    ),
                    '#prefix' => '<div id="citation">',
                    '#suffix' => '</div>',
                ),
                'view abstract' => array(
                    '#type' => 'markup',
                    '#markup' => l('View Abstract', $pm_url, $target),
                    '#prefix' => '<div id="view-abstract-link">',
                    '#suffix' => '</div>',
                ),
                'download full text' => $download_full_text,
            ),
            'right' => array(
                '#type' => 'container',
                '#attributes' => array('id' => 'article-reviews-right'),
                'reviews' => $reviews,
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

        // check for this page being used on behalf of another
        $account = pdq_ebms_find_effective_user(false);

        $page = array();
        if ($account) {
            // append the name of the intended user and add the manager's left
            // nav
            $page['left-nav'] = $this->build_left_nav('Record Responses');
            $page['main']['#prefix'] = "<div id='main-right-panel'>";
            $page['main']['#suffix'] = "</div>";
        }

        $page['main']['form'] =
            drupal_get_form('pdq_ebms_board_member_review_form', $this);

        return $page;
    }

    /**
     * Member landing page for literature; displays all the packets
     * in the member's queue for review.
     */
    private function member_packets() {
        // check for this page being used on behalf of another
        $account = pdq_ebms_find_effective_user(false);
        $header_suffix = '';
        $body_prefix = '';
        $body_suffix = '';
        $page = array();
        if ($account) {
            // append the name of the intended user and add the manager's left
            // nav
            $header_suffix = "for $account->name";
            $page['left-nav'] = $this->build_left_nav('Record Responses');
            $body_prefix = "<div id='main-right-panel'>";
            $body_suffix = "</div>";
        }

        drupal_set_breadcrumb(array('Literature'));
        $packets = $this->reviewer_packets();
        $body = array(
            '#prefix' => "$body_prefix<div id='member-packets'>" .
            "<h2>Assigned Packets $header_suffix</h2>",
            '#suffix' => "</div>$body_suffix",
        );
        $options = array('html' => true);
        $url = EbmsReview::$url;
        $npackets = 0;
        foreach ($packets as $p) {
            $npackets++;
            $options['query'] = array('packet' => $p->packet_id);
            pdq_ebms_preserve_on_behalf($options);
            $packet_link = l($p->packet_title, $url, $options);
            $options['query']['expanded'] = 's';
            $summaries_link = l('<span>Summary Documents</span> &nbsp; +',
                              $url, $options);
            $options['query']['expanded'] = 'a';
            $articles_link = l('<span>Articles</span> &nbsp; +', $url,
                             $options);
            $options['query']['expanded'] = 'u';
            $uploads_link = l('<span>Reviewer Uploads</span> &nbsp; +', $url,
                            $options);
            $body["packet-{$p->packet_id}"] = array(
                'packet-title' => array(
                    '#type' => 'markup',
                    '#markup' => "<h2>$packet_link</h2>",
                ),
                'summary-docs' => array(
                    '#type' => 'markup',
                    '#markup' =>
                    "<h3 class='h3-first collapsed'>$summaries_link</h3>",
                ),
                'articles' => array(
                    '#type' => 'markup',
                    '#markup' => "<h3 class='collapsed'>$articles_link</h3>",
                ),
                'reviewer-uploads' => array(
                    '#type' => 'markup',
                    '#markup' =>
                    "<h3 class='h3-last collapsed'>$uploads_link</h3>",
                ),
            );
        }

        // Added at Sridhar's request (TIR 2520).
        if (!$npackets)
            $body['empty-queue'] = array(
                '#type' => 'markup',
                '#markup' => '<p>There are no review packets in your queue.</p>',
            );

        $page['body'] = $body;

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

        // check for this page being used on behalf of another
        $account = pdq_ebms_find_effective_user(false);
        $header_suffix = '';
        $body_prefix = '';
        $body_suffix = '';
        $page = array();
        if ($account) {
            // append the name of the intended user and add the manager's left
            // nav
            $header_suffix = "(On behalf of $account->name)";
            $page['left-nav'] = $this->build_left_nav('Record Responses');
            $body_prefix = "<div id='main-right-panel'>";
            $body_suffix = "</div>";
        }

        // Build the page content.
        $body = array(
            '#prefix' => "$body_prefix<div id='member-packet'>",
            '#suffix' => "</div>$body_suffix",
            'packet-title' => array(
                '#type' => 'markup',
                '#markup' => "<h2>{$this->packet_title} $header_suffix</h2>",
            ),
        );

        // Fold in the three sections and we're done.
        $this->show_packet_summaries($body);
        $this->show_packet_articles($body, $articles);
        $this->show_reviewer_uploads($body);

        $page['body'] = $body;
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
            $link = l('<span>Summary Documents</span> &nbsp; +',
                    EbmsReview::$url, $options);
            $page['summaries'] = array(
                '#type' => 'markup',
                '#markup' => "<h3 class='collapsed'>$link</h3><br />",
            );
        }

        // The section is expanded; fetch and list the summaries docs.
        else {
            $expanded = str_replace('s', '', $this->expanded);
            $options['query']['expanded'] = $expanded;
            $label = '<span>Summary Documents</span> &nbsp &ndash;';
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
     *
     * 2013-01-05 (TIR 2176): Robin wants us to show articles which have
     * already been reviewed (without the Review button).
     */
    private function show_packet_articles(&$page, $articles) {

        // Create the options for expanding or collapsing the section.
        $options = array('html' => true, 'query' => $this->query_parms());

        // If the section is collapsed, just show the link to expand it.
        if (strpos($this->expanded, 'a') === false) {
            $options['query']['expanded'] = $this->expanded . 'a';
            $link = l('<span>Articles</span> &nbsp; +', EbmsReview::$url,
                    $options);
            $page['articles'] = array(
                '#type' => 'markup',
                '#markup' => "<h3 class='collapsed'>$link</h3><br />",
            );
        }

        // The section is expanded; show the articles still in the queue.
        // Changed 2013-01-05 (TIR 2176): show all the articles in the
        // packet.
        else {

            // Standard pieces we'll need for the link out to NLM.
            $target = array('attributes' => array('target' => '_blank'));
            $pubmed = 'http://www.ncbi.nlm.nih.gov/pubmed/';

            // Squeeze out article expansion for the "collapse" URL.
            $expanded = str_replace('a', '', $this->expanded);
            $options['query']['expanded'] = $expanded;
            $label = '<span>Articles</span> &nbsp &ndash;';
            $collapse_link = l($label, EbmsReview::$url, $options);

            // Restore article expansion for the review links.
            $options['query']['expanded'] = $expanded . 'a';
            $options['attributes']['class'] = array('button');

            // Build the array of rows for the article table.
            $rows = array();
            foreach ($articles as $article) {
                $pm_id = $article->source_id;
                $links = '<div class="links">' .
                    l('view abstract', $pubmed . $pm_id, $target);

                // Add the link for viewing the full text if we have it.
                if ($article->uri) {
                    $ft_url = file_create_url($article->uri);
                    $ft_link = l('download full text', $ft_url, $target);
                    $links .= " &nbsp; | &nbsp; $ft_link";
                }
                $links .= '</div>';
                $citation = EbmsReview::show_citation($article->article_id, 3);
                $options['query']['article'] = $article->article_id;
                if ($article->when_posted) {
                    $when_posted = date('m/d/Y',
                                   strtotime($article->when_posted));
                    $c2 = "<i>reviewed<br />$when_posted</i>";
                }
                else
                    $c2 = l('Review', EbmsReview::$url, $options);
                $rows[] = array(
                    'data' => array(
                        array(
                            'data' => "$citation<br />$links",
                            'class' => array('c1'),
                        ),
                        array(
                            'data' => $c2,
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
            $link = l('<span>Reviewer Uploads</span> &nbsp; +',
                    EbmsReview::$url, $options);
            $page['uploads'] = array(
                '#type' => 'markup',
                '#markup' =>
                "<h3 class='collapsed' id='reviewer-uploads-h3'>$link</h3>" .
                '<br />',
            );
        }

        // The section is expanded; display it.
        else {

            // Open uploaded documents in a separate window or tab.
            $target = array('attributes' => array('target' => '_blank'));

            // Squeeze out section expansion for the "collapse" URL.
            $expanded = str_replace('u', '', $this->expanded);
            $options['query']['expanded'] = $expanded;
            $label = '<span>Reviewer Uploads</span> &nbsp; &ndash;';
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
                        array('data' => htmlspecialchars($doc->description)),
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
            $options['attributes']['class'] = array('button');
            $options['attributes']['id'] = 'reviewer-post-button';
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
        $query->fields('d', array('when_posted', 'description'));
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
            if ($key == 'f')
                $headers[] = 'NOTES';
        }
        return array(
            'data' => $headers,
            'no_striping' => true,
        );
    }

    /**
     * Determines if the chosen user is a reviewer for the current packet.
     *
     * @return boolean TRUE if the user is in the reviewer list, else FALSE.
     */
    private function is_reviewer() {
        $count = db_select('ebms_packet_reviewer', 'r')
            ->condition('r.reviewer_id', $this->user->uid)
            ->condition('r.packet_id', $this->packet_id)
            ->countQuery()
            ->execute()
            ->fetchField();
        return ($count > 0);
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
     *
     * 2013-01-05 (TIR 2176): Robin has asked us to change the board
     * member's packet display so that it no longer excludes articles
     * which have already been reviewed.
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
        $query->leftJoin('file_managed', 'f', 'f.fid = a.full_text_id');
        $query->fields('f', array('uri'));
        $query->fields('r', array('review_id', 'when_posted'));
        $query->condition('p.packet_id', $this->packet_id);
        $query->orderBy('a.import_date', 'DESC');
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
        return EbmsReview::reviewer_packets_by_id($this->user->uid);
    }

    /**
     * Checks the state of the review against the user's permissions to
     * determine if any disallowed actions are being attempted.
     *
     * @return boolean
     */
    private function check_permissions(){

        // if the user is trying to act on behalf of another
        if ($this->on_behalf) {

            // check their 'manage boards' access
            if(!user_access('record print responses')) {
                return false;
            }

            // if the packet is chosen and the user is not assigned, prevent
            // further display of that packet
            if ($this->packet_id && !$this->is_reviewer()) {
                $this->packet_id = 0;
                $this->article_id = 0;
            }
        }

        return true;
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
            'obo' => $this->on_behalf,
        );
        foreach ($names as $name => $value)
            if ($value)
                $parms[$name] = $value;
        return $parms;
    }

    /**
     * Display very brief citation for an article, consisting only of the
     * first author's surname, the journal's brief title, the year date,
     * and the PubMed ID, separated by semicolons.  This form of the
     * citation is stored in the brf_citation column of the ebms_article
     * table, so there's very little work to do here.
     *
     * 2013-01-10: Robin has changed the requirements for this citation
     * display (see TIR 2260).  She now wants the title of the article
     * included, as well as a link to the full text of the article, if
     * available.
     */
    private static function brief_citation($article_id) {

        // Fetch the information we need from the database.
        $query = db_select('ebms_article', 'a');
        $query->leftJoin('file_managed', 'f', 'f.fid = a.full_text_id');
        $query->fields('a', array('source_data'));
        $query->fields('f', array('uri'));
        $query->condition('a.article_id', $article_id);
        $result = $query->execute()->fetch();
        $doc = new \SimpleXMLElement($result->source_data);
        $article = $doc->MedlineCitation->Article;
        $uri = $result->uri;

        // Assemble the pieces of the citation
        $citation = array();
        foreach ($doc->MedlineCitation->Article->AuthorList->Author as $a) {
            if ($a->LastName)
                $citation = array(htmlspecialchars($a->LastName));
            break;
        }
        $citation[] = htmlspecialchars($article->ArticleTitle);
        $ta = $doc->MedlineCitation->MedlineJournalInfo->MedlineTA;
        $citation[] = htmlspecialchars($ta);
        $year = $article->Journal->JournalIssue->PubDate->Year;
        if ($year)
            $citation[] = $year;
        $citation[] = $doc->MedlineCitation->PMID;
        $citation = implode('; ', $citation);

        // If we have the full text, provide a link to it.
        if ($uri) {
            $url = file_create_url($uri);
            $attrs = array('attributes' => array('target' => '_blank'));
            $link = l('DOWNLOAD FULL TEXT', $url, $attrs);
            $citation .= " &nbsp; $link";
        }

        // All done.
        return $citation;
    }

    /**
     * Get the topics handled by a particular board, and which have
     * articles which could be assigned to a review packet.  Can't
     * use Drupal's DB API, because it isn't sophisticated enough
     * to handle nested correlated queries.
     * TIR 2266: only include topics with eligible articles have
     * full text in the system.
     */
    public static function topics_for_board($board_id, $state_id) {
        $query = "
SELECT DISTINCT t.topic_id, t.topic_name
           FROM ebms_topic t
           JOIN ebms_article_state s
             ON t.topic_id = s.topic_id
           JOIN ebms_article a
             ON a.article_id = s.article_id
          WHERE s.active_status = 'A'
            AND s.current = 'Y'
            AND s.state_id = $state_id
            AND t.board_id = $board_id
            AND a.full_text_id IS NOT NULL
            AND s.article_id NOT IN
   (SELECT DISTINCT article_id
               FROM ebms_packet_article
              WHERE drop_flag = 0
                AND packet_id IN
       (SELECT packet_id
          FROM ebms_packet
         WHERE topic_id = s.topic_id))";
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
     *
     * 2013-02-06 (TIR 2266): apparently it's not enough for the
     * article to have reached the state of "passed full text
     * review"; now we must check to verify that the full text
     * is actually present (and that the user wasn't fibbing
     * when she said the article had passed full text review).
     */
    public static function articles_for_topic(
        $topic_id, $state_id, $packet_id=0
    ) {
        if (!$topic_id)
            return array();
        $subquery = db_select('ebms_packet', 'p')
            ->fields('p', array('packet_id'))
            ->condition('p.topic_id', $topic_id);
        $subquery = db_select('ebms_packet_article', 'a')
            ->fields('a', array('article_id'))
            ->condition('a.packet_id', $subquery, 'IN')
            ->condition('a.drop_flag', 0)
            ->distinct();
        $query = db_select('ebms_article_state', 's');
        $query->fields('s', array('article_id'));
        $query->condition('s.topic_id', $topic_id);
        $query->condition('s.active_status', 'A');
        $query->condition('s.state_id', $state_id);
        $query->condition('s.current', 'Y');
        $query->condition('s.article_id', $subquery, 'NOT IN');
        $query->join('ebms_article', 'ea', 'ea.article_id = s.article_id');
        $query->isNotNull('ea.full_text_id');
        $query->distinct();
        if ($packet_id) {
            $union = db_select('ebms_packet_article', 'pa')
                ->fields('pa', array('article_id'))
                ->condition('pa.packet_id', $packet_id)
                ->condition('pa.drop_flag', 0);
            $query->union($union);
        }
        if (false)
            dpq($query);
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
     * Build the list of board members who specialize in reviewing
     * articles for the specified topic.
     */
    public static function default_reviewers_for_topic($topic_id) {
        if (!$topic_id)
            return array();
        $query = db_select('users', 'u');
        $query->fields('u', array('uid', 'name'));
        $query->join('ebms_topic_reviewer', 'r', 'r.user_id = u.uid');
        $query->condition('r.topic_id', $topic_id);
        $query->condition('u.status', 1);
        $reviewers = array();
        $results = $query->execute();
        foreach ($results as $result)
            $reviewers[$result->uid] = $result->name;
        return $reviewers;
    }

    /**
     * Collect the list of active members of the specified board.
     */
    public static function board_members($board_id) {
        if (!$board_id)
            return array();
        $query = db_select('users', 'u');
        $query->fields('u', array('uid', 'name'));
        $query->join('ebms_board_member', 'm', 'm.user_id = u.uid');
        $query->join('users_roles', 'ur', 'ur.uid = u.uid');
        $query->join('role', 'r', 'r.rid = ur.rid');
        $query->condition('m.board_id', $board_id);
        $query->condition('r.name', 'board member');
        $query->condition('u.status', 1);
        $members = array();
        $results = $query->execute();
        foreach ($results as $result)
            $members[$result->uid] = $result->name;
        return $members;
    }

    /**
     * Fetch all of the packets assigned to the board member, in which
     * at least one article is still waiting to be reviewed.
     */
    public static function reviewer_packets_by_id($user_id) {
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
        $query->condition('m.reviewer_id', $user_id);
        $query->condition('p.active_status', 'A');
        $query->isNull('r.article_id');
        $query->distinct();
        $query->orderBy('p.created_at', 'DESC');
        $query->orderBy('t.topic_name');
        return $query->execute();
    }

    /**
     * Format the article's citation the way the users want it for the
     * board members' review pages.  It was a nice idea to have a single
     * routine in common.inc to do this for the whole site, but each
     * page has slightly different requirements for how the citations
     * are to be formatted.
     */
    public static function show_citation($article_id, $max_authors=99999) {
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
        foreach ($doc->MedlineCitation->Article->AuthorList->Author as $author)
        {
            if (sizeof($authors) == $max_authors) {
                $authors[] = 'et al.';
                break;
            }
            $name = $author->CollectiveName;
            if (!$name) {
                $name = $author->LastName;
                if ($name && $author->Initials)
                    $name .= ' ' . $author->Initials;
            }
            if ($name)
                $authors[] = htmlspecialchars($name);
        }
        $pieces = array();
        $authors = implode(', ', $authors);
        if ($authors)
            $pieces = array("$authors<br />");
        if ($title)
            $pieces[] = "<em>$title</em><br />";
        $ta = $doc->MedlineCitation->MedlineJournalInfo->MedlineTA;
        $ta = htmlspecialchars($ta);
        if ($ta) {
            $pieces[] = $ta;
            if ($vol) {
                $pieces[] = " $vol";
                if ($issue)
                    $pieces[] = " ($issue)";
            }
            if ($pgn)
                $pieces[] = ": $pgn";
            if ($year)
                $pieces[] = ", $year";
            $pieces[] = '<br />';
        }
        $pmid = $doc->MedlineCitation->PMID;
        $pieces[] = "PMID: $pmid";
        return implode($pieces);
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

    /**
     * Pull the information for the packet from the ebms_packet and
     * related tables.
     */
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

    /**
     * Get the list of board members assigned to review the articles in
     * this packet.
     */
    public static function fetch_reviewers($packet_id) {
        $query = db_select('ebms_packet_reviewer', 'r');
        $query->join('users', 'u', 'u.uid = r.reviewer_id');
        $results = $query
            ->fields('r', array('reviewer_id'))
            ->condition('r.packet_id', $packet_id)
            ->orderBy('u.name')
            ->execute();
        foreach ($results as $result)
            $reviewers[] = $result->reviewer_id;
        return $reviewers;
    }

    /**
     * Get the list of articles assigned for review as part of this packet.
     */
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

    /**
     * Get the list of summary documents attached to this packet by the
     * board manager.
     */
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

    // Assemble the rest of the values needed for the form.
    $article_id = $review->article_id;
    $title = htmlspecialchars($review->packet_title);
    $attrs = array('attributes' => array('target' => '_blank'));
    $pm_url = EbmsReview::pubmed_url($article_id);
    $links = '<div class="links">' . l('VIEW ABSTRACT', $pm_url, $attrs);
    $cite = EbmsReview::show_citation($article_id);

    // Add a link for viewing the article's full text (if we have it).
    $query = db_select('file_managed', 'f');
    $query->join('ebms_article', 'a', 'a.full_text_id = f.fid');
    $query->condition('a.article_id', $article_id);
    $query->fields('f', array('uri'));
    $results = $query->execute();
    foreach ($results as $result) {
        $ft_url = file_create_url($result->uri);
        $ft_link = l('DOWNLOAD FULL TEXT', $ft_url, $attrs);
        $links .= " &nbsp; | &nbsp; $ft_link";
    }
    $links .= '</div>';

    // check for this page being used on behalf of another
    $account = pdq_ebms_find_effective_user(false);
    $header_suffix = '';

    if ($account) {
        // append the name of the intended user and add the manager's left
        // nav
        $header_suffix = "(On behalf of $account->name)";
    }

    // Assemble and return the form's render array.
    return array(
        '#id' => 'member-review',
        'packet-title' => array(
            '#type' => 'markup',
            '#markup' => "<h2>$title $header_suffix</h2>"
        ),
        'packet-subtitle' => array(
            '#type' => 'markup',
            '#markup' => '<h3 class="red">REVIEW</h3>'
        ),
        'article-citation-block' => array(
            '#type' => 'markup',
            '#prefix' => '<p id="article-citation">',
            '#suffix' => '</p>',
            '#markup' => "$cite <br /> $links",
        ),
        'packet' => array(
            '#type' => 'hidden',
            '#value' => $review->packet_id,
        ),
        'article' => array(
            '#type' => 'hidden',
            '#value' => $article_id,
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
            '#title' => 'Reason(s) for Exclusion From PDQ&reg; Summary',
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
    $account = pdq_ebms_find_effective_user(false);

    // check for this page being used on behalf of another
    global $user;
    $comment_suffix = '';
    if ($account) {
        // append the name of the intended user and add the manager's left
        // nav.  TIR 2409: don't use HTML markup to do this, since we're
        // appending to raw user input.
        $comment_suffix = " (Recorded by $user->name on behalf of $account->name.)";
    }
    else
    {
        $account = $user;
    }

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
                'reviewer_id' => $account->uid,
                'when_posted' => $now,
                'comments'    => $comments.$comment_suffix,
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
    pdq_ebms_preserve_on_behalf($options);
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
    $account = pdq_ebms_find_effective_user();
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
                    'reviewer_id' => $account->uid,
                    'packet_id' => $packet_id,
                    'when_posted' => $now,
                    'doc_title' => $file->filename,
                    'description' => $filenotes,
                )
            )
            ->execute();

        // Find all the summaries subpages associated with the packet's topic.
        $query = db_select('ebms_summary_page_topic', 't');
        $query->fields('t', array('page_id'));
        $query->join('ebms_packet', 'p', 'p.topic_id = t.topic_id');
        $query->condition('p.packet_id', $packet_id);
        $results = $query->execute();
        $page_ids = array();
        foreach ($results as $result)
            $page_ids[] = $result->page_id;
        
        $path_parts = pathinfo($file->filename);
        $filename = $path_parts['filename'];

        // If there are any such subpages, also link this document with
        // each of them (TIR 2177).
        if (count($page_ids)) {
            $doc_id = db_insert('ebms_doc')
                ->fields(
                    array(
                        'file_id' => $file->fid,
                        'when_posted' => $now,
                        'description' => $filename,
                    )
                )
                ->execute();
            foreach ($page_ids as $page_id)
                db_insert('ebms_summary_returned_doc')
                ->fields(
                    array(
                        'doc_id' => $doc_id,
                        'page_id' => $page_id,
                        'notes' => $filenotes,
                    )
                )
                ->execute();
        }
        
        module_load_include('inc', 'ebms', 'EbmsArticle');
        $packetInfo = \Ebms\getPacketInfo($packet_id);
        \Ebms\Util::activityNewSummary(array(
            'account' => $account,
            'title' => $filename, 
            'topics' => array($packetInfo->topic_id)));

        // Tell the user that we succeeded.
        drupal_set_message('Posted document ' . $file->filename . '.');
    }
    else

        // Tell the user that we failed.
        drupal_set_message('File not found', 'error');

    // Go back to the user's packet.
    $options = array(
        'query' => array(
            'packet' => $packet_id,
            'expanded' => $expanded,
            'sort-col' => $sort_col,
            'sort-dir' => $sort_dir,
        ),
    );
    pdq_ebms_preserve_on_behalf($options);
    drupal_goto(EbmsReview::$url, $options);
}

/**
 * Create the form used by a board manager to create a new literature
 * review packet.  We show the parts of the form incrementally, as
 * lower portions depend on the selections made in the upper portions.
 */
function pdq_ebms_edit_packet_form($form, &$form_state, $packet_id=0) {

    // We'll use this more than once, so fetch and save it now.
    $state_id = Ebms\Util::get_state_id('Passed full text review');

    // If we're editing an existing packet, we already know board and topic.
    if ($packet_id) {
        $packet = new EbmsPacket($packet_id);
        $board_id = $packet->board_id;
        $topic_id = $current_topic = $packet->topic_id;
        $packet_header = 'Edit Packet - ' . htmlspecialchars($packet->title);
        $packet_name = $packet->title;
    }
    else {

        // See if the board and topic have already been selected.
        $board_id = $topic_id = $current_topic = 0;
        if (!empty($form_state['values']['board']))
            $board_id = $form_state['values']['board'];
        if (!empty($form_state['values']['topic']))
            $topic_id = $form_state['values']['topic'];
        if (!empty($form_state['values']['current-topic']))
            $current_topic = $form_state['values']['current-topic'];
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
            l('Literature', EbmsReview::$url . '/reviews'),
            $packet_header,
        )
    );

    // Check the state of the toggle for showing all board members.
    $all_reviewers = false;
    if (!empty($form_state['values']['all-reviewers']))
        $all_reviewers = $form_state['values']['all-reviewers'];
    if ($all_reviewers)
        $all_reviewers_title =
            '<span class="expanded">Show Only Topic Reviewers</span> &ndash;';
    else
        $all_reviewers_title = '<span>Show All Board Members</span> +';

    // Initialize default arrays (replaced below as appropriate).
    $article_defaults = $summary_defaults = $reviewer_defaults =
        $selected_reviewers = array();

    // Set up the fields which depend on having a topic selected.
    if ($topic_id) {

        // Find out which reviewers are already selected.
        if (isset($form_state['values'])) {

            // If the topic just got changed, ignore previous selections.
            if ($topic_id == $current_topic) {
                foreach ($form_state['values']['reviewers'] as $id => $val) {
                    if ($val)
                        $selected_reviewers[] = $id;
                }
            }
        }

        // Fresh form for an existing packet; selections are from the DB table.
        elseif ($packet_id)
            $selected_reviewers = $packet->reviewers;

        // Which reviewers should we show for selection?
        $board_members = EbmsReview::board_members($board_id);
        $topic_reviewers = EbmsReview::default_reviewers_for_topic($topic_id);
        $reviewers = $all_reviewers ? $board_members : $topic_reviewers;

        // Which ones should be checked?
        if ($topic_id == $current_topic)
            $reviewer_defaults = $selected_reviewers;
        else
            $reviewer_defaults = array_keys($topic_reviewers);

        // Make sure the reviewers shown include anyone who is currently
        // selected.
        foreach ($reviewer_defaults as $uid) {
            if (!array_key_exists($uid, $reviewers)) {
                $name = db_select('users', 'u')
                    ->fields('u', array('name'))
                    ->condition('u.uid', $uid)
                    ->execute()
                    ->fetchField();
                $reviewers[$uid] = $name;
            }
        }

        // Make sure the checkboxes are in the right order.
        natsort($reviewers);

        // Take care of the rest of the fields.
        $articles_title = 'Articles';
        $summaries_title = 'Summary Documents (Optional)';
        $reviewers_title = 'Reviewers';
        $all_reviewers_class = 'all-reviewers-visible';
        $articles = EbmsReview::articles_for_topic($topic_id, $state_id,
                    $packet_id);
        $summaries = EbmsReview::summaries_for_topic($topic_id, $packet_id);
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
    $form_state['ebms-checked-articles'] = $article_defaults;
    $form_state['ebms-checked-reviewers'] = $reviewer_defaults;

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
    if ($packet_id) {
        if (isset($form_state['values']))
            $packet_name = $form_state['values']['name'];
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
                    'name' => array(
                        '#type' => 'textfield',
                        '#title' => 'Packet Name',
                        '#default_value' => $packet_name,
                    ),
                ),
            ),
            'submit' => $submit_field,
        );
    }

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
            'current-topic' => array(
                '#type' => 'hidden',
                '#value' => $topic_id,
            ),
        );
        if ($topic_id) {
            $name = '';
            if (array_key_exists($topic_id, $topics))
                $name = $topics[$topic_id];
            $form[$bsf][$tsf]['articles'] = $articles_field;
            $form[$bsf][$tsf]['summaries'] = $summaries_field;
            $form[$bsf][$tsf]['reviewers-div'] = $reviewers_div;
            $form[$bsf][$tsf]['name'] = array(
                '#type' => 'textfield',
                '#title' => 'Packet Name',
                '#default_value' => $name,
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
    $name = trim($form_state['values']['name']);
    if (!$name)
        form_set_error('name', 'Packet must have a name.');
    if (!$packet_id) {
        if (!$form_state['values']['board'])
            form_set_error('board', 'Board selection is required.');
        if (!$form_state['values']['topic'])
            form_set_error('topic', 'Topic selection is required.');
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
    $name = trim($form_state['values']['name']);
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
        require_once('EbmsArticle.inc');

        // Which articles are in the packet already?
        $rows = db_select('ebms_packet_article', 'a')
            ->fields('a', array('article_id', 'drop_flag'))
            ->condition('a.packet_id', $packet_id)
            ->execute();
        $in_packet = array();
        foreach ($rows as $row)
            $in_packet[$row->article_id] = $row->drop_flag ? true : false;

        // Update the packet title (TIR 2495).
        db_update('ebms_packet')
            ->fields(array('packet_title' => $name))
            ->condition('packet_id', $packet_id)
            ->execute();

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
    
    // (create a recent activity notification for all of these users)
    \Ebms\Util::activityPacketCreated($name, $reviewers);

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
 * 2013-04-12: Same bug is affecting the reviewers field; same
 * workaround.
 */
function pdq_ebms_packet_topic_ajax($form, $form_state) {
    $checked_reviewers = $form_state['ebms-checked-reviewers'];
    $checked_articles = $form_state['ebms-checked-articles'];
    $subform = $form['board-specific-fields']['topic-specific-fields'];
    foreach ($subform['articles'] as $k => $v) {
        if (isset($v['#type']) and $v['#type'] == 'checkbox') {
            $checked = in_array($k, $checked_articles) ? 1 : 0;
            $subform['articles'][$k]['#checked'] = $checked;
        }
    }
    foreach ($subform['reviewers-div']['reviewers'] as $k => $v) {
        if (isset($v['#type']) and $v['#type'] == 'checkbox') {
            $checked = in_array($k, $checked_reviewers) ? 1 : 0;
            $subform['reviewers-div']['reviewers'][$k]['#checked'] = $checked;
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

/**
 * Allows searching through boards and board members in order to find all
 * packets available for review or a specific packet.
 *
 * @param type $form
 * @param type $form_state
 */
function pdq_ebms_record_responses_form($form, &$form_state) {
    module_load_include('inc', 'ebms', 'EbmsArticle');

    // define an attribute to turn off field autocompletes
    $attributes = array(
        'autocomplete' => 'off',
    );

    $form['board'] = array(
        '#type' => 'select',
        '#title' => t('Board'),
        '#required' => true,
        '#options' => Ebms\Util::boards(),
        '#attributes' => $attributes,
        '#ajax' => array(
            'callback' => 'pdq_ebms_record_responses_board_js',
            'wrapper' => 'member-field',
        ),
    );

    $memberOptions = array(false => t('Please select a board.'));
    $memberRequired = false;

    if (isset($form_state['values']['board'])) {
        $boardId = $form_state['values']['board'];

        if ($boardId) {
            $memberOptions = Ebms\Util::get_active_users(array('board member'),
                    array($boardId));
            $memberRequired = true;
        }
    }

    $form['member'] = array(
        '#type' => 'select',
        '#title' => t('Board Member'),
        '#required' => $memberRequired,
        '#disabled' => !$memberRequired,
        '#options' => $memberOptions,
        '#attributes' => $attributes,
        '#prefix' => "<div id='member-field'>",
        '#suffix' => "</div>",
        '#ajax' => array(
            'callback' => 'pdq_ebms_record_responses_member_js',
            'wrapper' => 'packet-field',
        ),
    );

    $packetOptions = array(false => t('Please select a member.'));
    if (isset($form_state['values']['member'])) {
        $memberId = $form_state['values']['member'];

        if ($memberId) {
            $packetOptions = array(false => '');
            $packets = EbmsReview::reviewer_packets_by_id($memberId);

            foreach ($packets as $p) {
                $packetOptions[$p->packet_id] = $p->packet_title;
            }
        }
    }

    $form['packet'] = array(
        '#type' => 'select',
        '#title' => t('Packet'),
        '#options' => $packetOptions,
        '#attributes' => $attributes,
        '#disabled' => sizeof($packetOptions) == 1,
        '#prefix' => "<div id='packet-field'>",
        '#suffix' => "</div>",
    );

    $form['search_button'] = array(
        '#type' => 'submit',
        '#value' => 'Search',
    );

    $form['cancel_button'] = array(
        '#type' => 'submit',
        '#value' => 'Cancel',
        '#limit_validation_errors' => array(),
        '#submit' => array('pdq_ebms_record_responses_form_cancel'),
    );

    return $form;
}

function pdq_ebms_record_responses_board_js($form, $form_state){
    return $form['member'];
}

function pdq_ebms_record_responses_member_js($form, $form_state){
    return $form['packet'];
}

function pdq_ebms_record_responses_form_submit($form, &$form_state) {
    // retrieve the selected user
    $memberId = $form_state['values']['member'];
    $url = EbmsReview::$url . '/assigned';

    // build the redirect options, namely the query
    $opts['query']['obo'] = $memberId;

    // if the packet is also set, add to the query and alter the destination
    $packetId = $form_state['values']['packet'];
    if ($packetId) {
        $url = EbmsReview::$url;
        $opts['query']['packet'] = $packetId;
    }

    drupal_goto($url, $opts);
}

function pdq_ebms_record_responses_form_cancel($form, &$form_state) {
    $form_state['rebuild'] = false;
}

/**
 * Takes an option array by reference and adds the on-behalf query if it exists
 * for the current page.
 *
 * @param array $opts The option array.
 */
function pdq_ebms_preserve_on_behalf(&$opts) {
    $params = drupal_get_query_parameters();
    if (isset($params['obo'])) {
        $opts['query']['obo'] = $params['obo'];
    }
}

/**
 * Finds for which particular user reviews should be attributed.  This will
 * usually be the current user, but may be another if entering print responses.
 *
 * @global type $user
 * @param bool $include_user Flag to include the current (global) user as a
 *                           potential output.
 * @return mixed             Returns the effective user, which is possibly the
 *                           current user.  Can return FALSE if the global user
 *                           is not to be included.
 */
function pdq_ebms_find_effective_user($include_user = true) {
    $params = drupal_get_query_parameters();
    if (isset($params['obo'])) {
        $account = user_load($params['obo']);
        if ($account !== false)
            return $account;
    }

    if ($include_user) {
        global $user;
        return $user;
    }

    return false;
}
