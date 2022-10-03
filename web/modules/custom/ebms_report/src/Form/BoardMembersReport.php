<?php

namespace Drupal\ebms_report\Form;

use Drupal\Core\Form\FormBase;
use Drupal\Core\Form\FormStateInterface;
use Drupal\ebms_board\Entity\Board;
use Drupal\ebms_core\Entity\SavedRequest;

/**
 * Form for submitting a request for a hotel reservation.
 *
 * @ingroup ebms
 */
class BoardMembersReport extends FormBase {
   /**
   * {@inheritdoc}
   */
  public function getFormId(): string {
    return 'board_members';
  }

  /**
  * {@inheritdoc}
  */
  public function buildForm(array $form, FormStateInterface $form_state, $report_id = 0): array {
    // Set the defaults, then override them as appropriate.
    $report = NULL;
    $default_board = '';
    $include_groups = FALSE;
    if (!empty($report_id)) {
      $report = SavedRequest::load($report_id);
      $parameters = $report->getParameters();
      $default_board = $parameters['board'];
      $include_groups = !empty($parameters['include_groups']);
    }

    // Need the list of boards for the drop-down list.
    $boards = Board::boards();

    /* Present the original form to submit which includes
     * - drop-down list
     * - checkbox for subgroups
     * - submit button
     */
    $form =  [
      'filters' => [
        '#type' => 'details',
        '#title' => 'Filters',
        'board' => [
          '#type' => 'select',
          '#title' => 'Editorial Board',
          '#default_value' => $default_board,
          '#options' => $boards,
          '#required' => TRUE,
          '#description' => 'The report is specific to a single PDQ® Editorial Board.',
        ],
        'include_groups' => [
            '#type' => 'checkbox',
            '#title' => 'Show Subgroups',
            '#default_value' => $include_groups,
            '#description' => "Optionally also include membership in each of the board's smaller groups. Some boards have no subgroups.",
        ],
      ],
      'submit' => [
        '#type' => 'submit',
        '#value' => 'Report',
      ],
    ];
    if (!empty($report)) {
      $form['report'] = $this->showReport($report);
    }
    return $form;
  }

  /**
  * {@inheritdoc}
  */
  public function submitForm(array &$form, FormStateInterface $form_state) {
    $request = SavedRequest::saveParameters('board members report', $form_state->getValues());
    $route = 'ebms_report.board_members';
    $parameters = ['report_id' => $request->id()];
    $options = ['query' => $this->getRequest()->query->all()];
    $form_state->setRedirect($route, $parameters, $options);
  }

  /**
   * Create the report.
   *
   * @param SavedRequest $request
   *   Object holding the parameters for the report request.
   *
   * @return array
   *   Render array for the report.
   */
  private function showReport(SavedRequest $request): array {

    // Get the parameters we need for the report.
    $parameters = $request->getParameters();
    $board_id = $parameters['board'];
    $include_groups = $parameters['include_groups'];

    // Retrieve members for the board selected for the full report.
    $boards = Board::boards();
    $board_name = $boards[$board_id];
    $members = Board::boardMembers($board_id);
    $all_members = [];
    foreach ($members as $member) {
      $all_members[] = $member->name->value;
    }

    // Creating the full board members report.
    $board_report = [
      '#theme' => 'item_list',
      '#list_type' => 'ul',
      '#title' => $board_name,
      '#items' => $all_members,
    ];

    // Create the individual subgroup reports if requested.
    $sub_group_report = [];
    if ($include_groups) {

      // Identify the groups for this board.
      $storage = \Drupal::entityTypeManager()->getStorage('ebms_group');
      $query = $storage->getQuery()->accessCheck(FALSE);
      $query->sort('name');
      $query->condition('boards', $board_id);
      $subgroups = $storage->loadMultiple($query->execute());

      // Retrieve the user information.
      $user_storage = \Drupal::entityTypeManager()->getStorage('user');

      // Iterate over the subgroups to retrieve the corresponding group
      // members for each group.
      foreach ($subgroups as $subgroup) {
        $query = $user_storage->getQuery()->accessCheck(FALSE)->condition('groups', $subgroup->id());
        $group_members = $user_storage->loadMultiple($query->execute());
        $group_names = [];
        foreach ($group_members as $member) {
          $group_names[] = $member->name->value;
        }

        // Prepare the report for the current subgroup.
        $sub_group_report[] = [
          '#theme' => 'item_list',
          '#list_type' => 'ul',
          '#title' => $subgroup->name->value,
          '#items' => $group_names,
        ];
      }
    }
    return [$board_report, $sub_group_report];
  }

}
