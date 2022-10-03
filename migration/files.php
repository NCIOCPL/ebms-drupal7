<?php

require(__DIR__ . '/console-log.php');

// Find out where the data is.
$repo_base = getenv('REPO_BASE') ?: '/var/www/ebms';

$start = microtime(TRUE);
$n = 0;
$fp = fopen("$repo_base/migration/exported/files.json", 'r');
while (($line = fgets($fp)) !== FALSE) {
  $values = json_decode($line, TRUE);
  if (empty($values['uri'])) {
    continue;
  }
  $file = \Drupal\file\Entity\File::create($values);
  $file->save();
  $n++;
}
$elapsed = round(microtime(TRUE) - $start);
log_success("Successfully loaded: $n files", $elapsed);
