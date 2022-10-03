<?php

namespace Drupal\ebms_review\Controller;

use Drupal\Core\Controller\ControllerBase;
use Drupal\ebms_review\Entity\PacketArticle;

/**
 * Show reviews of the article by other board members.
 */
class OtherReviews extends ControllerBase {
  public function display(int $packet_article_id) {
    $packet_article = PacketArticle::load($packet_article_id);
    $article = $packet_article->article->entity;
    $title = $article->title->value;
    $reviews = [];
    $current_user = $this->currentUser();
    $uid = $current_user->id();
    foreach ($packet_article->reviews as $review) {
      $review = $review->entity;
      if ($review->reviewer->target_id != $uid) {
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
          'date' => substr($review->posted->value, 0, 10),
          'comments' => $review->comments->value ?? '',
          'loe' => $review->loe_info->value ?? '',
          'dispositions' => $dispositions,
          'reasons' => $reasons,
        ];
      }
    }
    return [
      '#title' => $title,
      '#attached' => ['library' => ['ebms_review/other-reviews']],
      '#cache' => ['max-age' => 0],
      'reviews' => [
        '#theme' => 'other_reviews',
        '#reviews' => $reviews,
      ],
    ];
  }

}
