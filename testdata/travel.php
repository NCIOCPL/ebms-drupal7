<?php

require(__DIR__ . '/console-log.php');

// Suppress sidebar blocks we don't want.
$storage = \Drupal::entityTypeManager()->getStorage('block');
$query = $storage->getQuery()->accessCheck(FALSE);
$query->condition('region', 'sidebar_first');
foreach ($storage->loadMultiple($query->execute()) as $block) {
  if (str_starts_with($block->id(), 'bartik')) {
    $block->setStatus(FALSE);
    $block->save();
  }
}

// Find out where the data is.
$repo_base = getenv('REPO_BASE') ?: '/var/www/ebms';

// Install the travel configuration values.
$config_factory = \Drupal::service('config.factory');
$instructions = [
  'hotel' => 'hotel-form.html',
  'reimbursement' => 'reimbursement-form.html',
];
$config = $config_factory->getEditable('ebms_travel.instructions');
foreach ($instructions as $name => $filename) {
  $path = "$repo_base/testdata/$filename";
  $config->set($name, trim(file_get_contents($path)));
}
$config->save();
$config = $config_factory->getEditable('ebms_travel.email');
$config->set('travel_manager', 'klem@example.gov');
$config->set('developers', 'klem@example.gov');
$config->save();
log_success('Successfully loaded: 4 travel config values');

// Install the travel pages.
$values = [
  'title' => 'Directions',
  'uid' => 1,
  'type' => 'page',
  'path' => ['alias' => '/travel/directions'],
  'body' => [
    'value' => file_get_contents("$repo_base/testdata/travel-directions.html"),
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
    'value' => file_get_contents("$repo_base/testdata/travel-policies.html"),
    'format' => 'filtered_html',
  ],
];
$page = \Drupal\node\Entity\Node::create($values);
$page->save();
$filenames = [
  'hotel-request-202009.docx',
  'participant-info-202009.doc',
  'expense-statement-202111.docx',
];
$file_usage = \Drupal::service('file.usage');
foreach ($filenames as $filename) {
  $data = file_get_contents("$repo_base/testdata/$filename");
  file_put_contents("$repo_base/web/sites/default/files/$filename", $data);
  if (str_ends_with($filename, '.docx'))
    $mimetype = 'vnd.openxmlformats-officedocument.wordprocessingml.document';
  else
    $mimetype = 'msword';
  $values = [
    'filename' => $filename,
    'uid' => 1,
    'uri' => "public://$filename",
    'filemime' => "application/$mimetype",
    'status' => 1,
  ];
  $file = \Drupal\file\Entity\File::create($values);
  $file->save();
  $file_usage->add($file, 'node', 'node', $page->id());
}
log_success('Successfully loaded: 3 static travel pages');

// Install the travel requests.
$storage = \Drupal::entityTypeManager()->getStorage('taxonomy_term');
$query = $storage->getQuery();
$query->condition('vid', 'hotels');
$ids = $query->execute();
$terms = $storage->loadMultiple($ids);
$hotels = [];
foreach ($terms as $term)
  $hotels[$term->field_text_id->value] = $term->id();
$json = file_get_contents("$repo_base/testdata/hotel-requests.json");
$data = json_decode($json, true);
foreach ($data as $values) {
  $values['preferred_hotel'] = $hotels[$values['preferred_hotel']] ?? null;
  $request = \Drupal\ebms_travel\Entity\HotelRequest::create($values);
  $request->save();
}
$n = count($data);
log_success("Successfully loaded: $n hotel requests");

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
$json = file_get_contents("$repo_base/testdata/reimbursement-requests.json");
$data = json_decode($json, true);
foreach ($data as $values) {
  if (!empty($values['transportation'])) {
    foreach ($values['transportation'] as &$expense) {
      $expense['type'] = $transportation_expense_types[$expense['type']] ?? null;
      if (empty($expense['amount']))
        unset($expense['amount']);
      if (empty($expense['mileage']))
        unset($expense['mileage']);
    }
  }
  if (!empty($values['parking_and_tolls'])) {
    foreach ($values['parking_and_tolls'] as &$expense) {
      $expense['type'] = $parking_or_toll_expense_types[$expense['type']] ?? null;
      if (empty($expense['amount']))
        unset($expense['amount']);
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
}
$n = count($data);
log_success("Successfully loaded: $n reimbursement requests");
