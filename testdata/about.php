<?php

require(__DIR__ . '/console-log.php');

// Load the placeholder page.
$values = [
  'title' => 'About PDQ®',
  'uid' => 1,
  'type' => 'page',
  'path' => ['alias' => '/about'],
  'body' => [
    'value' => '<p><img src="/themes/custom/ebms/images/library.jpg" ' .
               'alt="Medical Research"><br>All the information about PDQ® ' .
               'and the EBMS. ℹ️</p>',
    'format' => 'filtered_html',
  ],
];
$page = \Drupal\node\Entity\Node::create($values);
$page->save();
log_success("Successfully loaded: 1 about page");
