<?php

// $Id$

/**
 * @file
 *
 * Implementation of EBMS page for general site information.
 */
function pdq_ebms_citations($action = null) {

	require_once('EbmsArticle.inc');

	// grab the resource stylesheet
	$css_path = drupal_get_path('theme', 'ebmstheme');
	$css_path .= "/css/citations.css";
	drupal_add_css($css_path, array('type' => 'file'));

	Ebms\Menu::$active = 'Citation Management';
	drupal_set_title('Citation Management');
	$crumbs = array(
		$action ? l('Citation Management', 'citations') : 'Citation Management'
	);
	$result = null;
	switch ($action) {
		case 'review':
			$crumbs[] = 'Review Citations';
			break;
		case 'not-list':
			$crumbs[] = "Not List Maintenance";
			$form = drupal_get_form('pdq_ebms_citations_not_list');


			$result['left_nav'] = \Ebms\Util::build_librarian_left_nav('Not List Maintentance');

			$result['not_list_form'] = $form;

			break;
		case null:
			$form = drupal_get_form('pdq_ebms_citation_queue');


			$result['left_nav'] = \Ebms\Util::build_librarian_left_nav();

			$result['cite_queue_form'] = $form;

			break;
	}
	drupal_set_breadcrumb($crumbs);
	if ($result)
		return $result;

	return "<i>Stub for 'Citations' page, action = $action</i>";
}

function pdq_ebms_citations_not_list($form, &$form_state) {

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

		$form['citation-results'] = pdq_ebms_not_list_results(array(), $form_state, $board, $brief_title, $full_title, $id, $all_journals);
	}

	return $form;
}

function pdq_ebms_citations_not_list_submit($form, &$form_state) {

	//dsm($form_state, 'form state');

	$form_state['rebuild'] = true;

	// always update the changed list
	pdq_ebms_update_journal_select($form_state);

	$clear_states = false;

	if (isset($form_state['values']['op'])) {
		$op = $form_state['values']['op'];
		if (!empty($form_state['values']['submit_source'])) {
			$op = $form_state['values']['submit_source'];
		}

		//dsm("op = $op");

		switch ($op) {
			case 'Reset':
				$form_state['rebuild'] = false;
				return;
			case 'Save':
				// write out the changes to the not list, in terms of
				// inserts and deletes to the DB
				pdq_ebms_write_not_list($form_state['altered_states']);

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

	//dsm($form_state['altered_states'], 'altered states');
	// wipe out button values so that the default_states calculated from
	// altered states can take effect
	pdq_ebms_clear_journal_select($form_state['input'], 'input');
	//pdq_ebms_clear_journal_select($form_state['values'], 'values');
}

// parse the values from the form state, keep the journals that have changed
// from their initial state, and discard those that are unchanged.
function pdq_ebms_update_journal_select(&$form_state) {
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
				//dsm("Found state $value at end of hash $hash");
				unset($altered_states[$hash]);
			} else {
				//dsm("Did not find state $value at end of hash $hash");
				$altered_states[$hash] = $value;
			}
		}
	}

	$form_state['altered_states'] = $altered_states;
}

function pdq_ebms_clear_journal_select(&$target, $desc) {
	$button_keys = array();
	foreach ($target as $key => $value) {
		if (strpos($key, 'not_list_state_') === 0) {
			$button_keys[$key] = $value;
		}
	}

	//dsm($target, "$desc initial keys");

	$diff = array_diff_key($target, $button_keys);

	//dsm($button_keys, "$desc check keys");
	//dsm($diff, "$desc final keys");
	$target = $diff;
}

function pdq_ebms_write_not_list($values) {
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

function pdq_ebms_not_list_results($form, &$form_state, $board, $brief_title, $full_title, $id, $all_journals) {

	// if all_journals is set, then a right-join needs to be performed

	$query = db_select('ebms_not_list', 'nl');
	if ($all_journals) {
		$query->rightJoin('ebms_journal', 'j', 'j.source = nl.source
				AND j.source_jrnl_id = nl.source_jrnl_id
				AND nl.board_id = :board', array(':board' => $board));
	} else {
		$query->join('ebms_journal', 'j', 'nl.source_jrnl_id = j.source_jrnl_id AND nl.source = j.source');
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
		->fields('j', array('brf_jrnl_title', 'jrnl_title', 'source_jrnl_id', 'source'))
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
		"<div class='citation-cell not-check right-column'>Not List</div>";


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
			'#prefix' => "<div class='citation-cell not-check right-column'>",
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

function pdq_ebms_citation_queue($form, &$form_state) {

	$form_size = sizeof($form_state);

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
				$result = db_select('ebms_topic', 't')->condition('board_id', $board_id)->fields('t')->execute();
			} catch (Exception $e) {
				dsm($e, 'topic query exception!');
			}
			foreach ($result as $record) {
				$form['citation_queue']['topic_select']['#options'][$record->topic_id] = $record->topic_name;
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

	pdq_ebms_citation_queue_results($form, $form_state);

	return $form;
}

function _ajax_citation_queue_board_select($form, $form_state) {
	return $form['citation_queue']['topic_select'];
}

function pdq_ebms_citation_queue_results(&$form, &$form_state) {

	$board_id = 0;
	$topic_id = 0;
	$journal_id = '';

	// get values if they exist
	if (isset($form_state['values'])) {
		$board_id = $form_state['values']['board_select'];
		$topic_id = $form_state['values']['topic_select'];
		$journal_id = $form_state['values']['journal_id_field'];
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
		$query->join('ebms_last_article_topic_state', 'l', 'l.article_state_id = s.article_state_id');
		if ($journal_id)
			$query->join('ebms_article', 'a', 'a.article_id = s.article_id');

		$state_id = Ebms\getStateIdByName('Ready for initial review');
		$query->condition('s.state_id', $state_id);
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

		// TODO: add the count back once the query is faster!
		$num_rows = $query->countQuery()->execute()->fetchField();
		//$num_rows = 10;

		$query
			->fields('s', array('article_id'))
			->limit(10);
		$query->addExpression('GROUP_CONCAT(s.topic_id)', 'topics');

		$result = $query->execute();
	} catch (Exception $e) {
		dsm('error with query');
		$num_rows = -1;
		$result = array();
	}

	$results['results-title']['#markup'] = "<h2>Citations Requiring Review <span class = 'results-count'>($num_rows)</span></h2>";

	/* $results['buttons'] = array(
	  '#prefix' => '<div id="citation-buttons-div">',
	  '#suffix' => '</div>',
	  );

	  $results['buttons']['save_button'] = array(
	  '#type' => 'submit',
	  '#value' => 'Save',
	  );

	  $results['buttons']['cancel_button'] = array(
	  '#type' => 'submit',
	  '#value' => 'Cancel',
	  ); */

	$results['top-pager'] = array(
		'#theme' => 'pager',
		'#attributes' => array(
			'class' => 'top-pager'
		));

	$results['table'] = array(
		'#prefix' => '<div id="citation-table-div">',
		'#suffix' => '</div>',
	);

	$results['table']['header'] = array(
		'#prefix' => "<div class='citation-header'>",
		'#suffix' => '</div>',
	);

	$results['table']['header'][]['#markup'] =
		"<div class='citation-cell brief-citation left-column'>Citations</div>";
	$results['table']['header'][]['#markup'] =
		"<div class='citation-cell board'>Board</div>";
	$results['table']['header'][]['#markup'] =
		"<div class='citation-cell summary-topic'><div class='summary-topic-name'>Summary Topic</div><div class='summary-topic-check'>Reject</div><div class='summary-topic-check'>Pass</div></div>";


	$ix = 0;
	$results['table']['rows'] = array();
	foreach ($result as $record) {
		$results['table']['rows'][$ix] = array(
			'#prefix' => "<div class='citation-row'>",
			'#suffix' => '</div>',
		);

		$row = &$results['table']['rows'][$ix];

		$article = new Ebms\EbmsArticle($record->article_id);

		//$authors = implode(', ', $article->getAuthors());
		//$title = $article->getArticleTitle();

		$cite = cite_article($record->article_id, $ix);

		$row['brief_citation']['#markup'] =
			"<div class='citation-cell brief-citation left-column'>$cite</div>";

		// retrieve the topics and related boards
		$curr_topics = explode(',', $record->topics);

		$topics = array();
		foreach ($article->getTopics() as $topic) {
			if (in_array($topic->topic_id, $curr_topics)) {
				$boards[$topic->board_name][$topic->topic_name] = array();
			}
		}

		//dsm($boards, 'current article topics');

		$row['brief_citation']['#markup'] =
			"<div class='citation-cell brief-citation left-column'>$cite</div>";

		// build the board->id cell
		$output = '';
		foreach ($boards as $board_name => $topics) {
			$output .= "<div class='citation-cell board'>$board_name</div><div class='citation-cell summary-topic'>";
			foreach ($topics as $topic_name => $topic) {
				$output .= "<div class='summary-topic-line'><div class='summary-topic-name'>$topic_name</div><div class='summary-topic-check'>Reject</div><div class='summary-topic-check'>Pass</div></div>";
			}
			$output .= '</div>';
		}

		$row['board_topic']['#markup'] = $output;

		//"<div class='citation-cell brief-citation left-column'>$brfCite<br><br><i>$title</i></div>";
		/* $results['table']['header'][]['#markup'] =
		  "<div class='citation-cell board'>Board</div>";
		  $results['table']['header'][]['#markup'] =
		  "<div class='citation-cell summary-topic'>Summary Topic</div>";
		  $results['table']['header'][]['#markup'] =
		  "<div class='citation-cell reject-check'>Reject</div>";
		  $results['table']['header'][]['#markup'] =
		  "<div class='citation-cell pass-check right-cell'>Pass</div>";

		  /* $results['table']['rows'][$ix]['brief_title']['#markup'] =
		  "<div class='citation-cell brief-title'>$record->brf_jrnl_title</div>";
		  $results['table']['rows'][$ix]['full_title']['#markup'] =
		  "<div class='citation-cell full-title'>$record->jrnl_title</div>";
		  $results['table']['rows'][$ix]['journal_id']['#markup'] =
		  "<div class='citation-cell id'>$record->source_jrnl_id</div>";

		  $initial = $record->default_value;
		  $journal_hash = "$record->source|$record->source_jrnl_id|$board|$initial";
		  $default = $initial;
		  if (isset($form_state['altered_states'][$journal_hash])) {
		  $default = $form_state['altered_states'][$journal_hash];
		  }

		  $results['table']['rows'][$ix]["not_list_state_$journal_hash"] = array(
		  '#type' => 'checkbox',
		  '#title' => "&nbsp;",
		  '#default_value' => $default,
		  '#prefix' => "<div class='citation-cell not-check'>",
		  '#suffix' => "</div>",
		  //'#return_value' => ,
		  //'#attributes' => array('name' => "search_row_${ix}_button"),
		  ); */

		$ix++;
	}

	$results['bottom-pager'] = array(
		'#theme' => 'pager',
		'#attributes' => array(
			'class' => 'top-pager'
		));
}

function pdq_ebms_citation_queue_submit($form, &$form_state) {

	$form_state['rebuild'] = true;

	if (isset($form_state['values']['op'])) {
		$op = $form_state['values']['op'];
		if (!empty($form_state['values']['submit_source'])) {
			$op = $form_state['values']['submit_source'];
		}

		dsm("op = $op");

		switch ($op) {
			case 'Reset':
				$form_state['rebuild'] = false;
				return;
			/* case 'Save':
			  // write out the changes to the not list, in terms of
			  // inserts and deletes to the DB
			  pdq_ebms_write_not_list($form_state['altered_states']);

			  $clear_states = true;

			  break; */
			case 'Search':
			case 'Cancel':
				//$clear_states = true;
				break;
		}
	}

	/* if ($clear_states)
	  $form_state['altered_states'] = array();

	  //dsm($form_state['altered_states'], 'altered states');
	  // wipe out button values so that the default_states calculated from
	  // altered states can take effect
	  pdq_ebms_clear_journal_select($form_state['input'], 'input');
	  //pdq_ebms_clear_journal_select($form_state['values'], 'values'); */
}

function cite_article($article_id, $position) {
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
	$pieces = array(
		'<div class="article-citation"><div>',
		implode(', ', $authors),
		"</div><div><em>$title</em></div><div>$ta $vol",
	);
	if ($issue)
		$pieces[] = "($issue)";
	if ($pgn)
		$pieces[] = ": $pgn";
	if ($year)
		$pieces[] = ", $year";
	$pieces[] = ".</div><div>PMID: $pmid</div><div>";
	$pieces[] = '<a href="/ebmsdev/stub">VIEW ABSTRACT</a> | ';
	$pieces[] = '<a href="/ebmsdev/stub">DOWNLOAD FULL TEXT</a>';
	$pieces[] = '</div></div>';
	return implode($pieces);
	#$results = db_select('ebms_article', 'a')
	#    ->fields('a', array('article_title', 'source_id',
	#return "Article $position (CMS ID $article_id)";
}