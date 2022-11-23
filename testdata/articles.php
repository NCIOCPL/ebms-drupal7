<?php

require(__DIR__ . '/console-log.php');

// Find out where the data is.
$repo_base = getenv('REPO_BASE') ?: '/var/www/ebms';

// Load the maps.
$json = file_get_contents("$repo_base/testdata/maps.json");
$maps = json_decode($json, true);

// Add extra indexes. Can't do this in the ebms_state module, as recommended
// by Berdir (see https://drupal.stackexchange.com/questions/221410) because
// one of the fields (columns) is added later when the ebms_article module is
// installed. So we do it by hand here.
\Drupal::database()->query(
  'CREATE INDEX ebms_state__current_by_board ' .
  'ON ebms_state (value, current, board, article)'
);
\Drupal::database()->query(
  'CREATE INDEX ebms_state__current_by_topic ' .
  'ON ebms_state (value, current, topic, article)'
);
\Drupal::database()->query(
  'CREATE INDEX ebms_state__comments__entered ' .
  'ON ebms_state__comments (comments_entered, entity_id)'
);
\Drupal::database()->query(
  'CREATE INDEX ebms_article_tag__comments__entered ' .
  'ON ebms_article_tag__comments (comments_entered, entity_id)'
);

// Load the states.
$path = "$repo_base/testdata/article_states.json";
$data = json_decode(file_get_contents($path), TRUE);
foreach ($data as $values) {
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
}
$n = count($data);
log_success("Successfully loaded: $n article states");

// Load the tags.
$path = "$repo_base/testdata/article_tags.json";
$data = json_decode(file_get_contents($path), TRUE);
foreach ($data as $values) {
  $values['tag'] = $maps['article_tags'][$values['tag']];
  $tag = \Drupal\ebms_article\Entity\ArticleTag::create($values);
  $tag->save();
}
$n = count($data);
log_success("Successfully loaded: $n article tags");

// Load the topics.
$path = "$repo_base/testdata/article_topics.json";
$data = json_decode(file_get_contents($path), TRUE);
foreach ($data as $values) {
  $topic = \Drupal\ebms_article\Entity\ArticleTopic::create($values);
  $topic->save();
}
$n = count($data);
log_success("Successfully loaded: $n article topics");

// Load the articles.
$json = file_get_contents("$repo_base/testdata/articles.json");
$data = json_decode($json, true);
foreach ($data as $values) {
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
}
$n = count($data);
log_success("Successfully loaded: $n articles");

$map = $maps['relationship_types'];
$json = file_get_contents("$repo_base/testdata/relationships.json");
$relationships = json_decode($json, TRUE);
foreach ($relationships as $values) {
  $values['type'] = $map[$values['type']];
  $relationship = \Drupal\ebms_article\Entity\Relationship::create($values);
  $relationship->save();
}
$n = count($relationships);
log_success("Successfully loaded: $n relationships");
