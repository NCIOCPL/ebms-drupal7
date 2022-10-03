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
    'vid' => 'print_job_modes',
    'values' => [
      ['', 'live'],
      ['', 'test'],
      ['', 'report'],
    ],
    'label' => 'print job modes',
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
  log_success("Successfully loaded: $n $label");;
}

// Load vocabularies pulled from the EBMS custom tables.
$maps = [];
$vocabularies = [
  ['tags.json', 'article_tags', 'article tags'],
  ['board_decisions.json', 'board_decisions', 'board decisions'],
  ['document_tags.json', 'doc_tags', 'document tags'],
  ['import_dispositions.json', 'import_dispositions', 'import dispositions'],
  ['internal_tags.json', 'internal_tags', 'internal tags'],
  ['print_statuses.json', '', 'print job statuses'],
  ['print_job_types.json', '', 'print job types'],
  ['rejection_reasons.json', 'rejection_reasons', 'rejection reasons'],
  ['relationship_types.json', 'relationship_types', 'relationship types'],
  ['dispositions.json', 'dispositions', 'review dispositions'],
  ['states.json', 'states', 'states'],
  ['topic_groups.json', 'topic_groups', 'topic groups'],
];
foreach ($vocabularies as list($filename, $mapname, $label)) {
  $json = file_get_contents("$repo_base/testdata/$filename");
  $data = json_decode($json, true);
  foreach ($data as $values) {
    if (!array_key_exists('status', $values)) {
      throw new Exception("missing status in $filename");
    }
    if (!empty($mapname)) {
      $id = $values['id'];
      unset($values['id']);
    }
    if (!empty($values['description'])) {
      if (!str_ends_with($values['description'], '.'))
        $values['description'] .= '.';
    }
    $term = \Drupal\taxonomy\Entity\Term::create($values);
    $term->save();
    if (!empty($mapname))
      $maps[$mapname][$id] = $term->id();
  }
  $n = count($data);
  log_success("Successfully loaded: $n $label");
}

// Make the ID maps available to other loaders.
$fp = fopen("$repo_base/testdata/maps.json", 'w');
fwrite($fp, json_encode($maps, JSON_PRETTY_PRINT));
//$n = count($maps);
//echo "$n ID maps saved\n";
