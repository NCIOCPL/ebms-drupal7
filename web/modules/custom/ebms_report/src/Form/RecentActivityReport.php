<?php

namespace Drupal\ebms_report\Form;

use Drupal\Core\Form\FormBase;
use Drupal\Core\Form\FormStateInterface;
use Drupal\ebms_board\Entity\Board;
use Drupal\ebms_core\Entity\SavedRequest;
use Drupal\user\Entity\User;

/**
 * Summarize recent board-related activity in the EBMS.
 *
 * @ingroup ebms
 */
class RecentActivityReport extends FormBase {

  /**
   * {@inheritdoc}
   */
  public function getFormId(): string {
    return 'recent_activity_report_form';
  }

  /**
   * {@inheritdoc}
   */
  public function buildForm(array $form, FormStateInterface $form_state): array {

    // Create defaults for the report's date range.
    $user = User::load($this->currentUser()->id());
    $end_date = date('Y-m-d');
    $start_date = date('Y-m-d', strtotime("$end_date -1 year +1 day"));

    // Assemble and return the render array for the form.
    return [
      '#title' => 'Recent Activity Report',
      'filters' => [
        '#type' => 'details',
        '#title' => 'Filters',
        'boards' => [
          '#type' => 'checkboxes',
          '#required' => TRUE,
          '#title' => 'Editorial Board(s)',
          '#options' => Board::boards(),
          '#default_value' => [Board::defaultBoard($user)],
          '#description' => 'Select one or more boards for inclusion in the report.',
        ],
        'types' => [
          '#type' => 'checkboxes',
          '#required' => TRUE,
          '#title' => 'Activity Type(s)',
          '#description' => 'Select which of the types of activity should appear on the report.',
          '#options' => [
            'literature' => 'Literature',
            'document' => 'Document',
            'meeting' => 'Meeting',
          ],
          '#default_value' => ['literature', 'document'],
        ],
        'dates' => [
          '#type' => 'container',
          '#attributes' => ['class' => ['inline-fields']],
          '#title' => 'Activity Date Range',
          '#required' => TRUE,
          '#description' => 'Limit the report to activity occurring during the specified date range.',
          'date-start' => [
            '#type' => 'date',
            '#required' => TRUE,
            '#default_value' => $start_date,
          ],
          'date-end' => [
            '#type' => 'date',
            '#default_value' => $end_date,
            '#required' => TRUE,
          ],
        ],
      ],
      'display-options' => [
        '#type' => 'details',
        '#title' => 'Display Options',
        'options' => [
          '#type' => 'checkboxes',
          '#title' => 'Options',
          '#options' => [
            'group' => 'Group activity by type under each board',
          ],
          '#description' => 'Decide how the report should be organized.',
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
  }

  /**
   * {@inheritdoc}
   */
  public function submitForm(array &$form, FormStateInterface $form_state) {
    $values = $form_state->getValues();
    $request = SavedRequest::saveParameters('recent activity report', $values);
    $form_state->setRedirect('ebms_report.recent_activity_report', ['report_id' => $request->id()]);
  }

  /**
   * Create a fresh form.
   */
  public function resetSubmit(array &$form, FormStateInterface $form_state) {
    $form_state->setRedirect('ebms_report.recent_activity');
  }

}
