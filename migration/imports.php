<?php

require(__DIR__ . '/console-log.php');

// Find out where the data is.
$repo_base = getenv('REPO_BASE') ?: '/var/www/ebms';

// Load the mappings for the import type values.
$storage = \Drupal::entityTypeManager()->getStorage('taxonomy_term');
$query = $storage->getQuery();
$query->condition('vid', 'import_types');
$ids = $query->execute();
$terms = $storage->loadMultiple($ids);
$types = [];
foreach ($terms as $term) {
  $key = $term->field_text_id->value;
  $types[$key] = $term->id();
}

// Load the mappings for the import disposition values.
$query = $storage->getQuery();
$query->condition('vid', 'import_dispositions');
$terms = $storage->loadMultiple($query->execute());
$dispositions = [];
foreach ($terms as $term) {
  $key = $term->field_text_id->value;
  $dispositions[$key] = $term->id();
}

// Load the import batch entities.
$start = microtime(TRUE);
$count = 0;
$fp = fopen("$repo_base/unversioned/exported/import_batches.json", 'r');
while (($line = fgets($fp)) !== FALSE) {
  $values = json_decode($line, TRUE);
  $actions = [];
  if (!empty($values['actions'])) {
    foreach ($values['actions'] as $action) {
      $action['disposition'] = $dispositions[$action['disposition']];
      $actions[] = $action;
    }
  }
  $values['import_type'] = $types[$values['import_type']];
  $values['actions'] = $actions;
  $batch = \Drupal\ebms_import\Entity\Batch::create($values);
  $batch->save();
  $count++;
}
$elapsed = round(microtime(TRUE) - $start);
log_success("Successfully loaded: $count import batches", $elapsed);

// Load the import request entities.
$count = 0;
$start = microtime(TRUE);
$fp = fopen("$repo_base/unversioned/exported/import_requests.json", 'r');
while (($line = fgets($fp)) !== FALSE) {
  $values = json_decode($line, TRUE);
  $report = json_decode($values['report'], TRUE);
  $report['import_type'] = $types[$report['import_type']];
  $actions = [];
  if (!empty($report['actions'])) {
    foreach ($report['actions'] as $action) {
      $action['disposition'] = $dispositions[$action['disposition']];
      $actions[] = $action;
    }
  }
  $report['actions'] = $actions;
  $values['report'] = json_encode($report);
  $values['batch'] = $report['batch'] ?? NULL;
  $request = \Drupal\ebms_import\Entity\ImportRequest::create($values);
  $request->save();
  $count++;
}
$elapsed = round(microtime(TRUE) - $start);
log_success("Successfully loaded: $count import requests", $elapsed);
