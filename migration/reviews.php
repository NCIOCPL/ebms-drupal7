<?php

require(__DIR__ . '/console-log.php');

// Find out where the data is.
$repo_base = getenv('REPO_BASE') ?: '/var/www/ebms';

# Load the ID maps.
$json = file_get_contents("$repo_base/unversioned/maps.json");
$maps = json_decode($json, true);

# Load the reviewer documents.
$start = microtime(TRUE);
$fp = fopen("$repo_base/unversioned/exported/reviewer_docs.json", 'r');
$count = 0;
while (($line = fgets($fp)) !== FALSE) {
  $values = json_decode($line, TRUE);
  $entity = \Drupal\ebms_review\Entity\ReviewerDoc::create($values);
  $entity->save();
  $count++;
}
$elapsed = round(microtime(TRUE) - $start);
log_success("Successfully loaded: $count reviewer documents", $elapsed);

# Load the reviews.
$fp = fopen("$repo_base/unversioned/exported/reviews.json", 'r');
$count = 0;
$start = microtime(TRUE);
while (($line = fgets($fp)) !== FALSE) {
  $values = json_decode($line, TRUE);
  if (array_key_exists('dispositions', $values)) {
    $dispositions = [];
    foreach ($values['dispositions'] as $disposition) {
      $dispositions[] = $maps['review_dispositions'][$disposition];
    }
    $values['dispositions'] = $dispositions;
  }
  if (array_key_exists('reasons', $values)) {
    $reasons = [];
    foreach ($values['reasons'] as $reason) {
      $reasons[] = $maps['rejection_reasons'][$reason];
    }
    $values['reasons'] = $reasons;
  }
  $values['comments'] = trim($values['comments'] ?? '');
  while (str_ends_with($values['comments'], '<br>')) {
    $values['comments'] = substr($values['comments'], 0, strlen($values['comments']) - 4);
  }
  $entity = \Drupal\ebms_review\Entity\Review::create($values);
  $entity->save();
  $count++;
}
$elapsed = round(microtime(TRUE) - $start);
log_success("Successfully loaded: $count reviews", $elapsed);

# Load the packet articles.
$fp = fopen("$repo_base/unversioned/exported/packet_articles.json", 'r');
$count = 0;
$start = microtime(TRUE);
while (($line = fgets($fp)) !== FALSE) {
  $values = json_decode($line, TRUE);
  $entity = \Drupal\ebms_review\Entity\PacketArticle::create($values);
  $entity->save();
  $count++;
}
$elapsed = round(microtime(TRUE) - $start);
log_success("Successfully loaded: $count packet articles", $elapsed);

# Load the packets.
$fp = fopen("$repo_base/unversioned/exported/packets.json", 'r');
$count = 0;
$start = microtime(TRUE);
while (($line = fgets($fp)) !== FALSE) {
  $values = json_decode($line, TRUE);
  $entity = \Drupal\ebms_review\Entity\Packet::create($values);
  $entity->save();
  $count++;
}
$elapsed = round(microtime(TRUE) - $start);
log_success("Successfully loaded: $count packets", $elapsed);
