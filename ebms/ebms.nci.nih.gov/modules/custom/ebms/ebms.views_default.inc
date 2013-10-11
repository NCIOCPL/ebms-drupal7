<?php

/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */

/**
 * Implements hook_views_default_views().
 */
function ebms_views_default_views() {
	$module = 'ebms';
	$directory = 'includes';
	$extension = 'view.inc';
	$name = 'view';

	// From workbench_load_all_exports().
	$return = array();
	// Find all the files in the directory with the correct extension.
	$files = file_scan_directory(drupal_get_path('module', $module) . "/$directory", "/.$extension/");
	foreach ($files as $path => $file) {
		require $path;
		if (isset($$name)) {
			$return[$$name->name] = $$name;
		}
	}

	return $return;
}
