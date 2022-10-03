<?php

require(__DIR__ . '/console-log.php');

// Find out where the data is.
$repo_base = getenv('REPO_BASE') ?: '/var/www/ebms';

$json = file_get_contents("$repo_base/testdata/users.json");
$password = trim(file_get_contents("$repo_base/userpw"));
$users = json_decode($json, true);
foreach ($users as $values) {
  unset($values['mail']);
  $values['pass'] = $password;
  $user = \Drupal\user\Entity\User::create($values);
  $user->save();
}
$n = count($users);
log_success("Successfully loaded: $n users");
