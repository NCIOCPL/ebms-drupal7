<?php

/**
 * Common classes, functions, etc., for the EBMS system.  This file is
 * included by ebms.module, it will always be present (you don't need
 * to include it yourself).
 *
 */

namespace Ebms;

define('Ebms\CONVERSION_DATE', variable_get('ebms_conversion_date',
        '2012-11-12'));
define('Ebms\HOST', $_SERVER['HTTP_HOST']);
define('Ebms\QA', strpos(HOST, 'ebms-qa') === 0);
define('Ebms\CA', strpos(HOST, 'ebms-ca') === 0);
define('Ebms\DEV', strpos(HOST, 'ebms-dev') === 0 ||
    strpos(HOST, 'ebms.rksystems') === 0 ||
    strpos(HOST, 'ebmsdev') === 0);
define('Ebms\PROD', !QA && !CA && !DEV);
define('Ebms\APP_DIR', base_path());
define('Ebms\THEME_NAME', 'ebmstheme');
define('Ebms\THEME_DIR', APP_DIR . drupal_get_path('theme', THEME_NAME));
define('Ebms\IMAGES_DIR', THEME_DIR . '/images');
define('Ebms\CSS_DIR', drupal_get_path('theme', THEME_NAME) . '/css');
define('Ebms\MODULE_DIR', drupal_get_path('module', 'ebms'));
define('Ebms\JS_DIR', MODULE_DIR . '/js');
define('Ebms\FROM', 'PDQ Editorial Board Management System <ebms@cancer.gov>');
define('Ebms\LEFT_ARROW', '<');
define('Ebms\RIGHT_ARROW', '>');
define('Ebms\UP_ARROW', "\xe2\x96\xb2");
define('Ebms\DOWN_ARROW', "\xe2\x96\xbc");
define('Ebms\ELLIPSIS', "\xe2\x80\xa6");
define('Ebms\STAR', "\xe2\x98\x86");
define('Ebms\BLACK_STAR', "\xe2\x98\x85");
define('Ebms\PICTURE_SIZE', 135);
define('Ebms\DOUBLE_RIGHT_ARROW', '>>');
define('Ebms\DOUBLE_LEFT_ARROW', '<<');

define('Ebms\ADD_DECISION_ACTIVE',
    IMAGES_DIR . '/EBMS_AddDecision_Icon_Active.png');
define('Ebms\ADD_DECISION_INACTIVE',
    IMAGES_DIR . '/EBMS_AddDecision_Icon_Inactive.png');

define('Ebms\PASS_ACTIVE', IMAGES_DIR . '/EBMS_Pass_Icon_Active.png');
define('Ebms\PASS_INACTIVE', IMAGES_DIR . '/EBMS_Pass_Icon_Inactive.png');

define('Ebms\REJECT_ACTIVE', IMAGES_DIR . '/EBMS_Reject_Icon_Active.png');
define('Ebms\REJECT_INACTIVE', IMAGES_DIR . '/EBMS_Reject_Icon_Inactive.png');
define('Ebms\DELETE_ICON', IMAGES_DIR . '/icon_deletelink.gif');

define('Ebms\TAG_ACTIVE', IMAGES_DIR . '/EBMS_Tag_Icon_Active.png');
define('Ebms\TAG_INACTIVE', IMAGES_DIR . '/EBMS_Tag_Icon_Inactive.png');

define('Ebms\EDIT_ACTIVE', IMAGES_DIR . '/EBMS_Edit_Icon_Active.png');
define('Ebms\EDIT_INACTIVE', IMAGES_DIR . '/EBMS_Edit_Icon_Inactive.png');

define('Ebms\COMMENT_ACTIVE', IMAGES_DIR . '/EBMS_AddComment_Icon_Active.png');
define('Ebms\COMMENT_INACTIVE',
    IMAGES_DIR . '/EBMS_AddComment_Icon_Inactive.png');

define('Ebms\FYI_ACTIVE', IMAGES_DIR . '/EBMS_FlagFYI_Icon_Active.png');
define('Ebms\FYI_INACTIVE', IMAGES_DIR . '/EBMS_FlagFYI_Icon_Inactive.png');
define('Ebms\CB_CHECKED', IMAGES_DIR . '/checkbox-checked.png');
define('Ebms\CB_UNCHECKED', IMAGES_DIR . '/checkbox-unchecked.png');
define ('Ebms\NOTE_ICON', IMAGES_DIR . '/note.ico');

define('Ebms\DOC_TYPES', 'txt doc pdf docx ppt pptx xls xlsx jpg jpeg');

define('Ebms\PUBMED_URL', 'https://www.ncbi.nlm.nih.gov/pubmed');

/**
 * As a result of bugs in the Superfish menu module, poor documentation
 * of the Drupal Menu API, unreliable behavior of the Drupal menu
 * caching mechanism, and other factors (for example, Drupal did not
 * always honor the weights stored in the array returned by ebms_menu())
 * we decided to implement our own menu theming.  We use Drupal's
 * Menu API for routing and as a first line of defense in checking
 * access permissions for the pages, but our own module is doing the
 * rest.
 */
class Menu {

    public
        $label,
        $path,
        $action,
        $children;
    static $menus;
    static $active = '';

    /**
     * Object representing a single item in the EBMS menu.  At a
     * minimum, we store the string displayed for the item and
     * the path to the item (relative to the root of the site;
     * Drupal will create the necessary URL when we pass the
     * label and path to the l() API function).  The object
     * can also include an array of child menu items.  The
     * EBMS menu structures have only a single level of nesting.
     */
    public function __construct($label, $path, $children=null, $tip=null) {
        $this->label = $label;
        $this->path = $path;
        $this->children = $children ? $children : array();
        $this->tip = $tip;
    }

    /**
     * Generate the Drupal 'render array' for this menu item,
     * ready for processing by our custom theming callback.
     * Called for the top-level menu items; children are handled
     * here, without recursion.
     */
    public function render_array() {
        $link = Menu::make_link($this->label, $this->path, $this->tip);
        $ra = array('data' => $link);
        if ($this->label == Menu::$active)
            $ra['class'] = array('active');
        $children = array();
        foreach ($this->children as $child)
            $children[] = Menu::make_link($child->label, $child->path);
        if (!empty($children))
            $ra['children'] = $children;
        return $ra;
    }

    /**
     * I had thought that using the 'fragment' option would get
     * Drupal to do the right thing in constructing the link, but
     * it still comes out with the hash character replaced with '%23';
     * so we have to fix it on the way out by hand.  Ugly!
     */
    static function make_link($label, $path, $tip=null) {
        $options = array();
        if (strpos('#', $path) !== false) {
            list($path, $fragment) = explode('#', $path, 2);
            $options['fragment'] = $fragment;
        }
        if ($tip)
            $options['attributes']['title'] = $tip;

        // Default Reviewed Packets page to view all (JIRA::OCEEBMS-183).
        // Ditto for Manage Documents page (JIRA::OCEEBMS-415).
        if ($path == 'review/reviews' || $path == 'docs')
            $options['query']['pager'] = 'off';

        $link = l($label, $path, $options);
        $link = str_replace('%23', '#', $link);
        return str_replace('@@BR@@', '<br>', $link);
    }

    /**
     * Retrieve the static array of menus.  Build the array first if
     * that hasn't been done already.  Do nothing if the user isn't
     * logged in.
     */
    public static function menus() {
        if (!user_is_logged_in())
            return array();
        if (empty(self::$menus))
            self::init();
        return self::$menus;
    }

    /**
     * Build the menu structures dynamically, based on the current
     * user's EBMS roles, as well as the boards and subgroups of
     * which she is a member.
     */
    public static function init() {
        // XXX Slice the roles finer as development evolves.
        global $user;
        if (in_array('icrdb manager', $user->roles))
            $role = 'icrdb manager';
        else if (in_array('medical librarian', $user->roles))
            $role = 'librarian';
        elseif (in_array('board member', $user->roles))
            $role = 'member';
        elseif (in_array('admin assistant', $user->roles))
            $role = 'admin assistant'; // XXX maybe later 'assistant';
        else
            $role = 'board manager';
        switch ($role) {
            case 'board manager':
            case 'icrdb manager':
            case 'admin assistant':
                self::$menus = array(
                    Menu::admin($role),
                    Menu::citations($role),
                    Menu::reports($role),
                    Menu::literature($role),
                    Menu::summaries($role),
                    Menu::travel($role),
                    Menu::calendar($role),
                );
                break;
            case 'librarian':
                self::$menus = array(
                    Menu::admin($role),
                    Menu::citations($role),
                    Menu::reports($role),
                    Menu::calendar($role),
                );
                break;
            case 'member':
                self::$menus = array(
                    Menu::search(),
                    Menu::literature($role),
                    Menu::summaries($role),
                    Menu::travel($role),
                    Menu::calendar($role),
                );
        }
    }

    /**
     * Build the EBMS administrator tool menu.
     */
    public static function admin($role) {
        global $user;
        $children = array();
        if ($role != 'librarian') {
            $children[] = new Menu('Upload Document', 'docs/post');
            $children[] = new Menu('Manage Documents', 'docs');
        }
        $children[] = new Menu('Manage Internal Tags', 'admin/internal-tags');
        $children[] = new Menu('Manage Summary Topics', 'admin/topics');
        $children[] = new Menu('Manage Summary Topic Groups',
                      'admin/topic-groups');
        if ($role != 'librarian') {
            $children[] = new Menu('Manage Groups', 'groups');
            $children[] = new Menu('Find Users', 'admin/users');
            $children[] = new Menu('Print Packets', 'print-packets/run');
            $children[] = new Menu('View Packet Printing Jobs',
                          'print-packets/view');
            $children[] = new Menu('Manage Print Recipients',
                          'print-packets/mbrs');
        }
        $children[] = new Menu('Journal Maintenance', 'citations/not-list');
        if ($role == 'librarian')
            $children[] = new Menu('Refresh Journal List',
                          'admin/refresh-journals');
        if (in_array('site manager', $user->roles) || $user->uid == 1)
            $children[] = new Menu('Site Administration', 'admin');
        if ($role != 'member')
            $children[] = new Menu('User Help', 'user-help');
        return new Menu('Admin', $children[0]->path, $children);
    }

    /**
     * Build the EBMS menu information for Citation management.
     */
    public static function citations($role) {
        $title = 'Manage@@BR@@Citations';
        switch ($role) {
            case 'librarian':
                $children = array(
                    new Menu('Review Citations', 'citations'),
                    new Menu('Search Database', 'citations/search'),
                    new Menu('Import Citations', 'citations/import'),
                    new Menu('Publish Citations', 'citations/publish'),
                    new Menu('Internal Articles', 'internal-articles'),
                    new Menu('Internal Import', 'import-internal-articles'),
                );
                return new Menu($title, 'citations', $children);
            case 'admin assistant':
                $children = array(
                    new Menu('Full Text Retrieval', 'citations/fulltext'),
                    new Menu('Search Database', 'citations/search'),
                    new Menu('Import Citations', 'citations/import'),
                    new Menu('Internal Articles', 'internal-articles'),
                    new Menu('Internal Import', 'import-internal-articles'),
                );
                return new Menu($title, 'citations/fulltext', $children);
            default:
                $children = array(
                    new Menu('Review Citations', 'citations'),
                    new Menu('Search Database', 'citations/search'),
                    new Menu('Import Citations', 'citations/import'),
                    new Menu('Internal Articles', 'internal-articles'),
                    new Menu('Internal Import', 'import-internal-articles'),
                );
                return new Menu($title, 'citations', $children);
        }
    }

    /**
     * Build the EBMS menu information for reports.
     */
    public static function reports($role) {
        return new Menu(
            'Reports', 'reports', array(
                new Menu('Board Management', 'reports/board'),
                new Menu('Citation Management', 'reports/cite'),
            )
        );
    }

    /**
     * Build the EBMS menu information for literature review.
     */
    public static function literature($role) {
        $title = 'Literature@@BR@@Packets';
        switch ($role) {
            case 'member':
                $submenus = array(
                    new Menu('Assigned Packets', 'review/assigned'),
                    new Menu('Completed Packets', 'review/completed'),
                    new Menu('FYI Packets', 'review/fyi'),
                );
                return new Menu($title, 'review', $submenus);
            case 'admin assistant':
            default:
                $submenus = array(
                    new Menu('Reviewed Packets', 'review/reviews'),
                    new Menu('Create Packet', 'review/add'),
                    new Menu('View/Edit Packets', 'review/packets'),
                    new Menu('Record Responses', 'review/record'),
                );
                return new Menu($title, 'review/reviews', $submenus);
        }
    }

    /**
     * Build the EBMS menu information for the Summaries pages.
     */
    public static function summaries($role) {
        $tip = 'View, post, or download summary documents';
        return new Menu('Manage@@BR@@Summaries', 'summaries', null, $tip);
    }

    /**
     * Build the EBMS menu information for the calendar pages.
     */
    public static function calendar($role) {
        $menu = new Menu('Calendar', 'calendar');
        $menu->children = array(
            new Menu('Month', 'calendar'),
            new Menu('Week', 'calendar/week'),
            new Menu('Day', 'calendar/day'),
        );
        return $menu;
    }

    /**
     * Build the EBMS menu information for the Travel pages.
     */
    public static function travel($role) {
        $menu = new Menu(
            'Travel', 'travel', array(
                new Menu('Directions', 'travel/directions'),
                new Menu('Policies and Procedures',
                    'travel/policies-and-procedures'),
                new Menu('Hotel Request', 'travel/hotel-request'),
                new Menu('Reimbursement Request',
                    'travel/reimbursement-request'),
            )
        );

        return $menu;
    }

    /**
     * Build the EBMS menu information for the board member search
     * page (unlike other users, board members have search as a
     * top-level menu choice).
     */
    public static function search() {
        return new Menu('Search@@BR@@Citations', 'citations/search', array());
    }
}

/**
 * Support for keeping the list of journal titles up to date.
 */
class Journal {
    public $pmid, $title, $brief;
    function __construct($pmid, $title, $brief) {
        $this->pmid = $pmid;
        $this->title = $title;
        $this->brief = $brief;
    }

    /**
     * The only public function.  Fetches the latest list of journals
     * from the National Library of Medicine, updating titles for
     * journals we already have and adding journals which are new.
     * Does not drop rows for journals which have disappeared from
     * NLM's list.  Writes to the ebms_journal table.  Records the
     * date/time the refresh was done in the drupal variable named
     * ebms_journals_refreshed.
     *
     *  @param bool $forceCheck a flag to skip the once-per-day rate limitation
     *                          of journal refreshes.
     *
     *  @return   array with counts, indexed by the name of the count:
     *              'fetched'  => how many journals NLM listed
     *              'checked'  => how many journals we already have
     *              'updated'  => how many existing rows we changed
     *              'inserted' => how many rows we inserted for new journals
     *
     *  @throw    \Exception if we cannot fetch the journal information
     *            from the National Library of Medicine.
     */
    public static function refresh($forceCheck = false) {

        // by default, gate the refreshes to only occur once a day
        if (!$forceCheck) {
            // retrieve the last update time
            $lastRefresh = variable_get('ebms_journals_refreshed');

            // if the last refresh is available and can be parsed...
            if ($lastRefresh) {
                $refreshDate = \DateTime::CreateFromFormat("Y-m-d H:i:s",
                        $lastRefresh);

                if ($refreshDate) {
                    // Check the date by adding a day to it and comparing to
                    // the current time
                    $dayInterval = new \DateInterval('P1D');
                    $refreshDate->add($dayInterval);

                    $currentDate = new \DateTime();

                    $diffInterval = $refreshDate->diff($currentDate);

                    if ($diffInterval->invert) {
                        // An inverted interval is negative, implying the
                        // refresh date (plus a day) is ahead of the current
                        // time, hence a day has not passed.
                        // so, can return from this point.
                        return array(
                            'fetched' => 0,
                            'checked' => 0,
                            'updated' => 0,
                            'inserted' => 0,
                        );
                    }
                }
            }
        }

        // Collect the two list of journals (ours and NLM's).
        $ebmsJournals = Journal::fetch_titles_from_db();
        try {
            $nlmJournals = Journal::fetch_titles_from_nlm();
        } catch (\Exception $e) {
            // should a drupal message be shown when unable to connect to nlm?
            drupal_set_message($e->getMessage(), 'error');

            // return empty results
            return array(
                'fetched' => 0,
                'checked' => 0,
                'updated' => 0,
                'inserted' => 0,
            );
        }

        // Update the rows for journals whose titles have changed.
        $updated = array();
        foreach ($ebmsJournals as $pmid => $ebmsJournal) {
            if (array_key_exists($pmid, $nlmJournals)) {
                $nlmJournal = $nlmJournals[$pmid];
                if ($nlmJournal->title != $ebmsJournal->title ||
                    $nlmJournal->brief != $ebmsJournal->brief) {
                    db_update('ebms_journal')
                        ->fields(
                            array(
                                'jrnl_title' => $nlmJournal->title,
                                'brf_jrnl_title' => $nlmJournal->brief,
                            )
                        )
                        ->condition('source_jrnl_id', $pmid)
                        ->execute();
                    $updated[] = $pmid;
                }
            }
        }

        // Add rows for the new journals.
        $inserted = array();
        foreach ($nlmJournals as $pmid => $nlmJournal) {
            if (!array_key_exists($pmid, $ebmsJournals)) {
                db_insert('ebms_journal')
                ->fields(
                    array(
                        'source' => 'Pubmed',
                        'source_jrnl_id' => $pmid,
                        'jrnl_title' => $nlmJournal->title,
                        'brf_jrnl_title' => $nlmJournal->brief,
                    )
                )
                ->execute();
                $inserted[] = $pmid;
            }
        }

        // Remember when we did the refresh.
        variable_set('ebms_journals_refreshed', strftime("%Y-%m-%d %H:%M:%S"));

        // Tell the caller what we did.
        return array(
            'fetched' => count($nlmJournals),
            'checked' => count($ebmsJournals),
            'updated' => count($updated),
            'inserted' => count($inserted),
        );
    }

    /**
     * Fetch and parse the journal data maintained by the National Library
     * of Medicine.
     */
    static function fetch_titles_from_nlm() {

        // Fetch the compressed file of journal information.
        $url = 'ftp://ftp.ncbi.nih.gov/pubmed/J_Medline.gz';
        $ch = curl_init($url);

        curl_setopt($ch, CURLOPT_FRESH_CONNECT, true);
        curl_setopt($ch, CURLOPT_RETURNTRANSFER, true);
        curl_setopt($ch, CURLOPT_TIMEOUT, 15);
        curl_setopt($ch, CURLOPT_SSL_VERIFYPEER, false);
        curl_setopt($ch, CURLOPT_SSL_VERIFYHOST, false);
        $results = curl_exec($ch);
        curl_close($ch);

        // Throw failure exception if no files retrieved
        if (!$results)
            throw new \Exception('Unable to retrieve journals from NLM');

        // Write it to a file so we can uncompress it.  PHP won't have
        // full support for uncompressing in memory until PHP6; see
        // http://proger.i-forge.net/gzdecode_doesnt_work_in_PHP_pre-6/OQB
        $tmpfname = tempnam(sys_get_temp_dir(), 'nlm-journals');
        file_put_contents($tmpfname, $results);
        ob_start();
        readgzfile($tmpfname);
        $results = ob_get_clean();

        // Break the uncompressed file into separate lines and parse
        // out the journals.  The Pubmed ID is always last in the set
        // of lines for a journal.
        $lines = preg_split("/(\r\n|\n|\r)/", $results);
        $pmid = $title = $brief = null;
        $journals = array();
        foreach ($lines as $line) {
            $matches = array();
            if (substr($line, 0, 5) == '-----')
                $pmid = $title = $brief = null;
            elseif (preg_match('/NlmId: (.*)/', $line, $matches))
                $pmid = $matches[1];
            elseif (preg_match('/JournalTitle: (.*)/', $line, $matches))
                $title = $matches[1];
            elseif (preg_match('/MedAbbr: (.*)/', $line, $matches))
                $brief = $matches[1];
            if ($pmid && $title) {
                $journals[$pmid] = new Journal($pmid, $title, $brief);
                $pmid = $title = $brief = null;
            }
        }

        // Should actually be more than 25,000 journals in the file.
        if (count($journals) < 10000)
            throw new \Exception('Failure retrieving journal titles from NLM');
        return $journals;
    }

    /**
     * Collect the journal information from the ebms_journal table.
     */
    static function fetch_titles_from_db() {
        $journals = array();
        $results = db_select('ebms_journal', 'j')
            ->fields(
                'j',
                array('source_jrnl_id', 'jrnl_title', 'brf_jrnl_title')
            )
            ->condition('j.source', 'Pubmed')
            ->execute();
        foreach ($results as $result) {
            $pmid = $result->source_jrnl_id;
            $title = $result->jrnl_title;
            $brief = $result->brf_jrnl_title;
            $journals[$pmid] = new Journal($pmid, $title, $brief);
        }
        return $journals;
    }
}

/**
 * Extension of the core Drupal EntityFieldQuery class.  Used for
 * filtering meeting events by date and/or type.  By default the
 * query represented by the object will find all nodes with type
 * ebms_event, but there are methods for narrowing down the results.
 *
 * NOTE: Only those meetings established in the EBMS, not the old
 *       CMS have the meeting nodes and board type and date fields
 *       needed for restrict_by_date() and restrict_by_type() to work.
 *       Earlier meeting events will not be found, much less filtered,
 *       by methods in this class.
 */
class EventQuery extends \EntityFieldQuery {

    /**
     * Set some defaults for the query object.
     */
    public function __construct() {
        $this->entityCondition('entity_type', 'node')
            ->propertyCondition('status', 1)
            ->entityCondition('bundle', 'ebms_event');
    }

    /**
     * If the user has asked to filter meetings by date, apply the
     * appropriate field condition.  It is acceptable for the user
     * to have specified more general components of the date (for
     * example, a year and month without a day, or just a year) but
     * not the other way around (can't have a day without both a
     * month and year specified, for example).  The form validation
     * routine will have enforced this assumption.
     *
     *  @param  array   $date   Structured array representing the
     *                          filtering options for this request
     *
     *  @return                 True iff query restricted
     */
    public function restrict_by_date($params) {
        $start = $params['date-start']['date'];
        $end = $params['date-end']['date'];
        if ($start || $end) {
            $range = array(
                strtotime(Util::fill_in_date($start)),
                strtotime(Util::fill_in_date($end, true)),
            );
            $field = 'field_datespan';
            $this->fieldCondition($field, 'value', $range, 'BETWEEN');
            return true;
        }
        return false;
    }

    /**
     * If the user wants to filter meetings by whether they were board-
     * specific or subgroup-specific, apply the appropriate conditions
     * by detecting whether a board (or subgroup) has been specified
     * for the event.  There is an event type field in the event node,
     * but it's used for something else (designating whether a meeting
     * is face-to-face, or teleconferenced).
     *
     * 2013-03-11 (TIR 2446): use the new event_category field,
     * instead of checking for boards or subgroups linked to the
     * event.  Note that this may result in contradictory results
     * (if the user selects an event category value which conflicts
     * with the type of group (board or subgroup) linked to the
     * event.
     *
     *  @param  string  $type    If a type has been specified, it will
     *                           be "board" or "subgroup."
     *
     *  @return                 None
     */
    public function restrict_by_type($type) {
        if ($type)
            $this->fieldCondition('field_event_category', 'value', $type, '=');
    }
}

/**
 * Common utility functions used throughout the EBMS.
 */
class Util {
    private static $u2aMapping = array(
        160 => ' ',
        163 => '[pounds]',
        165 => '[yen]',
        169 => '(C)',
        174 => '(R)',
        176 => '[degree]',
        177 => '+/-',
        178 => '2',
        179 => '3',
        180 => '\'',
        181 => 'm',
        183 => '.',
        185 => '1',
        189 => '1/2',
        191 => '?',
        192 => 'A',
        193 => 'A',
        194 => 'A',
        197 => 'A',
        199 => 'C',
        201 => 'E',
        205 => 'I',
        207 => 'I',
        211 => 'O',
        214 => 'O',
        215 => 'x',
        216 => 'O',
        220 => 'U',
        223 => 'ss',
        224 => 'a',
        225 => 'a',
        226 => 'a',
        227 => 'a',
        228 => 'a',
        229 => 'a',
        230 => 'ae',
        231 => 'c',
        232 => 'e',
        233 => 'e',
        234 => 'e',
        235 => 'e',
        236 => 'i',
        237 => 'i',
        238 => 'i',
        239 => 'i',
        240 => 'eth',
        241 => 'n',
        242 => 'o',
        243 => 'o',
        244 => 'o',
        245 => 'o',
        246 => 'o',
        247 => '/',
        248 => 'o',
        249 => 'u',
        250 => 'u',
        251 => 'u',
        252 => 'u',
        253 => 'y',
        255 => 'y',
        257 => 'a',
        259 => 'a',
        261 => 'a',
        263 => 'c',
        265 => 'c',
        269 => 'c',
        273 => 'd',
        275 => 'e',
        277 => 'e',
        279 => 'e',
        281 => 'e',
        283 => 'e',
        285 => 'g',
        287 => 'g',
        291 => 'g',
        297 => 'i',
        299 => 'i',
        301 => 'i',
        304 => 'I',
        305 => 'i',
        309 => 'j',
        311 => 'k',
        312 => 'K',
        314 => 'l',
        321 => 'L',
        322 => 'l',
        324 => 'n',
        326 => 'n',
        328 => 'n',
        333 => 'o',
        335 => 'o',
        339 => 'oe',
        341 => 'r',
        345 => 'r',
        346 => 'S',
        347 => 's',
        349 => 's',
        350 => 'S',
        351 => 's',
        352 => 'S',
        353 => 's',
        355 => 't',
        361 => 'u',
        363 => 'u',
        365 => 'u',
        367 => 'u',
        378 => 'z',
        380 => 'z',
        381 => 'Z',
        382 => 'z',
        402 => 'f',
        487 => 'g',
        603 => 'e',
        730 => 'o',
        776 => '',
        913 => 'A',
        916 => '[DELTA]',
        922 => 'K',
        924 => 'M',
        931 => '[SIGMA]',
        932 => 'T',
        934 => '[PHI]',
        935 => 'X',
        936 => '[PSI]',
        937 => '[OMEGA]',
        945 => '[alpha]',
        946 => '[beta]',
        947 => '[gamma]',
        948 => '[delta]',
        949 => '[epsilon]',
        950 => '[zeta]',
        951 => '[eta]',
        952 => '[theta]',
        953 => 'i',
        954 => 'k',
        955 => '[lamda]',
        956 => '[mu]',
        959 => 'o',
        960 => '[pi]',
        961 => '[rho]',
        963 => '[sigma]',
        964 => '[tau]',
        966 => '[phi]',
        967 => 'X',
        968 => '[psi]',
        969 => '[omega]',
        981 => '[phi]',
        1008 => '[kappa]',
        1013 => '[epsilon]',
        1050 => 'K',
        8048 => 'a',
        8194 => ' ',
        8195 => ' ',
        8197 => ' ',
        8200 => ' ',
        8201 => ' ',
        8202 => ' ',
        8208 => '-',
        8209 => '-',
        8210 => '-',
        8211 => '-',
        8212 => '--',
        8216 => '\'',
        8217 => '\'',
        8220 => '"',
        8221 => '"',
        8224 => '[dagger]',
        8226 => '.',
        8230 => '...',
        8232 => ' ',
        8243 => '\'\'',
        8244 => '\'\'\'',
        8260 => '/',
        8304 => '0',
        8308 => '4',
        8309 => '5',
        8310 => '6',
        8311 => '7',
        8312 => '8',
        8313 => '9',
        8314 => '+',
        8315 => '-',
        8320 => '0',
        8321 => '1',
        8322 => '2',
        8323 => '3',
        8324 => '4',
        8325 => '5',
        8326 => '6',
        8327 => '7',
        8328 => '8',
        8329 => '9',
        8331 => '-',
        8364 => '[euro]',
        8467 => 'l',
        8482 => 'TM',
        8544 => 'I',
        8594 => '->',
        8710 => '[increment]',
        8722 => '-',
        8723 => '-/+',
        8727 => '*',
        8730 => '[square root]',
        8734 => '[infinity]',
        8745 => '[intersection] ',
        8746 => '[union]',
        8758 => ':',
        8764 => '~',
        8776 => '=',
        8804 => '<=',
        8805 => '>=',
        8806 => '<=',
        8807 => '>=',
        8853 => '+',
        9552 => '=',
        9651 => '[triangle]',
        10877 => '<=',
        10878 => '>=',
        12288 => ' ',
        12296 => '>',
        12297 => '<',
        20185 => '?',
        24310 => '?',
        26041 => '?',
        27668 => '?',
        30410 => '?',
        31890 => '?',
        32958 => '?',
        34917 => '?',
        39063 => '?',
        40548 => '?',
        40836 => '?',
        58102 => '?',
        58106 => '?',
        61472 => '?',
        61538 => '?',
        64257 => 'fi',
        64258 => 'fl',
        65279 => '',
        65308 => '<',
    );

    /**
     * Create a version of a string which replaces non-ascii characters
     * with ascii equivalents more likely to be entered by users
     * searching for titles or authors of articles.
     */
    public static function utf8ToAscii($utf8) {
        $ucs2 = iconv("utf-8", "UCS-2LE", $utf8);
        $ascii = array();
        $i = 0;
        $len = strlen($ucs2);
        while ($i < $len) {
            $lo = ord($ucs2[$i++]);
            $hi = ord($ucs2[$i++]);
            $code = $lo + $hi * 256;
            if (array_key_exists($code, self::$u2aMapping))
                $ascii[] = self::$u2aMapping[$code];
            elseif ($code < 128)
                $ascii[] = chr($code);
            else
                $ascii[] = '?';
        }
        return implode('', $ascii);
    }

    /**
     * Get all of the text content of a node without the markup.
     *
     * @param object $node   DOMElement or SimpleXMLElement
     *
     * @return               Simple text string, without markup
     */
    public static function get_text($node) {
        if (!is_a($node, 'DOMElement')) {
            $node = dom_import_simplexml($node);
        }
        return $node->textContent;
    }

    /**
     * Create a version of a string specialized for indexing in one of
     * several ways:
     *
     *  @param string $text     String to convert.
     *  @param bool $ascii      True = convert string to ascii.
     *  @param bool $trim       True = trim whitespace from ends.
     *  @param int  $maxLen     Max chars to pass through, truncate if more.
     *                           Null = no max.
     *
     *  @return                 (Possibly) transformed string
     */
    public static function indexify($text, $ascii, $trim, $maxLen) {

        if ($ascii && $text)
            $text = Util::utf8ToAscii($text);
        if ($trim && $text)
            $text = trim($text);
        if ($maxLen && $text)
            $text = substr($text, 0, $maxLen);

        return $text;
    }

    /**
     * Scrub utf8 string so that it will be accepted by MySQL's
     * broken "utf8" charset (which chokes on 4-byte encodings).
     * All code points between U+10000 and U+10FFFF are converted
     * to &#xxxxxx; where xxxxxx represents the code point in
     * decimal (between 65536 and 1114111). The first of the
     * four bytes in the utf-8 encoding will be 11110xxx; the
     * other three are 10xxxxxx, so the lower three bytes carry
     * six bits each of the code point's value. OCEEBMS-434
     *
     * 262144 is 1 << 18
     *   4096 is 1 << 12
     *     64 is 1 << 6
     *      1 is 1 << 0
     */
    public static function fix_utf8($utf8) {

        // Handle the common (easy) case quickly.
        if (!preg_match("/[\360-\367]/", $utf8))
            return $utf8;

        // This won't get hit often (we've had only one case so far).
        return preg_replace_callback(
            "/([\360-\367])([\200-\277])([\200-\277])([\200-\277])/",
            function ($matches) {
                return '&#' . (
                    (ord($matches[1]) - 240) * 262144 +
                    (ord($matches[2]) - 128) * 4096 +
                    (ord($matches[3]) - 128) * 64 +
                    (ord($matches[4]) - 128)) . ';';
            },
            $utf8);
    }

    /**
     * Pulls a parameter from a request without letting in parameters
     * from cookies (which we'd get from $_REQUEST).
     */
    public static function get_request_value($name, $default='') {
        if (array_key_exists($name, $_POST))
            return $_POST[$name];
        if (array_key_exists($name, $_GET))
            return $_GET[$name];
        return $default;
    }

    /**
     * Prepare an address for a system email message.
     */
    public static function format_email_address($email, $name) {
        $name = addslashes(mime_header_encode($name));
        return "\"$name\" <$email>";
    }

    /**
     * Get user ID of current user, if not passed.
     *
     *  @param int $chkUserId   If non-null, return this, else current user.
     *                          Saves caller making his own check.
     *
     *  @return                 User ID.
     */
    public static function getUserId($chkUserId) {
        if (is_null($chkUserId)) {
            global $user;
            return $user->uid;
        }
        return $chkUserId;
    }

    /**
     * Fetch ID representing an article state from the database.
     */
    public static function get_state_id($state_name) {
        return db_select('ebms_article_state_type', 't')
            ->fields('t', array('state_id'))
            ->condition('t.state_name', $state_name)
            ->execute()
            ->fetchField();
    }

    /*
     * Load the Javascript libraries we need.
     */
    public static function add_js() {
        drupal_add_library('system', 'ui.dialog');
        drupal_add_js(JS_DIR . '/ebms.js');
    }

    /**
     * Get the real path to the directory where Drupal stores public files
     * using technique that is portable across servers.
     *
     * @return string   Absolute path to the directory.  No trailing '/'.
     */
    public static function get_public_files_path() {

        $site_base = DRUPAL_ROOT . '/' . conf_path();
        $files     = variable_get('file_public_path', conf_path() . '/files');
        $file_path = DRUPAL_ROOT . '/' . $files;

        return $file_path;
    }

    /**
     * Load the URI for a managed Drupal file.
     */
    public static function get_file_uri($file_id) {
        return db_select('file_managed', 'f')
            ->fields('f', array('uri'))
            ->condition('f.fid', $file_id)
            ->execute()
            ->fetchField();
    }

    /**
     * Get the URI for an article's full text document (if we have it).
     */
    public static function get_ft_uri($article_id) {
        $query = db_select('file_managed', 'f');
        $query->join('ebms_article', 'a', 'a.full_text_id = f.fid');
        $query->condition('a.article_id', $article_id);
        $query->fields('f', array('uri'));
        $results = $query->execute();
        foreach ($results as $result)
            return $result->uri;
        return null;
    }

    /**
     * Build a dictionary of the PDQ boards.  The keys are the board
     * IDs and the values are the board names.
     */
    public static function boards() {
        $results = db_select('ebms_board', 'b')
            ->fields('b', array('board_id', 'board_name'))
            ->orderBy('b.board_name')
            ->execute();
        $boards = array();
        foreach ($results as $result)
            $boards[$result->board_id] = $result->board_name;
        return $boards;
    }

    /**
     * Find the "default board" for a user.
     *
     * Selects the default board according to the following priority:
     *   field_data_dft_editorial_board.dft_editorial_board_value.
     *      A custom field in the 'user' bundle.
     *      Settable in ebms.../admin/people, choose person, "edit".
     *   First board found in $user->boards, established at login.
     *   Null.
     *
     * @param mixed $dft
     *   Return this as the default if no default found.
     * @param mixed $uid
     *   Integer or string of digits identifying the user.
     *   Null = use the current global $user.
     *
     * @return int
     *   Unique board ID, or null if no default established.
     */
    public static function get_user_default_board($dft=null, $uid=null) {

        global $user;

        // Default return value
        $dftBoard = $dft;

        // Do we have a stored custom default
        $dftBoard = self::get_custom_user_field_value('dft_editorial_board',
                                                       $uid);

        // Have we already loaded associated boards?
        if (!$dftBoard && !$uid) {
            if (isset($user->boards))
                $dftBoard = reset($user->boards);
        }

        return $dftBoard;
    }

    /**
     * Retrieves a list of active users, optionally limited by role and board.
     *
     * @param array $role_names An optional array of role names.
     * @param array $board_ids An optional array of board ids.
     * @return array An array of valid users based on the arguments, indexed
     *               by uid with a value of the user's name.
     */
    public static function get_active_users($role_names = array(),
        $board_ids = array()) {
        // screen the entity query against lists of role and board user ids
        $board_users = false;
        if (!empty($board_ids)) {
            $board_query = db_select('ebms_board_member', 'm')
                ->condition('m.board_id', $board_ids, 'IN');
            $board_query->addExpression('DISTINCT m.user_id', 'uid');
            $board_users = $board_query->execute()
                ->fetchCol();
        }

        $role_users = false;
        if (!empty($role_names)) {
            // look up role ids
            $role_ids = array();
            foreach ($role_names as $role_name) {
                $role = user_role_load_by_name($role_name);
                if ($role !== false) {
                    $role_ids[] = $role->rid;
                }
            }

            $role_query = db_select('users_roles', 'r')
                ->condition('r.rid', $role_ids, 'IN');
            $role_query->addExpression('DISTINCT r.uid', 'uid');
            $role_users = $role_query->execute()
                ->fetchCol();
        }

        // use the UIDs to limit the query if set
        $user_query = new \EntityFieldQuery();
        $user_query->entityCondition('entity_type', 'user')
            ->propertyCondition('status', '1')
            ->propertyOrderBy('name');

        if($role_users !== false)
            $user_query->propertyCondition('uid', $role_users);

        if($board_users !== false)
            $user_query->propertyCondition('uid', $board_users);

        $uids_result = $user_query->execute();

        $user_options = array();
        if (isset($uids_result['user'])) {
            $users = user_load_multiple(array_keys($uids_result['user']));

            foreach ($users as $key => $user) {
                $user_options[$key] = $user->name;
            }
        }

        return $user_options;
    }

    /**
     * Creates a recent activity message indicating the given board has new
     * articles
     *
     * @param int $board_id the new articles' board id
     */
    public static function activityArticlesPublished($board_id) {
            $message = message_create('activity_new_articles');
            $message->save();

            $wrapper = entity_metadata_wrapper('message', $message);
            $wrapper->field_boards->set(array($board_id));
            $wrapper->save();
    }

    public static function activityEventPublished($nodeWrapper) {

        if ($nodeWrapper->type->value() != 'ebms_event' ||
            !$nodeWrapper->status->value())
            return;

        $message = message_create('activity_new_event');
        $message->save();

        $wrapper = entity_metadata_wrapper('message', $message);

        \Ebms\Util::wrapEventMetadata($nodeWrapper, $wrapper);

        $wrapper->save();
    }

    public static function activityEventCancelled($nodeWrapper) {

        if ($nodeWrapper->type->value() != 'ebms_event' ||
            !$nodeWrapper->status->value())
            return;

        $message = message_create('activity_cancelled_event');
        $message->save();

        $wrapper = entity_metadata_wrapper('message', $message);

        \Ebms\Util::wrapEventMetadata($nodeWrapper, $wrapper);

        $wrapper->save();
    }


    public static function activityEventChanged($nodeWrapper) {

        if ($nodeWrapper->type->value() != 'ebms_event' ||
            !$nodeWrapper->status->value())
            return;

        $message = message_create('activity_changed_event');
        $message->save();

        $wrapper = entity_metadata_wrapper('message', $message);

        \Ebms\Util::wrapEventMetadata($nodeWrapper, $wrapper);

        $wrapper->save();
    }

    public static function activityAgendaPublished($nodeWrapper) {

        if ($nodeWrapper->type->value() != 'ebms_event' ||
            !$nodeWrapper->status->value())
            return;

        $message = message_create('activity_agenda_published');
        $message->save();

        $wrapper = entity_metadata_wrapper('message', $message);

        \Ebms\Util::wrapEventMetadata($nodeWrapper, $wrapper);

        $wrapper->save();
    }

    public static function activityEventType($nodeWrapper) {

        if ($nodeWrapper->type->value() != 'ebms_event' ||
            !$nodeWrapper->status->value())
            return;

        $message = message_create('activity_event_type');
        $message->save();

        $wrapper = entity_metadata_wrapper('message', $message);

        \Ebms\Util::wrapEventMetadata($nodeWrapper, $wrapper);

        // additionally add the event type
        $node_event_type = $nodeWrapper->field_event_type->value();
        if ($node_event_type) {
            $wrapper->field_event_type->set($node_event_type);
        }

        $wrapper->save();
    }

    public static function activityPacketCreated($name, $reviewers, $packet_id) {
            $message = message_create('activity_new_packet');
            $message->save();

            $wrapper = entity_metadata_wrapper('message', $message);
            $wrapper->field_title->set($name);
            $wrapper->field_individuals->set($reviewers);
            $wrapper->field_packet_id->set($packet_id);
            $wrapper->save();
    }

    public static function activityNewSummary(array $args = array()) {

        global $user;

        // Leave if there's nothing to do.
        if (empty($args) || !is_array($args)) return;

        // provide placeholder arguments
        $args += array(
            'title' => null,
            'account' => $user,
            'note' => null,
            'board' => null,
            'url' => null,
            'topics' => array(),
        );

        $title = $args['title'];
        $account = $args['account'];
        $board = $args['board'];
        $topics = $args['topics'];
        $note = $args['note'];
        $url = substr(parse_url($args['url'], PHP_URL_PATH), 1); // OCEEBMS-296

        // check input arguments
        // reject if no title, or no board and no valid topics
        if (empty($title) || (empty($board) && empty($topics))) return;

        // search board and/or topics to find intended recipients of message
        $boards = array();
        if ($board)
            $boards[$board] = $board;

        // dig through any topics for board as well
        foreach ($topics as $topic){
            $topicBoard = \Ebms\getTopicBoard($topic);
            $boards[$topicBoard] = $topicBoard;
        }

        // OCEEBMS-368: add board members for the boards, but only
        // if the summary has been posted by a board member, not a
        // board manager.
        $roles = array('board manager', 'icrdb manager', 'admin assistant');
        if (in_array('board member', $account->roles) && !empty($boards))
            $roles[] = 'board member';
        $individuals = array_keys(Util::get_active_users($roles, $boards));

        // abort if no users available to see notification
        if (empty($individuals)) return;

        // Create the message.
        $message = message_create('activity_new_summary', array(), $account);
        $message->save();

        $wrapper = entity_metadata_wrapper('message', $message);
        $wrapper->field_title->set($title);
        $wrapper->field_individuals->set($individuals);
        $wrapper->field_summary_url->set($url); // OCEEBMS-296
        $wrapper->field_notes->set($note); // OCEEBMS-369
        $wrapper->save();
    }

    public static function wrapEventMetadata($nodeWrapper, $msgWrapper) {

        // retrieve title
        $msgWrapper->field_title->set($nodeWrapper->title->value());

        // retrieve boards
        $node_boards = $nodeWrapper->field_boards->value();
        if ($node_boards) {
            $msgWrapper->field_boards->set($node_boards);
        }

        // retrieve subgroups
        $node_subgroups = $nodeWrapper->field_subgroups->value();
        if ($node_subgroups) {
            $msgWrapper->field_subgroups->set($node_subgroups);
        }

        // retrieve ad-hoc groups
        $node_adhoc_groups = $nodeWrapper->field_ad_hoc_groups->value();
        if ($node_adhoc_groups) {
            $msgWrapper->field_ad_hoc_groups->set($node_adhoc_groups);
        }

        // collect inhouse staff, board members, and individuals data
        $node_individuals = $nodeWrapper->field_individuals->value();
        if ($node_individuals)
            $msgWrapper->field_individuals->set($node_individuals);

        // Plug in the event ID (OCEEBMS-296).
        $msgWrapper->field_event_id->set($nodeWrapper->get("nid")->value());
    }

    /**
     * Build a dictionary of EBMS review cycles.  The keys are the cycle
     * IDs and the values are the cycle names.  The cycles are added in
     * reverse chronological order, to facilitate convenient selection
     * of recent cycles in the user interface.
     *
     *  @param boolean $no_legacy  Flag indicating whether to include
     *                             only cycles from after conversion
     *                             of the legacy CiteMS system's data;
     *                             default is to include all cycles.
     *
     *  @return                    Array of cycle names, indexed by
     *                             cycle IDs
     */
    public static function cycles($no_legacy=false) {
        $query = db_select('ebms_cycle', 'c')
            ->fields('c', array('cycle_id', 'cycle_name'))
            ->orderBy('c.start_date', 'DESC');
        if ($no_legacy) {
            $cutoff = substr(CONVERSION_DATE, 0, 7) . '-01';
            $query->condition('c.start_date', $cutoff, '>=');
        }
        $results = $query->execute();
        $cycles = array();
        foreach ($results as $result)
            $cycles[$result->cycle_id] = $result->cycle_name;
        return $cycles;
    }

    /**
     * Fetch the name of one of the PDQ boards, given its ID.
     */
    public static function get_board_name($board_id) {
        return db_select('ebms_board', 'b')
            ->fields('b', array('board_name'))
            ->condition('b.board_id', $board_id)
            ->execute()
            ->fetchField();
    }

    /**
     * Creates the render array for the left nav panel for a the
     * Citations Management page.
     *
     *  @param string $active  Current action, highlighted by CSS
     *
     *  @return                Render array for left nav panel
     */
    public static function build_left_nav($active=null) {

        global $user;

        $links = array(
            array('Search Database', 'citations/search'),
            array('Import Citations', 'citations/import'),
            array('Journal Maintenance', 'citations/not-list'),
            array('Internal Articles', 'internal-articles'),
            array('Internal Import', 'import-internal-articles'),
        );
        if (in_array('medical librarian', $user->roles))
            $links[] = array('Publish Citations', 'citations/publish');
        if (in_array('admin assistant', $user->roles))
            array_unshift($links, array('Full Text Retrieval',
                    'citations/fulltext'));
        else
            array_unshift($links, array('Review Citations', 'citations'));
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
     * Fetches display name for review cycle, given the cycle's ID.
     *
     *  @param int     $cycle_id  Primary key of row in cycle table
     *
     *  @return                   String for review cycle's name
     */
    public static function get_cycle_name($cycle_id) {
        return db_select('ebms_cycle', 'c')
            ->fields('c', array('cycle_name'))
            ->condition('c.cycle_id', $cycle_id)
            ->execute()
            ->fetchField();
    }

    /**
     * Fetches primary key for row in cycle table, give the start date
     * for the desired cycle.
     *
     *  @param   string  $start_date  String in the form YYYY-MM-DD
     *
     *  @return  int                  Primary key of row in cycle table
     */
    public static function get_cycle_id_from_start_date($start_date) {
        return db_select('ebms_cycle', 'c')
            ->fields('c', array('cycle_id'))
            ->condition('c.start_date', $start_date)
            ->execute()
            ->fetchField();
    }

    /**
     * Returns the highest start_date value in the ebms_cycle table.
     *
     *  @return  string              Latest cycle start date
     */
    public static function get_last_cycle_date() {
        $query = db_select('ebms_cycle', 'c');
        $query->addExpression('MAX(start_date)');
        return $query->execute()->fetchField();
    }

    /**
     * Add a row in the cycle table for the cycle immediately following
     * the one which starts on the caller's date parameter.
     *
     *  @param   string        Starting date of the last cycle
     *
     *  @returns               Nothing
     */
    public static function add_next_cycle($prev_start_date) {
        list($year, $month) = explode('-', $prev_start_date);
        if ($month++ == 12) {
            $year++;
            $month = 1;
        }
        $start_date = sprintf('%04d-%02d-01', $year, $month);
        $time = mktime(0, 0, 0, $month, 1, $year);
        $cycle_name = date('F Y', $time);
        db_insert('ebms_cycle')
            ->fields(
                array(
                    'cycle_name' => $cycle_name,
                    'start_date' => $start_date,
                )
            )
            ->execute();
    }

    /**
     * Determines what the default cycle should be for a picklist.
     * As a side effect, the cycle table is brought up to date so
     * that the default cycle is always available.
     *
     *  @return   int           Foreign key into the ebms_cycle table
     */
    public static function get_default_cycle() {
        $now = getdate();
        $y = $now['year'];
        $m = $now['mon'];
        $d = $now['mday'];
        if ($d > 10) {
            if ($m++ == 12) {
                $y++;
                $m = 1;
            }
        }
        $start_date = sprintf('%04d-%02d-01', $y, $m);
        $cycle_id = Util::get_cycle_id_from_start_date($start_date);
        $safety = 0;
        while (!$cycle_id) {
            $last_cycle = Util::get_last_cycle_date();
            if ($last_cycle > $start_date)
                throw new \Exception('Fatal error: gaps in cycle table');
            Util::add_next_cycle($last_cycle);
            $cycle_id = Util::get_cycle_id_from_start_date($start_date);
        }
        return $cycle_id;
    }

    /**
     * Fetches the primary key for a state type, given the
     * text ID for that type's row.
     *
     *  @param string    $text_id   The text ID for a row in the
     *                              ebms_article_state_type table;
     *                              these IDs are stable, and will
     *                              not change between conversions,
     *                              or if the user-visible state_name
     *                              is altered.  Use state_text_id
     *                              values in code to work with specific
     *                              states.
     *
     *  @return                     Primary key from the
     *                              ebms_article_state_type table.
     */
    public static function state_id_from_text_id($text_id) {
        return db_select('ebms_article_state_type', 't')
            ->fields('t', array('state_id'))
            ->condition('t.state_text_id', $text_id)
            ->execute()
            ->fetchField();
    }

    /**
     * Adds a condition to a DB query, restricting the results to
     * the date range represented by a specified review cycle.
     *
     *  @param SelectQuery $query     Reference to query object
     *  @param int         $cycle_id  Primary key of row in cycle table
     *  @param string      $field     Qualified string for date column
     *                                to be tested against date range
     *
     *  @return                       Boolean indicating whether query
     *                                was successfully modified.
     */
    public static function add_cycle_to_query(&$query, $cycle_id, $field) {
        $cycle_query = db_select('ebms_cycle', 'c')
            ->fields('c', array('start_date'))
            ->condition('c.cycle_id', $cycle_id);
        $cycle_query->addExpression(
            'DATE_ADD(c.start_date, INTERVAL 1 MONTH)',
            'end_date'
        );
        $results = $cycle_query->execute();
        foreach ($results as $result) {
            $date_range = array($result->start_date, $result->end_date);
            $query->condition($field, $date_range, 'BETWEEN');
            return true;
        }
        return false;
    }

    /**
     * Convert a possibly incomplete date to a valid ISO date or
     * date time string.
     *
     *  @param object   $date    Date string or component array
     *  @param boolean  $latest  Optional flag indicating that
     *                           the date represents the end of a
     *                           range, so any gaps should be
     *                           filled in using the latest
     *                           possible value
     *
     *  @return                  ISO date time string if $latest
     *                           is true; otherwise ISO date string
     */
    public static function fill_in_date($date, $latest=false) {
        if (is_string($date))
            $date = self::date_string_to_array($date);
        $y = $date['year'];
        $m = $date['month'];
        $d = $date['day'];
        if ($d) {
            if ($latest)
                return sprintf("%04d-%02d-%02d 23:59:59", $y, $m, $d);
            return sprintf("%04d-%02d-%02d", $y, $m, $d);
        }
        elseif ($m) {
            if ($latest)
                return date("Y-m-d H:i:s", mktime(0, 0, -1, $m + 1, 1, $y));
            return sprintf("%04d-%02d-01", $y, $m);
        }
        elseif ($y) {
            if ($latest)
                return sprintf("%04d-12-31 23:59:59", $y);
            return sprintf("%04d-01-01", $y);
        }
        return $latest ? '9999-12-31' : '1900-01-01';
    }

    /**
     * Adds a condition to a DB query, restricting the results to
     * the date range specified by the caller.
     *
     *  @param SelectQuery $query   Reference to query object
     *  @param string      $start   Beginning of date range
     *  @param string      $end     End of date range
     *  @param string      $field   Qualified string for date column
     *                              to be tested against date range
     *
     *  @return                     Boolean indicating whether query
     *                              was successfully modified.
     */
    public static function add_date_range_to_query(&$query, $start, $end,
        $field)
    {
        try {
            $range = array(
                self::fill_in_date($start),
                self::fill_in_date($end, true),
            );
            $query->condition($field, $range, 'BETWEEN');
            return true;
        }
        catch (Exception $e) {
            pdq_ebms_debug('ADD DATE RANGE TO QUERY', array('error'=>$e));
            return false;
        }
    }

    /**
     * Builds an array of options to be passed to the JQuery
     * datepicker package. By default these options just
     * include the instructions to disable picklists in
     * the popup title bar for jumping to specific years
     * or months. Can also include restriction of the
     * available dates which can be selected from the
     * widget.
     *
     *  @param  string  $range   Optional colon-delimited pair
     *                           of number of years back or
     *                           forward the range of allowable
     *                           dates should include; either
     *                           side of the colon can be blank;
     *                           e.g.: -3:+2
     *
     *  @return array            Option array
     */
    public static function datepicker_options($range='') {
        $options = array(
            'changeMonth' => false,
            'changeYear' => false,
        );
        if ($range) {
            list($min, $max) = @explode(':', $range);
            if (isset($min) && $min !== '')
                $options['minDate'] = $min . 'y';
            if (isset($max) && $max !== '')
                $options['maxDate'] = $max . 'y';
        }
        return $options;
    }

    /**
     * Create a Drupal date popup field array.
     *
     *  @param  string  $default  Optional default value for the field
     *  @param  string  $range    Optional limit for available values
     *                            (see notes on datepicker_options()
     *                            above)
     *  @param  string  $title    Optional string assigned to the
     *                            title attribute of the field to
     *                            pop up with an explanation when
     *                            the user hovers over the field
     *
     *  @return  array            Drupal field array
     */
    public static function date_field($default='', $range='', $title='') {
        $field = array(
            '#type' => 'date_popup',
            '#date_format' => 'Y-m-d',
            '#datepicker_options' => self::datepicker_options($range),
            '#default_value' => $default,
            '#date_label_position' => 'within',
            '#element_validate' => array('pdq_ebms_date_validate'),
        );
        if ($title)
            $field['#attributes'] = array('title' => $title);
        return $field;
    }

    /**
     * Assemble a Drupal form array for a pair of date_popup
     * fields.
     *
     *  @param  string  $label  Required label for the field pair
     *  @param  string  $name   Base name for the fields; will
     *                          have '-start' and '-end' appended
     *                          to the two fields respectively
     *  @param  string  $start  Optional default value for the
     *                          start of the date range
     *  @param  string  $end    Optional default value for the
     *                          end of the date range
     *  @param  boolean $label_right
     *                          Flag indicating that the label
     *                          should be inline to the right
     *                          of the field pair
     *
     *  @return array           Drupal form array for the
     *                          date pair block
     */
    public static function date_range_field($label, $name, $start='', $end='',
                                            $range='', $label_right=true) {
        $start_title = 'Start date (defaults to no starting date)';
        $end_title = 'End date (defaults to no ending date)';
        $start_field = self::date_field($start, $range, $start_title);
        $end_field = self::date_field($end, $range, $end_title);
        $separator = '<span class="date-range-sep">&mdash;</span>';
        $label = array('#markup' => "<div class='date-label'>$label</div>");
        $field = array(
            '#type' => 'container',
            '#attributes' => array('class' => array('date-field-wrapper')),
        );
        if ($label_right)
            $field['#attributes']['class'][] = 'label-on-right';
        else
            $field['label'] = $label;
        $field['date-pair'] = array(
            '#type' => 'container',
            '#attributes' => array('class' => array('date-field-widgets')),
            "$name-start" => $start_field,
            'separator' => array('#markup' => $separator),
            "$name-end" => $end_field,
        );
        if ($label_right)
            $field['label'] = $label;
        return $field;
    }

    /**
     * Adds a condition to a DB query, restricting the results based
     * on a (possibly incomplete) date field.  This function assumes
     * that if a day is specified, a month will also be specified.
     * Similarly, if a month is specified, a year must also be
     * specified.  Verification of these assumptions must be handled
     * by the caller in the form validation code.
     *
     *  @param SelectQuery $query   Reference to query object
     *  @param array       $date    Drupal date field
     *  @param string      $field   Qualified string for date column
     *                              to be tested against date range
     *
     *  @return                     Boolean indicating whether query
     *                              was successfully modified.
     */
    public static function add_date_to_query(&$query, $date, $field) {
        try {
            if (is_string($date))
                $date = self::date_string_to_array($date);
            $y = $date['year'];
            $m = $date['month'];
            $d = $date['day'];
            if ($d)
                $range = array(
                    @sprintf("%04d-%02d-%02d", $y, $m, $d),
                    @sprintf("%04d-%02d-%02d 23:59:59", $y, $m, $d),
                );
            elseif ($m)
                $range = array(
                    @sprintf("%04d-%02d-01", $y, $m),
                    @date("Y-m-d H:i:s", mktime(0, 0, -1, $m + 1, 1, $y)),
                );
            elseif ($y)
                $range = array(
                    @sprintf("%04d-01-01", $y),
                    @sprintf("%04d-12-31 23:59:59", $y),
                );
            else
                return false;
            $query->condition($field, $range, 'BETWEEN');
            return true;
        }
        catch (Exception $e) {
            return false;
        }
    }

    /**
     * Adds a report of imported articles to be added to a Drupal form
     * array, using the ImportBatch object returned by the EBMS
     * import API.
     *
     *  @param ImportBatch  $batch  Reference to object containing
     *                              statistics for an import job
     *  @param string       $title  Header for the report
     *  @param int          $cols   Number of ID pair columns
     *                              to create in each block
     *
     *  @return                     Render array for report
     */
    public static function import_report(&$batch, $title, $cols=5,
                                         $internal=false) {
        $ebms_ids = array();
        $blocks = array(
            'imported' => 'ARTICLE{S} IMPORTED',
            'notListed' => 'ARTICLE{S} NOT LISTED',
            'duplicate' => 'DUPLICATE ARTICLE{S}',
            'reviewReady' => 'ARTICLE{S} READY FOR REVIEW',
            'topicAdded' => 'ARTICLE{S} WITH TOPIC ADDED',
            'replaced' => 'ARTICLE{S} REPLACED',
            'error' => 'ARTICLE{S} WITH ERRORS',
        );
        foreach ($blocks as $key => $val) {
            foreach ($batch->action[$key] as $article) {
                if (array_key_exists($article[0], $ebms_ids)) {
                    if ($ebms_ids[$article[0]])
                        continue;
                }
                $ebms_ids[$article[0]] = $article[1];
            }
        }
        $report = array(
            '#type' => 'container',
            '#attributes' => array('id' => 'import-report'),
        );

        // If the job failed, add that to the report
        if ($batch->status != 'Success') {
            $msg = "<div class='messages'>" .
                   "<h4 class='error'>Job status = {$batch->status}:" .
                   "<br />{$batch->messages}</h4>" .
                   "</div>";
        }
        else
            $msg = '';

        $report['report-title'] = array(
            '#type' => 'markup',
            '#markup' => '<h2>' . $title . '</h2>' . $msg,
        );
        $unique_ids = array_keys($batch->uniqIds);
        $ids = array();
        foreach ($unique_ids as $pmid) {
            if (array_key_exists($pmid, $ebms_ids))
                $ids[$pmid] = $ebms_ids[$pmid];
            else
                $ids[$pmid] = null;
        }
        $report['report-totals'] =
            Util::import_report_block('UNIQUE ID{S} IN BATCH', $ids, $cols);
        $blocks = array(
            'imported' => 'ARTICLE{S} IMPORTED',
            'notListed' => 'ARTICLE{S} NOT LISTED',
            'duplicate' => 'DUPLICATE ARTICLE{S}',
            'reviewReady' => 'ARTICLE{S} READY FOR REVIEW',
            'topicAdded' => 'ARTICLE{S} WITH TOPIC ADDED',
            'replaced' => 'ARTICLE{S} REPLACED',
            'error' => 'ARTICLE{S} WITH ERRORS',
        );
        if ($internal)
            $blocks = array(
                'imported' => 'ARTICLE{S} IMPORTED',
                'duplicate' => 'DUPLICATE ARTICLE{S}',
                'replaced' => 'ARTICLE{S} REPLACED',
                'error' => 'ARTICLE{S} WITH ERRORS',
            );
        foreach ($blocks as $key => $title) {
            $ids = array();
            foreach ($batch->action[$key] as $article)
                $ids[$article[0]] = $article[1];
            $block = Util::import_report_block($title, $ids, $cols);
            $report["report-$key"] = $block;
        }
        return $report;
    }

    /**
     * Helper function to add to a form a collapsible block showing imported
     * articles in a specific category.  Invoked by import_report() above.
     */
    static function import_report_block($title, $ids, $cols) {
        $s = count($ids) == 1 ? '' : 'S';
        $title = count($ids) . ' ' . str_replace('{S}', $s, $title);
        ksort($ids, SORT_NUMERIC);
        $table = array('<table class="pmids">');
        $counter = 0;
        $options = array('attributes' => array('target' => '_blank'));
        foreach ($ids as $pmid => $ebmsid) {
            if ($counter % $cols == 0) {
                if ($counter)
                    $table[] = '</tr>';
                $table[] = '<tr>';
            }
            ++$counter;
            $url = \Ebms\Util::pubmed_url($pmid);
            $pmlink = l($pmid, $url, $options);
            $fclink = '';
            if ($ebmsid) {
                $url = 'citations/full/' . $ebmsid;
                $fclink = ' (' . l($ebmsid, $url, $options) . ')';
            }
            $table[] = "<td>$pmlink$fclink</td>";
        }
        while ($counter++ % $cols)
            $table[] = '<td>&nbsp;</td>';
        $table[] = '</tr></table>';
        return array(
            '#type' => 'fieldset',
            '#title' => $title,
            '#collapsible' => true,
            '#collapsed' => true,
            'ids' => array(
                '#type' => 'markup',
                '#markup' => implode($table),
            ),
        );
    }

    /**
     * Creates and returns an array representing an empty Drupal date
     * field.
     */
    public static function empty_date() {
        return array(
            'year' => 0,
            'month' => 0,
            'day' => 0
        );
    }

    /**
     * Converts a Drupal date field value into a Unix date/time stamp.
     *
     *  @param  array   $date    Stock Drupal date field structured array
     *
     *  @return int              Standard Unix epoch-based seconds counter
     */
    public static function date_field_to_timestamp($date) {
        if (is_string($date))
            $date = self::date_string_to_array($date);
        return mktime(0, 0, 0, $date['month'], $date['day'], $date['year']);
    }

    /**
     * Convert Drupal date field value array to ISO string.
     *
     *  @param  array  $d    Array of pieces of a date (month, day,
     *                       and year)
     *
     *  @return string       ISO formatted string for date
     */
    public static function date_array_to_string($d) {
        return sprintf("%04d-%02d-%02d", $d['year'], $d['month'], $d['day']);
    }

    /**
     * Convert an ISO date string to a Drupal date field value array.
     *
     *  @param  string  $string  ISO-formatted date string
     *
     *  @return array            Array of date components
     */
    public static function date_string_to_array($string) {
        $s = explode('-', substr($string, 0, 10));
        return array(
            'year' => intval(empty($s[0]) ? '0' : $s[0]),
            'month' => intval(empty($s[1]) ? '0' : $s[1]),
            'day' => intval(empty($s[2]) ? '0' : $s[2]),
        );
    }

    /**
     * Validate that a date is in 'YYYY-MM-DD' format.
     *
     *  @param string $ckDate   Date to be checked in ISO format. Time ignored.
     *
     *  @return                 True = success, else invalid ISO date.
     */
    public static function chkISOdate($ckDate) {

        $y = intval(substr($ckDate, 0, 4));
        $m = intval(substr($ckDate, 5, 2));
        $d = intval(substr($ckDate, 8, 2));

        // PHP date checker
        return checkdate($m, $d, $y);
    }

    /**
     * Add period to end of string unless it already has terminal punctuation.
     */
    public static function full_stop($s) {
        if (preg_match('/.*[\.?!]$/', $s))
            return $s;
        return $s . '.';
    }

    /**
     * Check whether a filename ends in one of the allowed extensions.
     * Use this when we don't want Drupal's default behavior for invalid
     * fields to kick in (see, for example, the article full text retrieval
     * queue).
     *
     *  @param  string  $name         File name to be checked
     *  @param  string  $extensions   Space-delimited extensions
     *
     *  @return boolean               True if the file name ends in
     *                                one of the specified extensions;
     *                                otherwise False
     */
    public static function has_valid_extension($name, $extensions) {
        $extensions = preg_replace('/ +/', '|', preg_quote($extensions));
        $regex = '/\.(' . $extensions . ')$/i';
        return preg_match($regex, $name) ? true : false;
    }

    /**
     * Examines a file to determine its MIME type.
     *
     *  @param  string  $path         Location of file to be checked
     *
     *  @return string                MIME type (e.g., "application/pdf")
     */
    public static function get_mime_type($path) {
        $finfo = new \finfo(FILEINFO_MIME_TYPE);
        return $finfo->file($path);
    }

    /**
     * Verify that a parameter is an integer.  The error message
     * emitted if the check fails is minimal, as we expect the
     * failures to be limited to hacking attacks, and we have no
     * incentive to provide an attacker with additional information.
     * If the caller wishes to handle an invalid parameter
     * itself, the second parameter should be omitted.
     *
     *  @param  string  $param        String value for parameter
     *  @param  string  $url          Where to go if check fails
     *
     *  @return int                   Paramater converted to integer
     */
    public static function validate_int($param, $url=null) {
        $int = (int)$param;
        if (strcmp($int, $param)) {
            if ($url) {
                drupal_set_message('Invalid parameter detected.', 'error');
                drupal_goto($url);
            }
            return null;
        }
        else
            return $int;
    }

    /**
     * Downlad a file to the user's workstation.
     *
     *  @param string $filePath         Full path to the file to download.
     *  @param string $showName         File name to appear to the browser.
     *  @param string $contentType      Mime content type.
     *  @param bool $deleteFile         Delete file after sending it.
     *
     *  @return                         False if failure, else no return.
     */
    public static function download_file($filePath, $showName, $contentType,
                                         $deleteFile=false) {

        $fd = fopen($filePath, 'rb');
        if (!$fd) {
            watchdog('downlad_file', "Can't open $filePath for sending",
                      null, WATCHDOG_ERROR);
            return false;
        }

        // Send headers
        drupal_add_http_header('Content-type', $contentType);
        drupal_add_http_header(
                'Content-disposition', "attachment; filename=$showName");
        drupal_send_headers();

        // Send contents
        while (!feof($fd))
            print(fread($fd, 1024));
        fclose($fd);

        // Delete file if requested
        if ($deleteFile)
            drupal_unlink($filePath);

        // Exit causes data to be sent independently of drupal
        drupal_exit();
    }

    /**
     * Get all user custom field item values.
     *
     * Retrieves every field defined in the global $ebms_user_fields,
     * which is populated in ebms.fields.inc.
     *
     * @param int $usr
     *   User uid identifying the user.  Null = use current global user.
     *
     * @return array
     *   Array of field_name = field value.
     *   Only one value is retrieved per field.
     *   NULL if user is not found.
     */
    private static function load_custom_user_fields($usr=NULL) {

        // Identify user and get all vals from the database
        if (is_null($usr)) {
            global $user;
            $usr = $user->uid;
        }
        // XXX Should I throw an exception if user not found?
        $account = user_load($usr);

        // Use the field_names set in ebms.fields.inc and stored in
        // $GLOBALS['ebms_user_fields'] to find values
        module_load_include('inc', 'ebms', 'includes/ebms.fields');
        global $ebms_user_fields;
        $fields = array();
        foreach ($ebms_user_fields as $fld) {
            $fld_name = $fld['field_name'];
            $val_array = field_get_items('user', $account, $fld_name);
            if ($val_array)
                $fields[$fld_name] = $val_array[0]['value'];
        }

        return $fields;
    }

    /**
     * Get user custom field item values.
     *
     *  @param string $field_name
     *    Name of the custom user field (not the table).  See ebms.fields.inc.
     *  @param mixed $usr
     *    Pass user uid to load a user object from the database, works with any
     *      user, not just the current one.
     *    Pass null to use the global current user.
     *
     *  @return string
     *    Single value of the field.  NULL if usr or field value not found.
     *    Note: integers are returned as strings, e.g., '10'.
     */
    public static function get_custom_user_field_value($field_name, $usr=NULL)
    {
        // Data may already be loaded into global.  If not, load it now.
        global $loaded_user_custom_fields;
        if (!isset($loaded_user_custom_fields)) {
            $loaded_user_custom_fields = self::load_custom_user_fields($usr);
        }

        if (array_key_exists($field_name, $loaded_user_custom_fields)) {
            return $loaded_user_custom_fields[$field_name];
        }

        // Either key or user is not found
        return NULL;
    }

    /**
     * Set user custom field item values.  Values are set in the user object
     * and saved to the database.
     *
     * WARNINGS:
     *   1. A side effect of passing null or user object is that object in
     *      memory is modified and saved to disk.  Any other changes in the
     *      user object will also be saved.
     *
     *      This is usually desirable but if saving without modifying the user
     *      in memory is desirable, pass the current user->uid instead of the
     *      object.
     *
     *   2. Field and value types are unchecked.  Be careful!
     *
     *  @param array $items
     *    Array of field name => field value pairs.
     *  @param mixed $usr
     *    Pass the user object to retrieve data from the in-memory object.
     *    Pass user uid to load a user object from the database, works with any
     *      user, not just the current one.
     *    Pass null to use the current user object.
     *
     *  @return
     *    Void.
     */
    public static function set_custom_user_field_values($items, $usr=NULL) {

        // Resolve user
        $account = self::_resolve_usr($usr);

        // Set our value
        foreach ($items as $field_name => $value) {
            // Language should always be LANGUAGE_NONE but I'll check
            $field_info = field_info_field($field_name);
            $lang = $field_info['translatable'] ?
                entity_language($entity_type, $entity) : LANGUAGE_NONE;

            // Set the value into the user object
            $account->{$field_name}[$lang]['0']['value'] = $value;
        }

        // Save to database
        user_save($account);
    }

    /**
     * Find or load a user object.
     *
     * Subroutine of pdq_ebms_(set|get)_custom_user_field_value(s).
     *
     *  @param mixed $usr
     *    Pass the user object to retrieve data from the in-memory object.
     *    Pass user uid to load a user object from the database, works with any
     *      user, not just the current one.
     *    Pass null to use the current user object.
     *
     *  @return
     *    Initialized user object or null if user not found.
     */
    private static function _resolve_usr($usr) {

        // Current user
        if (is_null($usr)) {
            global $user;
            return $user;
        }

        // User identified by integer uid
        else if (ctype_digit($usr)) {
            $account = user_load(intval($usr));
            if (!$account)
                return NULL;
            else
                return $account;
        }

        // No resolution needed (or maybe not possible)
        return $usr;
    }

    /**
     * Location where packet print jobs are put.
     *
     *  @param int $print_job_id
     *    If supplied, return a fully qualified filename incorporating job id
     *    but not the ".tar" which is added by the tar procedure.
     *
     *    Else just return the directory name where print tar files go.
     *
     *  @return string
     *    Directory or filename.
     */
    public static function get_packet_print_path($print_job_id=null) {

        $base_path = '/tmp/ebms/PrintJobs';
        if (!is_null($print_job_id)) {
            // File name
            // Note: Don't test job_id range.  It's probably a string
            return (sprintf("$base_path/PrintJob%05d", $print_job_id));
        }

        // Directory name
        return $base_path;
    }

    /**
     * Create PubMed URL for a given article.
     *
     *  @param string $pmid
     *      Unique ID of the article in PubMed
     *
     *  @return string
     *      HTTPS URL for the article on NLM's web site
     */
    public static function pubmed_url($pmid) {
        return \Ebms\PUBMED_URL . "/$pmid";
    }

    /**
     * Find all of the articles related to this one and create markup
     * identifying each of them.  See OCEEBMS-358 and OCEEBMS-381.
     *
     *  @param int $article_id
     *      Unique ID of the article for which we are seeking
     *      related articles
     *
     *  @param bool $link_ebms_id
     *      Whether to include links to the full citation pages
     *      for the related articles (defaults to false)
     *
     *  @return string
     *      HTML markup identifying related articles
     */
    public static function get_related_articles($article_id, $link_ebms_id=false) {
        // Add links to related articles.
        $query = db_select('ebms_related_article', 'a')
            ->fields('a', array('from_id', 'to_id'))
            ->condition(db_or()
                ->condition('a.from_id', $article_id)
                ->condition('a.to_id', $article_id))
            ->isNull('a.inactivated');
        $results = $query->execute();
        $related_ids = array();
        foreach ($results as $result) {
            $related_id = 0;
            if ($result->from_id != $article_id)
                $related_id = $result->from_id;
            elseif ($result->to_id != $article_id)
                $related_id = $result->to_id;
            if ($related_id && !in_array($related_id, $related_ids))
                $related_ids[] = $related_id;
        }
        sort($related_ids);
        $related_articles = '';
        foreach ($related_ids as $related_id) {
            $article = self::format_related_article($related_id,
                                                    $link_ebms_id);
            if ($article)
                $related_articles .= " $article";
        }
        return $related_articles;
    }

    /**
     * Create the markup identifying another article related to the
     * primary article being displayed. See OCEEBMS-358 and OCEEBMS-381.
     *
     *  @param int $id
     *      Unique ID of the related article to be identified
     *
     *  @param bool $link_ebms_id
     *      Whether to include a link to the full citation page
     *      for the related article
     *
     *  @return string
     *      HTML markup identifying related article
     */
    private static function format_related_article($id, $link_ebms_id=false) {
        $results = db_select('ebms_article', 'a')
            ->fields('a', array('source_data'))
            ->condition('a.article_id', $id)
            ->execute();
        foreach ($results as $result) {
            $display = array();
            $doc = new \SimpleXMLElement($result->source_data);
            if ($doc->MedlineCitation->Article->AuthorList->Author) {
                foreach ($doc->MedlineCitation->Article->AuthorList->Author as $a) {
                    if ($a->LastName)
                        $display[] = htmlspecialchars($a->LastName);
                    break;
                }
            }
            else
                $display[] = '[No authors listed]';
            $ta = $doc->MedlineCitation->MedlineJournalInfo->MedlineTA;
            $display[] = htmlspecialchars($ta);
            $article = $doc->MedlineCitation->Article;
            $year = $article->Journal->JournalIssue->PubDate->Year;
            if ($year)
                $display[] = $year;
            $pmid = $doc->MedlineCitation->PMID;
            $url = self::pubmed_url($pmid);
            $link = "<a href='$url' target='_blank'>$pmid</a>";
            $display[] = "PMID $link";
            if ($link_ebms_id)
                $display[] = " (<a href='/citations/full/$id'>$id</a>)";
            $display = implode(' ', $display);
            return "[RELATED CITATION: $display]";
        }
        return '';
    }

    /**
     * Assemble the list of tags assigned to an article for a topic.
     *
     *  @param int $article_id
     *      Unique ID of the article for which tags have been assigned
     *
     *  @param int $topic_id
     *      Unique ID of the topic associated with the assignment
     *
     *  @return array
     *       Sequence of tag display names
     */
    public static function get_topic_tags($article_id, $topic_id) {
        $query = db_select('ebms_article_tag_type', 'n');
        $query->join('ebms_article_tag', 't', 't.tag_id = n.tag_id');
        $query->condition('t.article_id', $article_id);
        $query->condition('t.topic_id', $topic_id);
        $query->condition('t.active_status', 'A');
        $query->orderBy('t.tag_dt');
        $query->fields('n', array('tag_name'));
        return $query->execute()->fetchCol();
    }
}

class EbmsPager extends \PagerDefault {
    /**
     * The total items found as a part of this query
     *
     * @var int
     */
    public $total_items = -1;

    /**
     * Override PagerDefault's execute method.
     *
     * if page = off, execute with no changes to the contained query.
     */
    public function execute() {

        // Add convenience tag to mark that this is an extended query. We have
        // to do this in the constructor to ensure that it is set before
        // preExecute() gets called.
        if (!$this->preExecute($this)) {
            return NULL;
        }

        // A NULL limit is the "kill switch" for pager queries.
        if (empty($this->limit)) {
            return;
        }
        $this->ensureElement();

        // ALTERATION: check for page==off
        $parms = drupal_get_query_parameters();

        // if no page query parameter or it is not equal to 'off'...
        if (!isset($parms['pager']) || $parms['pager'] != 'off') {
            // ...perform the normal alterations to the query.
            $this->total_items = $this->getCountQuery()->execute()->fetchField();
            $current_page = pager_default_initialize($this->total_items,
                            $this->limit, $this->element);
            $this->range($current_page * $this->limit, $this->limit);
        }
        // !ALTERATION
        // Now that we've added our pager-based range instructions, run the
        // query normally.
        return $this->query->execute();
    }
}
