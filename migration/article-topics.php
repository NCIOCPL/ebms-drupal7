<?php

require(__DIR__ . '/console-log.php');

// Find out where the data is.
$repo_base = getenv('REPO_BASE') ?: '/var/www/ebms';

// Load the maps.
$json = file_get_contents("$repo_base/unversioned/maps.json");
$maps = json_decode($json, true);

// Load the topics.
$n = 0;
$start = microtime(TRUE);
$fp = fopen("$repo_base/unversioned/exported/article_topics.json", 'r');
while (($line = fgets($fp)) !== FALSE) {
  $values = json_decode($line, TRUE);
  $topic = \Drupal\ebms_article\Entity\ArticleTopic::create($values);
  $topic->save();
  $n++;
}
$elapsed = round(microtime(TRUE) - $start);
log_success("Successfully loaded: $n article topics", $elapsed);
