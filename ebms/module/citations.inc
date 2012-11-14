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

    class EbmsTopicDecisions {
        static $INITIALREVIEW = null;
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
                EbmsArticleStates::AgendaFutureChg,
                EbmsArticleStates::AgendaFutureDiscuss,
                EbmsArticleStates::FullEnd,));
            self::$ONAGENDA =
                new EbmsTopicDecisions($index++, 'On Agenda', null,
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
     * @file
     *
     * Implementation of EBMS page for general site information.
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

                $form = drupal_get_form('pdq_ebms_full_citation', $article_id);

                $result['left_nav'] = \Ebms\Util::build_librarian_left_nav(null,
                        false);

                $result['left_nav']['actions'] = _build_full_citation_actions($article_id);

                $result['left_nav']['icon_key'] = _build_full_citation_icon_key();

                $result['full_citation_form'] = $form;

                break;
            case 'not-list':
                $crumbs[] = "Not List Maintenance";
                $form = drupal_get_form('pdq_ebms_citation_not_list');


                $result['left_nav'] = \Ebms\Util::build_librarian_left_nav('Not List Maintentance',
                        false);

                $result['not_list_form'] = $form;

                break;
            case null:
            case 'review':
                $crumbs[] = 'Review Citations';

                $form = drupal_get_form('pdq_ebms_citation_queue', $is_librarian);

                $result['left_nav'] = \Ebms\Util::build_librarian_left_nav(null,
                        false);

                $result['cite_queue_form'] = $form;

                break;
        }
        drupal_set_breadcrumb($crumbs);
        if ($result)
            return $result;

        return "<i>Stub for 'Citations' page, action = $action</i>";
    }

    function pdq_ebms_citation_not_list($form, &$form_state) {

        require_once('EbmsArticle.inc');

        //include js to redirect pager links
        $ret = drupal_add_js(EBMS\JS_DIR . '/citations.js', 'file');

        // retrieve boards
        $boards = Ebms\Util::boards();

        //db_select('ebms_not_list', 'nl')->join($table, $alias);
        $form['#prefix'] = '<div id="citation-div">';
        $form['#suffix'] = '</div>';

        $form['not-list']['#prefix'] = '<div id="citation-form-div">';
        $form['not-list']['#suffix'] = '</div>';

        $form['not-list']['description']['#markup'] = '<h2>Not List Maintenance</h2>';
        $form['not-list']['board_select'] = array(
            '#type' => 'select',
            '#title' => 'Editorial Board',
            '#title_display' => 'after',
            '#options' => $boards, //array(0 => t('All Boards')),
            '#prefix' => '<div class="citation-input-div">',
            '#suffix' => '</div>',
            //'#default_value' => $board,
        );
        //$form['not-list']['board_select']['#options'] += $boards;
        $form['not-list']['brief_title_field'] = array(
            '#type' => 'textfield',
            '#title' => 'Brief Journal Title',
            '#title_display' => 'after',
            '#prefix' => '<div class="citation-input-div">',
            '#suffix' => '</div>',
            //'#default_value' => $brief_title,
        );
        $form['not-list']['full_title_field'] = array(
            '#type' => 'textfield',
            '#title' => 'Full Journal Title',
            '#title_display' => 'after',
            '#prefix' => '<div class="citation-input-div">',
            '#suffix' => '</div>',
            //'#default_value' => $full_title,
        );
        $form['not-list']['journal_id_field'] = array(
            '#type' => 'textfield',
            '#title' => 'Journal ID',
            '#title_display' => 'after',
            '#prefix' => '<div class="citation-input-div">',
            '#suffix' => '</div>',
            //'#default_value' => $id,
        );
        $form['not-list']['all_journals_check'] = array(
            '#type' => 'checkbox',
            '#title' => 'All Journals',
            '#title_display' => 'after',
            '#prefix' => '<div class="citation-input-div">',
            '#suffix' => '</div>',
            //'#default_value' => $all_journals,
        );

        $form['not-list']['search_button'] = array(
            '#type' => 'submit',
            '#value' => 'Search',
            /* '#ajax' => array(
              'callback' => 'pdq_ebms_ajax_search_submit',
              'wrapper' => 'citation-results-div',
              'method' => 'replace',
              ), */
        );

        $form['not-list']['reset_button'] = array(
            '#type' => 'submit',
            '#value' => 'Reset',
            /* '#ajax' => array(
              'callback' => 'pdq_ebms_ajax_clear_form',
              'wrapper' => 'citation-div',
              'method' => 'replace',
              ), */
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

        if ($clear_states)
            $form_state['altered_states'] = array();

        // wipe out button values so that the default_states calculated from
        // altered states can take effect
        _clear_journal_select($form_state['input'], 'input');
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
        // add the pager early on in the process
        $query = $query->extend('PagerDefault');

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
                array('brf_jrnl_title', 'jrnl_title', 'source_jrnl_id', 'source'))
            ->limit(10);

        $query->addExpression('!ISNULL(nl.source_jrnl_id)', 'default_value');

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
            /* '#ajax' => array(
              'callback' => 'pdq_ebms_ajax_save_list',
              ), */
        );

        $form['buttons']['cancel_button'] = array(
            '#type' => 'submit',
            '#value' => 'Cancel',
            /* '#ajax' => array(
              'callback' => 'pdq_ebms_ajax_clear_form',
              'wrapper' => 'citation-results-div',
              'method' => 'replace',
              ), */
        );

        $form['top-pager'] = array(
            '#theme' => 'pager',
            '#attributes' => array(
                'class' => 'top-pager'
            ));

        $form['table'] = array(
            '#prefix' => '<div id="citation-table-div">',
            '#suffix' => '</div>',
        );

        $form['table']['header'] = array(
            '#prefix' => "<div class='citation-header'>",
            '#suffix' => '</div>',
        );

        $form['table']['header'][]['#markup'] =
            "<div class='citation-cell brief-title left-column'>Brief Journal Title</div>";
        $form['table']['header'][]['#markup'] =
            "<div class='citation-cell full-title'>Full Journal Title</div>";
        $form['table']['header'][]['#markup'] =
            "<div class='citation-cell id'>Journal ID</div>";
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

        $form['bottom-pager'] = array(
            '#theme' => 'pager',
            '#attributes' => array(
                'class' => 'top-pager'
            ));

        return $form;
    }

    function pdq_ebms_citation_queue($form, &$form_state, $is_librarian) {
        require_once('EbmsArticle.inc');

        //include js to redirect pager links
        $ret = drupal_add_js(EBMS\JS_DIR . '/citations.js', 'file');

        // retrieve boards
        $boards = Ebms\Util::boards();

        //db_select('ebms_not_list', 'nl')->join($table, $alias);
        /* $form['#prefix'] = '<div id="citation-div">';
          $form['#suffix'] = '</div>'; */

        $form['citation_queue']['#prefix'] = '<div id="citation-form-div">';
        $form['citation_queue']['#suffix'] = '</div>';

        $form['citation_queue']['description']['#markup'] = '<h2>Sort My Citation Queue</h2>';

        $form['citation_queue']['board_select'] = array(
            '#type' => 'select',
            '#title' => 'Editorial Board',
            '#title_display' => 'after',
            '#options' => array(0 => ''),
            '#prefix' => '<div class="citation-input-div">',
            '#suffix' => '</div>',
            '#ajax' => array(
                'callback' => '_ajax_citation_queue_board_select',
                'wrapper' => 'topic-select-div'
            ),
        );
        $form['citation_queue']['board_select']['#options'] += $boards;
        $form['citation_queue']['topic_select'] = array(
            '#type' => 'select',
            '#title' => 'Summary Topic',
            '#title_display' => 'after',
            '#options' => array(0 => 'Please select a board.'),
            '#prefix' => '<div id = "topic-select-div" class="citation-input-div">',
            '#suffix' => '</div>',
        );
        if (isset($form_state['values']['board_select'])) {
            $board_id = $form_state['values']['board_select'];
            if ($board_id != 0) {
                $form['citation_queue']['topic_select']['#options'] = array(0 => 'Please select a topic.');

                $result = array();
                try {
                    $result = db_select('ebms_topic', 't')->condition('board_id',
                            $board_id)->fields('t')->execute();
                } catch (Exception $e) {
                    
                }
                foreach ($result as $record) {
                    $form['citation_queue']['topic_select']['#options'][$record->topic_id]
                        = $record->topic_name;
                }
            }
        }

        $form['citation_queue']['journal_id_field'] = array(
            '#type' => 'textfield',
            '#title' => 'Journal ID',
            '#title_display' => 'after',
            '#prefix' => '<div class="citation-input-div">',
            '#suffix' => '</div>',
        );

        if (!$is_librarian) {

            $checks = array();
            $checks["abstract_review_check"] = array(
                '#type' => 'checkbox',
                '#title' => t('Abstract Review'),
                '#default_value' => true,
            );

            $checks["fulltext_review_check"] = array(
                '#type' => 'checkbox',
                '#title' => t('Full Text Review'),
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
            /* '#ajax' => array(
              'callback' => 'pdq_ebms_ajax_clear_form',
              'wrapper' => 'citation-div',
              'method' => 'replace',
              ), */
        );

        $form['citation_queue']['sort_button'] = array(
            '#type' => 'submit',
            '#value' => 'Sort',
        );

        $form['citation_queue']['submit_source'] = array(
            '#type' => 'hidden',
        );

        if (isset($form_state['triggering_element']['#type']) &&
            $form_state['triggering_element']['#type'] == 'select') {
            // skip results if updating via ajax
            return $form;
        }

        _citation_queue_results($form, $form_state, $is_librarian);

        return $form;
    }

    function _ajax_citation_queue_board_select($form, $form_state) {
        return $form['citation_queue']['topic_select'];
    }

    function _citation_queue_results(&$form, &$form_state, $is_librarian) {

        $board_id = 0;
        $topic_id = 0;
        $journal_id = '';
        $sort_by = '';
        $format = array();
        $per_page = 10;
        $core_only = false;
        $fulltext_review = false;

        // get values if they exist
        if (isset($form_state['values'])) {
            $board_id = $form_state['values']['board_select'];
            $topic_id = $form_state['values']['topic_select'];
            $journal_id = $form_state['values']['journal_id_field'];
            $sort_by = $form_state['values']['sort_by'];
            $format = $form_state['values']['format']['brief'] ? $form_state['values']['format']['brief']
                    : $form_state['values']['format']['abstract'];
            $per_page = $form_state['values']['per_page'];
            //$core_only = $form_state['values']['core_only'];
            $fulltext_review = isset($form_state['values']['fulltext_review_check'])
                    ? $form_state['values']['fulltext_review_check'] : false;
        }

        if (!isset($form['citation_queue_results'])) {
            $form['citation_queue_results'] = array(
                '#prefix' => '<div id="citation-results-div">',
                '#suffix' => '</div>',
            );
        }

        $results = &$form['citation_queue_results'];

        try {
            $query = db_select('ebms_article_state', 's');
            $query->join('ebms_article', 'a', 'a.article_id = s.article_id');

            $source_state = \Ebms\EbmsArticleStates::ReadyInitReview;
            if (!$is_librarian) {
                $source_state = \Ebms\EbmsArticleStates::Published;
                if ($fulltext_review)
                    $source_state = \Ebms\EbmsArticleStates::PassedBMReview;
            }
            $state = Ebms\getStateTypebyTextId($source_state);

            $query->condition('s.state_id', $state->state_id);
            // add the pager early on in the process
            $query = $query->extend('PagerDefault');

            // add the other filter fields
            // $topic_id implies $board_id
            if ($topic_id)
                $query->condition('s.topic_id', $topic_id);
            else if ($board_id)
                $query->condition('s.board_id', $board_id);

            if ($journal_id)
                $query->condition('a.source_jrnl_id', "%$journal_id%", 'LIKE');

            $query->groupBy('s.article_id');
            $query->condition('s.current', 'Y');

            $num_rows = $query->countQuery()->execute()->fetchField();

            if ($per_page != 'all') {
                $query->limit($per_page);
            }

            $query
                ->fields('s', array('article_id'));
            $query->addExpression('GROUP_CONCAT(s.topic_id)', 'topics');

            // add ORDER BYs based on display options
            switch ($sort_by) {
                case 'pmid':
                    $query->orderBy('a.source_id');
                    break;
                case 'cms_id':
                    $query->orderBy('s.article_id');
                    break;
                case 'title':
                    $query->orderBy('a.article_title');
                    break;
                case 'journal':
                    $query->orderBy('a.jrnl_title');
                    $query->orderBy('a.brf_jrnl_title');
                    break;
                case 'publication_date':
                    $query->orderBy('a.published_date');
                    break;
                case 'author':
                    _add_author_sort($query);
                    break;
            }

            $result = $query->execute();
        } catch (Exception $e) {
            $num_rows = -1;
            $result = array();
            watchdog('EBMS - citations', (string) $e);
        }

        $results['results-title']['#markup'] = "<h2>Citations Requiring Review <span class = 'results-count'>($num_rows)</span></h2>";

        $results['buttons'] = array(
            '#prefix' => '<div id="citation-buttons-div">',
            '#suffix' => '</div>',
        );

        $results['buttons']['save_button'] = array(
            '#type' => 'submit',
            '#value' => 'Save',
        );

        $results['buttons']['refresh_button'] = array(
            '#type' => 'submit',
            '#value' => 'Refresh',
        );

        $results['buttons']['display-options'] = array(
            '#type' => 'fieldset',
            '#title' => '<span class="citation-subheader">Display Options</span>',
            'options' => _build_citation_sort_options(),
            '#collapsible' => true,
            '#collapsed' => true,
            //'#prefix' => "<div class='full-citation-element'>",
            //'#suffix' => "</div>",
        );

        $results['top-pager'] = array(
            '#theme' => 'pager',
            '#attributes' => array(
                'class' => 'top-pager'
            ));

        $results['table'] = array(
            '#prefix' => '<table>',
            '#suffix' => '</table>',
        );

        $results['table']['header'] = array(
            '#prefix' => "<tr>",
            '#suffix' => '</tr>',
        );

        // width of the full citation and
        $colspan = $fulltext_review ? 5 : 4;
        $isAbstract = $format == 'abstract';

        if ($isAbstract) {
            $results['table']['header']['actions']['#markup'] =
                "<th class='actions left-column borderless'>&nbsp;</th>";
            $results['table']['header']['abstracts']['#markup'] =
                "<th colspan='$colspan' class='borderless'>Citation Abstracts</th>";
        } else {
            $results['table']['header']['actions']['#markup'] =
                "<th class='actions left-column borderless'>&nbsp;</th>";
            $results['table']['header']['brief_citation']['#markup'] =
                "<th class='brief-citation borderless'>Citations</th>";
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
            $article = new Ebms\EbmsArticle($record->article_id);

            // retrieve the topics and related boards
            $curr_topics = explode(',', $record->topics);

            // grab all topics and board from the topic objects
            $boards = array();
            $topics = array();
            foreach ($article->getTopics() as $topic) {
                if (in_array($topic->topic_id, $curr_topics)) {
                    $boardName = $topic->board_name;
                    if (!in_array($boardName, $boards))
                        $boards[] = $boardName;
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
            $cite = _cite_article($article, false, $isAbstract);

            $recordIx = $ix + $first_row;

            if ($isAbstract) {
                $row['actions']['#prefix'] = "<td class='borderless'>";
                $row['actions']['#suffix'] = "</td>";

                $row['actions']['row_num'] = array('#markup' => "<h2 class='borderless'>$recordIx</h2>");
                /* $row['actions']['flag'] = array('#type' => 'submit', '#value' => t('Flag'), '#suffix' => '<br />');
                  $row['actions']['edit'] = array('#type' => 'submit', '#value' => t('Edit'), '#suffix' => '<br />'); */
                $row['actions']['tag'] = _build_add_tag_button(true,
                    $form_state, $article->getId());

                // set the abstract to the remaining width of the table, finish
                // the row, and begin another with a header for the boards and topics
                $row['abstract']['#markup'] =
                    "<td colspan='$colspan' class='left-column borderless'><a href='/citations/full/$record->article_id'>$cite</a></td>";

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
                $row['actions']['#prefix'] = "<td rowspan='$topicCount' class='borderless'>";
                $row['actions']['#suffix'] = "</td>";

                $row['actions']['row_num'] = array('#markup' => "<h2 class='borderless'>$recordIx</h2>");
                /* $row['actions']['flag'] = array('#type' => 'submit', '#value' => t('Flag'), '#suffix' => '<br />');
                  $row['actions']['edit'] = array('#type' => 'submit', '#value' => t('Edit'), '#suffix' => '<br />'); */
                $row['actions']['tag'] = _build_add_tag_button(true,
                    $form_state, $article->getId());

                $row['brief_citation']['#markup'] =
                    "<td rowspan='$topicCount' class='brief-citation left-column borderless'><a href='/citations/full/$record->article_id'>$cite</a></td>";
            }
            while (isset($boardTopicMap[$boardIx])) {
                // add the board (and potentially actions), and cycle through 
                // the topics
                $topicIx = 0;
                $boardName = $boardTopicMap[$boardIx]['boardName'];
                $boardTopicCount = sizeof($boardTopicMap[$boardIx]['topics']);

                $boardClass = 'board-cell';
                if ($isAbstract) {
                    $row['actions']['#prefix'] = "<td rowspan='$boardTopicCount' class='borderless'>";
                    $row['actions']['#suffix'] = "</td>";

                    $boardClass = "borderless";
                    //$row['actions']['edit'] = array('#markup' => ($ix + $first_row) . "<br />");
                }

                $row['board']['#markup'] = "<td class='$boardClass' rowspan='$boardTopicCount'>$boardName</td>";

                while (isset($boardTopicMap[$boardIx]['topics'][$topicIx])) {
                    $topic = $boardTopicMap[$boardIx]['topics'][$topicIx];

                    // create topic label
                    $row['topic']['#markup'] = "<td class='summary-topic-name'>$topic->topic_name</td>";

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
            unset($rows[$rowIx]);

            $results['table']['rows'][$ix] = $rows;

            $ix++;
        }

        $results['bottom-pager'] = array(
            '#theme' => 'pager',
            '#attributes' => array(
                'class' => 'top-pager'
            ));
    }

    function _build_citation_sort_options() {
        $form = array(
            '#prefix' => '<div class="citation-input-div childfix">',
            '#suffix' => '</div>',
        );

        $sortOptions = array(
            null => '',
            'cms_id' => t("CMS ID#"),
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
            '#title_display' => 'after',
        );

        $formatOptions = array(
            'brief' => t('Brief'),
            'abstract' => t('Abstract'),
        );

        $form['format'] = array(
            '#type' => 'checkboxes',
            '#title' => t('Format'),
            '#title_display' => 'after',
            '#options' => $formatOptions,
            '#default_value' => array('brief'),
            '#attributes' => array('class' => array('full-citation-radio-check')),
        );

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
            '#title_display' => 'after',
        );

        /* $form['core_only'] = array(
          '#type' => 'checkbox',
          '#title' => t('Core Journals Only'),
          '#title_display' => 'after',
          ); */


        return $form;
    }

    function _add_author_sort(PagerDefault &$query) {
        $authorQuery = db_select('ebms_article_author', 'a')
            ->fields('a', array('author_id'));
        $authorQuery->addExpression("CONCAT_WS(', ', last_name, forename, collective_name)",
            'name');

        $nameQuery = db_select($authorQuery, 'n');
        $nameQuery->join('ebms_article_author_cite', 'c',
            'c.author_id = n.author_id');

        $nameQuery->condition('c.cite_order', 1)
            ->fields('c', array('article_id'))
            ->fields('n', array('name'));

        $query->join($nameQuery, 'l', "s.article_id = l.article_id");
        $query->orderBy('l.name');
        $query->fields('l', array('name'));
    }

    function pdq_ebms_citation_queue_submit($form, &$form_state) {
        $is_librarian = $form_state['build_info']['args'][0];

        $fulltext_review = isset($form_state['values']['fulltext_review_check'])
                ? $form_state['values']['fulltext_review_check'] : false;

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
                    return;
                case 'Save':
                    // write out the changes to the not list, in terms of
                    // inserts and deletes to the DB
                    $source_state = \Ebms\EbmsArticleStates::ReadyInitReview;
                    if (!$is_librarian) {
                        if ($fulltext_review)
                            $source_state = Ebms\EbmsArticleStates::PassedBMReview;
                        else
                            $source_state = \Ebms\EbmsArticleStates::Published;
                    }
                    _write_topic_states($form_state['altered_topics'],
                        $source_state);

                    $clear_states = true;
                    break;
                case 'Sort':
                case 'Refresh':
                    $clear_states = true;
                    break;
            }
        }

        if ($clear_states)
            $form_state['altered_topics'] = array();

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

            $children = $abstractXml->children();

            foreach ($children as $element) {
                $text = (string) $element;
                $label = $element['Label'];
                if ($label)
                    $pieces[] = "<div class='citation-abstract-label'>$label</div>";

                $pieces[] = "<div class='citation-abstract-text'>$text</div>";
            }

            return implode('', $pieces);
        } catch (Exception $e) {
            watchdog('EBMS - citations', (string) $e);
        }
    }

    function _cite_article(Ebms\EbmsArticle $article, $long_form = true,
        $showAbstract = false) {
        $article_id = $article->getId();
        $xml = $article->getSourceData();
        $doc = new \SimpleXMLElement($xml);
        $title = $ta = $vol = $issue = $pgn = $year = $pmid = '';
        $articleXML = $doc->MedlineCitation->Article;
        $title = htmlspecialchars($articleXML->ArticleTitle);
        if (!$long_form)
            $title = "<em>$title</em>";
        $pgn = htmlspecialchars($articleXML->Pagination->MedlinePgn);
        $year = $articleXML->Journal->JournalIssue->PubDate->Year;
        $issue = htmlspecialchars($articleXML->Journal->JournalIssue->Issue);
        $vol = htmlspecialchars($articleXML->Journal->JournalIssue->Volume);
        $authors = array();
        if (isset($doc->MedlineCitation->Article->AuthorList->Author)) {
            foreach ($doc->MedlineCitation->Article->AuthorList->Author
            as $author) {
                if (sizeof($authors) == 8) {
                    $authors[] = 'et al.';
                    break;
                }
                $name = $author->LastName;
                if ($author->Initials)
                    $name .= ' ' . $author->Initials;
                $authors[] = htmlspecialchars($name);
            }
        }
        if (count($authors) < 1)
            $authors = array('[No authors listed]');
        $ta = $doc->MedlineCitation->MedlineJournalInfo->MedlineTA;
        $ta = htmlspecialchars($ta);
        $pmid = $doc->MedlineCitation->PMID;
        if (!$long_form)
            $pmid = "PMID: $pmid";
        $cmsId = $article_id;

        $tAuthors = '';
        $tTitle = '';
        $tJournal = '';
        $tPMID = '';
        $tCMSID = '';
        $tReviewCycle = '';

        if ($long_form) {
            $tAuthors = '<div class="citation-subheader">AUTHORS</div>';
            $tTitle = '<div class="citation-subheader">TITLE</div>';
            $tJournal = '<div class="citation-subheader">JOURNAL</div>';
            $tPMID = '<div class="citation-subheader">PMID</div>';
            $tCMSID = '<div class="citation-subheader">CMS ID</div>';
            $tReviewCycle = '<div class="citation-subheader">REVIEW CYCLE</div>';


            /* $cycleYear = $doc->MedlineCitation->DateCreated->Year;
              $cycleMonth = $doc->MedlineCitation->DateCreated->Month;
              $reviewCycle = "$cycleMonth/$cycleYear"; */
            list($cycleYear, $cycleMonth) = explode('-',
                $article->getImportDate());
            $reviewCycle = "$cycleMonth/$cycleYear";
        }

        $pieces = array(
            "<div class='article-citation'>$tAuthors<div>",
            implode(', ', $authors),
            "</div>$tTitle<div>$title</div>$tJournal<div>$ta $vol",
        );
        if ($issue)
            $pieces[] = "($issue)";
        if ($pgn)
            $pieces[] = ": $pgn";
        if ($year)
            $pieces[] = ", $year";

        if ($showAbstract)
            $cmsId = "CMS ID: $cmsId";
        else
            $cmsId = '';

        $pieces[] = ".</div>$tPMID<div>$pmid&nbsp;&nbsp;&nbsp;&nbsp;$cmsId</div>";

        if (!$long_form) {
            if ($showAbstract) {
                $pieces[] = _format_abstract($article);
            }

            $pieces[] = '<div>';
            if (!$showAbstract)
                $pieces[] = '<a href="/ebmsdev/stub">VIEW ABSTRACT</a> | ';

            // build full citation text url
            $fileUrl = null;
            if ($article->getFullTextId()) {
                $file = file_load($article->getFullTextId());
                $fileUrl = file_create_url($file->uri);
            }

            $pieces[] = "<a href='$fileUrl'>DOWNLOAD FULL TEXT</a></div>";
        } else {
            $pieces[] = "$tCMSID<div>$cmsId</div>";

            $cycles = Ebms\getArticleReviewCycle($article_id);

            if (sizeof($cycles) > 1)
                $tReviewCycle = '<div class="citation-subheader">REVIEW CYCLES</div>';

            $cycleNames = array();
            foreach ($cycles as $cycle) {
                $cycleNames[] = $cycle->cycle_name;
            }

            $reviewCycle = implode(', ', $cycleNames);

            if (!empty($reviewCycle))
                $pieces[] = "$tReviewCycle<div>$reviewCycle</div>";
        }
        $pieces[] = '</div>';
        return implode($pieces);
        #$results = db_select('ebms_article', 'a')
        #    ->fields('a', array('article_title', 'source_id',
        #return "Article $position (CMS ID $article_id)";
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

        // look through each article/topic hash and determine if they are rejected or
        // passed
        foreach ($values as $hash => $actions) {
            // get the article id and topic id from the hash,
            // and the reject and pass states from the actions array
            list($article_id, $topic_id) = explode("|", $hash);
            $reject = $actions['reject'];
            $pass = $actions['pass'];
            $fyi = isset($actions['fyi']) ? $actions['fyi'] : 0;

            // define the new state, default to reject
            $new_state_name = $reject_name;

            // if not exactly one state set, do nothing
            // ((shouldn't actually happen))
            if (($pass + $reject + $fyi != 1)) {
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
                $topic_id);
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
    function _wrap_in_classes(&$form, $classes) {
        $classText = implode(' ', $classes);
        $form['#prefix'] = "<div class='$classText'>";
        $form['#suffix'] = "</div>";
    }

    function _build_full_citation_actions($articleId) {
        $form = array();
        _wrap_in_classes($form, array('left-nav-box'));

        $form['title']['#markup'] = t('Citation Actions');
        _wrap_in_classes($form['title'], array('left-nav-box-title'));

        $article = new Ebms\EbmsArticle($articleId);

        // check for search query
        $query = drupal_get_query_parameters();
        $searchId = null;
        if (isset($query['search']))
            $searchId = $query['search'];

        $searchPath = 'citations/search' . ($searchId ? "/r$searchId" : '');

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

        $links = array(
            array('Refine Search', $searchPath, array()),
            array('View Abstract', $pm_link, $target),
            array('Download Full Text', $fileUrl, array()),
            //array('Add Comments', 'citations/search'),
        );

        $items = array();
        foreach ($links as $link) {
            list($label, $url, $options) = $link;

            if ($url)
                $item = l($label, $url, $options);
            else {
                $item = "<a>$label</a>";
            }

            $items[]['#markup'] = "<div>$item</div>";
        }

        $form['links'] = $items;
        _wrap_in_classes($form['links'], array('left-nav-box-text'));

        return $form;
    }

    function _build_full_citation_icon_key() {
        $form = array();
        _wrap_in_classes($form, array('left-nav-box'));

        $form['title']['#markup'] = t('Icon Key');
        _wrap_in_classes($form['title'], array('left-nav-box-title'));

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
                $markup .= "<img class='left-nav-icon' src='$icon'>";
            else
                $text = "<div class='left-nav-most-recent'>$text</div>";

            $markup .= $text;

            $element = array();
            $element['#markup'] = $markup;
            _wrap_in_classes($element, array('left-nav-box-text'));

            $markups[] = $element;
        }

        $form['text'] = $markups;

        return $form;
    }

    function pdq_ebms_full_citation($form, &$form_state, $articleId) {
        require_once('EbmsArticle.inc');

        //include js to add radio/checkbox behavior
        $ret = drupal_add_js(EBMS\JS_DIR . '/citations.js', 'file');

        // hook into ajax completions to apply jQuery to form components
        drupal_add_js('jQuery(document).ajaxComplete(function(){       
            addHooks();                                      
        });',
            'inline');

        $article = new Ebms\EbmsArticle($articleId);

        $cite = _cite_article($article);

        $form['#prefix'] = '<div id="citation-form-div">';
        $form['#suffix'] = '</div">';

        $form['title'] = array('#markup' => "<h2>Full Citation</h2>");

        $form['citation'] = array('#markup' => "<div class='full-citation-element'>$cite</div>");

        // build the citation tags    
        $tags = Ebms\getTagHistory($articleId, null, null, true, false, 'D');

        // reverse order so most recent is first
        $tags = array_reverse($tags);

        // build the render array of tags
        $tag_array = _format_tags($form_state, $tags);
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

        // build the board topics
        $history = Ebms\getStatusHistory($articleId, null, null, true, true,
            'BTS');
        $form['editorial_boards'] = _format_boards($form_state, $article,
            $history, $tags);

        $importData = Ebms\getArticleImportData($article->getId());
        $form['import_process'] = array(
            '#type' => 'fieldset',
            '#title' => '<span class="citation-subheader">Import Process</span>',
            'section' => _format_import($article, $importData),
            '#collapsible' => true,
            '#collapsed' => true,
            '#prefix' => "<div class='full-citation-element'>",
            '#suffix' => "</div>",
        );

        $form['initial_review'] = array(
            '#type' => 'fieldset',
            '#title' => '<span class="citation-subheader">Medical Librarian Initial Review</span>',
            'section' => _format_boards($form_state, $article, $history, $tags,
                'librarian'),
            '#collapsible' => true,
            '#collapsed' => true,
            '#prefix' => "<div class='full-citation-element'>",
            '#suffix' => "</div>",
        );

        $form['publishing'] = array(
            '#type' => 'fieldset',
            '#title' => '<span class="citation-subheader">Publishing</span>',
            'section' => _format_boards($form_state, $article, $history, $tags,
                'publishing'),
            '#collapsible' => true,
            '#collapsed' => true,
            '#prefix' => "<div class='full-citation-element'>",
            '#suffix' => "</div>",
        );

        // need to ensure that the fieldsets leading to any triggered element
        // are expanded
        if (isset($form_state['triggering_element']['#array_parents'])) {
            $parents = $form_state['triggering_element']['#array_parents'];
            $path = &$form;
            foreach ($parents as $parent) {
                $path = &$path[$parent];

                if (isset($path['#type']) && $path['#type'] == 'fieldset') {
                    $path['#collapsed'] = false;
                }
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

        $form['article_tag_comment'] = _format_comments($tag->comments);

        return $form;
    }

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
                '#markup' => "<img class='full-citation-button image-button' src='$iconPath'>",
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
                '#markup' => "<img class='full-citation-button image-button' src='$iconPath'>",
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
     * @param mixed $tags
     * @return mixed the formatted render array.
     */
    function _format_tags(&$form_state, $tags, $showButtons = true,
        $topicId = null) {
        $form = array();

        // organize the tags by topic
        $topic_tags = _build_tag_array($tags);

        // determine the desired topic id, 0 representing topicless tags
        $targetTopicId = $topicId;
        if ($topicId == null)
            $targetTopicId = 0;

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

        // create hash for article/topic
        $hash = "$articleId|$topicId";

        $buttonId = "add_tag_$hash";

        // check if related form is triggered
        $triggered = _is_triggered_by($form_state, $buttonId);

        if ($triggered) {
            // grab icon for button
            $iconPath = $noTags ? Ebms\TAG_ACTIVE : Ebms\ADD_DECISION_ACTIVE;
            //Ebms\IMAGES_DIR . '/EBMS_AddDecision_Icon_Active.png';
            // create add tag button
            $form['button'][$buttonId] = array(
                '#markup' => "<img class='full-citation-button image-button' src='$iconPath'>",
            );

            $formId = "add_tag_form_$hash";
            $form['form'][$formId] = _build_add_tag_form($articleId, $topicId);
            $form['form'][$formId]['#prefix'] = '<div class="full-citation-form full-citation-indent">';
            $form['form'][$formId]['#suffix'] = '</div>';
        } else {
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
     * Checks if the form has been triggered by an element with a particular return
     * value.
     * 
     * @param mixed $form_state     The current form state.
     * @param string $element_value The return value of interest.
     * @return bool                 Returns true if the form state has a triggering
     *                              element with the given return value.
     */
    function _is_triggered_by(&$form_state, $element_value) {
        if ((isset($form_state['triggering_element']['#return_value']) &&
            $form_state['triggering_element']['#return_value'] == $element_value)
        )
            return 'trigger';

        if (isset($form_state['form_progress'][$element_value]))
            return 'progress';

        return false;
    }

    function _build_add_tag_form($article_id, $topic_id = null) {
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
            '#required' => true,
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

        return $form;
    }

    function _form_cancel($form, &$form_state) {
        $form_state['rebuild'] = TRUE;
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
        list($articleId, $topicId) = explode("|", $hash);

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
                '#markup' => "<img class='full-citation-button image-button' src='$iconPath'>",
            );

            $formId = "edit_decision_form_$hash";

            // check for the topic id to short-circuit to the add decision form
            $values = _retrieve_hash_values($form_state,
                array('add_board_topic' => array('select_board', 'select_topic')));

            if (isset($values['add_board_topic']['select_topic'])) {
                $topicId = $values['add_board_topic']['select_topic'];

                $decisions = array();
                if ($mode == 'librarian') {
                    $decisions = array(Ebms\EbmsTopicDecisions:: $INITIALREVIEW);
                } else if ($mode == 'publishing') {
                    $decisions = array(Ebms\EbmsTopicDecisions:: $PUBLISHED);
                } else {
                    $decisions = array(
                        Ebms\EbmsTopicDecisions:: $NCIREVIEWER,
                        Ebms\EbmsTopicDecisions:: $FULLTEXT,
                        Ebms\EbmsTopicDecisions:: $SURVEILLANCE,
                        Ebms\EbmsTopicDecisions:: $BOARDMEMBERRESPONSES,
                        Ebms\EbmsTopicDecisions:: $BOARDMANAGERACTIONS,
                        Ebms\EbmsTopicDecisions:: $ONAGENDA,
                        Ebms\EbmsTopicDecisions:: $EDITORIALBOARD,
                    );
                }

                $form_array = _build_edit_decision_form(
                    $articleId, $topicId, $decisions, true);
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
            $rows = db_select('ebms_topic', 't')->condition('t.board_id',
                    $boardId)->fields('t')->execute()->fetchAll();
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


        $buttonId = $isAdd ? "add_decision_$hash" : "edit_decision_$hash";

        // check if related form is triggered
        $triggered = _is_triggered_by($form_state, $buttonId);

        if ($triggered) {
            // grab icon for button
            $iconPath = Ebms\ADD_DECISION_ACTIVE;
            // create add tag button
            $form['button'][$buttonId] = array(
                '#markup' => "<img class='full-citation-button image-button' src='$iconPath'>",
            );

            $formId = "edit_decision_form_$hash";
            $form['form'][$formId] = _build_edit_decision_form($articleId,
                $topicId, $decisions, $isAdd);
            $form['form'][$formId]['#prefix'] = '<div class="full-citation-form full-citation-indent">';
            $form['form'][$formId]['#suffix'] = '</div>';
        } else {
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
                ),
                '#attributes' => array(
                    'class' => array('full-citation-button', 'image-button'),
                    'title' => $isAdd ? 'Add Decisions' : 'Edit Decision',
                    'alt' => $isAdd ? 'Add Decisions' : 'Edit Decision',
                ),
            );
            $form['form'] = array();
        }

        return $form;
    }

    function _build_edit_decision_form($article_id, $topic_id, $decisions,
        $isAdd) {
        $form = array();

        if (empty($decisions))
            return $form;

        $hash = "$article_id|$topic_id";

        $ts = time();
        $date = format_date($ts, 'custom', 'm/d/Y');

        global $user;

        $timetag = "<div>$date - <i>$user->name</i></div>";

        foreach ($decisions as $decision) {
            $title = $decision->title();
            if (!$isAdd)
                $title = 'Edit Decision';
            $full_title = array('#markup' => "<div class='citation-subheader'>$title</div><div>$timetag</div>");

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

                    $packetIds = Ebms\getArticlePacketIds($article_id, $topic_id);
                    if (empty($packetIds))
                        break;

                    $values = Ebms\getReviewDispositions();

                    $options = array(0 => '');
                    foreach ($values as $value) {
                        $options[$value->value_id] = $value->value_name;
                    }
                    $form['member_responses_title'] = $full_title;

                    $form["member_responses_select_$hash"] = array(
                        '#type' => 'select',
                        '#options' => $options,
                    );

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
                        '#options' => $options,
                    );

                    $form["board_decision_discussed_$hash"] = array(
                        '#type' => 'radios',
                        '#options' => array('Y' => 'Yes', 'N' => 'No'),
                        '#default_value' => 'Y',
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

    function _edit_decision_save($form, &$form_state) {
        require_once('EbmsArticle.inc');

        $form_state['rebuild'] = TRUE;

        $values = _retrieve_hash_values($form_state,
            array(
            'initial_review' => array('pass', 'reject', 'comment'),
            'nci_reviewer' => array('pass', 'reject', 'comment'),
            'fulltext' => array('file', 'check', 'comment'),
            'surveillance' => array('pass', 'reject', 'fyi', 'comment'),
            'member_responses' => array('select', 'comment'),
            'manager_actions' => array('select', 'comment'),
            'on_agenda' => array('comment'),
            'board_decision' => array('select', 'discussed', 'members', 'comment'),
            ));

        $hash = $values['hash'];

        // split the hash into articleId and topicId
        list($articleId, $topicId) = explode("|", $hash);

        _save_initial_review_decision($values['initial_review'], $articleId,
            $topicId);
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

        Ebms\setArticleState($articleId, $state, $topicId, null, $comment);

        drupal_set_message("Added Initial Review decision to article $articleId");
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

        Ebms\setArticleState($articleId, $state, $topicId, null, $comment);

        drupal_set_message("Added NCI Reviewer decision to article $articleId");
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

        $validators = array('file_validate_extensions' => array());
        foreach ($_FILES['files']['name'] as $key => $value) {
            if ($value) {
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
        if ($pass)
            $state = \Ebms\EbmsArticleStates::RejectFullReview;
        else if ($fyi)
            $state = \Ebms\EbmsArticleStates::FYI;

        Ebms\setArticleState($articleId, $state, $topicId, null, $comment);

        drupal_set_message("Added Literature Surveillance Committee decision to article $articleId");
    }

    function _save_member_responses_decision($values, $articleId, $topicId) {
        if (empty($values))
            return;

        $select = $values['select'];

        if (!$select)
            return;

        $comment = $values['comment'];
        if (!$comment)
            $comment = null;
        $packets = Ebms\getArticlePacketIds($articleId, $topicId);

        if (sizeof($packets) != 1)
            return;

        $packetId = $packets[0];
        global $user;

        // Here are the fields we'll store
        //   article_id and dates handled separately for new
        //   and replace records
        $fields = array(
            'packet_id' => $packetId,
            'article_id' => $articleId,
            'reviewer_id' => $user->uid,
            'when_posted' => db_query('SELECT NOW()')->fetchField(),
            'comments' => $comment,
            'loe_info' => NULL,
        );

        // Wrap this in a transaction
        $txn = db_transaction();
        try {

            // Insert a new record
            $query = db_insert('ebms_article_review');
            $query->fields($fields);

            $queryString = (string) $query;

            $reviewId = $query->execute();

            // also insert into the review disposition table
            $reviewFields = array(
                'review_id' => $reviewId,
                'value_id' => $select,
            );

            // Insert a new record
            $query = db_insert('ebms_review_disposition');
            $query->fields($reviewFields);

            $queryString = (string) $query;

            $rowId = $query->execute();
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

        Ebms\setArticleState($articleId, $select, $topicId, null, $comment);

        drupal_set_message("Added Board Manager Action decision to article $articleId");
    }

    function _save_on_agenda_decision($values, $articleId, $topicId) {
        if (empty($values))
            return;

        $comment = $values['comment'];

        if (empty($comment))
            $comment = null;
        if ($comment == null)
            return;

        Ebms\setArticleState($articleId, Ebms\EbmsArticleStates::OnAgenda,
            $topicId, null, $comment);

        drupal_set_message("Added Board Manager Action decision to article $articleId");
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

        $articleStateId = Ebms\setArticleState($articleId,
            Ebms\EbmsArticleStates::FinalBoardDecision, $topicId, null, $comment);

        $key = array(
            'article_state_id' => $articleStateId,
            'decision_value_id' => $value_id,
        );
        $fields = array(
            'discussed' => $discussed,
        );

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
            if (in_array('board manager', $user->roles)) {
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
        if ($showButtons)
            $form['boards_header']['button'] = $button_array['button'];
        $form['boards_header']['title'] = array("#markup" => '<div class="citation-subheader full-citation-indent full-citation-element">Editorial Board(s)</div>');
        if ($showButtons)
            $form['boards_header']['form'] = $button_array['form'];

        foreach ($board_topic_states as $board_name => $topics) {
            $boardId = Ebms\getBoardIdByName($board_name);

            // create add topic button
            $button_array = _build_add_topic_button($form_state, $mode,
                $article->getId(), $boardId);
            if ($showButtons)
                $form[$board_name]['button'] = $button_array['button'];
            $form[$board_name]['fieldset'] = array(
                '#type' => 'fieldset',
                '#title' => $board_name,
                '#collapsible' => TRUE,
                '#collapsed' => TRUE,
                'topics' => _format_topics($form_state, $article, $topics,
                    $tags, $display_cycle, $mode),
                '#attributes' => array(
                    'class' => array('full-citation-element'),
                ),
                '#prefix' => '<div class="full-citation-indent">',
                '#suffix' => '</div>',
            );
            if (!$mode) {
                $form[$board_name]['fieldset']['#attributes']['class'][] = 'full-citation-topic';
            }
            if ($showButtons)
                $form[$board_name]['form'] = $button_array['form'];
        }

        return $form;
    }

    function _format_import(Ebms\EbmsArticle &$article, &$importData) {
        $form = array();

        $form['#prefix'] = '<div class="full-citation-element">';
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
     * Formats a topic name and set of topic states for rendering.
     * @param string $topic
     * @param mixed $topic_states
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
        $tag_array = _format_tags($form_state, $tags, $showButtons, $topicId);
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

        if ($mode == 'publishing') {
            $decisionInfo = Ebms\EbmsTopicDecisions::$PUBLISHED;
            $decision = _format_topic_publish($articleId, $topicId,
                $topic_sequences, $form_state);
            if (empty($decision)) {
                /* $decision = _format_topic_decision($decisionInfo,
                  $topic_sequences, $form_state, 'publishing'); */

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
        if ($mode == 'librarian') {
            $decisions = array();
            $decisionInfo = Ebms\EbmsTopicDecisions::$INITIALREVIEW;
            if ($showButtons) {
                $decision = _format_topic_decision($decisionInfo,
                    $topic_sequences, $form_state);
            } else {
                $decision = _format_topic_decision($decisionInfo,
                    $topic_sequences);
            }
            $decisions['topic_initial_review_decision'] = _wrap_decision($form_state,
                $articleId, $topicId, $decision, $decisionInfo, $empty_decisions);
        } else if ($mode == 'publishing') {
            
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

            $decisions['topic_assigned_for_review_decision'] = _format_topic_assigned_for_review_decision($articleId,
                $topicId);

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
                if (!empty($array)) {
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
     * @param type $form_state
     * @param type $articleId
     * @param type $topicId
     * @param type $decision
     * @param Ebms\EbmsTopicDecisions $decisionInfo
     * @param type $emptyDecisions
     * @return type
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

        $form['button'] = $buttonArray['button'];
        $form['decision'] = $decision;
        $form['form'] = $buttonArray['form'];

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

        $cycleNames = array();
        foreach ($cycles as $cycle) {
            $cycleNames[] = $cycle->cycle_name;
        }

        $reviewCycle = implode(', ', $cycleNames);

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

    function _format_topic_decision(Ebms\EbmsTopicDecisions $decisionInfo,
        $topic_sequences, &$form_state = null, $modifier = null) {
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

            if (!$show_past && $modifier != 'publishing') {
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

                    if ($form_state != null && $modifier != 'publishing') {
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

            $form['decisions'] = $decisions;
        }

        // also dig up the past decisions if the currently displayed set is current
        if (!$show_past && $modifier != 'publishing' && isset($topic_sequences[$sequenceIndex]['I'])) {
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

            $form['decision'] = array("#markup" => "<div class='full-citation-indent'>$icon$date - $account->name</div>");
            $form['comment'] = array('#markup' => "<div class='full-citation-indent'>COMMENTS: $comment</div>");
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

        $form['decision'] = array("#markup" => "<div class='full-citation-indent'>$icon$date - $account->name</div>");
        $form['filename'] = array("#markup" => "<div class='full-citation-indent'>FILE NAME: $filename</div>");

        return $form;
    }

    function _format_topic_assigned_for_review_decision($articleId, $topicId) {
        $form = array();

        $packetIds = Ebms\getArticlePacketIds($articleId, $topicId);

        if (empty($packetIds))
            return $form;

        // enclose the element
        $form['#prefix'] = '<div class="full-citation-element">';
        $form['#suffix'] = '</div>';

        // add the header
        $form['header'] = array('#markup' => "<div class='citation-subheader'>Assigned For Review</div>");
        $form['header']['#prefix'] = '<div class="full-citation-indent">';
        $form['header']['#suffix'] = '</div>';

        $form['blocks'] = _format_packet_ids($packetIds);
        $form['blocks']['#prefix'] = '<div class="full-citation-indent">';
        $form['blocks']['#suffix'] = '</div>';

        // grab the past packets, if any
        $pastIds = Ebms\getArticlePacketIds($articleId, $topicId, false, false,
            true);
        if (empty($pastIds))
            return $form;

        $form['past_blocks'] = array(
            '#type' => 'fieldset',
            '#title' => t('Past Decisions'),
            '#collapsible' => TRUE,
            '#collapsed' => TRUE,
            'tags' => _format_packet_ids($pastIds),
            '#attributes' => array(
                'class' => array('full-citation-past-items'),
            ),
            '#prefix' => '<div class="full-citation-indent">',
            '#suffix' => '</div>'
        );

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
            $form['blocks'] = $responseBlocks;
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
        $action = $response->value_name;
        $dt = $response->when_posted;
        $date = _format_date($dt);

        $account = user_load($uid);

        $img = _build_decision_icon_tag('pass');

        $form['decision'] = array("#markup" => "<div class='full-citation-indent'>$img$date - $account->name</div>");
        $form['filename'] = array("#markup" => "<div class='full-citation-indent'>ACTION: $action</div>");

        if (isset($response->comments)) {
            $form['comments'] = array("#markup" => "<div class='full-citation-indent'>COMMENTS: $response->comments</div>");
        }
        if (isset($response->loe_info) && $response->loe_info != 'Not Selected') {
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


        $showButtons = true;
        // hide buttons for the managers
        global $user;
        if (in_array('board manager', $user->roles)) {
            $showButtons = false;
        }

        // check if any sequences are past the publish sequence
        $pubSequence = Ebms\EbmsTopicDecisions::$PUBLISHED->sequence();
        $history = Ebms\getStatusHistory($articleId, null, $topicId, true,
            false, 'I');
        foreach ($history as $status) {
            if (!($status->type_sequence < $pubSequence))
                return $form;
        }

        $form['header']['title'] = array("#markup" => "<div class='citation-subheader'>Publication Date</div>");
        $form['header']['date'] = array("#markup" => "<i>Awaiting Publication</i>");
        $form['header']['#prefix'] = "<div class='left-side'>";
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

        $form['#prefix'] = "<div class='full-citation-indent'>";
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

        $ret = Ebms\setArticleState($articleId,
            Ebms\EbmsArticleStates::Published, $topicId);

        if ($ret) {
            $article = new Ebms\EbmsArticle($articleId);
            $articleTitle = $article->getArticleTitle();
            $topicName = Ebms\getTopicNameById($topicId);

            drupal_set_message("Successfully published article '$articleTitle' to topic '$topicName'");
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

    function _build_decision_icon_tag($type, $active = true) {

        $icon = null;

        switch ($type) {
            case 'pass':
                $icon = $active ? Ebms\PASS_ACTIVE : Ebms\PASS_INACTIVE;
                break;
            case 'reject':
                $icon = $active ? Ebms\REJECT_ACTIVE : Ebms\REJECT_INACTIVE;
                break;
            case 'fyi':
                $icon = $active ? Ebms\FYI_ACTIVE : Ebms\FYI_INACTIVE;
                break;
            default:
                return '';
        }

        return $prefix = "<img class='full-citation-decision-icon' src='$icon'>";
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
            $date = $decision->cycle_name;
            if (!$date)
                $date = '';
            $form['discussed'] = array("#markup" => "<div class='full-citation-indent'>ARTICLE DISCUSSED - $date</div>");
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

    function _ajax_full_citation($form, &$form_state) {

        return $form;
    }
}