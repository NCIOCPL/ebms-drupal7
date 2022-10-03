<?php

require(__DIR__ . '/console-log.php');

// Find out where the data is.
$repo_base = getenv('REPO_BASE') ?: '/var/www/ebms';

// Make sure the directory for the pictures exists.
if (!file_exists("$repo_base/web/sites/default/files/pictures"))
  mkdir("$repo_base/web/sites/default/files/pictures");

// Load the user entities and add the profile images.
$users = \Drupal\user\Entity\User::loadMultiple();
$now = time();
$n = 0;
$file_usage = \Drupal::service('file.usage');
foreach ($users as $user) {
  $name = $user->name->value;
  if (empty($name)) {
    continue;
  }
  $normalized_name = str_replace(' ', '_', str_replace("'", '_', $name));
  $filename = $normalized_name . '.png';
  $bytes = file_get_contents("$repo_base/testdata/identicons/$filename");
  file_put_contents("$repo_base/web/sites/default/files/pictures/$filename", $bytes);
  $values = [
    'uid' => $user->id(),
    'filename' => $filename,
    'uri' => "public://pictures/$filename",
    'filemime' => 'image/png',
    'filesize' => strlen($bytes),
    'created' => $now,
    'changed' => $now,
    'status' => 1,
  ];
  $file = \Drupal\file\Entity\File::create($values);
  $file->save();
  $user->set('user_picture', $file->id());
  $user->save();
  $file_usage->add($file, 'user', 'user', $user->id());
  $n++;
}
log_success("Successfully loaded: $n profile images");
