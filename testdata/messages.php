<?php

require(__DIR__ . '/console-log.php');

use Drupal\ebms_meeting\Entity\Meeting;
use Drupal\ebms_message\Entity\Message;

// Find out where the data is.
$repo_base = getenv('REPO_BASE') ?: '/var/www/ebms';

$created = 0;
$meetings = \Drupal\ebms_meeting\Entity\Meeting::loadMultiple();
foreach ($meetings as $meeting) {
  $boards = [];
  $groups = [];
  $users = [];
  foreach ($meeting->boards as $board)
    $boards[] = $board->target_id;
  foreach ($meeting->groups as $group)
    $groups[] = $group->target_id;
  foreach ($meeting->individuals as $user)
    $users[] = $user->target_id;
  $values = json_encode([
    'meeting_id' => $meeting->id(),
    'title' => $meeting->name->value,
  ]);
  $start = new \DateTime($meeting->dates->value);
  $posted = $start->sub(new \DateInterval('P5M'))->format('Y-m-d H:i:s');
  if ($meeting->status->entity->name->value === Meeting::SCHEDULED) {
    Message::create([
      'message_type' => Message::MEETING_PUBLISHED,
      'user' => $meeting->user->target_id,
      'posted' => $posted,
      'boards' => $boards,
      'groups' => $groups,
      'individuals' => $users,
      'extra_values' => $values,
    ])->save();
    $created++;
    if (!empty($meeting->agenda_published->value)) {
      Message::create([
        'message_type' => Message::AGENDA_PUBLISHED,
        'user' => $meeting->user->target_id,
        'posted' => $posted,
        'boards' => $boards,
        'groups' => $groups,
        'individuals' => $users,
        'extra_values' => $values,
      ])->save();
      $created++;
    }
  }
  else {
    Message::create([
      'message_type' => Message::MEETING_CANCELED,
      'user' => $meeting->user->target_id,
      'posted' => $posted,
      'boards' => $boards,
      'groups' => $groups,
      'individuals' => $users,
      'extra_values' => $values,
    ])->save();
    $created++;
  }
}
$storage = \Drupal::entityTypeManager()->getStorage('ebms_state');
$ids = $storage->getQuery()
   ->accessCheck(FALSE)
   ->condition('value.entity.field_text_id', 'published')
   ->execute();
$published = $storage->loadMultiple($ids);
$batches = [];
foreach ($published as $state) {
  $board = $state->board->target_id;
  $date = substr($state->entered->value, 0, 10);
  $user = $state->user->target_id;
  $key = "$board|$user|$date";
  $batches[$key] = $key;
}
foreach ($batches as $batch) {
  list($board_id, $user, $date) = explode('|', $batch);
  $board = \Drupal\ebms_board\Entity\Board::load($board_id);
  Message::create([
    'message_type' => Message::ARTICLES_PUBLISHED,
    'user' => $user,
    'posted' => $date,
    'boards' => [$board_id],
    'extra_values' => json_encode(['board_name' => $board->name->value]),
  ])->save();
  $created++;
}
$boards = \Drupal\ebms_board\Entity\Board::loadMultiple();
$dates = [
  date('Y-m-d H:i:s', strtotime('-18 days')),
  date('Y-m-d H:i:s', strtotime('-22 days')),
];
foreach ($boards as $board_id => $board) {
  foreach ($dates as $date) {
    Message::create([
      'message_type' => Message::ARTICLES_PUBLISHED,
      'user' => 1,
      'posted' => $date,
      'boards' => [$board_id],
      'extra_values' => json_encode(['board_name' => $board->name->value]),
    ])->save();
    $created++;
  }
}
$packets = \Drupal\ebms_review\Entity\Packet::loadMultiple();
foreach ($packets as $packet) {
  $reviewers = [];
  foreach ($packet->reviewers as $reviewer)
    $reviewers[] = $reviewer->target_id;
  Message::create([
    'message_type' => Message::PACKET_CREATED,
    'user' => $packet->created_by->target_id,
    'posted' => $packet->created->value,
    'individuals' => $reviewers,
    'extra_values' => json_encode([
      'packet_id' => $packet->id(),
      'title' => $packet->title->value,
    ]),
  ])->save();
  $created++;
}
$pages = \Drupal\ebms_summary\Entity\SummaryPage::loadMultiple();
foreach ($pages as $page) {
  $boards = [];
  foreach ($page->topics as $topic)
    $boards[$topic->entity->board->target_id] = $topic->entity->board->target_id;
  $storage = \Drupal::entityTypeManager()->getStorage('user');
  $query = $storage->getQuery()->accessCheck(FALSE);
  if (empty($boards)) {
    echo 'no boards for ' . $page->name->value . "\n";
    continue;
  }
  // echo 'got boards for ' . $page->name->value . "\n";
  $query->condition('boards', $boards, 'IN');
  $query->condition('status', 1);
  $users = $storage->loadMultiple($query->execute());
  $docs = [];
  foreach ($page->manager_docs as $manager_doc)
    $docs[$manager_doc->doc] = $manager_doc->notes;
  foreach ($page->member_docs as $member_doc)
    $docs[$member_doc->doc] = $member_doc->notes;
  foreach ($docs as $doc_id => $notes) {
    $doc = \Drupal\ebms_doc\Entity\Doc::load($doc_id);
    Message::create([
      'message_type' => Message::SUMMARY_POSTED,
      'user' => $doc->file->entity->uid->target_id,
      'posted' => $doc->posted->value,
      'boards' => $boards,
      'extra_values' => json_encode([
        'summary_url' => $doc->file->entity->createFileUrl(),
        'title' => $doc->description->value,
        'notes' => $notes,
      ]),
    ])->save();
    $created++;
  }
}
$bytes = file_get_contents("$repo_base/testdata/test.docx");
$filename = 'sample-document-for-activity-messages.docx';
$timestamp = time();
file_put_contents("$repo_base/web/sites/default/files/$filename", $bytes);
$file = \Drupal\file\Entity\File::create([
  'uid' => 1,
  'filename' => $filename,
  'uri' => "public://$filename",
  'filemime' => 'application/vnd.openxmlformats-officedocument.wordprocessingml.document',
  'filesize' => strlen($bytes),
  'created' => $timestamp,
  'changed' => $timestamp,
  'status' => 1,
]);
$file->setPermanent();
$file->save();
$url = $file->createFileUrl();
$boards = \Drupal\ebms_board\Entity\Board::loadMultiple();
$json = file_get_contents("$repo_base/testdata/summaries.json");
$summaries = json_decode($json, TRUE);
foreach ($boards as $board_id => $board) {
  $board_name = $board->name->value;
  foreach($summaries[$board_name] as $summary_name) {
    $notes = random_int(1, 2) === 1 ? 'Test note' : NULL;
    $days = random_int(1, 50);
    $date = strtotime("-$days days");
    $suffix = date('Ymd', $date);
    $title = "{$summary_name}_{$suffix}";
    Message::create([
      'message_type' => Message::SUMMARY_POSTED,
      'user' => $board->manager->target_id,
      'posted' => date('Y-m-d H:i:s', $date),
      'boards' => [$board_id],
      'extra_values' => json_encode([
        'summary_url' => $url,
        'title' => $title,
        'notes' => $notes,
      ]),
    ])->save();
    $created++;
  }
}
log_success("Successfully loaded: $created messages");
