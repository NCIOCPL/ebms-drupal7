<?php

namespace Drupal\ebms_report\Form;

use Drupal\Core\Form\FormBase;
use Drupal\Core\Form\FormStateInterface;
use Drupal\Core\Link;
use Drupal\Core\Url;
use Drupal\ebms_board\Entity\Board;
use Drupal\ebms_core\Entity\SavedRequest;
use Drupal\ebms_doc\Entity\Doc;
use Drupal\ebms_topic\Entity\Topic;
use Drupal\taxonomy\Entity\Term;
use Drupal\user\Entity\User;

/**
 * Report on requests for hotel reservations.
 *
 * @ingroup ebms
 */
class ReimbursementRequestsReport extends FormBase {

  /**
   * {@inheritdoc}
   */
  public function getFormId(): string {
    return 'reimbursement_requests_report_form';
  }

  /**
  * {@inheritdoc}
  */
  public function buildForm(array $form, FormStateInterface $form_state, int $report_id = 0): array {

    // Prepare the values needed for the form.
    $params = empty($report_id) ? [] : SavedRequest::loadParameters($report_id);
    $boards = Board::boards();
    $board = $form_state->getValue('board', $params['board'] ?? '');
    $per_page = $params['per-page'] ?? '10';
    $storage = \Drupal::entityTypeManager()->getStorage('ebms_meeting');
    $query = $storage->getQuery()->accessCheck(FALSE);
    $query->sort('dates', 'DESC');
    if (!empty($board)) {
      $or_group = $query->orConditionGroup()
        ->condition('boards', $board)
        ->condition('groups.entity.boards', $board);
      $query->condition($or_group);
    }
    $meetings = [];
    foreach ($storage->loadMultiple($query->execute()) as $meeting) {
      $name = $meeting->name->value;
      $date = substr($meeting->dates->value, 0, 10);
      $meetings[$meeting->id()] = "$name - $date";
    }
    $meeting = $params['meeting'] ?? '';
    if (!empty($meeting) && !array_key_exists($meeting, $meetings)) {
      $meeting = '';
    }

    // Create the render array for the form's fields.
    $form = [
      '#title' => 'Reimbursement Requests Report',
      '#attached' => ['library' => ['ebms_report/reimbursement-requests-report']],
      'filters' => [
        '#type' => 'details',
        '#title' => 'Filters',
        'board' => [
          '#type' => 'select',
          '#title' => 'Board',
          '#description' => 'Optionally select a board to restrict the report to requests associated with meetings for that board.',
          '#options' => $boards,
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
          'meeting' => [
            '#type' => 'select',
            '#title' => 'Meeting',
            '#description' => 'Optionally select a meeting to restrict the report to requests associated with that meeting.',
            '#options' => $meetings,
            '#default_value' => $meeting,
            '#empty_value' => '',
          ],
        ],
        'request-date' => [
          '#type' => 'container',
          '#attributes' => ['class' => ['inline-fields']],
          '#title' => 'Request Date Range',
          '#description' => 'Show requests submitted during the specified date range.',
          'request-start' => [
            '#type' => 'date',
            '#default_value' => $params['request-start'] ?? '',
          ],
          'request-end' => [
            '#type' => 'date',
            '#default_value' => $params['request-end'] ?? '',
          ],
        ],
      ],
      'options' => [
        '#type' => 'details',
        '#title' => 'View Options',
        'per-page' => [
          '#type' => 'radios',
          '#title' => 'Requests Per Page',
          '#options' => [
            '10' => '10',
            '25' => '25',
            '50' => '50',
            '100' => '100',
            'all' => 'All',
          ],
          '#default_value' => $per_page,
          '#description' => 'How many requests should be displayed on each page?',
        ],
      ],
      'filter' => [
        '#type' => 'submit',
        '#value' => 'Filter',
      ],
      'reset' => [
        '#type' => 'submit',
        '#value' => 'Reset',
        '#submit' => ['::resetSubmit'],
        '#limit_validation_errors' => [],
      ],
    ];


    // Identify the documents for the report.
    $storage = \Drupal::entityTypeManager()->getStorage('ebms_reimbursement_request');
    $query = $storage->getQuery()->accessCheck(FALSE);
    if (!empty($meeting)) {
      $query->condition('meeting', $meeting);
    }
    elseif (!empty($board)) {
      $or_group = $query->orConditionGroup()
        ->condition('meeting.entity.boards', $board)
        ->condition('meeting.entity.groups.entity.boards', $board);
      $query->condition($or_group);
    }
    if (!empty($params['request-start'])) {
      $query->condition('submitted', $params['request-start'], '>=');
    }
    if (!empty($params['request-end'])) {
      $end = $params['request-end'];
      if (strlen($end) === 10) {
        $end .= ' 23:59:59';
      }
      $query->condition('submitted', $end, '<=');
    }
    $count_query = clone $query;
    $count = $count_query->count()->execute();
    $query->sort('submitted', 'DESC');
    $start = 1;
    if ($per_page !== 'all') {
      $query->pager($per_page);
      $page = $this->getRequest()->get('page');
      $start = 1 + $page * $per_page;
    }

    // Assemble the render arrays for each displayed request.
    $requests = [];
    $route = 'ebms_meeting.meeting';
    foreach ($storage->loadMultiple($query->execute()) as $request) {
      $name = $request->meeting->entity->name->value;
      $meeting_url = '';
      if (empty($name)) {
        $meeting_name = '[NO MEETING SPECIFIED]';
      }
      else {
        $date = substr($request->meeting->entity->dates->value, 0, 10);
        $meeting_name = "$name - $date";
        $meeting_url = Url::fromRoute($route, ['meeting' => $request->meeting->target_id]);
      }
      $transportation = [];
      foreach ($request->transportation as $expense) {
        $transportation[] = $expense->toString();
      }
      $parking_and_tolls = [];
      foreach ($request->parking_and_tolls as $expense) {
        $parking_and_tolls[] = $expense->toString();
      }
      $values = [
        'id' => $request->id(),
        'submitted' => $request->submitted->value,
        'meeting_name' => $meeting_name,
        'meeting_url' => $meeting_url,
        'user' => $request->user->entity->name->value,
        'user_url' => Url::fromRoute('entity.user.canonical', ['user' => $request->user->target_id]),
        'arrival' => $request->arrival->value,
        'departure' => $request->departure->value,
        'comments' => str_replace("\n\n", "\n", $request->comments->value),
        'transportation' => $transportation,
        'parking_and_tolls' => $parking_and_tolls,
        'hotel_payment' => $request->hotel_payment->entity->name->value,
        'hotel_amount' => $request->hotel_amount->value,
        'nights_stayed' => $request->nights_stayed->value,
        'meals_and_incidentals' => $request->meals_and_incidentals->entity->name->value,
        'honorarium' => $request->honorarium_request->value ? 'Requested' : 'Declined',
        'reimburse_to' => $request->reimburse_to->entity->name->value,
        'total' => $request->total_amount->value,
        'certified' => $request->certified->value,
        'confirmation_email' => $request->confirmation_email->value,
      ];
      $requests[] = $values;
    }

    // Add the report below the form and return the page's render array.
    $form['requests'] = [
      '#theme' => 'reimbursement_requests_report',
      '#cache' => ['max-age' => 0],
      '#title' => "Requests ($count)",
      '#requests' => $requests,
      '#start' => $start,
    ];
    $form['pager'] = [
      '#type' => 'pager',
    ];
    return $form;
  }

  /**
   * {@inheritdoc}
   */
  public function submitForm(array &$form, FormStateInterface $form_state) {
    $values = $form_state->getValues();
    $request = SavedRequest::saveParameters('reimbursement requests report', $values);
    $form_state->setRedirect('ebms_report.reimbursement_requests', ['report_id' => $request->id()]);
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
   * Create a fresh form.
   */
  public function resetSubmit(array &$form, FormStateInterface $form_state) {
    $form_state->setRedirect('ebms_report.reimbursement_requests');
  }

}
