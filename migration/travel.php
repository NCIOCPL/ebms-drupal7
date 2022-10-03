<?php

require(__DIR__ . '/console-log.php');

// Find out where the data is.
$repo_base = getenv('REPO_BASE') ?: '/var/www/ebms';

$start = microtime(TRUE);
$config_factory = \Drupal::service('config.factory');
$config_sources = [
  'instructions' => [
    'hotel' => 'hotel-form.html',
    'reimbursement' => 'reimbursement-form.html',
  ],
  'email' => [
    'travel_manager' => 'travel-manager',
    'developers' => 'developers',
  ],
];
foreach ($config_sources as $set_name => $values) {
  $config = $config_factory->getEditable("ebms_travel.$set_name");
  foreach ($values as $name => $filename) {
    $path = "$repo_base/migration/$filename";
    $config->set($name, trim(file_get_contents($path)));
  }
  $config->save();
}

// Suppress sidebar blocks we don't want (unnecessary now, but harmless).
$storage = \Drupal::entityTypeManager()->getStorage('block');
$query = $storage->getQuery()->accessCheck(FALSE);
$query->condition('region', 'sidebar_first');
foreach ($storage->loadMultiple($query->execute()) as $block) {
  if (str_starts_with($block->id(), 'bartik')) {
    $block->setStatus(FALSE);
    $block->save();
  }
}

// Load the pages.
$values = [
  'title' => 'Travel',
  'uid' => 1,
  'type' => 'page',
  'path' => ['alias' => '/travel'],
  'body' => [
    'value' => file_get_contents("$repo_base/migration/travel-landing.html"),
    'format' => 'filtered_html',
  ],
];
$page = \Drupal\node\Entity\Node::create($values);
$page->save();
$values = [
  'title' => 'Directions',
  'uid' => 1,
  'type' => 'page',
  'path' => ['alias' => '/travel/directions'],
  'body' => [
    'value' => file_get_contents("$repo_base/migration/travel-directions.html"),
    'format' => 'filtered_html',
  ],
];
$page = \Drupal\node\Entity\Node::create($values);
$page->save();
$values = [
  'title' => 'Policies and Procedures',
  'uid' => 1,
  'type' => 'page',
  'path' => ['alias' => '/travel/policies-and-procedures'],
  'body' => [
    'value' => file_get_contents("$repo_base/migration/travel-policies.html"),
    'format' => 'filtered_html',
  ],
];
$page = \Drupal\node\Entity\Node::create($values);
$page->save();
$elapsed = round(microtime(TRUE) - $start);
log_success('Successfully loaded: 3 static travel pages', $elapsed);

// Load the hotel requests.
$start = microtime(TRUE);
$storage = \Drupal::entityTypeManager()->getStorage('taxonomy_term');
$query = $storage->getQuery();
$query->condition('vid', 'hotels');
$terms = $storage->loadMultiple($query->execute());
$hotels = [];
foreach ($terms as $term)
  $hotels[$term->field_text_id->value] = $term->id();
$fp = fopen("$repo_base/migration/exported/hotel_requests.json", 'r');
$created = 0;
while (($line = fgets($fp)) !== FALSE) {
  $values = json_decode($line, TRUE);
  if (!empty($values['preferred_hotel'])) {
    $values['preferred_hotel'] = $hotels[$values['preferred_hotel']] ?? null;
  }
  $request = \Drupal\ebms_travel\Entity\HotelRequest::create($values);
  $request->save();
  $created++;
}
$elapsed = round(microtime(TRUE) - $start);
log_success("Successfully loaded: $created hotel requests", $elapsed);

// Load the reimbursement requests.
$start = microtime(TRUE);
$query = $storage->getQuery();
$query->condition('vid', 'reimbursement_to');
$terms = $storage->loadMultiple($query->execute());
$reimburse_to = [];
foreach ($terms as $term)
  $reimburse_to[$term->field_text_id->value] = $term->id();
$query = $storage->getQuery();
$query->condition('vid', 'transportation_expense_types');
$terms = $storage->loadMultiple($query->execute());
$transportation_expense_types = [];
foreach ($terms as $term)
  $transportation_expense_types[$term->field_text_id->value] = $term->id();
$query = $storage->getQuery();
$query->condition('vid', 'parking_or_toll_expense_types');
$terms = $storage->loadMultiple($query->execute());
$parking_or_toll_expense_types = [];
foreach ($terms as $term)
  $parking_or_toll_expense_types[$term->field_text_id->value] = $term->id();
$query = $storage->getQuery();
$query->condition('vid', 'hotel_payment_methods');
$terms = $storage->loadMultiple($query->execute());
$hotel_payment_methods = [];
foreach ($terms as $term)
  $hotel_payment_methods[$term->field_text_id->value] = $term->id();
$query = $storage->getQuery();
$query->condition('vid', 'meals_and_incidentals');
$terms = $storage->loadMultiple($query->execute());
$meals_and_incidentals = [];
foreach ($terms as $term)
  $meals_and_incidentals[$term->field_text_id->value] = $term->id();
$fp = fopen("$repo_base/migration/exported/reimbursement_requests.json", 'r');
$created = 0;
while (($line = fgets($fp)) !== FALSE) {
  $values = json_decode($line, TRUE);
  if (!empty($values['transportation'])) {
    foreach ($values['transportation'] as &$transportation_expense) {
      $transportation_expense['type'] = $transportation_expense_types[$transportation_expense['type']] ?? null;
    }
  }
  if (!empty($values['parking_and_tolls'])) {
    foreach ($values['parking_and_tolls'] as &$parking_or_toll_expense) {
      $parking_or_toll_expense['type'] = $parking_or_toll_expense_types[$parking_or_toll_expense['type']] ?? null;
    }
  }
  if (!empty($values['hotel_payment'])) {
    $values['hotel_payment'] = $hotel_payment_methods[$values['hotel_payment']] ?? null;
  }
  if (!empty($values['meals_and_incidentals'])) {
    $values['meals_and_incidentals'] = $meals_and_incidentals[$values['meals_and_incidentals']] ?? null;
  }
  if (!empty($values['reimburse_to'])) {
    $values['reimburse_to'] = $reimburse_to[$values['reimburse_to']] ?? null;
  }
  $request = \Drupal\ebms_travel\Entity\ReimbursementRequest::create($values);
  $request->save();
  $created++;
}
$elapsed = round(microtime(TRUE) - $start);
log_success("Successfully loaded: $created reimbursement requests", $elapsed);
