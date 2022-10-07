<?php

require(__DIR__ . '/console-log.php');

// Find out where the data is.
$repo_base = getenv('REPO_BASE') ?: '/var/www/ebms';

// Load the maps.
$json = file_get_contents("$repo_base/unversioned/maps.json");
$maps = json_decode($json, true);

// Add extra indexes. Can't do this in the ebms_state module, as recommended
// by Berdir (see https://drupal.stackexchange.com/questions/221410) because
// one of the fields (columns) is added later when the ebms_article module is
// installed. So we do it by hand here.
$count = \Drupal::database()->query(
  'SELECT COUNT(*) FROM information_schema.statistics ' .
  "WHERE INDEX_NAME = 'ebms_state__current_by_board'"
)->fetchField();
if (empty($count)) {
  \Drupal::database()->query(
    'CREATE INDEX ebms_state__current_by_board ' .
    'ON ebms_state (value, current, board, article)'
  );
}
$count = \Drupal::database()->query(
  'SELECT COUNT(*) FROM information_schema.statistics ' .
  "WHERE INDEX_NAME = 'ebms_state__current_by_topic'"
)->fetchField();
if (empty($count)) {
  \Drupal::database()->query(
    'CREATE INDEX ebms_state__current_by_topic ' .
    'ON ebms_state (value, current, topic, article)'
  );
}

// Load the states.
$n = 0;
$top_start = $start = microtime(TRUE);
$fp = fopen("$repo_base/unversioned/exported/states.json", 'r');
$sql = 'SELECT MAX(id) FROM ebms_state';
$last_loaded = \Drupal::database()->query($sql)->fetchField();
while (($line = fgets($fp)) !== FALSE) {
  $values = json_decode($line, TRUE);
  if ($values['id'] <= $last_loaded) {
    $n++;
    continue;
  }
  $values['value'] = $maps['states'][$values['value']];
  if (!empty($values['decisions'])) {
    $decisions = [];
    foreach ($values['decisions'] as $decision) {
      $decision['decision'] = $maps['board_decisions'][$decision['decision']];
      $decisions[] = $decision;
    }
    $values['decisions'] = $decisions;
  }
  $state = \Drupal\ebms_state\Entity\State::create($values);
  $state->save();
  $n++;
}
$elapsed = round(microtime(TRUE) - $start);
log_success("Successfully loaded: $n article states", $elapsed);

// Load the tags.
$start = microtime(TRUE);
$fp = fopen("$repo_base/unversioned/exported/article_tags.json", 'r');
$n = 0;
while (($line = fgets($fp)) !== FALSE) {
  $values = json_decode($line, TRUE);
  $values['tag'] = $maps['article_tags'][$values['tag']];
  $tag = \Drupal\ebms_article\Entity\ArticleTag::create($values);
  $tag->save();
  $n++;
}
$elapsed = round(microtime(TRUE) - $start);
log_success("Successfully loaded: $n article tags", $elapsed);

// Load the topics.
$n = 0;
$start = microtime(TRUE);
$fp = fopen("$repo_base/unversioned/exported/article_topics.json", 'r');
while (($line = fgets($fp)) !== FALSE) {
  $values = json_decode($line, TRUE);
  $topic = \Drupal\ebms_article\Entity\ArticleTopic::create($values);
  $topic->save();
  $n++;
}
$elapsed = round(microtime(TRUE) - $start);
log_success("Successfully loaded: $n article topics", $elapsed);

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

$start = microtime(TRUE);
$n = 0;
$map = $maps['relationship_types'];
$path = "$repo_base/unversioned/exported/article_relationships.json";
$fp = fopen($path, 'r');
while (($line = fgets($fp)) !== FALSE) {
  $values = json_decode($line, TRUE);
  $values['type'] = $map[$values['type']];
  $relationship = \Drupal\ebms_article\Entity\Relationship::create($values);
  $relationship->save();
  $n++;
}
$elapsed = round(microtime(TRUE) - $start);
log_success("Successfully loaded: $n relationships", $elapsed);
$elapsed = round(microtime(TRUE) - $top_start);
log_success("All article information loaded", $elapsed);
