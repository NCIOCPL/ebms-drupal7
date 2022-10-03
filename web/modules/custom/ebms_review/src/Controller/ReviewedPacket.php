<?php

namespace Drupal\ebms_review\Controller;

use Drupal\Core\Controller\ControllerBase;
use Drupal\Core\Link;
use Drupal\Core\Url;
use Drupal\ebms_review\Entity\Packet;
use Drupal\ebms_review\Entity\Review;
use Drupal\ebms_state\Entity\State;
use Drupal\file\Entity\File;
use Drupal\taxonomy\Entity\Term;

/**
 * Controller for a packet with at least one review.
 */
class ReviewedPacket extends ControllerBase {

  /**
   * Show the articles and summary of reviews for this packet.
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
    $revive = \Drupal::request()->get('revive');
    $show_archived = \Drupal::request()->get('sa');
    $sort = \Drupal::request()->get('sort');
    $filter_id = \Drupal::request()->get('filter-id');
    $current_options = ['query' => []];
    if (!empty($show_archived)) {
      $current_options['query']['sa'] = $show_archived;
    }
    if (!empty($sort)) {
      $current_options['query']['sort'] = $sort;
    }
    if (!empty($filter_id)) {
      $current_options['query']['filter-id'] = $filter_id;
    }

    // Load the packet entity and collect information about each article.
    $packet = Packet::load($packet_id);
    $articles = [];
    $route = 'ebms_review.reviewed_packet';
    $parms = ['packet_id' => $packet_id];
    foreach ($packet->articles as $packet_article) {

      // Walk through each review to see which way the wind is blowing.
      $last_review = $archive_url = $revive_url = '';
      $reviewers = [];
      $checked_reviewers = [];
      foreach ($packet_article->entity->reviews as $review) {
        $rejected = FALSE;
        $posted = $review->entity->posted->value;
        if ($posted > $last_review) {
          $last_review = $posted;
        }
        foreach ($review->entity->dispositions as $disposition) {
          if ($disposition->entity->name->value === Review::NO_CHANGES) {
            $rejected = TRUE;
            break;
          }
        }
        $checked_reviewers[] = $review->entity->reviewer->target_id;
        $reviewers[$rejected ? 'rejected' : 'approved'][] = $review->entity->reviewer->entity->name->value;
      }

      // See which of the assigned reviewers haven't reviewed the article.
      $current_state = $packet_article->entity->article->entity->getCurrentState($packet->topic->target_id);
      $unreviewed = 'unreviewed';
      if (empty($checked_reviewers) && $current_state->value->entity->field_text_id->value === 'fyi') {
        $unreviewed = 'fyi';
      }
      foreach ($packet->reviewers as $reviewer) {
        if (!in_array($reviewer->target_id, $checked_reviewers)) {
          $reviewers[$unreviewed][] = $reviewer->entity->name->value;
        }
      }

      // Toggle the article's "archived" state if so requested.
      if (!empty($archive) && $archive == $packet_article->target_id) {
        $packet_article->entity->set('archived', date('Y-m-d H:i:s'));
        $packet_article->entity->save();
      }
      if (!empty($revive) && $revive == $packet_article->target_id) {
        $packet_article->entity->set('archived', NULL);
        $packet_article->entity->save();
      }

      // Add a button for archiving or reviving the article (as appropriate).
      if (!empty($packet_article->entity->archived->value)) {
        if (empty($show_archived)) {
          continue;
        }
        $options = ['query' => ['revive' => $packet_article->target_id]];
        if (!empty($show_archived)) {
          $options['query']['sa'] = 1;
        }
        if (!empty($filter_id)) {
          $options['query']['filter-id'] = $filter_id;
        }
        $revive_url = Url::fromRoute($route, $parms, $options);
      }
      else {
        $options = ['query' => ['archive' => $packet_article->target_id]];
        if (!empty($show_archived)) {
          $options['query']['sa'] = 1;
        }
        if (!empty($filter_id)) {
          $options['query']['filter-id'] = $filter_id;
        }
        $archive_url = Url::fromRoute($route, $parms, $options);
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
      $articles[] = [
        'authors' => $authors,
        'id' => $article->id(),
        'title' => $article->title->value,
        'pmid' => $article->source_id->value,
        'url' => Url::fromRoute('ebms_article.article', ['article' => $article->id()]),
        'publication' => $article->getLabel(),
        'archive_url' => $archive_url,
        'revive_url' => $revive_url,
        'details_url' => Url::fromRoute('ebms_review.details', ['packet_id' => $packet_id, 'packet_article_id' => $packet_article->target_id], $current_options),
        'full_text_url' => $full_text_url,
        'reviewers' => $reviewers,
        'last_review' => $last_review,
        'comments' => $comments,
        'related' => $related,
        'state' => $current_state->laterStateDescription($full_text_approval->field_sequence->value),
        'tags' => $tags,
        'high_priority' => $high_priority,
      ];
    }

    // Make decisions about which dynamic buttons should appear.
    $show_archived_url = $hide_archived_url = '';
    $options = $current_options;
    if (!empty($show_archived)) {
      unset($options['query']['sa']);
      $hide_archived_url = Url::fromRoute($route, $parms, $options);
    }
    else {
      $options['query']['sa'] = 1;
      $show_archived_url = Url::fromRoute($route, $parms, $options);
    }
    $options = $current_options;

    // Sort the articles.
    if ($sort === 'title') {
      usort($articles, function(array $a, array $b): int {
        return $a['title'] <=> $b['title'];
      });
      $sort_label = 'Most Recent Updates First';
      unset($options['query']['sort']);
    }
    else {
      usort($articles, function(array $a, array $b): int {
        if ($a['last_review'] == $b['last_review']) {
          return $a['title'] <=> $b['title'];
        }
        return $b['last_review'] <=> $a['last_review'];
      });
      $sort_label = 'Sort By Title';
      $options['query']['sort'] = 'title';
    }
    $sort_url = Url::fromRoute($route, $parms, $options);

    // The the rows for the table of documents posted by board members
    // for the packet.
    $rows = [];
    foreach ($packet->reviewer_docs as $doc) {
      $file = $doc->entity->file->entity;
      $filename = $file->filename->value;
      $url = Url::fromUri($file->createFileUrl(FALSE));
      $link = Link::fromTextAndUrl($filename, $url);
      $notes = $doc->entity->description->value;
      $reviewer = $doc->entity->reviewer->entity->name->value;
      $posted = substr($doc->entity->posted->value, 0, 10);
      $rows[] = [$link, $notes, $reviewer, $posted];
    }

    // Return the page's render array.
    $opts = ['query' => []];
    if (!empty($filter_id)) {
      $opts['query']['filter-id'] = $filter_id;
    }
    return [
      '#title' => 'Reviews For ' . $packet->title->value,
      '#attached' => ['library' => ['ebms_review/reviewed-packet']],
      '#cache' => ['max-age' => 0],
      'packet' => [
        '#theme' => 'reviewed_packet',
        '#report_url' => Url::fromRoute('ebms_report.literature_reviews', [], ['query' => ['packet' => $packet_id]]),
        '#archive_url' => Url::fromRoute('ebms_review.archive_packet', ['ebms_packet' => $packet_id], $opts),
        '#show_archived_url' => $show_archived_url,
        '#hide_archived_url' => $hide_archived_url,
        '#sort_url' => $sort_url,
        '#sort_label' => $sort_label,
        '#articles' => $articles,
      ],
      'reviewer-docs' => [
        '#theme' => 'table',
        '#caption' => 'Reviewer Uploads',
        '#header' => ['File Name', 'Notes', 'Uploaded By', 'When Posted'],
        '#rows' => $rows,
        '#empty' => 'No reviewer documents have been posted for this packet yet.',
      ],
    ];
  }

}
