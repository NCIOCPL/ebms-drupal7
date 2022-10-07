<?php

require(__DIR__ . '/console-log.php');

// Find out where the data is.
$repo_base = getenv('REPO_BASE') ?: '/var/www/ebms';

$fp = fopen("$repo_base/unversioned/exported/summary_pages.json", 'r');
$n = 0;
$start = microtime(TRUE);
while (($line = fgets($fp)) !== FALSE) {
  $values = json_decode($line, TRUE);
  $page = \Drupal\ebms_summary\Entity\SummaryPage::create($values);
  $page->save();
  $n++;
}
$elapsed = round(microtime(TRUE) - $start);
log_success("Successfully loaded: $n summary pages", $elapsed);
$n = 0;
$start = microtime(TRUE);
$fp = fopen("$repo_base/unversioned/exported/board_summaries.json", 'r');
while (($line = fgets($fp)) !== FALSE) {
  $values = json_decode($line, TRUE);
  $entity = \Drupal\ebms_summary\Entity\BoardSummaries::create($values);
  $entity->save();
  $n++;
}
$elapsed = round(microtime(TRUE) - $start);
log_success("Successfully loaded: $n board summary page sets", $elapsed);
