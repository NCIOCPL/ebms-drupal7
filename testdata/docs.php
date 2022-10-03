<?php

require(__DIR__ . '/console-log.php');

// Find out where the data is.
$repo_base = getenv('REPO_BASE') ?: '/var/www/ebms';

$json = file_get_contents("$repo_base/testdata/maps.json");
$map = json_decode($json, true)['doc_tags'];
$json = file_get_contents("$repo_base/testdata/docs.json");
$data = json_decode($json, true);
$count = 0;
foreach ($data as $values) {
  if (array_key_exists('tags', $values)) {
    $tags = [];
    foreach ($values['tags'] as $tag) {
      $tags[] = $map[$tag];
    }
    $values['tags'] = $tags;
  }
  $doc = \Drupal\ebms_doc\Entity\Doc::create($values);
  $doc->save();
}
$n = count($data);
log_success("Successfully loaded: $n docs");
