<?php

require(__DIR__ . '/console-log.php');

// Find out where the data is.
$repo_base = getenv('REPO_BASE') ?: '/var/www/ebms';

// Load the page.
$start = microtime(TRUE);
$values = [
  'title' => 'About PDQ®',
  'uid' => 1,
  'type' => 'page',
  'path' => ['alias' => '/about'],
  'body' => [
    'value' => file_get_contents("$repo_base/migration/about.html"),
    'format' => 'filtered_html',
  ],
];
$page = \Drupal\node\Entity\Node::create($values);
$page->save();
$elapsed = round(microtime(TRUE) - $start);
log_success("Successfully loaded: 1 about page", $elapsed);
