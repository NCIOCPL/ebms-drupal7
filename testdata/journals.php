<?php

require(__DIR__ . '/console-log.php');

// Find out where the data is.
$repo_base = getenv('REPO_BASE') ?: '/var/www/ebms';

$query = \Drupal::database()->select('ebms_board', 'board');
$query->fields('board', ['id', 'manager']);
$boards = [];
foreach ($query->execute() as $row) {
  $boards[$row->id] = $row->manager;
}
$query = \Drupal::database()->select('ebms_article', 'article');
$query->addExpression('COUNT(*)', 'count');
$query->addField('article', 'source_journal_id', 'journal_id');
$query->groupBy('article.source_journal_id');
$rows = $query->execute();
$counts = [];
foreach ($rows as $row) {
  $counts[$row->journal_id] = $row->count;
}
$json = file_get_contents("$repo_base/testdata/journal-ids.json");
$journal_ids = json_decode($json, true);
$json = file_get_contents("$repo_base/testdata/journals.json");
$journals = json_decode($json, true);
$n = 0;
$now = date('Y-m-d H:i:s');
foreach ($journals as $values) {
  if (in_array($values['source_id'], $journal_ids)) {
    $values['not_lists'] = [];
    $core = FALSE;
    if (!array_key_exists($values['source_id'], $counts)) {
      foreach ($boards as $board_id => $manager_id) {
        $values['not_lists'][] = [
          'board' => $board_id,
          'start' => $now,
          'user' => $manager_id,
        ];
      }
    }
    elseif ($counts[$values['source_id']] > 5) {
      $core = TRUE;
    }
    $values['core'] = $core;
    $journal = \Drupal\ebms_journal\Entity\Journal::create($values);
    $journal->save();
    ++$n;
  }
}
log_success("Successfully loaded: $n journals");
