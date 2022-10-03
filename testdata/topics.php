<?php

require(__DIR__ . '/console-log.php');

// Find out where the data is.
$repo_base = getenv('REPO_BASE') ?: '/var/www/ebms';

$json = file_get_contents("$repo_base/testdata/topics.json");
$data = json_decode($json, true);
$json = file_get_contents("$repo_base/testdata/maps.json");
$groups = json_decode($json, true)['topic_groups'];
foreach ($data as $row) {
  $values = [
    'id' => $row['id'],
    'name' => $row['name'],
    'board' => $row['board'],
    'nci_reviewer' => $row['reviewer'],
    'active' => $row['status'],
  ];
  if (!empty($row['group']))
    $values['topic_group'] = $groups[$row['group']];
  $topic = \Drupal\ebms_topic\Entity\Topic::create($values);
  $topic->save();
}
$n = count($data);
log_success("Successfully loaded: $n topics");
