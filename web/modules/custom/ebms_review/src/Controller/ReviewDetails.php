<?php

namespace Drupal\ebms_review\Controller;

use Drupal\Core\Controller\ControllerBase;
use Drupal\Core\Url;
use Drupal\ebms_review\Entity\Packet;
use Drupal\ebms_review\Entity\PacketArticle;
use Drupal\file\Entity\File;

/**
 * Show the reviews for an article.
 */
class ReviewDetails extends ControllerBase {

  /**
   * Show the review details for a single article.
   *
   * The board manager, wanting to see more information than the thumbs-up/
   * thumbs-down icons in the page showing the whole packet, has clicked
   * on the "SHOW DETAILS" link for one of the articles in the packet.
   * Show all the information we have about the reviews submitted by the
   * board members for the article.
   */
  public function display(int $packet_id, int $packet_article_id) {

    // Load the relevant entities.
    $packet = Packet::load($packet_id);
    $packet_article = PacketArticle::load($packet_article_id);

    // Get the article information to be displayed.
    $article = $packet_article->article->entity;
    $authors = $article->getAuthors(3);
    if (empty($authors)) {
      $authors = ['[No authors listed.]'];
    }
    $full_text_url = '';
    if (!empty($article->full_text->file)) {
      $file = File::load($article->full_text->file);
      $full_text_url = $file->createFileUrl();
    }

    // Collect detailed information about each review.
    $reviews = [];
    foreach ($packet_article->reviews as $article_review) {
      $review = $article_review->entity;
      $dispositions = [];
      foreach ($review->dispositions as $disposition) {
        $dispositions[] = $disposition->entity->name->value;
      }
      $reasons = [];
      foreach ($review->reasons as $reason) {
        $reasons[] = $reason->entity->name->value;
      }
      $reviews[] = [
        'reviewer' => $review->reviewer->entity->name->value,
        'posted' => $review->posted->value,
        'dispositions' => $dispositions,
        'reasons' => $reasons,
        'comments' => $review->comments->value,
        'loe_info' => $review->loe_info->value,
      ];
    }

    // Assemble and return the render array for the page.
    return [
      '#attached' => ['library' => ['ebms_review/reviewed-article']],
      '#cache' => ['max-age' => 0],
      '#title' => $packet->title->value,
      'details' => [
        '#theme' => 'reviewed_article',
        '#authors' => $authors,
        '#title' => $article->title->value,
        '#publication' => $article->getLabel(),
        '#url' => Url::fromRoute('ebms_article.article', ['article' => $article->id()]),
        '#pmid' => $article->source_id->value,
        '#id' => $article->id(),
        '#full_text_url' => $full_text_url,
        '#reviews' => $reviews,
      ],
    ];
  }

}
