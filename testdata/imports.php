<?php

require(__DIR__ . '/console-log.php');

// Find out where the data is.
$repo_base = getenv('REPO_BASE') ?: '/var/www/ebms';

$storage = \Drupal::entityTypeManager()->getStorage('ebms_article');
$query = $storage->getQuery();
$article_ids = $query->execute();
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
$query = $storage->getQuery();
$query->condition('vid', 'import_dispositions');
$terms = $storage->loadMultiple($query->execute());
$dispositions = [];
foreach ($terms as $term) {
  $key = $term->field_text_id->value;
  $dispositions[$key] = $term->id();
}
$json = file_get_contents("$repo_base/testdata/import_batches.json");
$batches = json_decode($json, true);
$count = 0;
foreach ($batches as $values) {
  $actions = [];
  foreach ($values['actions'] as $action) {
    $article_id = $action['article'];
    if (empty($article_id) || in_array($article_id, $article_ids)) {
      $action['disposition'] = $dispositions[$action['disposition']];
      $actions[] = $action;
    }
  }
  if (!empty($actions)) {
    $values['import_type'] = $types[$values['import_type']];
    $values['actions'] = $actions;
    $batch = \Drupal\ebms_import\Entity\Batch::create($values);
    $batch->save();
    $count++;
  }
}
log_success("Successfully loaded: $count import batches");
$path = "$repo_base/testdata/import-requests.json";
$requests = json_decode(file_get_contents($path), TRUE);
foreach ($requests as $request) {
  $report = json_decode($request['report'], TRUE);
  $report['import_type'] = $types[$report['import_type']];
  $actions = [];
  foreach ($report['actions'] as $action) {
    $action['disposition'] = $dispositions[$action['disposition']];
    $actions[] = $action;
  }
  $report['actions'] = $actions;
  $request['report'] = json_encode($report);
  $request['batch'] = $report['batch'] ?? NULL;
  $request = \Drupal\ebms_import\Entity\ImportRequest::create($request);
  $request->save();
}
$n = count($requests);
log_success("Successfully loaded: $n import requests");
