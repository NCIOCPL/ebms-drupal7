<?php

require(__DIR__ . '/console-log.php');

// Find out where the assets go.
$repo_base = getenv('REPO_BASE') ?: '/var/www/ebms';
$theme = "$repo_base/web/themes/custom/ebms";

// Recursively remove a directory and all of its contents.
// PHP's standard library has some holes in it. Makes you appreciate Python.
function rrmdir($dir) {
  if (is_dir($dir)) {
    $objects = scandir($dir);
    foreach ($objects as $object) {
      if ($object != '.' && $object != '..') {
        if (is_dir($dir . DIRECTORY_SEPARATOR . $object) && !is_link($dir . '/' . $object))
          rrmdir($dir . DIRECTORY_SEPARATOR . $object);
        else
          unlink($dir . DIRECTORY_SEPARATOR . $object);
      }
    }
    rmdir($dir);
  }
}

// Fetch the assets from USWDS.
$start = microtime(TRUE);
$version = '3.0.0';
$url = "https://github.com/uswds/uswds/releases/download/v$version/uswds-uswds-$version.tgz";
$ch = curl_init();
curl_setopt($ch, CURLOPT_URL, $url);
curl_setopt($ch, CURLOPT_SSL_VERIFYPEER, FALSE);
curl_setopt($ch, CURLOPT_SSL_VERIFYHOST, FALSE);
curl_setopt($ch, CURLOPT_RETURNTRANSFER, TRUE);
curl_setopt($ch, CURLOPT_FOLLOWLOCATION, TRUE);
$bytes = curl_exec($ch);
if (!$bytes) {
  $err = curl_error($ch);
  log_error("Unable to retrieve assets from USWDS: $err");
}
else {
  $tar_path = '/tmp/uswds.tgz';
  if (file_exists($tar_path))
    unlink($tar_path);
  file_put_contents($tar_path, $bytes);

  // Unpack the file to the theme's package directory.
  try {
    $tar = new \PharData($tar_path);
    if (file_exists("$theme/package"))
      rrmdir("$theme/package");
    $tar->extractTo($theme);
    $elapsed = round(microtime(TRUE) - $start);
    log_success('Successfully loaded: USWDS assets', $elapsed);
  }
  catch (UnexpectedValueException $e) {
    log_error('Unable to load USWDS assets');
  }
}
