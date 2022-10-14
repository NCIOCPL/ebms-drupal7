<?php

require(__DIR__ . '/console-log.php');

// Find out where the data is.
$repo_base = getenv('REPO_BASE') ?: '/var/www/ebms';

// Load the maps.
$json = file_get_contents("$repo_base/unversioned/maps.json");
$maps = json_decode($json, true);

// Load the articles.
$n = 0;
$start = microtime(TRUE);
$fp = fopen("$repo_base/unversioned/exported/articles.json", 'r');
while (($line = fgets($fp)) !== FALSE) {
  $values = json_decode($line, TRUE);
  $id = $values['id'];
  $xml = file_get_contents("$repo_base/unversioned/articles/$id.xml");
  $pubmed_values = \Drupal\ebms_article\Entity\Article::parse($xml);
  $values = array_merge($values, $pubmed_values);
  if (!empty($values['internal_tags'])) {
    $tags = [];
    foreach ($values['internal_tags'] as $tag) {
      $tag['tag'] = $maps['internal_tags'][$tag['tag']];
      $tags[] = $tag;
    }
    $values['internal_tags'] = $tags;
  }
  $article = \Drupal\ebms_article\Entity\Article::create($values);
  $article->save();
  $n++;
}
$elapsed = round(microtime(TRUE) - $start);
log_success("Successfully loaded: $n articles", $elapsed);
