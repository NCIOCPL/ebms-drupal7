<?php

require(__DIR__ . '/console-log.php');

// Find out where the data is.
$repo_base = getenv('REPO_BASE') ?: '/var/www/ebms';

// Get the mappings we'll need.
$start = microtime(TRUE);
$json = file_get_contents("$repo_base/migration/maps.json");
$maps = json_decode($json, TRUE);
$meeting_categories = [
  'Board' => 'board',
  'Subgroup' => 'subgroup',
];
$meeting_statuses = [
  'Scheduled' => 'scheduled',
  'Canceled' => 'cancelled',
];
$meeting_types = [
  'In Person' => 'in_person',
  'Webex/Phone Conf.' => 'remote',
];
$meeting_maps = [];
$storage = \Drupal::entityTypeManager()->getStorage('taxonomy_term');
$query = $storage->getQuery();
$query->condition('vid', 'meeting_categories');
$ids = $query->execute();
$terms = $storage->loadMultiple($ids);
foreach ($terms as $term) {
  $key = $meeting_categories[$term->name->value];
  $meeting_maps['meeting_categories'][$key] = $term->id();
}
$query = $storage->getQuery();
$query->condition('vid', 'meeting_statuses');
$ids = $query->execute();
$terms = $storage->loadMultiple($ids);
foreach ($terms as $term) {
  $key = $meeting_statuses[$term->name->value];
  $meeting_maps['meeting_statuses'][$key] = $term->id();
}
$query = $storage->getQuery();
$query->condition('vid', 'meeting_types');
$ids = $query->execute();
$terms = $storage->loadMultiple($ids);
foreach ($terms as $term) {
  $key = $meeting_types[$term->name->value];
  $meeting_maps['meeting_types'][$key] = $term->id();
}

// Load the meetings.
$n = 0;
$fp = fopen("$repo_base/migration/exported/meetings.json", 'r');
while (($line = fgets($fp)) !== FALSE) {
  $values = json_decode($line, TRUE);
  if (!empty($values['category'])) {
    $category = $meeting_maps['meeting_categories'][$values['category']];
    $values['category'] = $category;
  }
  if (!empty($values['type']))
    $values['type'] = $meeting_maps['meeting_types'][$values['type']];
  if (!empty($values['status']))
    $values['status'] = $meeting_maps['meeting_statuses'][$values['status']];
  $groups = [];
  if (!empty($values['groups']['ad_hoc_groups'])) {
    foreach ($values['groups']['ad_hoc_groups'] as $group)
      $groups[] = $maps['ad_hoc_groups'][$group];
  }
  if (!empty($values['groups']['subgroups'])) {
    foreach ($values['groups']['subgroups'] as $group)
      $groups[] = $maps['subgroups'][$group];
  }
  $values['groups'] = empty($groups) ? null : $groups;
  $meeting = \Drupal\ebms_meeting\Entity\Meeting::create($values);
  $meeting->save();
  $n++;
}
$elapsed = round(microtime(TRUE) - $start);
log_success("Successfully loaded: $n meetings", $elapsed);
