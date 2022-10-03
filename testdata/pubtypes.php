<?php

require(__DIR__ . '/console-log.php');

// Find out where the data is.
$repo_base = getenv('REPO_BASE') ?: '/var/www/ebms';

// Load the publication types into the database.
$types = file_get_contents("$repo_base/testdata/pubtypes.json");
$connection = \Drupal::service('database');
$connection->insert('on_demand_config')
  ->fields([
    'name' => 'article-type-ancestors',
    'value' => $types,
  ])
  ->execute();
log_success('Successfully loaded: 1 configuration row');
