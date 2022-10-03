<?php

namespace Drupal\ebms_review\Controller;

use Drupal\Core\Controller\ControllerBase;
use Drupal\Core\Link;
use Drupal\Core\Url;
use Drupal\ebms_review\Entity\Packet;
use Drupal\file\Entity\File;
use Drupal\user\Entity\User;

/**
 * Show the articles in the review packet.
 */
class AssignedPacket extends ControllerBase {

  /**
   * Create the render array for the assigned packet page.
   *
   * @param int $packet_id
   *   ID of the assigned packet.
   *
   * @return array
   *   Render array for the packet page.
   */
  public function display($packet_id = NULL): array {

    // Get the reviewers assigned to the packet.
    $packet = Packet::load($packet_id);
    $reviewers = [];
    foreach ($packet->reviewers as $reviewer) {
      $reviewers[] = $reviewer->entity->name->value;
    }
    sort($reviewers);

    // Set up some defaults.
    $title = $packet->title->value;
    $options = ['query' => \Drupal::request()->query->all()];
    $uid = $this->currentUser()->id();

    // Override defaults if working on behalf of a board member.
    $obo = $options['query']['obo'] ?? '';
    if (!empty($obo)) {
      $uid = $obo;
      $user = User::load($uid);
      $name = $user->name->value;
      $title .= " (on behalf of $name)";
    }

    // Get the summary document links for the packet.
    $summaries = [];
    foreach ($packet->summaries as $summary) {
      $label = $summary->entity->description->value;
      $url = $summary->entity->file->entity->createFileUrl(FALSE);
      $summaries[] = Link::fromTextAndUrl($label, Url::fromUri($url));
    }
    // Sort the articles by journal title, core journals first.
    $packet_articles = [];
    foreach ($packet->articles as $article) {
      $packet_articles[] = $article->entity;
    }
    $storage = \Drupal::entityTypeManager()->getStorage('ebms_journal');
    $ids = $storage->getQuery()
      ->accessCheck(FALSE)
      ->condition('core', 1)
      ->execute();
    $journals = $storage->loadMultiple($ids);
    $core_ids = [];
    foreach ($journals as $journal) {
      $core_ids[] = $journal->source_id->value;
    }
    usort($packet_articles, function ($a, $b) use ($core_ids) {
      $a_core = in_array($a->article->entity->source_journal_id, $core_ids);
      $b_core = in_array($b->article->entity->source_journal_id, $core_ids);
      if ($a_core && !$b_core) {
        return -1;
      }
      if (!$a_core && $b_core) {
        return 1;
      }
      $a_journal_title = $a->article->entity->journal_title->value;
      $b_journal_title = $b->article->entity->journal_title->value;
      if ($a_journal_title === $b_journal_title) {
        return $a->article->entity->title->value <=> $b->article->entity->title->value;
      }
      return $a_journal_title <=> $b_journal_title;
    });

    // Build the render array for each article.
    $items = [];
    $topic_id = $packet->topic->target_id;
    $parms = ['packet_id' => $packet_id];
    foreach ($packet_articles as $packet_article) {
      $parms['packet_article_id'] = $packet_article->id();
      $article = $packet_article->article->entity;
      $authors = $article->getAuthors(3);
      if (empty($authors)) {
        $authors = ['[No authors named]'];
      }
      $agendas = $fyi = $review_posted = $board_reviewed = $full_text_url = $other_reviews_url = $review_url = $quick_rejection_url = '';
      if (!empty($article->full_text->file)) {
        $file = File::load($article->full_text->file);
        $full_text_url = $file->createFileUrl();
      }
      $current_state = $article->getCurrentState($topic_id);
      if ($current_state->field_text_id->value === 'on_agenda') {
        $meetings = [];
        foreach ($current_state->meetings as $meeting) {
          $name = $meeting->entity->name->value;
          $date = substr($meeting->entity->dates->value, 0, 10);
          $meetings[] = "$name - $date";
        }
        $meetings = implode('; ', $meetings);
        if (empty($meetings)) {
          $meetings = 'No meetings recorded';
        }
        $agendas = "on agenda(s): $meetings";
      }
      if ($current_state->value->entity->field_text_id->value === 'fyi') {
        $fyi = TRUE;
      }
      else {
        foreach ($packet_article->reviews as $review) {
          $review = $review->entity;
          if ($review->reviewer->target_id == $uid) {
            $review_posted = $review->posted->value;
          }
          elseif (empty($other_reviews_url)) {
            $other_reviews_url = Url::fromRoute('ebms_review.other_reviews', $parms);
          }
        }
        if (empty($review_posted)) {
          if ($current_state->value->entity->field_text_id->value === 'final_board_decision') {
            $board_reviewed = TRUE;
          }
          else {
            $review_url = Url::fromRoute('ebms_review.add_review', $parms, $options);
            $quick_rejection_url = Url::fromRoute('ebms_review.quick_reject', $parms, $options);
          }
        }
      }
      $items[] = [
        '#theme' => 'assigned_article',
        '#article' => [
          'authors' => $authors,
          'title' => $article->title->value,
          'publication' => $article->getLabel(),
          'pmid' => $article->source_id->value,
          'agendas' => $agendas,
          'fyi' => $fyi,
          'review_posted' => $review_posted,
          'board_reviewed' => $board_reviewed,
          'full_text_url' => $full_text_url,
          'other_reviews' => $other_reviews_url,
          'review_url' => $review_url,
          'quick_rejection' => $quick_rejection_url,
        ],
      ];
    }
    $page = [
      '#title' => $title,
      '#attached' => [
        'library' => [
          'core/drupal.dialog.ajax',
          'ebms_review/assigned-packet',
        ],
      ],
      '#cache' => ['max-age' => 0],
      'instructions' => [
        '#theme' => 'item_list',
        '#title' => 'Instructions',
        '#list_type' => 'ol',
        '#items' => [
          'Click on the summary document to open it on your computer so that it is ready for you to make changes.',
          'Review articles. Use the REJECT button to quickly reject any articles that warrant no changes to the summary.',
          'If needed, make changes to the summary using Track Changes and save it to your computer, adding your initials to the filename.',
          'Upload the document with your changes using the Post Reviewer Document button found in the Reviewer Uploads section below.',
        ],
      ],
      'reviewers' => [
        '#theme' => 'item_list',
        '#title' => 'Reviewers',
        '#list_type' => 'ul',
        '#items' => $reviewers,
      ],
      'summaries' => [
        '#theme' => 'item_list',
        '#title' => 'Summaries',
        '#list_type' => 'ul',
        '#items' => $summaries,
        '#attributes' => ['class' => ['usa-list--unstyled']],
        '#empty' => 'No summaries posted for this packet.',
      ],
      'articles' => [
        '#theme' => 'item_list',
        '#title' => 'Articles',
        '#list_type' => 'ol',
        '#items' => $items,
      ],
    ];
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
    $page['reviewer-docs'] = [
      '#theme' => 'table',
      '#caption' => 'Reviewer Uploads',
      '#header' => ['File Name', 'Notes', 'Uploaded By', 'When Posted'],
      '#rows' => $rows,
      '#empty' => 'No reviewer documents have been posted for this packet yet.',
    ];
    $label = 'Post Reviewer Document';
    $route = 'ebms_review.reviewer_doc_form';
    $parms = ['packet_id' => $packet_id];
    $page['reviewer-doc-button'] = [
      '#theme' => 'links',
      '#links' => [
        [
          'url' => Url::fromRoute($route, $parms, $options),
          'title' => 'Post Reviewer Document',
          'attributes' => ['class' => ['button', 'usa-button']],
        ],
      ],
    ];
    return $page;
  }

}
