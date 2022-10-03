<?php

require(__DIR__ . '/console-log.php');

// Find out where the data is.
$repo_base = getenv('REPO_BASE') ?: '/var/www/ebms';

$json = file_get_contents("$repo_base/migration/maps.json");
$groups = json_decode($json, true)['topic_groups'];
$fp = fopen("$repo_base/migration/exported/topics.json", 'r');
$n = 0;
$start = microtime(TRUE);
while (($line = fgets($fp)) !== FALSE) {
  $values = json_decode($line, TRUE);
  if (!empty($values['topic_group']))
    $values['topic_group'] = $groups[$values['topic_group']];
  $topic = \Drupal\ebms_topic\Entity\Topic::create($values);
  $topic->save();
  $n++;
}
$elapsed = round(microtime(TRUE) - $start);
log_success("Successfully loaded: $n topics", $elapsed);
