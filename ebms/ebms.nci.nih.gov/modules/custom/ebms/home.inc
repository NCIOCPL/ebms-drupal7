<?php

// $Id$

/**
 * @file
 *
 * Implementation of EBMS Home page.
 */

/**
 * Callback registered with the Drupal menu system.  This is the default
 * landing page, so there are no access restrictions.  Any user who is
 * logged in can view this page.  A visitor who is not logged in is
 * redirected to the login page.
 *
 * 2012-12-18 (RMK): that last bit (the redirect to the login page) is
 * now handled earlier, in ebms_init().
 */
function pdq_ebms_home() {
    $page = new EbmsHomePage();
    return $page->build();
}
/**
 * Wrapper for logic to build the home pages for the different roles.
 * Keeps our functions out of the global namespace with less ugly
 * syntax than the PHP namespace mechanism provides, and lets us
 * pass around the current state information more easily.
 */
class EbmsHomePage {

    private
        $user,
        $focus,
        $role,
        $url;

    /**
     * Fetch the parameters passed in with the request, along with other
     * information we'll need for building the home page.
     */
    public Function __construct() {
        global $user;
        $this->url = 'home';
        $this->user = $user;
        $focus = Ebms\Util::get_request_value('focus', '1');
        $this->focus = Ebms\Util::validate_int($focus, $this->url);
        $this->role = user_access('administer') ? 'admin' : 'member';
        if (in_array('medical librarian', $user->roles))
            $this->role = 'librarian';
        //if (in_array('board manager', $user->roles))
        //    $this->role = 'board manager';
        $this->mini = Ebms\Util::get_request_value('mini');
        if ($this->mini) {
            if (!preg_match('/^\d\d\d\d-\d\d$/', $this->mini)) {
                drupal_set_message('Invalid parameter', 'error');
                drupal_goto($this->url);
            }
        }
    }

    /**
     * Assemble the information to be shown on the home page, returned
     * as a Drupal 7 render array.  The home page is divided between
     * the main panel ("content") and a right-hand sidebar.  The
     * centerpiece of the page is a set of sliding images representing
     * the primary tasks performed by the current user, with links
     * to the pages for those tasks.  The remainder of the page is
     * made up of boxes of important information.  In theory, those
     * boxes would have been implemented using Drupal's Block API,
     * but at the time of initial implementation of this system
     * the Block API is so poorly documented and buggy that we
     * decided to implement the boxes without it.  The primary
     * benefit provided by use of the Block API is the ability of
     * administrator to control whether and where the blocks
     * are displayed from an administrative drag-and-drop interface.
     * If the blocks on this page need to be rearranged, it should
     * be much more straightforward to adjust the data structures
     * below than to try and work around the bugs in the Block API.
     * Redesigned for JIRA ticket OCEEBMS-169.
     */
    public function build() {
        drupal_set_title('');
        drupal_set_breadcrumb(array('Home'));
        $activity_panels = $this->activity_panels();
        $panel_count = count($activity_panels);
        $class = "panels-$panel_count";
        $content = array(
            '#prefix' => "<div id='left-panels' class='$class'>",
            '#suffix' => '</div>',
        );
        foreach ($activity_panels as $name => $panel)
            $content[$name] = $panel;
        return array(
            '#theme' => 'page',
            '#type' => 'page',
            '#show_messages' => true,
            'content' => $content,
            'sidebar_second' => array(
                '#prefix' => '<div id="right-panels">',
                '#suffix' => '</div>',
                'blocks' => $this->right_panel(),
            ),
        );
    }

    /**
     * Choose the activity panels shown on the home page for this role.
     */
    private function activity_panels() {
        switch ($this->role) {
            case 'member':
                return array(
                    'literature' => $this->literature_activity(),
                    'meetings' => $this->meeting_activity(),
                );
            case 'librarian':
                return array(
                    'librarian' => $this->librarian_activity(),
                    //'literature' => $this->literature_activity(),
                    //'documents' => $this->document_activity(),
                );
            case 'admin':
            default:
                return array(
                    'literature' => $this->literature_activity(),
                    'documents' => $this->document_activity(),
                    'meetings' => $this->meeting_activity(),
                );
        }
    }

    /**
     * Assemble Drupal reneder array for Literature Activity panel.
     */
    private function literature_activity() {
        $title = 'Literature Activity';
        $id = 'literature-activity';
        $items = array();
        $more = '';
        if ($this->role != 'admin')
            $img = Ebms\IMAGES_DIR . '/literature-activity-2-panel.jpg';
        else
            $img = Ebms\IMAGES_DIR . '/literature-activity-3-panel.jpg';
        $nada = '<p>No recent literature activity</p>';
        $view = views_get_view('recent_activity');
        $content = $view ? $view->preview('literature_activity_block') : $nada;
        return $this->activity_box2($id, $title, $img, $content);
        switch ($this->role) {
            case 'member':
                $query = db_select('ebms_packet', 'p');
                $query->fields('p', array('packet_title', 'created_at'));
                $query->join('ebms_topic_reviewer', 't',
                    't.topic_id = p.topic_id');
                $query->condition('t.user_id', $this->user->uid);
                $query->range(0, 4);
                $query->orderBy('p.created_at', 'DESC');
                $results = $query->execute();
                foreach ($results as $result) {
                    $when = date('m/d/Y', strtotime($result->created_at));
                    if (count($items) > 2)
                        $more = l('more', 'literature-activity');
                    else
                        $items[] = "$result->packet_title posted $when";
                }
                break;
            case 'admin':
            default:
                $items[] = 'New Cancer Genetics articles posted. ' .
                    '06/20/2014';
                break;
        }
        return $this->activity_box($id, $title, $img, $items, $more);
    }

    private function activity_box2($id, $title, $img, $content) {
        $h3 = "<h3>$title</h3>";
        $image = "<div class='image-wrapper'><img src='$img' alt=''></div>";
        $box = array(
            '#type' => 'container',
            '#attributes' => array(
                'id' => $id,
                'class' => array('activity-box'),
            ),
            'top' => array('#markup' => $h3 . $image),
            'content' => array('#markup' => $content),
        );
        /*
        if ($more)
            $box['more'] = array(
                '#markup' => "<p class='more'>$more</p>",
            );
    */
        return $box;
    }
    /**
     * Assemble Drupal reneder array for Meeting Activity panel.
     */
    private function meeting_activity() {
        if ($this->role != 'admin')
            $img = Ebms\IMAGES_DIR . '/meeting-activity-2-panel.jpg';
        else
            $img = Ebms\IMAGES_DIR . '/meeting-activity-3-panel.jpg';
        $id = 'meeting-activity';
        $title = 'Meeting Activity';
        $nada = '<p>No recent meeting activity</p>';
        $view = views_get_view('recent_activity');
        $content = $view ? $view->preview('meeting_activity_block') : $nada;
        //pdq_ebms_debug('MEETING ACTIVITY', array($view, $content));
        return $this->activity_box2($id, $title, $img, $content);
        return array(
            '#markup' => "<h3>Recent Activity</h3>" . $display,
            '#prefix' => "<div id='activity-enclosure' class='ebms-block'>",
            '#suffix' => /*$this->block_bottom_img() .*/ "</div>",
        );
        $more = l('more', 'meeting-activity');
        $items = array();
        switch ($this->role) {
            case 'member':
            case 'admin':
            default:
                $items = array(
                    'Genetics Board Meeting changed to Webex/Phone Conf ' .
                    '06/20/2014',
                    'Breast/Ovarian WB time updated 06/20/2014',
                    'Agenda published for Breast/Ovarian WG 06/20/2014',
                );
        }
        return $this->activity_box($id, $title, $img, $items, $more);
    }

    /**
     * Assemble Drupal reneder array for Document Activity panel.
     */
    private function document_activity() {
        if ($this->role != 'admin')
            $img = Ebms\IMAGES_DIR . '/document-activity-2-panel.jpg';
        else
            $img = Ebms\IMAGES_DIR . '/document-activity-3-panel.jpg';
        $more = l('more', 'document-activity');
        $id = 'document-activity';
        $title = 'Document Activity';
        $nada = '<p>No recent document activity</p>';
        $view = views_get_view('recent_activity');
        $content = $view ? $view->preview('document_activity_block') : $nada;
        //pdq_ebms_debug('MEETING ACTIVITY', array($view, $content));
        return $this->activity_box2($id, $title, $img, $content);
        $items = array();
        switch ($this->role) {
            case 'admin':
            default:
                $items = array(
                    'Mary Daly posted Genetics of Breast and Ovarian Cancer ' .
                    '4_28_14 - TBM edits + MD edits. 06/20/2014',
                    'Test Board Member posted Genetics of Breast and ' .
                    'Ovarian Cancer 4_28_14 - TBM edits + MD edits. ' .
                    '06/20/2014',
                    'Test Board Member posted Genetics of Breast and ' .
                    'Ovarian Cancer 4_28_14 - TBM edits + MD edits. ' .
                    '06/20/2014',
                    'Test Board Member posted Genetics of Breast and ' .
                    'Ovarian Cancer 4_28_14 - TBM edits + MD edits. ' .
                    '06/20/2014',
                );
                break;
        }
        return $this->activity_box($id, $title, $img, $items, $more);
    }

    /**
     * Assemble the Drupal render array for a single home page activity
     * box.
     */
    private function activity_box($id, $title, $img, $items, $more) {
        $h3 = "<h3>$title</h3>";
        $image = "<div class='image-wrapper'><img src='$img' alt=''></div>";
        $box = array(
            '#type' => 'container',
            '#attributes' => array(
                'id' => $id,
                'class' => array('activity-box'),
            ),
            'top' => array('#markup' => $h3 . $image),
            'content' => array(
                '#theme' => 'item_list',
                '#items' => $items,
            ),
        );
        if ($more)
            $box['more'] = array(
                '#markup' => "<p class='more'>$more</p>",
            );
        return $box;
    }

    /**
     * Assemble Drupal reneder array for Librarian Activity panel.
     */
    private function librarian_activity() {
        $img = Ebms\IMAGES_DIR . '/librarian.jpg';
        return array('#markup' => "<img src='$img' alt=''>");
        if ($this->role != 'admin')
            $img = Ebms\IMAGES_DIR . '/literature-activity-2-panel.jpg';
        else
            $img = Ebms\IMAGES_DIR . '/literature-activity-3-panel.jpg';
        $img = Ebms\IMAGES_DIR . '/member-activity-box.jpg';
        $more = l('more', 'librarian-activity');
        $id = 'libraian-activity';
        $title = 'Librarian Activity';
        $items = array();
        $items = array(
            'Lorem ipsum dolor sit amet, consectetur adipiscing elit. Donec suscipit dignissim elementum. Nulla nec dolor non velit suscipit convallis eget et ipsum. Pellentesque malesuada tempor interdum. Mauris venenatis justo nec velit gravida, eget vehicula quam lobortis. Nulla imperdiet risus id imperdiet malesuada. Cras imperdiet, eros sit amet dictum tincidunt, est diam interdum ligula, eu volutpat nulla augue at nunc. Vestibulum volutpat ipsum nisi, id tempus orci ultrices ac. Quisque a mi nec libero congue egestas. Fusce quis mollis nulla, eu tincidunt leo. Fusce porta mattis risus, in porta diam molestie suscipit. Donec egestas, neque ut interdum porttitor, lectus dui venenatis nisl, in mollis mauris lorem eget massa. Donec ultrices orci vel mi imperdiet viverra.',
            'Vivamus tristique posuere leo, id mattis nulla venenatis vitae. Curabitur interdum et erat pharetra congue. Nam ac tortor libero. Ut dignissim arcu in facilisis fringilla. Maecenas eget libero ac metus adipiscing tempus et varius purus. Proin tincidunt ligula nulla, ut ultricies magna bibendum convallis. Suspendisse purus ligula, blandit ac venenatis nec, porta eget quam. Etiam et sem dolor. Praesent vulputate libero arcu, nec pellentesque dolor vestibulum nec.',
        );
        return $this->activity_box($id, $title, $img, $items, $more);
    }

    /**
     * The right side panel has a stack of rectangular boxes
     * showing information of current interest.  As with the
     * main panel, the determination of which boxes are shown
     * is based on the role(s) of the current user.
     */
    private function right_panel() {
        if ($this->role == 'librarian')
            return array(
                // Librarians only get alerts for unread messages,
                // and the message feature has been deferred to a
                // later release.
                // 'alerts' => $this->alerts(),
                'calendar' => $this->calendar(),
                //'admin tasks' => $this->admin(),
            );
        $blocks = array(
            'alerts' => $this->alerts(),
            'next meeting' => $this->next_meeting(),
            'calendar' => $this->calendar(),
        );
        return $blocks;
    }

    /**
     * Create the home page box for displaying a list of recent
     * activities of interest to the current user.
     * XXX Replace stub activities when the related portions of the
     *     system have been implemented.
     */
    private function recent() {
        /*
        $events = array(
            array('Genetics Board meeting time updated.', '02/01/12'),
            array('New genetics articles posted.', '01/30/12'),
            array('New event Genetics Board event posted.', '01/23/12'),
        );
        foreach ($events as $e)
            $items[] = "{$e[0]} <span class='date nobreak'>{$e[1]}</span>";
        return $this->list_block('Recent Activity', 'recent-activity', $items);
        */

        $view = views_get_view('recent_activity');

        $display = "";

        if ($view) {
            $display = $view->preview('activity_block');
        }

        return array(
            '#markup' => "<h3>Recent Activity</h3>" . $display,
            '#prefix' => "<div id='activity-enclosure' class='ebms-block'>",
            '#suffix' => $this->block_bottom_img() . "</div>",
            );
    }

    /**
     * Create the box which contains links to administrative tasks.
     * XXX Replace stub URLs when the related portions of the system
     *     have been implemented.
     */
    private function admin() {
        global $user;
        switch ($this->role) {
            case 'librarian':
                $items = array(
                    l('Review Citations', 'citations'),
                    l('Search Database', 'citations/search'),
                    l('Import Citations', 'citations/import'),
                    l('Not List Maintenance', 'citations/not-list'),
                    l('Publish Citations', 'citations/publish'),
                );
                break;
            default:
                $items = array(
                    l('Post a Document', 'docs/post'),
                    l('Manage Documents', 'docs'),
                    l('Manage Groups', 'groups'),
                    l('Print Packets', 'print-packets/run'),
                    l('View Packet Printing Jobs', 'print-packets/view'),
                    l('Manage Print Recipients', 'print-packets/mbrs'),
                );
                break;
        }
        $title = 'Administrative Tasks';
        $id = 'admin';
        return $this->list_block($title, $id, $items, true);
    }

    /**
     * Create the home page box containing links to the board rosters.
     */
    private function rosters($middle=false) {
        $base = 'http://www.cancer.gov/cancertopics/pdq/';
        $attrs = array('attributes' => array('target' => '_blank'));
        $boards = array(
            array('Adult Treatment', 'adult-treatment-board'),
            array('Pediatric Treatment', 'pediatric-treatment-board'),
            array('Supportive and Palliative Care', 'supportive-care-board'),
            array('Screening and Prevention', 'screening-prevention-board'),
            array('Cancer Genetics', 'cancer-genetics-board'),
            array('Cancer Complementary and Alternative Medicine',
                'cancer-cam-board'),
        );
        foreach ($boards as $board)
            $items[] = l($board[0], $base . $board[1], $attrs);
        $query = db_select('file_managed', 'f');
        $query->join('ebms_doc', 'd', 'd.file_id = f.fid');
        $query->join('ebms_doc_tag', 't', 't.doc_id = d.doc_id');
        $query->join('ebms_tag', 'r', 'r.tag_id = t.tag_id');
        $query->fields('f', array('uri', 'filename'));
        $query->fields('d', array('description', 'doc_id'));
        $query->condition('r.tag_name', 'roster');
        $query->condition('d.drop_flag', 0);
        $query->orderBy('d.description');
        $results = $query->execute();
        foreach ($results as $result) {

            // Change request (TIR #2185): don't show document to user
            // if the document has been tagged with one or more boards
            // and the current user is not connected with any of those
            // boards.
            $board_rows = db_select('ebms_doc_board', 'b')
                ->fields('b', array('board_id'))
                ->condition('b.doc_id', $result->doc_id)
                ->execute();
            $board_ids = array();
            foreach ($board_rows as $board_row)
                $board_ids[] = $board_row->board_id;
            $wanted = true;
            if (count($board_ids) > 0) {
                $both = array_intersect($board_ids, $this->user->boards);
                if (count($both) < 1)
                    $wanted = false;
            }
            if ($wanted) {
                $item = l($result->description, file_create_url($result->uri));
                $items[] = $item;
            }
        }
        $title = 'PDQ<sup>&reg;</sup> Board Rosters';
        $id = 'board-rosters';
        return $this->list_block($title, $id, $items, $middle);
    }

    /**
     * Helper method to create the markup for a single line in
     * the Alerts box on the home page.
     */
    private static function alert($url, $label, $count) {
        $link = l($label, $url);
        return "$link <span class='alert-count'>$count</span>";
    }

    /**
     * Create the home page box showing counts of unread messages,
     * unread forum postings, and (for board members) articles
     * awaiting review or (for board managers) reviews which
     * have not yet been read.
     */
    private function alerts() {

        // Start with a blank slate.
        $total = 0;
        $items = array();

        // Deferred to a later release.
        // $total += $count = $this->get_message_count();
        // $items[] = EbmsHomePage::alert('messages', 'Messages', $count);

        // XXX FORUM COUNT METHOD TO BE IMPLEMENTED (BY LAUREN?)
        // $total += $count = $this->get_forum_count();
        // $items[] = EbmsHomePage::alert('forum', 'Forum', $count);

        // Show unreviewed articles for board members, unseen reviews
        // for board managers.
        // TIR 2342 (from Ashleigh): don't show review count to Bonnie
        global $user;
        if (!in_array('admin assistant', $user->roles)) {
            $total += $count = $this->get_review_count();
            $page = $this->role == 'member' ? 'assigned' : 'reviews';
            $items[] = EbmsHomePage::alert("review/$page", 'Reviews', $count);
        }
        if (module_exists('ebms_forums')) {
            $topic_ids = _ebms_forums_get_new_topics();
            $count = count($topic_ids);
            $url = $count ? "node/$topic_ids[0]" : 'forum';
            $items[] = EbmsHomePage::alert($url, "Forums", $count);
        }

        // Assemble and return the box.
        $title = "Alerts <span class='alert-count'>$total</span>";

        // They've changed their minds and don't want to see the
        // totals (TIR 2432).  Leaving the code above in place in case
        // they change their minds again.
        $title = 'Alerts';
        return $this->list_block($title, 'alerts', $items);
    }

    /**
     * Determines the count of unreviewed articles for a board member.
     * Otherwise, finds the count of board member reviews posted to packets
     * created by the current user since the last time the user looked
     * at the packet.
     */
    private function get_review_count() {

        // To count unreviewed articles, we need to know:
        //    1.  is the article in a packet assigned to this board member?
        //    2.  is the packet still active?
        //    3.  has the board member already posted a review for the article?
        if ($this->role == 'member') {
            $query = db_select('ebms_packet_article', 'a');
            $query->fields('a', array('article_id'));
            $query->join('ebms_packet', 'p', 'p.packet_id = a.packet_id');
            $query->join('ebms_packet_reviewer', 'm',
                'a.packet_id = m.packet_id');
            $query->leftJoin('ebms_article_review', 'r',
                implode(' AND ',
                    array(
                        'r.article_id = a.article_id',
                        'r.reviewer_id = m.reviewer_id',
                        'r.packet_id = a.packet_id',
                    )
                )
            );
            $query->condition('m.reviewer_id', $this->user->uid);
            $query->condition('p.active_status', 'A');
            $query->condition('a.drop_flag', 0);
            $query->isNull('r.article_id');
            $query->distinct();
            $query = $query->countQuery();
            return $query->execute()->fetchField();
        }

        // To find unseen reviews, we check:
        //    1. is the review for a packet this user created?
        //    2. Was the review posted since the last time the board
        //       manager looked at the packet?
        //    3. Is the packet still active?
        $query = db_select('ebms_article_review', 'r');
        $query->fields('r', array('review_id'));
        $query->join('ebms_packet', 'p', 'p.packet_id = r.packet_id');
        $query->condition('p.created_by', $this->user->uid);
        $query->condition('p.active_status', 'A');
        $query->where('(p.last_seen IS NULL OR r.when_posted > p.last_seen)');
        $query->distinct();
        $query = $query->countQuery();
        return $query->execute()->fetchField();
    }

    /**
     * Create the home page box highlighting the next meeting
     * scheduled for the current user.
     */
    private function next_meeting() {

        $view = views_get_view('event_calendar');

        $display = "";

        if ($view) {
            $display = $view->preview('block_next');
        }

        return array(
            '#markup' => "<h3>Next Meeting</h3>" . $display,
            '#prefix' => "<div id='activity-enclosure' class='ebms-block'>",
            '#suffix' => '</div>',
            );
    }

    /**
     * Show a small version of the current month's calendar,
     * highlighting days on which events visible to the current
     * user occur, with links to those events.  Allows the user
     * to navigate to different months.
     */
    private function calendar() {

        drupal_add_css(Ebms\CSS_DIR . '/calendar.css');

        $view = views_get_view('event_calendar');
        if(!$view)
            return array();
        $display = $view->preview('month_block');

        return array(
            '#markup' => $display,
            '#prefix' => "<div id='calendar-enclosure'>",
            '#suffix' => '</div>',
        );
    }

    /**
     * Assemble the Drupal render array for a home page block containing
     * a list of items.  If it turns out that we need similar blocks
     * on other pages in the system, this functionality will be moved
     * to common.inc and packaged in a separate class.
     */
    private function list_block($title, $id, $items, $middle=false) {
        $class = $middle ? 'ebms-block middle-block' : 'ebms-block';
        return array(
            '#theme' => 'item_list',
            '#title' => $title,
            '#items' => $items,
            '#attributes' => array('class' => 'ebms_block'),
            '#prefix' => '<div class="' . $class . '" id="' . $id . '">',
            '#suffix' => '</div>',
        );
    }
}
