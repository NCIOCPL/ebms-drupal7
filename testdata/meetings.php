<?php

require(__DIR__ . '/console-log.php');

// Find out where the data is.
$repo_base = getenv('REPO_BASE') ?: '/var/www/ebms';

$json = file_get_contents("$repo_base/testdata/maps.json");
$maps = json_decode($json, true);
$meeting_categories = [
  'Board' => 'board',
  'Subgroup' => 'subgroup',
];
$meeting_statuses = [
  'Scheduled' => 'scheduled',
  'Canceled' => 'cancelled',
];
$meeting_types = [
  'In Person' => 'in_person',
  'Webex/Phone Conf.' => 'remote',
];
$meeting_maps = [];
$storage = \Drupal::entityTypeManager()->getStorage('taxonomy_term');
$query = $storage->getQuery();
$query->condition('vid', 'meeting_categories');
$ids = $query->execute();
$terms = $storage->loadMultiple($ids);
foreach ($terms as $term) {
  $key = $meeting_categories[$term->name->value];
  $meeting_maps['meeting_categories'][$key] = $term->id();
}
$query = $storage->getQuery();
$query->condition('vid', 'meeting_statuses');
$ids = $query->execute();
$terms = $storage->loadMultiple($ids);
foreach ($terms as $term) {
  $key = $meeting_statuses[$term->name->value];
  $meeting_maps['meeting_statuses'][$key] = $term->id();
}
$query = $storage->getQuery();
$query->condition('vid', 'meeting_types');
$ids = $query->execute();
$terms = $storage->loadMultiple($ids);
foreach ($terms as $term) {
  $key = $meeting_types[$term->name->value];
  $meeting_maps['meeting_types'][$key] = $term->id();
}

// print_r($meeting_maps);
$sample_agenda = file_get_contents("$repo_base/testdata/sample-agenda.html");
$sample_notes = file_get_contents("$repo_base/testdata/sample-notes.html");
$json = file_get_contents("$repo_base/testdata/meetings.json");
$data = json_decode($json, true);
foreach ($data as $values) {
  $name = str_replace('Integrative, Alternative, and Complementary Therapies', 'IACT', $values['name']);
  $agenda = str_replace('TITLE-PLACEHOLDER', "$name Agenda", $sample_agenda);
  $agenda = str_replace('DATE-PLACEHOLDER', substr($values['dates']['value'], 0, 10), $agenda);
  $values['agenda'] = [
    'value' => $agenda,
    'format' => 'filtered_html',
  ];
  $values['notes'] = [
    'value' => $sample_notes,
    'format' => 'filtered_html',
  ];
  if (!empty($values['category'])) {
    $category = $meeting_maps['meeting_categories'][$values['category']];
    $values['category'] = $category;
  }
  if (!empty($values['type']))
    $values['type'] = $meeting_maps['meeting_types'][$values['type']];
  if (!empty($values['status']))
    $values['status'] = $meeting_maps['meeting_statuses'][$values['status']];
  $groups = [];
  foreach ($values['groups']['ad_hoc_groups'] as $group)
    $groups[] = $maps['ad_hoc_groups'][$group];
  foreach ($values['groups']['subgroups'] as $group)
    $groups[] = $maps['subgroups'][$group];
  $values['groups'] = empty($groups) ? null : $groups;
  $meeting = \Drupal\ebms_meeting\Entity\Meeting::create($values);
  $meeting->save();
}
$n = count($data);
log_success("Successfully loaded: $n meetings");

// Get some specific terminology IDs we need.
$storage = \Drupal::entityTypeManager()->getStorage('taxonomy_term');
$query = $storage->getQuery();
$query->condition('vid', 'meeting_categories');
$query->condition('name', 'Board');
$ids = $query->execute();
$board_category = reset($ids);
$query = $storage->getQuery();
$query->condition('vid', 'meeting_statuses');
$query->condition('name', 'Scheduled');
$ids = $query->execute();
$on_calendar = reset($ids);
$query = $storage->getQuery();
$query->condition('vid', 'meeting_types');
$query->condition('name', 'In Person');
$ids = $query->execute();
$in_person = reset($ids);

// Create some tiny test document files.
$extensions = [
  'doc' => 'application/msword',
  'docx' => 'application/vnd.openxmlformats-officedocument.wordprocessingml.document',
  'pdf' => 'application/pdf',
  'rtf' => 'application/rtf',
];
$documents = [];
foreach ($extensions as $extension => $mimetype) {
  $bytes = file_get_contents("$repo_base/testdata/test.$extension");
  $filename = "Sample test document for board meeting.$extension";
  $path = "$repo_base/web/sites/default/files/$filename";
  file_put_contents($path, $bytes);
  $file = \Drupal\file\Entity\File::create([
    'uid' => 1,
    'filename' => $filename,
    'uri' => "public://$filename",
    'filemime' => $mimetype,
    'filesize' => strlen($bytes),
    'created' => time(),
    'changed' => time(),
    'status' => 1,
  ]);
  $file->save();
  $documents[] = $file->id();
}

// Make sure we have some future meetings for testing.
// Don't worry about some being on the weekend.
$boards = \Drupal\ebms_board\Entity\Board::loadMultiple();
$day = 4;
$month = date('n');
$year = date('Y');
$n = 0;
foreach ($boards as $board) {
  $name = $board->name->value;
  $title = "$name Board Meeting";
  if (str_starts_with($name, 'Integrative')) {
    $name = 'IACT';
  }
  elseif (str_starts_with($name, 'Cancer Genetics')) {
    $name = 'Genetics';
  }
  for ($i = 0; $i < 6; ++$i) {
    $y = $year;
    $m = $month + $i;
    if ($m > 12) {
      $m -= 12;
      $y += 1;
    }
    $date = sprintf('%04d-%02d-%02d', $y, $m, $day);
    $start = "{$date}T13:00:00";
    $end = "{$date}T16:00:00";
    $agenda = str_replace('TITLE-PLACEHOLDER', $title, $sample_agenda);
    $agenda = str_replace('DATE-PLACEHOLDER', $date, $agenda);
    $values = [
      'user' => $board->manager->target_id,
      'entered' => date('Y-m-d H:i:s'),
      'name' => "$name Board Meeting",
      'dates' => [
        'value' => $start,
        'end_value' => $end,
      ],
      'type' => $in_person,
      'category' => $board_category,
      'status' => $on_calendar,
      'boards' => [$board->id()],
      'groups' => [],
      'individuals' => [],
      'published' => 1,
      'documents' => $documents,
      'agenda' => [
        'value' => $agenda,
        'format' => 'filtered_html',
      ],
      'notes' => [
        'value' => $sample_notes,
        'format' => 'filtered_html',
      ],
    ];
    $meeting = \Drupal\ebms_meeting\Entity\Meeting::create($values);
    $meeting->save();
    ++$n;
  }
  $day += 4;
}
log_success("Successfully loaded: $n test future meetings");
