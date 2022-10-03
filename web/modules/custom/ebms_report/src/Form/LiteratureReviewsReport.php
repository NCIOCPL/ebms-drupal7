<?php

namespace Drupal\ebms_report\Form;

use Drupal\Core\Form\FormBase;
use Drupal\Core\Form\FormStateInterface;
use Drupal\Core\Url;
use Drupal\ebms_article\Entity\Article;
use Drupal\ebms_article\Entity\ArticleTopic;
use Drupal\ebms_board\Entity\Board;
use Drupal\ebms_core\Entity\SavedRequest;
use Drupal\ebms_import\Entity\Batch;
use Drupal\ebms_review\Entity\Packet;
use Drupal\ebms_review\Entity\Review;
use Drupal\ebms_topic\Entity\Topic;
use Drupal\user\Entity\User;

/**
 * Report showing reviews of articles assigned to packets.
 *
 * @ingroup ebms
 */
class LiteratureReviewsReport extends FormBase {

  /**
   * {@inheritdoc}
   */
  public function getFormId(): string {
    return 'literature_reviews_report_form';
  }

  /**
   * {@inheritdoc}
   */
  public function buildForm(array $form, FormStateInterface $form_state, int $report_id = 0): array {

    // Collect the values for the report request, if we already have one.
    $params = empty($report_id) ? [] : SavedRequest::loadParameters($report_id);

    // First precedence for board selection goes to an AJAX callback.
    $boards = Board::boards();
    $board_id = $form_state->getValue('board');

    // Next, see if a packet ID was passed in as a query parameter.
    $packet_id = $topic_id = '';
    if (empty($board_id)) {
      $packet_id = $this->getRequest()->query->get('packet');
      if (!empty($packet_id)) {
        $packet = Packet::load($packet_id);
        $topic_id = $packet->topic->target_id;
        $board_id = $packet->topic->entity->board->target_id;
      }
    }

    // If not, see if a report request has been submitted.
    if (empty($board_id)) {
      if (empty($params['board'])) {
        $user = User::load($this->currentUser()->id());
        $board_id = Board::defaultBoard($user);
      }
      else {
        $board_id = $params['board'];
      }
    }

    // Get the values which depend on the board.
    $topics = empty($board_id) ? [] : Topic::topics($board_id);
    if (empty($topic_id)) {
      $topic_id = $params['topic'] ?? '';
    }
    if (!empty($topic_id) && !array_key_exists($topic_id, $topics)) {
      $topic_id = '';
    }
    $reviewers = empty($board_id) ? [] : $this->reviewers($board_id);
    $reviewer_id = $params['reviewer'] ?? '';
    if (!empty($reviewer) && !array_key_exists($reviewer_id, $reviewers)) {
      $reviewer_id = '';
    }
    $packets = empty($board_id) ? [] : $this->packets($board_id, $topic_id);
    if (empty($packet_id)) {
      $packet_id = $params['packet'] ?? '';
    }
    if (!empty($packet_id) && !array_key_exists($packet_id, $packets)) {
      $packet_id = '';
    }

    // Assemble the fields for the form.
    $form = [
      '#title' => 'Literature Reviews',
      '#attached' => ['library' => ['ebms_report/literature-reviews']],
      '#cache' => ['max-age' => 0],
      'filters' => [
        '#type' => 'details',
        '#title' => 'Filters',
        'board' => [
          '#type' => 'select',
          '#title' => 'Editorial Board',
          '#description' => 'Select the board for which the report is to be generated.',
          '#required' => TRUE,
          '#options' => $boards,
          '#default_value' => $board_id,
          '#empty_value' => '',
          '#ajax' => [
            'callback' => '::boardChangeCallback',
            'wrapper' => 'board-controlled',
            'event' => 'change',
          ],
        ],
        'board-controlled' => [
          '#type' => 'container',
          '#attributes' => ['id' => 'board-controlled'],
          'topic' => [
            '#type' => 'select',
            '#title' => 'Summary Topic',
            '#description' => 'Optionally select a topic to narrow the report.',
            '#options' => $topics,
            '#default_value' => $topic_id,
            '#empty_value' => '',
          ],
          'reviewer' => [
            '#type' => 'select',
            '#title' => 'Summary Reviewer',
            '#description' => 'Optionally select a reviewer to narrow the report.',
            '#options' => $reviewers,
            '#default_value' => $reviewer_id,
            '#empty_value' => '',
          ],
          'packet' => [
            '#type' => 'select',
            '#title' => 'Summary Packet',
            '#description' => 'Optionally select a packet to narrow the report.',
            '#options' => $packets,
            '#default_value' => $packet_id,
            '#empty_value' => '',
          ],
        ],
        'cycle' => [
          '#type' => 'select',
          '#title' => 'Summary cycle',
          '#description' => 'Optionally select a cycle to narrow the report.',
          '#options' => Batch::cycles(),
          '#default_value' => $params['cycle'] ?? '',
          '#empty_value' => '',
        ],
        'review-submission-date' => [
          '#type' => 'container',
          '#attributes' => ['class' => ['inline-fields']],
          '#title' => 'Review Submission Date Range',
          '#description' => 'Only show reviews submitted during the specified date range.',
          'submission-start' => [
            '#type' => 'date',
            '#default_value' => $params['submission-start'] ?? '',
          ],
          'submission-end' => [
            '#type' => 'date',
            '#default_value' => $params['submission-end'] ?? '',
          ],
        ],
        'packet-creation-date' => [
          '#type' => 'container',
          '#attributes' => ['class' => ['inline-fields']],
          '#title' => 'Packet Creation Date Range',
          '#description' => 'Only show reviews for packets created during the specified date range.',
          'creation-start' => [
            '#type' => 'date',
            '#default_value' => $params['creation-start'] ?? '',
          ],
          'creation-end' => [
            '#type' => 'date',
            '#default_value' => $params['creation-end'] ?? '',
          ],
        ],
      ],
      'submit' => [
        '#type' => 'submit',
        '#value' => 'Submit',
      ],
      'reset' => [
        '#type' => 'submit',
        '#value' => 'Reset',
        '#submit' => ['::resetSubmit'],
        '#limit_validation_errors' => [],
      ],
    ];

    // If we have a report request, generate and add it.
    if (!empty($params) && empty($form_state->getValues())) {
      $form['report'] = $this->report($params);
    }
    return $form;
  }

  /**
   * {@inheritdoc}
   */
  public function submitForm(array &$form, FormStateInterface $form_state) {
    $params = $form_state->getValues();
    $request = SavedRequest::saveParameters('literature reviews report', $params);
    $form_state->setRedirect('ebms_report.literature_reviews', ['report_id' => $request->id()]);
  }

  /**
   * Create a fresh form.
   */
  public function resetSubmit(array &$form, FormStateInterface $form_state) {
    $form_state->setRedirect('ebms_report.literature_reviews');
  }

  /**
   * Fill in the portion of the form driven by board selection.
   *
   * @param array $form
   *   Render array we are adjusting.
   * @param FormStateInterface $form_state
   *   Access to the form's values.
   */
  public function boardChangeCallback(array &$form, FormStateInterface $form_state) {
    return $form['filters']['board-controlled'];
  }

  /**
   * Show articles with reviews matching the filter criteria.
   *
   * Note that for this report we ignore whether packets are active and
   * whether articles have been dropped. If there's a review which matches
   * the user's filtering criteria, we show it.
   *
   * @param array $values
   *   Values entered by the user on the request form.
   *
   * @return array
   *   Render array for the report.
   */
  private function report(array $params): array {

    // Drupal's entity query API isn't up to the task of creating this report.
    $query = \Drupal::database()->select('ebms_review', 'review');
    $query->join('ebms_packet_article__reviews', 'reviews', 'reviews.reviews_target_id = review.id');
    $query->join('ebms_packet_article', 'packet_article', 'packet_article.id = reviews.entity_id');
    $query->join('ebms_packet__articles', 'articles', 'articles.articles_target_id = packet_article.id');
    $query->join('ebms_packet', 'packet', 'packet.id = articles.entity_id');
    $query->join('ebms_article', 'article', 'article.id = packet_article.article');
    $query->join('ebms_article__topics', 'topics', 'topics.entity_id = article.id');
    $query->join('ebms_article_topic', 'article_topic', 'article_topic.id = topics.topics_target_id AND article_topic.topic = packet.topic');
    $query->join('ebms_topic', 'topic', 'topic.id = packet.topic');
    $query->leftJoin('ebms_article__authors', 'authors', 'authors.entity_id = article.id AND authors.delta = 0');

    // Add the conditions to the query.
    if (!empty($params['topic'])) {
      $query->condition('packet.topic', $params['topic']);
    }
    else {
      $query->condition('topic.board', $params['board']);
    }
    if (!empty($params['reviewer'])) {
      $query->condition('review.reviewer', $params['reviewer']);
    }
    if (!empty($params['cycle'])) {
      $query->condition('article_topic.cycle', $params['cycle']);
    }
    if (!empty($params['packet'])) {
      $query->condition('packet.id', $params['packet']);
    }
    if (!empty($params['submission-start'])) {
      $query->condition('review.posted', $params['submission-start'], '>=');
    }
    if (!empty($params['submission-end'])) {
      $end = $params['submission-end'];
      if (strlen($end) === 10) {
        $end .= ' 23:59:59';
      }
      $query->condition('review.posted', $end, '<=');
    }
    if (!empty($params['creation-start'])) {
      $query->condition('packet.created', $params['creation-start'], '>=');
    }
    if (!empty($params['creation-end'])) {
      $end = $params['creation-end'];
      if (strlen($end) === 10) {
        $end .= ' 23:59:59';
      }
      $query->condition('packet.created', $end, '<=');
    }

    // Select the fields to be returned with the results set.
    $query->addField('article', 'id', 'article_id');
    $query->addField('article', 'search_title', 'title');
    $query->addField('article_topic', 'id', 'article_topic_id');
    $query->addField('article_topic', 'cycle', 'cycle');
    $query->addField('authors', 'authors_search_name', 'author');
    $query->addField('packet', 'title', 'packet');
    $query->addField('packet', 'created', 'packet_created');
    $query->addField('packet', 'id', 'packet_id');
    $query->addField('review', 'id', 'review_id');
    $query->addField('review', 'posted', 'review_posted');
    $query->addField('topic', 'id', 'topic_id');
    $query->addField('topic', 'name', 'topic');
    $query->distinct();

    // Provide a ressonable sort order for the results.
    $query->orderBy('author');
    $query->orderBy('title');
    $query->orderBy('topic');
    $query->orderBy('packet_created');
    $query->orderBy('review_posted');

    // Create the render arrays for the articles with their reviews.
    $articles = [];
    $packets = [];
    $review_count = 0;
    foreach ($query->execute() as $row) {
      $review_count++;
      $article_id = $row->article_id;
      $topic_id = $row->topic_id;
      $packet_id = $row->packet_id;
      $packets[$packet_id] = $packet_id;
      $review = Review::load($row->review_id);
      if (!array_key_exists($article_id, $articles)) {
        $article = Article::load($article_id);
        $article_topic = ArticleTopic::load($row->article_topic_id);
        $high_priority = FALSE;
        foreach ($article_topic->tags as $article_tag) {
          if ($article_tag->entity->tag->entity->field_text_id->value === 'high_priority') {
            $high_priority = TRUE;
            break;
          }
        }
        $articles[$article_id] = [
          'id' => $article->id(),
          'url' => Url::fromRoute('ebms_article.article', ['article' => $article->id()]),
          'authors' => implode(', ', $article->getAuthors(3)) ?: '[No authors named]',
          'title' => $article->title->value,
          'publication' => $article->getLabel(),
          'pmid' => $article->source_id->value,
          'high_priority' => $high_priority,
          'topics' => [],
        ];
      }
      if (!array_key_exists($topic_id, $articles[$article_id]['topics'])) {
        $articles[$article_id]['topics'][$topic_id] = [
          'name' => $row->topic,
          'cycle' => Batch::cycleString($row->cycle),
          'packets' => [],
        ];
      }
      if (!array_key_exists($packet_id, $articles[$article_id]['topics'][$topic_id]['packets'])) {
        $articles[$article_id]['topics'][$topic_id]['packets'][$packet_id] = [
          'name' => $row->packet,
          'created' => $row->packet_created,
          'reviews' => [],
        ];
      }
      $symbol = '✅';
      $dispositions = [];
      foreach ($review->dispositions as $disposition) {
        $dispositions[] = $disposition->entity->name->value;
        if ($disposition->entity->name->value === Review::NO_CHANGES) {
          $symbol = '❌';
        }
      }
      $comments = [];
      foreach ($review->comments as $comment) {
        if (!empty($comment->value)) {
          $comments[] = $comment->value;
        }
      }
      $reasons = [];
      foreach ($review->reasons as $reason) {
        $reasons[] = $reason->entity->name->value;
      }
      $articles[$article_id]['topics'][$topic_id]['packets'][$packet_id]['reviews'][] = [
        'posted' => $row->review_posted,
        'reviewer' => $review->reviewer->entity->name->value,
        'dispositions' => $dispositions,
        'comments' => $comments,
        'reasons' => $reasons,
        'symbol' => $symbol,
      ];
    }

    // Assemble and return the render array for the report.
    $article_count = count($articles);
    $packet_count = count($packets);
    $article_s = $article_count === 1 ? '' : 's';
    $review_s = $review_count === 1 ? '' : 's';
    $packet_s = $packet_count === 1 ? '' : 's';
    return [
      '#theme' => 'literature_reviews',
      '#title' => "Literature Reviews ($article_count Article$article_s with $review_count Review$review_s in $packet_count Packet$packet_s)",
      '#articles' => $articles,
    ];
  }

  /**
   * Create the picklist options for reviewer belonging to a specific board.
   *
   * @param int $board_id
   *   Entity ID for the board whose members we're looking for.
   *
   * @return array
   *   Sorted reviewer names indexed by user ID.
   */
  private function reviewers(int $board_id): array {
    $storage = \Drupal::entityTypeManager()->getStorage('user');
    $query = $storage->getQuery()->accessCheck(FALSE);
    $query->condition('boards', $board_id);
    $query->condition('roles', 'board_member');
    $query->sort('name');
    $reviewers = [];
    foreach ($storage->loadMultiple($query->execute()) as $reviewer) {
      $reviewers[$reviewer->id()] = $reviewer->name->value;
    }
    return $reviewers;
  }

  /**
   * Create the picklist options for packets created for a specific board.
   *
   * @param int $board_id
   *   Entity ID for the board whose packets we're looking for.
   * @param int|string $topic_id
   *   ID of selected topic, or empty string if no topic selected.
   *
   * @return array
   *   Sorted packet titles indexed by entity ID.
   */
  private function packets(int $board_id, int|string $topic_id): array {
    $storage = \Drupal::entityTypeManager()->getStorage('ebms_packet');
    $query = $storage->getQuery()->accessCheck(FALSE);
    if (!empty($topic_id)) {
      $query->condition('topic', $topic_id);
    }
    else {
      $query->condition('topic.entity.board', $board_id);
    }
    $query->sort('title');
    $packets = [];
    foreach ($storage->loadMultiple($query->execute()) as $packet) {
      $packets[$packet->id()] = $packet->title->value;
    }
    return $packets;
  }

}
