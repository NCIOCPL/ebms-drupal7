<?php

require(__DIR__ . '/console-log.php');

// Find out where the data is.
$repo_base = getenv('REPO_BASE') ?: '/var/www/ebms';

# Load the ID maps.
$json = file_get_contents("$repo_base/testdata/maps.json");
$maps = json_decode($json, true);

# Load the reviewer documents.
$json = file_get_contents("$repo_base/testdata/reviewer_docs.json");
$rows = json_decode($json, true);
foreach ($rows as $values) {
  $entity = \Drupal\ebms_review\Entity\ReviewerDoc::create($values);
  $entity->save();
}
$n = count($rows);
log_success("Successfully loaded: $n reviewer documents");

# Load the reviews.
$json = file_get_contents("$repo_base/testdata/reviews.json");
$rows = json_decode($json, true);
foreach ($rows as $values) {
  if (array_key_exists('dispositions', $values)) {
    $dispositions = [];
    foreach ($values['dispositions'] as $disposition) {
      $dispositions[] = $maps['dispositions'][$disposition];
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
}
$n = count($rows);
log_success("Successfully loaded: $n reviews");

# Load the packet articles.
$json = file_get_contents("$repo_base/testdata/packet_articles.json");
$rows = json_decode($json, true);
foreach ($rows as $values) {
  $entity = \Drupal\ebms_review\Entity\PacketArticle::create($values);
  $entity->save();
}
$n = count($rows);
log_success("Successfully loaded: $n packet articles");

# Load the packets.
$json = file_get_contents("$repo_base/testdata/packets.json");
$rows = json_decode($json, true);
foreach ($rows as $values) {
  $entity = \Drupal\ebms_review\Entity\Packet::create($values);
  $entity->save();
}
$n = count($rows);
log_success("Successfully loaded: $n packets");
