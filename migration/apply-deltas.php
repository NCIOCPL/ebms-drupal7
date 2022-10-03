<?php

require(__DIR__ . '/console-log.php');

// Make sure we don't create unwanted Message records.
if (empty(getenv('EBMS_MIGRATION_LOAD'))) {
  echo "Run this script using migration/apply-deltas.sh so that the\n";
  echo "EBMS_MIGRATION_LOAD environment variable gets set.\n";
  exit(1);
}

// Find out where the data is.
$repo_base = getenv('REPO_BASE') ?: '/var/www/ebms';
$deltas = "$repo_base/migration/deltas";
$start_top = microtime(TRUE);

// Load the ID mappings.
$json = file_get_contents("$repo_base/migration/maps.json");
$maps = json_decode($json, TRUE);
$meeting_vocabularies = [
  'meeting_categories' => ['Board' => 'board', 'Subgroup' => 'subgroup'],
  'meeting_statuses' => ['Scheduled' => 'scheduled', 'Canceled' => 'cancelled'],
  'meeting_types' => ['In Person' => 'in_person', 'Webex/Phone Conf.' => 'remote'],
];
$storage = \Drupal::entityTypeManager()->getStorage('taxonomy_term');
foreach ($meeting_vocabularies as $name => $values) {
  $ids = $storage->getQuery()->condition('vid', $name)->execute();
  foreach ($storage->loadMultiple($ids) as $term) {
    $key = $values[$term->name->value];
    $maps[$name][$key] = $term->id();
  }
}
$vocabularies = [
  'import_types',
  'import_dispositions',
  'hotels',
  'reimbursement_to',
  'transportation_expense_types',
  'parking_or_toll_expense_types',
  'hotel_payment_methods',
  'meals_and_incidentals',
];
foreach ($vocabularies as $vid) {
  $ids = $storage->getQuery()->condition('vid', $vid)->execute();
  foreach ($storage->loadMultiple($ids) as $term) {
    $maps[$vid][$term->field_text_id->value] = $term->id();
  }
}


// Map the user's group membership IDs and clear out empty picture fields.
function map_user_values(array &$values, array &$maps, string $repo_base) {
  if (!\Drupal::moduleHandler()->moduleExists('externalauth')) {
    $values['pass'] = trim(file_get_contents("$repo_base/userpw"));
  }
  $groups = [];
  foreach (['subgroups', 'ad_hoc_groups'] as $key) {
    if (!empty($values[$key])) {
      foreach ($values[$key] as $old_id) {
        $groups[] = $maps[$key][$old_id];
      }
    }
    unset($values[$key]);
  }
  if (!empty($groups)) {
    $values['groups'] = $groups;
  }
  if (empty($values['user_picture'])) {
    unset($values['user_picture']);
  }
}

// Take care of any updates to group mappings.
function map_ad_hoc_groups(array &$values, array &$maps, string $repo_base) {
  $original_id = $values['id'];
  if (empty($maps['ad_hoc_groups'][$original_id])) {
    $id = \Drupal::database()->query('SELECT MAX(id) FROM ebms_group')->fetchField() + 1;
    $maps['ad_hoc_groups'][$original_id] = $id;
    $fp = fopen("$repo_base/migration/maps.json", 'w');
    fwrite($fp, json_encode($maps, JSON_PRETTY_PRINT));
    fclose($fp);
  }
  $values['id'] = $maps['ad_hoc_groups'][$original_id];
}

// Take care of any updates to group mappings.
function map_subgroups(array &$values, array &$maps, string $repo_base) {
  $original_id = $values['id'];
  if (empty($maps['subgroups'][$original_id])) {
    $id = \Drupal::database()->query('SELECT MAX(id) FROM ebms_group')->fetchField() + 1;
    $maps['subgroups'][$original_id] = $id;
    $fp = fopen("$repo_base/migration/maps.json", 'w');
    fwrite($fp, json_encode($maps, JSON_PRETTY_PRINT));
    fclose($fp);
  }
  $values['id'] = $maps['subgroups'][$original_id];
}

// Plug in the Journal entity's ID, if this is not a new entity.
function map_journal_id(array &$values, array &$maps, string $repo_base) {
  $storage = \Drupal::entityTypeManager()->getStorage('ebms_journal');
  $ids = $storage->getQuery()->condition('source_id', $values['source_id'])->execute();
  $values['id'] = empty($ids) ? 0 : reset($ids);
}

// Map the topic group, if any.
function map_topic_group(array &$values, array &$maps, string $repo_base) {
  if (!empty($values['topic_group']))
    $values['topic_group'] = $maps['topic_groups'][$values['topic_group']];
}

// Map the meeting values.
function map_meeting_values(array &$values, array &$maps, string $repo_base) {
  if (!empty($values['category']))
    $values['category'] = $maps['meeting_categories'][$values['category']];
  if (!empty($values['type']))
    $values['type'] = $maps['meeting_types'][$values['type']];
  if (!empty($values['status']))
    $values['status'] = $maps['meeting_statuses'][$values['status']];
  $groups = [];
  if (!empty($values['groups']['ad_hoc_groups'])) {
    foreach ($values['groups']['ad_hoc_groups'] as $group)
      $groups[] = $maps['ad_hoc_groups'][$group];
  }
  if (!empty($values['groups']['subgroups'])) {
    foreach ($values['groups']['subgroups'] as $group)
      $groups[] = $maps['subgroups'][$group];
  }
  $values['groups'] = empty($groups) ? null : $groups;
}

// Map the document tag IDs.
function map_doc_values(array &$values, array &$maps, string $repo_base) {
  if (!empty($values['tags'])) {
    $tags = [];
    foreach ($values['tags'] as $tag) {
      $tags[] = $maps['doc_tags'][$tag];
    }
    $values['tags'] = $tags;
  }
}

// Map the state values.
function map_state_values(array &$values, array &$maps, string $repo_base) {
  $values['value'] = $maps['states'][$values['value']];
  if (!empty($values['decisions'])) {
    $decisions = [];
    foreach ($values['decisions'] as $decision) {
      $decision['decision'] = $maps['board_decisions'][$decision['decision']];
      $decisions[] = $decision;
    }
    $values['decisions'] = $decisions;
  }
}

// Map the article tag IDs.
function map_article_tags(array &$values, array &$maps, string $repo_base) {
  $values['tag'] = $maps['article_tags'][$values['tag']];
}

// Map internal tags and merge in values from PubMed.
function assemble_article(array &$values, array &$maps, string $repo_base) {
  $id = $values['id'];
  $xml = file_get_contents("$repo_base/migration/articles/$id.xml");
  $pubmed_values = \Drupal\ebms_article\Entity\Article::parse($xml);
  unset($pubmed_values['comments_corrections']);
  $values = array_merge($values, $pubmed_values);
  if (!empty($values['internal_tags'])) {
    $tags = [];
    foreach ($values['internal_tags'] as $tag) {
      $tag['tag'] = $maps['internal_tags'][$tag['tag']];
      $tags[] = $tag;
    }
    $values['internal_tags'] = $tags;
  }
}

// Map the article relationship type IDs.
function map_article_relationships(array &$values, array &$maps, string $repo_base) {
  $values['type'] = $maps['relationship_types'][$values['type']];
}

// Map the import batch vocabulary IDs.
function map_import_batch_values(array &$values, array &$maps, string $repo_base) {
  $actions = [];
  if (!empty($values['actions'])) {
    foreach ($values['actions'] as $action) {
      $action['disposition'] = $maps['import_dispositions'][$action['disposition']];
      $actions[] = $action;
    }
  }
  $values['actions'] = $actions;
  $values['import_type'] = $maps['import_types'][$values['import_type']];
}

// Handle mappings for import request entities.
function map_import_request_values(array &$values, array &$maps, string $repo_base) {
  $report = json_decode($values['report'], TRUE);
  $report['import_type'] = $maps['import_types'][$report['import_type']];
  $actions = [];
  if (!empty($report['actions'])) {
    foreach ($report['actions'] as $action) {
      $action['disposition'] = $maps['import_dispositions'][$action['disposition']];
      $actions[] = $action;
    }
  }
  $report['actions'] = $actions;
  $values['report'] = json_encode($report);
  $values['batch'] = $report['batch'] ?? NULL;
}

// Map vocubulary IDs for reviews.
function map_review_values(array &$values, array &$maps, string $repo_base) {
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
}

// Map the group IDs for messages.
function map_message_groups(array &$values, array &$maps, string $repo_base) {
  $groups = [];
  foreach (['subgroups', 'ad_hoc_groups'] as $key) {
    if (array_key_exists($key, $values)) {
      foreach ($values[$key] as $old_id) {
        $groups[] = $maps[$key][$old_id];
      }
    }
    unset($values[$key]);
  }
  if (!empty($groups)) {
    $values['groups'] = $groups;
  }
}

// Map the preferred hotel ID.
function map_hotel_id(array &$values, array &$maps, string $repo_base) {
  if (!empty($values['preferred_hotel'])) {
    $values['preferred_hotel'] = $maps['hotels'][$values['preferred_hotel']] ?? null;
  }
}

// Map the lookup values for reimbursement expenses.
function map_reimbursement_values(array &$values, array &$maps, string $repo_base) {
  if (!empty($values['transportation'])) {
    foreach ($values['transportation'] as &$transportation_expense) {
      $transportation_expense['type'] = $maps['transportation_expense_types'][$transportation_expense['type']] ?? null;
    }
  }
  if (!empty($values['parking_and_tolls'])) {
    foreach ($values['parking_and_tolls'] as &$parking_or_toll_expense) {
      $parking_or_toll_expense['type'] = $maps['parking_or_toll_expense_types'][$parking_or_toll_expense['type']] ?? null;
    }
  }
  if (!empty($values['hotel_payment'])) {
    $values['hotel_payment'] = $maps['hotel_payment_methods'][$values['hotel_payment']] ?? null;
  }
  if (!empty($values['meals_and_incidentals'])) {
    $values['meals_and_incidentals'] = $maps['meals_and_incidentals'][$values['meals_and_incidentals']] ?? null;
  }
  if (!empty($values['reimburse_to'])) {
    $values['reimburse_to'] = $maps['reimbursement_to'][$values['reimburse_to']] ?? null;
  }
}

// Plug in the board summaries entity's ID, if this is not a new entity.
function map_board_summaries(array &$values, array &$maps, string $repo_base) {
  $storage = \Drupal::entityTypeManager()->getStorage('ebms_board_summaries');
  $ids = $storage->getQuery()->condition('board', $values['board'])->execute();
  $values['id'] = empty($ids) ? 0 : reset($ids);
}

// Control values for each of the entity types.
$entity_types = [
  'files' => [
    'id_key' => 'fid',
    'class' => '\Drupal\file\Entity\File',
  ],
  'subgroups' => [
    'class' => '\Drupal\ebms_group\Entity\Group',
    'mapper' => 'map_subgroups',
  ],
  'ad_hoc_groups' => [
    'class' => '\Drupal\ebms_group\Entity\Group',
    'mapper' => 'map_ad_hoc_groups',
  ],
  'users' => [
    'id_key' => 'uid',
    'class' => '\Drupal\user\Entity\User',
    'mapper' => 'map_user_values',
  ],
  'boards' => [
    'class' => '\Drupal\ebms_board\Entity\Board',
  ],
  'journals' => [
    'class' => '\Drupal\ebms_journal\Entity\Journal',
    'mapper' => 'map_journal_id',
  ],
  'meetings' => [
    'class' => '\Drupal\ebms_meeting\Entity\Meeting',
    'mapper' => 'map_meeting_values',
  ],
  'docs' => [
    'class' => '\Drupal\ebms_doc\Entity\Doc',
    'mapper' => 'map_doc_values',
  ],
  'summary_pages' => [
    'class' => '\Drupal\ebms_summary\Entity\SummaryPage',
  ],
  'board_summaries' => [
    'class' => '\Drupal\ebms_summary\Entity\BoardSummaries',
    'mapper' => 'map_board_summaries',
  ],
  'states' => [
    'display' => 'article states',
    'class' => '\Drupal\ebms_state\Entity\State',
    'mapper' => 'map_state_values',
  ],
  'article_tags' => [
    'class' =>   '\Drupal\ebms_article\Entity\ArticleTag',
    'mapper' => 'map_article_tags',
  ],
  'article_topics' => [
    'class' => '\Drupal\ebms_article\Entity\ArticleTopic',
  ],
  'articles' => [
    'class' => '\Drupal\ebms_article\Entity\Article',
    'mapper' => 'assemble_article',
  ],
  'article_relationships' => [
    'class' => '\Drupal\ebms_article\Entity\Relationship',
    'mapper' => 'map_article_relationships',
  ],
  'import_batches' => [
    'class' => '\Drupal\ebms_import\Entity\Batch',
    'mapper' => 'map_import_batch_values',
  ],
  'import_requests' => [
    'class' => '\Drupal\ebms_import\Entity\ImportRequest',
    'mapper' => 'map_import_request_values',
  ],
  'reviewer_docs' => [
    'class' => '\Drupal\ebms_review\Entity\ReviewerDoc',
  ],
  'reviews' => [
    'class' => '\Drupal\ebms_review\Entity\Review',
    'mapper' => 'map_review_values',
  ],
  'packet_articles' => [
    'class' => '\Drupal\ebms_review\Entity\PacketArticle',
  ],
  'packets' => [
    'class' => '\Drupal\ebms_review\Entity\Packet',
  ],
  'hotel_requests' => [
    'class' => '\Drupal\ebms_travel\Entity\HotelRequest',
    'mapper' => 'map_hotel_id',
  ],
  'reimbursement_requests' => [
    'class' => '\Drupal\ebms_travel\Entity\ReimbursementRequest',
    'mapper' => 'map_reimbursement_values',
  ],
  'messages' => [
    'class' => '\Drupal\ebms_message\Entity\Message',
    'mapper' => 'map_message_groups',
  ],
];

// Process each entity type.
foreach ($entity_types as $type_name => $entity_type) {
  $display = $entity_type['display'] ?? str_replace('_', ' ', $type_name);
  $mapper = $entity_type['mapper'] ?? '';
  foreach (['mod', 'new'] as $subdir) {
    $path = "$deltas/$subdir/$type_name.json";
    if (file_exists($path)) {
      $n = 0;
      $start = microtime(TRUE);
      $fp = fopen($path, 'r');
      while (($line = fgets($fp)) !== FALSE) {
        $values = json_decode($line, TRUE);
        if (!empty($mapper)) {
          $mapper($values, $maps, $repo_base);
        }
        $id = $values[$entity_type['id_key'] ?? 'id'];
        $entity = $entity_type['class']::load($id);
        if (empty($entity)) {
          $entity = $entity_type['class']::create($values);
        }
        else {
          foreach ($values as $name => $value) {
            $entity->set($name, $value);
          }
        }
        $entity->save();
        $n++;
      }
      if ($n) {
        $elapsed = round(microtime(TRUE) - $start);
        $verb = $subdir === 'mod' ? 'updated' : 'added';
        log_success("Successfully $verb $n $display", $elapsed);
      }
    }
  }
}

// If we're using SSO, populate the authmap table.
if (\Drupal::moduleHandler()->moduleExists('externalauth')) {
  $db = \Drupal::database();
  $db->query('DELETE FROM authmap')->execute();
  $fp = fopen("$repo_base/migration/exported/authmap.json", 'r');
  while (($line = fgets($fp)) !== FALSE) {
    $values = json_decode($line, TRUE);
    $db->insert('authmap')->fields($values)->execute();
  }
}

// We're done.
$elapsed = round(microtime(TRUE) - $start_top);
log_success("Successfully applied all deltas", $elapsed);
