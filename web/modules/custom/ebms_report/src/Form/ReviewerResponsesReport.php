<?php

namespace Drupal\ebms_report\Form;

use Drupal\Core\Database\Query\TableSortExtender;
use Drupal\Core\Form\FormBase;
use Drupal\Core\Form\FormStateInterface;
use Drupal\ebms_board\Entity\Board;
use Drupal\ebms_core\Entity\SavedRequest;
use Drupal\ebms_topic\Entity\Topic;
use Drupal\user\Entity\User;

/**
 * Show completed/not-completed statistics for assigned reviews.
 *
 * @ingroup ebms
 */
class ReviewerResponsesReport extends FormBase {

  /**
   * {@inheritdoc}
   */
  public function getFormId(): string {
    return 'reviewer_responses_report_form';
  }

  /**
   * {@inheritdoc}
   */
  public function buildForm(array $form, FormStateInterface $form_state, int $report_id = 0): array {

    // Collect some values for the report request.
    $params = empty($report_id) ? [] : SavedRequest::loadParameters($report_id);
    $user = User::load($this->currentUser()->id());
    $board = $form_state->getValue('board', $params['board'] ?? Board::defaultBoard($user));
    $topics = empty($board) ? [] : Topic::topics($board);
    $selected_topics = $params['topics'] ?? [];
    if (!empty($selected_topics)) {
      foreach ($selected_topics as $topic) {
        if (!array_key_exists($topic, $topics)) {
          $selected_topics = [];
          break;
        }
      }
    }
    $reviewers = empty($board) ? [] : $this->reviewers($board);
    $selected_reviewers = $params['reviewers'] ?? [];
    if (!empty($selected_reviewers)) {
      foreach ($selected_reviewers as $reviewer) {
        if (!array_key_exists($reviewer, $reviewers)) {
          $selected_reviewers = [];
          break;
        }
      }
    }

    // Assemble the fields for the form.
    $form = [
      '#title' => 'Reviewer Responses',
      '#attached' => ['library' => ['ebms_report/reviewer-responses']],
      '#cache' => ['max-age' => 0],
      'filters' => [
        '#type' => 'details',
        '#title' => 'Filters',
        'board' => [
          '#type' => 'select',
          '#title' => 'Editorial Board',
          '#description' => 'Select the board for which the report is to be generated.',
          '#required' => TRUE,
          '#options' => Board::boards(),
          '#default_value' => $board,
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
          'topics' => [
            '#type' => 'select',
            '#title' => 'Summary Topic',
            '#description' => 'Optionally select one or more topics to narrow the report.',
            '#options' => $topics,
            '#multiple' => TRUE,
            '#default_value' => $selected_topics,
            '#empty_value' => '',
          ],
          'reviewers' => [
            '#type' => 'select',
            '#title' => 'Summary Reviewer',
            '#description' => 'Optionally select one or more reviewers to narrow the report.',
            '#options' => $reviewers,
            '#multiple' => TRUE,
            '#default_value' => $selected_reviewers,
            '#empty_value' => '',
          ],
        ],
        'packet-assigned-date' => [
          '#type' => 'container',
          '#attributes' => ['class' => ['inline-fields']],
          '#title' => 'Packet Assigned Date Range',
          '#description' => 'Only show statistics for reviews assigned during the specified date range.',
          'assigned-start' => [
            '#type' => 'date',
            '#default_value' => $params['assigned-start'] ?? '',
          ],
          'assigned-end' => [
            '#type' => 'date',
            '#default_value' => $params['assigned-end'] ?? '',
          ],
        ],
      ],
      'option-box' => [
        '#type' => 'details',
        '#title' => 'Options',
        'board-member-version' => [
          '#type' => 'checkbox',
          '#title' => 'Board Member Version',
          '#description' => 'Produce a version of the report which omits the "Created" column and which has a fixed sort order.',
          '#default_value' => $params['board-member-version'],
        ]
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
    if (!empty($params) && empty($form_state->getValue('board'))) {
      $form['report'] = $this->report($params);
    }
    return $form;
  }

  /**
   * {@inheritdoc}
   */
  public function submitForm(array &$form, FormStateInterface $form_state) {
    $params = $form_state->getValues();
    $request = SavedRequest::saveParameters('reviewer responses report', $params);
    $form_state->setRedirect('ebms_report.responses_by_reviewer', ['report_id' => $request->id()]);
  }

  /**
   * Create a fresh form.
   */
  public function resetSubmit(array &$form, FormStateInterface $form_state) {
    $form_state->setRedirect('ebms_report.responses_by_reviewer');
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
   * Show statistics on number of reviews assigned versus number performed.
   *
   * @param array $values
   *   Values entered by the user on the request form.
   *
   * @return array
   *   Render array for the report.
   */
  private function report(array $params): array {

    // Fetch the packets needed for the report. It's possible to do this with
    // Drupal's entity query API, but it takes over 20 seconds to generate the
    // reqort that way with just the sample developer data. The full report
    // against the live data set could well time out. This way it takes about
    // 1/50 of a second (representing a speedup of better than three orders of
    // magnitude).
    $query = \Drupal::database()->select('ebms_packet', 'packet');
    $query->join('ebms_packet__articles', 'articles', 'articles.entity_id = packet.id');
    if (!empty($params['topics'])) {
      $query->condition('packet.topic', $params['topics'], 'IN');
    }
    else {
      $query->join('ebms_topic', 'topic', 'topic.id = packet.topic');
      $query->condition('topic.board', $params['board']);
    }
    if (!empty($params['reviewers'])) {
      $query->join('ebms_packet__reviewers', 'reviewers', 'reviewers.entity_id = packet.id');
      $query->condition('reviewers.reviewers_target_id', $params['reviewers'], 'IN');
    }
    if (!empty($params['assigned-start'])) {
      $query->condition('packet.created', $params['assigned-start'], '>=');
    }
    if (!empty($params['assigned-end'])) {
      $end = $params['assigned-end'];
      if (strlen($end) === 10) {
        $end .= ' 23:59:59';
      }
      $query->condition('packet.created', $end, '<=');
    }
    $query->addExpression('COUNT(DISTINCT articles.articles_target_id)', 'assigned');
    $query->addField('packet', 'id', 'packet_id');
    $query->groupBy('packet.id');
    $articles = [];
    foreach ($query->execute() as $row) {
      $articles[$row->packet_id] = $row->assigned;
    }

    // Get the count of reviews for each packet/reviewer combination.
    if (empty($articles)) {
      $articles = [0 => 'SQL "WHERE ... IN" will not accept an empty array'];
    }
    $query = \Drupal::database()->select('ebms_review', 'review');
    $query->join('ebms_packet_article__reviews', 'reviews', 'reviews.reviews_target_id = review.id');
    $query->join('ebms_packet__articles', 'articles', 'articles.articles_target_id = reviews.entity_id');
    if (!empty($params['reviewers'])) {
      $query->condition('review.reviewer', $params['reviewers'], 'IN');
    }
    $query->condition('articles.entity_id', array_keys($articles), 'IN');
    $query->addExpression('COUNT(*)', 'reviewed');
    $query->addField('articles', 'entity_id', 'packet_id');
    $query->addField('review', 'reviewer', 'reviewer_id');
    $query->groupBy('articles.entity_id');
    $query->groupBy('review.reviewer');
    $reviews = [];
    foreach ($query->execute() as $row) {
      if (empty($reviews[$row->reviewer_id])) {
        $reviews[$row->reviewer_id] = [];
      }
      $reviews[$row->reviewer_id][$row->packet_id] = $row->reviewed;
    }

    // Create different headers depending on the report flavor.
    $board_member_version = !empty($params['board-member-version']);
    if ($board_member_version) {
      $header = [
        'Packet Name',
        'Reviewer',
        'Assigned',
        'Completed',
        'Not Completed',
      ];
    }
    else {
      $header = [
        [
          'data' => 'Packet Name',
          'field' => 'packet.title',
        ],
        [
          'data' => 'Created',
          'field' => 'packet.created',
          'sort' => 'desc',
        ],
        [
          'data' => 'Reviewer',
          'field' => 'reviewer.name',
        ],
        ['data' => 'Assigned'],
        ['data' => 'Completed'],
        ['data' => 'Not Completed'],
      ];
    }

    // Finally, find all the reviewers for the selected packets.
    $query = \Drupal::database()->select('ebms_packet', 'packet');
    $query->condition('packet.id', array_keys($articles), 'IN');
    $query->join('ebms_packet__reviewers', 'reviewers', 'reviewers.entity_id = packet.id');
    $query->join('users_field_data', 'reviewer', 'reviewer.uid = reviewers.reviewers_target_id');
    if (!empty($params['reviewers'])) {
      $query->condition('reviewer.uid', $params['reviewers'], 'IN');
    }
    $query->fields('reviewer', ['uid', 'name']);
    $query->fields('packet', ['id', 'title', 'created']);
    if ($board_member_version) {
      $query->orderBy('packet.created');
    }
    else {
      $query = $query->extend(TableSortExtender::class)->orderByHeader($header);
    }
    $rows = [];
    $total_assigned = $total_reviewed = 0;
    $reviewers = [];
    foreach ($query->execute() as $row) {
      $reviewer_id = $row->uid;
      $packet_id = $row->id;
      $reviewers[$reviewer_id] = $reviewer_id;
      $assigned = $articles[$packet_id];
      $reviewed = $reviews[$reviewer_id][$packet_id] ?? 0;
      $total_assigned += $assigned;
      $total_reviewed += $reviewed;
      $cells = [$row->title];
      if (!$board_member_version) {
        $cells[] = [
          'data' => substr($row->created, 0, 10),
          'class' => ['nowrap'],
        ];
      }
      $cells[] = $row->name;
      $cells[] = $assigned;
      $cells[] = $reviewed;
      $cells[] = $assigned - $reviewed;
      $rows[] = ['data' => $cells];
    }

    // Add a row for the totals.
    $row = ['Totals'];
    if (!$board_member_version) {
      $row[] = '';
    }
    $row[] = '';
    $row[] = $total_assigned;
    $row[] = $total_reviewed;
    $row[] = $total_assigned - $total_reviewed;
    $rows[] = [
      'class' => ['totals'],
      'data' => $row,
    ];

    // Assemble the table and return its render array.
    $packet_count = count($articles);
    $reviewer_count = count($reviewers);
    $reviewer_s = $reviewer_count === 1 ? '' : 's';
    $packet_s = $packet_count === 1 ? '' : 's';
    return [
      '#type' => 'table',
      '#sticky' => TRUE,
      '#rows' => $rows,
      '#header' => $header,
      '#empty' => 'No packets match the filtering criteria.',
      '#caption' => "Review Statistics ($packet_count Packet$packet_s for $reviewer_count Reviewer$reviewer_s)",
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

}
