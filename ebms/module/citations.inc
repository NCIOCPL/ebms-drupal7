<?php

// $Id$

/**
 * @file
 *
 * Implementation of EBMS page for general site information.
 */
function pdq_ebms_citations($action = null) {

	// grab the resource stylesheet
	$css_path = drupal_get_path('theme', 'ebmstheme');
	$css_path .= "/css/not_list.css";
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
	}
	drupal_set_breadcrumb($crumbs);
	if ($result)
		return $result;

	return "<i>Stub for 'Citations' page, action = $action</i>";
}

function pdq_ebms_citations_not_list($form, &$form_state) {

	//include js to redirect pager links
	$ret = drupal_add_js(EBMS\JS_DIR . '/not_list.js', 'file');

	// retrieve boards
	$boards = Ebms\Util::boards();

	//db_select('ebms_not_list', 'nl')->join($table, $alias);
	$form['#prefix'] = '<div id="not-list-div">';
	$form['#suffix'] = '</div>';

	$form['not-list']['#prefix'] = '<div id="not-list-form-div">';
	$form['not-list']['#suffix'] = '</div>';

	$form['not-list']['description']['#markup'] = '<h2>Not List Maintenance</h2>';
	$form['not-list']['board_select'] = array(
		'#type' => 'select',
		'#title' => 'Editorial Board',
		'#title_display' => 'after',
		'#options' => $boards, //array(0 => t('All Boards')),
		'#prefix' => '<div class="not-list-input-div">',
		'#suffix' => '</div>',
		//'#default_value' => $board,
	);
	//$form['not-list']['board_select']['#options'] += $boards;
	$form['not-list']['brief_title_field'] = array(
		'#type' => 'textfield',
		'#title' => 'Brief Journal Title',
		'#title_display' => 'after',
		'#prefix' => '<div class="not-list-input-div">',
		'#suffix' => '</div>',
		//'#default_value' => $brief_title,
	);
	$form['not-list']['full_title_field'] = array(
		'#type' => 'textfield',
		'#title' => 'Full Journal Title',
		'#title_display' => 'after',
		'#prefix' => '<div class="not-list-input-div">',
		'#suffix' => '</div>',
		//'#default_value' => $full_title,
	);
	$form['not-list']['journal_id_field'] = array(
		'#type' => 'textfield',
		'#title' => 'Journal ID',
		'#title_display' => 'after',
		'#prefix' => '<div class="not-list-input-div">',
		'#suffix' => '</div>',
		//'#default_value' => $id,
	);
	$form['not-list']['all_journals_check'] = array(
		'#type' => 'checkbox',
		'#title' => 'All Journals',
		'#title_display' => 'after',
		'#prefix' => '<div class="not-list-input-div">',
		'#suffix' => '</div>',
		//'#default_value' => $all_journals,
	);

	$form['not-list']['search_button'] = array(
		'#type' => 'submit',
		'#value' => 'Search',
		/* '#ajax' => array(
		  'callback' => 'pdq_ebms_ajax_search_submit',
		  'wrapper' => 'not-list-results-div',
		  'method' => 'replace',
		  ), */
	);

	$form['not-list']['reset_button'] = array(
		'#type' => 'submit',
		'#value' => 'Reset',
		/* '#ajax' => array(
		  'callback' => 'pdq_ebms_ajax_clear_form',
		  'wrapper' => 'not-list-div',
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

		$form['not-list-results'] = pdq_ebms_not_list_results(array(), $form_state, $board, $brief_title, $full_title, $id, $all_journals);
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
					$query = db_insert('ebms_not_list')
						->fields($record)
						->execute();
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
		'#prefix' => '<div id="not-list-results-div">',
		'#suffix' => '</div>',
	);

	$form['results-title']['#markup'] = "<h2>Not List Search Results <span class = 'results-count'>($num_rows)</span></h2>";

	$form['buttons'] = array(
		'#prefix' => '<div id="not-list-buttons-div">',
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
		  'wrapper' => 'not-list-results-div',
		  'method' => 'replace',
		  ), */
	);

	$form['top-pager'] = array(
		'#theme' => 'pager',
		'#attributes' => array(
			'class' => 'top-pager'
		));

	$form['table'] = array(
		'#prefix' => '<div id="not-list-table-div">',
		'#suffix' => '</div>',
	);

	$form['table']['header'] = array(
		'#prefix' => "<div class='not-list-header'>",
		'#suffix' => '</div>',
	);

	$form['table']['header'][]['#markup'] =
		"<div class='not-list-cell brief-title'>Brief Journal Title</div>";
	$form['table']['header'][]['#markup'] =
		"<div class='not-list-cell full-title'>Full Journal Title</div>";
	$form['table']['header'][]['#markup'] =
		"<div class='not-list-cell id'>Journal ID</div>";
	$form['table']['header'][]['#markup'] =
		"<div class='not-list-cell not-check'>Not List</div>";


	$ix = 0;
	$form['table']['rows'] = array();
	foreach ($result as $record) {
		$form['table']['rows'][$ix] = array(
			'#prefix' => "<div class='not-list-row'>",
			'#suffix' => '</div>',
		);

		$form['table']['rows'][$ix]['brief_title']['#markup'] =
			"<div class='not-list-cell brief-title'>$record->brf_jrnl_title</div>";
		$form['table']['rows'][$ix]['full_title']['#markup'] =
			"<div class='not-list-cell full-title'>$record->jrnl_title</div>";
		$form['table']['rows'][$ix]['journal_id']['#markup'] =
			"<div class='not-list-cell id'>$record->source_jrnl_id</div>";

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
			'#prefix' => "<div class='not-list-cell not-check'>",
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