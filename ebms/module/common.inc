<?php

/**
 * $Id$
 *
 * Common classes, functions, etc., for the EBMS system.
 *
 */

namespace Ebms;

define('Ebms\APP_DIR', base_path());
define('Ebms\THEME_NAME', 'ebmstheme');
define('Ebms\THEME_DIR', APP_DIR . drupal_get_path('theme', THEME_NAME));
define('Ebms\IMAGES_DIR', THEME_DIR . '/images');
define('Ebms\MODULE_DIR', drupal_get_path('module', 'ebms'));
define('Ebms\JS_DIR', MODULE_DIR . '/js');
define('Ebms\FROM', 'PDQ Editorial Board Management System <ebms@cancer.gov>');
define('Ebms\LEFT_ARROW', "\xe2\x97\x80");
define('Ebms\RIGHT_ARROW', "\xe2\x96\xb6");
define('Ebms\UP_ARROW', "\xe2\x96\xb2");
define('Ebms\DOWN_ARROW', "\xe2\x96\xbc");
define('Ebms\ELLIPSIS', "\xe2\x80\xa6");
define('Ebms\PICTURE_SIZE', 135);

/**
 * Initial implementation of discussion forums has one general forum
 * visible to all users, and one forum for each of the PDQ boards.
 * Later on we may add forums for at least some of the subgroups.
 * XXX The Discussions pages haven't been implemented yet.  This
 * initial work was done in order to build the main and sub menus
 * for the home page stories.
 *
 * Forums in the contributed Drupal "Forum" module are registered
 * using the Drupal taxonomy tables.  For now the code below relies
 * on the naming convention that the forums for the EBMS boards
 * are named by concatenating the name of the board (stored in
 * the board_name column of the ebms_board table) with the string
 * " Forum"; shouldn't be hard to change if this convention needs
 * to be altered.
 */
class Forum {
    public
        $id,
        $name,
        $board,
        $subgroup;
    static $forums;

    /**
     * Object to store the Forum name and ID, as well as the
     * board or subgroup which has access to it.  The name of
     * the forum is passed to the constructor and the ID is
     * pulled from the taxonomy_term_data database table.
     * The $board or $subgroup parameter is passed as a hash
     * with "id" and "name" members.
     */
    public function __construct($name, $board=null, $subgroup=null) {
        $this->name = $name;
        $this->board = $board;
        $this->subgroup = $subgroup;
        $this->id = db_select('taxonomy_term_data', 't')
            ->fields('t', array('tid'))
            ->condition('t.name', $name)
            ->execute()
            ->fetchField();
    }

    /**
     * Build the static array of EBMS forums.  Right now that's one
     * general discussion group open to everyone and one for each
     * board.  Cache the array for future access during the life of
     * the current request.
     */
    public static function init() {
        $results = db_select('ebms_board', 'b')
            ->fields('b', array('board_id', 'board_name'))
            ->orderBy('b.board_name')
            ->execute();
        $forum_name = 'General Forum';
        self::$forums[$forum_name] = new Forum($forum_name);
        foreach ($results as $result) {
            $forum_name = $result->board_name . ' Forum';
            $board = array(
                'id' => $result->board_id,
                'name' => $result->board_name,
            );
            self::$forums[$forum_name] = new Forum($forum_name, $board);
        }
    }

    /**
     * Retrieve the static array of forums.  Build the array first if
     * that hasn't been done already.
     */
    public static function forums() {
        if (empty(self::$forums))
            self::init();
        // pdq_ebms_debug('FORUMS', self::$forums);
        return self::$forums;
    }

    /**
     * Returns a Boolean indicating whether the forum represented by
     * this object is accessble to the current user.
     */
    public function visible() {
        global $user;
        if (!user_is_logged_in())
            return false;
        if (isset($this->board))
            return in_array($this->board['id'], $user->ebms_boards);
        if (isset($this->subgroup))
            return in_array($this->subgroup['id'], $user->ebms_subgroups);
        return true;
    }
}

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
    public function __construct($label, $path, $children=null) {
        $this->label = $label;
        $this->path = $path;
        $this->children = $children ? $children : array();
    }

    /**
     * Generate the Drupal 'render array' for this menu item,
     * ready for processing by our custom theming callback.
     * Called for the top-level menu items; children are handled
     * here, without recursion.
     */
    public function render_array() {
        $ra = array('data' => l($this->label, $this->path));
        if ($this->label == Menu::$active)
            $ra['class'] = array('active');
        $children = array();
        foreach ($this->children as $child)
            $children[] = l($child->label, $child->path);
        if (!empty($children))
            $ra['children'] = $children;
        return $ra;
    }

    /**
     * Retrieve the static array of forums.  Build the array first if
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
        if (in_array('medical librarian', $user->roles))
            $role = 'librarian';
        elseif (in_array('board member', $user->roles))
            $role = 'member';
        else
            $role = 'admin';
        switch ($role) {
            case 'librarian':
                self::$menus = array(
                    Menu::citations($role),
                    Menu::reports(),
                    Menu::admin_tool(),
                    Menu::calendar($role),
                    Menu::about(),
                );
                break;
            case 'admin':
                self::$menus = array(
                    Menu::citations($role),
                    Menu::calendar($role),
                    Menu::literature($role),
                    Menu::summaries(),
                    Menu::travel($role),
                    Menu::forums($user),
                    Menu::about(),
                );
                break;
            case 'member':
                self::$menus = array(
                    Menu::calendar($role),
                    Menu::literature($role),
                    Menu::summaries(),
                    Menu::travel($role),
                    Menu::forums($user),
                    Menu::about(),
                );
        }
    }

    /**
     * Build the EBMS menu information for reports.
     */
    public static function reports() {
        return new Menu(
            'Reports', 'reports', array(
                new Menu('Citation Management System Reports', 'reports/cite'),
                new Menu('Not List Reports', 'reports/not-list'),
            )
        );
    }

    /**
     * Build the EBMS administrator tool menu.
     */
    public static function admin_tool() {
        return new Menu(
            'Administrator Tool', 'admin-tool', array(
                new Menu('Manage Summary Topics', 'admin-tool/topics'),
                new Menu('Add Review Cycle', 'admin-tool/add-cycle'),
            )
        );
    }

    /**
     * Build the EBMS menu information for Citation management.
     */
    public static function citations($role) {
        switch ($role) {
            case 'librarian':
                $children = array(
                    new Menu('Import Citations', 'citations/import'),
                    new Menu('Review Citations', 'citations/review'),
                    new Menu('Find/Edit a record', 'citations/search'),
                    new Menu('Add New Journal', 'citations/add-journal'),
                    new Menu('Not List Maintenance', 'citations/not-list'),
                );
                break;
            default:
                $children = array(
                    new Menu('Search Database', 'citations/search'),
                    new Menu('Review Abstracts', 'citations/review'),
                    new Menu('Add Citation', 'citations/add'),
                    new Menu('System Help', 'citations/help'),
                    new Menu('Administrator Tool', 'citations/admin'),
                );
                break;
        }
        return new Menu('Citation Management', 'citations', $children);
    }

    /**
     * Build the EBMS menu information for the calendar pages.
     */
    public static function calendar($role) {
        $menu = new Menu('Calendar', 'calendar');
        if ($role == 'admin' || $role == 'librarian')
            $menu->children = array(
                new Menu('View', 'calendar/view'),
                new Menu('Create Event', 'calendar/create'),
            );
        return $menu;
    }

    /**
     * Build the EBMS menu information for literature review.
     */
    public static function literature($role) {
        $menu = new Menu('Literature', 'review');
        if ($role == 'admin')
            $menu->children = array(
                new Menu('Assigned Packets', 'review/packets'),
                new Menu('Create Packet', 'review/add'),
            );
        if ($role == 'member')
            $menu->children = array(
                new Menu('Assigned Packets', 'review/assigned'),
            );
        return $menu;
    }

    /**
     * Build the EBMS menu information for the Summaries pages.
     */
    public static function summaries() {
        return new Menu('Summaries', 'summaries');
    }

    /**
     * Build the EBMS menu information for the Travel pages.
     */
    public static function travel($role) {
        $menu = new Menu(
            'Travel', 'travel', array(
                new Menu('Travel Information', 'travel/info'),
                new Menu('Hotel Request', 'travel/hotel'),
                new Menu('Reimbursement Request', 'travel/reimbursement'),
            )
        );
        if ($role == 'admin')
            $menu->children[] =
                new Menu('Travel Reports', 'travel/reports');
        return $menu;
    }

    /**
     * Build the EBMS menu information for the Discussion forum
     * pages.
     */
    public static function forums($user) {
        $children = array();
        $forums = Forum::forums();
        //pdq_ebms_debug('MENU FORUMS', $forums);
        foreach ($forums as $forum) {
            if ($forum->visible()) {
                $url = "discussion/{$forum->id}";
                $children[] = new Menu($forum->name, $url);
            }
        }
        $url = $children[0]->path;
        return new Menu('Forums', $url, $children);
    }

    /**
     * Build the EBMS menu information for general information pages.
     */
    public static function about() {
        $reg = drupal_convert_to_utf8("\xAE", 'latin1');
        return new Menu(
            'About', 'about', array(
                new Menu('General Information', 'about/info'),
                new Menu("PDQ$reg Editorial Boards", 'about/boards'),
            )
        );
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
        $ucs2 = iconv("utf-8", "UCS-2", $utf8);
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
     * XXX Replace with Alan's version.
     */
    public static function get_full_citation($article_id) {
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
        $pieces = array(implode(',', $authors), ': <em>', $title, '</em> ',
                  $ta, ' ', $vol);
        if ($issue)
            $pieces[] = "($issue)";
        if ($pgn)
            $pieces[] = ": $pgn";
        if ($year)
            $pieces[] = ", $year";
        $pieces[] = ". $pmid";
        return implode($pieces);
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
     * Build a dictionary of the PDQ boards.  The keys are the board
     * IDs and the values are the board names.
     */
    public static function boards() {
        $results = db_select('ebms_board', 'b')
            ->fields('b', array('board_id', 'board_name'))
            ->orderBy('b.board_name')
            ->execute();
        foreach ($results as $result)
            $boards[$result->board_id] = $result->board_name;
        return $boards;
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
}
