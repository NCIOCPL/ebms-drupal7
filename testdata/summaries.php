<?php

require(__DIR__ . '/console-log.php');

// Find out where the data is.
$repo_base = getenv('REPO_BASE') ?: '/var/www/ebms';

$json = file_get_contents("$repo_base/testdata/summary-pages.json");
$pages = json_decode($json, true);
foreach ($pages as $values) {
  /*
  $links = [];
  foreach ($values['links'] as $link_values) {
    $uri = $link_values['url'];
    $label = $link_values['label'];
    $url = \Drupal\Core\Url::fromUri($uri);
    $links[] = \Drupal\Core\Link::fromTextAndUrl($label, $url);
  }
  $values['links'] = $links;
  */
  $page = \Drupal\ebms_summary\Entity\SummaryPage::create($values);
  $page->save();
}
$n = count($pages);
log_success("Successfully loaded: $n summary pages");
$json = file_get_contents("$repo_base/testdata/board-summaries.json");
$board_summaries = json_decode($json, true);
foreach ($board_summaries as $values) {
  $entity = \Drupal\ebms_summary\Entity\BoardSummaries::create($values);
  $entity->save();
}
$n = count($board_summaries);
log_success("Successfully loaded: $n board summaries");
