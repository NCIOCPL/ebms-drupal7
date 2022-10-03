<?php

namespace Drupal\ebms_journal\Form;

use Drupal\Core\Form\FormBase;
use Drupal\Core\Form\FormStateInterface;
use Drupal\Core\Url;
use Drupal\ebms_board\Entity\Board;
use Drupal\ebms_core\Entity\SavedRequest;
use Drupal\ebms_journal\Entity\Journal;

/**
 * Interface for managing included/excluded journals.
 *
 * @ingroup ebms
 */
class JournalMaintenanceForm extends FormBase {

  /**
   * Editing disabled with more than this number of journals per page.
   */
  const EDITING_THRESHOLD = 250;

  /**
   * {@inheritdoc}
   */
  public function getFormId(): string {
    return 'journal_maintenance';
  }

  /**
   * {@inheritdoc}
   */
  public function buildForm(array $form, FormStateInterface $form_state, int $queue_id = 0): array {
    $board = $brief_title = $full_title = $journal_id = $changes_json = '';
    $inclusion_exclusion = 'excluded';
    $per_page = 10;
    if (!empty($queue_id)) {
      $values = SavedRequest::loadParameters($queue_id);
      $board = $values['board'] ?? '';
      $brief_title = $values['brief-title'] ?? '';
      $full_title = $values['full-title'] ?? '';
      $journal_id = $values['journal_id'] ?? '';
      $inclusion_exclusion = $values['inclusion-exclusion'] ?? $inclusion_exclusion;
      $per_page = $values['per-page'] ?? $per_page;
      $changes_json = $values['changes'] ?? '{}';
      $change_items = $this->getQueuedChangeListItems($changes_json);
      $changes = json_decode($changes_json, TRUE);
    }
    $boards = Board::boards();
    $form = [
      '#title' => 'Journal Maintenance',
      '#attached' => ['library' => ['ebms_journal/maintenance']],
      'queue-id' => [
        '#type' => 'hidden',
        '#value' => $queue_id,
      ],
      'changes' => [
        '#type' => 'textfield',
        '#value' => $changes_json,
        '#ajax' => [
          'callback' => '::changesCallback',
          'event' => 'change',
          'wrapper' => 'queued-changes',
        ],
        '#attributes' => ['class' => ['hidden'], 'maxlength' => ''],
      ],
      'filters' => [
        '#type' => 'details',
        '#title' => 'Filters',
        'board' => [
          '#type' => 'select',
          '#title' => 'Editorial Board',
          '#description' => 'Select the PDQ® Board for the journals lists.',
          '#required' => TRUE,
          '#options' => $boards,
          '#default_value' => $board,
        ],
        'brief-title' => [
          '#type' => 'textfield',
          '#title' => 'Brief Journal Title',
          '#description' => 'Restrict the list to journals whose brief title includes the entered substring.',
          '#default_value' => $brief_title,
        ],
        'full-title' => [
          '#type' => 'textfield',
          '#title' => 'Full Journal Title',
          '#description' => 'Restrict the list to journals whose full title includes the entered substring.',
          '#default_value' => $full_title,
        ],
        'journal-id' => [
          '#type' => 'textfield',
          '#title' => 'Journal ID',
          '#description' => 'Restrict the list to journals whose PubMed ID includes the entered substring.',
          '#default_value' => $journal_id,
        ],
        'inclusion-exclusion' => [
          '#type' => 'radios',
          '#title' => 'Inclusion/Exclusion',
          '#required' => TRUE,
          '#options' => [
            'all' => 'All Journals',
            'included' => 'Included Journals',
            'excluded' => 'Excluded Journals',
          ],
          '#default_value' => $inclusion_exclusion,
        ],
      ],
      'options' => [
        '#type' => 'details',
        '#title' => 'Options',
        'per-page' => [
          '#type' => 'radios',
          '#title' => 'Journals Per Page',
          '#description' => 'Selecting the option to view all journals will disable editing of the Excluded values unless other filtering is applied to reduce the number of journals displayed at one time to ' . self::EDITING_THRESHOLD . ' or fewer. Any other option will preserve your queued changes across pages. Changes to this option will be reflected the next time filtering is applied.',
          '#required' => TRUE,
          '#options' => [
            '10' => '10',
            '25' => '25',
            '50' => '50',
            '100' => '100',
            'all' => 'All',
          ],
          '#default_value' => $per_page,
        ],
      ],
      'filter' => [
        '#type' => 'submit',
        '#value' => 'Filter',
      ],
      'reset' => [
        '#type' => 'submit',
        '#value' => 'Reset',
        '#submit' => ['::reset'],
        '#limit_validation_errors' => [],
      ],
    ];
    $method = $this->getRequest()->getMethod();
    ebms_debug_log("JournalMaintenanceForm::buildForm(): request method is $method");
    if (!empty($queue_id) && !empty($board) && $method !== 'POST') {
      $header = [
        'brief' => [
          'data' => 'Brief Title',
          'field' => 'brief_title',
          'specifier' => 'brief_title',
        ],
        'full' => [
          'data' => 'Full Title',
          'field' => 'title',
          'specifier' => 'title',
        ],
        'id' => [
          'data' => 'Journal ID',
          'field' => 'source_id',
          'specifier' => 'source_id',
        ],
        'excluded' => [
          'data' => 'Excluded?',
        ],
      ];
      $query = Journal::createQuery($values);
      $query->tableSort($header);
      $rows = [];
      $count_query = clone $query;
      $count = $count_query->count()->execute();
      ebms_debug_log("JournalMaintenanceForm::buildForm(): count query returned $count");
      if ($per_page !== 'all') {
        $query->pager($per_page);
      }
      $ids = $query->execute();
      $attributes = ['class' => ['exclusion-checkbox']];
      if (count($ids) > self::EDITING_THRESHOLD) {
        $attributes['disabled'] = '';
      }
      else {
        $form['apply-changes'] = [
          '#type' => 'submit',
          '#value' => 'Apply Queued Changes',
          '#states' => [
            'invisible' => [
              ':input[name="changes"]' => ['value' => '{}'],
            ],
          ],
        ];
      }
      if (!empty($count)) {
        $url = Url::fromRoute('ebms_journal.print_friendly', ['saved_request' => $queue_id])->toString();
        $form['print-friendly'] = [
          '#type' => 'button',
          '#value' => 'Print-Friendly Version',
          '#attributes' => ['onclick' => "window.open('$url', '_blank')"],
        ];
      }
      if (count($ids) <= self::EDITING_THRESHOLD) {
        $form['queued-changes'] = [
          '#type' => 'container',
          '#attributes' => ['id' => 'queued-changes'],
          'changes-list' => [
            '#theme' => 'item_list',
            '#title' => 'Queued Changes',
            '#empty' => 'No changes have been queued.',
            '#list_thype' => 'ul',
            '#items' => $change_items,
          ],
        ];
      }
      foreach (Journal::loadMultiple($ids) as $journal) {
        $id = $journal->id();
        $excluded = FALSE;
        foreach ($journal->not_lists as $not_list) {
          if ($not_list->board == $board) {
            $excluded = TRUE;
          }
        }
        $stored = $excluded ? 'excluded' : 'included';
        if (array_key_exists($id, $changes)) {
          $excluded = $changes[$id] === 'excluded';
        }
        $key = "journal-$id-$stored";
        $row = [
          $journal->brief_title->value,
          $journal->title->value,
          $journal->source_id->value,
          [
            'data' => [
              '#type' => 'checkbox',
              '#name' => $key,
              '#id' => $key,
              '#checked' => $excluded,
              '#title' => ' ',
              '#attributes' => $attributes,
            ],
          ],
        ];
        $rows[] = $row;
      }
      $form['journals'] = [
        '#type' => 'table',
        '#caption' => "Journals ($count)",
        '#header' => $header,
        '#rows' => $rows,
      ];
      if ($per_page !== 'all') {
        $form['pager'] = ['#type' => 'pager'];
      }
    }
    return $form;
  }

  /**
   * {@inheritdoc}
   */
  public function submitForm(array &$form, FormStateInterface $form_state) {
    $trigger = $form_state->getTriggeringElement()['#value'];
    if ($trigger === 'Filter' || $trigger === 'Apply Queued Changes') {
      $values = $form_state->getValues();
      if ($trigger === 'Apply Queued Changes') {
        $changes_json = $form_state->getUserInput()['changes'] ?? '{}';
        $changes = json_decode($changes_json, TRUE);
        $board_id = $values['board'];
        $now = date('Y-m-d H:i:s');
        $uid = $this->currentUser()->id();
        $count = 0;
        foreach ($changes as $id => $value) {
          $journal = Journal::load($id);
          $changed = $found = FALSE;
          foreach ($journal->not_lists as $position => $not_list) {
            if ($not_list->board == $board_id) {
              $found = TRUE;
              if ($value === 'included') {
                $journal->not_lists->removeItem($position);
                $changed = TRUE;
              }
            }
          }
          if ($value === 'excluded' && !$found) {
            $journal->not_lists[] = [
              'board' => $board_id,
              'start' => $now,
              'user' => $uid,
            ];
            $changed = TRUE;
          }
          if ($changed) {
            $count++;
            $journal->save();
          }
        }
        $what = $count === 1 ? '1 queued change' : "$count queued changes";
        $this->messenger()->addMessage("$what successfully saved.");
      }
      $values['changes'] = '{}';
      $request = SavedRequest::saveParameters('journal queue', $values);
      $form_state->setRedirect('ebms_journal.maintenance', ['queue_id' => $request->id()]);
    }
  }

  public function reset(array &$form, FormStateInterface $form_state) {
    $form_state->setRedirect('ebms_journal.maintenance');
  }

  /**
   * Update the queued changes.
   *
   * We have to pull the queued changes directly from the field because
   * Drupal hasn't yet updated the processed values array.
   *
   * @param array $form
   *   An associative array containing the structure of the form.
   * @param \Drupal\Core\Form\FormStateInterface $form_state
   *   The object tracking the current state of the form.
   *
   * @return array
   *   AJAX response.
   */
  public function changesCallback(array &$form, FormStateInterface $form_state): array {
    $changes_json = $form_state->getUserInput()['changes'] ?? '{}';
    $queue_id = $form_state->getValue('queue-id');
    $request = SavedRequest::load($queue_id);
    $parameters = $request->getParameters();
    $parameters['changes'] = $changes_json;
    $parameters_json = json_encode($parameters);
    $request->set('parameters', $parameters_json);
    $request->save();
    $items = $this->getQueuedChangeListItems($changes_json);
    $form['queued-changes']['changes-list']['#items'] = $items;
    return $form['queued-changes'];
  }

  /**
   * Collect the strings to display changes waiting to be saved.
   *
   * @param string $changes_json
   *   Serialized values pulled from our hidden field to track queued changes.
   *
   * @return array
   *   Strings describing the queued changes.
   */
  private function getQueuedChangeListItems($changes_json): array {
    $items = [];
    if ($changes_json !== '{}') {
      $changes = json_decode($changes_json, TRUE);
      foreach ($changes as $id => $value) {
        $journal = Journal::load($id);
        $brief_title = $journal->brief_title->value;
        $journal_id = $journal->source_id->value;
        $items[] = "Journal $brief_title ($journal_id) will be $value.";
      }
    }
    return $items;
  }

}
