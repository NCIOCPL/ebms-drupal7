<?php

require(__DIR__ . '/console-log.php');

// Find out where the data is.
$repo_base = getenv('REPO_BASE') ?: '/var/www/ebms';

$start = microtime(TRUE);
$n = 0;
$fp = fopen("$repo_base/migration/exported/journals.json", 'r');
while (($line = fgets($fp)) !== FALSE) {
  $values = json_decode($line, TRUE);
  $journal = \Drupal\ebms_journal\Entity\Journal::create($values);
  $journal->save();
  $n++;
}
$elapsed = round(microtime(TRUE) - $start);
log_success("Successfully loaded: $n journals", $elapsed);
