<?php

namespace Drupal\ebms_report\Form;

use Drupal\Core\Form\FormBase;
use Drupal\Core\Form\FormStateInterface;
use Drupal\ebms_board\Entity\Board;
use Drupal\ebms_core\Entity\SavedRequest;

/**
 * Placeholder for a report which has not yet been implemented.
 *
 * @ingroup ebms
 */
class ToBeImplemented extends FormBase {

  /**
   * {@inheritdoc}
   */
  public function getFormId(): string {
    return 'to_be_implemented';
  }

  /**
   * {@inheritdoc}
   */
  public function buildForm(array $form, FormStateInterface $form_state): array {
    $form = [
      '#title' => 'Title Goes Here',
      '#attached' => ['library' => ['ebms_report/library-id-goes-here']],
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
    return ['#markup' => 'This report is under construction.'];
  }

  /**
   * {@inheritdoc}
   */
  public function submitForm(array &$form, FormStateInterface $form_state) {
    $params = $form_state->getValues();
    dpm($params);
    $request = SavedRequest::saveParameters('name of the report', $params);
    $form_state->setRedirect('ebms_report.route_id_goes_here', ['report_id' => $request->id()]);
  }

  /**
   * Create a fresh form.
   */
  public function resetSubmit(array &$form, FormStateInterface $form_state) {
    $form_state->setRedirect('ebms_report.route_id_goes_here');
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
