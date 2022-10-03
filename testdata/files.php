<?php

require(__DIR__ . '/console-log.php');

// Find out where the data is.
$repo_base = getenv('REPO_BASE') ?: '/var/www/ebms';

$mime_map = [
  'application/msword' => file_get_contents("$repo_base/testdata/test.doc"),
  'application/pdf' => file_get_contents("$repo_base/testdata/test.pdf"),
  'application/vnd.openxmlformats-officedocument.presentationml.presentation' => file_get_contents("$repo_base/testdata/test.pptx"),
  'application/vnd.openxmlformats-officedocument.wordprocessingml.document' => file_get_contents("$repo_base/testdata/test.docx"),
];
$json = file_get_contents("$repo_base/testdata/files.json");
$files = json_decode($json, true);
$n = 0;
$done = [];
foreach ($files as $values) {
  $fid = $values['fid'];
  $basename = substr($values['uri'], 9);
  $path = "$repo_base/web/sites/default/files/$basename";
  if (array_key_exists($path, $done)) {
    log_warning(("DUPLICATE for $path"));
  }
  $done[$path] = $path;
  $data = $mime_map[$values['filemime']];
  $values['filesize'] = strlen($data);
  if (file_put_contents($path, $data) !== FALSE) {
    $file = \Drupal\file\Entity\File::create($values);
    $file->save();
    $n++;
  }
  else {
    log_warning("FAILURE for $basename");
  }
}
log_success("Successfully loaded: $n files");
