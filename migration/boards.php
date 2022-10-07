<?php

require(__DIR__ . '/console-log.php');

// Find out where the data is.
$repo_base = getenv('REPO_BASE') ?: '/var/www/ebms';

$start = microtime(TRUE);
$fp = fopen("$repo_base/unversioned/exported/boards.json", 'r');
$n = 0;
while (($line = fgets($fp)) !== FALSE) {
  $values = json_decode($line, TRUE);
  $board = \Drupal\ebms_board\Entity\Board::create($values);
  $board->save();
  $n++;
}
$elapsed = round(microtime(TRUE) - $start);
log_success("Successfully loaded: $n boards", $elapsed);
