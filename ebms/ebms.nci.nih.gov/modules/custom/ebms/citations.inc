<?php

// $Id$

namespace Ebms {

    require_once('EbmsArticle.inc');

    $GLOBALS['citations_state_map'] = array(
        \Ebms\EbmsArticleStates::ReadyInitReview => array(
            'reject' => \Ebms\EbmsArticleStates::RejectInitReview,
            'pass' => \Ebms\EbmsArticleStates::PassedInitReview,
        ),
        \Ebms\EbmsArticleStates::Published => array(
            'reject' => \Ebms\EbmsArticleStates::RejectBMReview,
            'pass' => \Ebms\EbmsArticleStates::PassedBMReview,
        ),
        \Ebms\EbmsArticleStates::PassedBMReview => array(
            'reject' => \Ebms\EbmsArticleStates::RejectFullReview,
            'pass' => \Ebms\EbmsArticleStates::PassedFullReview,
            'fyi' => \Ebms\EbmsArticleStates::FYI,
        ),
    );

    $GLOBALS['decision_value_map'] = array(
        'initial_review' => array('pass', 'reject', 'comment'),
        'published' => array('select', 'comment', 'state'),
        'nci_reviewer' => array('pass', 'reject', 'comment'),
        'fulltext' => array('file', 'check', 'comment'),
        'surveillance' => array('pass', 'reject', 'fyi', 'comment'),
        'member_responses' => array('packet', 'reviewer', 'select', 'reject',
            'loe', 'comment'),
        'manager_actions' => array('select', 'comment'),
        'on_agenda' => array('future', 'past', 'comment'),
        'board_decision' => array('select', 'discussed', 'members', 'comment'),
    );

    $GLOBALS['citation_cached_value_keys'] = array(
        'board_select',
        'topic_select',
        'cycle_select',
        'tag_select',
        'journal_id_field',
        'abstract_review_check',
        'fulltext_review_check',
        'sort_by',
        'format',
        'per_page',
    );

    /**
     * Class encapsulating information and methods used in decisions (i.e.,
     * state assignments) that can be made about an article.
     *
     * This is general information pertaining to any article, allowing the
     * program to locate information about any state encountered in the state
     * history of an article.  The information includes information about the
     * states themselves, and also some information specific to how we format
     * the state data for display (e.g., $this->title, $this->formatter).
     *
     * The class is initialized and instances created once during a run, and
     * not updated after initialization.
     */
    class EbmsTopicDecisions {
        static $INITIALREVIEW = null;
        static $READYINITIAL = null;
        static $PUBLISHED = null;
        static $NCIREVIEWER = null;
        static $FULLTEXT = null;
        static $SURVEILLANCE = null;
        static $ASSIGNEDFORREVIEW = null;
        static $BOARDMEMBERRESPONSES = null;
        static $BOARDMANAGERACTIONS = null;
        static $ONAGENDA = null;
        static $EDITORIALBOARD = null;
        private $index;
        private $title;
        private $sequence;
        private $formatter;
        private $pass_states;
        private $reject_states;
        private $fyi_states;
        static $currentIndex = 0;

        public function __construct($index, $title, $formatter = null,
            $pass_states = array(), $reject_states = array(),
            $fyi_states = array()) {
            $this->index = $index;
            $this->title = $title;

            $this->formatter = $formatter;
            $this->pass_states = $pass_states;
            $this->reject_states = $reject_states;
            $this->fyi_states = $fyi_states;

            // if any pass states exist, use them to populate the sequence
            $state = reset($pass_states);
            if ($state) {
                $this->sequence = getStateTypeSequence($state);
            }
        }

        public static function init() {
            $index = 1;
            self::$READYINITIAL =
                new EbmsTopicDecisions($index++,
                    'Ready for Initial Review', null,
                    array(EbmsArticleStates::ReadyInitReview));
            self::$INITIALREVIEW =
                new EbmsTopicDecisions($index++,
                    'Medical Librarian Initial Review', null,
                    array(EbmsArticleStates::PassedInitReview),
                    array(EbmsArticleStates::RejectInitReview));
            self::$PUBLISHED =
                new EbmsTopicDecisions($index++, 'Publishing',
                    '_format_topic_publishing', array(EbmsArticleStates::Published));
            self::$NCIREVIEWER =
                new EbmsTopicDecisions($index++, 'NCI Reviewer Decision', null,
                    array(EbmsArticleStates::PassedBMReview),
                    array(EbmsArticleStates::RejectBMReview));
            self::$FULLTEXT =
                new EbmsTopicDecisions($index++, 'Full Text Retrieved', null);
            self::$SURVEILLANCE =
                new EbmsTopicDecisions($index++,
                    'Literature Surveillance Committee Decision', null,
                    array(EbmsArticleStates::PassedFullReview),
                    array(EbmsArticleStates::RejectFullReview),
                    array(EbmsArticleStates::FYI));
            self::$ASSIGNEDFORREVIEW =
                new EbmsTopicDecisions($index++, 'Assigned For Review');
            self::$BOARDMEMBERRESPONSES =
                new EbmsTopicDecisions($index++, 'Board Member Responses');
            self::$BOARDMANAGERACTIONS =
                new EbmsTopicDecisions($index++, 'Board Manager Action(s)',
                    '_format_topic_board_manager_actions',
                    array(
                        EbmsArticleStates::NotForAgenda,
                        EbmsArticleStates::AgendaNoPaprChg,
                        EbmsArticleStates::AgendaFutureChg,
                        EbmsArticleStates::AgendaBoardDiscuss,
                        EbmsArticleStates::AgendaWrkGrpDiscuss,
                        EbmsArticleStates::FullEnd,));
            self::$ONAGENDA =
                new EbmsTopicDecisions($index++, 'On Agenda',
                    '_format_topic_on_agenda_decision',
                    array(EbmsArticleStates::OnAgenda));
            self::$EDITORIALBOARD =
                new EbmsTopicDecisions($index++, 'Editorial Board Decision',
                    '_format_topic_editorial_board_decision',
                    array(EbmsArticleStates::FinalBoardDecision));
        }

        public function index() {
            return $this->index;
        }

        public function title() {
            return $this->title;
        }

        public function sequence() {
            return $this->sequence;
        }

        public function formatter() {
            return $this->formatter;
        }

        public function pass_states() {
            return $this->pass_states;
        }

        public function reject_states() {
            return $this->reject_states;
        }

        public function fyi_states() {
            return $this->fyi_states;
        }

        public function find_state($text_id) {
            if (in_array($text_id, $this->pass_states())) {
                return 'pass';
            } else if (in_array($text_id, $this->reject_states())) {
                return 'reject';
            } else if (in_array($text_id, $this->fyi_states())) {
                return 'fyi';
            }

            return null;
        }
    }

    EbmsTopicDecisions::init();
}

namespace {

    /**
     * Class encapsulating phases of a full citation display.
     */
    class DISPLAY_PHASE {
        const NO_PHASE            = 0;
        const REVIEW_CYCLE        = 1;
        const IMPORT_PROCESS      = 2;
        const MED_LIB_INIT_REVIEW = 3;
        const PUBLISHING          = 4;
    }

    /**
     * Global variable declaring the section of the full citation screen that
     * is currently being processed.  This is global in order to make the data
     * available six levels deep in the call stack from where the data
     * originates without having to modify all of the intervening layers.
     *
     * See _showActionButtons().
     *
     * Values:          One of the DISPLAY_PHASE constants.
     */
    $citations_inc_display_phase = DISPLAY_PHASE::NO_PHASE;

    /**
     * @file
     *
     * Implementation of EBMS page for general site information.
     *
     * Entry point for all functions in this file.
     *
     * @param string $action    What kind of form to build, default (null) = review form.
     * @param int $article_id   What article to build, default (null) = not article specific.
     *
     * @return array $result    Render array for the main page content + side bar.
     *                          MENU_NOT_FOUND if unable to determine type of page requested.
     */
    function pdq_ebms_citations($action = null, $article_id = null) {

        // grab the resource stylesheet
        $css_path = drupal_get_path('theme', 'ebmstheme');
        $css_path .= "/css/citations.css";
        drupal_add_css($css_path, array('type' => 'file'));

        global $user;
        $is_librarian = in_array('medical librarian', $user->roles);

        Ebms\Menu::$active = 'Citation Management';
        drupal_set_title('Citation Management');
        $crumbs = array(
            $action ? l('Citation Management', 'citations') : 'Citation Management'
        );
        $result = null;
        switch ($action) {
            case 'full':
                $crumbs[] = 'Review Citations';
                $crumbs[] = 'Full Citation';

                // Build the full citation main page
                $form = drupal_get_form('pdq_ebms_full_citation', $article_id);

                // Build the left nav components
                $result['left_nav'] = \Ebms\Util::build_left_nav();
                $result['left_nav']['actions'] = _build_full_citation_actions($article_id);
                $result['left_nav']['icon_key'] = _build_full_citation_icon_key();

                // Combine the two into a single render array
                $result['full_citation_form'] = $form;

                break;
            case 'not-list':
                $crumbs[] = "Not List Maintenance";
                $form = drupal_get_form('pdq_ebms_citation_not_list');

                $result['left_nav'] = \Ebms\Util::build_left_nav('Not List Maintenance');

                $result['not_list_form'] = $form;

                break;
            case null:
            case 'review':
                $crumbs[] = 'Review Citations';

                $form = drupal_get_form('pdq_ebms_citation_queue', $is_librarian);

                $result['left_nav'] = \Ebms\Util::build_left_nav();

                $result['cite_queue_form'] = $form;

                break;
        }
        drupal_set_breadcrumb($crumbs);
        if ($result)
            return $result;

        return MENU_NOT_FOUND;
    }

    function pdq_ebms_citation_not_list($form, &$form_state) {

        require_once('EbmsArticle.inc');

        //include js to redirect pager links
        $ret = drupal_add_js(EBMS\JS_DIR . '/citations.js', 'file');

        // hook into ajax completions to apply jQuery to form components
        drupal_add_js('jQuery(document).ajaxComplete(function(){
            addHooks();
        });',
            'inline');

        // handle header sorting
        _update_table_sort($form_state);

        // retrieve boards
        $boards = Ebms\Util::boards();

        $form['#prefix'] = '<div id="citation-div">';
        $form['#suffix'] = '</div>';

        $form['not-list']['#prefix'] = '<div id="citation-form-div">';
        $form['not-list']['#suffix'] = '</div>';

        $form['not-list']['description']['#markup'] = '<h2>Not List Maintenance</h2>';
        $form['not-list']['board_select'] = array(
            '#type' => 'select',
            '#title' => 'Editorial Board',
            '#options' => $boards,
            '#prefix' => '<div class="citation-input-div">',
            '#suffix' => '</div>',
        );
        $form['not-list']['brief_title_field'] = array(
            '#type' => 'textfield',
            '#title' => 'Brief Journal Title',
            '#prefix' => '<div class="citation-input-div">',
            '#suffix' => '</div>',
        );
        $form['not-list']['full_title_field'] = array(
            '#type' => 'textfield',
            '#title' => 'Full Journal Title',
            '#prefix' => '<div class="citation-input-div">',
            '#suffix' => '</div>',
        );
        $form['not-list']['journal_id_field'] = array(
            '#type' => 'textfield',
            '#title' => 'Journal ID',
            '#prefix' => '<div class="citation-input-div">',
            '#suffix' => '</div>',
        );
        $form['not-list']['all_journals_check'] = array(
            '#type' => 'checkbox',
            '#title' => 'All Journals',
            '#prefix' => '<div class="citation-input-div">',
            '#suffix' => '</div>',
        );

        $form['not-list']['reset_button'] = array(
            '#type' => 'submit',
            '#value' => 'Reset',
        );

        $form['not-list']['search_button'] = array(
            '#type' => 'submit',
            '#value' => 'Search',
        );

        $form['not-list']['submit_source'] = array(
            '#type' => 'hidden',
        );

        // build the results if the values exist in the form state
        if (isset($form_state['values'])) {
            $board = $form_state['values']['board_select'];
            $brief_title = $form_state['values']['brief_title_field'];
            $full_title = $form_state['values']['full_title_field'];
            $id = $form_state['values']['journal_id_field'];
            $all_journals = $form_state['values']['all_journals_check'];

            $form['citation-results'] = _not_list_results(array(), $form_state,
                $board, $brief_title, $full_title, $id, $all_journals);
        }

        return $form;
    }

    function pdq_ebms_citation_not_list_submit($form, &$form_state) {
        $form_state['rebuild'] = true;

        // always update the changed list
        _update_journal_select($form_state);

        $clear_states = false;

        if (isset($form_state['values']['op'])) {
            $op = $form_state['values']['op'];
            if (!empty($form_state['values']['submit_source'])) {
                $op = $form_state['values']['submit_source'];
            }

            switch ($op) {
                case 'Reset':
                    $form_state['rebuild'] = false;
                    return;
                case 'Save':
                    // write out the changes to the not list, in terms of
                    // inserts and deletes to the DB
                    _write_not_list($form_state['altered_states']);

                    $clear_states = true;

                    break;
                case 'Search':
                case 'Cancel':
                    $clear_states = true;
                    break;
            }
        }

        if ($clear_states) {
            $form_state['altered_states'] = array();
            unset($form_state['header_sort']);
        }

        // wipe out button values so that the default_states calculated from
        // altered states can take effect
        _clear_journal_select($form_state['input'], 'input');
    }

    /**
     * Should add/edit buttons be shown on the full citation page for any
     * particular article, section of the display, and user?
     *
     * The only function that reads the above global.
     *
     * Note: This routine is a later addition to this module.  Not all
     *       decisions about buttons are made here.  However any change to
     *       those decisions should be made by updating and using this
     *       function.
     *
     * @return bool     True = Yes, show the buttons.
     *                  False = No, leave them out.
     */
    function _showActionButtons() {
        global $user;
        global $citations_inc_display_phase;

        // For most places and user roles, we show the buttons
        $showButtons = true;

        // Either publishing or librarian phases forbid showing action buttons
        // XXX If BM is also icrdb manager, is this still right?
        if (in_array($citations_inc_display_phase,
                    array(DISPLAY_PHASE::MED_LIB_INIT_REVIEW,
                          DISPLAY_PHASE::PUBLISHING))) {
            // But only to board managers
            if (in_array('board manager', $user->roles)) {
                $showButtons = false;
            }
        }

        return $showButtons;
    }

    // parse the values from the form state, keep the journals that have changed
    // from their initial state, and discard those that are unchanged.
    function _update_journal_select(&$form_state) {
        $altered_states = array();
        if (isset($form_state['altered_states']))
            $altered_states = $form_state['altered_states'];

        foreach ($form_state['values'] as $key => $value) {
            if (strpos($key, 'not_list_state_') === 0) {
                $hash = str_ireplace('not_list_state_', '', $key);

                // compare the end of $hash with it's value -
                // if the differ, add to $altered_states,
                // else remove
                if (strrpos($hash, (string) $value) == (strlen($hash) - 1)) {
                    unset($altered_states[$hash]);
                } else {
                    $altered_states[$hash] = $value;
                }
            }
        }

        $form_state['altered_states'] = $altered_states;
    }

    function _update_table_sort(&$form_state) {
        // refactor: get any current sort directives from a triggered header
        // or sort_by value, then determine which one differs from the saved
        // header_sort field.  Finally update header_sort and direction etc
        // based on this information
        // first, current data if available
        if (isset($form_state['header_sort'])) {
            $currField = $form_state['header_sort']['#field'];
            $currDir = $form_state['header_sort']['#dir'];
            $currIx = $form_state['header_sort']['#index'];
        }

        // next, any triggered header_sort
        if (isset($form_state['triggering_element']['#sort'])) {
            $headerField = $form_state['triggering_element']['#sort']['#field'];
            $headerIx = $form_state['triggering_element']['#sort']['#index'];
        }

        // last, find any existing sort_by value, which may have been updated
        // as part of the current submit
        if (isset($form_state['values']['sort_by'])) {
            $sortField = $form_state['values']['sort_by'];
        }

        // determine the new values to use
        // prefer the triggered header above all
        if (isset($headerField)) {
            $newField = $headerField;
            $newIx = $headerIx;

            // if previously set to this header, reverse the sort direction
            if (isset($currField) && $currField == $newField) {
                $newDir = ($currDir == 'ASC' ? 'DESC' : 'ASC');
            }
        } else if (isset($sortField) &&
            (!isset($currField) || ($sortField != $currField))) {
            // use sort_by if able (and no current value or != from sort_by)
            $newField = $sortField;
            $newIx = '';
        }

        // default direction to ascending if not yet set
        if (!isset($newDir))
            $newDir = 'ASC';

        // a new field has been determined, update header_sort
        if (isset($newField)) {
            $form_state['header_sort']['#field'] = $newField;
            $form_state['header_sort']['#dir'] = $newDir;
            $form_state['header_sort']['#index'] = $newIx;
        }
    }

    function _clear_journal_select(&$target, $desc) {
        $button_keys = array();
        foreach ($target as $key => $value) {
            if (strpos($key, 'not_list_state_') === 0) {
                $button_keys[$key] = $value;
            }
        }

        $diff = array_diff_key($target, $button_keys);

        $target = $diff;
    }

    function _write_not_list($values) {
        // need to use the mapped values to insert / delete rows in ebms_not_list
        //
        // loop though the values, explode and split up based on values
        $changed = array(1 => array(), 0 => array());
        foreach ($values as $value => $state) {
            list($source, $id, $board, $initial) = explode('|', $value);
            if ($initial != $state) {

                // add to added or removed based on state
                if (!isset($changed[$state][$board]))
                    $changed[$state][$board] = array();
                if (!isset($changed[$state][$board][$source]))
                    $changed[$state][$board][$source] = array();

                $changed[$state][$board][$source][] = $id;
            }
        }

        // insert new rows, if any
        if (!empty($changed[1])) {
            // loop through and perform inserts

            global $user;
            foreach ($changed[1] as $board_id => $sources) {
                foreach ($sources as $source => $journals) {
                    foreach ($journals as $id) {
                        // insert this journal
                        $timestamp = date('Y-m-d G:i:s');
                        $record = array(
                            'source' => $source,
                            'source_jrnl_id' => $id,
                            'board_id' => $board_id,
                            'start_date' => $timestamp,
                            'user_id' => $user->uid,
                        );
                        db_insert('ebms_not_list')
                            ->fields($record)->execute();
                    }
                }
            }
        }

        // delete removed rows, if any
        if (!empty($changed[0])) {
            // loop through and perform inserts
            foreach ($changed[0] as $board_id => $sources) {
                foreach ($sources as $source => $journals) {
                    $query = db_delete('ebms_not_list');
                    $query->condition('board_id', $board_id);
                    $query->condition('source', $source);
                    $query->condition('source_jrnl_id', $journals);
                    $ret = $query->execute();
                }
            }
        }
    }

    function _not_list_results($form, &$form_state, $board, $brief_title,
        $full_title, $id, $all_journals) {

        // first, refresh the journal list
        require_once('common.inc');
        require_once('EbmsArticle.inc');
        $refreshRet = Ebms\Journal::refresh();

        // collect the display options
        $per_page = 10;

        if (isset($form_state['values']['per_page']))
            $per_page = $form_state['values']['per_page'];

        // if all_journals is set, then a right-join needs to be performed

        $query = db_select('ebms_not_list', 'nl');
        if ($all_journals) {
            $query->rightJoin('ebms_journal', 'j',
                'j.source = nl.source
				AND j.source_jrnl_id = nl.source_jrnl_id
				AND nl.board_id = :board',
                array(':board' => $board));
        } else {
            $query->join('ebms_journal', 'j',
                'nl.source_jrnl_id = j.source_jrnl_id AND nl.source = j.source');
            $query->condition('nl.board_id', $board);
        }

        if ($per_page != 'all')
            $query = $query->extend('Ebms\EbmsPager')->limit($per_page);

        // add the other filter fields
        if ($brief_title)
            $query->condition('j.brf_jrnl_title', "%$brief_title%", 'LIKE');

        if ($full_title)
            $query->condition('j.jrnl_title', "%$full_title%", 'LIKE');

        if ($id)
            $query->condition('j.source_jrnl_id', "%$id%", 'LIKE');

        $num_rows = $query->countQuery()->execute()->fetchField();

        $query
            ->fields('j',
                array('brf_jrnl_title', 'jrnl_title', 'source_jrnl_id', 'source'));

        $query->addExpression('!ISNULL(nl.source_jrnl_id)', 'default_value');

        // set up header sorting
        $field = 'j.jrnl_title';
        $dir = 'ASC';
        $index = 'header-title';
        if (isset($form_state['header_sort'])) {
            $field = $form_state['header_sort']['#field'];
            $dir = $form_state['header_sort']['#dir'];
            $index = $form_state['header_sort']['#index'];
        } else {
            $form_state['header_sort']['#field'] = $field;
            $form_state['header_sort']['#dir'] = $dir;
            $form_state['header_sort']['#index'] = $index;
        }

        $query->orderBy($field, $dir);

        $result = $query->execute();

        $form += array(
            '#prefix' => '<div id="citation-results-div">',
            '#suffix' => '</div>',
        );

        $form['results-title']['#markup'] = "<h2>Not List Search Results <span class = 'results-count'>($num_rows)</span></h2>";

        $form['buttons'] = array(
            '#prefix' => '<div id="citation-buttons-div">',
            '#suffix' => '</div>',
        );

        $form['buttons']['save_button'] = array(
            '#type' => 'submit',
            '#value' => 'Save',
        );

        $form['buttons']['cancel_button'] = array(
            '#type' => 'submit',
            '#value' => 'Cancel',
        );

        $form['buttons']['display_options'] = array(
            '#type' => 'fieldset',
            '#title' => '<span class="citation-subheader">Display Options</span>',
            'options' => _build_citation_sort_options($form_state, false),
            '#collapsible' => true,
            '#collapsed' => true,
            //'#prefix' => "<div class='full-citation-element'>",
            //'#suffix' => "</div>",
        );

        $_GET['q'] = 'citations/not-list';

        $form['top-pager'] = array(
            '#theme' => 'pager',
        );

        $form['table'] = array(
            '#prefix' => '<div id="citation-table-div">',
            '#suffix' => '</div>',
        );

        $form['table']['header'] = array(
            '#prefix' => "<div class='citation-header'>",
            '#suffix' => '</div>',
        );

        /* $form['table']['header'][]['#markup'] =
          "<div class='citation-cell brief-title left-column'>Brief Journal Title</div>"; */

        $form['table']['header']['header-brief-title'] = array(
            '#type' => 'container',
            'header-submit' => array(
                '#type' => 'submit',
                '#value' => 'Brief Journal Title',
                '#sort' => array(
                    '#field' => 'j.brf_jrnl_title',
                    '#index' => 'header-brief-title',
                ),
                '#ajax' => array(
                    'callback' => '_not_list_results_ajax',
                    'wrapper' => 'citation-results-div',
                    'progress' => array(
                        'type' => 'throbber',
                        'message' => t('')
                    ),
                ),
            ),
            '#attributes' => array(
                'class' => array(
                    'citation-cell',
                    'brief-title',
                    'left-column',
                    'sortable',
                ),
            ),
        );

        /* $form['table']['header'][]['#markup'] =
          "<div class='citation-cell full-title'>Full Journal Title</div>"; */
        $form['table']['header']['header-title'] = array(
            '#type' => 'container',
            'header-submit' => array(
                '#type' => 'submit',
                '#value' => 'Full Journal Title',
                '#sort' => array(
                    '#field' => 'j.jrnl_title',
                    '#index' => 'header-title',
                ),
                '#ajax' => array(
                    'callback' => '_not_list_results_ajax',
                    'wrapper' => 'citation-results-div',
                    'progress' => array(
                        'type' => 'throbber',
                        'message' => t('')
                    ),
                ),
            ),
            '#attributes' => array(
                'class' => array(
                    'citation-cell',
                    'full-title',
                    'sortable',
                ),
            ),
        );

        /* $form['table']['header'][]['#markup'] =
          "<div class='citation-cell id'>Journal ID</div>"; */
        $form['table']['header']['header-journal-id'] = array(
            '#type' => 'container',
            'header-submit' => array(
                '#type' => 'submit',
                '#value' => 'Journal ID',
                '#sort' => array(
                    '#field' => 'j.source_jrnl_id',
                    '#index' => 'header-journal-id',
                ),
                '#ajax' => array(
                    'callback' => '_not_list_results_ajax',
                    'wrapper' => 'citation-results-div',
                    'progress' => array(
                        'type' => 'throbber',
                        'message' => t('')
                    ),
                ),
            ),
            '#attributes' => array(
                'class' => array(
                    'citation-cell',
                    'id',
                    'sortable',
                ),
            ),
        );

        if (isset($index)) {
            $suffix = '&#x25b2;';
            if ($dir == 'ASC')
                $suffix = '&#x25bc;';

            $form['table']['header'][$index]['header-submit']['#suffix'] =
                "<div class='inline direction-indicator'>$suffix</div>";
        }

        $form['table']['header'][]['#markup'] =
            "<div class='citation-cell not-check borderless'>Not List</div>";

        $ix = 0;
        $form['table']['rows'] = array();
        foreach ($result as $record) {
            $form['table']['rows'][$ix] = array(
                '#prefix' => "<div class='citation-row'>",
                '#suffix' => '</div>',
            );

            $form['table']['rows'][$ix]['brief_title']['#markup'] =
                "<div class='citation-cell brief-title left-column'>$record->brf_jrnl_title</div>";
            $form['table']['rows'][$ix]['full_title']['#markup'] =
                "<div class='citation-cell full-title'>$record->jrnl_title</div>";
            $form['table']['rows'][$ix]['journal_id']['#markup'] =
                "<div class='citation-cell id'>$record->source_jrnl_id</div>";

            $initial = $record->default_value;
            $journal_hash = "$record->source|$record->source_jrnl_id|$board|$initial";
            $default = $initial;
            if (isset($form_state['altered_states'][$journal_hash])) {
                $default = $form_state['altered_states'][$journal_hash];
            }

            $form['table']['rows'][$ix]["not_list_state_$journal_hash"] = array(
                '#type' => 'checkbox',
                '#title' => "&nbsp;",
                '#default_value' => $default,
                '#prefix' => "<div class='citation-cell not-check borderless'>",
                '#suffix' => "</div>",
                //'#return_value' => ,
                //'#attributes' => array('name' => "search_row_${ix}_button"),
            );

            $ix++;
        }

        $form['trailing-buttons'] = array(
            '#prefix' => '<div id="citation-buttons-div">',
            '#suffix' => '</div>',
        );

        $form['trailing-buttons']['save_button'] = array(
            '#type' => 'submit',
            '#value' => 'Save',
        );

        $form['trailing-buttons']['cancel_button'] = array(
            '#type' => 'submit',
            '#value' => 'Cancel',
        );

        $form['bottom-pager'] = array(
            '#theme' => 'pager',
        );

        return $form;
    }

    function _update_persistent_elements(&$form_state) {
        if (isset($form_state['triggering_element']['#return_value'])
            && isset($form_state['triggering_element']['#persist'])) {

            $persist = $form_state['triggering_element']['#persist'];
            $elementValue = $form_state['triggering_element']['#return_value'];

            // set or clear the related triggered_elements field
            if ($persist['type'] === 'toggle' &&
                isset($form_state['triggered_elements'][$elementValue]) &&
                !empty($form_state['triggered_elements'][$elementValue])) {
                $groupId = $form_state['triggered_elements'][$elementValue]['group'];
                // clear this and all other elements with same groupid
                $toRemove = array();
                foreach ($form_state['triggered_elements'] as $key => $element) {
                    if ($element['group'] === $groupId)
                        $toRemove[] = $key;
                }

                foreach ($toRemove as $elementId) {
                    unset($form_state['triggered_elements'][$elementId]);
                }

                // keep an empty element to prevent counting as triggered
                $form_state['triggered_elements'][$elementValue] = array();
            } else {
                $form_state['triggered_elements'][$elementValue] = $persist;
                $form_state['triggered_elements'][$elementValue]['state'] = $form_state['triggering_element'];
            }
        }
    }

    /**
     * Simple wrapper to include often-needed ctools and modal settings scripts.
     */
    function _include_modal_scripts() {
        // add ctools includes and style for popups
        ctools_include('ajax');
        ctools_include('modal');
        ctools_modal_add_js();

        // create settings for the popup
        $tag_style = array(
            'tag-style' => array(
                'modalSize' => array(
                    'type' => 'fixed',
                    'width' => 800,
                    'height' => 600,
                ),
                'modalOptions' => array(
                    'opacity' => .8,
                    'background-color' => '#696559',
                ),
            ),
        );
        drupal_add_js($tag_style, 'setting');
    }

    function _citation_queue_handle_cached_values(&$form_state,
            $saveActions = false){
        global $citation_cached_value_keys;
        $cachedKeys = drupal_map_assoc($citation_cached_value_keys);

        $query = drupal_get_query_parameters();
        if(isset($query['queue'])){
            $queryId = $query['queue'];
            require_once('search.inc');
            $jsonString = EbmsSearch::fetch_search_spec($queryId, false);

            if($jsonString !== FALSE){
                // populate the values
                $jsonValues = drupal_json_decode($jsonString);

                // if this is at least a valid cache type
                if (isset($jsonValues['type']) &&
                        $jsonValues['type'] == 'citation_queue') {

                    // make use of the values
                    if (isset($jsonValues['values'])) {
                        // merge the values into the form state
                        if (isset($form_state['values'])) {
                            $form_state['values'] = array_merge(
                                $jsonValues['values'], $form_state['values']);
                        } else {
                            $form_state['values'] = $jsonValues['values'];
                        }

                        // save the json string and row id into the form state as
                        // well
                        $form_state['cached_values']['json_string'] = $jsonString;
                        $form_state['cached_values']['id'] = $queryId;
                        $form_state['cached_values']['values'] =
                            $jsonValues['values'];
                    }

                    if(!isset($form_state['altered_topics']) &&
                        isset($jsonValues['altered_topics']) &&
                        isset($jsonValues['source_state'])) {
                        // while the form_state is uninitialized,
                        // import from the cache any article/topic
                        // combo still in the given source state
                        $cachedSourceState = $jsonValues['source_state'];
                        $cachedSourceSequence = Ebms\getStateTypeSequence($cachedSourceState);

                        foreach($jsonValues['altered_topics'] as $key => $actions) {
                            list($articleId, $topicId) = explode('|', $key);
                            $currState = Ebms\getCurrentState($articleId, $topicId);

                            if($currState['sequence'] == $cachedSourceSequence) {
                                $form_state['altered_topics'][$key] = $actions;
                            }
                        }
                    }
                }
            }
        }

        // capture certain inputs for caching
        $jsonValues = array();

        // save any matching values
        if (isset($form_state['values'])) {
            $cachedValues =
                array_intersect_key($form_state['values'], $cachedKeys);

            $jsonValues['values'] = $cachedValues;
        }

        // dig out any altered topics as well
        if($saveActions &&
                isset($form_state['altered_topics']) &&
                !empty($form_state['altered_topics'])) {
            $sourceState = _citation_queue_get_source_state($form_state);

            $jsonValues['altered_topics'] = $form_state['altered_topics'];
            $jsonValues['source_state'] = $sourceState;
        }

        // if any data has been retrieved for caching
        if(!empty($jsonValues)) {
            $jsonValues['type'] = 'citation_queue';

            $jsonString = drupal_json_encode($jsonValues);

            // generate a new database row if the inputs have changed
            if (!isset($form_state['cached_values']['json_string']) ||
                $jsonString != $form_state['cached_values']['json_string']) {
                $form_state['cached_values']['json_string'] = $jsonString;
                $form_state['cached_values']['values'] = $cachedValues;
                global $user;
                $now = strftime("%Y-%m-%d %H:%M:%S");
                $search_id = db_insert('ebms_search')
                    ->fields(
                        array(
                            'when_searched' => $now,
                            'searched_by' => $user->uid,
                            'search_spec' => $jsonString,
                        )
                    )
                    ->execute();

                // save the database row in the form state
                $form_state['cached_values']['id'] = $search_id;
            }
        }

        // set the queue query parameter, in case something builds a link to
        // the current page
        if(isset($form_state['cached_values']['id'])){
            global $_GET;
            $_GET['queue'] = $form_state['cached_values']['id'];
        }
    }

    /**
     * Build an array suitable for use in a select element defining the user's
     * available topics, with article counts for each.
     *
     * @param int $boardId         The chosen board's id
     * @param bool $abstractReview Flag set if generating topics for the
     *                             NCI reviewer step, or false if for a full
     *                             text review
     * @return array               A keyed array of topic id/topic name pairs,
     *                             with article counts for each topic
     */
    function _build_board_topic_options($boardId, $abstractReview) {
        $topicOptions = array();

        // build an option list based on all topics related to the given board
        $topicQuery = db_select('ebms_topic', 't')
            ->condition('t.board_id', $boardId)
            ->condition('t.active_status', 'A')
            ->fields('t')
            ->orderBy('t.topic_name');

        $topicRows = $topicQuery
            ->execute()
            ->fetchAll();

        $targetState = $abstractReview ? Ebms\EbmsArticleStates::Published :
            Ebms\EbmsArticleStates::PassedBMReview;
        $stateId = Ebms\getStateTypebyTextId($targetState)->state_id;

        foreach ($topicRows as $topicRow) {
            // need to query for each topic to find the number of
            // related articles
            //depends on abstract/full text review switch
            $query = db_select('ebms_article_state', 's');
            $query->join('ebms_article', 'a', 'a.article_id = s.article_id');

            if (!$abstractReview)
                $query->isNotNull('a.full_text_id');

            $count = $query->condition('s.topic_id', $topicRow->topic_id)
                ->condition('s.state_id', $stateId)
                ->condition('s.active_status', 'A')
                ->condition('s.current', 'Y')
                ->countQuery()
                ->execute()
                ->fetchField();

            $topicOptions[$topicRow->topic_id] = "$topicRow->topic_name ($count)";
        }

        // The business of determining who the "NCI Reviewer" only has
        // applicability for the abstract review, not the review from full
        // text.
        if (!$abstractReview)
            return $topicOptions;

        // can return this value if th user is unknown or the user has no
        // specific topics
        global $user;
        $account = user_load($user->uid);
        if(!$account || empty($account->topics)){
            return $topicOptions;
        }

        // otherwise, need to examine user topics to see if any are assigned
        $assignedTopics = array();
        $otherTopics = array();
        foreach($topicOptions as $tid => $topicLabel) {
            if(in_array($tid, $account->topics))
                $assignedTopics[$tid] = $topicLabel;
            else
                $otherTopics[$tid] = $topicLabel;
        }

        if (empty($assignedTopics))
            return $topicOptions;

        return array(
            'My Topics' => $assignedTopics,
            'Other Topics' => $otherTopics,
        );
    }

    function pdq_ebms_citation_queue($form, &$form_state, $is_librarian) {
        require_once('EbmsArticle.inc');

        //include js to redirect pager links
        $ret = drupal_add_js(EBMS\JS_DIR . '/citations.js', 'file');

        // hook into ajax completions to apply jQuery to form components
        drupal_add_js('jQuery(document).ajaxComplete(function(){
            addHooks();
        });',
            'inline');

        // if triggered by the board select, clear any current value for
        // topic select
        if (isset($form_state['triggering_element']['#name'])) {
            if ($form_state['triggering_element']['#name'] == 'board_select') {
                unset($form_state['values']['topic_select']);
                unset($form_state['input']['topic_select']);
            }
        }

        // process and save any queued values
        _citation_queue_handle_cached_values($form_state);

        // preserve existing page query, if any
        if(current_path() == 'system/ajax' &&
            isset($form_state['complete form']['#action'])) {
            $url = drupal_parse_url($form_state['complete form']['#action']);

            if(isset($url['query'])) {
                global $_GET;
                $_GET += $url['query'];
            }
        }

        _include_modal_scripts();

        // maintain triggered element states
        _update_persistent_elements($form_state);

        // handle header sorting
        _update_table_sort($form_state);

        // convert any cached values to available defaults
        global $citation_cached_value_keys;
        $cachedKeys = drupal_map_assoc($citation_cached_value_keys);
        foreach($cachedKeys as $cachedKey) {
            $keyname = "default_$cachedKey";
            $$keyname = null;

            if(isset($form_state['cached_values']['values'][$cachedKey]))
                $$keyname = $form_state['cached_values']['values'][$cachedKey];
        }

        // retrieve boards
        $boards = Ebms\Util::boards();

        // pull the default user board, if any
        global $user;

        // comb the user topics to find all related boards
        if (!empty($user->topics)) {
            $topicBoards = db_select('ebms_topic', 't')
                ->condition('t.topic_id', $user->topics)
                ->fields('t', array('board_id'))
                ->distinct()
                ->execute()
                ->fetchCol();
            $userBoards = array_unique(array_merge($user->boards,
                    $topicBoards));
        }
        else
            $userBoards = $user->boards;

        if(!$default_board_select) {
            $default_board_select = reset($userBoards);
            if ($default_board_select === false) {
                $default_board_select = 0;
            }
        }

        $form['#prefix'] = '<div id="citation-complete-div">';
        $form['#suffix'] = '</div>';

        $form['citation_queue']['#prefix'] = '<div id="citation-form-div">';
        $form['citation_queue']['#suffix'] = '</div>';

        $form['citation_queue']['description']['#markup'] = '<h2>Sort My Citation Queue</h2>';

        $form['citation_queue']['board_select'] = array(
            '#type' => 'select',
            '#title' => 'Editorial Board',
            '#options' => array(0 => ''),
            '#default_value' => $default_board_select,
            '#prefix' => '<div class="citation-input-div">',
            '#suffix' => '</div>',
            '#ajax' => array(
                'callback' => '_ajax_citation_queue_board_select',
                'wrapper' => 'topic-select-div',
                'progress' => array(
                    'type' => 'throbber',
                    'message' => t('')
                ),
            ),
        );
        $form['citation_queue']['board_select']['#options'] += $boards;

        $defaultBoardTopics = array();
        $selectionDesc = empty($default_topic_select) ? '' : 'Selected Topics';

        if ($is_librarian) {
            $form['citation_queue']['topic_select'] = array(
                '#type' => 'select',
                '#title' => 'Summary Topic',
                '#options' => array(0 => 'Please select a board.'),
                '#prefix' => '<div id = "topic-select-div" class="citation-input-div">',
                '#suffix' => '</div>',
                '#default_value' => $default_topic_select,
            );
            if (isset($form_state['values']['board_select'])) {
                $board_id = $form_state['values']['board_select'];
                if ($board_id != 0) {
                    $form['citation_queue']['topic_select']['#options'] =
                        array(0 => 'Please select a topic.');

                    $topicOptions = array();
                    try {
                        $topicOptions = db_select('ebms_topic', 't')
                            ->condition('board_id', $board_id)
                            ->condition('t.active_status', 'A')
                            ->fields('t', array('topic_id', 'topic_name'))
                            ->orderBy('t.topic_name')
                            ->execute()
                            ->fetchAllKeyed();
                    } catch (Exception $e) {

                    }

                    $form['citation_queue']['topic_select']['#options'] =
                        $topicOptions;
                }
            }
        } else {
            $form['citation_queue']['topic_select'] = array(
                '#type' => 'select',
                '#title' => 'Summary Topic',
                '#options' => array(0 => 'Please select a board.'),
                '#prefix' => '<div id = "topic-select-div" class="citation-input-div">',
                '#suffix' => '</div>',
                '#default_value' => $default_topic_select,
            );

            // use the default board id, or dig the value from the form state
            $board_id = $default_board_select;
            $abstractReview = 1;
            if (isset($form_state['values']['board_select']))
                $board_id = $form_state['values']['board_select'];

            if ($board_id != 0) {
                if (isset($form_state['values']['abstract_review_check']))
                    $abstractReview = $form_state['values']['abstract_review_check'];

                $topicOptions = _build_board_topic_options($board_id,
                    $abstractReview);

                $form['citation_queue']['topic_select']['#options'] = $topicOptions;
                $form['citation_queue']['topic_select']['#multiple'] = true;
                $form['citation_queue']['topic_select']['#size'] = 10;

                // if no other source of default topic selection exists, then
                // choose topics under 'My Topics' if available
                if (
                    empty(
                        $form['citation_queue']['topic_select']['#default_value']) &&
                    isset($topicOptions['My Topics'])) {
                    $defaultBoardTopics =
                        array_keys($topicOptions['My Topics']);
                    $selectionDesc = 'My Topics';
                }
            }
        }

        if ($is_librarian) {
            $form['citation_queue']['journal_id_field'] = array(
                '#type' => 'textfield',
                '#title' => 'Journal ID',
                '#prefix' => '<div class="citation-input-div">',
                '#suffix' => '</div>',
                '#default_value' => $default_journal_id_field,
            );
        } else {

            // get all of the cycles
            $cycles = Ebms\Util::cycles(); /* db_select('ebms_cycle', 'c')
              ->fields('c')
              ->execute()
              ->fetchAllKeyed(); */

            $form['citation_queue']['cycle_select'] = array(
                '#type' => 'select',
                '#title' => 'Review Cycle',
                '#options' => array(0 => ''),
                '#prefix' => '<div class="citation-input-div">',
                '#suffix' => '</div>',
                '#default_value' => $default_cycle_select,
            );
            $form['citation_queue']['cycle_select']['#options'] += $cycles;

            // get all of the tags
            $tags = db_select('ebms_article_tag_type', 't')
                ->fields('t', array('tag_id', 'tag_name'))
                ->execute()
                ->fetchAllKeyed();

            $form['citation_queue']['tag_select'] = array(
                '#type' => 'select',
                '#title' => 'Tag',
                '#options' => array(0 => ''),
                '#prefix' => '<div class="citation-input-div">',
                '#suffix' => '</div>',
                '#default_value' => $default_tag_select,
            );
            $form['citation_queue']['tag_select']['#options'] += $tags;

            $checks = array();

            $checks["abstract_review_check"] = array(
                '#type' => 'checkbox',
                '#title' => t('Abstract Review'),
                '#default_value' => $default_abstract_review_check !== null ?
                    $default_abstract_review_check : true,
            );

            $checks["fulltext_review_check"] = array(
                '#type' => 'checkbox',
                '#title' => t('Full Text Review'),
                '#default_value' => $default_fulltext_review_check
            );

            $form['citation_queue']["review_select_checks"] = array(
                'checks' => $checks,
                '#prefix' => '<div class="citation-input-div full-citation-radio-check clearfix">',
                '#suffix' => '</div>',
            );
        }

        $form['citation_queue']['reset_button'] = array(
            '#type' => 'submit',
            '#value' => 'Reset',
        );

        $form['citation_queue']['filter_button'] = array(
            '#type' => 'submit',
            '#value' => 'Filter',
            '#attributes' => array(
                'class' => array('filter-button-submit'),
            ),
        );

        $form['citation_queue']['submit_source'] = array(
            '#type' => 'hidden',
        );

        _citation_queue_results($form, $form_state, $is_librarian,
            $default_board_select,
            $default_topic_select ? $default_topic_select : $defaultBoardTopics,
            $selectionDesc);

        return $form;
    }

    function _ajax_citation_queue_board_select($form, $form_state) {
        return $form['citation_queue']['topic_select'];
    }

    function _citation_queue_results(&$form, &$form_state, $is_librarian,
        $default_board, $default_topics, $selectionDesc = '') {
        $board_id = $default_board;
        $topic_id = $default_topics;
        $journal_id = '';
        $sort_by = 'ebms_id';
        $format = array();
        $per_page = 10;
        $core_only = false;
        $fulltext_review = false;
        $cycle_id = 0;
        $tag_id = 0;

        // get values if they exist
        if (isset($form_state['values'])) {
            $board_id = $form_state['values']['board_select'];
            if (isset($form_state['values']['topic_select']))
                $topic_values = $form_state['values']['topic_select'];

            if(!empty($topic_values))
                $topic_id = $topic_values;

            $format = $form_state['values']['format'];
            $per_page = $form_state['values']['per_page'];
            //$core_only = $form_state['values']['core_only'];

            if ($is_librarian) {
                $journal_id = $form_state['values']['journal_id_field'];
            } else {
                $fulltext_review = $form_state['values']['fulltext_review_check'];
                $cycle_id = $form_state['values']['cycle_select'];
                $tag_id = $form_state['values']['tag_select'];
            }
        }

        // set up header sorting
        $dir = 'ASC';
        $index = '';
        if (isset($form_state['header_sort'])) {
            $sort_by = $form_state['header_sort']['#field'];
            $dir = $form_state['header_sort']['#dir'];
            $index = $form_state['header_sort']['#index'];
        }

        if (!isset($form['citation_queue_results'])) {
            $form['citation_queue_results'] = array(
                '#prefix' => '<div id="citation-results-div">',
                '#suffix' => '</div>',
            );
        }

        $results = &$form['citation_queue_results'];

        try {
            // find the proper source state for the articles
            $source_state = \Ebms\EbmsArticleStates::ReadyInitReview;
            if (!$is_librarian) {
                $source_state = \Ebms\EbmsArticleStates::Published;
                if ($fulltext_review)
                    $source_state = \Ebms\EbmsArticleStates::PassedBMReview;
            }
            $state = Ebms\getStateTypebyTextId($source_state);

            // first generate a list of valid article ids if the topic is specified
            if ($topic_id) {
                $selectedTopics = array();
                if (is_array($topic_id)) {
                    // grab any non-zero value from the topic_select options
                    foreach ($topic_id as $selected_id) {
                        if ($selected_id)
                            $selectedTopics[] = $selected_id;
                    }
                }
                else {
                    $selectedTopics[] = $topic_id;
                }
            }

            // query article state table first
            $stateQuery = db_select('ebms_article_state', 'eas');

            // Omit states for topics which are no longer active.
            $stateQuery->join('ebms_topic', 'topic', 'topic.topic_id = eas.topic_id');
            $stateQuery->condition('topic.active_status', 'A');

            // limit by proper state and current flag
            $stateQuery->condition('eas.state_id', $state->state_id);
            $stateQuery->condition('eas.current', 'Y');

            // if filter topics exist, select all articles that belong to
            // any of them
            if (!empty($selectedTopics)) {
                $stateQuery->condition('eas.topic_id', $selectedTopics, 'IN');
            }

            // filter by board as well
            if ($board_id)
                $stateQuery->condition('eas.board_id', $board_id);

            if ($tag_id) {
                // need to join query to ebms_article_tag
                $stateQuery->join('ebms_article_tag', 't',
                    't.article_id = eas.article_id');
                $stateQuery->condition('t.tag_id', $tag_id);
            }

            // select only article_id
            $stateQuery->fields('eas', array('article_id'));

            // join to article table
            $articleQuery = db_select('ebms_article', 'ea');
            $articleQuery->join($stateQuery, 's', 's.article_id = ea.article_id');

            if ($fulltext_review)
                $articleQuery->isNotNull('ea.full_text_id');

            if ($journal_id)
                $articleQuery->condition('ea.source_jrnl_id', "%$journal_id%",
                    'LIKE');

            if ($cycle_id) {
                // ultimately need to filter on article id and topic id by the given cycle
                $articleQuery->join('ebms_article_topic', 'eat',
                    "eat.article_id = ea.article_id");
                $articleQuery->condition('eat.cycle_id', $cycle_id);
            }

            $articleQuery->fields('ea',
                array(
                'article_id',
                'source_id',
                'article_title',
                'jrnl_title',
                'brf_jrnl_title',
                'published_date',
                )
            );

            // wrap the article query in a sort query
            $sortQuery = db_select($articleQuery, 'a');
            $sortQuery->fields('a', array('article_id'));

            // add ORDER BYs based on display options
            switch ($sort_by) {
                case 'pmid':
                    $sortQuery->orderBy('a.source_id');
                    break;
                case 'ebms_id':
                    $sortQuery->orderBy('a.article_id');
                    break;
                case 'title':
                    $sortQuery->orderBy('a.article_title');
                    break;
                case 'journal':
                    $sortQuery->orderBy('a.jrnl_title');
                    $sortQuery->orderBy('a.brf_jrnl_title');
                    break;
                case 'publication_date':
                    $sortQuery->orderBy('a.published_date');
                    break;
                case 'author':
                    _add_author_sort($sortQuery, $dir);
                    break;
            }

            $sortQuery->distinct();

            $num_rows = db_select($sortQuery, 'sq')
                ->fields('sq')
                ->countQuery()
                ->execute()
                ->fetchField();

            if ($per_page != 'all')
                $sortQuery = $sortQuery->extend('Ebms\EbmsPager')->limit($per_page);

            $result = $sortQuery->execute();
        } catch (Exception $e) {
            $num_rows = -1;
            $result = array();
            watchdog('EBMS - citations', (string) $e);
        }

        if ($is_librarian) {
            $results['results-title']['#markup'] = "<h2>Citations Requiring Review <span class = 'results-count'>($num_rows)</span></h2>";
        } else {
            $suffix = empty($selectionDesc) ? '' : "- $selectionDesc ";
            if ($fulltext_review) {
                $results['results-title']['#markup'] = "<h2>Full Text Requiring Review $suffix<span class = 'results-count'>($num_rows)</span></h2>";
            } else {
                $results['results-title']['#markup'] = "<h2>Abstracts Requiring Review $suffix<span class = 'results-count'>($num_rows)</span></h2>";
            }
        }

        $results['buttons'] = array(
            '#prefix' => '<div id="citation-buttons-div">',
            '#suffix' => '</div>',
        );

        $results['buttons']['save_button'] = array(
            '#type' => 'submit',
            '#value' => 'Submit',
        );

        $results['buttons']['display-options'] = array(
            '#type' => 'fieldset',
            '#title' => '<span class="citation-subheader">Display Options</span>',
            'options' => _build_citation_sort_options($form_state, true, $sort_by),
            '#collapsible' => true,
            '#collapsed' => true,
        );

        if (!$num_rows)
            return;

        $_GET['q'] = 'citations';

        $results['top-pager'] = array(
            '#theme' => 'pager',
        );

        $results['table'] = array(
            '#prefix' => "<table id='citation-results-table'>",
            '#suffix' => '</table>',
        );

        $results['table']['header'] = array(
            '#prefix' => "<tr>",
            '#suffix' => '</tr>',
        );

        // width of the full citation and
        $colspan = $fulltext_review ? 5 : 4;
        $isAbstract = ($format == 'abstract');

        if ($isAbstract) {
            $results['table']['header']['actions']['#markup'] =
                "<th class='actions left-column borderless'>&nbsp;</th>";
            /* $results['table']['header']['abstracts']['#markup'] =
              "<th colspan='$colspan' class='borderless'>Citation Abstracts</th>"; */

            $results['table']['header']['abstracts'] = array(
                '#type' => 'submit',
                '#value' => 'Citation Abstracts',
                '#sort' => array(
                    '#field' => 'author',
                    '#index' => 'abstracts',
                ),
                '#attributes' => array(
                    'class' => array(
                        'sortable',
                    ),
                ),
                '#prefix' => "<th colspan='$colspan' class='borderless'>",
                '#suffix' => "</th>",
            );

            if ($sort_by == 'author')
                $index = 'abstracts';
        } else {
            $results['table']['header']['actions']['#markup'] =
                "<th class='actions left-column borderless'>&nbsp;</th>";
            /* $results['table']['header']['brief_citation']['#markup'] =
              "<th class='brief-citation borderless'>Citations</th>"; */

            $results['table']['header']['brief_citation'] = array(
                '#type' => 'submit',
                '#value' => 'Citations',
                '#ajax' => array(
                    'callback' => '_citation_queue_results_ajax',
                    'wrapper' => 'citation-results-div',
                    'progress' => array(
                        'type' => 'throbber',
                        'message' => t('')),
                ),
                '#sort' => array(
                    '#field' => 'author',
                    '#index' => 'brief_citation',
                ),
                '#attributes' => array(
                    'class' => array(
                        'sortable',
                    ),
                ),
                '#prefix' => "<th class='brief-citation borderless'>",
                '#suffix' => "</th>",
            );

            if ($sort_by == 'author')
                $index = 'brief_citation';

            $results['table']['header']['board']['#markup'] =
                "<th class='board'>Board</th>";
            $results['table']['header']['topic']['#markup'] =
                "<th class='summary-topic-name'>Summary Topic</th>";
            $results['table']['header']['pass']['#markup'] =
                "<th class='summary-topic-check'>Reject</th>";
            $results['table']['header']['reject']['#markup'] =
                "<th class='summary-topic-check'>Pass</th>";
            if ($fulltext_review) {
                $results['table']['header']['fyi']['#markup'] =
                    "<th class='summary-topic-check'>FYI</th>";
            }
        }

        if ($index) {
            $suffix = '&#x25b2;';
            if ($dir == 'ASC')
                $suffix = '&#x25bc;';

            $results['table']['header'][$index]['#suffix'] =
                "<div class='inline direction-indicator'>$suffix</div>" .
                $results['table']['header'][$index]['#suffix'];
        }

        $page = 0;
        $parms = drupal_get_query_parameters();
        if (isset($parms['page']) && is_numeric($parms['page'])) {
            $page = $parms['page'];
        }

        $rows_per_page = 10;

        $first_row = ($page * $rows_per_page) + 1;

        $ix = 0;
        $results['table']['rows'] = array();
        foreach ($result as $record) {
            $articleId = $record->article_id;
            $article = new Ebms\EbmsArticle($articleId);

            // retrieve the topics and related boards
            $curr_topics_query = db_select('ebms_article_state', 's');
            $curr_topics_query->join('ebms_topic', 't', 't.topic_id = s.topic_id');
            $curr_topics = $curr_topics_query->condition('t.active_status', 'A')
                ->condition('s.article_id', $articleId)
                ->condition('s.current', 'Y')
                //->condition('s.state_id', $state->state_id)
                ->fields('s', array('topic_id', 'state_id'))
                ->execute()
                ->fetchAllKeyed();

            //$curr_topics = explode(',', $record->topics);
            // grab all topics and board from the topic objects
            $boards = array();
            $topics = array();
            foreach ($article->getTopics() as $topic) {
                if (isset($curr_topics[$topic->topic_id])) {
                    $boardName = $topic->board_name;
                    if (!in_array($boardName, $boards))
                        $boards[] = $boardName;
                    $topic->curr_state = $curr_topics[$topic->topic_id];
                    $topics[] = $topic;
                }
            }

            // now that all boards are known and indexed, create an array that
            // maps boards to topic objects
            $boardTopicMap = array();
            foreach ($boards as $boardName) {
                $localMap = array();
                // add each matching topic using an integer index
                foreach ($topics as $topic) {
                    if ($boardName == $topic->board_name) {
                        $localMap['topics'][] = $topic;
                    }
                }
                // store the board name
                $localMap['boardName'] = $boardName;

                // add the mapping using an integer index
                $boardTopicMap[] = $localMap;
                unset($localMap);
            }

            $topicCount = sizeof($topics);

            // need to construct a set of rows for eact record, as rowspans will
            // be used for every column apart from topics
            // $topicRows indicates the total number of rows per result, with
            // the actions and citation columns spanning all rows,
            // each topic spanning just its topics' rows, and the topics each
            // taking up a single row.
            // rows will look something like
            // actions  citation    board   topic
            //                              topic
            //                      board   topic
            //                              topic
            //                              topic
            // will use an index into the current board and topic to generate
            // each row, incrementing topic until it rolls over into the next
            // board, and then board until all boards are done.

            $rowIx = 0;
            $boardIx = 0;
            $topicIx = 0;
            $groupId = "";

            // set up the rows array, and prep the first one.
            $rows = array();

            $rows[$rowIx] = array(
                '#prefix' => "<tr>",
                '#suffix' => '</tr>',
            );

            $row = &$rows[$rowIx];

            // always add actions and citations to the first row
            // build actions.
            //
            // will need a separate row for these two items if in the full-citation view
            // build citation
            $cite = _cite_article($article, false, $isAbstract, $fulltext_review);

            $recordIx = $ix + $first_row;

            $expandButton = _build_expand_boards_button($form_state,
                $article->getId());

            // grab the groupId to use for the related elements
            foreach ($expandButton['button'] as $button) {
                if (isset($button['#persist']['group'])) {
                    $groupId = $button['#persist']['group'];
                    break;
                }
            }

            // IF THE BOARDS AND TOPICS ARE EXPANDED FOR ADDITION:
            if (isset($expandButton['triggered'])) {
                // need to add placeholders to the board/topic map
                // to represent the slots for adding.
                foreach ($boardTopicMap as $index => $localMap) {
                    $boardTopicMap[$index]['topics'][] = 'Add New';
                    $topicCount++;
                }

                $boardTopicMap[] = array(
                    'boardName' => 'Add New',
                    'topics' => array(),
                );
                $topicCount++;
            }

            if ($isAbstract) {
                $row['actions']['#prefix'] = "<td class='action-cell borderless'>";
                $row['actions']['#suffix'] = "</td>";

                $row['actions']['row_num'] = array('#markup' => "<h2 class='borderless'>$recordIx</h2>");

                $row['actions']['edit'] = $expandButton;
                _wrap_in_classes($row['actions']['edit'], array('clearfix'));

                $row['actions']['tag'] = _build_add_tag_popup($article->getId());
                _wrap_in_classes($row['actions']['tag'], array('clearfix'));

                // set the abstract to the remaining width of the table, finish
                // the row, and begin another with a header for the boards and topics
                $row['abstract'] = array(
                    '#prefix' => "<td colspan='$colspan' class='left-column borderless'>",
                    'citation' => $cite,
                    '#suffix' => "</td>",
                );

                $rows["${rowIx}_header"] = array(
                    '#prefix' => "<tr>",
                    '#suffix' => '</tr>',
                );

                $row = &$rows["${rowIx}_header"];

                $row['actions']['#markup'] =
                    "<th class='actions left-column borderless'>&nbsp;</th>";
                $row['board']['#markup'] =
                    "<th class='board borderless'>Board(s)</th>";
                $row['topic']['#markup'] =
                    "<th class='summary-topic-name'>Summary Topic(s)</th>";
                $row['pass']['#markup'] =
                    "<th class='summary-topic-check'>Reject</th>";
                $row['reject']['#markup'] =
                    "<th class='summary-topic-check'>Pass</th>";
                if ($fulltext_review) {
                    $row['fyi']['#markup'] =
                        "<th class='summary-topic-check'>FYI</th>";
                }

                $rows["${rowIx}_boards"] = array(
                    '#prefix' => "<tr>",
                    '#suffix' => '</tr>',
                );

                $row = &$rows["${rowIx}_boards"];

                // finally, set up the actual row for the boards and topics
            } else {
                $row['actions']['#prefix'] = "<td rowspan='$topicCount' class='action-cell borderless'>";
                $row['actions']['#suffix'] = "</td>";

                $row['actions']['row_num'] = array('#markup' => "<h2 class='borderless'>$recordIx</h2>");
                /* $row['actions']['flag'] = array('#type' => 'submit', '#value' => t('Flag'), '#suffix' => '<br />'); */

                $row['actions']['edit'] = $expandButton;
                _wrap_in_classes($row['actions']['edit'], array('clearfix'));

                $row['actions']['tag'] = _build_add_tag_popup($article->getId());
                _wrap_in_classes($row['actions']['tag'], array('clearfix'));

                $row['brief_citation'] = array(
                    '#prefix' => "<td rowspan='$topicCount' class='brief-citation left-column borderless'>",
                    'citation' => $cite,
                    '#suffix' => "</td>",
                );
            }

            while (isset($boardTopicMap[$boardIx])) {
                // add the board (and potentially actions), and cycle through
                // the topics
                $topicIx = 0;
                $boardName = $boardTopicMap[$boardIx]['boardName'];
                $currBoard = 0;
                if ($boardName !== 'Add New') {
                    $currBoard = Ebms\getBoardIdByName($boardName);
                }
                $boardTopicCount = sizeof($boardTopicMap[$boardIx]['topics']);

                $boardClass = 'board-cell';
                if ($isAbstract) {
                    if ($currBoard) {
                        $expandButton = _build_expand_boards_button($form_state,
                            $article->getId(), $currBoard);

                        // grab the groupId to use for the related elements
                        foreach ($expandButton['button'] as $button) {
                            if (isset($button['#persist']['group'])) {
                                $groupId = $button['#persist']['group'];
                                break;
                            }
                        }

                        // IF THE BOARDS AND TOPICS ARE EXPANDED FOR ADDITION:
                        if (isset($expandButton['triggered'])) {
                            // need to add a placeholder to the board/topic map
                            // to represent the slot for adding a topic.
                            $boardTopicMap[$boardIx]['topics'][] = 'Add New';
                            $boardTopicCount++;
                        }

                        $row['actions']['edit'] = $expandButton;
                        _wrap_in_classes($row['actions']['edit'],
                            array('clearfix'));
                    } else {
                        // else if no board, then the row must be a created to
                        // select one
                        $boardTopicCount++;
                    }

                    $row['actions']['#prefix'] = "<td rowspan='$boardTopicCount' class='action-cell borderless'>";
                    $row['actions']['#suffix'] = "</td>";

                    $boardClass = "borderless";
                }

                // preserve any existing choice for new boards
                $boardTriggered = false;
                if ($currBoard === 0) {
                    // first check for persistent value
                    $hash = "$articleId|";
                    $selectId = "add_board_$hash";

                    if (_is_triggered_by($form_state, $selectId)) {
                        // check for existing board choice
                        if (isset($form_state['values'][$selectId])) {
                            $currBoard = $form_state['values'][$selectId];
                            $boardName = Ebms\getBoardNameById($currBoard);
                        }

                        if ($form_state['triggering_element']['#return_value'] == $selectId)
                            $boardTriggered = true;
                    }

                    // create dropdown
                    // build a list of board options
                    $boardOptions = array();
                    $dbBoards = db_select('ebms_board', 'b')->fields('b')->execute()->fetchAll();
                    foreach ($dbBoards as $board) {
                        $boardFound = false;
                        foreach ($boardTopicMap as $localMap) {
                            if ($localMap['boardName'] === $board->board_name) {
                                $boardFound = true;
                                break;
                            }
                        }

                        if (!$boardFound)
                            $boardOptions[$board->board_id] = $board->board_name;
                    }

                    if (empty($boardOptions)) {
                        // nothing to show, blank this row out
                        $row = array(
                            '#prefix' => "<tr>",
                            '#suffix' => '</tr>',
                        );
                        $boardIx++;
                        continue;
                    }

                    $options = array();
                    $options[0] = strtoupper('ADD BOARD');
                    $options += $boardOptions;

                    $row['board'][$selectId] = array(
                        '#type' => 'select',
                        '#title' => '',
                        '#options' => $options,
                        '#return_value' => $selectId,
                        '#persist' => array(
                            'type' => 'update',
                            'group' => $groupId),
                        '#ajax' => array(
                            'event' => 'change',
                            'callback' => '_ajax_expand_boards',
                            'wrapper' => 'citation-results-div',
                            'progress' => array(
                                'type' => 'throbber',
                                'message' => t('')
                            ),
                        //'progress' => array('message' => '!'),
                        ),
                    );

                    $row['board']['#prefix'] = "<td class='$boardClass'>";
                    $row['board']['#suffix'] = "</td>";

                    $topicText = '&nbsp';

                    // if no board choice, drop in an empty topic
                    if ($currBoard === 0) {
                        $topicspan = $colspan - 1;
                        $row['empty']['#markup'] = "<td colspan='$topicspan'>$topicText</td>";
                    } else {
                        // else add an 'add new' element to the board map
                        $boardTopicMap[$boardIx]['topics'][] = 'Add New';
                    }
                } else {
                    $row['board']['#markup'] = "<td class='$boardClass' rowspan='$boardTopicCount'>$boardName</td>";
                }

                while (isset($boardTopicMap[$boardIx]['topics'][$topicIx])) {
                    $topic = $boardTopicMap[$boardIx]['topics'][$topicIx];

                    // default the current topic and name to zero and empty
                    $currTopic = 0;
                    $topicName = '';
                    $topicAdded = false;

                    if ($topic == 'Add New') {
                        $topicName = $topic;
                    } else {
                        $currTopic = $topic->topic_id;
                        $topicName = $topic->topic_name;
                    }

                    if ($currTopic === 0) {
                        $topicSelectHash = "$articleId|$currBoard";
                        $topicSelectId = "add_topic_$topicSelectHash";

                        // check for existing board choice
                        if (isset($form_state['values'][$topicSelectId])) {
                            $currTopic = $form_state['values'][$topicSelectId];
                        }

                        // check the triggered elements as a backup
                        if (isset($form_state['triggered_elements'][$topicSelectId]['state']['#value'])) {
                            $currTopic = $form_state['triggered_elements'][$topicSelectId]['state']['#value'];
                        }

                        $topicBoard = 0;
                        // look for board match with topic
                        if ($currTopic != 0)
                            $topicBoard = Ebms\getTopicBoard($currTopic);

                        if (!$boardTriggered && $currTopic && $currBoard == $topicBoard) {
                            $topicAdded = true;
                            $topicName = Ebms\getTopicNameById($currTopic);

                            // need to replace the current index with a proper topic object
                            $topic = db_select('ebms_topic', 't')
                                ->condition('t.topic_id', $currTopic)
                                ->fields('t')
                                ->execute()
                                ->fetch();
                        } else {
                            // clear any old value from trigggered_elements
                            unset($form_state['triggered_elements'][$topicSelectId]);

                            // current topic choice is irrelevant at this point
                            $currTopic = 0;

                            // if the topic is still 0, carry on with no selected topic
                            //
                            // find the board id, and build a list or available topics
                            if ($currBoard) {
                                // subquery to find the in-use topics for the
                                // article
                                $usedTopic = db_select('ebms_article_topic',
                                        'eat')
                                    ->condition('eat.article_id', $articleId)
                                    ->fields('eat', array('topic_id'));

                                // find all active topics for the board that
                                // do not match the in-use topics
                                $availableTopics = db_select('ebms_topic', 't')
                                    ->condition('t.board_id', $currBoard)
                                    ->condition('t.active_status', 'A')
                                    ->condition('t.topic_id', $usedTopic,
                                        'NOT IN')
                                    ->orderBy('t.topic_name')
                                    ->fields('t',
                                        array('topic_id', 'topic_name'))
                                    ->execute()
                                    ->fetchAllKeyed();

                                if (empty($availableTopics)) {
                                    // simply add a blank cell of the proper width
                                    $topicspan = $colspan - 1;
                                    $row['empty']['#markup'] = "<td colspan='$topicspan' class='summary-topic-name'>&nbsp;</td>";
                                } else {
                                    // create a select field for the topic
                                    $options = array();
                                    $options[0] = strtoupper("ADD $boardName TOPIC");
                                    $options += $availableTopics;

                                    $row['topic_select'][$topicSelectId] = array(
                                        '#type' => 'select',
                                        '#title' => '',
                                        '#options' => $options,
                                        '#return_value' => $topicSelectId,
                                        '#persist' => array(
                                            'type' => 'update',
                                            'group' => $groupId),
                                        '#ajax' => array(
                                            'event' => 'change',
                                            'callback' => '_ajax_expand_boards',
                                            'wrapper' => 'citation-results-div',
                                            'progress' => array(
                                                'type' => 'throbber',
                                                'message' => t('')
                                            ),
                                        //'progress' => array('message' => '!'),
                                        ),
                                    );

                                    $topicspan = $colspan - 1;
                                    $row['topic_select']['#prefix'] = "<td colspan='$topicspan' class='summary-topic-name'>";
                                    $row['topic_select']['#suffix'] = "</td>";
                                }
                            } else {
                                // simply add a blank cell of the proper width
                                $topicspan = $colspan - 1;
                                $row['empty']['#markup'] = "<td colspan='$topicspan' class='summary-topic-name'>&nbsp;</td>";
                            }
                        }
                    }

                    // if the topic is now or has been set...
                    if ($currTopic) {
                        $button = _build_add_tag_popup($article->getId(),
                            $currTopic);
                        // create topic label
                        $row['topic']['button'] = $button;
                        $row['topic']['name']['#markup'] = "<div>$topic->topic_name</div>";

                        $tags = Ebms\getTagHistory($articleId, $currTopic);
                        $tagNames = array();
                        foreach ($tags as $tag) {
                            $tagNames[$tag->type_name] = $tag->type_name;
                        }

                        if (!empty($tagNames)) {
                            $list = implode(', ', $tagNames);
                            $row['topic']['tags']['#markup'] = "<div>TAGS: $list</div>";
                        }

                        $row['topic']['#prefix'] = "<td class='summary-topic-name'>";
                        $row['topic']['#suffix'] = "</td>";

                        // EDIT: Dan Young
                        // need to only allow passing/rejecting based
                        // on user's assigned boards/topics
                        global $user;

                        // only show options if the current board or topic is
                        // in the user's lists
                        $currCycle = 0;
                        if ($topicAdded) {
                            $cycleSelectHash = "$articleId|$currTopic";
                            $cycleSelectId = "add_cycle_$cycleSelectHash";

                            // check for existing selected cycle
                            if (isset($form_state['values'][$cycleSelectId])) {
                                $currCycle = $form_state['values'][$cycleSelectId];
                            }

                            // check the triggered elements as a backup
                            if (isset($form_state['triggered_elements'][$cycleSelectId]['state']['#value'])) {
                                $currCycle = $form_state['triggered_elements'][$cycleSelectId]['state']['#value'];
                            }
                        }

                        // if the topic is newly added and no cycle is chosen,
                        // then show the select element
                        if ($topicAdded && $currCycle == 0) {
                            // create a select field for the topic
                            $options = array();
                            $options[0] = strtoupper("Choose Review Cycle");
                            $options += Ebms\Util::cycles();

                            $row['cycle_select'][$cycleSelectId] = array(
                                '#type' => 'select',
                                '#title' => '',
                                '#options' => $options,
                                '#return_value' => $cycleSelectId,
                                '#persist' => array(
                                    'type' => 'update',
                                    'group' => $groupId),
                                '#ajax' => array(
                                    'event' => 'change',
                                    'callback' => '_ajax_expand_boards',
                                    'wrapper' => 'citation-results-div',
                                    'progress' => array(
                                        'type' => 'throbber',
                                        'message' => t('')
                                    ),
                                ),
                            );

                            $topicspan = $colspan - 2;
                            $row['cycle_select']['#prefix'] = "<td colspan='$topicspan' class='summary-review_cycle borderless'>";
                            $row['cycle_select']['#suffix'] = "</td>";
                        }
                        // else, proceed with showing or hiding
                        else if ((!isset($topic->curr_state) ||
                            $topic->curr_state == $state->state_id) && (
                            user_access('administer ebms') ||
                            $is_librarian ||
                            in_array($currBoard, $user->boards) ||
                            in_array($currTopic, $user->topics))) {
                            $hash = "$record->article_id|$topic->topic_id";
                            $actions = _get_topic_actions($form_state, $hash);

                            // create reject button
                            $row['reject']['#prefix'] = "<td class='summary-topic-check'>";
                            $row['reject']['#suffix'] = "</td>";

                            $row['reject']["reject_$hash"] = array(
                                '#type' => 'checkbox',
                                '#title' => t('Reject'),
                            );
                            if (isset($actions[$hash]['reject']) &&
                                $actions[$hash]['reject'])
                                $row['reject']["reject_$hash"]['#default_value'] = 1;

                            // create pass button
                            $row['pass']['#prefix'] = "<td class='summary-topic-check'>";
                            $row['pass']['#suffix'] = "</td>";

                            $row['pass']["pass_$hash"] = array(
                                '#type' => 'checkbox',
                                '#title' => t('Pass'),
                            );
                            if (isset($actions[$hash]['pass']) &&
                                $actions[$hash]['pass'])
                                $row['pass']["pass_$hash"]['#default_value'] = 1;

                            if ($fulltext_review) {
                                // create fyi button
                                $row['fyi']['#prefix'] = "<td class='summary-topic-check'>";
                                $row['fyi']['#suffix'] = "</td>";

                                $row['fyi']["fyi_$hash"] = array(
                                    '#type' => 'checkbox',
                                    '#title' => t('FYI'),
                                );
                                if (isset($actions[$hash]['fyi']) &&
                                    $actions[$hash]['fyi'])
                                    $row['fyi']["fyi_$hash"]['#default_value'] = 1;
                            }
                        }
                        else {
                            // simply add a blank cell of the proper width
                            $topicspan = $colspan - 2;
                            $row['empty']['#markup'] =
                                "<td colspan='$topicspan' class='borderless'>&nbsp;</td>";
                        }
                    }

                    // done with this row, set up the next.
                    $topicIx++;
                    $rowIx++;

                    $rows[$rowIx] = array(
                        '#prefix' => "<tr>",
                        '#suffix' => '</tr>',
                    );

                    $row = &$rows[$rowIx];
                }

                $boardIx++;
            }

            // should be an extra, empty row, unset it
            if ($boardName != 'Add New') {
                unset($rows[$rowIx]);
            }

            $results['table']['rows'][$ix] = $rows;
            $ix++;
        }

        $results['trailing-buttons'] = array(
            '#prefix' => '<div id="citation-buttons-div">',
            '#suffix' => '</div>',
        );

        $results['trailing-buttons']['save_button'] = array(
            '#type' => 'submit',
            '#value' => 'Save',
        );

        $results['bottom-pager'] = array(
            '#theme' => 'pager',
        );
    }

    function _build_citation_sort_options(&$form_state, $showAbstract = true,
            $currentSort = null) {
        // retrieve default values
        global $citation_cached_value_keys;
        $cachedKeys = drupal_map_assoc($citation_cached_value_keys);
        foreach($cachedKeys as $cachedKey) {
            $keyname = "default_$cachedKey";
            $$keyname = null;

            if(isset($form_state['cached_values']['values'][$cachedKey]))
                $$keyname = $form_state['cached_values']['values'][$cachedKey];
        }

        $form = array(
            '#prefix' => '<div class="citation-input-div childfix">',
            '#suffix' => '</div>',
        );

        if ($showAbstract) {
            $sortOptions = array(
                null => '',
                'ebms_id' => t("EBMS ID#"),
                'pmid' => t("PMID #"),
                'author' => t("Author"),
                'title' => t("Title"),
                'journal' => t("Journal"),
                'publication_date' => t("Publication Date"),
            );

            $form['sort_by'] = array(
                '#type' => 'select',
                '#title' => t('Sort By'),
                '#options' => $sortOptions,
                '#attributes' => array(
                    'autocomplete' => 'off',
                ),
                '#default_value' => $default_sort_by,
            );

            if ($currentSort) {
                $form['sort_by']['#default_value'] = $currentSort;
            }

            $formatOptions = array(
                'brief' => t('Brief'),
                'abstract' => t('Abstract'),
            );

            $form['format'] = array(
                '#type' => 'radios',
                '#title' => t('Format'),
                '#options' => $formatOptions,
                '#default_value' => $default_format !== null ?
                    $default_format : 'brief',
            );
        }

        $pagerOptions = array(
            10 => 10,
            25 => 25,
            50 => 50,
            'all' => t("View All"),
        );

        $form['per_page'] = array(
            '#type' => 'select',
            '#title' => t('Items Per Page'),
            '#options' => $pagerOptions,
            '#default_value' => $default_per_page,
        );

        $form['apply'] = array(
            '#type' => 'button',
            '#value' => t('Apply'),
        );

        // return the proper form section based on abstract
        if (!$showAbstract) {
            $form['apply']['#ajax'] = array(
                'callback' => '_not_list_results_ajax',
                'wrapper' => 'citation-results-div',
                'progress' => array(
                    'type' => 'throbber',
                    'message' => t('')
                ),
            );
        } else {
            $form['apply']['#ajax'] = array(
                'callback' => '_citation_queue_results_ajax',
                'wrapper' => 'citation-results-div',
                'progress' => array(
                    'type' => 'throbber',
                    'message' => t('')
                ),
            );
        }

        return $form;
    }

    function _not_list_results_ajax($form, &$form_state) {
        return $form['citation-results'];
    }

    function _citation_queue_results_ajax($form, &$form_state) {
        return $form['citation_queue_results'];
    }

    function _add_author_sort(SelectQueryInterface &$query, $dir = 'ASC') {
        $query->leftJoin('ebms_article_author_cite', 'c',
            "a.article_id = c.article_id");
        $query->leftJoin('ebms_article_author', 'auth',
            'c.author_id = auth.author_id AND c.cite_order = 1');
        $query->orderBy('auth.last_name', $dir);
        $query->orderBy('auth.initials', $dir);
        $query->orderBy('auth.collective_name', $dir);
        // fall-back sorting in case no author exists
        $query->orderBy('a.brf_jrnl_title', $dir);
    }

    /**
     * Retrieves the article state from which all articles on the page should
     * originate, as defined in Ebms\EbmsArticleStates.
     *
     * @param array $form_state The current form state
     * @return string A string matching an article state's text id
     */
    function _citation_queue_get_source_state(&$form_state){
        $is_librarian = $form_state['build_info']['args'][0];

        $fulltext_review = isset($form_state['values']['fulltext_review_check'])
                ? $form_state['values']['fulltext_review_check'] : false;

        if (!$is_librarian) {
            if ($fulltext_review)
                return Ebms\EbmsArticleStates::PassedBMReview;
            else
                return \Ebms\EbmsArticleStates::Published;
        }

        return \Ebms\EbmsArticleStates::ReadyInitReview;
    };

    function pdq_ebms_citation_queue_submit($form, &$form_state) {
        $source_state = _citation_queue_get_source_state($form_state);

        _update_topic_actions($form_state);

        $form_state['rebuild'] = true;

        $clear_states = false;

        if (isset($form_state['values']['op'])) {
            $op = $form_state['values']['op'];
            if (!empty($form_state['values']['submit_source'])) {
                $op = $form_state['values']['submit_source'];
            }

            switch ($op) {
                case 'Reset':
                    $form_state['rebuild'] = false;
                    $form_state['redirect'] = 'citations';
                    return;
                case 'Save':
                case 'Submit':
                    // write out the changes to the not list, in terms of
                    // inserts and deletes to the DB
                    _write_topic_states($form_state['altered_topics'],
                        $source_state);

                    $clear_states = true;
                    break;
                case 'Filter':
                    $clear_states = true;
                    break;
                case 'Refresh':

                    break;
            }
        }

        if ($clear_states) {
            $form_state['altered_topics'] = array();
            $form_state['triggered_elements'] = array();
            unset($form_state['header_sort']);
        }

        // wipe out button values so that the default_values calculated from
        // altered topics can take effect
        _clear_topic_actions($form_state['input'], 'input');
    }

    /**
     * Formats an article's abstract into a useful HTML fragment, with classes
     * applied to the labels and text.
     *
     * @param Ebms\EbmsArticle $article
     * @return string $abstract
     */
    function _format_abstract(Ebms\EbmsArticle $article) {
        $pieces = array();
        try {
            $xml = $article->getSourceData();
            $doc = new \SimpleXMLElement($xml);
            $abstractXml = $doc->MedlineCitation->Article->Abstract;

            if ($abstractXml->count() > 0) {
                $children = $abstractXml->children();

                foreach ($children as $element) {
                    $text = htmlspecialchars((string) $element);
                    $label = htmlspecialchars($element['Label']);
                    if ($label)
                        $pieces[] = "<div class='citation-abstract-label'>$label</div>";

                    $pieces[] = "<div class='citation-abstract-text'>$text</div>";
                }
            }
            return implode('', $pieces);
        } catch (Exception $e) {
            watchdog('EBMS - citations', (string) $e);
        }
    }

    /**
     * Construct a render array for the contents of the article citation.  This
     * is only for the record itself, not any states or decisions that pertain
     * to it - which may be added to the form later.
     *
     * @param object $article       Loaded EbmsArticle object for the article.
     * @param boolean $long_form    True=show the fuller form of the article.
     * @param boolean $showAbstract True=include the abstract.
     * @param boolean $showFullText True=include a link to the full article
     *                              text PDF, if we have it in our database.
     *
     * @return Render array for the article.
     */
    function _cite_article(Ebms\EbmsArticle $article, $long_form = true,
        $showAbstract = false, $showFullText = true) {
        $form = array();

        $article_id = $article->getId();

        // Fetch the legacy CMS ID if available.
        $legacy_id = db_select('ebms_legacy_article_id', 'i')
            ->fields('i', array('legacy_id'))
            ->condition('i.article_id', $article_id)
            ->execute()
            ->fetchField();

        $xml = $article->getSourceData();
        $doc = new \SimpleXMLElement($xml);
        $ta = $vol = $issue = $pgn = $year = $pmid = '';
        $title = array();
        $articleXML = $doc->MedlineCitation->Article;
        $rawTitle = (string) $articleXML->ArticleTitle;
        if (!$long_form) {
            // create link-like submit element
            $title = array (
                '#type' => 'submit',
                '#value' => filter_xss($rawTitle),
                '#article_id' => $article_id,
                '#button_type' => 'full-citation-link',
                '#submit' => array('_full_citation_redirect'),
            );
        }
        else
        {
            $title['#markup'] = "<div>$rawTitle</div>";
        }
        $pgn = htmlspecialchars($articleXML->Pagination->MedlinePgn);
        $year = $articleXML->Journal->JournalIssue->PubDate->Year;
        $issue = htmlspecialchars($articleXML->Journal->JournalIssue->Issue);
        $vol = htmlspecialchars($articleXML->Journal->JournalIssue->Volume);
        $authors = array();
        if (isset($doc->MedlineCitation->Article->AuthorList->Author)) {
            foreach ($doc->MedlineCitation->Article->AuthorList->Author
            as $author) {
                if (!$long_form && sizeof($authors) == 10) {
                    $authors[] = 'et al.';
                    break;
                }
                if ($author->CollectiveName)
                    $name = $author->CollectiveName;
                else {
                    $name = $author->LastName;
                    if ($author->Initials)
                        $name .= ' ' . $author->Initials;
                }
                $authors[] = htmlspecialchars($name);
            }
        }
        if (count($authors) < 1)
            $authors = array('[No authors listed]');
        $ta = $doc->MedlineCitation->MedlineJournalInfo->MedlineTA;
        $ta = htmlspecialchars($ta);
        $pmid = $doc->MedlineCitation->PMID;
        if (!$long_form) {

            // Add link to PubMed ID (TIR 2492).
            $target = array('attributes' => array('target' => '_blank'));
            $url = "http://www.ncbi.nlm.nih.gov/pubmed/$pmid";
            $link = l($pmid, $url, $target);
            $pmid = "PMID: $link";
        }
        $ebmsId = $article_id;

        $tAuthors = '';
        $tTitle = '';
        $tJournal = '';
        $tPMID = '';
        $tEBMSID = '';
        $tLEGACYID = '';
        $tReviewCycle = '';

        if ($long_form) {
            $tAuthors = '<div class="citation-subheader">AUTHORS</div>';
            $tTitle = '<div class="citation-subheader">TITLE</div>';
            $tJournal = '<div class="citation-subheader">JOURNAL</div>';
            $tPMID = '<div class="citation-subheader">PMID</div>';
            $tEBMSID = '<div class="citation-subheader">EBMS ID</div>';
            $tLEGACYID = '<div class="citation-subheader">LEGACY CMS ID</div>';
            $tReviewCycle = '<div class="citation-subheader">REVIEW CYCLE</div>';
        } else if (!$showAbstract) {
            $ebmsId = "EBMS ID: $article_id";
            if ($legacy_id)
                $legacy_id = "LEGACY CMS ID: $legacy_id";
        }

        $form['authors']['#markup'] =
            implode('',
            array(
                "<div class='article-citation'>$tAuthors<div>",
                implode(', ', $authors),
                "</div>$tTitle"
            )
        );

        $form['title'] = $title;

        $pieces = array(
            "$tJournal<div>$ta $vol",
        );
        if ($issue)
            $pieces[] = "($issue)";
        if ($pgn)
            $pieces[] = ": $pgn";
        if ($year)
            $pieces[] = ", $year";

        if ($showAbstract) {
            $pieces[] = ".</div>$tPMID<div>$pmid&nbsp;&nbsp;&nbsp;&nbsp;EBMS ID: $ebmsId";
            if ($legacy_id)
                $pieces[] = "&nbsp;&nbsp;&nbsp;&nbsp;Legacy CMS ID: $legacy_id";
            $pieces[] = '</div>';
        }
        else {
            $pieces[] = ".</div>$tPMID<div>$pmid</div>";
            $pieces[] = "$tEBMSID<div>$ebmsId</div>";
            if ($legacy_id)
                $pieces[] = "$tLEGACYID<div>$legacy_id</div>";
        }

        if (!$long_form) {
            if ($showAbstract) {
                $pieces[] = _format_abstract($article);
            }

            $pieces[] = '<div>';
            $abstractLink = null;
            if (!$showAbstract) {
                $link = ctools_modal_text_button('VIEW ABSTRACT',
                    "citations/nojs/view_abstract/$article_id", 'View Abstract',
                    'ctools-modal-tag-style');

                $abstractLink = $link; //'<a href="/ebmsdev/stub">VIEW ABSTRACT</a> | ';
            }

            // build full citation text url
            $fileUrl = null;
            if ($showFullText && $article->getFullTextId()) {
                $file = file_load($article->getFullTextId());
                $fileUrl = file_create_url($file->uri);
            }

            if ($abstractLink) {
                $pieces[] = $abstractLink;
                if ($fileUrl)
                    $pieces[] = ' | ';
            }

            $target = array('attributes' => array('target' => '_blank'));

            if ($fileUrl)
                $pieces[] = l('DOWNLOAD FULL TEXT', $fileUrl, $target);

            $pieces[] = "</div>";
        } else {
            $cycles = Ebms\getArticleReviewCycle($article_id);

            if (sizeof($cycles) > 1)
                $tReviewCycle = '<div class="citation-subheader">REVIEW CYCLES</div>';

            $reviewCycle = implode(', ', $cycles);

            if (!empty($reviewCycle))
                $pieces[] = "$tReviewCycle<div>$reviewCycle</div>";
        }
        $pieces[] = '</div>';

        $form['citation']['#markup'] = implode('', $pieces);

        return $form;
        #$results = db_select('ebms_article', 'a')
        #    ->fields('a', array('article_title', 'source_id',
        #return "Article $position (CMS ID $article_id)";
    }

    function _full_citation_redirect($form, &$form_state){
        // check for the article id
        if(isset($form_state['triggering_element']['#article_id'])) {
            // update the form state's topic actions
            _update_topic_actions($form_state);

            // update any changed values, including pass/reject/fyi
            _citation_queue_handle_cached_values($form_state, true);

            // also check for saved query parameters
            $query = drupal_get_query_parameters();
            $options = array();
            if(isset($query['page'])){
                $options['query']['page'] = $query['page'];
            }
            if(isset($form_state['cached_values']['id'])){
                $options['query']['queue'] =
                    $form_state['cached_values']['id'];
            }

            $articleId = $form_state['triggering_element']['#article_id'];
            drupal_goto("citations/full/$articleId", $options);
        }
    }

    function _get_topic_actions(&$form_state, $hash = null) {

        if (!isset($form_state['altered_topics']))
            return array();

        if (!isset($hash))
            return $form_state['altered_topics'];

        $topic_actions = array();
        foreach ($form_state['altered_topics'] as $key => $value) {
            if ($key != $hash)
                continue;

            $topic_actions[$key] = $value;
        }

        return($topic_actions);
    }

    /**
     * Retrieves the reject or pass decisions on the listed topics from the form state
     * and stores the relevant values for later use.
     * @param array $form_state
     */
    function _update_topic_actions(&$form_state) {
        $altered_topics = array();
        if (isset($form_state['altered_topics']))
            $altered_topics = $form_state['altered_topics'];

        foreach ($form_state['values'] as $key => $value) {
            if (strpos($key, 'reject_') === 0 ||
                strpos($key, 'pass_') === 0 ||
                strpos($key, 'fyi_') === 0) {
                list($action, $hash) = explode('_', $key);

                // whatever the hash, add it and the current action to the altered
                // topics array
                $altered_topics[$hash][$action] = $value;
            }
        }

        // clear out double/triple-0 actions (no need to persist sets of unchecked boxes)
        $set_topics = array();
        foreach ($altered_topics as $hash => $actions) {
            if (!$actions['reject'] && !$actions['pass'] && (!isset($actions['fyi'])
                || !$actions['fyi'])) {
                continue;
            }

            $set_topics[$hash] = $actions;
        }

        // also, dig all added review cycles from the triggered elements,
        // regardless of the existence of the actions
        if (isset($form_state['triggered_elements'])) {
            foreach ($form_state['triggered_elements'] as $key => $props) {
                if (strpos($key, 'add_cycle_') === 0) {
                    list($action, $field, $hash) = explode('_', $key);

                    // whatever the hash, add it and the current action to the altered
                    // topics array
                    $set_topics[$hash][$field] = $props['state']['#value'];
                }
            }
        }

        $form_state['altered_topics'] = $set_topics;
    }

    /**
     *
     * @param type $target
     * @param type $desc
     */
    function _clear_topic_actions(&$target, $desc) {
        $button_keys = array();
        foreach ($target as $key => $value) {
            if (strpos($key, 'reject_') === 0 ||
                strpos($key, 'pass_') === 0) {
                $button_keys[$key] = $value;
            }
        }

        $diff = array_diff_key($target, $button_keys);
        $target = $diff;
    }

    /**
     * Used the given initial state as a starting point, determines the proper new
     * state for each topic, and updates the articles' states.
     * @global type $user
     * @param type $values an array of topic actions, pulled from $form_state
     * @param type $initial_state the original state of the listed article/topic pairs
     */
    function _write_topic_states($values, $initial_state) {
        // first, ensure the topic is valid
        global $citations_state_map;

        $reject_name = $citations_state_map[$initial_state]['reject'];
        $pass_name = $citations_state_map[$initial_state]['pass'];
        $fyi_name = null;
        if (isset($citations_state_map[$initial_state]['fyi'])) {
            $fyi_name = $citations_state_map[$initial_state]['fyi'];
        }

        // look through each article/topic hash and determine if they are
        // rejected or passed
        try {
            foreach ($values as $hash => $actions) {
                // get the article id and topic id from the hash,
                // and the reject and pass states from the actions array
                list($article_id, $topic_id) = explode('|', $hash);
                $reject = isset($actions['reject']) ? $actions['reject'] : 0;
                $pass = isset($actions['pass']) ? $actions['pass'] : 0;
                $fyi = isset($actions['fyi']) ? $actions['fyi'] : 0;
                $cycle = isset($actions['cycle']) ? $actions['cycle'] : null;

                // define the new state, default to reject
                $new_state_name = $reject_name;

                // if not exactly one state set, then save cycle to initial
                // state, if available
                if (($pass + $reject + $fyi != 1)) {
                    if ($cycle) {
                        $row_id = Ebms\setArticleState($article_id,
                            $initial_state, $topic_id, null, null, null, null,
                            $cycle);
                    }
                    continue;
                } else if ($pass) {
                    // else if pass is set, use the pass name
                    $new_state_name = $pass_name;
                } else if ($fyi) {
                    // else if pass is set, use the pass name
                    $new_state_name = $fyi_name;
                }

                // otherwise, reject is already chosen and should be set
                // update the article's state
                $row_id = Ebms\setArticleState($article_id, $new_state_name,
                    $topic_id, null, null, null, null, $cycle);
            }
        } catch (Exception $e) {
            watchdog_exception('citations', $e);
            drupal_set_message('Could not save one or more topic decisions.',
                'error');
        }
    }

    /**
     * Sets the prefix and suffix of a form so as to wrap it in a div with
     * given classes.
     *
     * @param mixed $form The form, passed by reference, to modify.
     *
     * @param mixed $classes An array of class string to be added to the wrapping
     *      div.
     */
    function _wrap_in_classes(&$form, $classes = array()) {
        $classText = implode(' ', $classes);
        $form['#prefix'] = "<div class='$classText'>";
        $form['#suffix'] = "</div>";
    }

    function _build_full_citation_actions($articleId) {
        $form = array();
        _wrap_in_classes($form, array('nav-block'));

        $form['title']['#markup'] = '<h3>' . t('Citation Actions') . '</h3>';

        $article = new Ebms\EbmsArticle($articleId);

        $links = array();

        // check for search query
        $query = drupal_get_query_parameters();
        if (isset($query['search'])){
            $searchId = $query['search'];
            $searchPath = "citations/search/R$searchId";
            $links[] = array('Refine Search', $searchPath, array());
        }

        if (isset($query['queue'])){
            $options = array(
                'query' => array(
                    'queue' => $query['queue'],
                )
            );
            if (isset($query['page']))
                $options['query']['page'] = $query['page'];
            $links[] = array('Return to Queue', 'citations', $options);
        }

        // build full citation text url
        $fileUrl = null;
        if ($article->getFullTextId()) {
            $file = file_load($article->getFullTextId());
            $fileUrl = file_create_url($file->uri);
        }

        // build abstract link
        $target = array('attributes' => array('target' => '_blank'));
        $pubmed = 'http://www.ncbi.nlm.nih.gov/pubmed/';
        $pm_id = $article->getSourceId();
        $pm_link = $pubmed . $pm_id;

        $links[] = array('View Abstract', $pm_link, $target);

        if ($fileUrl)
            $links[] = array('Download Full Text', $fileUrl, $target);

        $items = array();
        foreach ($links as $link) {
            list($label, $url, $options) = $link;

            if ($url)
                $item = l($label, $url, $options);
            else {
                $item = "<a>$label</a>";
            }

            $items[]['#markup'] = "<li>$item</li>";
        }

        $form['links'] = $items;

        $form['links']['#prefix'] = "<div class='actions'><ul>";
        $form['links']['#suffix'] = "</ul></div>";

        $form['footer']['#markup'] = "<div class='footer'></div>";

        return $form;
    }

    function _build_full_citation_icon_key() {
        $form = array();
        _wrap_in_classes($form, array('nav-block'));

        $form['title']['#markup'] = '<h3>' . t('Icon Key') . '</h3>';

        $editIcon = Ebms\EDIT_INACTIVE;
        $tagIcon = Ebms\TAG_INACTIVE;
        $addIcon = Ebms\ADD_DECISION_INACTIVE;
        $commentIcon = Ebms\COMMENT_INACTIVE;

        $lines = array();
        $lines[] = array($editIcon, 'Edit');
        $lines[] = array($tagIcon, 'Tag');
        $lines[] = array($addIcon, 'Add New');
        $lines[] = array(null, 'Most Recent Decision');
        $lines[] = array($commentIcon, 'Add New Comment');

        $markups = array();
        foreach ($lines as $line) {
            list($icon, $text) = $line;

            $markup = '';
            if ($icon)
                $markup .= "<img class='line-icon' alt='' src='$icon'>";
            else
                $text = "<div class='left-nav-most-recent'></div>$text";

            $markup .= $text;

            $element = array();
            $element['#markup'] = "<li>$markup</li>";

            $markups[] = $element;
        }

        $form['text'] = $markups;

        $form['text']['#prefix'] = "<div class='actions'><ul>";
        $form['text']['#suffix'] = "</ul></div>";

        $form['footer']['#markup'] = "<div class='footer'></div>";

        return $form;
    }

    /**
     * Build the form for a full citation display for one article.
     *
     * @param array $form           Standard drupal form render array
     * @param array &$form_state
     * @param int $articleId        Internal EBMS unique ID of one article citation.
     *
     * @return Updated $form render array.
     */
    function pdq_ebms_full_citation($form, &$form_state, $articleId) {
        require_once('EbmsArticle.inc');

        // This global is set and reset here
        global $citations_inc_display_phase;
        $citations_inc_display_phase = DISPLAY_PHASE::NO_PHASE;

        //include js to add radio/checkbox behavior
        $ret = drupal_add_js(EBMS\JS_DIR . '/citations.js', 'file');

        // hook into ajax completions to apply jQuery to form components
        drupal_add_js('jQuery(document).ajaxComplete(function(){
            addHooks();
        });',
            'inline');

        // include modal script necessities
        _include_modal_scripts();

        // maintain triggered element states
        _update_persistent_elements($form_state);

        // Get the full article object data from the database
        $article = new Ebms\EbmsArticle($articleId);

        // Construct a render array for the citation itself (not its history).
        $cite = _cite_article($article);

        $form['#prefix'] = '<div id="citation-form-div">';
        $form['#suffix'] = '</div">';

        $form['title'] = array('#markup' => "<h2>Full Citation</h2>");

        $form['citation'] = array(
            '#prefix' => "<div class='full-citation-element'>",
            'body' => $cite,
            '#suffix' => "</div>",
        );

        $citations_inc_display_phase = DISPLAY_PHASE::REVIEW_CYCLE;
        // build array of currently active citation tags ever assigned, in date order
        $tags = Ebms\getTagHistory($articleId, null, null, true, false, 'D');

        // reverse order so most recent is first
        $tags = array_reverse($tags);

        // build the render array of tags
        $tag_array = _format_tags($form_state, $tags);

        // False if no active tags for article
        $no_tags = (empty($tag_array));

        // generate the add tag button (or placeholder if triggered)
        $button_array = _build_add_tag_button($no_tags, $form_state, $articleId);
        $form['article_tag_add_button'] = $button_array['button'];
        if ($no_tags) {
            if (count($button_array['form']))
                $form['button_form'] = $button_array['form'];
            else
                $form['placeholder'] = array('#markup' => '<div class="full-citation-element">&nbsp;</div>');
        }
        else {
            $form['article_tags'] = $tag_array;
            $form['button_form'] = $button_array['form'];
        }

        // Retrieve an array of Ebms\ArticleStatus objects for every board,
        //  topic, and state, active or not, ever assigned to the article.
        // Order is B(oard), T(opic), S(equence number).
        $history = Ebms\getStatusHistory($articleId, null, null, true, true,
            'BTS');

        // Place the info in a render array
        $form['editorial_boards'] = _format_boards($form_state, $article,
            $history, $tags);

        // Retrieve an array of import activities by Board, Topic, Active
        //  status, import batch ID.
        $importData = Ebms\getArticleImportData($article->getId());

        // And render the import info
        $citations_inc_display_phase = DISPLAY_PHASE::IMPORT_PROCESS;
        $form['import_process'] = array(
            '#type' => 'fieldset',
            '#title' => '<span class="citation-subheader">Import Process</span>',
            'section' => _format_import($article, $importData),
            '#collapsible' => true,
            '#collapsed' => true,
            '#prefix' => "<div class='full-citation-element citation-subheader'>",
            '#suffix' => "</div>",
        );

        // Render the initial_review info, i.e., first review at import time
        $citations_inc_display_phase = DISPLAY_PHASE::MED_LIB_INIT_REVIEW;
        $form['initial_review'] = array(
            '#type' => 'fieldset',
            '#title' => '<span class="citation-subheader">Medical Librarian Initial Review</span>',
            'section' => _format_boards($form_state, $article, $history, $tags,
                                        DISPLAY_PHASE::MED_LIB_INIT_REVIEW),
            '#collapsible' => true,
            '#collapsed' => true,
            '#prefix' => "<div class='full-citation-element citation-subheader'>",
            '#suffix' => "</div>",
        );

        // Render the publishing history section
        $citations_inc_display_phase = DISPLAY_PHASE::PUBLISHING;
        $form['publishing'] = array(
            '#type' => 'fieldset',
            '#title' => '<span class="citation-subheader">Publishing</span>',
            'section' => _format_boards($form_state, $article, $history, $tags,
                                        DISPLAY_PHASE::PUBLISHING),
            '#collapsible' => true,
            '#collapsed' => true,
            '#prefix' => "<div class='full-citation-element citation-subheader'>",
            '#suffix' => "</div>",
        );
        $citations_inc_display_phase = DISPLAY_PHASE::NO_PHASE;

        // need to ensure that the fieldsets leading to any triggered element
        // are expanded
        if (isset($form_state['triggering_element']['#array_parents'])) {
            $parents = $form_state['triggering_element']['#array_parents'];
            $path = &$form;
            foreach ($parents as $parent) {
                // do not create a path that does not already exist
                if (isset($path[$parent])) {
                    $path = &$path[$parent];

                    if (isset($path['#type']) && $path['#type'] == 'fieldset') {
                        $path['#collapsed'] = false;
                    }
                }
                else
                    break;
            }
        }

        return $form;
    }

    function pdq_ebms_full_citation_submit($form, &$form_state) {
        $form_state['rebuild'] = TRUE;
    }

    /**
     * Formats an article or topic tag for rendering.
     * @param EbmsArticleTag $tag
     * @return mixed the formatted render array.
     */
    function _format_tag($tag, &$form_state = null) {
        $form = array();
        $form['#prefix'] = '<div class="full-citation-element">';
        $form['#suffix'] = '</div>';

        $date = _format_date($tag->dt);
        $form['article_tag'] = array('#markup' => "<div class='full-citation-indent'>$tag->type_name - $date - $tag->user_name</div>");

        // if a form state is provided, indent the elements to provide space for
        // the button
        if ($form_state != null) {
            //drop in 'add comment' button
            $form['article_tag_comment_add_button'] =
                _build_add_tag_comment_button($form_state, $tag->article_tag_id);
        }

        if ($tag->comments)
            $form['article_tag_comment'] = _format_comments($tag->comments);

        return $form;
    }

    /**
     * Create a string referencing an icon suitable for inclusion in a form
     * that displays an image icon for adding a comment (via an ajax subform)
     * to an article state.
     *
     * See also _build_decision_icon_tag(), _build_add_state_comment_button().
     *
     * @param array &$form_state    Drupal form state to find if related form
     *                              is triggered.
     * @param int $articleTagId     Used in making a unique ID for the button.
     *
     * @return                      Render array for icon and ajax invoked form.
     */
    function _build_add_tag_comment_button(&$form_state, $articleTagId) {
        $form = array();

        $hash = $articleTagId;
        $buttonId = "add_tag_comment_$hash";

        // check if related form is triggered
        $triggered = _is_triggered_by($form_state, $buttonId);

        if ($triggered) {
            // grab icon for button
            $iconPath = Ebms\IMAGES_DIR . '/EBMS_AddComment_Icon_Active.png';

            // create add tag button
            $form[$buttonId] = array(
                '#markup' =>
                  "<img class='full-citation-button image-button' alt ='' src='$iconPath'>",
            );

            // create add comment form
            $formId = "add_tag_comment_form_$hash";
            $form[$formId] = _build_add_tag_comment_form($articleTagId);
            $form[$formId]['#prefix'] = '<div class="full-citation-form full-citation-indent">';
            $form[$formId]['#suffix'] = '</div>';
        } else {
            // grab icon for button
            $iconPath = Ebms\IMAGES_DIR . '/EBMS_AddComment_Icon_Inactive.png';

            // create add tag button
            $form[$buttonId] = array(
                '#type' => 'image_button',
                '#return_value' => $buttonId,
                '#src' => $iconPath,
                '#ajax' => array(
                    'callback' => '_ajax_full_citation',
                    'wrapper' => 'citation-form-div',
                    'progress' => array(
                        'type' => 'throbber',
                        'message' => t('')
                    ),
                ),
                '#attributes' => array(
                    'class' => array('full-citation-button', 'image-button'),
                    'title' => 'Add Comment',
                    'alt' => 'Add Comment',
                ),
            );
        }



        return $form;
    }

    function _build_add_state_comment_button(&$form_state,
        Ebms\ArticleStatus $status) {
        $form = array();

        $stateId = $status->article_state_id;

        $hash = $stateId;
        $buttonId = "add_state_comment_$hash";

        // check if related form is triggered
        $triggered = _is_triggered_by($form_state, $buttonId);

        if ($triggered) {
            // grab icon for button
            $iconPath = Ebms\IMAGES_DIR . '/EBMS_AddComment_Icon_Active.png';

            // create add tag button
            $form[$buttonId] = array(
                '#markup' => "<img class='full-citation-button image-button' alt='' src='$iconPath'>",
            );

            // create add comment form
            $formId = "add_state_comment_form_$hash";
            $form[$formId] = _build_add_state_comment_form($stateId);
            $form[$formId]['#prefix'] = '<div class="full-citation-form full-citation-indent">';
            $form[$formId]['#suffix'] = '</div>';
        } else {
            // grab icon for button
            $iconPath = Ebms\IMAGES_DIR . '/EBMS_AddComment_Icon_Inactive.png';

            // create add tag button
            $form[$buttonId] = array(
                '#type' => 'image_button',
                '#return_value' => $buttonId,
                '#src' => $iconPath,
                '#ajax' => array(
                    'callback' => '_ajax_full_citation',
                    'wrapper' => 'citation-form-div',
                    'progress' => array(
                        'type' => 'throbber',
                        'message' => t('')
                    ),
                ),
                '#attributes' => array(
                    'class' => array('full-citation-button', 'image-button'),
                    'title' => 'Add Comment',
                    'alt' => 'Add Comment',
                ),
            );
        }



        return $form;
    }

    function _format_date($dt) {
        list($date) = explode(' ', $dt);
        $date = str_replace('-', '/', $date);

        return $date;
    }

    function _format_comments($comments) {
        $form = array();

        $commentTexts = array();
        $firstComment = true;
        foreach ($comments as $comment) {
            if (empty($comment->text))
                continue;

            $date = _format_date($comment->dt);

            // skip the date and user fro the first comment
            if ($firstComment) {
                $firstComment = false;
                $commentTexts[] = "$comment->text";
            } else {
                $commentTexts[] = "$date - $comment->user_name<br />$comment->text";
            }
        }


        $markup = '<div class="citation-comment">';
        if (!empty($commentTexts))
            $markup .= 'COMMENTS: ' . implode('<br /><br />', $commentTexts);
        else
            return $form;

        $markup .= "</div>";
        $form['#markup'] = $markup;
        $form['#prefix'] = '<div class="full-citation-indent">';
        $form['#suffix'] = '</div>';

        return $form;
    }

    /**
     * Formats an array of tags for rendering.
     * @param array ref &$form_state    Drupal form state
     * @param mixed $tags
     * @param int $topicId              Unique ID of the topic of an article.
     *
     * @return mixed the formatted render array.
     */
    function _format_tags(&$form_state, $tags, $topicId = null) {
        $form = array();

        // organize the tags by topic
        $topic_tags = _build_tag_array($tags);

        // determine the desired topic id, 0 representing topicless tags
        $targetTopicId = $topicId;
        if ($topicId == null)
            $targetTopicId = 0;

        // Are we showing buttons?
        $showButtons = _showActionButtons();

        if (isset($topic_tags[$targetTopicId])) {
            //add header
            $form['article_tag_header'] = array('#markup' => '<div class="citation-subheader">Tag(s)</div>');
            $form['article_tag_header']['#prefix'] = '<div class="full-citation-indent">';
            $form['article_tag_header']['#suffix'] = '</div>';

            $tag = array_shift($topic_tags[$targetTopicId]);
            if ($showButtons) {
                $form['latest_article_tag'] = _format_tag($tag, $form_state);
            } else {
                $form['latest_article_tag'] = _format_tag($tag);
            }
        }

        // if there is no latest tag...
        if (empty($form)) {
            // return an empty array
            return $form;
        }

        // include the rest of the tags in a fieldset
        $article_tags = array();
        if (isset($topic_tags[$targetTopicId])) {
            foreach ($topic_tags[$targetTopicId] as $tag) {
                $article_tags[] = _format_tag($tag);
            }
        }

        // add the remaining tags to a collapsible set
        if (!empty($article_tags)) {
            $form['past_article_tags'] = array(
                '#type' => 'fieldset',
                '#title' => t('Past Tag(s)'),
                '#collapsible' => TRUE,
                '#collapsed' => TRUE,
                'tags' => $article_tags,
                '#attributes' => array(
                    'class' => array('full-citation-past-items'),
                ),
            );
        }

        $form['#prefix'] = '<div class="full-citation-element">';
        $form['#suffix'] = '</div>';

        return $form;
    }

    function _build_tag_array($tags) {
        $topic_tags = array();
        foreach ($tags as $tag) {
            $tag_topic_id = $tag->topic_id;
            if (isset($tag_topic_id))
                $topic_tags[$tag_topic_id][] = $tag;
            else
                $topic_tags[0][] = $tag;
        }

        return $topic_tags;
    }

    /**
     * Builds a button and form for adding a tag to a article or topic.
     *
     * @param bool $noTags      Indicates if no tags yet exist.
     * @param type $form_state  The form_state, used to determine if the button
     *      has been clicked.
     * @param type $articleId   The article to tag's id.
     * @param type $topicId     An optional topic to tag.
     * @return array $form      The renderable array, divided into 'button' and
     *      'form' subelements.  These elements can be placed separately in
     *      a larger render array.
     */
    function _build_add_tag_button($noTags, &$form_state, $articleId,
        $topicId = null) {
        $form = array();

        $showButtons = _showActionButtons();

        // create hash for article/topic
        $hash = "$articleId|$topicId";

        $buttonId = "add_tag_$hash";

        // check if related form is triggered
        $triggered = _is_triggered_by($form_state, $buttonId);

        if ($triggered && $showButtons) {
            // grab icon for button
            $iconPath = $noTags ? Ebms\TAG_ACTIVE : Ebms\ADD_DECISION_ACTIVE;
            //Ebms\IMAGES_DIR . '/EBMS_AddDecision_Icon_Active.png';
            // create add tag button
            $form['button'][$buttonId] = array(
                '#markup' => "<img class='full-citation-button image-button' alt='Add Tag' src='$iconPath'>",
            );

            $formId = "add_tag_form_$hash";
            $form['form'][$formId] = _build_add_tag_form($articleId, $topicId);
            $form['form'][$formId]['#prefix'] = '<div class="full-citation-form full-citation-indent">';
            $form['form'][$formId]['#suffix'] = '</div>';
        } else if ($showButtons) {
            // grab icon for button
            $iconPath = $noTags ? Ebms\TAG_INACTIVE : Ebms\ADD_DECISION_INACTIVE;
            //Ebms\IMAGES_DIR . '/EBMS_AddDecision_Icon_Inactive.png';
            // create add tag button
            $form['button'][$buttonId] = array(
                '#type' => 'image_button',
                '#return_value' => $buttonId,
                '#src' => $iconPath,
                '#ajax' => array(
                    'callback' => '_ajax_full_citation',
                    'wrapper' => 'citation-form-div',
                    'progress' => array(
                        'type' => 'throbber',
                        'message' => t('')
                    ),
                ),
                '#attributes' => array(
                    'class' => array('full-citation-button', 'image-button'),
                    'title' => 'Add Tag',
                    'alt' => 'Add Tag',
                ),
            );
            $form['form'] = array();
        }

        return $form;
    }

    /**
     * Checks if an ajax element in the form has been triggered with a particular
     * value.
     *
     * If a button has been triggered (clicked) in the past and the result
     * saved by our code in the form_state, then it is known to be triggered.
     *
     * @param mixed $form_state     The current form state.
     * @param string $element_value The return value of interest.
     *
     * @return string or bool       'cleared' = Element value was triggered but is cleared.
     *                              'persist' = Element value was persisted in form.
     *                              'trigger' = Element was #return_value in last form action.
     *                              false     = form_state['triggered_element'] and
     *                                           form_state['triggering_element']
     *                                           both unset for this element value.
     */
    function _is_triggered_by(&$form_state, $element_value) {
        // check existing triggered elements
        if (isset($form_state['triggered_elements'][$element_value])) {
            return empty($form_state['triggered_elements'][$element_value]) ? 'cleared'
                    : 'persist';
        }

        // otherwise check for recently triggered elements
        if ((isset($form_state['triggering_element']['#return_value']) &&
            $form_state['triggering_element']['#return_value'] == $element_value)
        )
            return 'trigger';

        return false;
    }

    function _build_add_tag_form($article_id, $topic_id = null, $js = false) {
        $form = array();

        $hash = "$article_id|$topic_id";

        $articleOnly = ($topic_id == null);

        $options = Ebms\getTagTypeNames($articleOnly);

        $form["tag_select_$hash"] = array(
            '#type' => 'select',
            '#title' => t('Add Tag'),
            '#options' => $options,
            '#required' => true,
        );

        $form["tag_comment_$hash"] = array(
            '#type' => 'textfield',
            '#title' => t('Comment'),
        );

        $form["tag_save_$hash"] = array(
            '#type' => 'submit',
            '#value' => t('Save'),
            '#submit' => array('_add_tag_save'),
        );

        $form["tag_cancel_$hash"] = array(
            '#type' => 'submit',
            '#value' => 'Cancel',
            '#limit_validation_errors' => array(),
            '#submit' => array('_form_cancel'),
        );

        if ($js) {
            // drop in ajax-specific versions of the submit functions
            $form["tag_save_$hash"]['#submit'] = array('_add_tag_save_js');
            $form["tag_cancel_$hash"]['#submit'] = array('_form_cancel_js');
        }

        return $form;
    }

    function _form_cancel_js($form, &$form_state) {
        // ensure the form is not rebuilt.
        $form_state['rebuild'] = FALSE;

        // issue command to close the window
        ctools_include('modal');
        $commands = array();
        $commands[] = ctools_modal_command_dismiss();

        print ajax_render($commands);
        exit;
    }

    function _form_cancel($form, &$form_state) {
        $form_state['rebuild'] = TRUE;

        // clear out the triggered elements
        $form_state['triggered_elements'] = array();
    }

    function _add_tag_save_js($form, &$form_state) {
        // wrap the normal submit
        _add_tag_save($form, $form_state);

        // issue command to close the window
        ctools_include('modal');
        $commands = array();
        $commands[] = ctools_modal_command_dismiss();
        //$commands[] = ajax_command_invoke('#pdq-ebms-citation-queue #edit-refresh-button',
        //    'click');

        print ajax_render($commands);
        exit;
    }

    function _add_tag_save($form, &$form_state) {
        require_once('EbmsArticle.inc');

        $form_state['rebuild'] = TRUE;

        $values = _retrieve_hash_values($form_state,
            array('tag_select', 'tag_comment'));

        $tagIdStr = $values['tag_select'];
        $comment = $values['tag_comment'];
        $hash = $values['hash'];

        // split the hash into articleId and topicId
        list($articleId, $topicId) = explode('|', $hash);

        // bind topicId to null if undefined
        if (!$topicId)
            $topicId = null;

        Ebms\addArticleTag($tagIdStr, $articleId, $topicId, $comment);
    }

    function _build_add_tag_comment_form($articleTagId) {
        $form = array();

        $hash = "$articleTagId";

        $form["tag_comment_$hash"] = array(
            '#type' => 'textfield',
            '#title' => t('Add Comment'),
            '#required' => true,
        );

        $form["tag_save_$hash"] = array(
            '#type' => 'submit',
            '#value' => t('Save'),
            '#submit' => array('_add_tag_comment_save'),
        );

        $form["tag_cancel_$hash"] = array(
            '#type' => 'submit',
            '#value' => t('Cancel'),
            '#limit_validation_errors' => array(),
            '#submit' => array('_form_cancel'),
        );

        return $form;
    }

    function _build_add_state_comment_form($stateId) {
        $form = array();

        $hash = "$stateId";

        $form["state_comment_$hash"] = array(
            '#type' => 'textfield',
            '#title' => t('Add Comment'),
            '#required' => true,
        );

        $form["state_save_$hash"] = array(
            '#type' => 'submit',
            '#value' => t('Save'),
            '#submit' => array('_add_state_comment_save'),
        );

        $form["state_cancel_$hash"] = array(
            '#type' => 'submit',
            '#value' => t('Cancel'),
            '#limit_validation_errors' => array(),
            '#submit' => array('_form_cancel'),
        );

        return $form;
    }

    function _add_tag_comment_save($form, &$form_state) {
        require_once('EbmsArticle.inc');

        $form_state['rebuild'] = TRUE;

        $values = _retrieve_hash_values($form_state, array('tag_comment'));

        $comment = $values['tag_comment'];
        $articleTagId = $values['hash'];

        Ebms\addTagComment($articleTagId, $comment);
    }

    function _add_state_comment_save($form, &$form_state) {
        require_once('EbmsArticle.inc');

        $form_state['rebuild'] = TRUE;

        $values = _retrieve_hash_values($form_state, array('state_comment'));

        $comment = $values['state_comment'];
        $stateId = $values['hash'];

        Ebms\addArticleStateComment($stateId, $comment);
    }

    function _retrieve_hash_values(&$form_state, $stems, $prefix = '') {
        $values = array();
        $hash = null;
        foreach ($stems as $label => $stem) {
            $localHash = null;
            if (is_array($stem)) {
                $values[$label] = _retrieve_hash_values($form_state, $stem,
                    $label);
                $localHash = $values[$label]['hash'];
                unset($values[$label]['hash']);
            } else {
                $phrase = $stem;
                if ($prefix)
                    $phrase = "${prefix}_${stem}";

                foreach ($form_state['values'] as $key => $value) {
                    // check the current key if it begins with the stem
                    if (strpos($key, $phrase) === 0) {
                        // begins with stem, retrieve hash from end of key
                        $localHash = str_replace("${phrase}_", '', $key);
                        $values[$stem] = $value;

                        break;
                    }
                }
            }

            if (isset($localHash)) {
                if ($hash == null || $hash == $localHash)
                    $hash = $localHash;
                else {
                    drupal_set_message("conflicting hash: $hash vs $localHash",
                        'error');
                }
            }
        }

        $values['hash'] = $hash;

        return $values;
    }

    function _build_add_topic_button(&$form_state, $mode, $articleId,
        $boardId = null) {
        $form = array();

        // create hash for article/board
        $hash = "$articleId|$boardId";

        if ($mode)
            $hash = "${mode}_$hash";

        $buttonId = $boardId ? "add_board_$hash" : "add_topic_$hash";

        // check if related form is triggered
        $triggered = _is_triggered_by($form_state, $buttonId);

        if ($triggered) {
            // grab icon for button
            $iconPath = Ebms\EDIT_ACTIVE;
            // create add tag button
            $form['button'][$buttonId] = array(
                '#markup' => "<img class='full-citation-button image-button' alt='' src='$iconPath'>",
            );

            $formId = "edit_decision_form_$hash";

            // check for the topic id to short-circuit to the add decision form
            $values = _retrieve_hash_values($form_state,
                array('add_board_topic' => array('select_board', 'select_topic')));

            if (isset($values['add_board_topic']['select_topic'])) {
                $topicId = $values['add_board_topic']['select_topic'];

                $decisions = array();
                if ($mode == DISPLAY_PHASE::MED_LIB_INIT_REVIEW) {
                    $decisions = array(Ebms\EbmsTopicDecisions:: $READYINITIAL);
                } else {
                    $decisions = array(Ebms\EbmsTopicDecisions:: $PUBLISHED);
                }

                $form_array = _build_edit_decision_form($form_state, $articleId,
                    $topicId, $decisions, true);
            } else {
                if (!$boardId) {
                    // attempt to dig selected board id form form state
                    if (isset($values['add_board_topic']['select_board'])) {
                        $boardId = $values['add_board_topic']['select_board'];
                    }
                }

                $form_array = _build_add_topic_form($hash, $buttonId,
                    $articleId, $boardId);
            }

            $title = '';
            if (isset($topicId)) {
                $topicName = Ebms\getTopicNameById($topicId);
                $title = "<div class='citation-subheader'>SUMMARY TOPIC</div><div class='full-citation-element'>$topicName</div>";
            }

            $form['form'][$formId] = $form_array;
            $form['form'][$formId]['#prefix'] = "<div class='full-citation-form full-citation-indent'>$title";
            $form['form'][$formId]['#suffix'] = '</div>';
        } else {
            // grab icon for button
            $iconPath = Ebms\EDIT_INACTIVE;

            // create edit decision button
            $form['button'][$buttonId] = array(
                '#type' => 'image_button',
                '#return_value' => $buttonId,
                '#src' => $iconPath,
                '#ajax' => array(
                    'callback' => '_ajax_full_citation',
                    'wrapper' => 'citation-form-div',
                    'progress' => array(
                        'type' => 'throbber',
                        'message' => t('')
                    ),
                ),
                '#attributes' => array(
                    'class' => array('full-citation-button', 'image-button'),
                    'title' => $boardId ? 'Add Topic' : 'Edit Topic',
                    'alt' => $boardId ? 'Add Topic' : 'Edit Topic',
                ),
            );
            $form['form'] = array();
        }

        return $form;
    }

    function _build_add_tag_popup($articleId, $topicId = null) {
        $form = array();

        // grab icon for button
        $iconPath = Ebms\TAG_INACTIVE;

        $dest = "citations/nojs/add_tag/$articleId/$topicId";
        $alt = 'Add Tag';
        $button = ctools_ajax_text_button(theme('image',
                array('path' => $iconPath, 'alt' => $alt)), $dest, $alt, '',
            'ctools-use-modal ctools-modal-tag-style');

        // create edit decision button
        $form['#markup'] = $button;

        return $form;
    }

    function _build_add_topic_form($hash, $return_value, $articleId,
        $boardId = null) {
        $form = array();

        // retrieve the article history in order to determine valid boards and topics
        $history = Ebms\getStatusHistory($articleId, $boardId, null, false,
            true, 'BTS');

        $boards = array();
        $topics = array();
        foreach ($history as $status) {
            $boards[$status->board_id] = $status->board_id;
            $topics[$status->topic_id] = $status->topic_id;
        }

        if (!$boardId) {
            // build a list of board options
            $boardOptions = array();
            $rows = db_select('ebms_board', 'b')->fields('b')->execute()->fetchAll();
            foreach ($rows as $row) {
                if (isset($boards[$row->board_id]))
                    continue;

                $boardOptions[$row->board_id] = $row->board_name;
            }

            if (empty($boardOptions))
                return;

            $options = $boardOptions;
            $title = 'Select a Board';
            $action = 'select_board';
        }
        else {
            // build a list of topic options
            $topicOptions = array();
            $rows = db_select('ebms_topic', 't')
                ->condition('t.board_id', $boardId)->fields('t')
                ->condition('t.active_status', 'A')
                ->orderBy('t.topic_name')
                ->execute()
                ->fetchAll();
            foreach ($rows as $row) {
                if (isset($topics[$row->topic_id]))
                    continue;

                $topicOptions[$row->topic_id] = $row->topic_name;
            }

            if (empty($topicOptions))
                return;

            $options = $topicOptions;
            $title = 'Select a Topic';
            $action = 'select_topic';
        }

        $form["add_board_topic_${action}_$hash"] = array(
            '#type' => 'select',
            '#title' => $title,
            '#options' => $options,
        );

        $form["add_board_topic_continue_$hash"] = array(
            '#type' => 'submit',
            '#value' => t('Continue'),
            '#submit' => array('_add_board_topic_continue'),
            '#return_value' => $return_value,
        );

        $form["add_board_topic_cancel_$hash"] = array(
            '#type' => 'submit',
            '#value' => 'Cancel',
            '#limit_validation_errors' => array(),
            '#submit' => array('_form_cancel'),
        );

        return $form;
    }

    function _add_board_topic_continue($form, &$form_state) {

        $form_state['rebuild'] = TRUE;
    }

    /**
     * Interface to _build_add_decision_button that converts last parameter
     * to an array of length 1.
     */
    function _build_edit_decision_button(&$form_state, $articleId, $topicId,
        Ebms\EbmsTopicDecisions $decision) {
        return _build_add_decision_button($form_state, $articleId, $topicId,
                array($decision));
    }

    function _build_add_decision_button(&$form_state, $articleId, $topicId,
        $decisions, $isAdd = false) {
        $form = array();

        // create hash for article/topic
        $hash = "$articleId|$topicId";

        // append any given decision indexes
        foreach ($decisions as $decision) {
            $index = $decision->index();
            $hash .= "|$index";
        }

        // Are we in a place where we add buttons?
        $showButtons = _showActionButtons();

        $buttonId = $isAdd ? "add_decision_$hash" : "edit_decision_$hash";
        $altText = $isAdd ? "Add Decisions" : "Edit Decision";

        // check if related form is triggered
        $triggered = _is_triggered_by($form_state, $buttonId);

        if ($showButtons) {
            if ($triggered) {
                // Prepare action button
                $iconPath = Ebms\ADD_DECISION_ACTIVE;
                $form['button'][$buttonId] = array('#markup' =>
                    "<img class='full-citation-button image-button' alt='$altText' src='$iconPath'>",
                );

                $formId = "edit_decision_form_$hash";
                $form['form'][$formId] = _build_edit_decision_form($form_state,
                    $articleId, $topicId, $decisions, $isAdd);
                $form['form'][$formId]['#prefix'] = '<div class="full-citation-form full-citation-indent">';
                $form['form'][$formId]['#suffix'] = '</div>';
            } else {
                // Prepare action button and ajax function, if we need it
                // grab icon for button
                $iconPath = Ebms\ADD_DECISION_INACTIVE;

                // create edit decision button
                $form['button'][$buttonId] = array(
                    '#type' => 'image_button',
                    '#return_value' => $buttonId,
                    '#src' => $iconPath,
                    '#ajax' => array(
                        'callback' => '_ajax_full_citation',
                        'wrapper' => 'citation-form-div',
                        'progress' => array(
                            'type' => 'throbber',
                            'message' => t('')
                        ),
                    ),
                    '#attributes' => array(
                        'class' => array('full-citation-button', 'image-button'),
                        'title' => $altText,
                        'alt' => $altText,
                    ),
                    '#persist' => array(
                        'type' => 'toggle',
                        'group' => $buttonId),
                );
                $form['form'] = array();
            }
        }
        else{
            // No buttons
            $form['button'] = '';
            $form['form'] = array();
        }

        return $form;
    }

    function _build_edit_decision_form(&$form_state, $article_id, $topic_id,
        $decisions, $isAdd) {
        $form = array();

        if (empty($decisions))
            return $form;

        $hash = "$article_id|$topic_id";

        $ts = time();
        $date = format_date($ts, 'custom', 'm/d/Y');

        global $user;

        $timetag = "$date - <i>$user->name</i>";

        foreach ($decisions as $decision) {
            $title = $decision->title();
            if (!$isAdd)
                $title = 'Edit Decision';
            $full_title = array(
                '#markup' => "<div class='citation-subheader'>$title</div>" .
                "<div class='full-citation-element'>$timetag</div>"
            );

            switch ($decision) {
                case Ebms\EbmsTopicDecisions::$INITIALREVIEW:
                    $checks = array();
                    $checks["initial_review_pass_$hash"] = array(
                        '#type' => 'checkbox',
                        '#title' => t('Approve'),
                    );

                    $checks["initial_review_reject_$hash"] = array(
                        '#type' => 'checkbox',
                        '#title' => t('Reject'),
                    );

                    $form["initial_review_checks_$hash"] = array(
                        'checks' => $checks,
                        '#prefix' => '<div class="full-citation-radio-check">',
                        '#suffix' => '</div>',
                    );

                    $form["initial_review_comment_$hash"] = array(
                        '#type' => 'textarea',
                        '#title' => t('Comment'),
                        '#prefix' => '<div class="full-citation-element">',
                        '#suffix' => '</div>',
                    );

                    break;
                case \Ebms\EbmsTopicDecisions::$READYINITIAL:
                case \Ebms\EbmsTopicDecisions::$PUBLISHED:
                    // present a list of available review cycles to the user
                    $options = Ebms\Util::cycles();

                    $form["published_select_$hash"] = array(
                        '#type' => 'select',
                        '#title' => 'Review Cycle',
                        '#options' => $options,
                    );

                    $form["published_comment_$hash"] = array(
                        '#type' => 'textarea',
                        '#title' => t('Comment'),
                        '#prefix' => '<div class="full-citation-element">',
                        '#suffix' => '</div>',
                    );

                    $states = $decision->pass_states();
                    $state = $states[0];

                    $form["published_state_$hash"] = array(
                        '#type' => 'hidden',
                        '#value' => $state,
                    );

                    break;
                case Ebms\EbmsTopicDecisions::$NCIREVIEWER:
                    $form['reviewer_title'] = $full_title;

                    $checks = array();
                    $checks["nci_reviewer_pass_$hash"] = array(
                        '#type' => 'checkbox',
                        '#title' => t('Approve'),
                    );

                    $checks["nci_reviewer_reject_$hash"] = array(
                        '#type' => 'checkbox',
                        '#title' => t('Reject'),
                    );

                    $form["nci_reviewer_checks_$hash"] = array(
                        'checks' => $checks,
                        '#prefix' => '<div class="full-citation-radio-check">',
                        '#suffix' => '</div>',
                    );

                    $form["nci_reviewer_comment_$hash"] = array(
                        '#type' => 'textarea',
                        '#title' => t('Comment'),
                        '#prefix' => '<div class="full-citation-element">',
                        '#suffix' => '</div>',
                    );

                    break;
                case Ebms\EbmsTopicDecisions::$FULLTEXT:
                    $form['fulltext_title'] = $full_title;

                    $form["fulltext_file_$hash"] = array(
                        '#title' => t('Full Text File'),
                        '#title_display' => 'invisible',
                        '#type' => 'file',
                    );

                    $form["fulltext_check_$hash"] = array(
                        '#type' => 'checkbox',
                        '#title' => t('Unable to Upload'),
                    );

                    $form["fulltext_comment_$hash"] = array(
                        '#type' => 'textarea',
                        '#title' => t('Comment'),
                        '#prefix' => '<div class="full-citation-element">',
                        '#suffix' => '</div>',
                    );

                    break;
                case Ebms\EbmsTopicDecisions::$SURVEILLANCE:
                    $form['surveillance_title'] = $full_title;

                    $checks = array();
                    $checks["surveillance_pass_$hash"] = array(
                        '#type' => 'checkbox',
                        '#title' => t('Approve'),
                    );

                    $checks["surveillance_reject_$hash"] = array(
                        '#type' => 'checkbox',
                        '#title' => t('Reject'),
                    );

                    $checks["surveillance_fyi_$hash"] = array(
                        '#type' => 'checkbox',
                        '#title' => t('Flag for FYI'),
                    );

                    $form["surveillance_checks_$hash"] = array(
                        'checks' => $checks,
                        '#prefix' => '<div class="full-citation-radio-check">',
                        '#suffix' => '</div>',
                    );


                    $form["surveillance_comment_$hash"] = array(
                        '#type' => 'textarea',
                        '#title' => t('Comment'),
                        '#prefix' => '<div class="full-citation-element">',
                        '#suffix' => '</div>',
                    );

                    break;
                case Ebms\EbmsTopicDecisions::$BOARDMEMBERRESPONSES:

                    require_once('review.inc');

                    $packetIds = Ebms\getArticlePacketIds($article_id,
                                 $topic_id, true);

                    if (empty($packetIds))
                        break;

                    // generate packet information
                    $packetOptions = array(0 => t('Available Packets:'));
                    foreach ($packetIds as $packetId) {
                        $packetOptions[$packetId] = Ebms\getPacketInfo($packetId)->packet_title;
                    }

                    // retrieve the available board members, if any
                    $reviewerOptions = array(0 => 'Please select a packet.');

                    $packetId = 0;
                    if (isset($form_state['values']
                            ["member_responses_packet_$hash"])) {
                        $packetId = $form_state['values']
                            ["member_responses_packet_$hash"];

                        if ($packetId) {
                            $reviewerIds = \EbmsPacket::fetch_reviewers($packetId);

                            $reviewers = user_load_multiple($reviewerIds);
                            foreach ($reviewers as $reviewer) {
                                $reviewerOptions[$reviewer->uid] = $reviewer->name;
                            }

                            if (sizeof($reviewerOptions) > 1) {
                                $reviewerOptions[0] = 'Available Members:';
                            } else {
                                $reviewerOptions[0] = 'No members available.';
                            }
                        }
                    }

                    $reviewerId = 0;
                    if (isset($form_state['values']
                            ["member_responses_reviewer_$hash"])) {
                        $reviewerId = $form_state['values']
                            ["member_responses_reviewer_$hash"];
                    }

                    $values = Ebms\getReviewDispositions();

                    foreach ($values as $value) {
                        $options[$value->value_id] = $value->value_name;
                    }

                    $form['member_responses_title'] = $full_title;

                    $form["member_responses_packet_$hash"] = array(
                        '#type' => 'select',
                        '#title' => 'Literature Surveillance Packet',
                        '#title_display' => 'invisible',
                        '#options' => $packetOptions,
                        '#ajax' => array(
                            'callback' => '_ajax_full_citation',
                            'wrapper' => 'citation-form-div',
                        ),
                    );

                    $form["member_responses_reviewer_$hash"] = array(
                        '#type' => 'select',
                        '#title' => 'Board Member',
                        '#title_display' => 'invisible',
                        '#options' => $reviewerOptions,
                        '#disabled' => !$packetId,
                        '#ajax' => array(
                            'callback' => '_ajax_full_citation',
                            'wrapper' => 'citation-form-div',
                        ),
                    );

                    $form["member_responses_select_$hash"] = array(
                        '#type' => 'checkboxes',
                        '#title' => t('Board Member Responses'),
                        '#options' => $options,
                        '#ajax' => array(
                            'callback' => '_ajax_full_citation',
                            'wrapper' => 'citation-form-div',
                        ),
                        '#disabled' => !$reviewerId,
                    );

                    // need to clean up the select input and value arrays
                    // to prevent selection of reject and other values at the
                    // same time

                    if(isset($form_state['triggering_element']['#name'])){
                        $name = $form_state['triggering_element']['#name'];
                        $element = "member_responses_select_$hash";

                        // check if the name is of the select element with an
                        // offset to indicate which checkbox was clicked
                        $stem = preg_quote($element);
                        $pattern = "#$stem\\[(?'option'[\\d])\\]#";
                        $matches = array();

                        $ret = preg_match($pattern, $name, $matches);

                        // if an option is found
                        if(isset($matches['option'])){
                            $option = $matches['option'];

                            // ...and it is on...
                            if($form_state['values'][$element][$option]){
                                // loop though values and inputs to turn off
                                // the unrelated options
                                foreach($form_state['values'][$element]
                                        as $index => $value){
                                    // if the chosen option is 1 and the
                                    // current index is not (or vice versa)
                                    if(($option == 1) != ($index == 1)) {
                                        // clear the value and input
                                        $form_state['values'][$element][$index] = 0;
                                        unset($form_state['input'][$element][$index]);
                                    }
                                }
                            }
                        }
                    }

                    $showRejects = false;
                    $showLOE = false;
                    // parse the chosen reasons
                    if (isset(
                            $form_state['values']["member_responses_select_$hash"])) {
                        foreach ($form_state['values']["member_responses_select_$hash"]
                        as $key => $value) {
                            if ($value) {
                                if ($key === 1)
                                    $showRejects = true;
                                else {
                                    $showLOE = true;
                                }
                            }
                        }
                    }

                    if ($showRejects) {
                        $rejectValues = db_select('ebms_review_rejection_value',
                                'v')
                            ->fields('v')
                            ->orderBy('v.value_pos')
                            ->execute()
                            ->fetchAllKeyed();

                        $form["member_responses_reject_fieldset_$hash"] = array(
                            '#type' => 'fieldset',
                            '#title' => "<span class='citation-subheader'>
                            Reason For Rejection</span>",
                            '#collapsible' => TRUE,
                            '#collapsed' => FALSE,
                            "member_responses_reject_$hash" => array(
                                '#type' => 'checkboxes',
                                '#options' => $rejectValues,
                            ),
                        );
                    }

                    if ($showLOE) {
                        $form["member_responses_loe_$hash"] = array(
                            '#type' => 'textarea',
                            '#title' => t('Level of Evidence'),
                            '#prefix' => '<div class="full-citation-element">',
                            '#suffix' => '</div>',
                        );
                    }

                    $form["member_responses_comment_$hash"] = array(
                        '#type' => 'textarea',
                        '#title' => t('Comment'),
                        '#prefix' => '<div class="full-citation-element">',
                        '#suffix' => '</div>',
                    );

                    break;

                case Ebms\EbmsTopicDecisions::$BOARDMANAGERACTIONS:

                    // get the full text for the board manager decisions
                    $passStates = $decision->pass_states();

                    $options = array(0 => '');
                    foreach ($passStates as $state) {
                        $type = Ebms\getStateTypebyTextId($state);
                        $options[$type->state_text_id] = $type->state_name;
                    }

                    $form['manager_actions_title'] = $full_title;

                    $form["manager_actions_select_$hash"] = array(
                        '#type' => 'select',
                        '#title' => t('Action'),
                        '#title_display' => 'invisible',
                        '#options' => $options,
                    );

                    $form["manager_actions_comment_$hash"] = array(
                        '#type' => 'textarea',
                        '#title' => t('Comment'),
                        '#prefix' => '<div class="full-citation-element">',
                        '#suffix' => '</div>',
                    );

                    break;

                case Ebms\EbmsTopicDecisions::$ONAGENDA:
                    $form['on_agenda_title'] = $full_title;

                    $query = new EntityFieldQuery();
                    $query
                        ->entityCondition('entity_type', 'node')
                        ->entityCondition('bundle', 'ebms_event')
                        ->propertyCondition('status', '1')
                        ->fieldCondition('field_datespan', 'value', time(), '>')
                        ->fieldOrderBy('field_datespan', 'value')
                        ->entityOrderBy('entity_id');

                    $result = $query->execute();

                    $futureEvents = array(0 => '');
                    if (isset($result['node'])) {
                        foreach ($result['node'] as $node) {
                            $fullNode = node_load($node->nid);
                            $nodeWrapper = entity_metadata_wrapper('node', $fullNode);
                            $datespan = $nodeWrapper->field_datespan->value();
                            $suffix = '';
                            if($datespan) {
                                $date = format_date($datespan['value'], 'custom', 'm/d/Y');
                                $suffix = " - $date";
                            }
                            $futureEvents[$node->nid] = $fullNode->title . $suffix;
                        }
                    }

                    $query = new EntityFieldQuery();
                    $query
                        ->entityCondition('entity_type', 'node')
                        ->entityCondition('bundle', 'ebms_event')
                        ->propertyCondition('status', '1')
                        ->fieldCondition('field_datespan', 'value2', time(), '<')
                        ->fieldOrderBy('field_datespan', 'value', 'DESC')
                        ->entityOrderBy('entity_id', 'DESC');

                    $result = $query->execute();

                    $pastEvents = array(0 => '');
                    if (isset($result['node'])) {
                        foreach ($result['node'] as $node) {
                            $fullNode = node_load($node->nid);$nodeWrapper = entity_metadata_wrapper('node', $fullNode);
                            $datespan = $nodeWrapper->field_datespan->value();
                            $suffix = '';
                            if($datespan) {
                                $date = format_date($datespan['value'], 'custom', 'm/d/Y');
                                $suffix = " - $date";
                            }
                            $pastEvents[$node->nid] = $fullNode->title . $suffix;
                        }
                    }

                    $form["on_agenda_future_$hash"] = array(
                        '#type' => 'select',
                        '#title' => 'Future Meetings',
                        '#options' => $futureEvents,
                    );

                    $form["on_agenda_past_$hash"] = array(
                        '#type' => 'select',
                        '#title' => 'Past Meetings',
                        '#options' => $pastEvents,
                    );

                    $form["on_agenda_comment_$hash"] = array(
                        '#type' => 'textarea',
                        '#title' => t('Comment'),
                        '#prefix' => '<div class="full-citation-element">',
                        '#suffix' => '</div>',
                    );

                    break;

                case Ebms\EbmsTopicDecisions::$EDITORIALBOARD:
                    $form['board_decision_title'] = $full_title;

                    $values = Ebms\getBoardDecisionValues();

                    $options = array(0 => '');
                    foreach ($values as $value) {
                        $options[$value->value_id] = $value->value_name;
                    }

                    $form["board_decision_select_$hash"] = array(
                        '#type' => 'select',
                        '#title' => 'Decision',
                        '#title_display' => 'invisible',
                        '#options' => $options,
                    );

                    $form["board_decision_discussed_$hash"] = array(
                        '#type' => 'radios',
                        '#options' => array('Y' => 'Yes', 'N' => 'No'),
                        '#title' => 'Discussed',
                    );

                    $boardId = Ebms\getTopicBoard($topic_id);

                    $memberIds = db_select('ebms_board_member', 'm')
                        ->condition('m.board_id', $boardId)
                        ->fields('m', array('user_id'))
                        ->execute()
                        ->fetchCol();

                    $members = user_load_multiple($memberIds);
                    $memberOpts = array();
                    foreach ($members as $member) {
                        $memberOpts[$member->uid] = $member->name;
                    }

                    $form["board_decision_members_fieldset_$hash"] = array(
                        '#type' => 'fieldset',
                        '#title' => 'Board Members',
                        '#collapsible' => 'true',
                        '#collapsed' => 'true',
                        "board_decision_members_$hash" => array(
                            '#type' => 'checkboxes',
                            '#options' => $memberOpts,
                        )
                    );

                    $form["board_decision_comment_$hash"] = array(
                        '#type' => 'textarea',
                        '#title' => t('Comment'),
                        '#prefix' => '<div class="full-citation-element">',
                        '#suffix' => '</div>',
                    );

                    break;
                default:
                    $form[] = array('#markup' => "<div>$title</div>");
            }
        }

        $form["edit_decision_save_$hash"] = array(
            '#type' => 'submit',
            '#value' => t('Save'),
            '#validate' => array('_edit_decision_validate'),
            '#submit' => array('_edit_decision_save'),
        );

        $form["edit_decision_cancel_$hash"] = array(
            '#type' => 'submit',
            '#value' => 'Cancel',
            '#limit_validation_errors' => array(),
            '#submit' => array('_form_cancel'),
        );

        return $form;
    }

    function _edit_decision_validate($form, &$form_state) {
        require_once('EbmsArticle.inc');

        //include js to add radio/checkbox behavior
        $ret = drupal_add_js(EBMS\JS_DIR . '/citations.js', 'file');

        // hook into ajax completions to apply jQuery to form components
        drupal_add_js(
            'jQuery(document).ajaxComplete(function(){
                addHooks();
            });',
            'inline');

        global $decision_value_map;
        $values = _retrieve_hash_values($form_state, $decision_value_map);

        $hash = $values['hash'];

        // split the hash into articleId and topicId
        list($articleId, $topicId) = explode('|', $hash);

        // if reject reasons are present in the form, make sure one is chosen
        if (isset($values['member_responses']['reject'])) {
            $reasonChosen = false;
            foreach ($values['member_responses']['reject'] as $value) {
                if ($value) {
                    $reasonChosen = true;
                    break;
                }
            }

            if (!$reasonChosen) {
                form_set_error("member_responses_reject_$hash",
                    "At least one rejection reason must be chosen.");
            }
        }

        // if the LOE field is present in the form, make sure it has content
        /*if (isset($values['member_responses']['loe'])) {
            $loe = $values['member_responses']['loe'];
            if (empty($loe)) {
                form_set_error("member_responses_loe_$hash",
                    "The Level of Evidence must be provided.");
            }
        }*/

        // reject if both a future and past meeting are chosen
        if (isset($values['on_agenda']['future']) &&
            isset($values['on_agenda']['past']) &&
            $values['on_agenda']['future'] &&
            $values['on_agenda']['past']) {

            form_set_error("on_agenda_past_$hash",
                "Only one meeting may be chosen.");
        }
    }

    function _edit_decision_save($form, &$form_state) {
        require_once('EbmsArticle.inc');

        $form_state['rebuild'] = TRUE;

        // clear out the triggered elements
        $form_state['triggered_elements'] = array();

        global $decision_value_map;
        $values = _retrieve_hash_values($form_state, $decision_value_map);

        $hash = $values['hash'];

        // split the hash into articleId and topicId
        list($articleId, $topicId) = explode('|', $hash);

        _save_initial_review_decision($values['initial_review'], $articleId,
            $topicId);
        _save_published_decision($values['published'], $articleId, $topicId);
        _save_nci_reviewer_decision($values['nci_reviewer'], $articleId,
            $topicId);
        _save_fulltext_decision($values['fulltext'], $articleId, $topicId);
        _save_surveillance_decision($values['surveillance'], $articleId,
            $topicId);
        _save_member_responses_decision($values['member_responses'], $articleId,
            $topicId);
        _save_manager_actions_decision($values['manager_actions'], $articleId,
            $topicId);
        _save_on_agenda_decision($values['on_agenda'], $articleId, $topicId);
        _save_board_decision($values['board_decision'], $articleId, $topicId);
    }

    function _save_initial_review_decision($values, $articleId, $topicId) {
        if (empty($values))
            return;

        $pass = $values['pass'];
        $reject = $values['reject'];
        $comment = $values['comment'];

        if (empty($comment))
            $comment = null;

        if (!($pass xor $reject)) {
            return;
        }

        $state = \Ebms\EbmsArticleStates::RejectInitReview;
        if ($pass)
            $state = \Ebms\EbmsArticleStates::PassedInitReview;

        try {
            Ebms\setArticleState($articleId, $state, $topicId, null, $comment);

            drupal_set_message("Added Initial Review decision to article $articleId");
        } catch (Exception $e) {
            watchdog_exception('citations', $e);
            drupal_set_message('Could not save the Medical Librarian Initial Review decision.',
                'error');
        }
    }

    function _save_published_decision($values, $articleId, $topicId) {
        if (empty($values))
            return;

        $select = $values['select'];
        $comment = $values['comment'];
        $state = $values['state'];

        if (empty($comment))
            $comment = null;

        if (!$select) {
            return;
        }

        $topic = Ebms\getTopicNameById($topicId);

        try {
            Ebms\setArticleState($articleId, $state, $topicId, null, $comment,
                null, null, $select);

            drupal_set_message("Added topic $topic to article $articleId");
        } catch (Exception $e) {
            watchdog_exception('citations', $e);
            drupal_set_message("Could not add topic $topic to the article.",
                'error');
        }
    }

    function _save_nci_reviewer_decision($values, $articleId, $topicId) {
        if (empty($values))
            return;

        $pass = $values['pass'];
        $reject = $values['reject'];
        $comment = $values['comment'];

        if (empty($comment))
            $comment = null;

        if (!($pass xor $reject)) {
            return;
        }

        $state = \Ebms\EbmsArticleStates::RejectBMReview;
        if ($pass)
            $state = \Ebms\EbmsArticleStates::PassedBMReview;

        try {
            Ebms\setArticleState($articleId, $state, $topicId, null, $comment);
            drupal_set_message("Added NCI Reviewer decision to article $articleId");
        } catch (Exception $e) {
            watchdog_exception('citations', $e);
            drupal_set_message('Could not save the NCI Initial Reviewer decision.',
                'error');
        }
    }

    function _save_fulltext_decision($values, $articleId, $topicId) {
        if (empty($values))
            return;

        // prefer 'unavailable' check
        $check = $values['check'];
        $comment = $values['comment'];

        if (empty($comment))
            $comment = null;

        if ($check) {
            global $user;
            $now = strftime("%Y-%m-%d %H:%M:%S");
            try {
                db_insert('ebms_ft_unavailable')
                    ->fields(
                        array(
                            'article_id' => $articleId,
                            'flagged' => $now,
                            'flagged_by' => $user->uid,
                            'comment' => $comment,
                        )
                    )
                    ->execute();
                drupal_set_message('Marked article ' . $articleId .
                    ' as unavailable');
            } catch (Exception $e) {
                // Shouldn't happen if this code is written correctly.
                drupal_set_message('Unavailable flag already set for article '
                    . $articleId, 'error');
            }

            return;
        }

        $validators = array('file_validate_extensions' => array('pdf'));
        foreach ($_FILES['files']['name'] as $key => $value) {
            if ($value) {
                $path = $_FILES['files']['tmp_name'][$key];
                $mime_type = Ebms\Util::get_mime_type($path);
                if ($mime_type != 'application/pdf') {
                    drupal_set_message(htmlspecialchars($value) .
                        ' is not a PDF file.', 'error');
                    continue;
                }
                $file = file_save_upload($key, $validators);
                if ($file) {
                    $file = file_move($file, 'public://');
                    $file->status |= FILE_STATUS_PERMANENT;
                    $file = file_save($file);
                    db_update('ebms_article')
                        ->fields(array('full_text_id' => $file->fid))
                        ->condition('article_id', $articleId)
                        ->execute();
                    drupal_set_message('Posted document ' . $file->filename .
                        ' for article ' . $articleId . '.');
                }
                else
                    drupal_set_message('Unable to upload ' . $value, 'error');
            }
        }

        return;
    }

    function _save_surveillance_decision($values, $articleId, $topicId) {
        if (empty($values))
            return;

        $pass = $values['pass'];
        $reject = $values['reject'];
        $fyi = $values['fyi'];

        $comment = $values['comment'];

        if (empty($comment))
            $comment = null;

        // if less or more than one state is set...
        if (($pass + $reject + $fyi) != 1) {
            return;
        }

        $state = \Ebms\EbmsArticleStates::PassedFullReview;
        if ($reject)
            $state = \Ebms\EbmsArticleStates::RejectFullReview;
        else if ($fyi)
            $state = \Ebms\EbmsArticleStates::FYI;

        try {
            Ebms\setArticleState($articleId, $state, $topicId, null, $comment);
            drupal_set_message("Added Literature Surveillance Committee decision to article $articleId");
        } catch (Exception $e) {
            watchdog_exception('citations', $e);
            drupal_set_message('Could not save the Literature Surveillance Committee decision.',
                'error');
        }
    }

    function _save_member_responses_decision($values, $articleId, $topicId) {
        if (empty($values))
            return;

        $packetId = $values['packet'];
        $memberId = $values['reviewer'];
        $select = $values['select'];

        if (!$packetId || !$memberId || !$select)
            return;

        $reject = array();
        if (isset($values['reject']))
            $reject = $values['reject'];

        if (isset($values['loe']))
            $loe = $values['loe'];
        if (empty($loe))
            $loe = null;

        // retrieve current user and member names to populate comment annotation
        global $user;
        $account = user_load($memberId);

        if ($account === FALSE) {
            drupal_set_message(
                "Could not load user $memberId - response not saved.", 'error');
            return;
        }

        $userName = $user->name;
        $memberName = $account->name;
        $commentSuffix =
            "<i>Recorded by $userName on behalf of $memberName.</i>";

        $comment = $values['comment'];
        if (!$comment)
            $comment = $commentSuffix;
        else
            $comment .= "<br /><br />$commentSuffix";

        $now = strftime("%Y-%m-%d %H:%M:%S");

        // Here are the fields we'll store
        //   article_id and dates handled separately for new
        //   and replace records
        $key = array(
            'packet_id' => $packetId,
            'article_id' => $articleId,
            'reviewer_id' => $memberId,
        );
        $fields = array(
            'when_posted' => $now,
            'comments' => $comment,
            'loe_info' => $loe,
        );

        // Wrap this in a transaction
        $txn = db_transaction();
        try {
            // Insert a new record
            $query = db_merge('ebms_article_review');
            $query->key($key);
            $query->fields($fields);

            $statusId = $query->execute();

            // retrieve the review id
            $reviewId = db_select('ebms_article_review', 'r')
                ->condition('r.packet_id', $packetId)
                ->condition('r.article_id', $articleId)
                ->condition('r.reviewer_id', $memberId)
                ->fields('r', array('review_id'))
                ->execute()
                ->fetchField();

            // clear the review disposition table
            db_delete('ebms_review_disposition')
                ->condition('review_id', $reviewId)
                ->execute();

            // insert choices into the review disposition table
            foreach ($select as $choice => $value) {
                if ($value) {
                    $reviewFields = array(
                        'review_id' => $reviewId,
                        'value_id' => $choice,
                    );

                    // Insert a new record
                    $query = db_merge('ebms_review_disposition');
                    $query->key($reviewFields);

                    $query->execute();
                }
            }

            // clear all current rejections for the review
            db_delete('ebms_review_rejection_reason')
                ->condition('review_id', $reviewId)
                ->execute();

            // save the current rejection reasons if present
            if (!empty($reject)) {
                foreach ($reject as $reason) {
                    if ($reason) {
                        $rejectFields = array(
                            'review_id' => $reviewId,
                            'value_id' => $reason,
                        );

                        // Insert a new record
                        $query = db_insert('ebms_review_rejection_reason');
                        $query->fields($rejectFields);
                        $query->execute();
                    }
                }
            }

            drupal_set_message("Saved Board Member Response for article $articleId");
        } catch (\Exception $e) {
            $txn->rollback();
            throw $e;
        }
    }

    function _save_manager_actions_decision($values, $articleId, $topicId) {
        if (empty($values))
            return;

        $select = $values['select'];
        if (!$select)
            return;

        $comment = $values['comment'];

        if (empty($comment))
            $comment = null;

        try {
            Ebms\setArticleState($articleId, $select, $topicId, null, $comment);
            drupal_set_message("Added Board Manager Action decision to article $articleId");
        } catch (Exception $e) {
            watchdog_exception('citations', $e);
            drupal_set_message('Could not save the Board Manager Action.',
                'error');
        }
    }

    function _save_on_agenda_decision($values, $articleId, $topicId) {
        if (empty($values))
            return;

        $future = $values['future'];
        $past = $values['past'];
        $comment = $values['comment'];

        $eventId = $future ? $future : $past;

        if (empty($comment))
            $comment = null;

        if (!$eventId)
            return;

        // create the article state and add the event relation
        try {
            $stateId = Ebms\setArticleState($articleId,
                Ebms\EbmsArticleStates::OnAgenda, $topicId, null, $comment);

            $result = db_merge('ebms_agenda_meeting')
                ->key(array('nid' => $eventId, 'article_state_id' => $stateId))
                ->execute();

            drupal_set_message("Added On Agenda decision to article $articleId");
        } catch (Exception $e) {
            watchdog_exception('citations', $e);
            drupal_set_message('Could not save the On Agenda information.',
                'error');
        }
    }

    function _save_board_decision($values, $articleId, $topicId) {
        if (empty($values) || !$values['select'])
            return;

        $value_id = $values['select'];
        $discussed = $values['discussed'];
        $memberIds = $values['members'];
        $comment = $values['comment'];
        if (!$comment)
            $comment = null;

        try {
            $articleStateId = Ebms\setArticleState($articleId,
                Ebms\EbmsArticleStates::FinalBoardDecision, $topicId, null,
                $comment);

            // fetch review cycle, should only be one for an article/topic
            // combo
            $cycles = Ebms\getArticleReviewCycle($articleId, $topicId);

            $key = array(
                'article_state_id' => $articleStateId,
                'decision_value_id' => $value_id,
            );
            $fields = array(
                'discussed' => $discussed,
            );

            if(!empty($cycles)){
                $cycle_ids = array_keys($cycles);
                $fields['meeting_date'] = reset($cycle_ids);
            }

            // insert should work, as the article state is freshly created
            $ret = db_merge('ebms_article_board_decision')
                ->key($key)
                ->fields($fields)
                ->execute();

            // proceed to save the decision members if the merge worked
            if ($ret) {
                foreach ($memberIds as $id) {
                    if ($id) {
                        $key = array(
                            'article_state_id' => $articleStateId,
                            'uid' => $id,
                        );

                        // merge in article state / uid pair
                        $ret = db_merge('ebms_article_board_decision_member')
                            ->key($key)
                            ->execute();
                    }
                }
            }
        } catch (Exception $e) {
            watchdog_exception('citations', $e);
            drupal_set_message('Could not save the Editorial Board decision.',
                'error');
        }
    }

    /**
     * Formats an array of boards to their topic state histories for rendering.
     * @param mixed $history
     * @return mixed the formatted render array.
     */
    function _format_boards(&$form_state, Ebms\EbmsArticle $article, $history,
        $tags, $mode = null) {
        $form = array();
        $showButtons = true;
        // if in a specific mode, hide buttons for the managers
        if ($mode) {
            global $user;
            if (!in_array('medical librarian', $user->roles)) {
                $showButtons = false;
            }
        }

        // divide into arrays by board and topic
        $board_topic_states = array();
        foreach ($history as $status) {
            if ($status->board_name && $status->topic_name) {
                $board_topic_states[$status->board_name][$status->topic_name][$status->type_sequence][$status->active_status][]
                    = $status;
            }
        }

        // determine if multiple review cycles exist
        $display_cycle = false;
        if (!$mode) {
            $cycles = Ebms\getArticleReviewCycle($article->getId());
            $display_cycle = (sizeof($cycles) > 1);
        }

        $button_array = _build_add_topic_button($form_state, $mode,
            $article->getId());
        if ($showButtons && $mode != DISPLAY_PHASE::PUBLISHING)
            $form['boards_header']['button'] = $button_array['button'];
        $form['boards_header']['title'] = array("#markup" => '<div class="citation-subheader full-citation-indent full-citation-element">Editorial Board(s)</div>');
        if ($showButtons && $mode != DISPLAY_PHASE::PUBLISHING)
            $form['boards_header']['form'] = $button_array['form'];

        foreach ($board_topic_states as $board_name => $topics) {
            $boardId = Ebms\getBoardIdByName($board_name);

            // create add topic button
            $button_array = _build_add_topic_button($form_state, $mode,
                $article->getId(), $boardId);
            if ($showButtons && $mode != DISPLAY_PHASE::PUBLISHING)
                $form[$board_name]['button'] = $button_array['button'];
            $form[$board_name]['fieldset'] = array(
                '#type' => 'fieldset',
                '#title' => $board_name,
                '#collapsible' => TRUE,
                '#collapsed' => TRUE,
                'topics' => _format_topics($form_state, $article, $topics,
                    $tags, $display_cycle, $mode),
                '#prefix' => '<div class="full-citation-indent">',
                '#suffix' => '</div>',
            );
            if (!$mode) {
                $form[$board_name]['fieldset']['#attributes']['class'][] = 'full-citation-topic';
            }
            if ($showButtons && $mode != DISPLAY_PHASE::PUBLISHING)
                $form[$board_name]['form'] = $button_array['form'];
        }

        return $form;
    }

    /**
     * Construct a render array of all of the import actions for this article.
     *
     * @param object &$article      Loaded EbmsArticle object for the article.
     * @param array &$importData    Array of arrays, one for each import action
     *                              for this article.
     *
     *
     * @return array $form          Render array for the import activity info,
     *                              by board, topic, action.
     */
    function _format_import(Ebms\EbmsArticle &$article, &$importData) {
        $form = array();

        $form['#prefix'] = '<div class="full-citation-element full-citation-indent">';
        $form['#suffix'] = '</div>';

        $articleId = $article->getId();
        $fullTextId = $article->getFullTextId();

        if ($fullTextId) {
            $file = file_load($fullTextId);

            $form['import_file_title'] = array("#markup" => '<div class="citation-subheader">Source File Name</div>');
            $form['import_file_name'] = array("#markup" => "<div class='full-citation-element'>$file->filename</div>");
        }

        $dt = $article->getImportDate();
        $date = _format_date($dt);

        $uid = $article->getImportedBy();
        $account = user_load($uid);
        $name = $account->name;

        $form['importer_title'] = array("#markup" => '<div class="citation-subheader">Imported By</div>');
        $form['importer_name'] = array("#markup" => "<div class='full-citation-element'>$name - $date</div>");

        foreach ($importData as $boardId => $topics) {
            $boardName = Ebms\getBoardNameById($boardId);

            // wrap board-specific import data in fieldset
            $form[$boardName]['fieldset'] = array(
                '#type' => 'fieldset',
                '#title' => $boardName,
                '#collapsible' => TRUE,
                '#collapsed' => TRUE,
                'underscore' => array('#markup' => '<hr>'),
                'topics' => _format_import_topics($article, $topics),
                '#prefix' => '<div class="full-citation-element">',
                '#suffix' => '</div>',
            );
        }

        return $form;
    }

    function _format_import_topics($article, $topics) {
        $form = array();

        foreach ($topics as $topicId => $topic) {
            if (!empty($form)) {
                // drop in a dividing rule
                $form[] = array('#markup' => '<hr>');
            }

            $topicName = Ebms\getTopicNameById($topicId);
            $form[$topicName] = _format_import_topic($article, $topicName,
                $topic);
        }

        $form['#prefix'] = '<div class="full-citation-indent">';
        $form['#suffix'] = '</div>';

        return $form;
    }

    /**
     * Formats an array of topics and their related state histories for rendering.
     * @param mixed $topics
     * @return mixed the formatted render array.
     */
    function _format_topics(&$form_state, $article, $topics, $tags,
        $display_cycle, $mode) {
        $form = array();

        foreach ($topics as $topic_name => $topic_sequences) {
            if (!empty($form)) {
                // drop in a dividing rule
                $form[] = array('#markup' => '<hr>');
            }

            $form[$topic_name] = _format_topic($form_state, $article,
                $topic_name, $topic_sequences, $tags, $display_cycle, $mode);
        }

        return $form;
    }

    /**
     * Formats a topic name and set of topic states for rendering.
     * @param string $topic
     * @param mixed $topic_states
     * @return mixed the formatted render array.
     */
    function _format_import_topic($article, $topicName, $importData) {
        $form = array();

        // retrieve the relevant ids for this article and topic
        // topic id should have error-checking, but originates from an error-checked
        // method
        $articleId = $article->getId();
        $topicId = Ebms\getTopicIdByName($topicName);

        $form['header'] = array(
            "#markup" => "<div class='citation-subheader'>Summary Topic</div>"
        );
        $form['topic'] = array(
            "#markup" => "<div class='full-citation-element'>$topicName</div>");

        // find each of the dispositions, along with the related date
        if (isset($importData['A'])) {
            foreach ($importData['A'] as $batchId => $actions) {
                $dt = '';
                $comment = null;
                foreach ($actions as $action) {
                    $dt = $action->import_date;
                    $disposition = array();
                    $disposition['#markup'] = "<div class='citation-subheader full-citation-element'>$action->disposition_name</div>";
                    if ($action->comment)
                        $comment = $action->comment;

                    $form['actions'][$batchId][] = $disposition;
                }

                $form['actions'][$batchId]['tag']['#markup'] = "<div class='citation-subheader'>TAG(S)</div>";
                $date = _format_date($dt);
                $form['actions'][$batchId]['time']['#markup'] = "<div>Batch - $date</div>";
                $form['actions'][$batchId]['comment']['#markup'] = "<div>COMMENT: $action->comment</div>";

                $form['actions'][$batchId]['#prefix'] = "<div class='full-citation-element'>";
                $form['actions'][$batchId]['#suffix'] = "</div>";
            }
        }

        return $form;
    }

    /**
     * Formats a topic a single name and associated topic states for rendering.
     * Subroutine of _format_topics.
     *
     * @param array &$form_state        Current drupal form state.
     * @param object $article           Loaded EbmsArticle object for which we generate info.
     * @param string $topicName         Human readable topic name.
     * @param mixed $topic_sequences
     * @return mixed the formatted render array.
     */
    function _format_topic(&$form_state, $article, $topicName, $topic_sequences,
        $tags, $display_cycle, $mode) {
        $form = array();

        $showButtons = true;
        // if in a specific mode, hide buttons for the managers
        if ($mode) {
            global $user;
            if (in_array('board manager', $user->roles)) {
                $showButtons = false;
            }
        }

        // retrieve the relevant ids for this article and topic
        // topic id should have error-checking, but originates from an error-checked
        // method
        $articleId = $article->getId();
        $topicId = Ebms\getTopicIdByName($topicName);

        // generate the add tag button and render array of tags
        $tag_array = _format_tags($form_state, $tags, $topicId);
        $no_tags = empty($tag_array);
        $button_array = _build_add_tag_button($no_tags, $form_state, $articleId,
            $topicId);

        // split the button and potential form into separate areas based on the existence of
        // tags
        $form['topic_header'] = _format_topic_header($topicName,
            $no_tags && $showButtons ? $button_array['button'] : array());
        if ($display_cycle) {
            $form['topic_cycle'] = _format_topic_cycle($articleId, $topicId);
        }

        if ($mode == DISPLAY_PHASE::PUBLISHING) {
            $decision = _format_topic_publish($articleId, $topicId,
                $topic_sequences, $form_state);
            if (empty($decision)) {
                /* $decision = _format_topic_decision($decisionInfo,
                  $topic_sequences, $form_state, DISPLAY_PHASE::PUBLISHING); */

                $info = _format_topic_publishing_info($articleId, $topicId,
                    $topic_sequences);
                $decision = $info;
            }
            $form['topic_publishing_decision'] = $decision;
        }

        if (!$no_tags && $showButtons)
            $form['article_tag_add_button'] = $button_array['button'];
        $form['topic_tags'] = $tag_array;
        if ($showButtons)
            $form['button_form'] = $button_array['form'];

        // build decision arrays and missing decisions
        $empty_decisions = array();


        $decisions = array();
        if ($mode == DISPLAY_PHASE::MED_LIB_INIT_REVIEW) {
            $decisions = array();
            $decisionInfo = Ebms\EbmsTopicDecisions::$INITIALREVIEW;
            // if ($showButtons) {
            if (true) {
                $decision = _format_topic_decision($decisionInfo,
                    $topic_sequences, $form_state, null, $showButtons);
                $decisions['topic_initial_review_decision'] = _wrap_decision($form_state,
                    $articleId, $topicId, $decision, $decisionInfo,
                    $empty_decisions);
            } else {
                $decisions['topic_initial_review_decision'] =
                    _format_topic_decision($decisionInfo, $topic_sequences);
            }
        } else if ($mode == DISPLAY_PHASE::PUBLISHING) {

        } else {
            $decisions = array();
            $decisionInfo = Ebms\EbmsTopicDecisions::$NCIREVIEWER;
            $decision = _format_topic_decision($decisionInfo, $topic_sequences,
                $form_state);
            $decisions['topic_nci_reviewer_decision'] = _wrap_decision($form_state,
                $articleId, $topicId, $decision, $decisionInfo, $empty_decisions);

            $decisionInfo = Ebms\EbmsTopicDecisions::$FULLTEXT;
            $decision = _format_topic_full_text_decision($article);
            $decisions['topic_full_text_decision'] = _wrap_decision($form_state,
                $articleId, $topicId, $decision, $decisionInfo, $empty_decisions);

            $decisionInfo = Ebms\EbmsTopicDecisions::$SURVEILLANCE;
            $decision = _format_topic_decision($decisionInfo, $topic_sequences,
                $form_state);
            $decisions['topic_nci_surveillance_decision'] = _wrap_decision($form_state,
                $articleId, $topicId, $decision, $decisionInfo, $empty_decisions);

            $decision = _format_topic_assigned_for_review_decision($articleId,
                $topicId);
            $decisions['topic_assigned_for_review_decision'] =
                _wrap_decision_simple($decision);

            $decisionInfo = Ebms\EbmsTopicDecisions::$BOARDMEMBERRESPONSES;
            $decision = _format_topic_board_member_responses($articleId,
                $topicId);
            $decisions['topic_board_member_responses'] = _wrap_decision($form_state,
                $articleId, $topicId, $decision, $decisionInfo, $empty_decisions);

            $decisionInfo = Ebms\EbmsTopicDecisions::$BOARDMANAGERACTIONS;
            $decision = _format_topic_decision($decisionInfo, $topic_sequences,
                $form_state);
            $decisions['topic_board_manager_actions'] = _wrap_decision($form_state,
                $articleId, $topicId, $decision, $decisionInfo, $empty_decisions);

            $decisionInfo = Ebms\EbmsTopicDecisions::$ONAGENDA;
            $decision = _format_topic_decision($decisionInfo, $topic_sequences,
                $form_state);
            $decisions['topic_on_agenda'] = _wrap_decision($form_state,
                $articleId, $topicId, $decision, $decisionInfo, $empty_decisions);

            $decisionInfo = Ebms\EbmsTopicDecisions::$EDITORIALBOARD;
            $decision = _format_topic_decision($decisionInfo, $topic_sequences,
                $form_state);
            $decisions['topic_board_decision'] = _wrap_decision($form_state,
                $articleId, $topicId, $decision, $decisionInfo, $empty_decisions);

            $last_key = null;
            $last_array = null;
            foreach ($decisions as $key => $array) {
                if (isset($array['decision']['current_decisions'])) {
                    $last_key = $key;
                    $last_array = $array;
                }
            }

            if ($last_array != null && $last_key != null) {
                $decisions[$last_key] = array(
                    'decision' => $last_array,
                    '#prefix' => '<div class="full-citation-latest-decision">',
                    '#suffix' => '</div>',
                );
            }
        }

        $form['decisions'] = $decisions;

        if ($showButtons && !empty($empty_decisions)) {
            $dec_button_arr = _build_add_decision_button($form_state,
                $articleId, $topicId, $empty_decisions, true);
            $form['add_decisions'] = $dec_button_arr['button'];
            $form['add_decisions_label'] = array('#markup' => '<div class="full-citation-element full-citation-indent">ADD NEW DECISION</div>');
            $form['add_decisions_form'] = $dec_button_arr['form'];
        }

        return $form;
    }

    /**
     * Wraps a decision with edit buttons and determines if the decision
     * is empty.
     *
     * @param array $form_state
     * @param int $articleId
     * @param int $topicId
     * @param array $decision
     * @param Ebms\EbmsTopicDecisions $decisionInfo
     * @param array $emptyDecisions
     * @return array
     */
    function _wrap_decision(&$form_state, $articleId, $topicId, &$decision,
        Ebms\EbmsTopicDecisions $decisionInfo, &$emptyDecisions) {
        $form = array();
        if (empty($decision)) {
            $emptyDecisions[] = $decisionInfo;
            return null;
        }
        $buttonArray = _build_edit_decision_button($form_state, $articleId,
            $topicId, $decisionInfo);

        // Sometimes there is no button, value should be ''
        $form['button'] = $buttonArray['button'];
        $form['decision'] = $decision;
        $form['form'] = $buttonArray['form'];

        return $form;
    }

    function _wrap_decision_simple(&$decision) {
        $form = array();

        $form['decision'] = $decision;

        return $form;
    }

    function _format_topic_header($topic, $button) {
        $form = array();

        $form['#prefix'] = '<div class="full-citation-element">';
        $form['#suffix'] = '</div>';

        $form['header'] = array(
            "#markup" => "<div class='citation-subheader full-citation-indent'>Summary Topic</div>",
            '#weight' => 0,
        );

        $form['button'] = $button;

        $form['topic'] = array(
            "#markup" => "<div class='full-citation-indent'>$topic</div>",
            '#weight' => 2);

        return $form;
    }

    function _format_topic_cycle($articleId, $topicId) {
        $form = array();

        $form['#prefix'] = '<div class="full-citation-element full-citation-indent">';
        $form['#suffix'] = '</div>';

        $form['header'] = array("#markup" => "<div class='citation-subheader'>Review Cycle</div>");

        $cycles = Ebms\getArticleReviewCycle($articleId, $topicId);

        if (empty($cycles))
            return array();

        if (sizeof($cycles) > 1)
            $form['header'] = array("#markup" => "<div class='citation-subheader'>Review Cycles</div>");

        $reviewCycle = implode(', ', $cycles);

        $form['cycle'] = array("#markup" => "<div>$reviewCycle</div>");

        return $form;
    }
    /*
      function _format_topic_nci_reviewer_decision($topic_sequences, $show_past = false) {
      $form = array();

      $index = $show_past ? 'I' : 'A';
      if (!isset($topic_sequences[4][$index]))
      return $form;

      if (!$show_past) {
      $form['#prefix'] = '<div class="full-citation-element full-citation-indent">';
      $form['#suffix'] = '</div>';
      }

      $statii = $topic_sequences[4][$index];

      if (!$show_past)
      $form['header'] = array("#markup" => "<div class='citation-subheader'>NCI Reviewer Decision</div>");

      $decisions = array();
      foreach ($statii as $status) {
      $decision = array();
      $date = _format_date($status->dt);
      $decision['decision'] = array("#markup" => "<div>$status->type_name - $date - $status->user_name</div>");
      $decision['comments'] = _format_comments($status->comments);
      $decision['#prefix'] = '<div class="full-citation-element">';
      $decision['#suffix'] = '</div>';

      $decisions[] = $decision;
      }

      $form['decisions'] = $decisions;

      // also dig up the past decisions if the currently displayed set is current
      if (!$show_past && isset($topic_sequences[4]['I'])) {
      // add the remaining decisions to a collapsible set
      $form['past_decisions'] = array(
      '#type' => 'fieldset',
      '#title' => t('Past Decisions'),
      '#collapsible' => TRUE,
      '#collapsed' => TRUE,
      'tags' => _format_topic_nci_reviewer_decision($topic_sequences, true),
      '#attributes' => array(
      'class' => array('full-citation-past-items'),
      ),
      '#prefix' => '<div class="full-citation-element full-citation-indent">',
      '#suffix' => '</div>'
      );
      }
      return $form;
      } */

    /**
     * Generate a render array for a decision pertaining to an article state.
     * Called for each article state for a citation during a full display.
     * Constructs form components for the title of the state, the name of the
     * user, date, and buttons, depending on the content to be formatted, any
     * custom formatter invoked (see class EbmsTopicDecisions->formatter), and
     * other factors.
     *
     * @param object $decisionInfo     EbmsTopicDecisions object general
     *                                  state info.
     * @param array $topic_sequences   Array of state info for one topic
     *                                  for one article.
     * @param array &$form_state       Standard Drupal form state info.
     * @param string $modifier         DISPLAY_PHASE constant, tells about
     *                                  the purpose of the formatted data.
     *
     * @return array $form             Render array to render this data in a form.
     */
    function _format_topic_decision(Ebms\EbmsTopicDecisions $decisionInfo,
        $topic_sequences, &$form_state = null, $modifier = null, $showButtons = true) {
        $form = array();

        // generating past content if no form_state (and)
        $show_past = ($form_state == NULL);

        $sequenceIndex = $decisionInfo->sequence();
        $title = $decisionInfo->title();

        if (!isset($topic_sequences[$sequenceIndex]))
            return $form;

        $activeIndex = $show_past ? 'I' : 'A';
        if (isset($topic_sequences[$sequenceIndex][$activeIndex])) {
            $statii = $topic_sequences[$sequenceIndex][$activeIndex];

            if (!$show_past && $modifier != DISPLAY_PHASE::PUBLISHING) {
                $form['header'] = array("#markup" => "<div class='citation-subheader full-citation-indent'>$title</div>");
            }

            $decisions = array();
            foreach ($statii as $status) {
                $decision = array();

                if (function_exists($decisionInfo->formatter())) {
                    $formatter = $decisionInfo->formatter();
                    $decision = $formatter($status, $form_state);
                } else {
                    $date = _format_date($status->dt);

                    $name = $status->type_name;
                    $text_id = $status->type_text_id;

                    $prefix = "$name - ";

                    $state = $decisionInfo->find_state($text_id);
                    if ($state != NULL) {
                        $active = ($state != 'fyi');
                        $prefix = _build_decision_icon_tag($state, $active);
                    }

                    $decision['decision'] = array("#markup" => "$prefix$date - $status->user_name");
                    $decision['decision']['#prefix'] = '<div class="full-citation-indent">';
                    $decision['decision']['#suffix'] = '</div>';

                    if ($form_state != null &&
                        $modifier != DISPLAY_PHASE::PUBLISHING &&
                        $showButtons) {

                        //drop in 'add comment' button
                        $decision['comments_add_button'] =
                            _build_add_state_comment_button($form_state, $status);
                    }

                    $decision['comments'] = _format_comments($status->comments);

                    if (empty($decision['comments']) && !empty($decision['comments_add_button'])) {
                        // set the weight of the add comment button to move it level with the decision
                        $decision['comments_add_button']['#weight'] = -1;
                    }
                }

                $decision['#prefix'] = '<div class="full-citation-element">';
                $decision['#suffix'] = '</div>';
                $decisions[] = $decision;
            }

            $form['current_decisions'] = $decisions;
        }

        // also dig up the past decisions if the currently displayed set is current
        if (!$show_past && $modifier != DISPLAY_PHASE::PUBLISHING
                        && isset($topic_sequences[$sequenceIndex]['I'])) {
            // ensure the header exists
            $form['header'] = array("#markup" => "<div class='citation-subheader full-citation-indent'>$title</div>");

            // add the remaining decisions to a collapsible set
            $form['past_decisions'] = array(
                '#type' => 'fieldset',
                '#title' => t('Past Decisions'),
                '#collapsible' => TRUE,
                '#collapsed' => TRUE,
                'past_decisions_list' => _format_topic_decision($decisionInfo,
                    $topic_sequences),
                '#attributes' => array(
                    'class' => array('full-citation-past-items'),
                ),
            );
        }

        if (!$show_past && !empty($form)) {
            $form['#prefix'] = '<div class="full-citation-element">';
            $form['#suffix'] = '</div>';
        }

        return $form;
    }

    function _format_topic_full_text_decision(Ebms\EbmsArticle $article) {
        $form = array();

        $fullTextId = $article->getFullTextId();

        $form['#prefix'] = '<div class="full-citation-element">';
        $form['#suffix'] = '</div>';

        // add the header
        $form['header'] = array("#markup" => "<div class='citation-subheader full-citation-indent'>Full Text Retrieved</div>");


        // if the id is null...
        if (!$fullTextId) {
            // check the full text unavailable table for this article id
            $row = db_select('ebms_ft_unavailable', 'u')
                ->condition('u.article_id', $article->getId())
                ->fields('u')
                ->execute()
                ->fetch();

            // if no row is found, then there's nothing to display for this decision
            if ($row === false)
                return array();

            $dt = $row->flagged;
            $date = _format_date($dt);
            $uid = $row->flagged_by;
            $comment = $row->comment;

            $account = user_load($uid);
            $icon = _build_decision_icon_tag('reject');

            $form['current_decisions']['decision'] = array("#markup" => "<div class='full-citation-indent'>$icon$date - $account->name</div>");
            $form['current_decisions']['comment'] = array('#markup' => "<div class='full-citation-indent'>COMMENTS: $comment</div>");
            return $form;
        }

        // get the related database row
        $records = db_select('file_managed', 'f')
            ->condition('f.fid', $fullTextId)
            ->fields('f')
            ->execute()
            ->fetchAll();

        if (!isset($records[0]))
            return array();

        $record = $records[0];
        $uid = $record->uid;
        $filename = $record->filename;
        $ts = $record->timestamp;
        $date = format_date($ts, 'custom', 'm/d/Y');

        $account = user_load($uid);
        $icon = _build_decision_icon_tag('pass');

        $form['current_decisions']['decision'] = array("#markup" => "<div class='full-citation-indent'>$icon$date - $account->name</div>");
        $form['current_decisions']['filename'] = array("#markup" => "<div class='full-citation-indent'>FILE NAME: $filename</div>");

        return $form;
    }

    function _format_topic_assigned_for_review_decision($articleId, $topicId) {
        $form = array();

        // grab the current and past packet ids
        $packetIds = Ebms\getArticlePacketIds($articleId, $topicId);
        $pastIds = Ebms\getArticlePacketIds($articleId, $topicId, false, false,
            true);

        // if no packets of either kind, done
        if (empty($packetIds) && empty($pastIds))
            return $form;

        // enclose the element
        $form['#prefix'] = '<div class="full-citation-element">';
        $form['#suffix'] = '</div>';

        // add the header
        $form['header'] = array('#markup' => "<div class='citation-subheader'>Assigned For Review</div>");
        $form['header']['#prefix'] = '<div class="full-citation-indent">';
        $form['header']['#suffix'] = '</div>';

        if (!empty($packetIds)) {
            $form['current_decisions']['blocks'] = _format_packet_ids($packetIds);
        }

        if (!empty($pastIds)) {
            $form['past_blocks'] = array(
                '#type' => 'fieldset',
                '#title' => t('Past Decisions'),
                '#collapsible' => TRUE,
                '#collapsed' => TRUE,
                'tags' => _format_packet_ids($pastIds),
                '#attributes' => array(
                    'class' => array('full-citation-past-items'),
                ),
            );
        }

        return $form;
    }

    function _format_packet_ids($packetIds) {
        $form = array();

        $packetInfos = array();
        foreach ($packetIds as $packetId) {
            $packet = Ebms\getPacketInfo($packetId);
            if ($packet != null)
                $packetInfos[] = $packet;
        }

        if (empty($packetInfos))
            return $form;

        // build each packet block
        $packetBlocks = array();
        foreach ($packetInfos as $packetInfo) {
            $packetBlocks[] = _format_packet_info($packetInfo);
        }

        $form = $packetBlocks;

        $form['#prefix'] = '<div class="full-citation-indent">';
        $form['#suffix'] = '</div>';

        return $form;
    }

    function _format_packet_info($packetInfo) {

        $form = array();

        // format contributor row
        $uid = $packetInfo->created_by;
        $title = $packetInfo->packet_title;
        $dt = $packetInfo->created_at;
        $date = _format_date($dt);

        $account = user_load($uid);

        // load the reviewers
        $reviewers = user_load_multiple($packetInfo->reviewers);
        $names = array();
        foreach ($reviewers as $reviewer) {
            $names[] = $reviewer->name;
        }

        $nameList = implode(', ', $names);

        $img = _build_decision_icon_tag('pass');

        $form['decision'] = array("#markup" => "<div>$img$date - $account->name</div>");
        $form['filename'] = array("#markup" => "<div>PACKET TITLE: $title</div>");
        $form['reviewers'] = array("#markup" => "<div>REVIEWERS: $nameList</div>");

        $form['#prefix'] = '<div class="full-citation-element">';
        $form['#suffix'] = '</div>';

        return $form;
    }

    function _format_topic_board_member_responses($articleId, $topicId) {
        $form = array();

        $responses = Ebms\getBoardMemberResponses($articleId, $topicId);
        $pastResponses = Ebms\getBoardMemberResponses($articleId, $topicId,
            false, false);

        if (empty($responses) && empty($pastResponses)) {
            return $form;
        }

        $form['#prefix'] = '<div class="full-citation-element">';
        $form['#suffix'] = '</div>';

        // add the header
        $form['header'] = array('#markup' => "<div class='citation-subheader full-citation-indent'>Board Member Responses</div>");

        // build each packet block
        $responseBlocks = array();
        foreach ($responses as $response) {
            $responseBlocks[] = _format_board_member_response($response);
        }

        if (!empty($responseBlocks)) {
            $form['current_decisions']['blocks'] = $responseBlocks;
        }

        // build out past responses if any
        if (empty($pastResponses)) {
            return $form;
        }

        // build each packet block
        $pastBlocks = array();
        foreach ($pastResponses as $response) {
            $pastBlocks[] = _format_board_member_response($response);
        }

        $form['past_blocks'] = array(
            '#type' => 'fieldset',
            '#title' => t('Past Decisions'),
            '#collapsible' => TRUE,
            '#collapsed' => TRUE,
            'tags' => $pastBlocks,
            '#attributes' => array(
                'class' => array('full-citation-past-items'),
            ),
        );

        return $form;
    }

    function _format_board_member_response($response) {
        $form = array();

        // format contributor row
        $uid = $response->reviewer_id;
        $dt = $response->when_posted;
        $date = _format_date($dt);

        $account = user_load($uid);

        // check the first disposition for a rejection
        if(isset($response->dispositions[0]) &&
                $response->dispositions[0]->value_id != 1) {
            $img = _build_decision_icon_tag('pass');
        }
        else {
            $img = _build_decision_icon_tag('reject');
        }

        $form['decision'] = array("#markup" => "<div class='full-citation-indent'>$img$date - $account->name</div>");
        $form['dispositions'] = array();
        foreach($response->dispositions as $disposition) {
            $action = $disposition->value_name;
            $form['dispositions'][] =
                array("#markup" => "<div class='full-citation-indent'>ACTION: $action</div>");
        }
        foreach ($response->reasons as $index => $reason) {
            $form['reason'][$index] = array("#markup" => "<div class='full-citation-indent'>REASON: $reason->value_name</div>");
        }

        if (isset($response->comments)&&
            !empty($response->comments)) {
            $form['comments'] = array("#markup" => "<div class='full-citation-indent'>COMMENTS: $response->comments</div>");
        }
        if (isset($response->loe_info) &&
            !empty($response->loe_info) &&
            $response->loe_info != 'Not Selected') {
            $form['loe_info'] = array("#markup" => "<div class='full-citation-indent'>LOE: $response->loe_info</div>");
        }

        $form['#prefix'] = '<div class="full-citation-element">';
        $form['#suffix'] = '</div>';

        return $form;
    }

    function _format_topic_publishing($status, &$form_state = null) {
        $form = array();

        $date = _format_date($status->dt);

        $form['header'] = array("#markup" => "<div class='citation-subheader full-citation-indent'>Publication Date</div>");
        $form['date'] = array("#markup" => "<div class='full-citation-indent'>$date</div>");
        $form['comments'] = _format_comments($status->comments);


        return $form;
    }

    function _format_topic_publish($articleId, $topicId, $sequences,
        &$form_state) {
        $form = array();

        $state = Ebms\getCurrentState($articleId, $topicId);

        $showButtons = true;
        // hide buttons for the managers
        global $user;
        if (in_array('board manager', $user->roles)) {
            $showButtons = false;
        }

        // if the current state is not passed initial review...
        if ($state['state_text_id'] != \Ebms\EbmsArticleStates::PassedInitReview)
            return $form;

        // build the rest of the form and the publish button
        $form['header']['title'] = array("#markup" => "<div class='citation-subheader'>Publication Date</div>");
        $form['header']['date'] = array("#markup" => "<i>Awaiting Publication</i>");
        $form['header']['#prefix'] = "<div class='left-side full-citation-element'>";
        $form['header']['#suffix'] = "</div>";

        $hash = "$articleId|$topicId";
        $buttonId = "topic_publish_submit_$hash";

        if ($showButtons)
            $form[$buttonId] = array(
                '#type' => 'submit',
                '#return_value' => $buttonId,
                '#name' => $buttonId,
                '#submit' => array('_topic_publish_save'),
                '#value' => t('Publish'),
                '#attributes' => array(
                    'class' => array('small-button'),
                ),
            );

        $form['#prefix'] = "<div class='full-citation-indent full-citation-element'>";
        $form['#suffix'] = "</div>";

        return $form;
    }

    function _format_topic_publishing_info($articleId, $topicId, $sequences) {
        $form = array();
        _wrap_in_classes($form, array('full-citation-element'));

        // get all known history for this topic
        $allHistory = \Ebms\getStatusHistory($articleId, null, $topicId, true,
            true, 'I');

        // check for a republish
        $publishId = \Ebms\EbmsArticleStates::Published;
        $published = false;
        $republished = false;
        foreach ($allHistory as $state) {
            if ($state->type_text_id == $publishId) {
                if (!$published) {
                    $published = $state;
                } else if (!$republished) {
                    $republished = $state;
                    break;
                }
            }
        }

        if ($published) {

            $dt = $published->dt;
            $date = _format_date($dt);
            $form['publish']['header']["#markup"] = "Publication Date";
            _wrap_in_classes($form['publish']['header'],
                array('citation-subheader', 'full-citation-indent'));
            $form['publish']['date']["#markup"] = "$date";
            _wrap_in_classes($form['publish']['date'],
                array('full-citation-indent'));
            $form['publish']['comments'] = _format_comments($published->comments);
            _wrap_in_classes($form['publish'], array('full-citation-element'));

            if ($republished) {
                $dt = $republished->dt;
                $date = _format_date($dt);

                $form['republish']['header']['#markup'] = 'Re-Publication Date';
                _wrap_in_classes($form['republish']['header'],
                    array('citation-subheader', 'full-citation-indent'));

                $form['republish']['info']['#markup'] = "$date";
                _wrap_in_classes($form['republish']['info'],
                    array('full-citation-element', 'full-citation-indent'));
            }
        }

        // check for any initial rejections
        $rejectId = \Ebms\EbmsArticleStates::RejectInitReview;
        $history = \Ebms\getStatusHistory($articleId, null, $topicId, true,
            false, 'I');
        // if the reject state is followed by any other state...
        $rejectFound = false;
        $unrejected = false;
        foreach ($history as $state) {
            if ($state->type_text_id == $rejectId) {
                $rejectFound = true;
            } else if ($rejectFound) {
                $unrejected = $state;
                break;
            }
        }

        if ($unrejected) {
            $dt = $unrejected->dt;
            $date = _format_date($dt);
            $name = $unrejected->user_name;
            $comments = $unrejected->comments;

            $form['unreject']['header']['#markup'] = 'Unrejected By - On';
            _wrap_in_classes($form['unreject']['header'],
                array('citation-subheader', 'full-citation-indent'));

            $form['unreject']['info']['#markup'] = "$name - $date";
            _wrap_in_classes($form['unreject']['info'],
                array('full-citation-element', 'full-citation-indent'));
        }

        // check for a not list bypass
        // (keep first not list state and the next following state
        // if not list exists)
        $notListId = \Ebms\EbmsArticleStates::RejectJournalTitle;
        $notList = false;
        $bypass = false;
        foreach ($allHistory as $state) {
            if ($state->type_text_id == $notListId && !$notList) {
                $notList = $state;
            } else if ($notList) {
                $bypass = $state;
                break;
            }
        }

        if ($bypass) {
            $dt = $bypass->dt;
            $date = _format_date($dt);
            $name = $bypass->user_name;
            $comments = $bypass->comments;

            $form['bypass']['header']['#markup'] = 'Not Journal Filter Bypassed';
            _wrap_in_classes($form['bypass']['header'],
                array('citation-subheader', 'full-citation-indent'));

            $form['bypass']['info']['#markup'] = "$name - $date";
            _wrap_in_classes($form['bypass']['info'],
                array('full-citation-element', 'full-citation-indent'));
        }

        return $form;
    }

    function _topic_publish_save($form, &$form_state) {
        require_once('EbmsArticle.inc');

        $form_state['rebuild'] = TRUE;

        $returnValue = $form_state['triggering_element']['#return_value'];
        $hash = str_replace('topic_publish_submit_', '', $returnValue);
        list($articleId, $topicId) = explode('|', $hash);

        try {
            $ret = Ebms\setArticleState($articleId,
                Ebms\EbmsArticleStates::Published, $topicId);
        } catch (Exception $e) {
            watchdog_exception('citations', $e);
            drupal_set_message('Could not publish the topic.', 'error');
        }

        if ($ret) {
            $article = new Ebms\EbmsArticle($articleId);
            $articleTitle = $article->getArticleTitle();
            $topicName = Ebms\getTopicNameById($topicId);

            drupal_set_message("Successfully published article '$articleTitle' to topic '$topicName'");

            // also create message for publishing
            Ebms\Util::activityArticlesPublished(Ebms\getTopicBoard($topicId));
        }

        return;
    }

    function _format_topic_board_manager_actions($status, &$form_state = null) {
        $form = array();

        $date = _format_date($status->dt);

        $img = _build_decision_icon_tag('pass');

        $form['decision'] = array("#markup" => "<div class='full-citation-indent'>$img$date - $status->user_name</div>");
        $form['action'] = array("#markup" => "<div class='full-citation-indent'>ACTION: $status->type_name</div>");

        if ($form_state != null) {
            //drop in 'add comment' button
            $form['comments_add_button'] =
                _build_add_state_comment_button($form_state, $status);
        }

        $form['comments'] = _format_comments($status->comments);

        if (empty($form['comments']) && !empty($form['comments_add_button'])) {
            // set the weight of the add comment button to move it level with the decision
            $form['comments_add_button']['#weight'] = -1;
        }


        return $form;
    }

    /**
     * Create a string referencing an icon suitable for inclusion in a form
     * that displays an image icon for passing, rejecting, or fyi'ing an
     * article state.
     *
     * See also _build_add_tag_comment().
     *
     * @param string $type      The kind of icon we want.  Unsupported names return ''.
     * @param bool $active      True=select the active flavored icon, else inactive.
     *                          The meaning of active/inactive seems to vary with context.
     *
     * @return                  String for inclusion in the form.
     */
    function _build_decision_icon_tag($type, $active = true) {

        $icon = null;
        $alt = '';

        switch ($type) {
            case 'pass':
                $icon = $active ? Ebms\PASS_ACTIVE : Ebms\PASS_INACTIVE;
                $alt = 'pass';
                break;
            case 'reject':
                $icon = $active ? Ebms\REJECT_ACTIVE : Ebms\REJECT_INACTIVE;
                $alt = 'reject';
                break;
            case 'fyi':
                $icon = $active ? Ebms\FYI_ACTIVE : Ebms\FYI_INACTIVE;
                $alt = 'FYI';
                break;
            default:
                return '';
        }

        return $prefix = "<img class='full-citation-decision-icon' alt = '$alt' src='$icon'>";
    }
    /* function _format_topic_on_agenda($topic_sequences) {
      $form = array();

      if (!isset($topic_sequences[12]['A'][0]))
      return $form;

      $form['#prefix'] = '<div class="full-citation-element full-citation-indent">';
      $form['#suffix'] = '</div>';

      $status = $topic_sequences[12]['A'][0];

      $form['header'] = array("#markup" => "<div class='citation-subheader'>On Agenda</div>");
      $date = _format_date($status->dt);
      $form['decision'] = array("#markup" => "<div>Approved - $date - $status->user_name</div>");
      $form['comments'] = _format_comments($status->comments);

      return $form;
      } */

    function _format_topic_editorial_board_decision($status, &$form_state) {
        $form = array();

        // get the decisions for this status
        $decisions = Ebms\getBoardDecisionsByArticleState($status->article_state_id);

        foreach ($decisions as $decision) {
            $form[] = array("#markup" => "<div class='full-citation-indent'>DISPOSITION: $decision->value_name</div>");
            $date = '';
            if ($decision->cycle_name)
                $date = "( $decision->cycle_name )";
            $discussed = 'UNKNOWN';
            if($decision->discussed == 'Y') $discussed = 'YES';
            if($decision->discussed == 'N') $discussed = 'NO';
            $form['discussed'] = array("#markup" => "<div class='full-citation-indent'>ARTICLE DISCUSSED - $discussed</div>");
        }

        $memberIds = Ebms\getBoardDecisionMembers($status->article_state_id);
        $members = user_load_multiple($memberIds);
        $memberNames = array();
        foreach ($members as $member) {
            $memberNames[] = $member->name;
        }

        $names = implode(', ', $memberNames);

        $form['member'] = array("#markup" => "<div class='full-citation-indent'>BOARD MEMBERS: $names</div>");

        if ($form_state != null) {
            //drop in 'add comment' button
            $form['comments_add_button'] =
                _build_add_state_comment_button($form_state, $status);
        }

        $form['comments'] = _format_comments($status->comments);

        if (empty($form['comments']) && !empty($form['comments_add_button'])) {
            // set the weight of the add comment button to move it level with the decision
            $form['comments_add_button']['#weight'] = -1;
        }

        return $form;
    }

    function _format_topic_on_agenda_decision($status, &$form_state) {
        $form = array();

        // look up the related event for this state
        $nid = db_select('ebms_agenda_meeting', 'm')
            ->condition('m.article_state_id', $status->article_state_id)
            ->fields('m', array('nid'))
            ->execute()
            ->fetchField();

        if ($nid) {
            $event = node_load($nid);
            if ($event) {
                $nodeWrapper = entity_metadata_wrapper('node', $event);
                $datespan = $nodeWrapper->field_datespan->value();
                $suffix = '';
                if($datespan) {
                    $date = format_date($datespan['value'], 'custom', 'm/d/Y');
                    $suffix = " - $date";
                }

                $form[] = array(
                    "#markup" =>
                    "<div class='full-citation-indent'>MEETING: $event->title$suffix</div>"
                );
            }
        }

        $form['comments'] = _format_comments($status->comments);

        if (empty($form['comments']) && !empty($form['comments_add_button'])) {
            // set the weight of the add comment button to move it level with the decision
            $form['comments_add_button']['#weight'] = -1;
        }

        return $form;
    }

    function _ajax_full_citation($form, &$form_state) {
        $form_state['rebuild'] = true;

        return $form;
    }

    function _ajax_modal_add_tag($js, $articleId, $topicId = null) {
        // grab the resource stylesheet
        $css_path = drupal_get_path('theme', 'ebmstheme');
        $css_path .= "/css/citations.css";
        drupal_add_css($css_path, array('type' => 'file'));

        $form = drupal_get_form('_ajax_modal_add_tag_form', $js, $articleId,
            $topicId);

        if ($js) {
            // react with the modal
            ctools_include('ajax');
            ctools_include('modal');
            ctools_modal_add_js();

            $output = drupal_render($form);

            $form_state = array();
            $form_state['title'] = $topicId ? t('TAG TOPIC') : t('TAG CITATION');
            $commands = ctools_modal_form_render($form_state, $output);

            print ajax_render($commands);
            exit;
        } else {
            return $form;
        }
    }

    function _ajax_modal_add_tag_form($form, &$form_state, $js, $articleId,
        $topicId = null) {

        $form['add_tag'] = _build_add_tag_form($articleId, $topicId, $js);
        $form['add_tag']['#prefix'] = "<div class='full-citation-form'>";
        $form['add_tag']['#suffix'] = "</div>";

        $form['#prefix'] = "<div id='citation-form-div'>";
        $form['#suffix'] = "</div>";

        return $form;
    }

    function _ajax_modal_view_abstract($js, $articleId) {
        // grab the resource stylesheet
        $css_path = drupal_get_path('theme', 'ebmstheme');
        $css_path .= "/css/citations.css";
        drupal_add_css($css_path, array('type' => 'file'));

        $form = drupal_get_form('_ajax_modal_view_abstract_form', $articleId);

        if ($js) {
            // react with the modal
            ctools_include('ajax');
            ctools_include('modal');
            ctools_modal_add_js();

            $output = drupal_render($form);

            $form_state = array();
            $form_state['title'] = t('VIEW ABSTRACT');
            $commands = ctools_modal_form_render($form_state, $output);

            print ajax_render($commands);
            exit;
        } else {
            return $form;
        }
    }

    function _ajax_modal_view_abstract_form($form, &$form_state, $articleId) {
        require_once('EbmsArticle.inc');

        $article = new Ebms\EbmsArticle($articleId);
        $abstract = _format_abstract($article);
        $form['abstract']['#markup'] = $abstract;

        // build pubmed direct link
        $target = array('attributes' => array('target' => '_blank'));
        $pubmed = 'http://www.ncbi.nlm.nih.gov/pubmed/';
        $pm_id = $article->getSourceId();
        $pm_link = $pubmed . $pm_id;

        $item = l(t('VIEW ABSTRACT ON PUBMED'), $pm_link, $target);

        $form['pubmed_link']['#markup'] = "<div>$item</div>";

        $form['#prefix'] = "<div id='view-abstract-div'>";
        $form['#suffix'] = "</div>";

        return $form;
    }

    function _build_expand_boards_button(&$form_state, $articleId, $boardId = 0) {
        $form = array();

        // create hash for article/board
        $hash = "$articleId|$boardId";

        $buttonId = "expand_board_$hash";
        $groupId = "expand_$hash";

        // check if related form is triggered
        $triggered = _is_triggered_by($form_state, $buttonId);

        if ($triggered === 'persist') {
            // grab icon for button
            $iconPath = Ebms\EDIT_ACTIVE;

            $form['triggered'] = array();
        } else {
            // grab icon for button
            $iconPath = Ebms\EDIT_INACTIVE;
        }

        // create edit decision button
        $form['button'][$buttonId] = array(
            '#type' => 'image_button',
            '#return_value' => $buttonId,
            '#src' => $iconPath,
            '#ajax' => array(
                'callback' => '_ajax_expand_boards',
                'wrapper' => 'citation-results-div',
                'progress' => array(
                    'type' => 'throbber',
                    'message' => t('')),
            ),
            '#attributes' => array(
                'class' => array('full-citation-button', 'image-button'),
                'title' => 'Add Topic',
                'alt' => 'Add Topic',
            ),
            '#persist' => array(
                'type' => 'toggle',
                'group' => $groupId),
        );

        return $form;
    }

    function _ajax_expand_boards($form, &$form_state) {
        $form_state['rebuild'] = true;

        return $form['citation_queue_results'];
    }
}
