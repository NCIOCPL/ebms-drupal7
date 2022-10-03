<?php

require(__DIR__ . '/console-log.php');

// Find out where the data is.
$repo_base = getenv('REPO_BASE') ?: '/var/www/ebms';

$start = microtime(TRUE);
$n = 0;
$json = file_get_contents("$repo_base/migration/maps.json");
$map = json_decode($json, TRUE)['doc_tags'];
$fp = fopen("$repo_base/migration/exported/docs.json", 'r');
while (($line = fgets($fp)) !== FALSE) {
  $values = json_decode($line, TRUE);
  if (!empty($values['tags'])) {
    $tags = [];
    foreach ($values['tags'] as $tag) {
      $tags[] = $map[$tag];
    }
    $values['tags'] = $tags;
  }
  $doc = \Drupal\ebms_doc\Entity\Doc::create($values);
  $doc->save();
  $n++;
}
$elapsed = round(microtime(TRUE) - $start);
log_success("Successfully loaded: $n docs", $elapsed);
