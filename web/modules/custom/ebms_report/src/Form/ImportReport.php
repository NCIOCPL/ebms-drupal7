<?php

namespace Drupal\ebms_report\Form;

use Drupal\Core\Form\FormBase;
use Drupal\Core\Form\FormStateInterface;
use Drupal\Core\Link;
use Drupal\ebms_board\Entity\Board;
use Drupal\ebms_core\Entity\SavedRequest;
use Drupal\ebms_import\Entity\Batch;
use Drupal\ebms_import\Entity\ImportRequest;
use Drupal\ebms_topic\Entity\Topic;

/**
 * Placeholder for a report which has not yet been implemented.
 *
 * @ingroup ebms
 */
class ImportReport extends FormBase {

  /**
   * {@inheritdoc}
   */
  public function getFormId(): string {
    return 'import_report_form';
  }

  /**
   * {@inheritdoc}
   */
  public function buildForm(array $form, FormStateInterface $form_state, int $report_id = 0): array {

    // Set some default values.
    $params = empty($report_id) ? [] : SavedRequest::loadParameters($report_id);
    $end_date = $params['date-end'] ?? date('Y-m-d');
    $start_date = $params['date-start'] ?? date('Y-m-d', strtotime("$end_date -1 month +1 day"));
    $per_page = $params['per-page'] ?? 10;

    // Assemble the render array for the form's fields.
    $form = [
      '#title' => 'Import Report',
      'filters' => [
        '#type' => 'details',
        '#title' => 'Filters',
        'board' => [
          '#type' => 'select',
          '#title' => 'Editorial Board',
          '#options' => Board::boards(),
          '#default_value' => $params['board'] ?? '',
          '#description' => 'Optionally restrict the report to imports for a specific board.',
          '#empty_value' => '',
        ],
        'topic' => [
          '#type' => 'select',
          '#title' => 'Summary Topic',
          '#options' => Topic::topics(),
          '#default_value' => $params['topic'] ?? '',
          '#description' => 'Optionally restrict the report to imports for a specific topic.',
          '#empty_value' => '',
        ],
        'cycle' => [
          '#type' => 'select',
          '#title' => 'Review Cycle',
          '#options' => Batch::cycles(),
          '#description' => 'Optionally restrict the report to imports for a specific review cycle.',
          '#default_value' => $params['cycle'] ?? '',
          '#empty_value' => '',
        ],
        'dates' => [
          '#type' => 'container',
          '#attributes' => ['class' => ['inline-fields']],
          '#title' => 'Import Date Range',
          '#description' => 'Show import jobs performed during the specified date range.',
          'date-start' => [
            '#type' => 'date',
            '#default_value' => $start_date,
          ],
          'date-end' => [
            '#type' => 'date',
            '#default_value' => $end_date,
          ],
        ],
      ],
      'display-options' => [
        '#type' => 'details',
        '#title' => 'Display Options',
        'per-page' => [
          '#type' => 'radios',
          '#title' => 'Import Jobs Per Page',
          '#options' => [
            '10' => '10',
            '25' => '25',
            '50' => '50',
            '100' => '100',
            'all' => 'All',
          ],
          '#default_value' => $per_page,
          '#description' => 'How many import jobs should be display per page?',
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

    // Show the details of a single job is so requested.
    $request_id = $this->getRequest()->query->get('request');
    if (!empty($request_id)) {
      $request = ImportRequest::load($request_id);
      $batch_id = $request->batch->target_id;
      $importer = $request->batch->entity->user->entity->name->value;
      $imported = substr($request->batch->entity->imported->value, 0, 10);
      $form['report'] = $request->getReport("Job $batch_id Imported $imported by $importer");
    }

    // Otherwise, show the jobs which match the filtering criteria.
    elseif (!empty($params)) {

      // Support column sorting of the report table.
      $header = [
        'job-id' => [
          'data' => 'Job',
        ],
        'imported' => [
          'data' => 'Imported',
          'field' => 'batch.entity.imported',
          'specifier' => 'batch.entity.imported',
          'sort' => 'desc',
        ],
        'board' => [
          'data' => 'Board',
          'field' => 'batch.entity.topic.entity.board.entity.name',
          'specifier' => 'batch.entity.topic.entity.board.entity.name',
        ],
        'topic' => [
          'data' => 'Topic',
          'field' => 'batch.entity.topic.entity.name',
          'specifier' => 'batch.entity.topic.entity.name',
        ],
        'article-count' => [
          'data' => 'Articles',
          'field' => 'batch.entity.article_count',
          'specifier' => 'batch.entity.article_count',
        ],
      ];

      // Find the matching import jobs which have a topic specified.
      $storage = \Drupal::entityTypeManager()->getStorage('ebms_import_request');
      $query = $storage->getQuery()->accessCheck(FALSE);
      $topic_or_board_filtered = FALSE;
      if (!empty($params['topic'])) {
        $topic_or_board_filtered = TRUE;
        $query->condition('batch.entity.topic', $params['topic']);
      }
      elseif (!empty($params['board'])) {
        $topic_or_board_filtered = TRUE;
        $query->condition('batch.entity.topic.entity.board', $params['board']);
      }
      if (!empty($params['cycle'])) {
        $query->condition('batch.entity.cycle', $params['cycle']);
      }
      if (!empty($params['date-start'])) {
        $query->condition('batch.entity.imported', $params['date-start'], '>=');
      }
      if (!empty($params['date-end'])) {
        $query->condition('batch.entity.imported', $params['date-end'] . ' 23:59:59', '<=');
      }
      if (!$topic_or_board_filtered) {
        $query->exists('batch.entity.topic');
      }
      $count_query = clone $query;
      $count = $count_query->count()->execute();
      if ($per_page !== 'all') {
        $query->pager($per_page);
      }
      $query->tableSort($header);

      // Collect the information for the table rows.
      $rows = [];
      foreach ($storage->loadMultiple($query->execute()) as $request) {
        $request_id = $request->id();
        $batch_id = $request->batch->target_id;
        $rows[] = [
          Link::createFromRoute($batch_id, 'ebms_report.import', ['report_id' => $report_id], ['query' => ['request' => $request_id]]),
          substr($request->batch->entity->imported->value, 0, 10),
          $request->batch->entity->topic->entity->board->entity->name->value,
          $request->batch->entity->topic->entity->name->value,
          $request->batch->entity->article_count->value,
        ];
      }

      // Add the table to the page and return the completed render array.
      $form['jobs'] = [
        '#theme' => 'table',
        '#caption' => "Import Jobs ($count)",
        '#rows' => $rows,
        '#header' => $header,
        '#empty' => 'No jobs match the filter criteria.',
      ];
      $form['pager'] = [
        '#type' => 'pager',
      ];
    }
    return $form;
  }

  /**
   * {@inheritdoc}
   */
  public function submitForm(array &$form, FormStateInterface $form_state) {
    $request = SavedRequest::saveParameters('imports report', $form_state->getValues());
    $form_state->setRedirect('ebms_report.import', ['report_id' => $request->id()]);
  }

  /**
   * Create a fresh form.
   */
  public function resetSubmit(array &$form, FormStateInterface $form_state) {
    $form_state->setRedirect('ebms_report.import');
  }

}
