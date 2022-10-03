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
 * Show packets which have no submitted reviews.
 */
class UnreviewedPackets extends FormBase {

  /**
   * {@inheritdoc}
   */
  public function getFormId(): string {
    return 'unreviewed_packets_form';
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
      '#description' => 'Select one or more reviewers to restrict the display of packets to those assigned to any of the selected reviewers.',
      '#options' => $reviewers,
      '#multiple' => TRUE,
      '#default_value' => $selected_reviewers,
    ];

    // Start the render array for the form.
    $form = [
      '#title' => 'Unreviewed Packets',
      '#attached' => ['library' => ['ebms_review/unreviewed-packets']],
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
    $form['filters']['creation-date'] = [
      '#type' => 'container',
      '#attributes' => ['class' => ['inline-fields']],
      '#title' => 'Packet Creation Date Range',
      '#description' => 'Only show packets created during the specified date range.',
      'creation-start' => [
        '#type' => 'date',
        '#default_value' => $params['creation-start'] ?? '',
      ],
      'creation-end' => [
        '#type' => 'date',
        '#default_value' => $params['creation-end'] ?? '',
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
          'created' => 'Date Packet Was Created',
        ],
        '#default_value' => $params['sort'] ?? 'created',
        '#description' => 'Select the element to be used for ordering the list of packets.',
      ],
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

    // Add the packets underneath the form fields.
    $start = $params['creation-start'] ?? '';
    $end = $params['creation-end'] ?? '';
    if (strlen($end) === 10) {
      $end .= ' 23:59:59';
    }
    $storage = \Drupal::entityTypeManager()->getStorage('ebms_packet');
    $query = $storage->getQuery()->accessCheck(FALSE);
    $query->condition('active', 1);
    $query->exists('articles.entity.reviews');
    $with_reviews = $query->execute();
    $query = $storage->getQuery()->accessCheck(FALSE);
    $query->sort('title');
    $query->condition('active', 1);
    $query->condition('id', $with_reviews, 'NOT IN');
    if (!empty($selected_topics)) {
      $query->condition('topic', $selected_topics, 'IN');
    }
    else {
      $query->condition('topic.entity.board', $selected_boards ?: array_keys($boards), 'IN');
    }
    if (!empty($selected_reviewers)) {
      $query->condition('reviewers', $selected_reviewers, 'IN');
    }
    if (!empty($start)) {
      $query->condition('created', $start, '>=');
    }
    if (!empty($end)) {
      $query->condition('created', $end, '<=');
    }
    if (!empty($params['name'])) {
      $query->condition('title', $params['name'], 'LIKE');
    }
    $packets = [];
    $opts = [];
    if (!empty($filter_id)) {
      $opts['query'] = ['filter-id' => $filter_id];
    }
    foreach (Packet::loadMultiple($query->execute()) as $packet) {
      $packet_id = $packet->id();
      $reviewers = [];
      foreach ($packet->reviewers as $reviewer) {
        $reviewers[] = $reviewer->entity->name->value;
      }
      sort($reviewers);
      $packets[] = [
        'id' => $packet_id,
        'url' => Url::fromRoute('ebms_review.unreviewed_packet', ['packet_id' => $packet_id], $opts),
        'title' => $packet->title->value,
        'created' => $packet->created->value,
        'reviewers' => $reviewers,
      ];
    }

    // Finish off the render array and return it.
    $sort = 'sorted by packet name';
    if (empty($params['sort']) || $params['sort'] === 'created') {
      usort($packets, function(array $a, array $b): int {
        return $b['created'] <=> $a['created'];
      });
      $sort = 'most-recently-created first';
    }
    $form['packets'] = [
      '#theme' => 'unreviewed_packets',
      '#packets' => $packets,
      '#sort' => $sort,
    ];
    return $form;
  }

  /**
   * {@inheritdoc}
   */
  public function submitForm(array &$form, FormStateInterface $form_state) {
    $request = SavedRequest::saveParameters('unreviewed packets', $form_state->getValues());
    $form_state->setRedirect('ebms_review.unreviewed_packets', ['filter_id' => $request->id()]);
  }

  /**
   * Create a fresh form.
   */
  public function resetSubmit(array &$form, FormStateInterface $form_state) {
    $form_state->setRedirect('ebms_review.unreviewed_packets');
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
