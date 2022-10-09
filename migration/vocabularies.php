<?php

require(__DIR__ . '/console-log.php');

// Find out where the data is.
$repo_base = getenv('REPO_BASE') ?: '/var/www/ebms';

// Load the vocabularies which have no integer IDs in the existing system.
$vocabularies = [
  [
    'vid' => 'hotel_payment_methods',
    'values' => [
      ['nci_paid', 'NCI paid for my hotel'],
      ['i_paid', 'I paid for my hotel'],
      ['no_hotel', 'I did not stay in a hotel'],
    ],
    'label' => 'hotel payment methods',
  ],
  [
    'vid' => 'hotels',
    'values' => [
      ['gbcourtyard', 'Courtyard Gaithersburg Washingtonian Center'],
      ['gbmarriot', 'Gaithersburg Marriott Washingtonian Center'],
      ['hhsuites', 'Homewood Suites by Hilton Rockville-Gaithersburg'],
      ['gbresidence', 'Residence Inn Gaithersburg Washingtonian Center'],
    ],
    'label' => 'hotels',
  ],
  [
    'vid' => 'import_types',
    'values' => [
      ['R', 'Regular import'],
      ['F', 'Fast-track import'],
      ['S', 'Special search import'],
      ['D', 'Data refresh from source'],
      ['I', 'Internal import'],
    ],
    'label' => 'import types',
  ],
  [
    'vid' => 'meals_and_incidentals',
    'values' => [
      ['per_diem', 'Per diem requested'],
      ['per_diem_declined', 'Per diem declined'],
      ['per_diem_ineligible', 'I am not eligible to receive a per diem'],
    ],
    'label' => 'per-diem expense options',
  ],
  [
    'vid' => 'meeting_categories',
    'values' => [
      ['', 'Board'],
      ['', 'Subgroup'],
    ],
    'label' => 'meeting categories',
  ],
  [
    'vid' => 'meeting_statuses',
    'values' => [
      ['', 'Scheduled'],
      ['', 'Canceled'],
    ],
    'label' => 'meeting statuses',
  ],
  [
    'vid' => 'meeting_types',
    'values' => [
      ['', 'In Person'],
      ['', 'Webex/Phone Conf.'],
    ],
    'label' => 'meeting types',
  ],
  [
    'vid' => 'parking_or_toll_expense_types',
    'values' => [
      ['airport', 'Airport Parking'],
      ['hotel', 'Hotel Parking'],
      ['toll', 'Toll'],
    ],
    'label' => 'parking/toll expense values',
  ],
  [
    'vid' => 'reimbursement_to',
    'values' => [
      ['work', 'Work'],
      ['home', 'Home'],
      ['other', 'Other'],
    ],
    'label' => 'reimbursement destinations',
  ],
  [
    'vid' => 'transportation_expense_types',
    'values' => [
      ['taxi', 'Taxi'],
      ['metro', 'Metro'],
      ['shuttle', 'Shuttle'],
      ['private', 'Privately Owned Vehicle'],
    ],
    'label' => 'transportation expense types',
  ],
];
foreach ($vocabularies as $vocabulary) {
  $start = microtime(TRUE);
  $weight = 10;
  foreach ($vocabulary['values'] as list($text_id, $name)) {
    $values = [
      'vid' => $vocabulary['vid'],
      'name' => $name,
      'status' => True,
      'weight' => $weight,
    ];
    $weight += 10;
    if (!empty($text_id)) {
      $values['field_text_id'] = $text_id;
    }
    $term = \Drupal\taxonomy\Entity\Term::create($values);
    $term->save();
  }
  $n = count($vocabulary['values']);
  $label = $vocabulary['label'];
  $elapsed = round(microtime(TRUE) - $start);
  log_success("Successfully loaded: $n $label", $elapsed);
}

// Load vocabularies pulled from the EBMS custom tables.
$maps = [];
$vocabularies = [
  'article_tags',
  'board_decisions',
  'doc_tags',
  'import_dispositions',
  'internal_tags',
  'rejection_reasons',
  'relationship_types',
  'review_dispositions',
  'states',
  'topic_groups',
];
foreach ($vocabularies as $name) {
  $start = microtime(TRUE);
  $path = "$repo_base/unversioned/exported/$name" . '_vocabulary.json';
  $fp = fopen($path, 'r');
  $n = 0;
  while (($line = fgets($fp)) !== FALSE) {
    $values = json_decode($line, TRUE);
    if (!array_key_exists('status', $values)) {
      throw new Exception("missing status in $path");
    }
    $id = '';
    if (!empty($values['id'])) {
      $id = $values['id'];
      unset($values['id']);
    }
    if (!empty($values['description'])) {
      $description = $values['description'];
      if (is_array($description)) {
        $description = $description[0];
      }
      if (!str_ends_with($description, '.')) {
        $description .= '.';
      }
      $values['description'] = $description;
    }
    $term = \Drupal\taxonomy\Entity\Term::create($values);
    $term->save();
    if (!empty($id))
      $maps[$name][$id] = $term->id();
    $n++;
  }
  $label = str_replace('_', ' ', $name);
  $elapsed = round(microtime(TRUE) - $start);
  log_success("Successfully loaded: $n $label", $elapsed);
}

// Make the ID maps available to other loaders.
$fp = fopen("$repo_base/unversioned/maps.json", 'w');
fwrite($fp, json_encode($maps, JSON_PRETTY_PRINT));
fclose($fp);
