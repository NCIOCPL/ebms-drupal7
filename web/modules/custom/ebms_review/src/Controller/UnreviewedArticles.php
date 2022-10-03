<?php

namespace Drupal\ebms_review\Controller;

use Drupal\Core\Controller\ControllerBase;
use Drupal\ebms_article\Entity\Article;
use Symfony\Component\HttpFoundation\Response;

/**
 * Articles for which we've given up on getting reviews.
 *
 * See https://tracker.nci.nih.gov/browse/OCEEBMS-426.
 */
class UnreviewedArticles extends ControllerBase {

  /**
   * Tag the articles for which we're not going to get reviews.
   */
  public function tag(): Response {
    try {
      $lines = self::applyTags();
      $report = implode("\n", $lines) . "\n" . count($lines) . " article/topics marked\n";
      $response = new Response($report);
      $response->headers->set('Content-type', 'text/plain');
      return $response;
    }
    catch (\Exception $e) {
      $response = new Response("failure: $e\n");
      $response->headers->set('Content-type', 'text/plain');
      return $response;
    }
  }

  /**
   * Find the lost causes and tag the ones which aren't already tagged.
   *
   * The query logic is too complicated for the entiry query API.
   *
   * @return array
   *   Sequence of strings reporting on the articles we've tagged.
   */
  public static function applyTags(): array {

    // Get some vocabulary IDs we'll need.
    $storage = \Drupal::entityTypeManager()->getStorage('taxonomy_term');
    $query = $storage->getQuery()->accessCheck(FALSE);
    $query->condition('vid', 'article_tags');
    $query->condition('field_text_id', 'not_reviewed');
    $not_reviewed_id = reset($query->execute());
    $query = $storage->getQuery()->accessCheck(FALSE);
    $query->condition('vid', 'states');
    $query->condition('field_text_id', 'fyi');
    $fyi_id = reset($query->execute());

    // Find out which article/topic combinations have already been tagged.
    $query = \Drupal::database()->select('ebms_article_tag', 'article_tag');
    $query->condition('article_tag.tag', $not_reviewed_id);
    $query->join('ebms_article_topic__tags', 'tags', 'tags.tags_target_id = article_tag.id');
    $query->join('ebms_article_topic', 'article_topic', 'article_topic.id = tags.entity_id');
    $query->join('ebms_article__topics', 'topics', 'topics.topics_target_id = article_topic.id');
    $query->addField('topics', 'entity_id', 'article_id');
    $query->addField('article_topic', 'topic', 'topic_id');
    $query->distinct();
    $results = $query->execute();
    $already_tagged = [];
    foreach ($results as $result) {
      $article_id = $result->article_id;
      $topic_id = $result->topic_id;
      $key = "$article_id|$topic_id";
      $already_tagged[$key] = $key;
    }

    // Find the articles which have been languishing.
    $cutoff = date('Y-m-d', strtotime('-2 year'));
    $query = \Drupal::database()->select('ebms_packet_article', 'packet_article');
    $query->join('ebms_packet__articles', 'articles', 'articles.articles_target_id = packet_article.id');
    $query->join('ebms_packet', 'packet', 'packet.id = articles.entity_id');
    $query->join('ebms_topic', 'topic', 'topic.id = packet.topic');
    $query->join('ebms_state', 'state', 'state.article = packet_article.article AND state.topic = topic.id');
    $query->leftJoin('ebms_packet_article__reviews', 'reviews', 'reviews.entity_id = packet_article.id');
    $query->condition('packet.active', 1);
    $query->condition('packet.created', $cutoff, '<');
    $query->condition('state.entered', $cutoff, '<=');
    $query->condition('packet.created', Article::CONVERSION_DATE, '>');
    $query->condition('state.current', 1);
    $query->condition('state.value', $fyi_id, '<>');
    $query->condition('packet_article.dropped', 0);
    $query->isNull('reviews.reviews_target_id');
    $query->addField('state', 'article', 'article_id');
    $query->addField('topic', 'id', 'topic_id');
    $query->addField('topic', 'name', 'topic_name');
    $query->distinct();
    $query->orderBy('article_id');
    $query->orderBy('topic_name');
    $results = $query->execute();
    $lines = [];
    foreach ($results as $result) {
      $key = $result->article_id . '|' . $result->topic_id;
      if (!array_key_exists($key, $already_tagged)) {
        $article = Article::load($result->article_id);
        $article->addTag('not_reviewed', $result->topic_id);
        $message = 'marked article ' . $result->article_id . ' as unreviewed for topic ' . $result->topic_name;
        \Drupal::logger('ebms_review')->info($message);
        $lines[] =  $message;
      }
    }
    return $lines;
  }

}
