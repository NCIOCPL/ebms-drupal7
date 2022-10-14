<?php

require(__DIR__ . '/console-log.php');

// Find out where the data is.
$repo_base = getenv('REPO_BASE') ?: '/var/www/ebms';

// Load the maps.
$json = file_get_contents("$repo_base/unversioned/maps.json");
$maps = json_decode($json, true);

// Load the relationships.
$start = microtime(TRUE);
$n = 0;
$map = $maps['relationship_types'];
$path = "$repo_base/unversioned/exported/article_relationships.json";
$fp = fopen($path, 'r');
while (($line = fgets($fp)) !== FALSE) {
  $values = json_decode($line, TRUE);
  $values['type'] = $map[$values['type']];
  $relationship = \Drupal\ebms_article\Entity\Relationship::create($values);
  $relationship->save();
  $n++;
}
$elapsed = round(microtime(TRUE) - $start);
log_success("Successfully loaded: $n relationships", $elapsed);
