<?php

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
        $sort_col,
        $sort_dir,
        $action,
        $reviewed,
        $pager,
        $on_behalf;
    public static $url = 'review';
    public static $fyi_state = 0;

    /**
     * Passing settings in the URL instead of in $_SESSION in order
     * to avoid dependency on Javascript being enabled on the user's
     * browser.
     */
    public function __construct($action, $packet_id, $article_id) {
        $this->packet_id     = Ebms\Util::get_request_value('packet',
                               $packet_id);
        $this->article_id    = Ebms\Util::get_request_value('article',
                               $article_id);
        $this->sort_col      = Ebms\Util::get_request_value('sort-col', 'f');
        $this->sort_dir      = Ebms\Util::get_request_value('sort-dir', 'a');
        $this->action        = $action;
        $this->reviewed      = Ebms\Util::get_request_value('reviewed');
        $this->last_reviewed = Ebms\Util::get_request_value('last-reviewed');
        $this->pager         = Ebms\Util::get_request_value('pager');
        $this->on_behalf     = Ebms\Util::get_request_value('obo');
        $this->packet_title  = EbmsPacket::fetch_title($this->packet_id);

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
     * Select the version of the page appropriate for the user's role.
     */
    public function run() {
        Ebms\Util::add_js();
        drupal_add_css(Ebms\CSS_DIR . '/review.css');
        Ebms\Menu::$active = 'Literature@@BR@@Packets';

        if (!$this->check_permissions())
            return MENU_ACCESS_DENIED;

        if (in_array('board member', $this->user->roles)) {
            if ($this->action == 'reviewer-upload') {
                return $this->reviewer_upload_page();
            }
            return $this->choose_member_review_page();
        }
        if ($this->needs_packet_bulk_archive_confirmation())
            return drupal_get_form('ebms_packet_bulk_archive_confirm');
        $this->perform_requested_bulk_archive();
        return $this->choose_manager_review_page();
    }

    /**
     * Determine whether we need to get the user to confirm archiving.
     *
     * Yes, this looks strange, but this is the way core Drupal code
     * does it (by examining $_POST directly).
     */
    private function needs_packet_bulk_archive_confirmation() {
        # drupal_set_message('POST is ' . print_r($_POST, true));
        $archive = 'ebms_packet_bulk_archive';
        if (!isset($_POST['form_id']) || $_POST['form_id'] !== $archive)
            return false;
        if (!isset($_POST['op']) || $_POST['op'] !== 'ARCHIVE')
            return false;
        return !empty($_POST['packets']);
    }

    /**
     * Perform a bulk packet archive request, if approved.
     *
     * This is even stranger, but Drupal is not routing the confirm
     * button to the submit handler for the confirmation form. So
     * we have to work from $_POST directly.
     */
    private function perform_requested_bulk_archive() {

        // Are we catching the results of the confirmation page?
        $form_id = 'ebms_packet_bulk_archive_confirm';
        if (!isset($_POST['form_id']) || $_POST['form_id'] !== $form_id)
            return;

        // Make sure the action is confirmed and has selected packets.
        if (empty($_POST['confirm']) || empty($_POST['packets']))
            return;

        // Walk through the selected packets marking each as inactive.
        $packets = array_filter($_POST['packets']);
        foreach ($packets as $pid => $value) {
            db_update('ebms_packet')
                ->fields(array('active_status' => 'I'))
                ->condition('packet_id', $pid)
                ->execute();
        }

        // Tell the user what we did.
        $count = count($packets);
        drupal_set_message("Archived $count packet(s).");
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
            case 'unreviewed':
                return $this->manager_unreviewed();
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
        drupal_set_message("Packet '$title' has been archived.");
        $opts = array('query' => drupal_get_query_parameters());
        if (isset($opts['query']['target'])) {
            $target = $opts['query']['target'];
            unset($opts['query']['target']);
            if ($target == 'reviews')
                drupal_goto(EbmsReview::$url . '/reviews', $opts);
            if ($target == 'unreviewed')
                drupal_goto(EbmsReview::$url . '/unreviewed', $opts);
        }
        drupal_goto(EbmsReview::$url . '/packets', $opts);
    }

    /**
     * Shows the form to select a user and optionally a packet in order to
     * enter responses on behalf of a board member.
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
                '#prefix' =>
                '<div id="main-right-panel"><h2>Record Responses</h2>',
                '#suffix' => '</div>',
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
    public static function manager_packets_view_toggle($options) {
        if (Ebms\Util::get_request_value('reviewed')) {
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
                'View/Edit Packets',
            )
        );
        $url = EbmsReview::$url . '/packets';
        $opts = array('query' => drupal_get_query_parameters());

        // If there was a filter request, the drupal path will say:
        //  {url}/packets/search/{num}
        // where {num} is a string form row id in the ebms_search table
        if ($this->packet_id == 'search')
            $search_id = intval($this->article_id);
        else
            $search_id = 0;

        // Get the form for the top (selecting packets).
        $form_id = 'pdq_ebms_manager_packets_filter_form';
        $form = drupal_get_form($form_id, $search_id);
        $form['#prefix'] = '<div id="manager-packets-filter">';
        $form['#suffix'] = '</div>';

        // Get the form for the bottom (showing packets).
        $form_id = 'ebms_packet_bulk_archive';
        $report = drupal_get_form($form_id, $search_id);
        if ($this->pager != 'off')
            $report['pager'] = array('#theme' => 'pager');
        else {
            unset($opts['query']['pager']);
            $link = l('VIEW PAGES', $url, $opts);
            $report['pager'] = array(
                '#type' => 'markup',
                '#markup' => "<div id='pager-view-pages'>$link</div>",
            );
        }

        // Get the navigation panel for the left side of the page.
        $left_nav = $this->build_left_nav('View / Edit Packets');
        if (count($this->user->boards) > 1)
            $left_nav['board filter box'] =
                $this->board_filter_box($opts, $url);

        // Put the whole page together.
        return array(
            'left-nav' => $left_nav,
            'packets' => array(
                '#type' => 'container',
                '#attributes' => array('id' => 'main-right-panel'),
                'form' => $form,
            ),
            'report' => $report,
        );
    }

    /**
     * Apply filter criteria to the manager_packets list.
     * Subroutine of manager_packets().
     *
     * @param object $query     Partially completed
     * @param int $search_id    Row ID in the ebms_search table.
     *
     * @return                  Modified query object.
     */
    public static function apply_manager_packet_filters($query, $search_id) {

        // Filter criteria are in the database, get them
        $filters = array();
        if ($search_id > 0)
            $filters = pdq_ebms_get_filter_criteria($search_id);

        if (count($filters)) {
            // Topics checkboxes
            if (count($filters['filt_topic']) > 0) {

                // Gather checked topic ids
                $topics = array();
                foreach ($filters['filt_topic'] as $tid => $val)
                    if ($val != 0)
                        array_push($topics, $tid);

                // If any, add them to the query
                if (count($topics) > 0)
                    $query->condition('t.topic_id', $topics, 'IN');
            }

            // Wildcard string in title
            if ($filters['packet_title_text'])
                $query->condition('p.packet_title',
                                   $filters['packet_title_text'], 'LIKE');
            // Packet creation date limits
            if (isset($filters['date-start']['date']))
                $query->condition('p.created_at',
                                  $filters['date-start']['date'], '>=');
            if (isset($filters['date-end']['date'])) {
                $date = Ebms\Util::fill_in_date($filters['date-end']['date'],
                                                true);
                $query->condition('p.created_at', $date, '<=');
            }

            // Archived and legacy system packets. These are interrelated
            $scope = $filters['pkt_scope_active'];
            $active = $scope['Active'] ? True : False;
            $archived = $scope['Archived'] ? True : False;
            $scope = $filters['pkt_scope_reviewed'];
            $reviewed = $scope['Reviewed'] ? True : False;
            $unreviewed = $scope['Unreviewed'] ? True : False;

            // Apply the checkboxes.
            if (!($active && $archived)) {
                if ($active)
                    $query->condition('p.active_status', 'A');
                elseif ($archived)
                    $query->condition('p.active_status', 'I');
                else
                    $query->condition('p.packet_id', 0);
            }
            if (!($reviewed && $unreviewed)) {
                if ($unreviewed) {
                    $query->leftJoin('ebms_article_review', 'ear',
                                     'ear.packet_id = p.packet_id');
                    $query->isNull('ear.packet_id');
                }
                elseif ($reviewed)
                    $query->join('ebms_article_review', 'ear',
                                 'ear.packet_id = p.packet_id');
                else
                    $query->condition('p.packet_id', 0);
            }
        }

        // No filters selected, default to just active docs
        else
            $query->condition('p.active_status', 'A');

        return $query;
    }

    /**
     * Apply filter criteria to modifying the default values in
     * the filtering form for the manager_packets list.
     *
     * When a user filters the packets to display we rebuild the form for
     * filtering when constructing the newly filtered packet display.  By
     * showing the just used filter criteria as default values the user
     * can see what criteria caused her search results to appear as they
     * are, and can also easily refine the search without remembering and
     * re-entering everything.
     *
     * @param object $form      Modify defaults in this form render array.
     * @param int $search_id    Row ID in the ebms_search table containing
     *                          the last search - new default values.
     *
     * @return                  Modified form array
     */
    public static function apply_manager_packet_filter_search($form,
                                                              $search_id=0) {
        $filters = pdq_ebms_get_filter_criteria($search_id);
        if (count($filters) == 0) {
            // Nothing to do
            return $form;
        }

        // Checked topics
        $checked_topics = array();
        if (count($filters['filt_topic']) > 0) {
            foreach ($filters['filt_topic'] as $key => $val) {
                if ($key == $val) {
                    array_push($checked_topics, $val);
                }
            }
        }
        if (count($checked_topics) > 0)
            $form['topic_box']['filt_topic']['#default_value']=$checked_topics;

        // Wildcard string in title
        if ($filters['packet_title_text']) {
            $form['packet_title_text']['#default_value'] =
                    $filters['packet_title_text'];
        }

        // Date limits
        if (!empty($filters['date-start']['date'])) {
            $form['date-limits']['date-pair']['date-start']['#default_value'] =
                Ebms\Util::fill_in_date($filters['date-start']['date']);
        }
        if (!empty($filters['date-end']['date'])) {
            $form['date-limits']['date-pair']['date-end']['#default_value'] =
                Ebms\Util::fill_in_date($filters['date-end']['date'], true);
        }

        // Scope checkboxes
        $scopes = ['pkt_scope_active', 'pkt_scope_reviewed'];
        foreach ($scopes as $scope) {
            $default_scope = array();
            foreach ($filters[$scope] as $key => $val) {
                if ($key === $val) {
                    array_push($default_scope, $val);
                }
            }
            $form[$scope]['#default_value']= $default_scope;
        }

        return $form;
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
            array('Unreviewed Packets', 'review/unreviewed'),
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
                l('Literature', EbmsReview::$url),
                'Reviewed Packets',
            )
        );
        $left_nav = $this->build_left_nav('Reviewed Packets');
        $url = EbmsReview::$url . '/reviews';
        if (count($this->user->boards) > 1) {
            $board_opts = array('query' => array('pager' => 'off'));
            if (!empty($opts['query']['boards']))
                $board_opts['query']['boards'] = $opts['query']['boards'];
            $left_nav['board filter box'] =
                $this->board_filter_box($board_opts, $url);
        }
        $form = drupal_get_form('ebms_reviewed_packets_filter_form', $opts);
        return array(
            'left-nav' => $left_nav,
            'filter' => array(
                '#type' => 'container',
                '#attributes' => array('id' => 'main-right-panel'),
                'form' => $form,
            ),
            'packets' => $this->reviewed_packet_list($opts),
        );
    }

    /**
     * Show packets for which no reviews have been submitted yet.
     * Switches to the appropriate page type, if the user has drilled
     * down to a specific packet or article.
     */
    private function manager_unreviewed() {
        $opts = array('query' => drupal_get_query_parameters());
        if ($this->packet_id)
            return $this->show_unreviewed_packet($opts);
        drupal_set_breadcrumb(
            array(
                l('Literature', EbmsReview::$url),
                'Unreviewed Packets',
            )
        );
        $left_nav = $this->build_left_nav('Unreviewed Packets');
        $url = EbmsReview::$url . '/unreviewed';
        if (count($this->user->boards) > 1) {
            $board_opts = array('query' => array('pager' => 'off'));
            if (!empty($opts['query']['boards']))
                $board_opts['query']['boards'] = $opts['query']['boards'];
            $left_nav['board filter box'] =
                $this->board_filter_box($board_opts, $url);
        }
        $form = drupal_get_form('ebms_unreviewed_packets_filter_form', $opts);
        return array(
            'left-nav' => $left_nav,
            'filter' => array(
                '#type' => 'container',
                '#attributes' => array('id' => 'main-right-panel'),
                'form' => $form,
            ),
            'packets' => $this->unreviewed_packet_list($opts),
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
        $boards = EbmsReview::get_board_ids();

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
        $query->fields('p', array('packet_id', 'packet_title', 'last_seen',
                'starred'));
        $query->fields('b', array('board_manager'));
        $query->addExpression('MAX(r.when_posted)', 'updated');
        $query->condition('b.board_id', $boards, 'IN');
        $query->condition('p.active_status', 'A');
        $query->orderByHeader($headers);
        $query->groupBy('p.packet_id');
        if (!empty($opts['query']['filter'])) {
            $filter_id = $opts['query']['filter'];
            $filter = pdq_ebms_get_filter_criteria($filter_id);
            $topics = array();
            foreach ($filter['topics'] as $id => $val) {
                if ($val)
                    $topics[] = $id;
            }
            switch (count($topics)) {
            case 0:
                break;
            case 1:
                $query->condition('p.topic_id', $topics[0]);
                break;
            default:
                $query->condition('p.topic_id', $topics, 'IN');
            }

            // Get review start and end dates.
            $ds = $de = '';
            if (!empty($filter['date-start']['date']))
                $ds = Ebms\Util::fill_in_date($filter['date-start']['date']);
            if (!empty($filter['date-end']['date']))
                $de = Ebms\Util::fill_in_date($filter['date-end']['date'], 1);

            $filtering_by_reviewer = false;
            foreach ($filter['reviewers'] as $id => $val) {
                if ($val) {
                    $alias = "r$id";
                    $subquery = db_select('ebms_article_review', $alias)
                        ->fields($alias, array('review_id'))
                        ->where("$alias.packet_id = p.packet_id")
                        ->condition("$alias.reviewer_id", $id);
                    if ($ds)
                        $subquery->condition("$alias.when_posted", $ds, '>=');
                    if ($de)
                        $subquery->condition("$alias.when_posted", $de, '<=');
                    $query->exists($subquery);
                    $filtering_by_reviewer = true;
                }
            }
            if (!empty($filter['packet_title_text'])) {
                $pattern = $filter['packet_title_text'];
                $query->condition('p.packet_title', $pattern, 'LIKE');
            }
            if (!$filtering_by_reviewer && ($ds || $de)) {
                $subquery = db_select('ebms_article_review', 'd')
                    ->fields('d', array('review_id'))
                    ->where('d.packet_id = p.packet_id');
                if ($ds)
                    $subquery->condition('d.when_posted', $ds, '>=');
                if ($de)
                    $subquery->condition('d.when_posted', $de, '<=');
                $query->exists($subquery);
            }
        }
        $packets = $query->execute();

        // Assemble the rows for the table.
        $rows = array();
        $opts = array('query' => drupal_get_query_parameters());
        foreach ($packets as $p) {
            $last_seen = $p->last_seen;
            $title = str_replace(' [Legacy]', '', $p->packet_title);
            $title .= " [Packet #{$p->packet_id}]";
            $query = db_select('users', 'u');
            $query->fields('u', array('name', 'uid'));
            $query->join('ebms_article_review', 'r', 'r.reviewer_id = u.uid');
            $query->condition('r.packet_id', $p->packet_id);
            $query->distinct();
            $query->orderBy('u.name');
            $results = $query->execute();
            $reviewers = array();

            foreach ($results as $result) {

                // 2014-08-01 (OCEEBMS-184): show count of unseen reviews.
                $reviewers[] = '<div>' . htmlspecialchars($result->name);
                if ($this->user->uid == $p->board_manager) {
                    $query = db_select('ebms_article_review', 'r')
                        ->condition('r.packet_id', $p->packet_id)
                        ->condition('r.reviewer_id', $result->uid);
                    if ($last_seen)
                        $query->condition('r.when_posted', $last_seen, '>');
                    $count = $query->countQuery()->execute()->fetchField();
                    if ($count)
                        $reviewers[] = " <span class='new'>$count new</span>";
                }
                $reviewers[] = '</div>';
            }

            // Added support for the board managers to flag packets with a
            // star so they can remind themselves which ones they need to come
            // back to. See https://tracker.nci.nih.gov/browse/OCEEBMS-350.
            $inner = pdq_ebms_packet_star_span($p->packet_id, $p->starred);
            $star = "<span id='packet-star-{$p->packet_id}'>$inner</span>";

            $url = EbmsReview::$url . '/reviews/' . $p->packet_id;
            $link = l($title, $url);
            $data = array(
                "$star $link",
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
            '#attributes' => array('id' => 'reviewed-packet-list'),
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
     * Builds the Drupal render array for the list of all the unreviewed
     * packets connected with the user's board(s).  If the user has explicitly
     * identified boards for which she wants to see review, show packets
     * for those boards instead.
     */
    private function unreviewed_packet_list($opts) {

        // See if the user has identified one or more boards.
        $boards = EbmsReview::get_board_ids();

        // If we don't have any boards, then there's nothing more to do.
        if (count($boards) < 1) {
            drupal_set_message('You are not linked to any boards');
            return array(
                '#type' => 'container',
                '#attributes' => array('id' => 'main-right-panel'),
                'page title' => array(
                    '#type' => 'markup',
                    '#markup' => '<h2>Unreviewed Packets</h2>',
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
                'data' => 'Assigned To',
            ),
            array(
                'data' => 'Posted',
                'field' => 'p.created_at',
                'sort' => 'desc',
            ),
        );

        // Build the query to show the packets without reviews.
        $query = db_select('ebms_packet', 'p');
        if ($this->pager != 'off')
            $query = $query->extend('PagerDefault')->limit(10);
        $query = $query->extend('TableSort');
        $query->join('ebms_topic', 't', 't.topic_id = p.topic_id');
        $query->join('ebms_board', 'b', 'b.board_id = t.board_id');
        $query->leftJoin('ebms_article_review', 'r',
                         'r.packet_id = p.packet_id');
        $query->isNull('r.packet_id');
        $query->fields('p', array('packet_id', 'packet_title', 'created_at'));
        $query->condition('b.board_id', $boards, 'IN');
        $query->condition('p.active_status', 'A');
        $query->orderByHeader($headers);
        // $query->groupBy('p.packet_id');
        if (!empty($opts['query']['filter'])) {
            $filter_id = $opts['query']['filter'];
            $filter = pdq_ebms_get_filter_criteria($filter_id);
            $topics = array();
            foreach ($filter['topics'] as $id => $val) {
                if ($val)
                    $topics[] = $id;
            }
            switch (count($topics)) {
            case 0:
                break;
            case 1:
                $query->condition('p.topic_id', $topics[0]);
                break;
            default:
                $query->condition('p.topic_id', $topics, 'IN');
            }

            // Get packet creation start and end dates.
            if (!empty($filter['date-start']['date'])) {
                $ds = Ebms\Util::fill_in_date($filter['date-start']['date']);
                if ($ds)
                    $query->condition('p.created_at', $ds, '>=');
            }
            if (!empty($filter['date-end']['date'])) {
                $de = Ebms\Util::fill_in_date($filter['date-end']['date'], 1);
                if ($de)
                    $query->condition('p.created_at', $de, '<=');
            }

            // Narrow by assigned reviewer(s).
            $assigned = array();
            foreach ($filter['reviewers'] as $id => $val) {
                if ($val)
                    $assigned[] = $id;
            }
            if (count($assigned) > 0) {
                $query->join('ebms_packet_reviewer', 'u',
                             'u.packet_id = p.packet_id');
                if (count($assigned) > 1)
                    $query->condition('u.reviewer_id', $assigned, 'IN');
                else
                    $query->condition('u.reviewer_id', $assigned[0]);
            }
            if (!empty($filter['packet_title_text'])) {
                $pattern = $filter['packet_title_text'];
                $query->condition('p.packet_title', $pattern, 'LIKE');
            }
        }
        $packets = $query->execute();

        // Assemble the rows for the table.
        $rows = array();
        $opts = array('query' => drupal_get_query_parameters());
        foreach ($packets as $p) {
            $title = str_replace(' [Legacy]', '', $p->packet_title);
            $title .= " [Packet #{$p->packet_id}]";
            $query = db_select('users', 'u');
            $query->fields('u', array('name', 'uid'));
            $query->join('ebms_packet_reviewer', 'r', 'r.reviewer_id = u.uid');
            $query->condition('r.packet_id', $p->packet_id);
            $query->distinct();
            $query->orderBy('u.name');
            $results = $query->execute();
            $reviewers = array();
            foreach ($results as $result) {
                $name = htmlspecialchars($result->name);
                $reviewers[] = "<div>$name</div>";
            }

            $url = EbmsReview::$url . '/unreviewed/' . $p->packet_id;
            $link = l($title, $url);
            $data = array(
                $link,
                array(
                    'data' => implode('', $reviewers),
                    'class' => array('reviewers'),
                ),
                date("m/d/Y", strtotime($p->created_at)),
            );
            $rows[] = array(
                'data' => $data,
                'no_striping' => true,
            );
        }

        // Assemble the render array for the table and return it.
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
            '#attributes' => array('id' => 'reviewed-packet-list'),
            'title' => array(
                '#type' => 'markup',
                '#markup' => '<h2>Unreviewed Packets</h2>',
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
     *
     * The name of this method is now a little misleading, as with
     * OCEEBMS-550 it's now also used by the Unreviewed Packets pages.
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
        $title = htmlspecialchars(Ebms\Util::get_text($article->ArticleTitle));
        $opts = array();
        if (!$narrow)
            $opts['attributes']['target'] = '_blank';
        $title = l($title, 'citations/full/' . $article_id, $opts);
        $pgn = htmlspecialchars($article->Pagination->MedlinePgn);
        $year = $article->Journal->JournalIssue->PubDate->Year;
        $issue = htmlspecialchars($article->Journal->JournalIssue->Issue);
        $vol = htmlspecialchars($article->Journal->JournalIssue->Volume);
        $authors = array();
        if ($doc->MedlineCitation->Article->AuthorList->Author) {
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
        }
        else
            $authors[] = '[No authors listed]';
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
     * Assemble list of topic-specific tags for an article, to be
     * displayed on the Reviewed Packet page.
     */
    private static function get_topic_tags($article_id, $topic_id) {
        $tags = Ebms\Util::get_topic_tags($article_id, $topic_id);
        return htmlspecialchars(implode('; ', $tags));
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
     * turn off that suppression.)
     */
    private function show_reviewed_packet($opts) {

        // We'll need this for finding the latest article statuses.
        $full_review_sequence = db_select('ebms_article_state_type', 't')
            ->fields('t', array('sequence'))
            ->condition('t.state_text_id', 'PassedFullReview')
            ->execute()
            ->fetchField();

        // We'll use this a lot.
        $packet_id = $this->packet_id;

        // We'll have at least one use for the current date/time.
        $now = strftime("%Y-%m-%d %H:%M:%S");

        // If the user asked to suppress one of the articles in the
        // packet, update the table to reflect this request.
        if (isset($opts['query']['archive'])) {
            $article_id = $opts['query']['archive'];
            if ($article_id) {
                db_update('ebms_packet_article')
                    ->fields(array('archived' => $now))
                    ->condition('article_id', $article_id)
                    ->condition('packet_id', $packet_id)
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
                    ->condition('packet_id', $packet_id)
                    ->execute();
            }

            // See note above about clearing the 'archive' parameter.
            unset($_GET['revive']);
            unset($opts['query']['revive']);
        }

        // Manual breadcrumb setting to match requirements.
        $url = EbmsReview::$url . '/reviews/' . $packet_id;
        drupal_set_breadcrumb(
            array(
                l('Literature', EbmsReview::$url),
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
        $query->leftJoin('ebms_article_review', 'r',
            "r.article_id = a.article_id AND r.packet_id = $packet_id");
        //$query->leftJoin('ebms_review_disposition', 'd',
        //    'd.review_id = r.review_id');
        $query->leftJoin('file_managed', 'f', 'f.fid = a.full_text_id');
        $query->fields('a', array('article_id', 'article_title', 'source_id'));
        $query->fields('p', array('archived'));
        $query->fields('f', array('uri'));
        $query->addExpression("MAX(IFNULL(r.when_posted, ''))", 'updated');
        $query->condition('p.packet_id', $packet_id);
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
        $query->condition('r.packet_id', $packet_id);
        $query->orderBy('u.name');
        $results = $query->execute();
        foreach ($results as $result)
            $packet_reviewers[] = $result->name;

        // Build up the sequence of table rows.
        $rows = array();
        $target = array('attributes' => array('target' => '_blank'));
        foreach ($articles as $article) {

            // Get the base string for the article citation display.
            $article_id = $article->article_id;
            $citation = $this->get_reviewed_citation($article_id, false);

            // Index for walking through the $packet_reviewers list.
            $i = 0;

            // Collect information about what each reviewer said about the
            // article.
            $query = db_select('users', 'u');
            $query->fields('u', array('name'));
            $query->join('ebms_article_review', 'r', 'r.reviewer_id = u.uid');
            $query->fields('r', array('review_id'));
            $query->join('ebms_review_disposition', 'd',
                'd.review_id = r.review_id');
            $query->condition('r.packet_id', $packet_id);
            $query->condition('r.article_id', $article_id);
            $query->orderBy('u.name');
            $query->distinct();
            $results = $query->execute();
            $reviewers = array();

            // Add related articles and board manager comment (OCEEBMS-381).
            // Also most recent status and topic-associated tags (OCEEBMS-405).
            // Also highlight high-priority article/topic combos (OCEEBMS-478).
            $related = Ebms\Util::get_related_articles($article_id);
            if ($related)
                $citation .= "<br>$related";
            $topic_id = db_select('ebms_packet', 'p')
                ->fields('p', array('topic_id'))
                ->condition('p.packet_id', $packet_id)
                ->execute()
                ->fetchField();
            $topic_tags = EbmsReview::get_topic_tags($article_id, $topic_id);
            if ($topic_tags)
                $citation .= "<br>[ARTICLE TAG(S): $topic_tags]";
            $comment = EbmsReview::get_article_topic_comment($article_id,
                                                             $topic_id);
            if (strpos($topic_tags, 'High priority') !== false) {
                $citation .= '<br><br><span class="hi-priority">';
                $citation .= 'HIGH PRIORITY</span>';
            }
            if ($comment) {
                $citation .= '<br><br><span class="bmc">BOARD MANAGER COMMENT';
                $citation .= "<br>$comment</span>";
            }
            $latest_status = Ebms\Util::get_latest_status($article_id,
                             $topic_id, $full_review_sequence);
            if ($latest_status)
                $citation .= '<br><br><span class="mrs">ARTICLE STATE<br>' .
                    $latest_status . '</span>';

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
            $is_fyi = EbmsReview::is_fyi($article_id, $topic_id);
            $fyi_span = '<span class="fyi" style="float: right">FYI</span>';
            while ($i < count($packet_reviewers)) {
                $fyi = ($i == 0 && $is_fyi) ? $fyi_span : '';
                $reviewers[] = '<div class="reviewer">' .
                    '<span class="reviewer-name">' .
                    htmlspecialchars($packet_reviewers[$i++]) .
                    "</span>$fyi</div>";
            }

            // Assemble the row.
            $article_opts = $opts;
            $action = $article->archived ? 'revive' : 'archive';
            $article_opts['query'][$action] = $article_id;
            $href = url($url, $article_opts);
            $type = 'type="checkbox"';
            $onclick = 'onclick=\'location.href="' . $href . '"\'';
            $checked = $article->archived ? ' checked="checked"' : '';
            $checkbox = "<input $type $onclick$checked />";
            $pm_url = Ebms\Util::pubmed_url($article->source_id);
            $links = '<div class="links">' .
                l('View Abstract', $pm_url, $target);
            if ($article->uri) {
                $f_url = file_create_url($article->uri);
                $links .= ' | ' . l('Download Full Text', $f_url, $target);
            }
            $a_url = EbmsReview::$url . '/reviews/' . $packet_id . '/' .
                $article_id;
            $links .= ' | ' . l('Show Details', $a_url) . '</div>';
            $updated = '';
            if ($article->updated)
                $updated = date("m/d/Y", strtotime($article->updated));
            $data = array(
                $citation . $links,
                implode('<br />', $reviewers),
                $updated,
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
        // 2014-08-08 (OCEEBMS-184): Robin changed her mind. She now wants
        // us to remember when the manager for the packet's board saw
        // the reviews, not when the packet creator saw them.
        $query = db_select('ebms_board', 'b');
        $query->fields('b', array('board_manager'));
        $query->join('ebms_topic', 't', 't.board_id = b.board_id');
        $query->join('ebms_packet', 'p', 'p.topic_id = t.topic_id');
        $query->condition('p.packet_id', $packet_id);
        $board_manager = $query->execute()->fetchField();
        if ($board_manager == $this->user->uid)
            db_update('ebms_packet')
                ->fields(array('last_seen' => $now))
                ->condition('packet_id', $packet_id)
                ->execute();

        // Put together and return the render array for the table.
        $attrs = array(
            'attributes' => array('class' => array('button')),
            'query' => array('packet' => $packet_id),
        );
        $report_button = l('Create Report', 'reports/lit-review', $attrs);
        $main_url = EbmsReview::$url;
        $del_func = 'javascript:ebmsscript.delete_packet';
        $opts = array('query' => drupal_get_query_parameters());
        $opts['query']['target'] = 'reviews';
        $del_parm = url("$main_url/delete/$packet_id", $opts);
        $del_url = "$del_func('$del_parm');";
        $del_button = '<a href="' . $del_url .
            '" class="button">Archive Packet</a>';
        $buttons = "$report_button &nbsp; $del_button";
        $page = array(
            'title' => array(
                '#type' => 'markup',
                '#markup' => '<h2>' . htmlspecialchars($this->packet_title) .
                '</h2>',
            ),
            'buttons' => array(
                '#type' => 'markup',
                '#markup' => $buttons,
                '#prefix' => '<div id="buttons">',
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
            'delete-confirm' => array(
                '#type' => 'container',
                '#attributes' => array(
                    'id' => 'confirm-packet-delete',
                    'title' => 'Archive Packet?',
                ),
                'confirm-message' => array(
                    '#type' => 'markup',
                    '#markup' =>
                    '<p>Archiving this packet will remove it from the queues'
                     . ' of any reviewers who have not responded.</p>'
                ),
            ),
        );
        $reviewer_files = $this->get_reviewer_files();
        if ($reviewer_files)
            $page['reviewer files'] = $reviewer_files;
        return $page;
    }

    /**
     * The user (a board manager) has selected a specific packet from the
     * list of unreviewed packets. Show the articles in the packet, with
     * a checkbox allowing the board manager to suppress ("archive") the
     * article from the packet (or to turn off that suppression.)
     */
    private function show_unreviewed_packet($opts) {

        // We'll need this for finding the latest article statuses.
        $full_review_sequence = db_select('ebms_article_state_type', 't')
            ->fields('t', array('sequence'))
            ->condition('t.state_text_id', 'PassedFullReview')
            ->execute()
            ->fetchField();

        // We'll use this a lot.
        $packet_id = $this->packet_id;

        // Get the date the packet was created.
        $created = db_select('ebms_packet', 'p')
            ->fields('p', array('created_at'))
            ->condition('p.packet_id', $packet_id)
            ->execute()
            ->fetchField();
        $created = date('Y-m-d', strtotime($created));

        // We'll have at least one use for the current date/time.
        $now = strftime("%Y-%m-%d %H:%M:%S");

        // If the user asked to suppress one of the articles in the
        // packet, update the table to reflect this request.
        if (isset($opts['query']['archive'])) {
            $article_id = $opts['query']['archive'];
            if ($article_id) {
                db_update('ebms_packet_article')
                    ->fields(array('archived' => $now))
                    ->condition('article_id', $article_id)
                    ->condition('packet_id', $packet_id)
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
                    ->condition('packet_id', $packet_id)
                    ->execute();
            }

            // See note above about clearing the 'archive' parameter.
            unset($_GET['revive']);
            unset($opts['query']['revive']);
        }

        // Manual breadcrumb setting to match requirements.
        $url = EbmsReview::$url . '/unreviewed/' . $packet_id;
        drupal_set_breadcrumb(
            array(
                l('Literature', EbmsReview::$url),
                l('Unreviewed Packets', EbmsReview::$url . '/unreviewed'),
                $this->packet_title,
            )
        );

        // Handle some of the other page state settings.
        if ($this->pager != 'off')
            $pager = array('#theme' => 'pager');
        else {
            unset($opts['query']['pager']);
            $link = l('VIEW PAGES', EbmsReview::$url . '/unreviewed', $opts);
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
                'data' => l('Archive', $url, $archive_opts),
                'class' => array('col-4'),
            ),
        );

        // Construct the query to get the articles in the packet.
        $query = db_select('ebms_packet_article', 'p');
        $query = $query->extend('TableSort');
        $query->join('ebms_article', 'a', 'a.article_id = p.article_id');
        $query->leftJoin('file_managed', 'f', 'f.fid = a.full_text_id');
        $query->fields('a', array('article_id', 'article_title', 'source_id'));
        $query->fields('p', array('archived'));
        $query->fields('f', array('uri'));
        $query->condition('p.packet_id', $packet_id);
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
        $query->condition('r.packet_id', $packet_id);
        $query->orderBy('u.name');
        $results = $query->execute();
        foreach ($results as $result)
            $packet_reviewers[] = $result->name;

        // Build up the sequence of table rows.
        $rows = array();
        $target = array('attributes' => array('target' => '_blank'));
        foreach ($articles as $article) {

            // Get the base string for the article citation display.
            // Yes, the name of the method is inappropriate. But it works. :-)
            $article_id = $article->article_id;
            $citation = $this->get_reviewed_citation($article_id, false);

            // Add related articles and board manager comment (OCEEBMS-381).
            // Also most recent status and topic-associated tags (OCEEBMS-405).
            // Also highlight high-priority article/topic combos (OCEEBMS-478).
            $related = Ebms\Util::get_related_articles($article_id);
            if ($related)
                $citation .= "<br>$related";
            $topic_id = db_select('ebms_packet', 'p')
                ->fields('p', array('topic_id'))
                ->condition('p.packet_id', $packet_id)
                ->execute()
                ->fetchField();
            $topic_tags = EbmsReview::get_topic_tags($article_id, $topic_id);
            if ($topic_tags)
                $citation .= "<br>[ARTICLE TAG(S): $topic_tags]";
            $comment = EbmsReview::get_article_topic_comment($article_id,
                                                             $topic_id);
            if (strpos($topic_tags, 'High priority') !== false) {
                $citation .= '<br><br><span class="hi-priority">';
                $citation .= 'HIGH PRIORITY</span>';
            }
            if ($comment) {
                $citation .= '<br><br><span class="bmc">BOARD MANAGER COMMENT';
                $citation .= "<br>$comment</span>";
            }
            $latest_status = Ebms\Util::get_latest_status($article_id,
                             $topic_id, $full_review_sequence);
            if ($latest_status)
                $citation .= '<br><br><span class="mrs">ARTICLE STATE<br>' .
                    $latest_status . '</span>';

            // Assemble the row.
            $article_opts = $opts;
            $action = $article->archived ? 'revive' : 'archive';
            $article_opts['query'][$action] = $article_id;
            $href = url($url, $article_opts);
            $type = 'type="checkbox"';
            $onclick = 'onclick=\'location.href="' . $href . '"\'';
            $checked = $article->archived ? ' checked="checked"' : '';
            $checkbox = "<input $type $onclick$checked />";
            $pm_url = Ebms\Util::pubmed_url($article->source_id);
            $links = '<div class="links">' .
                l('View Abstract', $pm_url, $target);
            if ($article->uri) {
                $f_url = file_create_url($article->uri);
                $links .= ' | ' . l('Download Full Text', $f_url, $target);
            }
            $a_url = EbmsReview::$url . '/reviews/' . $packet_id . '/' .
                $article_id;
            $links .= '</div>';
            $data = array(
                $citation . $links,
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

        // Put together and return the render array for the table.
        $main_url = EbmsReview::$url;
        $del_func = 'javascript:ebmsscript.delete_packet';
        $opts = array('query' => drupal_get_query_parameters());
        $opts['query']['target'] = 'unreviewed';
        $del_parm = url("$main_url/delete/$packet_id", $opts);
        $del_url = "$del_func('$del_parm');";
        $del_button = '<a href="' . $del_url .
            '" class="button">Archive Packet</a>';
        $page = array(
            'title' => array(
                '#type' => 'markup',
                '#markup' => '<h2>' . htmlspecialchars($this->packet_title) .
                '</h2>'
                . '<p><i><b>Assigned ' . $created . ' for review to: </b>'
                . implode(', ', $packet_reviewers)
                . '</i></p>'
                ,
            ),
            'buttons' => array(
                '#type' => 'markup',
                '#markup' => $del_button, // $buttons,
                '#prefix' => '<div id="buttons">',
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
            'delete-confirm' => array(
                '#type' => 'container',
                '#attributes' => array(
                    'id' => 'confirm-packet-delete',
                    'title' => 'Archive Packet?',
                ),
                'confirm-message' => array(
                    '#type' => 'markup',
                    '#markup' =>
                    '<p>Archiving this packet will remove it from the queues'
                     . ' of the assigned reviewers.</p>'
                ),
            ),
        );
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
                $markup[] = '<p>' .
                    filter_xss($result->comments, array('br', 'i')) . '</p>';
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
        $target = array('attributes' => array('target' => '_blank'));
        $pmid = db_select('ebms_article', 'a')
            ->fields('a', array('source_id'))
            ->condition('a.article_id', $this->article_id)
            ->execute()
            ->fetchField();
        $pm_url = Ebms\Util::pubmed_url($pmid);
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
     * Build the page for showing the board member a packet which has
     * been archived, or for which she has already submitted at least
     * one review, or both.  OCEEBMS-95.
     * 2013-12-06: users asked for extensive changes to the requirements
     * for this page.  It's now exclusively used to show fully completed
     * (by this reviewer) packets.  See Robin's comment posted to the
     * JIRA ticket 2013-12-05 15:20.
     *
     * More new requirements; the original requirements said "Board members
     * should have read-only access to their past review decisions ..."
     * but Robin just added (2014-01-09 15:21): "Please give Board members
     * visibility to other reviewers' decisions in their completed packets."
     * (Still OCEEBMS-95).
     *
     *  @return  array    Drupal render array for archived packet page
     */
    private function member_completed_packet() {

        // Get the basic information for the packet.
        $packet_info = db_select('ebms_packet', 'p')
            ->fields('p', array('packet_title', 'topic_id'))
            ->condition('p.packet_id', $this->packet_id)
            ->execute()
            ->fetch();
        $title = htmlspecialchars($packet_info->packet_title);
        $topic_id = $packet_info->topic_id;

        // Override the default breadcrumbs.
        drupal_set_breadcrumb(
            array(
                l('Literature', EbmsReview::$url),
                l('COMPLETED PACKETS', EbmsReview::$url . '/completed'),
                $title,
            )
        );

        // Start building the render array.
        $body = array(
            'title' => array(
                '#markup' => "<h2>$title</h2>",
            ),
        );

        // Add links to the summaries posted by the board manager for the
        // packet.
        $body['summaries']['heading'] = array(
            '#type' => 'markup',
            '#markup' => "<h3>Summary Documents</h3>",
        );
        $summaries = $this->fetch_packet_summaries();
        $links = array();
        foreach ($summaries as $uri => $description)
            $links[] = l($description, file_create_url($uri));
        $body['summaries']['links'] = array(
            '#prefix' => '<div id="summary-links">',
            '#suffix' => '</div>',
            '#items' => $links,
            '#theme' => 'item_list',
        );

        // Fetch the articles in the packet.
        $body['articles']['heading'] = array(
            '#type' => 'markup',
            '#markup' => "<h3>Articles</h3>",
        );
        $query = db_select('ebms_article', 'a');
        $query->join('ebms_packet_article', 'p',
            'p.article_id = a.article_id');
        $query->condition('p.packet_id', $this->packet_id);
        $query->fields('a', array('article_id'));
        $query->fields('p', array('drop_flag', 'archived'));
        $query->leftJoin('ebms_article_author_cite', 'sort_auth_cite',
            'a.article_id = sort_auth_cite.article_id AND ' .
            'sort_auth_cite.cite_order = 1');
        $query->leftJoin('ebms_article_author', 'sort_auth',
            'sort_auth.author_id = sort_auth_cite.author_id');
        $query->orderBy('sort_auth.last_name');
        $query->orderBy('sort_auth.initials');
        $query->orderBy('sort_auth.collective_name');
        $query->orderBy('a.article_title');
        $articles = $query->execute();

        // Show each article, along with the board member's review (if any).
        global $user;
        foreach ($articles as $article) {
            $article_id = $article->article_id;
            $citation = EbmsReview::show_citation($article_id, $topic_id, 3, true);
            $block = array();
            $block['citation'] = array(
                '#markup' => '<p>' . $citation . '</p>',
            );
            if ($article->drop_flag)
                $block['dropped'] = array(
                    '#markup' =>
                    '<p>This article was removed from the packet ' .
                    'by your Board Manager.</p>',
                );
            $query = db_select('ebms_article_review', 'r');
            $query->join('users', 'u', 'u.uid = r.reviewer_id');
            $query->condition('r.packet_id', $this->packet_id);
            $query->condition('r.article_id', $article_id);
            $query->orderBy('r.when_posted');
            $query->fields('u', array('name'));
            $query->fields('r', array('review_id', 'when_posted', 'comments',
                    'loe_info'));
            $results = $query->execute();
            $reviews = array();
            $reviewers = array();
            foreach ($results as $result) {
                $reviewer = htmlspecialchars($result->name);
                if (!in_array($reviewer, $reviewers))
                    $reviewers[] = $reviewer;
                $reviews[] = $result;
            }
            sort($reviewers);
            $reviewers = implode(', ', $reviewers);
            # Suppressed; see 2014-01-10 comments for OCEEBMS-95.
            #$block['reviewers'] = array(
            #    '#markup' => "<p>Reviewed by $reviewers.</p>",
            #);
            foreach ($reviews as $review) {
                $review_key = "review-$review->review_id";
                $block[$review_key] = array();
                $block[$review_key]['review-header'] = array(
                    '#markup' =>
                    '<p class="archived-review-header">Review posted ' .
                    $review->when_posted . ' by ' .
                    htmlspecialchars($review->name) . '</p>',
                );
                $query = db_select('ebms_review_disposition', 'd');
                $query->join('ebms_review_disposition_value', 'v',
                    'v.value_id = d.value_id');
                $query->fields('v', array('value_name'));
                $query->condition('d.review_id', $review->review_id);
                $query->orderBy('v.value_pos');
                $dispositions = $query->execute()->fetchCol();
                if (count($dispositions) > 0) {
                    $block[$review_key]['dispositions'] = array(
                        '#theme' => 'item_list',
                        '#items' => $dispositions,
                        '#prefix' => '<p><b>REVIEW DISPOSITION(S)</b>:<br>',
                        '#suffix' => '</p>',
                    );
                }
                $query = db_select('ebms_review_rejection_reason', 'r');
                $query->join('ebms_review_rejection_value', 'v',
                    'v.value_id = r.value_id');
                $query->fields('v', array('value_name'));
                $query->condition('r.review_id', $review->review_id);
                $query->orderBy('v.value_pos');
                $reasons = $query->execute()->fetchCol();
                if (count($reasons) > 0) {
                    $block[$review_key]['reasons'] = array(
                        '#theme' => 'item_list',
                        '#items' => $reasons,
                        '#prefix' => '<p><b>REJECTION REASON(S):</b><br>',
                        '#suffix' => '</p>',
                    );
                }
                if ($review->comments)
                    $block[$review_key]['comments'] = array(
                        '#markup' =>
                        '<p><b>COMMENTS: </b>' .
                        htmlspecialchars($review->comments) . '</p>',
                    );
                if ($review->loe_info)
                    $block[$review_key]['loe'] = array(
                        '#markup' =>
                        '<p><b>LOE INFO:</b> ' .
                        htmlspecialchars($review->loe_info) . '</p>',
                    );
            }
            $block['divider'] = array(
                '#markup' => '<hr>',
            );
            $body['articles']["article-$article_id"] = $block;
        }

        // Show the docs posted by board members, with a button to post more.
        $opts = array(
            'attributes' => array(
                'class' => array('button float-right pad-top-10'),
            ),
        );
        $url = EbmsReview::$url . "/archpost/$this->packet_id";
        $body['uploads']['heading'] = array(
            '#type' => 'markup',
            '#markup' => '<div class="clearfix">' .
            '<h3 class="float-left">Reviewer Uploads</h3>' .
            l('POST DOCUMENT', $url, $opts) . '</div>',
        );
        $query = db_select('file_managed', 'f');
        $query->join('ebms_reviewer_doc', 'd', 'd.file_id = f.fid');
        $query->join('users', 'u', 'u.uid = d.reviewer_id');
        $query->fields('f', array('uri', 'filename'));
        $query->fields('u', array('name'));
        $query->fields('d', array('when_posted', 'description'));
        $query->condition('d.packet_id', $this->packet_id);

        // Robin asked (2013-12-05) that we eliminate this test.
        // $query->condition('d.drop_flag', 0);
        $query->orderBy('f.filename');

        $headers = array(
            'FILE NAME',
            'NOTES',
            'UPLOADED BY',
            'WHEN POSTED',
        );
        $result = $query->execute();
        $docs = array();
        foreach ($result as $record)
            $docs[] = $record;
        $rows = array();

        // Create new options array for document links.
        $doc_opts = array(
            'attributes' => array(
                'target' => '_blank',
                'title' => 'Click to download/open document',
            ),
        );
        foreach ($docs as $doc) {
            $url = file_create_url($doc->uri);
            $posted = date("Y-m-d", strtotime($doc->when_posted));
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
                #    'no_striping' => true,
            );
        }
        $body['docs'] = array(
            '#attributes' => array(
                'id' => 'reviewer-uploads-table',
                'class' => array('clearfix'),
            ),
            '#theme' => 'table',
            '#rows' => $rows,
            '#header' => $headers,
        );
        return $body;
    }

    private function member_fyi_packet() {

        // Get the basic information for the packet.
        $packet = db_select('ebms_packet', 'p')
            ->fields('p', array('packet_title', 'topic_id'))
            ->condition('p.packet_id', $this->packet_id)
            ->execute()
            ->fetch();
        $title = htmlspecialchars($packet->packet_title);
        $topic_id = $packet->topic_id;

        // Override the default breadcrumbs.
        drupal_set_breadcrumb(
            array(
                l('Literature', EbmsReview::$url),
                l('FYI PACKETS', EbmsReview::$url . '/fyi'),
                $title,
            )
        );

        // Start building the render array.
        $body = array(
            'title' => array(
                '#markup' => "<h2>$title [FYI]</h2>",
            ),
        );

        // Add links to the summaries posted by the board manager for the
        // packet.
        $body['summaries']['heading'] = array(
            '#type' => 'markup',
            '#markup' => "<h3>Summary Documents</h3>",
        );
        $summaries = $this->fetch_packet_summaries();
        $links = array();
        foreach ($summaries as $uri => $description)
            $links[] = l($description, file_create_url($uri));
        $body['summaries']['links'] = array(
            '#prefix' => '<div id="summary-links">',
            '#suffix' => '</div>',
            '#items' => $links,
            '#theme' => 'item_list',
        );

        // Fetch the articles in the packet.
        $body['articles']['heading'] = array(
            '#type' => 'markup',
            '#markup' => "<h3>Articles</h3>",
        );
        $query = db_select('ebms_article', 'a');
        $query->join('ebms_packet_article', 'p',
            'p.article_id = a.article_id');
        $query->condition('p.packet_id', $this->packet_id);
        $query->fields('a', array('article_id'));
        $query->fields('p', array('drop_flag', 'archived'));
        $query->leftJoin('ebms_article_author_cite', 'sort_auth_cite',
            'a.article_id = sort_auth_cite.article_id AND ' .
            'sort_auth_cite.cite_order = 1');
        $query->leftJoin('ebms_article_author', 'sort_auth',
            'sort_auth.author_id = sort_auth_cite.author_id');
        $query->orderBy('sort_auth.last_name');
        $query->orderBy('sort_auth.initials');
        $query->orderBy('sort_auth.collective_name');
        $query->orderBy('a.article_title');
        $articles = $query->execute();

        // Show each article, along with the board member's review (if any).
        global $user;
        foreach ($articles as $article) {
            $article_id = $article->article_id;
            $citation = EbmsReview::show_citation($article_id, $topic_id, 3, true);
            $block = array();
            $block['citation'] = array(
                '#markup' => '<p>' . $citation . '</p>',
            );
            if ($article->drop_flag)
                $block['dropped'] = array(
                    '#markup' =>
                    '<p>This article was removed from the packet ' .
                    'by your Board Manager.</p>',
                );
            $query = db_select('ebms_article_review', 'r');
            $query->join('users', 'u', 'u.uid = r.reviewer_id');
            $query->condition('r.packet_id', $this->packet_id);
            $query->condition('r.article_id', $article_id);
            $query->orderBy('r.when_posted');
            $query->fields('u', array('name'));
            $query->fields('r', array('review_id', 'when_posted', 'comments',
                    'loe_info'));
            $results = $query->execute();
            $reviews = array();
            $reviewers = array();
            foreach ($results as $result) {
                $reviewer = htmlspecialchars($result->name);
                if (!in_array($reviewer, $reviewers))
                    $reviewers[] = $reviewer;
                $reviews[] = $result;
            }
            sort($reviewers);
            $reviewers = implode(', ', $reviewers);
            # Suppressed; see 2014-01-10 comments for OCEEBMS-95.
            #$block['reviewers'] = array(
            #    '#markup' => "<p>Reviewed by $reviewers.</p>",
            #);
            foreach ($reviews as $review) {
                $review_key = "review-$review->review_id";
                $block[$review_key] = array();
                $block[$review_key]['review-header'] = array(
                    '#markup' =>
                    '<p class="archived-review-header">Review posted ' .
                    $review->when_posted . ' by ' .
                    htmlspecialchars($review->name) . '</p>',
                );
                $query = db_select('ebms_review_disposition', 'd');
                $query->join('ebms_review_disposition_value', 'v',
                    'v.value_id = d.value_id');
                $query->fields('v', array('value_name'));
                $query->condition('d.review_id', $review->review_id);
                $query->orderBy('v.value_pos');
                $dispositions = $query->execute()->fetchCol();
                if (count($dispositions) > 0) {
                    $block[$review_key]['dispositions'] = array(
                        '#theme' => 'item_list',
                        '#items' => $dispositions,
                        '#prefix' => '<p><b>REVIEW DISPOSITION(S)</b>:<br>',
                        '#suffix' => '</p>',
                    );
                }
                $query = db_select('ebms_review_rejection_reason', 'r');
                $query->join('ebms_review_rejection_value', 'v',
                    'v.value_id = r.value_id');
                $query->fields('v', array('value_name'));
                $query->condition('r.review_id', $review->review_id);
                $query->orderBy('v.value_pos');
                $reasons = $query->execute()->fetchCol();
                if (count($reasons) > 0) {
                    $block[$review_key]['reasons'] = array(
                        '#theme' => 'item_list',
                        '#items' => $reasons,
                        '#prefix' => '<p><b>REJECTION REASON(S):</b><br>',
                        '#suffix' => '</p>',
                    );
                }
                if ($review->comments)
                    $block[$review_key]['comments'] = array(
                        '#markup' =>
                        '<p><b>COMMENTS: </b>' .
                        htmlspecialchars($review->comments) . '</p>',
                    );
                if ($review->loe_info)
                    $block[$review_key]['loe'] = array(
                        '#markup' =>
                        '<p><b>LOE INFO:</b> ' .
                        htmlspecialchars($review->loe_info) . '</p>',
                    );
            }
            $block['divider'] = array(
                '#markup' => '<hr>',
            );
            $body['articles']["article-$article_id"] = $block;
        }

        // Show the docs posted by board members, with a button to post more.
        $opts = array(
            'attributes' => array(
                'class' => array('button float-right pad-top-10'),
            ),
        );
        $url = EbmsReview::$url . "/archpost/$this->packet_id";
        $body['uploads']['heading'] = array(
            '#type' => 'markup',
            '#markup' => '<div class="clearfix">' .
            '<h3 class="float-left">Reviewer Uploads</h3>' .
            //l('POST DOCUMENT', $url, $opts) .
            '</div>',
        );
        $query = db_select('file_managed', 'f');
        $query->join('ebms_reviewer_doc', 'd', 'd.file_id = f.fid');
        $query->join('users', 'u', 'u.uid = d.reviewer_id');
        $query->fields('f', array('uri', 'filename'));
        $query->fields('u', array('name'));
        $query->fields('d', array('when_posted', 'description'));
        $query->condition('d.packet_id', $this->packet_id);

        // Robin asked (2013-12-05) that we eliminate this test.
        // $query->condition('d.drop_flag', 0);
        $query->orderBy('f.filename');

        $headers = array(
            'FILE NAME',
            'NOTES',
            'UPLOADED BY',
            'WHEN POSTED',
        );
        $result = $query->execute();
        $docs = array();
        foreach ($result as $record)
            $docs[] = $record;
        $rows = array();

        // Create new options array for document links.
        $doc_opts = array(
            'attributes' => array(
                'target' => '_blank',
                'title' => 'Click to download/open document',
            ),
        );
        foreach ($docs as $doc) {
            $url = file_create_url($doc->uri);
            $posted = date("Y-m-d", strtotime($doc->when_posted));
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
                #    'no_striping' => true,
            );
        }
        $body['docs'] = array(
            '#attributes' => array(
                'id' => 'reviewer-uploads-table',
                'class' => array('clearfix'),
            ),
            '#theme' => 'table',
            '#rows' => $rows,
            '#header' => $headers,
        );
        return $body;
    }

    /**
     * Has the current user reviewed all the articles in the packet?
     * Ignore drop flag.  Called by member_completed() method.
     *
     * More new requirements (2016-01-13 OCEEBMS-232): ignore FYI articles
     * when calculating whether a packet has been completed.
     *
     * Regard articles with states beyond FYI and siblings as "complete"
     * (OCEEBMS-450).
     */
    public static function packet_fully_completed($packet_id) {
        global $user;
        $fyi = db_select('ebms_article_state_type', 't')
            ->condition('t.state_text_id', 'FYI')
            ->fields('t', array('state_id', 'sequence'))
            ->execute()
            ->fetch();
        $query = db_select('ebms_packet_article', 'a');
        $query->join('ebms_packet', 'p', 'p.packet_id = a.packet_id');
        $query->join('ebms_article_state', 's',
            's.article_id = a.article_id AND s.topic_id = p.topic_id');
        $query->join('ebms_article_state_type', 'st',
                     'st.state_id = s.state_id');
        $query->leftJoin('ebms_article_review', 'r',
            'r.article_id = a.article_id AND ' .
            'r.packet_id = a.packet_id AND ' .
            'r.reviewer_id = ' . $user->uid);
        $query->condition('a.packet_id', $packet_id);
        $query->condition('s.current', 'Y');
        $query->condition('s.state_id', $fyi->state_id, '<>');
        $query->condition('st.sequence', $fyi->sequence, '<=');
        $query->isNull('r.article_id');
        $count = $query->countQuery()->execute()->fetchField();
        return $count == 0;
    }

    /**
     * Build the Drupal render array for the page showing the board member
     * packets to which she has been assigned, and which have been archived,
     * or have at least one article which she has already reviewed, or both.
     * See JIRA ticket OCEEBMS-95.
     * 2013-12-06: Users asked for extensive changes to the requirements
     * for this page.  See Robin's 2013-12-05 15:20 comment posted to the
     * JIRA ticket.
     *
     *  @return  array     Drupal render array for the page
     */
    private function member_completed() {

        // Override the default breadcrumbs.
        drupal_set_breadcrumb(
            array(
                l('Literature', EbmsReview::$url),
                'COMPLETED PACKETS',
            )
        );

        // Start building the render array.
        $body = array(
            'title' => array(
                '#markup' => '<h2>COMPLETED PACKETS</h2>',
            ),
        );

        // Make two of the columns sortable.
        $headers = array(
            array(
                'data' => 'Packet Name',
                'field' => 'p.packet_title',
            ),
            array(
                'data' => 'Date Completed',
                'field' => 'last_review',
                'sort' => 'desc',
            ),
            'Number of Articles',
        );

        // Find the relevant packets.  Drupal's DB API isn't capable of
        // handling an expression in the havingCondition() method, so
        // we get the value in a separate query.
        $two_years_ago = db_query('SELECT DATE_SUB(NOW(), INTERVAL 2 YEAR)')
            ->fetchField();
        global $user;
        $rows = array();
        $query = db_select('ebms_packet', 'p')
            ->fields('p', array('packet_id', 'packet_title'));
        $query = $query->extend('TableSort');
        $query->join('ebms_article_review', 'r', 'r.packet_id = p.packet_id');
        $query->condition('r.reviewer_id', $user->uid);
        $query->addExpression('MAX(r.when_posted)', 'last_review');
        $query->groupBy('p.packet_id');
        $query->havingCondition('last_review', $two_years_ago, '>');
        $query->orderByHeader($headers);
        $results = $query->execute();

        // Create the rows for the table of packets.
        foreach ($results as $result) {
            if (!EbmsReview::packet_fully_completed($result->packet_id))
                continue;
            $reviewed = db_select('ebms_article_review', 'r')
                ->condition('r.packet_id', $result->packet_id)
                ->condition('r.reviewer_id', $user->uid)
                ->countQuery()
                ->execute()
                ->fetchField();
            $rows[] = array(
                l(htmlspecialchars($result->packet_title),
                    EbmsReview::$url . "/completed/$result->packet_id"),
                array(
                    'data' => date('Y-m-d', strtotime($result->last_review)),
                    'class' => array('ebms-align-center'),
                ),
                array(
                    'data' => $reviewed,
                    'class' => array('ebms-align-right'),
                ),
            );
        }
        $body['table'] = array(
            '#attributes' => array('id' => 'completed-packets-table'),
            '#theme' => 'table',
            '#rows' => $rows,
            '#header' => $headers,
        );
        return $body;
    }

    /**
     * Assemble a list of the board member's "FYI" packets. In the
     * wake of OCEEBMS-232, there are two different subtypes of such
     * packets. The first (the original "FYI" packets) are packets
     * which were not assigned to the current user, but the current
     * user is designated as a default reviewer for the packet's
     * assigned topic. The second type of "FYI" packet (added by
     * OCEEBMS-232) is a packet assigned to the current user,
     * containing only articles whose current state is "Flagged
     * as FYI." Sorting of the packets has to be done the hard
     * way, as the two types of "FYI" packets have to be queried
     * separately.
     */
    private function member_fyi() {

        // Override the default breadcrumbs.
        drupal_set_breadcrumb(
            array(
                l('Literature', EbmsReview::$url),
                'FYI PACKETS',
            )
        );

        // Start building the render array.
        $body = array(
            'title' => array(
                '#markup' => '<h2>FYI PACKETS</h2>',
            ),
        );

        // Make two of the columns sortable.
        $headers = array(
            array(
                'data' => 'Packet Name',
                'field' => 'p.packet_title',
            ),
            array(
                'data' => 'Date Created',
                'field' => 'created_at',
                'sort' => 'desc',
            ),
            'Number of Articles',
        );

        // Find the relevant packets.  Drupal's DB API isn't capable of
        // handling an expression in the havingCondition() method, so
        // we get the value in a separate query.
        $two_years_ago = db_query('SELECT DATE_SUB(NOW(), INTERVAL 2 YEAR)')
            ->fetchField();
        global $user;
        $uid = $user->uid;
        $rows = array();
        $subquery = db_select('ebms_packet_reviewer', 'r')
            ->fields('r', array('packet_id'))
            ->condition('r.reviewer_id', $uid);
        $query = db_select('ebms_packet', 'p')
            ->fields('p', array('packet_id', 'packet_title', 'created_at'));
        //$query = $query->extend('TableSort');
        $query->join('ebms_topic_reviewer', 'tr', 'tr.topic_id = p.topic_id');
        $query->condition('tr.user_id', $uid);
        $query->condition('p.created_at', $two_years_ago, '>');
        $query->condition('p.active_status', 'A');
        $query->condition('p.packet_id', $subquery, 'NOT IN');
        //$query->orderByHeader($headers);
        //$q = '' . $query;
        $results = $query->execute();
        $packets = array();
        foreach ($results as $result)
            $packets[] = $result;

        // Augment this list with packets assigned to the current
        // user, with only "FYI" articles (OCEEBMS-232).
        $fyi = EbmsReview::get_fyi_state_id();
        $subquery = db_select('ebms_packet', 'p')
            ->fields('p', array('packet_id'))
            ->condition('p.created_at', $two_years_ago, '>');
        $subquery->join('ebms_packet_article', 'a',
            'a.packet_id = p.packet_id');
        $subquery->join('ebms_article_state', 's',
            implode(' AND ',
                array(
                    "s.current = 'Y'",
                    's.article_id = a.article_id',
                    's.topic_id = p.topic_id',
                    "s.state_id <> $fyi",
                )
            )
        );
        $subquery->distinct();
        $query = db_select('ebms_packet', 'p')
            ->fields('p', array('packet_id', 'packet_title', 'created_at'));
        $query->join('ebms_packet_reviewer', 'r', 'r.packet_id = p.packet_id');
        $query->condition('r.reviewer_id', $uid);
        $query->condition('p.packet_id', $subquery, 'NOT IN');
        $query->condition('p.created_at', $two_years_ago, '>');
        $query->condition('p.active_status', 'A');
        $results = $query->execute();
        foreach ($results as $result)
            $packets[] = $result;

        // Sort the results by hand (OCEEBMS-232).
        $parms = drupal_get_query_parameters();
        if (!empty($parms['order']) && $parms['order'] == 'Packet Name')
            usort($packets, 'EbmsReview::compare_packet_titles');
        else
            usort($packets, 'EbmsReview::compare_packet_creation');
        if (!empty($parms['sort']) && $parms['sort'] == 'desc')
            $packets = array_reverse($packets);
        if (empty($parms['sort']) && empty($parms['order']))
            $packets = array_reverse($packets);

        // Create the rows for the table of packets.
        foreach ($packets as $packet) {
            $count = db_select('ebms_packet_article', 'a')
                ->condition('a.packet_id', $packet->packet_id)
                ->countQuery()
                ->execute()
                ->fetchField();
            $rows[] = array(
                l(htmlspecialchars($packet->packet_title),
                    EbmsReview::$url . "/fyi/$packet->packet_id"),
                array(
                    'data' => date('Y-m-d', strtotime($packet->created_at)),
                    'class' => array('ebms-align-center'),
                ),
                array(
                    'data' => $count,
                    'class' => array('ebms-align-right'),
                ),
            );
        }
        $body['table'] = array(
            '#attributes' => array('id' => 'fyi-packets-table'),
            '#theme' => 'table',
            '#rows' => $rows,
            '#header' => $headers,
        );
        return $body;
    }

    private static function compare_packet_titles($p1, $p2) {
        if ($p1->packet_title == $p2->packet_title)
            return 0;
        return ($p1->packet_title < $p2->packet_title) ? -1 : 1;
    }
    private static function compare_packet_creation($p1, $p2) {
        if ($p1->created_at == $p2->created_at)
            return 0;
        return ($p1->created_at < $p2->created_at) ? -1 : 1;
    }

    /**
     * Pick the page for the board member/reviewer.
     */
    private function choose_member_review_page() {

        if ($this->action == 'fyi') {
            if ($this->packet_id)
                return $this->member_fyi_packet();
            return $this->member_fyi();
        }
        if ($this->action == 'completed') {
            if ($this->packet_id)
                return $this->member_completed_packet();
            return $this->member_completed();
        }
        if ($this->action == 'archpost') {
            $url = EbmsReview::$url . '/completed/' . $this->packet_id;
            drupal_set_breadcrumb(
                array(
                    l('Literature', EbmsReview::$url),
                    l($this->packet_title, $url),
                    "UPLOAD",
                )
            );
            return drupal_get_form('pdq_ebms_reviewer_upload_form', $this);
        }
        if ($this->action == 'others')
            return $this->show_other_reviews();
        if ($this->article_id) {
            if (!$this->already_reviewed())
                return $this->member_review();
            $this->article_id = 0;
        }
        if ($this->packet_id) {
            $articles = $this->reviewer_articles();
            if (count($articles) > 0 || $this->last_reviewed)
                return $this->member_packet($articles);
            $this->packet_id = 0;
        }
        return $this->member_packets();
    }

    private function show_other_reviews() {

        // Set the bread crumbs.
        $options = array('query' => $this->query_parms());
        unset($options['query']['article']);
        drupal_set_breadcrumb(
            array(
                l('Literature', EbmsReview::$url),
                l($this->packet_title, EbmsReview::$url, $options),
                'OTHER REVIEWS',
            )
        );

        // Start building the render array for the page.
        $title = db_select('ebms_article', 'a')
            ->fields('a', array('article_title'))
            ->condition('a.article_id', $this->article_id)
            ->execute()
            ->fetchField();
        $title = htmlspecialchars($title);
        $page = array(
            'title' => array('#markup' => "<h2>$title</h2>"),
        );
        global $user;
        $query = db_select('ebms_article_review', 'r')
            ->fields('r', array('review_id', 'when_posted', 'comments',
                    'loe_info'))
            ->condition('r.packet_id', $this->packet_id)
            ->condition('r.article_id', $this->article_id)
            ->condition('r.reviewer_id', $user->uid, '<>');
        $query->join('users', 'u', 'u.uid = r.reviewer_id');
        $query->orderBy('u.name');
        $query->fields('u', array('name'));
        $reviews = $query->execute();
        $hr = '';
        foreach ($reviews as $review) {
            $block = array();
            $name = htmlspecialchars($review->name);
            $date = date('Y-m-d', strtotime($review->when_posted));
            $block['hr'] = array('#markup' => $hr);
            $hr = '<hr>';
            $block['reviewer'] = array(
                'title' => array('#markup' => '<h3>Reviewer</h3>'),
                'value' => array('#markup' => "<p>$name</p>"),
            );
            $block['reviewed'] = array(
                'title' => array('#markup' => '<h3>Review Date</h3>'),
                'value' => array('#markup' => "<p>$date</p>"),
            );
            $query = db_select('ebms_review_disposition', 'd');
            $query->join('ebms_review_disposition_value', 'v',
                'v.value_id = d.value_id');
            $query->fields('v', array('value_name'));
            $query->condition('d.review_id', $review->review_id);
            $query->orderBy('v.value_pos');
            $dispositions = $query->execute()->fetchCol();
            if (count($dispositions) > 0) {
                $block['dispositions'] = array(
                    '#theme' => 'item_list',
                    '#items' => $dispositions,
                    '#prefix' => '<h3>REVIEW DISPOSITION(S)</h3>',
                );
            }
            $query = db_select('ebms_review_rejection_reason', 'r');
            $query->join('ebms_review_rejection_value', 'v',
                'v.value_id = r.value_id');
            $query->fields('v', array('value_name'));
            $query->condition('r.review_id', $review->review_id);
            $query->orderBy('v.value_pos');
            $reasons = $query->execute()->fetchCol();
            if (count($reasons) > 0) {
                $block['reasons'] = array(
                    '#theme' => 'item_list',
                    '#items' => $reasons,
                    '#prefix' => '<h3>REJECTION REASON(S)</h3>',
                );
            }
            if ($review->comments) {
                $comments = htmlspecialchars($review->comments);
                $block['comments'] = array(
                    'title' => array('#markup' => '<h3>COMMENTS</h3>'),
                    'value' => array('#markup' => "<p>$comments</p>"),
                );
            }
            if ($review->loe_info) {
                $loe = htmlspecialchars($review->loe_info);
                $block['loe'] = array(
                    'title' => array('#markup' => '<h3>LOE INFO</h3>'),
                    'value' => array('#markup' => "<p>$loe</p>"),
                );
            }
            $page[$review->review_id] = $block;
        }
        return $page;
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
                'REVIEW',
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
        $links = array();
        foreach ($packets as $p) {
            $count = db_select('ebms_packet_article', 'a')
                ->condition('a.packet_id', $p->packet_id)
                ->condition('a.drop_flag', 0)
                ->countQuery()
                ->execute()
                ->fetchField();
            $options['query'] = array('packet' => $p->packet_id);
            pdq_ebms_preserve_on_behalf($options);
            $title = htmlspecialchars($p->packet_title);
            $what = $count == 1 ? 'article' : 'articles';
            $links[] = l("$title <b>($count $what)</b>", $url, $options);
        }

        // Added at Sridhar's request (TIR 2520).
        if (empty($links)) {
            $no_packets = 'There are no review packets in your queue.';
            $body['empty-queue'] = array(
                '#type' => 'markup',
                '#markup' => "<p>$no_packets</p>",
            );
        }
        else
            $body['packets'] = array(
                '#theme' => 'item_list',
                '#items' => $links,
                '#attributes' => array('class' => array('no-bullets')),
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
        // List assigned reviewers (OCEEBMS-379).
        $query = db_select('ebms_packet_reviewer', 'r');
        $query->join('users', 'u', 'u.uid = r.reviewer_id');
        $query->condition('r.packet_id', $this->packet_id);
        $query->fields('u', array('name'));
        $query->orderBy('u.name');
        $reviewers = implode(', ', $query->execute()->fetchCol());
        $body = array(
            '#prefix' => "$body_prefix<div id='member-packet'>",
            '#suffix' => "</div>$body_suffix",
            'packet-title' => array(
                '#type' => 'markup',
                '#markup' => "<h2>{$this->packet_title} $header_suffix</h2>" .
                '<br><h3>Instructions</h3>' .
                '<ol style="font-style: italic">' .
                '<li>Click on the summary document to open it on your ' .
                'computer so that it is ready for you to make changes.</li>' .
                '<li>Review articles. Use the REJECT button to quickly ' .
                'reject any articles that warrant no changes ' .
                'to the summary.</li>' .
                '<li>If needed, make changes to the summary using Track ' .
                'Changes and save it to your computer, adding your initials ' .
                'to the filename.</li>' .
                '<li>Upload the document with your changes ' .
                'using the POST DOCUMENT button found in the Reviewer ' .
                'Uploads section below.</li></ol>' .
                "<p>Assigned reviewers: $reviewers</p><br>",
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
     */
    private function show_packet_summaries(&$page) {


        // Fetch and list the summaries docs.
        $options = array('html' => true, 'query' => $this->query_parms());
        $page['summaries']['heading'] = array(
            '#type' => 'markup',
            '#markup' => '<h3>Summary Documents</h3>',
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

    /**
     * List the articles in the current packet which the board member still
     * needs to review. We've already fetched the articles because we had
     * to make sure there was actually a reason to display this page (which
     * we don't do if there are no more articles in the packet left for this
     * reviewer to look at).
     *
     * 2013-01-05 (TIR 2176): Robin wants us to show articles which have
     * already been reviewed (without the Review button).
     *
     * 2016-01-13 (OCEEBMS-232): show FYI articles separately, with
     * red "FYI" in place of "REVIEW" button.
     * 2017-06-09 (OCEEBMS-438): no, don't show them separately.
     *
     * 2017-06-09 (OCEEBMS-438): add button for "quick no" decision.
     */
    private function show_packet_articles(&$page, $articles) {

        // Add a popup for quick no ("exclude") decision.
        ctools_include('ajax');
        ctools_include('modal');
        ctools_modal_add_js();
        $exclude_url = 'review/nojs/quick_no';

        // Standard pieces we'll need for the link out to NLM.
        $target = array('attributes' => array('target' => '_blank'));

        // Options for links.
        $options = array('html' => true, 'query' => $this->query_parms());
        $options['attributes']['class'] = array('button');
        $ajax_opts = $options;
        $ajax_opts['attributes']['class'][] = 'ctools-use-modal';

        // Separate set for viewing other board member reviews.
        $other_opts = array(
            'html' => true,
            'query' => $this->query_parms()
        );

        // Build the array of rows for the article table.
        // OCEEBMS-232: save FYI articles for last.
        // OCEEBMS-436: requirement retracted.
        $rows = array();
        global $user;
        $topic_id = db_select('ebms_packet', 'p')
            ->fields('p', array('topic_id'))
            ->condition('p.packet_id', $this->packet_id)
            ->execute()
            ->fetchField();
        foreach ($articles as $article) {
            $is_fyi = EbmsReview::is_fyi($article->article_id, $topic_id);
            $options['query']['article'] = $article->article_id;
            $pm_id = $article->source_id;
            $pm_url = Ebms\Util::pubmed_url($pm_id);
            $pm_link = l('view abstract', $pm_url, $target);
            $links = '<div class="links">' . $pm_link;

            // Add the link for viewing the full text if we have it.
            if ($article->uri) {
                $ft_url = file_create_url($article->uri);
                $ft_link = l('download full text', $ft_url, $target);
                $links .= " &nbsp; | &nbsp; $ft_link";
            }

            // Add a link to view reviews by other board members
            // (OCEEBMS-96).
            if (!$is_fyi) {
                $count = db_select('ebms_article_review', 'r')
                    ->condition('r.packet_id', $this->packet_id)
                    ->condition('r.article_id', $article->article_id)
                    ->condition('r.reviewer_id', $user->uid, '<>')
                    ->countQuery()
                    ->execute()
                    ->fetchField();
                if ($count > 0) {
                    $other_opts['query']['article'] = $article->article_id;
                    $other_url = EbmsReview::$url . '/others';
                    $other_link = l('other reviews', $other_url, $other_opts);
                    $links .= " &nbsp; | &nbsp; $other_link";
                }
            }
            $links .= '</div>';
            $citation = EbmsReview::show_citation($article->article_id,
                        $topic_id, 3);
            $state = EbmsReview::get_on_agenda_state($article->article_id,
                                                     $topic_id);
            if ($state) {
                if (!preg_match('/<br>\s*$/', $citation))
                    $citation .= '<br>';
                $citation .= '<br><span class="mrs">ARTICLE STATE<br>' .
                    $state . '</span>';
            }
            if ($is_fyi)
                $c2 = '<span class="fyi">FYI</span>';
            elseif ($article->when_posted) {
                $when_posted = date('m/d/Y', strtotime($article->when_posted));
                $c2 = "<i>reviewed<br />$when_posted</i>";
            }
            elseif (EbmsReview::has_fbd($article->article_id, $topic_id))
                $c2 = '<i>Reviewed by Editorial Board</i>';
            else {
                $ajax_opts['query']['article'] = $article->article_id;
                $c2 = '<span id="article-buttons-' . $article->article_id .
                    '">' .
                    l('Review', EbmsReview::$url, $options) . ' &nbsp; '.
                    l('Reject', $exclude_url, $ajax_opts) . '</span>';
            }
            $row = array(
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
            $rows[] = $row;
        }

        // Plug the "Articles" heading and table into the page.
        $page['articles'] = array(
            'heading' => array(
                '#type' => 'markup',
                '#markup' => '<h3>Articles</h3>',
            ),
            'table' => array(
                '#prefix' => '<div id="articles">',
                '#theme' => 'table',
                '#rows' => $rows,
                '#suffix' => '</div><br />',
            ),
        );
    }

    /**
     * Get the string showing meeting on whose agenda this article appears.
     * OCEEBMS-465
     */
    public static function get_on_agenda_state($article_id, $topic_id) {
        $query = db_select('ebms_article_state', 's');
        $join = 't.state_id = s.state_id';
        $query->join('ebms_article_state_type', 't', $join);
        $query->fields('s', array('article_state_id'));
        $query->condition('s.article_id', $article_id);
        $query->condition('s.topic_id', $topic_id);
        $query->condition('s.current', 'Y');
        $query->condition('t.state_text_id', 'OnAgenda');
        $results = $query->execute();
        foreach ($results as $result) {
            $id = $result->article_state_id;
            $join = 'd.entity_id = n.nid';
            $query = db_select('node', 'n');
            $query->join('ebms_agenda_meeting', 'm', 'm.nid = n.nid');
            $query->join('field_data_field_datespan', 'd', $join);
            $query->condition('m.article_state_id', $id);
            $query->fields('n', array('title'));
            $query->fields('d', array('field_datespan_value'));
            $rows = $query->execute();
            $meetings = array();
            foreach ($rows as $row) {
                $title = $row->title;
                $date = date('Y-m-d', $row->field_datespan_value);
                $meetings[] = "$title - $date";
            }
            $meetings = htmlspecialchars(implode('; ', $meetings));
            if (!$meetings === '')
                $meetings = 'NO MEETINGS RECORDED';
            return "On Agenda: $meetings";
        }
        return '';
    }


    /**
     * Display a table listing the files the user has uploaded for this
     * packet.
     */
    private function show_reviewer_uploads(&$page) {

        // Create a button for uploading a new document.
        $options = array('html' => true, 'query' => $this->query_parms());
        $options['attributes']['class'] = array('button');
        $options['attributes']['id'] = 'reviewer-post-button';
        $url = EbmsReview::$url . '/reviewer-upload';
        $button = l('post document', $url, $options);

        // Open uploaded documents in a separate window or tab.
        $target = array('attributes' => array('target' => '_blank'));

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
        $page['uploads'] = array(
            '#prefix' => '<div id="reviewer-uploads">',
            '#suffix' => '</div>',
            'heading' => array(
                '#type' => 'markup',
                '#markup' => '<h3>Reviewer Uploads</h3>',
            ),
            'post link' => array(
                '#type' => 'markup',
                '#markup' => $button,
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
        return Ebms\Util::pubmed_url($source_id);
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
     * which have already been reviewed.  In order to make the software
     * behave as expected, this method needs to return an empty array
     * in the case where none of the articles in the packet are
     * waiting review (either because the board member already has
     * reviewed the articles or they have been dropped from the
     * packet).
     *
     * OCEEBMS-436: put articles from core journals on top.
     */
    private function reviewer_articles() {
        $query = db_select('ebms_article', 'a');
        $query->fields('a',
            array('article_id', 'brf_citation', 'full_text_id',
                'source_id'));
        $query->join('ebms_packet_article', 'p',
            'p.article_id = a.article_id');
        $query->join('ebms_journal', 'j',
            'j.source = a.source AND j.source_jrnl_id = a.source_jrnl_id');
        $query->leftJoin('ebms_article_review', 'r',
            'r.article_id = a.article_id AND ' .
            'r.packet_id = p.packet_id AND ' .
            'r.reviewer_id = :uid', array(':uid' => $this->user->uid));
        $query->leftJoin('file_managed', 'f', 'f.fid = a.full_text_id');
        $query->leftJoin('ebms_core_journal', 'c',
            'c.source = j.source AND c.source_jrnl_id = j.source_jrnl_id');
        $query->fields('f', array('uri'));
        $query->fields('r', array('review_id', 'when_posted'));
        $query->fields('p', array('drop_flag'));
        $query->condition('p.packet_id', $this->packet_id);
        $query->orderBy('c.source', 'DESC');
        $query->orderBy('j.jrnl_title');
        $query->orderBy('a.import_date', 'DESC');
        $result = $query->execute();
        $articles = array();
        $awaiting_review = 0;
        foreach ($result as $article) {
            if ($article->drop_flag) {
                if (!$article->review_id)
                    continue;
            }
            elseif (!$article->review_id)
                ++$awaiting_review;
            $articles[] = $article;
        }
        if (!$awaiting_review)
            $articles = array();
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
        if (in_array('board member', $this->user->roles))
            return true;

        return user_access('manage citations');
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
        $query->condition('v.active_status', 'A');
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
            //'expanded' => $this->expanded,
            'obo' => $this->on_behalf,
            'last-reviewed' => $this->last_reviewed,
        );
        foreach ($names as $name => $value)
            if ($value)
                $parms[$name] = $value;
        return $parms;
    }

    private static function get_article_topic_comment($article_id, $topic_id) {
        $results = db_select('ebms_article_topic_comment', 'c')
            ->fields('c', array('comment'))
            ->condition('c.article_id', $article_id)
            ->condition('c.topic_id', $topic_id)
            ->execute();
        foreach ($results as $result)
            if ($result->comment)
                return htmlspecialchars($result->comment);
        return '';
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
     *
     * 2016-06-29 (OCEEBMS-381): include related articles and board
     * manager comments.
     */
    private static function brief_citation($article_id, $topic_id) {

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
        if ($doc->MedlineCitation->Article->AuthorList->Author) {
            foreach ($doc->MedlineCitation->Article->AuthorList->Author as $a) {
                if ($a->LastName)
                    $citation[] = htmlspecialchars($a->LastName);
                break;
            }
        }
        else
            $citation[] = '[No authors listed]';
        $title_opts = array('attributes' => array('target' => '_blank'));
        $url = "citations/full/$article_id";
        $title = l(Ebms\Util::get_text($article->ArticleTitle), $url, $title_opts);
        $citation[] = $title; //htmlspecialchars($article->ArticleTitle);
        $ta = $doc->MedlineCitation->MedlineJournalInfo->MedlineTA;
        $citation[] = htmlspecialchars($ta);
        $year = $article->Journal->JournalIssue->PubDate->Year;
        if ($year)
            $citation[] = $year;
        $citation[] = $doc->MedlineCitation->PMID;
        $citation = implode('; ', $citation);


        // Add related articles and board manager comments (OCEEBMS-381).
        // Also flag high-priority article/topic combos (OCEEBMS-478).
        $citation .= Ebms\Util::get_related_articles($article_id, true);
        $topic_tags = EbmsReview::get_topic_tags($article_id, $topic_id);
        $comment = EbmsReview::get_article_topic_comment($article_id,
                                                         $topic_id);
        if (strpos($topic_tags, 'High priority') !== false) {
            $citation .= '<br><br><span class="hi-priority">';
            $citation .= 'HIGH PRIORITY</span>';
        }
        if ($comment) {
            $citation .= '<br><br><span class="bmc">BOARD MANAGER COMMENT';
            $citation .= "<br>$comment</span><br>";
        }

        // If we have the full text, provide a link to it.
        if ($uri) {
            $url = file_create_url($uri);
            $attrs = array(
                'attributes' => array(
                    'target' => '_blank',
                    'class' => 'full-text-link',
                ),
            );
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
     * OCEEBMS-202 (2014-08-01): include article counts for each topic.
     * OCEEBMS-232 (2016-01-12): include FYI articles
     * OCEEBMS-232 (2016-02-03): only include post-2016-02-01 FYI articles
     */
    public static function topics_for_board($board_id, $state_id) {
        $fyi_state_id = EbmsReview::get_fyi_state_id();
        $query = "
  SELECT t.topic_id, t.topic_name, COUNT(DISTINCT a.article_id) AS articles
    FROM ebms_topic t
    JOIN ebms_article_state s
      ON t.topic_id = s.topic_id
    JOIN ebms_article a
      ON a.article_id = s.article_id
   WHERE s.active_status = 'A'
     AND s.current = 'Y'
     AND (s.state_id = $state_id
      OR (s.state_id = $fyi_state_id
     AND s.status_dt >= '2016-02-01'))
     AND t.board_id = $board_id
     AND a.full_text_id IS NOT NULL
     AND s.article_id NOT IN (
         SELECT DISTINCT article_id
                    FROM ebms_packet_article
                   WHERE drop_flag = 0
                     AND packet_id IN (
                         SELECT packet_id
                           FROM ebms_packet
                          WHERE topic_id = s.topic_id))
GROUP BY t.topic_id, t.topic_name
ORDER BY t.topic_name";
        $results = db_query($query);
        $topics = array();
        foreach ($results as $result) {
            $name = $result->topic_name;
            $articles = $result->articles;
            $topics[$result->topic_id] = "$name ($articles)";
        }
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
     *
     * 2016-01-12 (OCEEBMS-232): include FYI articles.
     *
     * 2016-06-29 (OCEEBMS-381): include related articles and board
     * manager comments.
     *
     * @param int $topic_id
     *   The topic for which articles that passed review are selected.
     * @param int $state_id
     *   Only articles with this ebms_article_state_id are selected.
     *   Probably always 'Passed full text review'.
     * @param string $sort_by
     *   Order the results by this.  Valid values: 'Author', 'Journal Title'.
     * @param int $packet_id
     *   Only select articles NOT in this packet.
     *   0 = There is no packet 0, so packet_id==0 excludes nothing.
     *   See also $query->union() below
     *
     * @return
     *   Array of article_id => brief citation for each selected article.
     */
    public static function articles_for_topic(
        $topic_id, $state_id, $sort_by, $packet_id=0
    ) {
        // 2016-02-02: Robin changed the requirements. She now only
        // wants FYI articles if they were put in that state after
        // a certain point in time.
        // https://tracker.nci.nih.gov/browse/OCEEBMS-232#comment-530157
        $fyi_test = db_and();
        $fyi_test->condition('s.state_id', EbmsReview::get_fyi_state_id());
        $fyi_test->condition('s.status_dt', '2016-02-01', '>=');
        $state_test = db_or();
        $state_test->condition('s.state_id', $state_id);
        $state_test->condition($fyi_test);
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
        $query->condition($state_test);
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
        if ($sort_by == 'Journal Title')
            $query->orderBy('ea.jrnl_title');
        if (false)
            dpq($query);
        $results = $query->execute();
        $articles = array();
        foreach ($results as $result) {
            $id = $result->article_id;
            $articles[$id] = EbmsReview::brief_citation($id, $topic_id);
        }
        // Real sort is complicated, but brief citation has author at front
        if ($sort_by == 'Author')
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
        $query->condition('d.drop_flag', 0);
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
     * at least one active article is still waiting to be reviewed.
     * 2016-01-13 (OCEEBMS-232) ignore FYI articles.
     *
     * Regard articles with states beyond FYI and siblings as "complete"
     * (OCEEBMS-450).
     */
    public static function reviewer_packets_by_id($user_id) {
        $fyi = Ebms\Util::state_id_from_text_id('FYI');
        $on_agenda_sequence = db_select('ebms_article_state_type', 't')
            ->condition('t.state_text_id', 'OnAgenda')
            ->fields('t', array('sequence'))
            ->execute()
            ->fetchField();
        $query = db_select('ebms_packet', 'p');
        $query->fields('p', array('packet_id', 'packet_title'));
        $query->join('ebms_packet_reviewer', 'm', 'p.packet_id = m.packet_id');
        $query->join('ebms_packet_article', 'a', 'a.packet_id = p.packet_id');
        $query->join('ebms_topic', 't', 't.topic_id = p.topic_id');
        $query->join('ebms_article_state', 's',
            's.article_id = a.article_id AND s.topic_id = t.topic_id');
        $query->join('ebms_article_state_type', 'st',
                     'st.state_id = s.state_id');
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
        $query->condition('a.drop_flag', 0);
        $query->isNull('r.article_id');
        $query->condition('s.current', 'Y');
        $query->condition('s.state_id', $fyi, '<>');
        $query->condition('st.sequence', $on_agenda_sequence, '<=');
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
    public static function show_citation($article_id, $topic_id,
                                         $max_authors=99999,
                                         $include_links=false) {
        $xml = db_select('ebms_article', 'a')
            ->fields('a', array('source_data'))
            ->condition('a.article_id', $article_id)
            ->execute()
            ->fetchField();
        $doc = new \SimpleXMLElement($xml);
        $title = $ta = $vol = $issue = $pgn = $year = $pmid = '';
        $article = $doc->MedlineCitation->Article;
        $title = htmlspecialchars(Ebms\Util::get_text($article->ArticleTitle));
        $pgn = htmlspecialchars($article->Pagination->MedlinePgn);
        $year = $article->Journal->JournalIssue->PubDate->Year;
        $issue = htmlspecialchars($article->Journal->JournalIssue->Issue);
        $vol = htmlspecialchars($article->Journal->JournalIssue->Volume);
        $authors = array();
        $pieces = array();
        if ($doc->MedlineCitation->Article->AuthorList->Author) {
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
            $authors = implode(', ', $authors);
            if ($authors)
                $pieces[] = "$authors<br />";
        }
        else
            $pieces[] = "[No authors listed]<br />";
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

        // Add related articles and board manager comments if present (OCEEBMS-381).
        // Also, highlight high-priority article/topic combos (OCEEBMS-477).
        $related_articles = Ebms\Util::get_related_articles($article_id);
        if ($related_articles)
            $pieces[] = "<br>$related_articles";
        $topic_tags = EbmsReview::get_topic_tags($article_id, $topic_id);
        if (strpos($topic_tags, 'High priority') !== false) {
            $pieces[] = '<br><br><span class="hi-priority">';
            $pieces[] = 'HIGH PRIORITY</span>';
        }
        $comment = EbmsReview::get_article_topic_comment($article_id,
                                                         $topic_id);
        if ($comment) {
            $pieces[] = '<br><br><span class="bmc">BOARD MANAGER COMMENT';
            $pieces[] = "<br>$comment</span><br>";
        }

        if ($include_links) {
            if (!$comment)
                $pieces[] = '<br>';
            $pieces[] = '<br>';
            $url = Ebms\Util::pubmed_url($pmid);
            $pieces[] = "<a href='$url' target='_blank'>VIEW ABSTRACT</a>";
            $ft_uri = Ebms\Util::get_ft_uri($article_id);
            if ($ft_uri) {
                $ft_url = file_create_url($ft_uri);
                $target = array('attributes' => array('target' => '_blank'));
                $ft_link = l('DOWNLOAD FULL TEXT', $ft_url, $target);
                $pieces[] = ' &nbsp;|&nbsp; ' . $ft_link;
            }
            $pieces[] = '<br />';
        }
        return implode($pieces);
    }

    /**
     * Get an array of board IDs established for the current literature
     * review activity.
     *
     * For board managers, this is usually the manager's board.  However some
     * users (e.g., and administrative assistant) will see a menu in a side
     * bar allowing them to select one or more boards.
     *
     * Uses globals to find the info.
     *
     * @return Array of zero or more integer board ids.
     */
    public static function get_board_ids() {

        global $user;

        $boards = array();
        $opts   = array('query' => drupal_get_query_parameters());

        // From the sidebar checkboxes
        if (isset($opts['query']['boards']))
            $boards = explode(',', $opts['query']['boards']);

        // Or from the user's boards
        if (count($boards) == 0)
            $boards = $user->boards;

        // Can't happen I think
        if (count($boards) == 0) {
            watchdog('review.inc',
                     "User {$user->name} has no boards.  How can that be?");
            $boards = array();
        }

        return $boards;
    }

    /**
     * Get a list of topic names to use in constructing a topic filter options
     * list.
     *
     * @return array    topic_id => topic_name, in name alpha order.
     */
    public static function packet_topic_list() {

        global $user;

        $boards = EbmsReview::get_board_ids();

        $query = db_select('ebms_topic', 't')
            ->fields('t', array('topic_id', 'topic_name'))
            ->orderBy('t.topic_name');
        if (!empty($boards))
            $query->condition('t.board_id', $boards, 'IN');

        $results = $query->execute();

        // Results in an array
        $topics = $results->fetchAllKeyed(0, 1);

        return $topics;
    }

    /**
     * Collect board members who have review work pending.  The result
     * will be used for the picklist on the form for recording responses
     * on behalf of a board member.
     *
     *  @param  integer           ID of board from which to collect
     *                            members
     *
     *  @return array             board member names, keyed by user
     *                            ID, sorted by name
     */
    public static function reviewers_with_work($board_id) {
        $query = db_select('users', 'u')
            ->fields('u', array('uid', 'name'));
        $query->join('ebms_packet_reviewer', 'r', 'r.reviewer_id = u.uid');
        $query->join('ebms_packet_article', 'a',
            'a.packet_id = r.packet_id AND a.drop_flag = 0');
        $query->join('ebms_packet', 'p', 'p.packet_id = r.packet_id');
        $query->join('ebms_topic', 't', 't.topic_id = p.topic_id');
        $query->leftJoin('ebms_article_review', 'w',
            'w.packet_id = a.packet_id AND ' .
            'w.article_id = w.article_id AND ' .
            'w.reviewer_id = u.uid');
        $query->condition('p.active_status', 'A');
        $query->condition('t.board_id', $board_id);
        $query->isNull('w.review_id');
        $query->orderBy('u.name');
        $results = $query->execute();
        $reviewers = array();
        foreach ($results as $result)
            $reviewers[$result->uid] = $result->name;
        return $reviewers;
    }

    /**
     * Find out whether the current state for this article/topic
     * combination is FYI.
     */
    public static function is_fyi($article_id, $topic_id) {
        $query = db_select('ebms_article_state', 's');
        $query->fields('s', array('state_id'));
        $query->condition('s.topic_id', $topic_id);
        $query->condition('s.article_id', $article_id);
        $query->condition('s.active_status', 'A');
        $query->condition('s.current', 'Y');
        $state_id = $query->execute()->fetchField();
        return $state_id == EbmsReview::get_fyi_state_id();
    }

    /**
     * Find out whether the current state for this article/topic
     * combination is Final Board Decision.
     */
    public static function has_fbd($article_id, $topic_id) {
        $fbd = Ebms\Util::state_id_from_text_id('FinalBoardDecision');
        $query = db_select('ebms_article_state', 's');
        $query->fields('s', array('state_id'));
        $query->condition('s.topic_id', $topic_id);
        $query->condition('s.article_id', $article_id);
        $query->condition('s.active_status', 'A');
        $query->condition('s.current', 'Y');
        $state_id = $query->execute()->fetchField();
        return $state_id == $fbd;
    }

    /**
     * Get the primary key for the row in the state table for "Flagged
     * as FYI," using a cached answer so we don't have to keep hitting
     * the database multiple times during a single web request to get
     * the same value repeatedly.
     */
    public static function get_fyi_state_id() {
        if (!EbmsReview::$fyi_state)
            EbmsReview::$fyi_state = Ebms\Util::state_id_from_text_id('FYI');
        return EbmsReview::$fyi_state;
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
        $fyi_articles,
        $summaries;

    /**
     * Pull the information for the packet from the ebms_packet and
     * related tables.
     */
    public function __construct($packet_id) {
        $this->articles = array();
        $this->fyi_articles = array();
        $this->id = $packet_id;
        $this->reviewers = EbmsPacket::fetch_reviewers($packet_id);
        $this->summaries = EbmsPacket::fetch_summaries($packet_id);
        $articles = EbmsPacket::fetch_articles($packet_id);
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
        foreach ($articles as $article_id) {
            if (EbmsReview::is_fyi($article_id, $this->topic_id))
                $this->fyi_articles[] = $article_id;
            else
                $this->articles[] = $article_id;
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
        $reviewers = array();
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

    // Get the topic ID.
    $topic_id = db_select('ebms_packet', 'p')
        ->condition('p.packet_id', $review->packet_id)
        ->fields('p', array('topic_id'))
        ->execute()
        ->fetchField();

    // Assemble the rest of the values needed for the form.
    $article_id = $review->article_id;
    $title = htmlspecialchars($review->packet_title);
    $attrs = array('attributes' => array('target' => '_blank'));
    $pm_url = EbmsReview::pubmed_url($article_id);
    $links = '<div class="links">' . l('VIEW ABSTRACT', $pm_url, $attrs);
    $cite = EbmsReview::show_citation($article_id, $topic_id);
    $state = EbmsReview::get_on_agenda_state($article_id, $topic_id);
    if ($state) {
        if (!preg_match('/<br>\s*$/', $cite))
            $cite .= '<br>';
        $cite .= '<br><span class="mrs">ARTICLE STATE<br>' .
            $state . '</span>';
    }

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

    // Add link to other reviews of this article (OCEEBMS-380)
    $count = db_select('ebms_article_review', 'r')
        ->condition('r.packet_id', $review->packet_id)
        ->condition('r.article_id', $article_id)
        ->condition('r.reviewer_id', $review->user->uid, '<>')
        ->countQuery()
        ->execute()
        ->fetchField();
    if ($count > 0) {
        $other_opts['query'] = drupal_get_query_parameters();
        $other_url = EbmsReview::$url . '/others';
        $other_link = l('OTHER REVIEWS', $other_url, $other_opts);
        $links .= " &nbsp; | &nbsp; $other_link";
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
        //'expanded' => array(
        //    '#type' => 'hidden',
        //    '#value' => $review->expanded,
        //),
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
            '#title' =>
            'Reason(s) for Exclusion From PDQ<sup>&reg;</sup> Summary',
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
            '#title' => 'Comments and Changes to Summary',
            '#description' =>
            'Please make your suggested changes directly in the summary ' .
            'using Track Changes and upload the revised summary on the page ' .
            'for this literature packet. If you have additional comments, ' .
            'please add them here.',
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
        // 2019-08-12 - overturned by OCEEBMS-451: store with markup.
        $uname = $user->name;
        $aname = $account->name;
        $comment_suffix = "<i>Recorded by $uname on behalf of $aname</i>";
    }
    else
        $account = $user;

    $packet_id = $form_state['values']['packet'];
    $article_id = $form_state['values']['article'];
    //$expanded = $form_state['values']['expanded'];
    $sort_col = $form_state['values']['sortcol'];
    $sort_dir = $form_state['values']['sortdir'];
    $comments = trim($form_state['values']['comments']);
    if (empty($comments))
        $comments = $comment_suffix;
    else
        $comments .= "<br><br>$comment_suffix";
    $loe = $form_state['values']['loe'];
    $now = strftime("%Y-%m-%d %H:%M:%S");
    $review_id = db_insert('ebms_article_review')
        ->fields(
            array(
                'packet_id'   => $packet_id,
                'article_id'  => $article_id,
                'reviewer_id' => $account->uid,
                'when_posted' => $now,
                'comments'    => $comments,
                'loe_info'    => $loe,
            )
        )
        ->execute();
    db_update('ebms_packet_article')
        ->fields(array('archived' => null))
        ->condition('article_id', $article_id)
        ->condition('packet_id', $packet_id)
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
            //'expanded' => $expanded,
            'sort-col' => $sort_col,
            'sort-dir' => $sort_dir,
            'last-reviewed' => $article_id,
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
            '#markup' => '<h2>Reviewer Upload</h2>' .
            '<p><em>Select the document you would like to upload. ' .
            'To complete the file upload, you must click the Upload ' .
            'File button below.</em></p>',
        ),
        'packet' => array(
            '#type' => 'hidden',
            '#value' => $review->packet_id,
        ),
        //'expanded' => array(
        //    '#type' => 'hidden',
        //    '#value' => $review->expanded,
        //),
        'sort-col' => array(
            '#type' => 'hidden',
            '#value' => $review->sort_col,
        ),
        'sort-dir' => array(
            '#type' => 'hidden',
            '#value' => $review->sort_dir,
        ),
        'context-action' => array(
            '#type' => 'hidden',
            '#value' => $review->action,
        ),
        'last-reviewed' => array(
            '#type' => 'hidden',
            '#value' => $review->last_reviewed,
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
    //$expanded = $form_state['values']['expanded'];
    $sort_col = $form_state['values']['sort-col'];
    $sort_dir = $form_state['values']['sort-dir'];
    $last_reviewed = $form_state['values']['last-reviewed'];
    $filenotes = $form_state['values']['filenotes'];
    $action = $form_state['values']['context-action'];

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
        $query->join('ebms_topic', 'b', 'b.topic_id = t.topic_id');
        $query->fields('b', array('board_id'));
        $query->join('ebms_packet', 'p', 'p.topic_id = t.topic_id');
        $query->condition('p.packet_id', $packet_id);
        $results = $query->execute();
        $page_ids = array();
        $subpage = null;
        foreach ($results as $result) {
            $page_ids[] = $result->page_id;
            if (!$subpage)
                $subpage = 'page/' . $result->board_id . '/' .
                    $result->page_id;
        }

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
        $uri = db_select('file_managed', 'f')
            ->fields('f', array('uri'))
            ->condition('f.fid', $file->fid)
            ->execute()
            ->fetchField();
        $url = file_create_url($uri);
        \Ebms\Util::activityNewSummary(array(
            'account' => $account,
            'title' => $filename,
            'url' => $url,
            'note' => $filenotes,
            'topics' => array($packetInfo->topic_id)));

        // Tell the user that we succeeded.
        drupal_set_message('Posted document ' . $file->filename . '.');
    }
    else

        // Tell the user that we failed.
        drupal_set_message('File not found', 'error');

    if ($action == 'archpost')
        drupal_goto(EbmsReview::$url . '/completed/' . $packet_id);
    else {
        // Go back to the user's packet.
        $options = array(
            'query' => array(
                'packet' => $packet_id,
                //'expanded' => $expanded,
                'sort-col' => $sort_col,
                'sort-dir' => $sort_dir,
                'last-reviewed' => $last_reviewed,
            ),
        );
        pdq_ebms_preserve_on_behalf($options);
        drupal_goto(EbmsReview::$url, $options);
    }
}

/**
 * What is the sort order for the display of articles in packet creation?
 *
 * @param array $form_state
 *   Access to current form submit values.
 *
 * @return string
 *   One of the legal values
 */
function _get_packet_create_article_sort_key($form_state) {

    // Set default value for sort key
    if (!empty($form_state['values']['sort-by'])) {
        $sort_by = $form_state['values']['sort-by'];
    }
    else if (!empty($form_state['input']['sort-by'])) {
        $sort_by = $form_state['input']['sort-by'];
    }
    else {
        $sort_by = 'Author';
    }

    return $sort_by;
}

/**
 * Create the "display options" field to allow a user to specify how
 * to sort articles for display.
 *
 * @param array $form_state
 *   Allows access to current settings of the sort-by value.
 *
 * @return
 *   Drupal render array for the fieldset with display options
 */
function _build_article_display_options($form_state) {

    $form = array(
        '#type' => 'fieldset',
        '#title' => 'Display options',
        '#collapsible' => true,
        '#collapsed' => true,
        // '#attributes' => array('class' => array('clearfix')),
        'sort-by' => array(
            '#type' => 'select',
            '#title' => 'SORT BY',
            '#title_display' => 'after',
            '#attributes' => array('class' => array('fixed-width')),
            '#options' => drupal_map_assoc(
                array(
                    'Author',
                    'Journal Title',
                )
            ),
            '#default_value' =>
                    _get_packet_create_article_sort_key($form_state),
            '#ajax' => array(
                'callback' => 'pdq_ebms_packet_topic_ajax',
                'wrapper' => 'topic-specific-fields',
            ),
        ),
    );

    return $form;
}

/**
 * Create the form used by a board manager to create a new literature
 * review packet.  We show the parts of the form incrementally, as
 * lower portions depend on the selections made in the upper portions.
 */
function pdq_ebms_edit_packet_form($form, &$form_state, $packet_id=0) {

    // We'll use this more than once, so fetch and save it now.
    $state_id = Ebms\Util::get_state_id('Passed full text review');
    $boards = Ebms\Util::boards();

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
        $board_id = Ebms\Util::get_user_default_board(0);
        $topic_id = $current_topic = 0;
        if (!empty($form_state['values']['board']))
            $board_id = $form_state['values']['board'];
        if (!empty($form_state['values']['topic']))
            $topic_id = $form_state['values']['topic'];
        if (!empty($form_state['values']['current-topic']))
            $current_topic = $form_state['values']['current-topic'];
        $packet_header = 'Add New Literature Surveillance Packet';

        // If the user doesn't have a default board, pick one at random.
        if (!$board_id)
            $board_id = array_keys($boards)[0];

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
            'Create Packet',
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
        $selected_reviewers = $fyi_article_defaults = array();

    // Set up the fields which depend on having a topic selected.
    $need_disp_opts = true;
    if ($topic_id) {
        $need_disp_opts = false;

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

        // Get sort order for articles
        $sort_by = _get_packet_create_article_sort_key($form_state);

        // Take care of the rest of the fields.
        $articles_title = 'Articles';
        $fyi_articles_title = 'FYI Articles';
        $summaries_title = 'Summary Documents (Optional)';
        $reviewers_title = 'Reviewers';
        $all_reviewers_class = 'all-reviewers-visible';

        $all_articles = EbmsReview::articles_for_topic($topic_id, $state_id,
                                                       $sort_by, $packet_id);
        $articles = array();
        $fyi_articles = array();
        foreach ($all_articles as $article_id => $citation) {
            if (EbmsReview::is_fyi($article_id, $topic_id))
                $fyi_articles[$article_id] = $citation;
            else
                $articles[$article_id] = $citation;
        }
        $summaries = EbmsReview::summaries_for_topic($topic_id, $packet_id);
        $articles_desc =
            'Selected articles will be included in the packet.  ' .
            'You may unselect any articles that you do not want ' .
            'to be included.';
        $fyi_articles_desc = 'These articles will not be reviewed.';
        $summaries_desc = 'You may select more than one summary document.';
        $reviewers_desc =
            'The following reviewers are associated with this topic ' .
            'in the EBMS. ' .
            'Selected reviewers will receive this packet to review on ' .
            'their Assigned Packets page. ' .
            'Unselected reviewers who are associated with this topic ' .
            'will be able to view the packet on their FYI Packets page. ' .
            'You may also select additional Board members who are not ' .
            'associated with this topic to review this packet.';

        // Are we editing an existing packet?
        if ($packet_id) {
            $article_defaults = $packet->articles;
            $fyi_article_defaults = $packet->fyi_articles;
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

        else {
            $article_defaults = array_keys($articles);
            $fyi_article_defaults = array_keys($fyi_articles);
        }
    }
    else {

        $all_reviewers_class = 'all-reviewers-hidden';
        $articles_title = 'Articles';
        $fyi_articles_title = 'FYI Articles';
        $summaries_title = 'Summary Documents (Optional)';
        $reviewers_title = 'Reviewers';
        $articles_desc = $summaries_desc = $reviewers_desc =
            $fyi_articles_desc = '';
        $articles = $summaries = $reviewers = $article_defaults =
            $summary_defaults = $reviewer_defaults =
            $fyi_articles = $fyi_article_defaults = array();
        $all_reviewers = false;
    }
    $form_state['ebms-checked-articles'] = $article_defaults;
    $form_state['ebms-checked-fyi-articles'] = $fyi_article_defaults;
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
        '#prefix' => '<div class="packet-article-block">',
        '#suffix' => '</div>',
        '#type' => 'checkboxes',
        '#title' => $articles_title,
        '#description' => $articles_desc,
        '#options' => $articles,
        '#default_value' => $article_defaults,
    );
    $fyi_articles_field = array(
        '#prefix' => '<div class="packet-article-block">',
        '#suffix' => '</div>',
        '#type' => 'checkboxes',
        '#title' => $fyi_articles_title,
        '#description' => $fyi_articles_desc,
        '#options' => $fyi_articles,
        '#default_value' => $fyi_article_defaults,
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
        $topic_specific_fields = array();
        if (!empty($articles))
            $topic_specific_fields['articles'] = $articles_field;
        if (!empty($fyi_articles))
            $topic_specific_fields['fyi-articles'] = $fyi_articles_field;
        $topic_specific_fields['summaries'] = $summaries_field;
        $topic_specific_fields['reviewers-div'] = $reviewers_div;
        $topic_specific_fields['name'] = array(
            '#type' => 'textfield',
            '#title' => 'Packet Name',
            '#default_value' => $packet_name,
        );
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
                'topic-specific-fields' => $topic_specific_fields,
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
            '#options' => $boards,
            '#default_value' => $board_id,
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
            //'#group_info' => array(1 => 'foo', 5 => 'bar'),
            '#ajax' => array(
                'callback' => 'pdq_ebms_packet_topic_ajax',
                'wrapper' => $tsf,
            ),
        );
        if ($boards[$board_id] == 'Adult Treatment')
            $form[$bsf]['topic']['#pre_render'] = array('_group_ebms_topics');

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
            $topic_name = db_select('ebms_topic', 't')
                ->fields('t', array('topic_name'))
                ->condition('t.topic_id', $topic_id)
                ->execute()
                ->fetchField();

            if ($topic_name) {

                // OCEEBMS-202 (2014-08-01): add cycle to default packet name.
                //$cycle_id = Ebms\Util::get_default_cycle();
                //$cycle_name = Ebms\Util::get_cycle_name($cycle_id);
                // Robin changed her mind (2014-08-20) use the current month,
                // not the current cycle name.
                $now = date('F Y');
                $name = "$topic_name ($now)";
            }
            // All of these blocks are displayed when a topic is selected
            //  and all change if and when the topic changes
            $form[$bsf][$tsf]['disp-opt'] =
                    _build_article_display_options($form_state);
            if (!empty($articles))
                $form[$bsf][$tsf]['articles'] = $articles_field;
            if (!empty($fyi_articles))
                $form[$bsf][$tsf]['fyi-articles'] = $fyi_articles_field;
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

function _group_ebms_topics($element) {
    $results = db_select('ebms_topic_group', 'g')
        ->fields('g', array('group_id', 'group_name'))
        ->orderBy('g.group_name')
        ->execute();
    $groups = array();
    foreach ($results as $group) {
        $group->topics = array();
        $groups[$group->group_id] = $group;
    }
    $ungrouped = new stdClass();
    $ungrouped->group_id = null;
    $ungrouped->group_name = 'Ungrouped';
    $ungrouped->topics = array();
    $groups[null] = $ungrouped;
    $results = db_select('ebms_topic', 't')
        ->fields('t', array('topic_id', 'topic_group'))
        ->execute();
    $topic_groups = array();
    foreach ($results as $result)
        $topic_groups[$result->topic_id] = $result->topic_group;
    $new_element = array();
    foreach ($element as $key => $value) {
        if (strpos($key, '#') !== false)
            $new_element[$key] = $value;
        else {
            $group_id = $topic_groups[$key];
            $groups[$group_id]->topics[$key] = $value;
        }
    }
    $weight = .001;
    foreach ($groups as $group) {
        if (count($group->topics) > 0) {
            $name = htmlspecialchars($group->group_name);
            $prefix = "<p class='topic-group'>$name</p>";
            $elements[$key]['#prefix'] = $prefix;
            $first = true;
            foreach ($group->topics as $key => $value) {
                if ($first) {
                    $value['#prefix'] = $prefix;
                    $first = false;
                }
                $value['#weight'] = $weight;
                $weight += .001;
                $new_element[$key] = $value;
            }
        }
    }
    return $new_element;
}

/**
 * Verify that the required information is present for a new or
 * modified packet.  We do this here instead of by using the
 * 'required' property because the users don't like the red asterisks
 * which Drupal injects into the form's markup.
 */
function pdq_ebms_edit_packet_form_validate($form, &$form_state) {
    $packet_id = $form_state['values']['packet'];
    $name = '';
    if (!empty($form_state['values']['name']))
        $name = trim($form_state['values']['name']);
    if (!$name)
        form_set_error('name', 'Packet must have a name.');
    if (!$packet_id) {
        if (!$form_state['values']['board'])
            form_set_error('board', 'Board selection is required.');
        if (!$form_state['values']['topic'])
            form_set_error('topic', 'Topic selection is required.');
    }
    $article_count = 0;
    $articles = array();
    if (!empty($form_state['values']['articles']))
        $articles = $form_state['values']['articles'];
    foreach ($articles as $id => $val)
        if ($val)
            $article_count++;
    $fyi_articles = array();
    if (!empty($form_state['values']['fyi-articles']))
        $fyi_articles = $form_state['values']['fyi-articles'];
    foreach ($fyi_articles as $id => $val)
        if ($val)
            $article_count++;
    if (!$article_count)
        form_set_error('articles', 'At least one article must be selected.');
    $form_state['redirect'] = array(
        EbmsReview::$url . '/packets',
        'query' => drupal_get_query_parameters(),
    );
}

/**
 * Store the new or modified packet, along with the articles assigned
 * for review, the board members assigned to perform the review, and
 * the summary documents attached to the packet.
 *
 * (2015-10-14) Modify logic for notification of new/modified packet:
 *  OCEEBMS-258 - only notify reviewers for topic of an FYI packet
 *  OCEEBMS-276 - don't renotify reviewer already on existing packet
 */
function pdq_ebms_edit_packet_form_submit($form, &$form_state) {

    // Collect all of the form's values.
    $packet_id = $form_state['values']['packet'];
    $articles = array();
    $reviewers = array();
    $summaries = array();
    $notify = array();
    $name = trim($form_state['values']['name']);
    if (!empty($form_state['values']['articles']))
        foreach ($form_state['values']['articles'] as $id => $value)
            if ($value)
                $articles[$value] = $value;
    if (!empty($form_state['values']['fyi-articles']))
        foreach ($form_state['values']['fyi-articles'] as $id => $value)
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
        if (count($reviewers) > 0)
            $notify = $reviewers;

        // Add FYI notification for the topic's other reviewers.
        $query = db_select('users', 'u');
        $query->fields('u', array('uid'));
        $query->join('ebms_topic_reviewer', 'r', 'r.user_id = u.uid');
        $query->condition('r.topic_id', $topic_id);
        $query->condition('u.status', 1);
        $results = $query->execute();
        foreach ($results as $result)
            $notify[$result->uid] = $result->uid;
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
                        ->condition('packet_id', $packet_id)
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
                        ->condition('packet_id', $packet_id)
                        ->execute();
        }

        // Only notify reviewers being added to the packet.
        $query = db_select('ebms_packet_reviewer', 'r');
        $query->fields('r', array('reviewer_id'));
        $query->condition('r.packet_id', $packet_id);
        $already_notified = $query->execute()->fetchCol();
        foreach ($reviewers as $reviewer_id) {
            if (!in_array($reviewer_id, $already_notified))
                $notify[$reviewer_id] = $reviewer_id;
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
    if (count($notify) > 0)
        \Ebms\Util::activityPacketCreated($name, $notify, $packet_id);

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

// XXX DEBUGGING FOR QA TEAM
//     $status = db_select('ebms_packet', 'p')
//         ->fields('p', array('active_status'))
//         ->condition('p.packet_id', $packet_id)
//         ->execute()
//         ->fetchField();
//     pdq_ebms_debug('PACKET EDIT', array(
//         'FORM VALUES' => $form_state['values'],
//         'PACKET STATUS' => $status));
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
    $checked_fyi_articles = $form_state['ebms-checked-fyi-articles'];
    $subform = $form['board-specific-fields']['topic-specific-fields'];
    foreach ($subform['articles'] as $k => $v) {
        if (isset($v['#type']) and $v['#type'] == 'checkbox') {
            $checked = in_array($k, $checked_articles) ? 1 : 0;
            $subform['articles'][$k]['#checked'] = $checked;
        }
    }
    if (!empty($subform['fyi-articles'])) {
        foreach ($subform['fyi-articles'] as $k => $v) {
            if (isset($v['#type']) and $v['#type'] == 'checkbox') {
                $checked = in_array($k, $checked_fyi_articles) ? 1 : 0;
                $subform['fyi-articles'][$k]['#checked'] = $checked;
            }
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
 * Create a form to filter packets appearing in the literature
 * surveillance packet listing.  The form will appear above the main body
 * of the View/Edit packets display.
 *
 * A user may fill in the search fields and click the Filter button
 * to restrict the packets appearing on the page.
 *
 * If no filtering is done, the page will show all packets that
 * belong to the board(s) being viewed by this manager which have not
 * been archived.
 *
 * Subroutine of manager_packets.
 *
 *  @param array $form
 *  @param array &$form_state
 *  @param_search_id          If non-zero, the value is passed to
 *                            EbmsReview::apply_manager_packet_filter_search()
 *                            to replace default values with values from the
 *                            last conducted search.
 *
 *  @return the drupal render array for the form.
 */
function pdq_ebms_manager_packets_filter_form (
        $form, &$form_state, $search_id=0) {

    $form['#id'] = 'packet-filter-form';

    $form['title'] = array(
        '#type' => 'markup',
        '#markup' => '<h3>Filter packet selection</h3>',
    );
    $form['topic_box'] = array(
        '#type' => 'fieldset',
        '#title' => 'Summary topics',
        '#collapsible' => true,
        '#collapsed' => true,
    );
    $form['topic_box']['filt_topic'] = array(
        '#type' => 'checkboxes',
        '#options' => EbmsReview::packet_topic_list(),
    );
    $form['packet_title_text'] = array(
        '#type' => 'textfield',
        '#size' => 40,
        '#title' => 'Packet Name: ',
        '#title_display' => 'before',
    );
    $date_fields = Ebms\Util::date_range_field('Packet Creation Date:', 'date',
                                               '', '', ':+0', false);
    $form['date-limits'] = $date_fields;
    $form['pkt_scope_active'] = array(
        '#type' => 'checkboxes',
        '#options' => drupal_map_assoc(array('Active', 'Archived')),
        '#default_value' => array('Active'),
        '#title' => 'Packet Statuses To Include:',
    );
    $form['pkt_scope_reviewed'] = array(
        '#type' => 'checkboxes',
        '#options' => array(
            'Reviewed' =>
            'Reviewed (packets that have at least one review for any article)',
            'Unreviewed' =>
            'Unreviewed (packets that have no reviews for any article)',
        ),
        '#default_value' => array('Reviewed', 'Unreviewed'),
        '#title' => 'Review Statuses To Include:',
    );
    $form['submit'] = array(
        '#type' => 'submit',
        '#value' => 'Filter',
    );
    $form['reset'] = array(
        '#type' => 'submit',
        '#value' => '  Reset  ',
    );

    // Modify default values to conform to previous search if needed
    if ($search_id > 0)
        $form = EbmsReview::apply_manager_packet_filter_search($form,
                                                               $search_id);

    return $form;
}

/**
 * Submit function gathers the info into a JSON string and inserts it in
 * the database.  Then re-invokes the top level function with the row ID
 * of the row in ebms_search containing the filter criteria.
 */
function pdq_ebms_manager_packets_filter_form_submit (
        $form, &$form_state) {

    global $user;

    // Reset just repaint the screen from scratch
    if (trim($form_state['values']['op']) == 'Reset')
        drupal_goto(EbmsReview::$url . '/packets');

    // Prepare parms to store
    $filter_spec = drupal_json_encode($form_state['values']);
    $now         = db_query('SELECT NOW()')->fetchField();
    $uid         = $user->uid;

    // Save 'em in the database for access in the goto/callback
    $ins = db_insert('ebms_search')
             ->fields(array(
                         'when_searched' => $now,
                         'searched_by'   => $uid,
                         'search_spec'   => $filter_spec));
    $search_id = $ins->execute();

    // Re-invoke the entry point for packet reviewing with the search ID
    // Using "search" + id to avoid any future conflicts with use of the
    //  article_id parameter after "/packets/"
    drupal_goto(EbmsReview::$url . "/packets/search/$search_id");
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
            $memberOptions = EbmsReview::reviewers_with_work($boardId);
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

    $form['cancel_button'] = array(
        '#type' => 'submit',
        '#value' => 'Cancel',
        '#limit_validation_errors' => array(),
        '#submit' => array('pdq_ebms_record_responses_form_cancel'),
    );

    $form['search_button'] = array(
        '#type' => 'submit',
        '#value' => 'Search',
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

function _get_query_string(SelectQueryInterface $query) {
  $string = (string) $query;
  $arguments = $query->arguments();

  if (!empty($arguments) && is_array($arguments)) {
    foreach ($arguments as $placeholder => &$value) {
      if (is_string($value)) {
        $value = "'$value'";
      }
    }

    $string = strtr($string, $arguments);
  }

  return $string;
}

/**
 * Retrieve and decode any packet search filtering criteria.
 * Subroutine of manager_packets().
 *
 * @param int $search_id    Row ID in the ebms_search table.
 *
 * @return                  Array of search key=>values.
 *                          Empty array if none.
 */
function pdq_ebms_get_filter_criteria($search_id) {

    $fltr_json = null;
    try {
        $fltr_json = db_select('ebms_search', 's')
            ->fields('s', array('search_spec'))
            ->condition('search_id', $search_id)
            ->execute()
            ->fetchField();
    }
    catch (\Exception $e) {
        watchdog('review.inc',
            "Database error fetching filter criteria: $e");
        drupal_set_message('Lost filter criteria, tell support staff');
    }
    if ($fltr_json)
        return json_decode($fltr_json, true);
    else
        return array();
}

/**
 * Throttle the display of packets for which reviews have been submitted.
 * Added for JIRA ticket OCEEBMS-182.
 */
function ebms_reviewed_packets_filter_form($form_id, $form_state, $opts) {

    // Make sure the current user is connected with at least one board.
    $board_ids = EbmsReview::get_board_ids();
    if (count($board_ids) < 1)
        return array();

    // Get a list of all the board members for those boards.
    $reviewers = array();
    foreach ($board_ids as $board_id)
        $reviewers += EbmsReview::board_members($board_id);
    natcasesort($reviewers);

    // Get a list of all of the topics for those boards.
    $topics = db_select('ebms_topic', 't')
        ->fields('t', array('topic_id', 'topic_name'))
        ->condition('t.board_id', $board_ids, 'IN')
        ->orderBy('t.topic_name')
        ->execute()
        ->fetchAllKeyed();

    // Initialize the default form values to empty.
    $topic_defaults = array();
    $title = $start_date = $end_date = '';
    $reviewer_defaults = array();

    // If we're handling a filter request, adjust the default values.
    if (!empty($opts['query']['filter'])) {
        $filter_id = $opts['query']['filter'];
        $filter = pdq_ebms_get_filter_criteria($filter_id);
        foreach ($filter['topics'] as $id => $val) {
            if ($val && array_key_exists($id, $topics))
                $topic_defaults[] = $id;
        }
        foreach ($filter['reviewers'] as $id => $val) {
            if ($val && array_key_exists($id, $reviewers))
                $reviewer_defaults[] = $id;
        }
        $title = $filter['packet_title_text'];
        $start_date = $filter['date-start']['date'];
        $end_date = $filter['date-end']['date'];
        if ($start_date)
            $start_date = Ebms\Util::fill_in_date($start_date);
        if ($end_date)
            $end_date = Ebms\Util::fill_in_date($end_date, true);
    }

    // Build and return the Drupal render array for the filtering form.
    $date_fields = Ebms\Util::date_range_field('Review Date:', 'date',
                                               $start_date, $end_date,
                                               ':+0', false);
    return array(
        '#id' =>'reviewed-packets-filter-form',
        'title' => array(
            '#type' => 'markup',
            '#markup' => '<h3>Filter Packet Selection</h3>',
        ),
        'topic_box' => array(
            '#type' => 'fieldset',
            '#title' => 'Summary topics',
            '#collapsible' => true,
            '#collapsed' => true,
            'topics' => array(
                '#type' => 'checkboxes',
                '#options' => $topics,
                '#default_value' => $topic_defaults,
            ),
        ),
        'reviewer_box' => array(
            '#type' => 'fieldset',
            '#title' => 'Reviewers',
            '#collapsible' => true,
            '#collapsed' => true,
            'reviewers' => array(
                '#type' => 'checkboxes',
                '#options' => $reviewers,
                '#default_value' => $reviewer_defaults,
            ),
        ),
        'packet_title_text' => array(
            '#type' => 'textfield',
            '#size' => 40,
            '#title' => 'Packet Name: ',
            '#title_display' => 'before',
            '#default_value' => $title,
        ),
        'date-limits' => $date_fields,
        'buttons' => array(
            '#type' => 'container',
            '#attributes' => array('class' => array('button-wrapper')),
            'submit' => array(
                '#type' => 'submit',
                '#value' => 'Filter',
            ),
            'cancel' => array(
                '#type' => 'submit',
                '#value' => 'Reset',
            ),
        ),
    );
}

/**
 * Submit function gathers the info into a JSON string and inserts it in
 * the database.  Then re-invokes the top level function with the row ID
 * of the row in ebms_search containing the filter criteria.
 */
function ebms_reviewed_packets_filter_form_submit($form, &$form_state) {

    global $user;

    // Start fresh with no filtering if the user clicks the 'Reset' button.
    if (trim($form_state['values']['op']) == 'Reset')
        drupal_goto(EbmsReview::$url . '/reviews');

    // Save the filter options so we can use them to find the right packets.
    $filter_spec = drupal_json_encode($form_state['values']);
    $parms       = drupal_get_query_parameters();
    $now         = db_query('SELECT NOW()')->fetchField();
    $uid         = $user->uid;
    $fields      = array(
        'when_searched' => $now,
        'searched_by'   => $uid,
        'search_spec'   => $filter_spec
    );
    $filter_id = db_insert('ebms_search')->fields($fields)->execute();

    // Rebuild the reviewed packets page with the new filter options.
    $opts = array('query' => array('filter' => $filter_id));
    $opts['query']['pager'] = 'off';
    if (!empty($parms['boards']))
        $opts['query']['boards'] = $parms['boards'];
    drupal_goto(EbmsReview::$url . "/reviews", $opts);
}

/**
 * Throttle the display of packets for which reviews have not been submitted.
 * Added for JIRA ticket OCEEBMS-550.
 */
function ebms_unreviewed_packets_filter_form($form_id, $form_state, $opts) {

    // Make sure the current user is connected with at least one board.
    $board_ids = EbmsReview::get_board_ids();
    if (count($board_ids) < 1)
        return array();

    // Get a list of all the board members for those boards.
    $reviewers = array();
    foreach ($board_ids as $board_id)
        $reviewers += EbmsReview::board_members($board_id);
    natcasesort($reviewers);

    // Get a list of all of the topics for those boards.
    $topics = db_select('ebms_topic', 't')
        ->fields('t', array('topic_id', 'topic_name'))
        ->condition('t.board_id', $board_ids, 'IN')
        ->orderBy('t.topic_name')
        ->execute()
        ->fetchAllKeyed();

    // Initialize the default form values to empty.
    $topic_defaults = array();
    $title = $start_date = $end_date = '';
    $reviewer_defaults = array();

    // If we're handling a filter request, adjust the default values.
    if (!empty($opts['query']['filter'])) {
        $filter_id = $opts['query']['filter'];
        $filter = pdq_ebms_get_filter_criteria($filter_id);
        foreach ($filter['topics'] as $id => $val) {
            if ($val && array_key_exists($id, $topics))
                $topic_defaults[] = $id;
        }
        foreach ($filter['reviewers'] as $id => $val) {
            if ($val && array_key_exists($id, $reviewers))
                $reviewer_defaults[] = $id;
        }
        $title = $filter['packet_title_text'];
        $start_date = $filter['date-start']['date'];
        $end_date = $filter['date-end']['date'];
        if ($start_date)
            $start_date = Ebms\Util::fill_in_date($start_date);
        if ($end_date)
            $end_date = Ebms\Util::fill_in_date($end_date, true);
    }

    // Build and return the Drupal render array for the filtering form.
    $date_fields = Ebms\Util::date_range_field('Packet Created:', 'date',
                                               $start_date, $end_date,
                                               ':+0', false);
    return array(
        '#id' =>'unreviewed-packets-filter-form',
        'title' => array(
            '#type' => 'markup',
            '#markup' => '<h3>Filter Packet Selection</h3>',
        ),
        'topic_box' => array(
            '#type' => 'fieldset',
            '#title' => 'Summary topics',
            '#collapsible' => true,
            '#collapsed' => true,
            'topics' => array(
                '#type' => 'checkboxes',
                '#options' => $topics,
                '#default_value' => $topic_defaults,
            ),
        ),
        'reviewer_box' => array(
            '#type' => 'fieldset',
            '#title' => 'Assigned Reviewers',
            '#collapsible' => true,
            '#collapsed' => true,
            'reviewers' => array(
                '#type' => 'checkboxes',
                '#options' => $reviewers,
                '#default_value' => $reviewer_defaults,
            ),
        ),
        'packet_title_text' => array(
            '#type' => 'textfield',
            '#size' => 40,
            '#title' => 'Packet Name: ',
            '#title_display' => 'before',
            '#default_value' => $title,
        ),
        'date-limits' => $date_fields,
        'buttons' => array(
            '#type' => 'container',
            '#attributes' => array('class' => array('button-wrapper')),
            'submit' => array(
                '#type' => 'submit',
                '#value' => 'Filter',
            ),
            'cancel' => array(
                '#type' => 'submit',
                '#value' => 'Reset',
            ),
        ),
    );
}

/**
 * Submit function gathers the info into a JSON string and inserts it in
 * the database.  Then re-invokes the top level function with the row ID
 * of the row in ebms_search containing the filter criteria.
 */
function ebms_unreviewed_packets_filter_form_submit($form, &$form_state) {

    global $user;

    // Start fresh with no filtering if the user clicks the 'Reset' button.
    if (trim($form_state['values']['op']) == 'Reset')
        drupal_goto(EbmsReview::$url . '/unreviewed');

    // Save the filter options so we can use them to find the right packets.
    $filter_spec = drupal_json_encode($form_state['values']);
    $parms       = drupal_get_query_parameters();
    $now         = db_query('SELECT NOW()')->fetchField();
    $uid         = $user->uid;
    $fields      = array(
        'when_searched' => $now,
        'searched_by'   => $uid,
        'search_spec'   => $filter_spec
    );
    $filter_id = db_insert('ebms_search')->fields($fields)->execute();

    // Rebuild the unreviewed packets page with the new filter options.
    $opts = array('query' => array('filter' => $filter_id));
    $opts['query']['pager'] = 'off';
    if (!empty($parms['boards']))
        $opts['query']['boards'] = $parms['boards'];
    drupal_goto(EbmsReview::$url . "/unreviewed", $opts);
}

/**
 * Assemble the HTML markup for a packet's star, to be displayed on the
 * board manager's Reviewed Packets page.
 * See https://tracker.nci.nih.gov/browse/OCEEBMS-350.
 */
function pdq_ebms_packet_star_span($packet_id, $starred) {
    if ($starred) {
        $class = 'class="starred-packet"';
        $char = '&#x2605;';
        $action = 'off';
    }
    else {
        $class = 'class="unstarred-packet"';
        $char = '&#x2606;';
        $action = 'on';
    }
    $args = "\"$packet_id\",\"$action\"";
    $onclick = "onclick='ebmsscript.flip_packet_star($args);'";
    return "<span $class $onclick>$char</span>";
}

/**
 * Called by AJAX invocation from ebmsscript.flip_packet_star(),
 * defined in js/ebms.js. Registered by the menu to be invoked
 * with the URL /packet-star/{packet-id}/{on-or-off-flag}.
 * Update the database and return the HTML markup for the replaced
 * star on the board manager's Reviewed Packets page.
 * See https://tracker.nci.nih.gov/browse/OCEEBMS-350.
 */
function pdq_ebms_packet_star($packet_id, $state) {
    $starred = $state == 'on' ? 1 : 0;
    db_update('ebms_packet')
        ->fields(array('starred' => $starred))
        ->condition('packet_id', $packet_id)
        ->execute();
    echo pdq_ebms_packet_star_span($packet_id, $starred);
    exit(0);
}

/**
 * Ajax callback for the button on a reviewer's packet page to
 * report that an article should not be included in any further
 * consideration by the board. In the unlikely event that the
 * user has disabled javascript, the form will be drawn on
 * on a separate web page. Otherwise (the normal case) the
 * form will appear in a popup hovering above the packet page.
 * OCEEBMS-438.
 */
function pdq_ebms_modal_quick_no($js=false) {

    // This is the form we're using, whether or not javascript is enabled.
    $form = 'quick_no_form';

    // If javascript is turned off, bring up a separate page for the form.
    if (!$js)
        return drupal_get_form($form);

    // Requirements have changed, we need more information for the title bar.
    $parms = drupal_get_query_parameters();
    $article_id = $parms['article'];
    $xml = db_select('ebms_article', 'a')
        ->fields('a', array('source_data'))
        ->condition('a.article_id', $article_id)
        ->execute()
        ->fetchField();
    $doc = new \SimpleXMLElement($xml);
    $citation = $doc->MedlineCitation;
    $article = $citation->Article;
    $year = $article->Journal->JournalIssue->PubDate->Year;
    $author = '';
    if (!empty($article->AuthorList->Author)) {
        foreach ($article->AuthorList->Author as $a) {
            $author = ' ' . htmlspecialchars($a->LastName);
            break;
        }
    }
    $journal = htmlspecialchars($citation->MedlineJournalInfo->MedlineTA);
    $pmid = $citation->PMID;

    // Bring in support for ctools' ajax and modal popups.
    ctools_include('ajax');
    ctools_include('modal');

    // Set up the form state and wrap it for a modal popup form.
    $title = "Rejecting article$author $journal $year PMID $pmid";
    $form_state = array(
        'ajax' => true,
        'title' => $title,
    );
    $output = ctools_modal_form_wrapper($form, $form_state);

    // If the form was submitted, close the dialog and update
    // the packet page to replace the review buttons with
    // "reviewed [today's date].
    if (!empty($form_state['executed'])) {
        $article_id = $form_state['values']['article'];
        $span_id = "#article-buttons-$article_id";
        $today = date('m/d/Y');
        $html = "<i>reviewed<br>$today</i>";
        $output = array(
            ajax_command_html($span_id, $html),
            ctools_modal_command_dismiss(),
        );
    }

    // Otherwise we return the javascript for drawing the ajax form.
    elseif (!empty($form_state['ajax_commands']))
        $output = $form_state['ajax_commands'];
    print ajax_render($output);
    drupal_exit();
}

/**
 * Assemble the fields for an abbreviated form to be used in a popup
 * for quickly dispatching with an article which the reviewer believes
 * should be excluded from further consideration by the board.
 */
function quick_no_form($form, &$form_state) {
    $parms = drupal_get_query_parameters();
    return array(
        'packet' => array(
            '#type' => 'hidden',
            '#value' => $parms['packet'],
        ),
        'article' => array(
            '#type' => 'hidden',
            '#value' => $parms['article'],
        ),
        'reasons' => array(
            '#type' => 'checkboxes',
            '#title' =>
            'Reason(s) for Exclusion From PDQ<sup>&reg;</sup> Summary',
            '#description' =>
            'Please indicate which of the reasons below led to your ' .
            'decision to exclude the article. You may choose more than ' .
            'one reason.',
            '#options' => EbmsReview::reason_options(),
            '#prefix' => '<div id="reasons-wrapper">',
            '#suffix' => '</div>',
        ),
        'comments' => array(
            '#type' => 'textarea',
            '#resizable' => false,
            '#title' => 'Comments',
            '#description' =>
            'If you have additional comments, please add them here.',
        ),
        'submit' => array(
            '#type' => 'submit',
            '#value' => 'SUBMIT',
            '#submit' => array('quick_no_form_submit'),
            '#executes_submit_callback' => true,
        ),
        '#validate' => array('quick_no_form_validate'),
    );
}

/**
 * Verify that at least one reason for excluding the article is
 * selected.
 */
function quick_no_form_validate($form, &$form_state) {

    $reason_count = 0;
    $reasons = $form_state['values']['reasons'];
    foreach ($reasons as $id => $val)
        if ($val)
            $reason_count++;
    if (!$reason_count)
        form_set_error('reasons',
            'At least one exclusion reason must be selected.');
}

/**
 * Store the board member's feedback on this article.
 */
function quick_no_form_submit($form, &$form_state) {

    // check for this page being used on behalf of another
    global $user;
    $comment_suffix = '';
    $account = pdq_ebms_find_effective_user(false);
    if ($account) {
        // append the name of the intended user and add the manager's left
        // nav.  TIR 2409: don't use HTML markup to do this, since we're
        // appending to raw user input.
        // 2019-08-12 - overturned by OCEEBMS-451: store with markup.
        $uname = $user->name;
        $aname = $account->name;
        $comment_suffix = "<i>Recorded by $uname on behalf of $aname</i>";
    }
    else
        $account = $user;

    $packet_id = $form_state['values']['packet'];
    $article_id = $form_state['values']['article'];
    $comments = trim($form_state['values']['comments']);
    if (empty($comments))
        $comments = $comment_suffix;
    else
        $comments .= "<br><br>$comment_suffix";
    $now = strftime("%Y-%m-%d %H:%M:%S");
    $review_id = db_insert('ebms_article_review')
        ->fields(
            array(
                'packet_id'   => $packet_id,
                'article_id'  => $article_id,
                'reviewer_id' => $account->uid,
                'when_posted' => $now,
                'comments'    => $comments,
                'loe_info'    => '',
            )
        )
        ->execute();
    pdq_ebms_debug('QUICK NO REVIEW ID', $review_id);
    db_update('ebms_packet_article')
        ->fields(array('archived' => null))
        ->condition('article_id', $article_id)
        ->condition('packet_id', $packet_id)
        ->execute();

    // Look up the ID of the disposition for rejecting the article
    // (always in the first position).
    $no_change_option = db_select('ebms_review_disposition_value', 'v')
            ->fields('v', array('value_id'))
            ->condition('v.value_pos', 1)
            ->execute()
            ->fetchField();
    db_insert('ebms_review_disposition')
        ->fields(
            array(
                'review_id' => $review_id,
                'value_id' => $no_change_option,
            )
        )
        ->execute();
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
}

/**
 * Assemble the manager packets table with bulk archive checkboxes.
 *
 *  @param  array   $form         Structured array containing the
 *                                elements and properties of the form
 *  @param  array   &$form_state  Modifiable structured array
 *                                containing the current values
 *                                of the form, as well as other
 *                                form state information
 *  @param  int     $search_id    Primary key for ebms_search table
 *  @return array                 Render array usable by FAPI
 */
function ebms_packet_bulk_archive($form, &$form_state, $search_id) {
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
    $boards = EbmsReview::get_board_ids();

    // Build the query to find the manager's packets.
    $query = db_select('ebms_packet', 'p');
    $pager = Ebms\Util::get_request_value('pager');
    if ($pager != 'off')
        $query = $query->extend('PagerDefault')->limit(10);
    $query = $query->extend('TableSort');
    $query->join('ebms_topic', 't', 't.topic_id = p.topic_id');
    $query->join('ebms_board', 'b', 'b.board_id = t.board_id');
    $query->join('users', 'u', 'u.uid = p.created_by');
    $query->distinct();
    #if (Ebms\Util::get_request_value('reviewed')) {
    #    $query->join('ebms_article_review', 'r',
    #        'r.packet_id = p.packet_id');
    #}
    $query->fields('p', array('packet_id', 'packet_title', 'created_at'));
    $query->fields('b', array('board_name'));
    $query->fields('u', array('name'));
    if (count($boards) == 1)
        $query->condition('b.board_id', reset($boards));
    elseif (count($boards) > 1)
        $query->condition('b.board_id', $boards, 'IN');
    $query = EbmsReview::apply_manager_packet_filters($query, $search_id);
    $query->orderByHeader($headers);
    $rows = $query->execute();

    // Assemble the packet rows.
    $divider = '<span class="action-divider">|</span>';
    $main_url = EbmsReview::$url;
    $del_func = 'javascript:ebmsscript.delete_packet';
    $opts = array('query' => drupal_get_query_parameters());
    $packets = array();
    foreach ($rows as $p) {
        $edit_url = "$main_url/edit/{$p->packet_id}";
        $edit_link = l('EDIT', $edit_url, $opts);
        $del_parm = url("$main_url/delete/{$p->packet_id}", $opts);
        $del_url = "$del_func('$del_parm');";
        $del_link = '<a href="' . $del_url . '">ARCHIVE</a>';
        $title = str_replace(' [Legacy]', '', $p->packet_title);
        $title .= " [Packet #{$p->packet_id}]";
        $title_link = l(htmlspecialchars($title), $edit_url, $opts);
        $packets[$p->packet_id] = array(
            $title_link,
            htmlspecialchars($p->board_name),
            htmlspecialchars($p->name),
            date('m/d/Y', strtotime($p->created_at)),
            "$edit_link $divider $del_link",
        );
    }

    // Put it all together in a FAPI-ready render array.
    $view = EbmsReview::manager_packets_view_toggle($opts);
    $add = l('Add New', EbmsReview::$url . '/add', $opts);
    $title = 'Literature Surveillance Packets';
    $id = 'manager-packets-page-title';
    return array(
        'archive' => array(
            '#type' => 'submit',
            '#value' => 'ARCHIVE',
            '#suffix' => '<span>Consider clicking VIEW ALL before ' .
            'selecting documents to archive.',
        ),
        'title' => array(
            '#type' => 'markup',
            '#markup' => "<h2 id='$id'>$title $add $view</h2>",
        ),
        'packets' => array(
            '#attributes' => array('id' => 'manager-packets-table'),
            '#type' => 'tableselect',
            '#options' => $packets,
            '#header' => $headers,
            '#empty' => t('No packets found.'),
        ),
        'delete-confirm' => array(
            '#type' => 'container',
            '#attributes' => array(
                'id' => 'confirm-packet-delete',
                'title' => 'Archive Packet?',
            ),
            'confirm-message' => array(
                '#type' => 'markup',
                '#markup' =>
                '<p>Archiving this packet will remove it from the queues'
                . ' of any reviewers who have not responded.</p>'
            ),
        ),
    );
}

/**
 * Make sure packets have been selected for archiving.
 *
 * This is only invoked if we have already detected that no packets
 * have been selected to be archived. An odd way to do things, but
 * we're following the lead of Drupal core.
 *
 *  @param  array   $form         Structured array containing the
 *                                elements and properties of the form
 *  @param  array   &$form_state  Modifiable structured array
 *                                containing the current values
 *                                of the form, as well as other
 *                                form state information
 *  @param  int     $search_id    Primary key for ebms_search table
 *  @return array                 Render array usable by FAPI
 */
function ebms_packet_bulk_archive_validate($form, &$form_state) {
    $packets = array_diff($form_state['values']['packets'], [0]);
    if (empty($packets))
        form_set_error('', t('No packets selected for archiving.'));
}

/**
 * This method will never be invoked, as far as I can tell.
 */
function ebms_packet_bulk_archive_submit($form, &$form_state) {
    $debug_values = print_r($form_state['values'], true);
    drupal_set_message("archive submit: $debug_values");
}

/**
 * Give the user a chance to back away from a bulk archive request.
 *
 * We route directly to this form ourselves, based on an
 * examination of the global $_POST values, following the
 * lead of Drupal core.
 *
 *  @param  array   $form         Structured array containing the
 *                                elements and properties of the form
 *  @param  array   &$form_state  Modifiable structured array
 *                                containing the current values
 *                                of the form, as well as other
 *                                form state information
 *  @return array                 Render array usable by FAPI
 */
function ebms_packet_bulk_archive_confirm($form, &$form_state) {

    // Show the user where she's been.
    drupal_set_breadcrumb(
        array(
            l('Literature', EbmsReview::$url . '/reviews'),
            l('View/Edit Packets', '/review/packets'),
            'Bulk Packet Archive',
        )
    );

    // Put our own title up (EBMS CSS suppresses display of the
    // Drupal page title).
    $title = t('Are your sure you want to archive these packets?');
    $title_id = 'packet-bulk-archive-confirm-title';

    // Assemble a form to remind the user what she's asked to archive.
    $form = array(
        'packets' => array(
            '#prefix' => "<h1 id=\"$title_id\">$title</h1><ul>",
            '#suffix' => '</ul>',
            '#tree' => true,
        ),
    );
    $main_url = EbmsReview::$url;
    $opts = array('query' => drupal_get_query_parameters());
    $count = 0;
    $input = $form_state['input'];
    foreach (array_filter($input['packets']) as $pid => $value) {
        $query = db_select('ebms_packet', 'p');
        $query->fields('p', array('packet_title'));
        $query->condition('p.packet_id', $pid);
        $title = htmlspecialchars($query->execute()->fetchField());
        $title = str_replace(' [Legacy]', '', $title);
        $url = "$main_url/edit/{$pid}";
        $link = l("$title [Packet #$pid]", $url, $opts);
        $form['packets'][$pid] = array(
            '#type' => 'hidden',
            '#value' => $pid,
            '#prefix' => '<li>',
            '#suffix' => "$link</li>",
        );
        ++$count;
    }

    // This could only happen in the very unlikely event that another
    // user snuck in an archive in the very tiny window between clicking
    // the Archive button and drawing this page.
    if (!$count) {
        drupal_set_message('No packets selected to be archived.');
        drupal_goto('review/packets');
    }

    // Put up the confirmation form.
    $question = 'Really archive packets?';
    $form['operation'] = array('#type' => 'hidden', '#value' => 'archive');
    $path = 'review/packets';
    $description = '<span class="packet-bulk-archive-confirm-description">' .
            t('Archiving these packets will remove them from the queues ' .
            'of any reviewers who have not yet responded.') . '</span>';
    $yes = t('Archive Packets');
    $no = t('Cancel');
    return confirm_form($form, $question, $path, $description, $yes, $no);
}

/**
 * When the user clicks the $yes button from the confirm_form() call
 * this submit handler should be invoked, according to the documentation
 * (https://api.drupal.org/api/drupal/modules!system!system.module/function/confirm_form/7.x)
 * but that isn't happening. So we're handling the requested bulk
 * archive request in the needs_packet_bulk_archive_confirmation()
 * method near the top of this file.
 */
function ebms_packet_bulk_archive_confirm_submit($form, &$form_state) {
    drupal_set_message('confirm submit: ' .  print_r($form_state, true));
}
