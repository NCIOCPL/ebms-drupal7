<?php

namespace Drupal\ebms_report\Form;

use Drupal\Core\Form\FormBase;
use Drupal\Core\Form\FormStateInterface;
use Drupal\Core\Url;
use Drupal\ebms_board\Entity\Board;
use Drupal\ebms_core\Entity\SavedRequest;
use Drupal\ebms_doc\Entity\Doc;
use Drupal\ebms_topic\Entity\Topic;
use Drupal\user\Entity\User;

/**
 * Report on documents uploaded to the system.
 *
 * @ingroup ebms
 */
class DocumentsReport extends FormBase {

  /**
   * {@inheritdoc}
   */
  public function getFormId(): string {
    return 'documents_report';
  }

  /**
  * {@inheritdoc}
  */
  public function buildForm(array $form, FormStateInterface $form_state, int $report_id = 0): array {

    // Make requested adjustments to the state of a document.
    $request_query = $this->getRequest()->query->all();
    if (!empty($request_query['archive'])) {
      $doc = Doc::load($request_query['archive']);
      $doc->set('dropped', TRUE);
      $doc->save();
      unset($request_query['archive']);
    }
    if (!empty($request_query['restore'])) {
      $doc = Doc::load($request_query['restore']);
      $doc->set('dropped', FALSE);
      $doc->save();
      unset($request_query['restore']);
    }

    // Prepare the values needed for the form.
    $params = empty($report_id) ? [] : SavedRequest::loadParameters($report_id);
    $boards = Board::boards();
    $board = $form_state->getValue('board', $params['board'] ?? '');
    $sort = $params['sort'] ?? 'file.entity.filename';
    $per_page = $params['per-page'] ?? '10';
    $topics = empty($board) ? [] : Topic::topics($board);
    $selected_topics = $params['topics'] ?? [];
    $selected_topics = array_values(array_diff($selected_topics, [0]));
    foreach ($selected_topics as $topic_id) {
      if (!array_key_exists($topic_id, $topics)) {
        $selected_topics = [];
        break;
      }
    }
    $storage = \Drupal::entityTypeManager()->getStorage('taxonomy_term');
    $ids = $storage->getQuery()->accessCheck(FALSE)
      ->condition('vid', 'doc_tags')
      ->condition('status', 1)
      ->sort('name')
      ->execute();
    $tags = [];
    foreach ($storage->loadMultiple($ids) as $term) {
      $tags[$term->id()] = $term->name->value;
    }
    $staff = [];
    $members = [];
    foreach (User::loadMultiple() as $user) {
      if ($user->hasRole('board_member')) {
        $members[$user->id()] = $user->name->value;
      }
      else {
        $staff[$user->id()] = $user->name->value;
      }
    }

    // Create the render array for the form's fields.
    $form = [
      '#title' => 'Documents Report',
      '#attached' => ['library' => ['ebms_report/documents-report']],
      'filters' => [
        '#type' => 'details',
        '#title' => 'Filters',
        'board' => [
          '#type' => 'select',
          '#title' => 'Board',
          '#description' => 'Optionally select a board to restrict the report to documents associated with that board.',
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
          'topics' => [
            '#type' => 'select',
            '#title' => 'Topics',
            '#multiple' => TRUE,
            '#description' => 'Optionally select one or more topics to restrict the report to documents linked to at least one of those topics.',
            '#options' => $topics,
            '#default_value' => $selected_topics,
          ],
        ],
        'tag' => [
          '#type' => 'select',
          '#title' => 'Tag',
          '#description' => 'Optionally select a tag to limit the report to documents to which that tag has been assisgned.',
          '#options' => $tags,
          '#default_value' => $params['tag'] ?? '',
          '#empty_value' => '',
        ],
        'staff' => [
          '#type' => 'select',
          '#title' => 'Uploaded By (Staff)',
          '#description' => 'Optionally restrict the report to documents uploaded by the selected staff member.',
          '#options' => $staff,
          '#sort_options' => TRUE,
          '#default_value' => $params['staff'] ?? '',
          '#empty_value' => '',
        ],
        'member' => [
          '#type' => 'select',
          '#title' => 'Uploaded By (Board Member)',
          '#description' => 'Optionally restrict the report to documents uploaded by a specific board member.',
          '#options' => $members,
          '#sort_options' => TRUE,
          '#default_value' => $params['member'] ?? '',
          '#empty_value' => '',
        ],
        'member-of-board' => [
          '#type' => 'select',
          '#title' => 'Uploaded By (Member of Selected Board)',
          '#description' => 'Optionally restrict the report to documents uploaded by a member of the selected board.',
          '#options' => $boards,
          '#default_value' => $params['member-of-board'] ?? '',
          '#empty_value' => '',
        ],
        'upload-date' => [
          '#type' => 'container',
          '#attributes' => ['class' => ['inline-fields']],
          '#title' => 'Upload Date Range',
          '#description' => 'Show documents uploaded during the specified date range.',
          'upload-start' => [
            '#type' => 'date',
            '#default_value' => $params['upload-start'] ?? '',
          ],
          'upload-end' => [
            '#type' => 'date',
            '#default_value' => $params['upload-end'] ?? '',
          ],
        ],
        'archived' => [
          '#type' => 'checkbox',
          '#title' => 'Include Archived Documents',
          '#default_value' => $params['archived'] ?? FALSE,
          '#description' => 'Check this box to show all documents which meet the other filtering criteria, including documents which have been archived.',
        ],
      ],
      'options' => [
        '#type' => 'details',
        '#title' => 'View Options',
        'sort' => [
          '#type' => 'radios',
          '#title' => 'Order By',
          '#options' => [
            'file.entity.filename' => 'File Name',
            'file.entity.uid.entity.name' => 'Uploaded By',
            'posted' => 'Date Uploaded',
          ],
          '#default_value' => $sort,
        ],
        'per-page' => [
          '#type' => 'radios',
          '#title' => 'Documents Per Page',
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
        '#submit' => ['::resetSubmit'],
        '#limit_validation_errors' => [],
      ],
    ];

    // Identify the documents for the report.
    $storage = \Drupal::entityTypeManager()->getStorage('ebms_doc');
    $query = $storage->getQuery()->accessCheck(FALSE);
    if (!empty($board)) {
      $query->condition('boards', $board);
    }
    if (!empty($selected_topics)) {
      $query->condition('topics', $selected_topics, 'IN');
    }
    if (!empty($params['tag'])) {
      $query->condition('tags', $params['tag']);
    }
    if (!empty($params['staff'])) {
      $query->condition('file.entity.uid', $params['staff']);
    }
    if (!empty($params['member'])) {
      $query->condition('file.entity.uid', $params['member']);
    }
    if (!empty($params['member-of-board'])) {
      $query->condition('file.entity.uid.entity.boards', $params['member-of-board']);
    }
    if (!empty($params['upload-start'])) {
      $query->condition('posted', $params['upload-start'], '>=');
    }
    if (!empty($params['upload-end'])) {
      $end = $params['upload-end'];
      if (strlen($end) === 10) {
        $end .= ' 23:59:59';
      }
      $query->condition('posted', $end, '<=');
    }
    if (empty($params['archived'])) {
      $query->condition('dropped', 0);
    }
    $count_query = clone $query;
    $count = $count_query->count()->execute();
    $query->sort($sort);
    if ($per_page !== 'all') {
      $query->pager($per_page);
    }

    // Assemble the render arrays for each displayed document.
    $docs = [];
    $route = 'ebms_report.documents';
    $parms = ['report_id' => $report_id];
    $caller = Url::fromRoute($route, $parms, ['query' => $request_query, 'absolute' => FALSE]);
    $options = ['query' => ['caller' => $caller->toString()]];
    foreach ($storage->loadMultiple($query->execute()) as $doc) {
      $doc_boards = [];
      foreach ($doc->boards as $doc_board) {
        $doc_boards[] = $doc_board->entity->name->value;
      }
      $doc_tags = [];
      foreach ($doc->tags as $doc_tag) {
        $doc_tags[] = $doc_tag->entity->name->value;
      }
      $doc_topics = [];
      foreach ($doc->topics as $doc_topic) {
        $doc_topics[] = $doc_topic->entity->name->value;
      }
      $archive_url = $restore_url = '';
      if (empty($doc->dropped->value)) {
        $archive_url = Url::fromRoute($route, $parms, ['query' => $request_query + ['archive' => $doc->id()]]);
      }
      else {
        $restore_url = Url::fromRoute($route, $parms, ['query' => $request_query + ['restore' => $doc->id()]]);
      }
      $docs[] = [
        'filename' => $doc->file->entity->filename->value,
        'url' => $doc->file->entity->createFileUrl(),
        'uploader' => $doc->file->entity->uid->entity->name->value,
        'uploaded' => $doc->posted->value,
        'boards' => $doc_boards,
        'topics' => $doc_topics,
        'tags' => $doc_tags,
        'edit_url' => Url::fromRoute('ebms_doc.edit', ['doc' => $doc->id()], $options),
        'archive_url' => $archive_url,
        'restore_url' => $restore_url,
      ];
    }

    // Add the report below the form and return the page's render array.
    $form['docs'] = [
      '#theme' => 'doc_report',
      '#cache' => ['max-age' => 0],
      '#docs' => $docs,
      '#total' => $count,
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
    $request = SavedRequest::saveParameters('document report', $form_state->getValues());
    $form_state->setRedirect('ebms_report.documents', ['report_id' => $request->id()]);
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
    $form_state->setRedirect('ebms_report.documents');
  }

}
