<?php

require(__DIR__ . '/console-log.php');

// Find out where the data is.
$repo_base = getenv('REPO_BASE') ?: '/var/www/ebms';

// Load the group ID mappings.
$json = file_get_contents("$repo_base/unversioned/maps.json");
$maps = json_decode($json, true);

// Find out if we're creating SSO logins.
$start = microtime(TRUE);
$authmap = [];
$sso = \Drupal::moduleHandler()->moduleExists('externalauth');
if ($sso) {
  $db = \Drupal::database();
  $db->query('DELETE FROM authmap')->execute();
  $authmap = [];
  $fp = fopen("$repo_base/unversioned/exported/authmap.json", 'r');
  while (($line = fgets($fp)) !== FALSE) {
    $values = json_decode($line, TRUE);
    $db->insert('authmap')->fields($values)->execute();
    $authmap[$values['uid']] = $values;
  }
}

// Load the users.
$password = trim(file_get_contents("$repo_base/unversioned/userpw"));
$n = 0;
$fp = fopen("$repo_base/unversioned/exported/users.json", 'r');
while (($line = fgets($fp)) !== FALSE) {
  $values = json_decode($line, TRUE);
  if (!$sso) {
    $values['pass'] = $password;
  }
  elseif (str_starts_with($values['name'], 'Test ')) {
    if (!array_key_exists($values['uid'], $authmap)) {
      $values['pass'] = $password;
    }
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
  $user = \Drupal\user\Entity\User::create($values);
  $user->save();
  $n++;
}
$elapsed = round(microtime(TRUE) - $start);
log_success("Successfully loaded: $n users", $elapsed);
