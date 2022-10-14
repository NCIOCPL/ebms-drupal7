<?php

require(__DIR__ . '/console-log.php');

// Find out where the data is.
$repo_base = getenv('REPO_BASE') ?: '/var/www/ebms';

// Load the maps.
$json = file_get_contents("$repo_base/unversioned/maps.json");
$maps = json_decode($json, true);

// Load the tags.
$start = microtime(TRUE);
$fp = fopen("$repo_base/unversioned/exported/article_tags.json", 'r');
$n = 0;
while (($line = fgets($fp)) !== FALSE) {
  $values = json_decode($line, TRUE);
  $values['tag'] = $maps['article_tags'][$values['tag']];
  $tag = \Drupal\ebms_article\Entity\ArticleTag::create($values);
  $tag->save();
  $n++;
}
$elapsed = round(microtime(TRUE) - $start);
log_success("Successfully loaded: $n article tags", $elapsed);
