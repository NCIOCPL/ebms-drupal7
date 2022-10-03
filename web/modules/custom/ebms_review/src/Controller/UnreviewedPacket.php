<?php

namespace Drupal\ebms_review\Controller;

use Drupal\Core\Controller\ControllerBase;
use Drupal\Core\Url;
use Drupal\ebms_review\Entity\Packet;
use Drupal\file\Entity\File;

/**
 * Controller for a packet with no reviews.
 */
class UnreviewedPacket extends ControllerBase {

  /**
   * Show the articles for this packet.
   *
   * @param int $packet_id
   *   Unique identifier for the packet's entity.
   *
   * @return
   *   Render array for the page.
   */
  public function display(int $packet_id): array {

    // Collect some preliminary information.
    $full_text_approval = \Drupal::service('ebms_core.term_lookup')->getState('passed_full_review');
    $archive = \Drupal::request()->get('archive');
    $filter_id = \Drupal::request()->get('filter-id');
    $packet = Packet::load($packet_id);
    $reviewers = [];
    foreach ($packet->reviewers as $reviewer) {
      $reviewers[] = $reviewer->entity->name->value;
    }
    sort($reviewers);

    // Collect information about each article.
    $articles = [];
    $route = 'ebms_review.unreviewed_packet';
    $parms = ['packet_id' => $packet_id];
    $opts = ['query' => []];
    if (!empty($filter_id)) {
      $opts['query']['filter-id'] = $filter_id;
    }
    foreach ($packet->articles as $packet_article) {

      // Archive the article if so requested.
      if (!empty($archive) && $archive == $packet_article->target_id) {
        $packet_article->entity->set('archived', date('Y-m-d H:i:s'));
        $packet_article->entity->save();
        continue;
      }

      // Skip the article if it has been archived.
      if (!empty($packet_article->entity->archived->value)) {
        continue;
      }

      // Collect the information describing the article itself.
      $article = $packet_article->entity->article->entity;
      $authors = $article->getAuthors(3);
      if (empty($authors)) {
        $authors = ['[No authors listed.]'];
      }
      $full_text_url = '';
      if (!empty($article->full_text->file)) {
        $file = File::load($article->full_text->file);
        $full_text_url = $file->createFileUrl();
      }
      $related = [];
      foreach ($packet_article->entity->article->entity->getRelatedArticles() as $related_article) {
        $related[] = [
          'citation' => $related_article->getLabel(),
          'pmid' => $related_article->source_id->value,
          'id' => $related_article->id(),
          'url' => Url::fromRoute('ebms_article.article', ['article' => $article->id()]),
        ];
      }
      $article_topic = $packet_article->entity->article->entity->getTopic($packet->topic->target_id);
      $comments = [];
      foreach ($article_topic->comments as $topic_comment) {
        $comments[] = $topic_comment->comment;
      }
      $tags = [];
      $high_priority = FALSE;
      foreach ($article_topic->tags as $topic_tag) {
        $tags[] = $topic_tag->entity->tag->entity->name->value;
        if ($topic_tag->entity->tag->entity->field_text_id->value === 'high_priority') {
          $high_priority = TRUE;
        }
      }

      // Add the article's render array to the array of articles.
      $current_state = $packet_article->entity->article->entity->getCurrentState($packet->topic->target_id);
      $opts['query']['archive'] = $packet_article->target_id;
      $articles[] = [
        'authors' => $authors,
        'id' => $article->id(),
        'title' => $article->title->value,
        'pmid' => $article->source_id->value,
        'url' => Url::fromRoute('ebms_article.article', ['article' => $article->id()]),
        'publication' => $article->getLabel(),
        'archive_url' => Url::fromRoute($route, $parms, $opts),
        'full_text_url' => $full_text_url,
        'comments' => $comments,
        'related' => $related,
        'tags' => $tags,
        'high_priority' => $high_priority,
        'state' => $current_state->laterStateDescription($full_text_approval->field_sequence->value),
      ];
    }

    // Sort the articles.
    usort($articles, function(array $a, array $b): int {
      return $a['title'] <=> $b['title'];
    });

    // Return the page's render array, using the "reviewed packet" CSS.
    $opts['query']['unreviewed'] = 1;
    unset($opts['query']['archive']);
    return [
      '#title' => 'Unreviewed Packet ' . $packet->title->value,
      '#attached' => ['library' => ['ebms_review/reviewed-packet']],
      '#cache' => ['max-age' => 0],
      'packet' => [
        '#theme' => 'unreviewed_packet',
        '#archive_url' => Url::fromRoute('ebms_review.archive_packet', ['ebms_packet' => $packet_id], $opts),
        '#articles' => $articles,
        '#reviewers' => $reviewers,
        '#created' => $packet->created->value,
      ],
    ];
  }

}
