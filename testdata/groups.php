<?php

require(__DIR__ . '/console-log.php');

// Find out where the data is.
$repo_base = getenv('REPO_BASE') ?: '/var/www/ebms';

// Load the maps generated by vocabularies.php -- we'll add to them.
$json = file_get_contents("$repo_base/testdata/maps.json");
$maps = json_decode($json, true);

// Create the group entities.
$json = file_get_contents("$repo_base/testdata/groups.json");
$groups = json_decode($json, true);
$keys = ['ad_hoc_groups', 'subgroups'];
$n = 0;
foreach ($keys as $key) {
  foreach ($groups[$key] as $id => $values) {
    $group = \Drupal\ebms_group\Entity\Group::create($values);
    $group->save();
    $maps[$key][$id] = $group->id();
    $n++;
  }
}
log_success("Successfully loaded: $n groups");

// Attach group memberships to the user profile entities.
$storage = \Drupal::entityTypeManager()->getStorage('user');
$n = $count = 0;
foreach ($groups['memberships'] as $id => $memberships) {
  $user = $storage->load($id);
  if (!empty($user)) {
    $group_ids = [];
    foreach ($memberships as list($key, $group_id)) {
      $group_ids[] = $maps[$key . 's'][$group_id];
      $count++;
    }
    $user->set('groups', $group_ids);
    $user->save();
    $n++;
  }
}
//echo "added group memberships to $n users\n";
log_success("Successfully loaded: $count group memberships");

// Save the augmented maps.
$fp = fopen("$repo_base/testdata/maps.json", 'w');
fwrite($fp, json_encode($maps, JSON_PRETTY_PRINT));
$n = count($maps);
// echo "$n ID maps saved\n";