<?php

require(__DIR__ . '/console-log.php');

// Find out where the data is.
$repo_base = getenv('REPO_BASE') ?: '/var/www/ebms';

// Load the image files.
$start = microtime(TRUE);
$paths = array_filter(glob("$repo_base/unversioned/inline-images/*"), 'is_file');
$count = 0;
$mime_map = ['jpg' => 'image/jpeg', 'png' => 'image/png'];
foreach ($paths as $path) {
  $path_parts = pathinfo($path);
  $extension = $path_parts['extension'];
  $filename = $path_parts['basename'];
  $values = [
    'uid' => 1,
    'filename' => $filename,
    'uri' => "public://inline-images/$filename",
    'filemime' => $mime_map[$extension],
    'filesize' => filesize($path),
    'created' => filemtime($path),
    'status' => 1,
  ];
  $file = \Drupal\file\Entity\File::create($values);
  $file->save();
  $count++;
}
$elapsed = round(microtime(TRUE) - $start);
log_success("Successfully loaded: $count help images", $elapsed);

// Load the pages.
$start = microtime(TRUE);
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
    $name = strtolower($title);
    $name = str_replace(' ', '-', $name);
    $name = str_replace('/', '-', $name);
    $path = "$repo_base/unversioned/$top/$name.html";
    $values = [
      'title' => "$prefix $title",
      'uid' => 1,
      'type' => 'page',
      'path' => ['alias' => "/$top/$alias"],
      'body' => [
        'value' => file_get_contents($path),
        'format' => 'full_html',
      ],
    ];
    $page = \Drupal\node\Entity\Node::create($values);
    $page->save();
    $count++;
  }
}
$elapsed = round(microtime(TRUE) - $start);
log_success("Successfully loaded: $count help pages", $elapsed);
