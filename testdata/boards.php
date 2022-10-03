<?php

require(__DIR__ . '/console-log.php');

// Find out where the data is.
$repo_base = getenv('REPO_BASE') ?: '/var/www/ebms';

$json = file_get_contents("$repo_base/testdata/boards.json");
$boards = json_decode($json, true);
foreach ($boards as $values) {
  $board = \Drupal\ebms_board\Entity\Board::create($values);
  $board->save();
}
$n = count($boards);
log_success("Successfully loaded: $n boards");
