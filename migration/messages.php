<?php

require(__DIR__ . '/console-log.php');

// Find out where the data is.
$repo_base = getenv('REPO_BASE') ?: '/var/www/ebms';

// Load the group ID mappings.
$start = microtime(TRUE);
$json = file_get_contents("$repo_base/unversioned/maps.json");
$maps = json_decode($json, TRUE);

// Load the messages.
$fp = fopen("$repo_base/unversioned/exported/messages.json", 'r');
$n = 0;
$legacy_group_types = ['subgroups', 'ad_hoc_groups'];
while (($line = fgets($fp)) !== FALSE) {
  $values = json_decode($line, TRUE);

  // Map the groups.
  $groups = [];
  foreach ($legacy_group_types as $key) {
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

  // Create and save the message enttty.
  $message = \Drupal\ebms_message\Entity\Message::create($values);
  $message->save();
  $n++;
}
$elapsed = round(microtime(TRUE) - $start);
log_success("Successfully loaded: $n messages", $elapsed);
