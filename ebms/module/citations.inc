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

			$result['debug_area'] = array(
				'#prefix' => '<div id="not-list-debug-div">',
				'#suffix' => '</div>',
			);

			$result['not_list_form'] = $form;
			$result['not_list_result'] = array(
				'#prefix' => '<div id="not-list-results-div">',
				'#suffix' => '</div>',
			);
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
		'#options' => array(0 => t('All Boards')),
		'#prefix' => '<div class="not-list-input-div">',
		'#suffix' => '</div>',
		//'#default_value' => $board,
	);
	$form['not-list']['board_select']['#options'] += $boards;
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

	$form_state['rebuild'] = true;
	
	// always update the changed list

	if (isset($form_state['values']['op'])) {
		switch ($form_state['values']['op']) {
			case 'Reset':
				$form_state['rebuild'] = false;
				return;
			case 'Save':
				$values = array();
				// find the currently set button values, and generate a true/false
				// map of value to checked state
				foreach ($form_state['values'] as $key => $value) {
					if (strpos($key, 'not_list_state_') === 0) {
						$hash = str_ireplace('not_list_state_', '', $key);
						$values[$hash] = $value;
					}
				}

				pdq_ebms_update_not_list($values);

				break;
		}
	}
}

function pdq_ebms_update_not_list($values, $form_state) {
	// need to use the mapped values to insert / delete rows in ebms_not_list
	//array('source' => , 'source_jrnl_id' => , 'board_id')
	dsm($values, 'update values');
	// loop though the values, explode and check for alterations
	$changed = array(1 => array(), 0 => array());
	foreach ($values as $value => $state) {
		list($source, $id, $board, $initial) = explode('|', $value);
		if ($initial != $state) {

			// add to added or removed based on state
			if (!isset($changed[$state][$board]))
				$changed[$state][$board] = array();
			$changed[$state][$board][] = array(
				'source' => $source,
				'id' => $id,
			);
		}
	}

	dsm($changed, 'changed journals');
}

function pdq_ebms_ajax_save_list($form, &$form_state) {
	dsm($form_state, 'save form_state');

	//$form['debug']['#markup'] = kpr($form_state, true, "form state debug");
	//return kpr($form_state, true, "form state debug");//$form;
	/* $form_state['input'] = array();
	  $form_state['values'] = array();
	  $form_state['rebuild'] = TRUE;

	  return drupal_rebuild_form($form_state['build_info']['form_id'], $form_state); */
}

function pdq_ebms_ajax_clear_form($form, &$form_state) {
	$form_state['input'] = array();
	$form_state['values'] = array();
	$form_state['rebuild'] = TRUE;

	return drupal_rebuild_form($form_state['build_info']['form_id'], $form_state);
}

function pdq_ebms_ajax_search_submit($form, &$form_state) {
	$board = $form_state['values']['board_select'];
	$brief_title = $form_state['values']['brief_title_field'];
	$full_title = $form_state['values']['full_title_field'];
	$id = $form_state['values']['journal_id_field'];
	$all_journals = $form_state['values']['all_journals_check'];

	return drupal_get_form('pdq_ebms_not_list_results', $board, $brief_title, $full_title, $id, $all_journals);
}

function pdq_ebms_not_list_results($form, &$form_state, $board, $brief_title, $full_title, $id, $all_journals) {
	$query = db_select('ebms_not_list', 'nl');
	$query->join('ebms_journal', 'j', 'nl.source_jrnl_id = j.source_jrnl_id AND nl.source = j.source');
	$query = $query->extend('PagerDefault');

	if ($board != 0)
		$query->condition('nl.board_id', $board);
	else {
		$query->groupBy('j.source');
		$query->groupBy('j.source_jrnl_id');
		$query->groupBy('j.jrnl_title');
		$query->groupBy('j.brf_jrnl_title');
	}

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

		$initial = 1;
		$journal_hash = "$record->source|$record->source_jrnl_id|$board|$initial";
		$form['table']['rows'][$ix]["not_list_state_$journal_hash"] = array(
			'#type' => 'checkbox',
			'#title' => "&nbsp;",
			'#default_value' => true,
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