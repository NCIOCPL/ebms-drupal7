<?php

require(__DIR__ . '/console-log.php');

// Find out where the data is.
$repo_base = getenv('REPO_BASE') ?: '/var/www/ebms';

// Load the stub pages.
$pages = [
  'Board Members' => [
    'Login/Logout',
    'Home Page',
    'Article Search',
    'Calendar',
    'Packets',
    'Summaries',
    'Travel',
    'Profile',
  ],
  'NCI Staff' => [
    'Login/Logout',
    'Home Page',
  ],
];
$count = 0;
foreach ($pages as $section => $titles) {
  $prefix = "$section User Manual —";
  $top = $section === 'NCI Staff' ? 'ncihelp' : 'help';
  foreach ($titles as $title) {
    switch ($title) {
      case 'Login/Logout':
        $alias = 'login';
        break;
      case 'Home Page':
        $alias = 'home';
        break;
      case 'Article Search':
        $alias = 'search';
        break;
      default:
        $alias = strtolower($title);
        break;
    }
    $values = [
      'title' => "$prefix $title",
      'uid' => 1,
      'type' => 'page',
      'path' => ['alias' => "/$top/$alias"],
      'body' => [
        'value' => '<p>This is a stub help page.</p>',
        'format' => 'full_html',
      ],
    ];
    $page = \Drupal\node\Entity\Node::create($values);
    $page->save();
    $count++;
  }
}
log_success("Successfully loaded: $count help pages");
