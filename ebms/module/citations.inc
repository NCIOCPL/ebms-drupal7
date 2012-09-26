<?php

// $Id$

/**
 * @file
 *
 * Implementation of EBMS page for general site information.
 */
function pdq_ebms_citations($action = null) {
	Ebms\Menu::$active = 'Citation Management';
	drupal_set_title('Citation Management');
	$crumbs = array(
		$action ? l('Citation Management', 'citations') : 'Citation Management'
	);
	$form_id = null;
	switch ($action) {
		case 'review':
			$crumbs[] = 'Review Citations';
			break;
		case 'not-list':
			$crumbs[] = "Not List Maintenance";
			$form_id = 'pdq_ebms_citations_not_list';
			break;
	}
	drupal_set_breadcrumb($crumbs);
	if ($form_id)
		return drupal_get_form($form_id);

	return "<i>Stub for 'Citations' page, action = $action</i>";
}

function pdq_ebms_citations_not_list($form, &$form_state) {
	// retrieve boards
	$boards = Ebms\Util::boards();

	//db_select('ebms_not_list', 'nl')->join($table, $alias);
	$form['#prefix'] = '<div id="not_list_form_div">';
	$form['#suffix'] = '</div>';
	$form['not-list']['description']['#markup'] = '<h2>Not List Maintenance</h2>';
	$form['not-list']['board_select'] = array(
		'#type' => 'select',
		'#title' => 'Editorial Board',
		'#title_display' => 'after',
		'#options' => array(0 => t('All Boards')),
		'#prefix' => '<div class="not_list_input_div">',
		'#suffix' => '</div>',
	);
	$form['not-list']['board_select']['#options'] += $boards;
	$form['not-list']['brief_title_field'] = array(
		'#type' => 'textfield',
		'#title' => 'Brief Journal Title',
		'#title_display' => 'after',
		'#prefix' => '<div class="not_list_input_div">',
		'#suffix' => '</div>',
	);
	$form['not-list']['full_title_field'] = array(
		'#type' => 'textfield',
		'#title' => 'Full Journal Title',
		'#title_display' => 'after',
		'#prefix' => '<div class="not_list_input_div">',
		'#suffix' => '</div>',
	);
	$form['not-list']['journal_id_field'] = array(
		'#type' => 'textfield',
		'#title' => 'Journal ID',
		'#title_display' => 'after',
		'#prefix' => '<div class="not_list_input_div">',
		'#suffix' => '</div>',
	);
	$form['not-list']['all_journals_check'] = array(
		'#type' => 'checkbox',
		'#title' => 'All Journals',
		'#title_display' => 'after',
		'#prefix' => '<div class="not_list_input_div">',
		'#suffix' => '</div>',
	);
	$form['not-list']['search_button'] = array(
		'#type' => 'button',
		'#value' => 'Search',
		'#ajax' => array(
			'callback' => 'pdq_ebms_ajax_search_submit',
			'wrapper' => 'not_list_results_div',
			'method' => 'replace',
		),
	);
	$form['not-list']['reset_button'] = array(
		'#type' => 'button',
		'#value' => 'Reset',
		'#ajax' => array(
			'callback' => 'pdq_ebms_ajax_clear_form',
			'wrapper' => 'not_list_form_div',
			'method' => 'replace',
		),
	);
	$form['not-list']['not-list-results'] = array(
		'#type' => 'container',
		'#prefix' => '<div id="not_list_results_div">',
		'#suffix' => '</div>',
	);

	//$form['not-list']['debug-form']['#markup'] = kpr($form, TRUE, 'form');
	//$form['not-list']['debug-form-state']['#markup'] = kpr($form_state, TRUE, 'form');

	return $form;
}

function pdq_ebms_ajax_clear_form($form, &$form_state) {
	$form_state['input'] = array();
	$form_state['values'] = array();
	$form_state['rebuild'] = TRUE;

	/* $form['not-list']['not-list-results']['form_debug']['#markup'] =
	  kpr($form, true, 'form');
	  $form['not-list']['not-list-results']['form_state_debug']['#markup'] =
	  kpr($form_state, true, 'form_state'); */

	return drupal_rebuild_form($form_state['build_info']['form_id'], $form_state);
}

function pdq_ebms_ajax_search_submit($form, &$form_state) {
	/* $form['not-list']['not-list-results']['form_state_debug']['#markup'] =
	  kpr($form_state['values'], true, 'values'); */

	$board = $form_state['values']['board_select'];
	$brief_title = $form_state['values']['brief_title_field'];
	$full_title = $form_state['values']['full_title_field'];
	$id = $form_state['values']['journal_id_field'];
	$all_journals = $form_state['values']['all_journals_check'];

	$query = db_select('ebms_not_list', 'nl');
	$query->join('ebms_journal', 'j', 'nl.source_jrnl_id = j.source_jrnl_id AND nl.source = j.source');
	$query = $query->extend('PagerDefault');

	if ($board != 0)
		$query->condition('nl.board_id', $board);
	else{
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
		$query->condition('j.source_jrnl_id', $id);

	$query
		->fields('j', array('brf_jrnl_title', 'jrnl_title', 'source_jrnl_id'))
		->limit(10);

	/* $form['not-list']['not-list-results']['query_debug']['#markup'] =
	  (string) $query; //kpr($query, true, 'query'); */

	$result = $query->execute();

	/* $form['not-list']['not-list-results']['results_debug']['#markup'] =
	  kpr($result, true, 'result'); */

	$form['not-list']['not-list-results']['top-pager'] = array('#theme' => 'pager');

	$header = '';
	$header .= "<div class='not-list-header'>
		<div class='not-list-cell brief-title'>Brief Journal Title</div>
		<div class='not-list-cell full-title'>Full Journal Title</div>
		<div class='not-list-cell id'>Journal ID</div>
		<div class='not-list-cell not-check'>Not List</div>
		</div>";

	$form['not-list']['not-list-results']['header']['#markup'] = $header;

	$ix = 0;
	$form['not-list']['not-list-results']['rows'] = array();
	foreach ($result as $record) {
		$row = "<div class='not-list-row'>
				<div class='not-list-cell brief-title'>$record->brf_jrnl_title</div>
				<div class='not-list-cell full-title'>$record->jrnl_title</div>
				<div class='not-list-cell id'>$record->source_jrnl_id</div>
				<div class='not-list-cell not-check'></div>
				</div>";

		$form['not-list']['not-list-results']['rows'][$ix++]['#markup'] = $row;
	}

	$form['not-list']['not-list-results']['bottom-pager'] = array('#theme' => 'pager');

	/* $form['not-list']['not-list-results']['records_debug']['#markup'] =
	  kpr($records, true, 'records'); */




	return $form['not-list']['not-list-results'];
}