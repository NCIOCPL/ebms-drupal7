<?php

namespace Drupal\ebms_review\Form;

use Drupal\Core\Form\FormBase;
use Drupal\Core\Form\FormStateInterface;
use Drupal\Core\Url;
use Drupal\ebms_board\Entity\Board;
use Drupal\ebms_core\Entity\SavedRequest;
use Drupal\ebms_review\Entity\Packet;
use Drupal\ebms_topic\Entity\Topic;
use Drupal\user\Entity\User;

/**
 * Show packets which have at least one submitted review.
 */
class ReviewedPackets extends FormBase {

  /**
   * {@inheritdoc}
   */
  public function getFormId(): string {
    return 'reviewed_packets_form';
  }

  /**
   * {@inheritdoc}
   */
  public function buildForm(array $form, FormStateInterface $form_state, int $filter_id = 0): array {

    // Load the saved form parameters (if any).
    $params = empty($filter_id) ? [] : SavedRequest::loadParameters($filter_id);

    // Determine available boards based on the user.
    $user = User::load($this->currentUser()->id());
    $boards = [];
    foreach ($user->boards as $board) {
      $boards[$board->target_id] = $board->entity->name->value;
    }
    if (empty($boards)) {
      $boards = Board::boards();
    }
    $selected_boards = $form_state->getValue('boards');
    if (empty($selected_boards)) {
      $selected_boards = $params['boards'] ?? [];
    }
    $selected_boards = array_values(array_diff($selected_boards, [0]));

    // The topic picklist is based on the boards.
    $topics = Topic::topics($selected_boards ?: array_keys($boards));
    $selected_topics = $form_state->getValue('topics');
    if (empty($selected_topics)) {
      $selected_topics = $params['topics'] ?? [];
    }
    $selected_topics = array_values(array_diff($selected_topics, [0]));
    foreach ($selected_topics as $topic_id) {
      if (!array_key_exists($topic_id, $topics)) {
        $selected_topics = [];
        break;
      }
    }

    // The reviewers are also based on the boards.
    $reviewers = [];
    foreach (Board::boardMembers($selected_boards ?: array_keys($boards)) as $reviewer) {
      $reviewers[$reviewer->id()] = $reviewer->name->value;
    }
    $selected_reviewers = $form_state->getValue('reviewers');
    if (empty($selected_reviewers)) {
      $selected_reviewers = $params['reviewers'] ?? [];
    }
    $selected_reviewers = array_values(array_diff($selected_reviewers, [0]));
    foreach ($selected_reviewers as $uid) {
      if (!array_key_exists($uid, $reviewers)) {
        $selected_reviewers = [];
        break;
      }
    }

    // Create fields for topics and reviewers (two places to add them).
    $topics_field = [
      '#type' => 'select',
      '#options' => $topics,
      '#multiple' => TRUE,
      '#title' => 'Topics',
      '#description' => 'Select one or more topics to restrict the display of packets to those created for one of the selected topics.',
      '#default_value' => $selected_topics,
    ];
    $reviewers_field = [
      '#type' => 'select',
      '#title' => 'Reviewers',
      '#description' => 'Select one or more reviewers to restrict the display of packets to those with reviews by all of the selected reviewers.',
      '#options' => $reviewers,
      '#multiple' => TRUE,
      '#default_value' => $selected_reviewers,
    ];

    // Start the render array for the form.
    $form = [
      '#title' => 'Reviewed Packets',
      '#attached' => ['library' => ['ebms_review/reviewed-packets']],
      '#cache' => ['max-age' => 0],
      'filters' => [
        '#type' => 'details',
        '#title' => 'Filters',
      ],
    ];

    // Only add the board options if there's more than one to choose from.
    if (count($boards) > 1) {
      $form['filters']['boards'] = [
        '#type' => 'checkboxes',
        '#title' => 'Boards',
        '#description' => 'Select one or more boards to restrict the display of packets to those created for those boards.',
        '#options' => $boards,
        '#default_value' => $selected_boards,
        '#ajax' => [
          'callback' => '::boardChangeCallback',
          'wrapper' => 'board-controlled',
          'event' => 'change',
        ],
      ];
      $form['filters']['board-controlled'] = [
        '#type' => 'container',
        '#attributes' => ['id' => 'board-controlled'],
        'topics' => $topics_field,
        'reviewers' => $reviewers_field,
      ];
    }
    else {
      $form['filters']['topics'] = $topics_field;
      $form['filters']['reviewers'] = $reviewers_field;
    }

    // Fill in the rest of the form's fields.
    $form['filters']['name'] = [
      '#type' => 'textfield',
      '#title' => 'Packet Name',
      '#description' => 'Limit the list of packets to those whose names match the specified value. Use wildcards for partial name matching.',
      '#default_value' => $params['name'] ?? '',
    ];
    $form['filters']['review-date'] = [
      '#type' => 'container',
      '#attributes' => ['class' => ['inline-fields']],
      '#title' => 'Review Date Range',
      '#description' => 'Only show packets created during the specified date range.',
      'review-start' => [
        '#type' => 'date',
        '#default_value' => $params['review-start'] ?? '',
      ],
      'review-end' => [
        '#type' => 'date',
        '#default_value' => $params['review-end'] ?? '',
      ],
    ];
    $form['options'] = [
      '#type' => 'details',
      '#title' => 'Display Options',
      'sort' => [
        '#type' => 'radios',
        '#title' => 'Sort By',
        '#options' => [
          'title' => 'Packet Title',
          'updated' => 'Date Updated',
        ],
        '#default_value' => $params['sort'] ?? 'updated',
        '#description' => 'Select the element to be used for ordering the list of packets.',
      ],
      /* Because of the limitations of (and bugs in) Drupal's entity query
         API, we can support sorting or pagination, but not both.
      'per-page' => [
        '#type' => 'radios',
        '#title' => 'Packets Per Page',
        '#options' => [10 => 10, 25 => 25, 50 => 50, 100 => 100, 'all' => 'All'],
        '#default_value' => $per_page,
        '#description' => 'Decide how many articles to display on each page.',
      ],
      See https://www.drupal.org/project/drupal/issues/3282364.
      See also https://www.drupal.org/project/drupal/issues/3188258.
      */
    ];
    $form['submit'] = [
      '#type' => 'submit',
      '#value' => 'Filter',
    ];
    $form['reset'] = [
      '#type' => 'submit',
      '#value' => 'Reset',
      '#submit' => ['::resetSubmit'],
      '#limit_validation_errors' => [],
    ];
    $form['report'] = [
      '#type' => 'submit',
      '#value' => 'Generate Report',
      '#submit' => ['::reportSubmit'],
      '#limit_validation_errors' => [],
    ];

    // Add the packets underneath the form fields.
    $start = $params['review-start'] ?? '';
    $end = $params['review-end'] ?? '';
    if (strlen($end) === 10) {
      $end .= ' 23:59:59';
    }
    $storage = \Drupal::entityTypeManager()->getStorage('ebms_packet');
    $query = $storage->getAggregateQuery()->accessCheck(FALSE);
    // $query = $storage->getQuery()->accessCheck(FALSE);
    $query->sort('title');
    $query->condition('active', 1);
    $query->exists('articles.entity.reviews');
    $alias = 'updated';
    $query->aggregate('articles.entity.reviews.entity.posted', 'MAX', NULL, $alias);
    // $query->sortAggregate('articles.entity.reviews.entity.posted', 'MAX', 'DESC');
    if (!empty($selected_topics)) {
      $query->condition('topic', $selected_topics, 'IN');
    }
    else {
      $query->condition('topic.entity.board', $selected_boards ?: array_keys($boards), 'IN');
    }
    if (empty($selected_reviewers)) {
      if (!empty($start)) {
        $query->condition('articles.entity.reviews.entity.posted', $start, '>=');
      }
      if (!empty($end)) {
        $query->condition('articles.entity.reviews.entity.posted', $end, '<=');
      }
    }
    if (!empty($params['name'])) {
      $query->condition('title', $params['name'], 'LIKE');
    }
    $query->groupBy('id');
    $updated = [];
    $titles = [];
    foreach ($query->execute() as $values) {
      $id = $values['id'];
      $titles[$id] = $values['title'];
      $updated[$id] = $values['updated'];
    }
    $packets = [];

    // The loadMultiple is much more efficient than calling ::load()
    // separately for each packet.
    $opts = [];
    if (!empty($filter_id)) {
      $opts['query'] = ['filter-id' => $filter_id];
    }
    foreach (Packet::loadMultiple(array_keys($titles)) as $packet) {
      $packet_id = $packet->id();
      $manager_id = $packet->topic->entity->board->entity->manager->target_id;
      $is_manager = $manager_id == $user->id();
      $last_seen = $packet->last_seen->value;
      if (empty($selected_reviewers) || $this->wanted($packet, $selected_reviewers, $start, $end)) {
        $packet_reviewers = [];
        foreach ($packet->articles as $article) {
          foreach ($article->entity->reviews as $review) {
            $reviewer_id = $review->entity->reviewer->target_id;
            if (!array_key_exists($reviewer_id, $packet_reviewers)) {
              $packet_reviewers[$reviewer_id] = [
                'name' => $review->entity->reviewer->entity->name->value,
                'new' => 0,
              ];
            }
            if ($is_manager && (empty($last_seen) || $review->entity->posted->value > $last_seen)) {
              $packet_reviewers[$reviewer_id]['new']++;
            }
          }
        }
        sort($packet_reviewers);
        $packets[] = [
          'id' => $packet_id,
          'url' => Url::fromRoute('ebms_review.reviewed_packet', ['packet_id' => $packet_id], $opts),
          'title' => $titles[$packet_id],
          'updated' => $updated[$packet_id],
          'reviewers' => $packet_reviewers,
          'star' => [
            '#theme' => 'packet_star',
            '#id' => $packet_id,
            '#starred' => $packet->starred->value,
          ],
        ];
      }
    }

    // Finish off the render array and return it.
    $sort = 'sorted by packet name';
    if (empty($params['sort']) || $params['sort'] === 'updated') {
      usort($packets, function(array $a, array $b): int {
        return $b['updated'] <=> $a['updated'];
      });
      $sort = 'most-recently-reviewed first';
    }
    $form['packets'] = [
      '#theme' => 'reviewed_packets',
      '#packets' => $packets,
      '#sort' => $sort,
    ];
    return $form;
  }

  /**
   * Find out if a packet should be included in the display.
   *
   * This is just doing the part that the entity query can't do, which is
   * verifying that each selected reviewer has submitted at least one review
   * for the packet (within the date range if specified).
   *
   * @param Packet $packet
   *   The packet we are vetting.
   * @param array $reviewers
   *   The array of user IDs for the selected reviewers.
   * @param string $start
   *   The possibly empty start of the date range for review submission.
   * @param string $end
   *   The possibly empty end of the date range for review submission.
   *
   * @return bool
   *   TRUE iff we want to show the packet.
   */
  private function wanted(Packet $packet, array $reviewers, string $start, string $end): bool {
    if (empty($reviewers)) {
      return TRUE;
    }
    foreach ($packet->articles->entity->reviews as $review) {
      if (empty($start) || $review->entity->posted->value >= $start) {
        if (empty($end) || $review->entity->posted->value <= $end) {
          $position = array_search($review->entity->reviewer->target_id, $reviewers);
          if ($position !== FALSE) {
            unset($reviewers[$position]);
            if (empty($reviewers)) {
              return TRUE;
            }
          }
        }
      }
    }
    return FALSE;
  }

  /**
   * {@inheritdoc}
   */
  public function submitForm(array &$form, FormStateInterface $form_state) {
    $request = SavedRequest::saveParameters('reviewed packets', $form_state->getValues());
    $form_state->setRedirect('ebms_review.reviewed_packets', ['filter_id' => $request->id()]);
  }

  /**
   * Create a fresh form.
   */
  public function resetSubmit(array &$form, FormStateInterface $form_state) {
    $form_state->setRedirect('ebms_review.reviewed_packets');
  }

  /**
   * Redirect to the report form.
   */
  public function reportSubmit(array &$form, FormStateInterface $form_state) {
    $form_state->setRedirect('ebms_review.literature_reviews');
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

}
