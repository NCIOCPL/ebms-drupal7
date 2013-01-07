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
        $cal,
        $role,
        $url;

    /**
     * Fetch the parameters passed in with the request, along with other
     * information we'll need for building the home page.
     */
    public Function __construct() {
        global $user;
        $current_month = date('Y') . '-' . date('m');
        $this->user = $user;
        $this->focus = Ebms\Util::get_request_value('focus', '1');
        $this->cal = Ebms\Util::get_request_value('cal', $current_month);
        $this->role = user_access('administer') ? 'admin' : 'member';
        $this->url = 'home';
        if (in_array('medical librarian', $user->roles))
            $this->role = 'librarian';
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
     */
    public function build() {
        drupal_set_title('');
        drupal_set_breadcrumb(array('Home'));
        return array(
            '#theme' => 'page',
            '#type' => 'page',
            '#show_messages' => true,
            'content' => $this->main_panel(),
            'sidebar_second' => $this->right_panel(),
        );
    }

    /**
     * The main panel has three blocks.  The first block is the
     * sliding image block with links to tasks common performed
     * by the current user ("featured" task areas).  The other
     * two blocks are smaller blocks aligned next to each other.
     * The content of the blocks varies depending on the role
     * of the current user.
     */
    private function main_panel() {
        switch ($this->role) {
            case 'librarian':
                return array('featured' => $this->featured());
            case 'member':
                return array(
                    'featured' => $this->featured(),
                    'recent' => $this->recent(),
                    'rosters' => $this->rosters(true),
                );
            case 'admin':
                return array(
                    'featured' => $this->featured(),
                    'recent' => $this->recent(),
                    'admin' => $this->admin(),
                );
        }
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
                'admin tasks' => $this->admin(),
            );
        $blocks = array(
            'alerts' => $this->alerts(),
            'next meeting' => $this->next_meeting(),
            'calendar' => $this->calendar(),
        );
        if ($this->role == 'admin')
            $blocks['rosters'] = $this->rosters();
        return $blocks;
    }

    /**
     * The centerpiece of the home page is made up of a set of images,
     * only one of which is displayed at any given time.  The images
     * present the illusion of multiple stacked images, each representing
     * one of the primary areas of the system in which the current user
     * would work (for example, Citation Management or Literature
     * Surveillance).  By clicking on the portions of the images which
     * appear to represent another image underneath the one for the area
     * which currently has the focus, she can cause the image to change
     * so that another task area gets the focus.  In the portion of the
     * image for the featured task area links are available for navigating
     * to the pages used to perform the tasks in that portion of the system.
     * In most cases, these links duplicate navigation provided by the
     * main menu system.
     */
    private function featured() {
        $info = $this->featured_info($this->role, $this->focus);
        $id = "featured-{$this->role}-{$this->focus}";
        $prefix = $this->featured_prefix($id);
        foreach ($info['links'] as $link)
            $items[] = $link;
        return array(
            '#theme' => 'item_list',
            '#title' => $info['title'],
            '#items' => $items,
            '#prefix' => $prefix,
            '#suffix' => '</div>',
        );
    }

    /**
     * Assemble the information needed for displaying the image
     * for the home page's main block, as well as the linkable
     * areas of the image for "sliding" to another featured
     * task focus.
     */
    private function featured_prefix($id) {
        $div = "<div class='ebms-featured-block' id='$id'>";
        $url = Ebms\IMAGES_DIR . "/$id.jpg";
        $img = "<img src='$url' usemap='#featured-map' alt='' />";
        $map = array('<map id="featured-map" name="featured-map">');
        switch ($this->role) {
            case 'admin':
                $alts = array(
                    'Citation Management',
                    'Literature Surveillance',
                    'Summaries',
                    'Reports',
                );
                break;
            case 'librarian':
                $alts = array(
                    'Citation Management',
                    'Reports',
                    'Administrator Tool',
                );
                break;
            default:
                $alts = array(
                    'Literature Surveillance',
                    'Summaries',
                    'Travel',
                );
                break;
        }
        $shape = 'shape="rect"';
        $base = "{$this->url}?cal={$this->cal}&focus=";
        for ($i = 0; $i < count($alts); ++$i) {
            $f = $i + 1;
            if ($f != $this->focus) {
                $href = "href='$base$f'";
                if ($f < $this->focus)
                    $x1 = $i * 100 + 10;
                else {
                    $distance_from_last = count($alts) - $i;
                    $x1 = 635 - 100 * $distance_from_last;
                }
                $x2 = $x1 + 85;
                $coords = "coords='$x1,0,$x2,449'";
                $alt = "alt='{$alts[$i]}'";
                $map[] = "<area $href $shape $coords $alt />";
            }
        }
        $map[] = '</map>';
        return $div . $img . implode($map);
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
        switch ($this->role) {
            case 'librarian':
                $items = array(
                    // l('Send a Message', 'stub'),
                    l('Search Database', 'citations/search'),
                    l('Find/Edit a Record', 'stub'),
                    l('Manage Journals', 'stub'),
                    l('Import Citations', 'citations/import'),
                    l('Review Citations', 'citations'),
                    l('Publish Citations', 'citations/publish'),
                );
                break;
            default:
                $items = array(
                    l('Post a Document', 'docs/post'),
                    l('Manage Documents', 'docs'),
                    l('Manage Groups', 'groups'),
                    // l('Send a Message', 'stub'),
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
                $both = array_intersect($board_ids, $this->user->ebms_boards);
                if (count($both) < 1)
                    $wanted = false;
            }
            if ($wanted) {
                $item = l($result->description, file_create_url($result->uri));
                $items[] = $item;
            }
        }
        $title = 'PDQ&reg; Board Rosters';
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
     * XXX Replace stub data when the relevant portions of the
     *     system have been implemented.
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
        $total += $count = $this->get_review_count();
        $page = $this->role == 'member' ? 'assigned' : 'reviews';
        $items[] = EbmsHomePage::alert("review/$page", 'Reviews', $count);

        // Assemble and return the box.
        $title = "Alerts <span class='alert-count'>$total</span>";
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
            $query->isNull('r.article_id');
            $query->distinct();
            $query = $query->countQuery();
            return $query->execute()->fetchField();
        }

        // To find unseen reviews, we check:
        //    1. is the review for a packet this user created?
        //    2. was the review posted since the last time the board
        //       manager looked at the packet?
        $query = db_select('ebms_article_review', 'r');
        $query->fields('r', array('review_id'));
        $query->join('ebms_packet', 'p', 'p.packet_id = r.packet_id');
        $query->condition('p.created_by', $this->user->uid);
        $query->where('(p.last_seen IS NULL OR r.when_posted > p.last_seen)');
        $query->distinct();
        $query = $query->countQuery();
        return $query->execute()->fetchField();
    }

    /**
     * Create the home page box highlighting the next meeting
     * scheduled for the current user.
     * XXX Replace stub information when the calendar portion
     *     of the system has been implemented.
     */
    private function next_meeting() {
        /*
        $now = date_create();
        $then = date_modify($now, '+ 14 days');
        $date = date_format($then, 'F j, Y');
        $time = '10:00 - 11:00 AM ET.';
        $desc = 'Stub Meeting (no events yet)';
        $link = l("<span>$date</span><br />$time<br />$desc", 'calendar',
                array('html' => true));
        $items = array($link);
        return $this->list_block('Next Meeting', 'next-meeting', $items);
         *
         */
        $view = views_get_view('event_calendar');

        $display = "";

        if ($view) {
            $display = $view->preview('block_next');
        }

        return array(
            '#markup' => "<h3>Next Meeting</h3>" . $display,
            '#prefix' => "<div id='activity-enclosure' class='ebms-block'>",
            '#suffix' => $this->block_bottom_img() . "</div>",
            );
    }

    /**
     * Show a small version of the current month's calendar,
     * highlighting days on which events visible to the current
     * user occur, with links to those events.  Allows the user
     * to navigate to different months.
     * XXX The current implementation does not use the third-party
     *     Calendar module, which has not yet been integrated
     *     into the system.  Stub event links are used.
     */
    private function calendar() {
        /*$cal_date = strtotime($this->cal);
        $cal_year = date('Y', $cal_date);
        $cal_month = date('n', $cal_date);
        $title = date('F Y', $cal_date);
        $today = date('Y-m-d');
        $weeks = $this->get_weeks();
        $rows = array(array('S', 'M', 'T', 'W', 'T', 'F', 'S'));
        foreach ($weeks as $week) {
            $row = array();
            foreach ($week as $date) {
                $day = date('j', $date);
                $month = date('n', $date);
                $dow = date('w', $date);
                $is_weekday = $dow > 0 && $dow < 6;
                if ($month != $cal_month)
                    $row[] = array(
                        'data' => $day,
                        'class' => array('not-this-month'),
                    );
                elseif ($today == date('Y-m-d', $date))
                    $row[] = array(
                        'data' => $day,
                        'class' => array('today'),
                    );
                elseif (rand(1, 5) == 1 && $is_weekday)
                    $row[] = array(
                        'data' => l($day, 'calendar'),#"<a href='stub'>$day</a>",
                        'class' => array('day-has-events'),
                    );
                else
                    $row[] = $day;
            }
            $rows[] = array('data' => $row, 'no_striping' => true);
        }
        $id = 'cal-prev-month';
        $date = mktime(0, 0, 0, $cal_month - 1, 1, $cal_year);
        $cal = date('Y', $date) . '-' . date('m', $date);
        $url = "{$this->url}?focus={$this->focus}&cal=$cal";
        $prev = "<div id='$id'><a href='$url'>&lt;</a></div>";
        $id = 'cal-next-month';
        $date = mktime(0, 0, 0, $cal_month + 1, 1, $cal_year);
        $cal = date('Y', $date) . '-' . date('m', $date);
        $url = "{$this->url}?focus={$this->focus}&cal=$cal";
        $next = "<div id='$id'><a href='$url'>&gt;</a></div>";
        $prefix = $prev . $next;
        return $this->table_block($title, 'calendar', $rows, $prefix);*/

        drupal_add_css(Ebms\CSS_DIR . '/calendar.css');

        $view = views_get_view('event_calendar');
        if(!$view)
            return array();
        $display = $view->preview('month_block');

        return array(
            '#markup' => $display,
            '#prefix' => "<div id='calendar-enclosure'>",
            '#suffix' => $this->block_bottom_img() . "</div>",
            );
    }

    /**
     * Helper function to collect a nested array of dates for
     * the selected month's calendar, filling out complete
     * weeks with days at the end of the preceding month and
     * at the beginning of the following month as needed.
     */
    private function get_weeks() {
        $first = strtotime($this->cal . '-01');
        $dow = date('w', $first);
        $month = date('n', $first);
        $year = date('Y', $first);
        $date = mktime(0, 0, 0, $month, 1 - $dow, $year);
        $rows = array();
        while (date('Y-m', $date) <= $this->cal) {
            $row = array();
            while (count($row) < 7) {
                $row[] = $date;
                $m = date('n', $date);
                $d = date('j', $date) + 1;
                $y = date('Y', $date);
                $date = mktime(0, 0, 0, $m, $d, $y);
            }
            $rows[] = $row;
            if (count($rows) > 7) {
                drupal_set_message('bug in get_weeks', 'error');
                break;
            }
        }
        return $rows;
    }

    /**
     * Data controlling which links are displayed in the featured
     * task area of the main panel on the home page, depending
     * the current user's role(s) and on which portion of the
     * panel the user wants to see.
     */
    private static function featured_info($role, $focus) {
        $info = array(
            'librarian' => array(
                '1' => array(
                    'title' => l('Citation Management', 'citations'),
                    'links' => array(
                        l('Import Citation', 'citations/import'),
                        l('Find/Edit a Record', 'stub'),
                        l('Review Citations', 'stub'),
                    ),
                ),
                '2' => array(
                    'title' => l('Reports', 'reports'),
                    'links' => array(
                        l('Citation Management System Reports', 'stub'),
                        l('Not List Reports', 'stub'),
                    ),
                ),
                '3' => array(
                    'title' => l('Administrator Tool', 'stub'),
                    'links' => array(
                        l('Add New Review Cycle', 'stub'),
                        l('Manage Summary Topics', 'stub'),
                    ),
                ),
            ),
            'admin' => array(
                '1' => array(
                    'title' => l('Citation Management', 'citations'),
                    'links' => array(
                        l('Search Database', 'citations/search'),
                        l('Review Citations', 'citations'),
                        l('Add Citation', 'citations/import'),
                    ),
                ),
                '2' => array(
                    'title' => l('Literature Surveillance', 'review/reviews'),
                    'links' => array(
                        l('Create A Literature Surveillance Packet',
                            'review/add'),
                        l('Edit a Literature Surveillance Packet',
                            'review/packets'),
                        l('Review Responses to a Literature Surveillance ' .
                            'Packet', 'review/reviews'),
                    ),
                ),
                '3' => array(
                    'title' => l('Summaries', 'summaries'),
                    'links' => array(
                        l('Post Summary Document', 'summaries'),
                        l('Download and Review Summaries', 'summaries'),
                    ),
                ),
                '4' => array(
                    'title' => l('Reports', 'reports'),
                    'links' => array(
                        l('Literature Surveillance Review Report', 'stub'),
                        l('Travel Reports', 'stub'),
                    ),
                ),
            ),
            'member' => array(
                '1' => array(
                    'title' => l('Literature Surveillance', 'review'),
                    'links' => array(
                        l('Review Assigned Articles', 'review'),
                    )
                ),
                '2' => array(
                    'title' => l('Summaries', 'summaries'),
                    'links' => array(
                        l('Download and Review Summaries', 'summaries'),
                        l('Post Summaries with Changes', 'summaries'),
                    )
                ),
                '3' => array(
                    'title' => l('Travel', 'travel'),
                    'links' => array(
                        l('Hotel Reservations', 'travel'),
                        l('Directions to NCI', 'travel'),
                        l('Reimbursements', 'travel'),
                    )
                )
            )
        );
        return $info[$role][$focus];
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
            '#suffix' => $this->block_bottom_img() . '</div>',
        );
    }

    /**
     * Assemble the Drupal render array for a home page block containing
     * a table.  If it turns out that we need similar blocks on other
     * pages in the system, this functionality will be moved to the
     * common.inc file and packaged in a separate class.
     */
    private function table_block($title, $id, $rows, $extra, $middle=false)
    {
        $class = $middle ? 'ebms-block middle-block' : 'ebms-block';
        $title = '<h3 class="ebms-block-title">' . $title . '</h3>';
        $div = '<div class="' . $class . '" id="' . $id . '">';
        $prefix = $div . $title . $extra;
        return array(
            '#theme' => 'table',
            '#title' => $title,
            '#rows' => $rows,
            '#prefix' => $prefix,
            '#suffix' => $this->block_bottom_img() . '</div>',
        );
    }

    /**
     * Create the markup for the image used for the bottom of one of
     * the EBMS home page boxes.
     */
    private static function block_bottom_img() {
        $url = EbmsHomePage::block_bottom_image();
        return "<img class='ebms-block-bottom' src='$url' alt='' />";
    }

    /**
     * Create the URL for the image used for the bottom of one of the
     * EBMS home page boxes.
     */
    private static function block_bottom_image() {
        return Ebms\IMAGES_DIR . '/box-bottom.png';
    }
}
