<?php

namespace Drupal\ebms_review\Controller;

use Drupal\Core\Controller\ControllerBase;
use Drupal\Core\Url;
use Drupal\ebms_review\Entity\Packet;
use Drupal\file\Entity\File;

/**
 * Describe a packet this board member has completed.
 */
class CompletedPacket extends ControllerBase {

  /**
   * Create the render array for the packet.
   */
  public function display(int $packet_id) {

    $packet = Packet::load($packet_id);

    // Get the summaries posted for the packet.
    $summaries = [];
    foreach ($packet->summaries as $summary) {
      $summaries[] = [
        'title' => $summary->entity->description->value,
        'url' => $summary->entity->file->entity->createFileUrl(),
      ];
    }

    // Get the articles in the packet.
    $articles = [];
    foreach ($packet->articles as $packet_article) {
      $article = $packet_article->entity->article->entity;
      $authors = $article->getAuthors(3);
      $full_text_url = '';
      if (!empty($article->full_text->file)) {
        $file = File::load($article->full_text->file);
        $full_text_url = $file->createFileUrl();
      }

      // Collect the reviews of the article.
      $reviews = [];
      foreach ($packet_article->entity->reviews as $review) {
        $dispositions = [];
        foreach ($review->entity->dispositions as $disposition) {
          $dispositions[] = $disposition->entity->name->value;
        }
        $reasons = [];
        foreach ($review->entity->reasons as $reason) {
          $reasons[] = $reason->entity->name->value;
        }
        $reviews[] = [
          'reviewer' => $review->entity->reviewer->entity->name->value,
          'posted' => $review->entity->posted->value,
          'dispositions' => $dispositions,
          'reasons' => $reasons,
          'comments' => $review->entity->comments->value,
          'loe_info' => $review->entity->loe_info->value,
        ];
      }

      // Sort the reviews by when they were posted.
      usort($reviews, function(array $a, array $b): int {
        return $a['posted'] <=> $b['posted'];
      });

      // Get the topic-specific values.
      $article_topic = $article->getTopic($packet->topic->target_id);
      $high_priority = FALSE;
      foreach ($article_topic->tags as $tag) {
        if ($tag->entity->tag->entity->field_text_id->value === 'high_priority') {
          if (!empty($tag->entity->active->value)) {
            $high_priority = TRUE;
            break;
          }
        }
      }
      $comments = [];
      foreach ($article_topic->comments as $comment) {
        $comments[] = $comment->comment;
      }
      $current_state = $article->getCurrentState($packet->topic->target_id);
      $fyi = $current_state->value->entity->field_text_id->value === 'fyi';

      // Assemble the values for this article.
      $articles[] = [
        'title' => $article->title->value,
        'authors' => $authors,
        'publication' => $article->getLabel(),
        'pmid' => $article->source_id->value,
        'full_text_url' => $full_text_url,
        'dropped' => $packet_article->entity->dropped->value,
        'high_priority' => $high_priority,
        'comments' => $comments,
        'fyi' => $fyi,
        'reviews' => $reviews,
      ];
    }

    // Sort the articles by first author name.
    usort($articles, function(array $a, array $b): int {
      if (empty($a['authors'])) {
        if (empty($b['authors'])) {
          return $a['title'] <=> $b['title'];
        }
        return -1;
      }
      elseif (empty($b['authors'])) {
        return 1;
      }
      return $a['authors'][0] <=> $b['authors'][0];
    });

    // Get the documents uploaded by the reviewers.
    $uploads = [];
    foreach ($packet->reviewer_docs as $reviewer_doc) {
      if (empty($reviewer_doc->entity->dropped->value)) {
        $uploads[] = [
          'url' => $reviewer_doc->entity->file->entity->createFileUrl(),
          'filename' => $reviewer_doc->entity->file->entity->getFilename(),
          'notes' => $reviewer_doc->entity->description->value,
          'user' => $reviewer_doc->entity->reviewer->entity->name->value,
          'date' => $reviewer_doc->entity->posted->value,
        ];
      }
    }

    // Assemble the render array for the page.
    $options = ['query' => ['referer' => 'ebms_review.completed_packet']];
    return [
      '#cache' => ['max-age' => 0],
      '#attached' => ['library' => ['ebms_review/completed-packet']],
      '#title' => $packet->title->value,
      'packet' => [
        '#theme' => 'completed_packet',
        '#summaries' => $summaries,
        '#articles' => $articles,
        '#uploads' => $uploads,
        '#upload_url' => Url::fromRoute('ebms_review.reviewer_doc_form', ['packet_id' => $packet_id], $options),
      ],
    ];
  }

}
