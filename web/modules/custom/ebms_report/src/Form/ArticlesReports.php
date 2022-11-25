<?php

namespace Drupal\ebms_report\Form;

use Drupal\Core\Form\FormBase;
use Drupal\Core\Form\FormStateInterface;
use Drupal\Core\Link;
use Drupal\Core\Url;
use Drupal\ebms_board\Entity\Board;
use Drupal\ebms_core\Entity\SavedRequest;
use Drupal\ebms_import\Entity\Batch;
use Drupal\ebms_import\Entity\ImportRequest;

/**
 * Collection of basic article reports on a single form page.
 *
 * These reports are largely statistical/historical, so they are not
 * driven by which states are current at the time the report is requested,
 * but rather by which states were entered at specified times (or for
 * specific review cycles), even if those states have been superceded
 * since that time. See the "Articles By Status" reports for logic which
 * is driven by current states.
 *
 * @todo Split out random collections of reports to their own pages.
 *
 * @ingroup ebms
 */
class ArticlesReports extends FormBase {

  /**
   * Names of the sub reports.
   */
  const PUBLISHED = 'Articles Published';
  const PUB_DECISIONS = 'Articles Rejected/Accepted for Publishing';
  const IMPORTED = 'Articles Imported';
  const REJECTED_FROM_ABSTRACT = 'Articles Rejected In Review From Abstract';
  const TOPIC_CHANGES = 'Article Summary Topic Changes';
  const JOURNAL_EXCLUSION = 'Articles Excluded by Journal';
  const LIBRARIAN_REVIEW = 'Articles Reviewed by Medical Librarian';
  const APPROVED_FROM_ABSTRACT = 'Articles Approved in Review From Abstract';
  const REPORTS = [
    ArticlesReports::PUBLISHED,
    ArticlesReports::PUB_DECISIONS,
    ArticlesReports::IMPORTED,
    ArticlesReports::REJECTED_FROM_ABSTRACT,
    ArticlesReports::TOPIC_CHANGES,
    ArticlesReports::JOURNAL_EXCLUSION,
    ArticlesReports::LIBRARIAN_REVIEW,
    ArticlesReports::APPROVED_FROM_ABSTRACT,
  ];


  /**
   * {@inheritdoc}
   */
  public function getFormId(): string {
    return 'articles_reports';
  }

  /**
   * {@inheritdoc}
   */
  public function buildForm(array $form, FormStateInterface $form_state): array {

    // We need this in multiple place, so fetch them once here.
    $cycles = Batch::cycles();

    // Assemble the form fields.
    $form = [
      '#title' => 'Article Statistics Reports',
      '#attached' => ['library' => ['ebms_report/articles-reports']],
      '#cache' => ['max-age' => 0],
      'filters' => [
        '#type' => 'details',
        '#title' => 'Filters',
        'report' => [
          '#type' => 'select',
          '#title' => 'Report',
          '#description' => 'Choose which of the reports to create.',
          '#options' => array_combine(self::REPORTS, self::REPORTS),
          '#attributes' => ['name' => 'report'],
          '#default_value' => $form_state->getValue('report') ?? '',
        ],
        'board' => [
          '#type' => 'select',
          '#title' => 'Editorial Board',
          '#description' => 'Optionally narrow the report to a single board.',
          '#options' => Board::boards(),
          '#empty_value' => '',
          '#default_value' => $form_state->getValue('board') ?? '',
          '#states' => [
            'invisible' => [
              ':input[name="report"]' => ['value' => self::TOPIC_CHANGES],
            ],
          ],
        ],
        'cycle' => [
          '#type' => 'select',
          '#title' => 'Review Cycle',
          '#description' => 'Restrict the report to articles assigned to a single review cycle. Required if a cycle range (or date range in the case of the "Articles Not Selected for Full-Text Retrieval" reort) is not specified.',
          '#options' => $cycles,
          '#empty_value' => '',
          '#default_value' => $form_state->getValue('cycle') ?? '',
          '#states' => [
            'invisible' => [
              ':input[name="report"]' => ['value' => self::TOPIC_CHANGES],
            ],
          ],
        ],
        'cycles' => [
          '#type' => 'container',
          '#attributes' => ['class' => ['inline-fields']],
          '#title' => 'Review Cycle Range',
          '#description' => 'Optionally restrict the report using a range of review cycles. For the "Articles Not Selected for Full-Text Retrieval" report, specifying a review cycle range will result in the per-topic statistics version of the report, whereas a single cycle or a decision date range will produce a per-article details report.',
          '#states' => [
            'invisible' => [
              ':input[name="report"]' => ['value' => self::TOPIC_CHANGES],
            ],
          ],
          'cycle-start' => [
            '#type' => 'select',
            '#options' => $cycles,
            '#empty_value' => '',
            '#default_value' => $form_state->getValue('cycle-start') ?? '',
          ],
          'cycle-end' => [
            '#type' => 'select',
            '#options' => $cycles,
            '#empty_value' => '',
            '#default_value' => $form_state->getValue('cycle-end') ?? '',
          ],
        ],
        'decision-dates' => [
          '#type' => 'container',
          '#attributes' => ['class' => ['inline-fields']],
          '#title' => 'Decision Date Range',
          '#description' => 'Limit the report by date of decision.',
          '#states' => [
            'visible' => [
              [':input[name="report"]' => ['value' => self::TOPIC_CHANGES]],
              'or',
              [':input[name="report"]' => ['value' => self::REJECTED_FROM_ABSTRACT]],
            ],
          ],
          'decision-date-start' => [
            '#type' => 'date',
            '#default_value' => $form_state->getValue('decision-date-start') ?? '',
          ],
          'decision-date-end' => [
            '#type' => 'date',
            '#default_value' => $form_state->getValue('decision-date-end') ?? '',
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

    // Add the report if one has been requested.
    $values = $form_state->getValues();
    if (!empty($values['report'])) {
      switch ($values['report']) {
        case self::PUBLISHED:
          $form['report'] = $this->publishedReport($values);
          break;
        case self::PUB_DECISIONS:
          $form['report'] = $this->pubDecisionsReport($values);
          break;
        case self::IMPORTED:
          $form['report'] = $this->importedReport($values);
          break;
        case self::REJECTED_FROM_ABSTRACT:
          $form['report'] = $this->abstractRejectedReport($values);
          break;
        case self::TOPIC_CHANGES:
          $form['report'] = $this->topicChangesReport($values);
          break;
        case self::JOURNAL_EXCLUSION:
          $form['report'] = $this->journalExclusionReport($values);
          break;
        case self::LIBRARIAN_REVIEW:
          $form['report'] = $this->librarianReviewReport($values);
          break;
        case self::APPROVED_FROM_ABSTRACT:
          $form['report'] = $this->abstractApprovedReport($values);
          break;
        }
    }
    return $form;
  }

  /**
   * {@inheritdoc}
   */
  public function validateForm(array &$form, FormStateInterface $form_state) {
    $values = $form_state->getValues();
    $report = $values['report'];
    if ($report === self::TOPIC_CHANGES) {
      if (empty($values['decision-date-start'])) {
        if (empty($values['decision-date-end'])) {
          $form_state->setErrorByName('decision-date-start', 'No decision date range specified.');
        }
        else {
          $form_state->setErrorByName('decision-date-start', 'Beginning of decision date range missing.');
        }
      }
      elseif (empty($values['decision-date-end'])) {
        $form_state->setErrorByName('decision-date-end', 'End of decision date range missing.');
      }
    }
    elseif (!empty($values['cycle'])) {
      if (!empty($values['cycle-start'])) {
        $form_state->setErrorByName('cycle-start', 'Cannot specifiy both a single cycle and a cycle range.');
      }
      elseif (!empty($values['cycle-start'])) {
        $form_state->setErrorByName('cycle-end', 'Cannot specifiy both a single cycle and a cycle range.');
      }
      elseif ($report === self::REJECTED_FROM_ABSTRACT) {
        if (!empty($values['decision-date-start'])) {
          $form_state->setErrorByName('cycle-end', 'Cannot specifiy both a single cycle and a decision date range.');
        }
        elseif (!empty($values['decision-date-end'])) {
          $form_state->setErrorByName('cycle-end', 'Cannot specifiy both a single cycle and a decision date range.');
        }
      }
    }
    elseif (!empty($values['cycle-start']) || !empty($values['cycle-end'])) {
      if (empty($values['cycle-start'])) {
        $form_state->setErrorByName('cycle-start', 'Beginning of cycle range missing.');
      }
      elseif (empty($values['cycle-end'])) {
        $form_state->setErrorByName('cycle-end', 'End of cycle range missing.');
      }
      elseif ($report === self::REJECTED_FROM_ABSTRACT) {
        if (!empty($values['decision-date-start'])) {
          $form_state->setErrorByName('decision-date-end', 'Cannot specifiy both a cycle range and a decision date range.');
        }
        elseif (!empty($values['decision-date-end'])) {
          $form_state->setErrorByName('decision-date-end', 'Cannot specifiy both a cycle range and a decision date range.');
        }
      }
    }
    elseif ($report === self::REJECTED_FROM_ABSTRACT) {
      if (empty($values['decision-date-start'])) {
        if (empty($values['decision-date-end'])) {
          $form_state->setErrorByName('cycle', 'No cycle or decision date range specified.');
        }
        else {
          $form_state->setErrorByName('decision-date-start', 'Beginning of decision date range missing.');
        }
      }
      elseif (empty($values['decision-date-end'])) {
        $form_state->setErrorByName('decision-date-end', 'End of decision date range missing.');
      }
    }
    else {
      $form_state->setErrorByName('cycle', 'No cycle or cycle range specified.');
    }
  }

  /**
   * {@inheritdoc}
   */
  public function submitForm(array &$form, FormStateInterface $form_state) {
    SavedRequest::saveParameters('article statistics report', $form_state->getValues());
    $form_state->setRebuild(TRUE);
  }

  /**
   * Create a fresh form.
   */
  public function resetSubmit(array &$form, FormStateInterface $form_state) {
    $form_state->setRedirect('ebms_report.articles');
  }

  /**
   * Show counts of articles published for the specified review cycle(s).
   *
   * @param array $values
   *   Values entered by the user on the request form.
   *
   * @return array
   *   Render array for the report.
   */
  private function publishedReport(array $values): array {

    // See whether we have a single cycle or a range.
    if (!empty($values['cycle'])) {
      $cycle = $values['cycle'];
      $cycle_string = Batch::cycleString($cycle);
      $cycle_display = "Review cycle: $cycle_string";
    }
    else {
      $cycle = [$values['cycle-start'], $values['cycle-end']];
      $cycle_start = Batch::cycleString($cycle[0]);
      $cycle_end = Batch::cycleString($cycle[1]);
      $cycle_display = "Review cycle range: $cycle_start through $cycle_end";
    }

    // Initialize the report's render array with some header information.
    $report = [
      'heading-wrapper' => [
        '#type' => 'container',
        '#attributes' => ['class' => ['report-header']],
        'heading' => [
          '#theme' => 'item_list',
          '#title' => 'Articles Published',
          '#items' => [
            'Report Date: ' . date('Y-m-d'),
            $cycle_display,
            'If an article was published for more than one topic, the total for the board will be lower than then sum of the topic counts.',
          ],
        ],
      ],
    ];

    // Initialize the array in which we collect the data.
    if (!empty($values['board'])) {
      $board = Board::load($values['board']);
      $boards = [
        $values['board'] => [
          'name' => $board->name->value,
          'topics' => [],
          'articles' => [],
        ],
      ];
    }
    else {
      $boards = [];
      foreach (Board::boards() as $board_id => $board_name) {
        $boards[$board_id] = [
          'name' => $board_name,
          'topics' => [],
          'articles' => [],
        ];
      }
    }

    // Create an entity query for the states we're looking for.
    $storage = \Drupal::entityTypeManager()->getStorage('ebms_state');
    $query = $storage->getQuery()->accessCheck(FALSE);
    $query->condition('value.entity.field_text_id', 'published');
    if (!empty($values['board'])) {
      $query->condition('board', $values['board']);
    }
    $query->addTag('states_for_cycle');
    $query->addMetaData('cycle', $cycle);
    $query->addMetaData('operator', is_array($cycle) ? 'BETWEEN' : '=');

    // Fetch the state entities and collect the data.
    foreach ($storage->loadMultiple($query->execute()) as $state) {
      $board_id = $state->board->target_id;
      $topic_id = $state->topic->target_id;
      $article_id = $state->article->target_id;
      $boards[$board_id]['articles'][$article_id] = $article_id;
      if (!array_key_exists($topic_id, $boards[$board_id]['topics'])) {
        $boards[$board_id]['topics'][$topic_id] = [
          'name' => $state->topic->entity->name->value,
          'count' => 1,
        ];
      }
      else {
        $boards[$board_id]['topics'][$topic_id]['count']++;
      }
    }

    // Create a separate table for each board.
    foreach ($boards as &$board) {
      if (!empty($board['articles'])) {
        usort($board['topics'], function(array &$a, array &$b): int {
          return $a['name'] <=> $b['name'];
        });
        $rows = [];
        foreach ($board['topics'] as $topic) {
          $rows[] = [$topic['name'], $topic['count']];
        }
        $rows[] = ['Total', count($board['articles'])];
        $report[] = [
          '#type' => 'table',
          '#caption' => $board['name'],
          '#header' => ['Topic', 'Published'],
          '#rows' => $rows,
          '#attributes' => ['class' => ['articles-published-table']],
        ];
      }
    }
    return $report;
  }

  /**
   * Show counts for the decisions made during the preliminary review.
   *
   * @param array $values
   *   Values entered by the user on the request form.
   *
   * @return array
   *   Render array for the report.
   */
  private function pubDecisionsReport(array $values): array {

    // See whether we have a single cycle or a range.
    if (!empty($values['cycle'])) {
      $cycle = $values['cycle'];
      $cycle_string = Batch::cycleString($cycle);
      $cycle_display = "Review cycle: $cycle_string";
    }
    else {
      $cycle = [$values['cycle-start'], $values['cycle-end']];
      $cycle_start = Batch::cycleString($cycle[0]);
      $cycle_end = Batch::cycleString($cycle[1]);
      $cycle_display = "Review cycle range: $cycle_start through $cycle_end";
    }

    // Initialize the report's render array with some header information.
    $report = [
      'heading-wrapper' => [
        '#type' => 'container',
        '#attributes' => ['class' => ['report-header']],
        'heading' => [
          '#theme' => 'item_list',
          '#title' => 'Articles Rejected/Accepted for Publication',
          '#items' => [
            'Report Date: ' . date('Y-m-d'),
            $cycle_display,
            'If an article was reviewed for more than one topic, the total for the board may be lower than then sum of the topic counts.',
          ],
        ],
      ],
    ];

    // Initialize the array in which we collect the data.
    if (!empty($values['board'])) {
      $board = Board::load($values['board']);
      $boards = [
        $values['board'] => [
          'name' => $board->name->value,
          'topics' => [],
          'articles_approved' => [],
          'articles_rejected' => [],
        ],
      ];
    }
    else {
      $boards = [];
      foreach (Board::boards() as $board_id => $board_name) {
        $boards[$board_id] = [
          'name' => $board_name,
          'topics' => [],
          'passed_init_review' => [],
          'reject_init_review' => [],
        ];
      }
    }

    // Create an entity query for the states we're looking for.
    $storage = \Drupal::entityTypeManager()->getStorage('ebms_state');
    $query = $storage->getQuery()->accessCheck(FALSE);
    $query->condition('value.entity.field_text_id', ['passed_init_review', 'reject_init_review'], 'IN');
    if (!empty($values['board'])) {
      $query->condition('board', $values['board']);
    }
    $query->addTag('states_for_cycle');
    $query->addMetaData('cycle', $cycle);
    $query->addMetaData('operator', is_array($cycle) ? 'BETWEEN' : '=');

    // Fetch the state entities and collect the data.
    foreach ($storage->loadMultiple($query->execute()) as $state) {
      $board_id = $state->board->target_id;
      $topic_id = $state->topic->target_id;
      $article_id = $state->article->target_id;
      $decision = $state->value->entity->field_text_id->value;
      $boards[$board_id][$decision][$article_id] = $article_id;
      if (!array_key_exists($topic_id, $boards[$board_id]['topics'])) {
        $boards[$board_id]['topics'][$topic_id] = [
          'name' => $state->topic->entity->name->value,
          'passed_init_review' => 0,
          'reject_init_review' => 0,
        ];
      }
      $boards[$board_id]['topics'][$topic_id][$decision]++;
    }

    // Create a separate table for each board.
    foreach ($boards as &$board) {
      if (!empty($board['passed_init_review']) || !empty($board['reject_init_review'])) {
        usort($board['topics'], function(array &$a, array &$b): int {
          return $a['name'] <=> $b['name'];
        });
        $rows = [];
        foreach ($board['topics'] as $topic) {
          $rows[] = [$topic['name'], $topic['reject_init_review'], $topic['passed_init_review']];
        }
        $rows[] = ['Total', count($board['reject_init_review']), count($board['passed_init_review'])];
        $report[] = [
          '#type' => 'table',
          '#caption' => $board['name'],
          '#header' => ['Topic', 'Rejected', 'Accepted'],
          '#rows' => $rows,
          '#attributes' => ['class' => ['initial-review-table']],
        ];
      }
    }
    return $report;
  }

  /**
   * Show counts of articles imported for the specified review cycle(s).
   *
   * @param array $values
   *   Values entered by the user on the request form.
   *
   * @return array
   *   Render array for the report.
   */
  private function importedReport(array $values): array {

    // See whether we have a single cycle or a range.
    if (!empty($values['cycle'])) {
      $cycle = $values['cycle'];
      $cycle_string = Batch::cycleString($cycle);
      $cycle_display = "Review cycle: $cycle_string";
    }
    else {
      $cycle = [$values['cycle-start'], $values['cycle-end']];
      $cycle_start = Batch::cycleString($cycle[0]);
      $cycle_end = Batch::cycleString($cycle[1]);
      $cycle_display = "Review cycle range: $cycle_start through $cycle_end";
    }

    // Initialize the report's render array with some header information.
    $report = [
      'heading-wrapper' => [
        '#type' => 'container',
        '#attributes' => ['class' => ['report-header']],
        'heading' => [
          '#theme' => 'item_list',
          '#title' => 'Articles Imported',
          '#items' => [
            'Report Date: ' . date('Y-m-d'),
            $cycle_display,
            'If an article was imported for more than one topic, the total for the board will be lower than then sum of the topic counts.',
          ],
        ],
      ],
    ];

    // Initialize the array in which we collect the data.
    if (!empty($values['board'])) {
      $board = Board::load($values['board']);
      $boards = [
        $values['board'] => [
          'name' => $board->name->value,
          'topics' => [],
          'articles' => [],
        ],
      ];
    }
    else {
      $boards = [];
      foreach (Board::boards() as $board_id => $board_name) {
        $boards[$board_id] = [
          'name' => $board_name,
          'topics' => [],
          'articles' => [],
        ];
      }
    }

    // Create an entity query for the states we're looking for.
    $storage = \Drupal::entityTypeManager()->getStorage('ebms_state');
    $query = $storage->getQuery()->accessCheck(FALSE);
    $query->condition('value.entity.field_text_id', ['ready_init_review', 'reject_journal_title'], 'IN');
    if (!empty($values['board'])) {
      $query->condition('board', $values['board']);
    }
    $query->addTag('states_for_cycle');
    $query->addMetaData('cycle', $cycle);
    $query->addMetaData('operator', is_array($cycle) ? 'BETWEEN' : '=');

    // Fetch the state entities and collect the data.
    foreach ($storage->loadMultiple($query->execute()) as $state) {
      $board_id = $state->board->target_id;
      $topic_id = $state->topic->target_id;
      $article_id = $state->article->target_id;
      $boards[$board_id]['articles'][$article_id] = $article_id;
      if (!array_key_exists($topic_id, $boards[$board_id]['topics'])) {
        $boards[$board_id]['topics'][$topic_id] = [
          'name' => $state->topic->entity->name->value,
          'count' => 1,
        ];
      }
      else {
        $boards[$board_id]['topics'][$topic_id]['count']++;
      }
    }

    // Create a separate table for each board.
    foreach ($boards as &$board) {
      if (!empty($board['articles'])) {
        usort($board['topics'], function(array &$a, array &$b): int {
          return $a['name'] <=> $b['name'];
        });
        $rows = [];
        foreach ($board['topics'] as $topic) {
          $rows[] = [$topic['name'], $topic['count']];
        }
        $rows[] = ['Total', count($board['articles'])];
        $report[] = [
          '#type' => 'table',
          '#caption' => $board['name'],
          '#header' => ['Topic', 'Imported'],
          '#rows' => $rows,
          '#attributes' => ['class' => ['articles-imported-table']],
        ];
      }
    }
    return $report;
  }

  /**
   * Show information about articles rejected during the review from abstract.
   *
   * This is actually two different reports. If the user specifies a cycle
   * range, then a per-topic statistics report is generated, similar to most
   * of the other reports in this set (except not broken out into separate
   * tables for each board). Otherwise (the user has specified either a single
   * cycle or a decision date range), the report shows one line for each per-
   * article decision.
   *
   * @param array $values
   *   Values entered by the user on the request form.
   *
   * @return array
   *   Render array for the report.
   */
  private function abstractRejectedReport(array $values): array {

    // Start with some defaults.
    $summary_report = FALSE;
    $columns = ['EBMS ID', 'PMID', 'Topic', 'Reviewer', 'Comments'];
    $caption = 'Rejected Articles';
    $heading_details = ['Report Date: ' . date('Y-m-d')];
    $cycle = $dates = '';
    if (!empty($values['board'])) {
      $board = Board::load($values['board']);
      $heading_details[] = 'Board: ' . $board->name->value;
    }

    // See whether we have cycle(s) or decision date.
    if (!empty($values['cycle'])) {
      $cycle = $values['cycle'];
      $cycle_string = Batch::cycleString($cycle);
      $heading_details[] = "Review cycle: $cycle_string";
    }
    elseif ($values['cycle-start']) {
      $cycle = [$values['cycle-start'], $values['cycle-end']];
      $cycle_start = Batch::cycleString($cycle[0]);
      $cycle_end = Batch::cycleString($cycle[1]);
      $heading_details[] = "Review cycle range: $cycle_start through $cycle_end";
      $summary_report = TRUE;
      $caption = 'Rejections By Topic';
      $columns = ['Topic', 'Count'];
    }
    else {
      $start_date = $values['decision-date-start'];
      $end_date = $values['decision-date-end'];
      $heading_details[] = "Decision Date Range: $start_date through $end_date";
      if (strlen($end_date) === 10) {
        $end_date .= ' 23:59:59';
      }
      $dates = [$start_date, $end_date];
    }

    // Initialize the report's render array with some header information.
    $report = [
      'heading-wrapper' => [
        '#type' => 'container',
        '#attributes' => ['class' => ['report-header']],
        'heading' => [
          '#theme' => 'item_list',
          '#title' => 'Articles Rejected During Review From Abstract',
          '#items' => $heading_details,
        ],
      ],
    ];

    // Create an entity query for the states we're looking for.
    $storage = \Drupal::entityTypeManager()->getStorage('ebms_state');
    $query = $storage->getQuery()->accessCheck(FALSE);
    $query->condition('value.entity.field_text_id', 'reject_bm_review');
    if (!empty($values['board'])) {
      $query->condition('board', $values['board']);
    }
    if (!empty($cycle)) {
      $query->addTag('states_for_cycle');
      $query->addMetaData('cycle', $cycle);
      $query->addMetaData('operator', is_array($cycle) ? 'BETWEEN' : '=');
    }
    else {
      $query->condition('entered', $dates, 'BETWEEN');
    }

    // Fetch the state entities and collect the data.
    $rows = [];
    $states = $storage->loadMultiple($query->execute());
    if ($summary_report) {
      $topics = [];
      foreach ($states as $state) {
        $topic = $state->topic->entity->name->value;
        if (!array_key_exists($topic, $topics)) {
          $topics[$topic] = 1;
        }
        else {
          $topics[$topic]++;
        }
      }
      ksort($topics);
      foreach ($topics as $name => $count) {
        $rows[] = [$name, $count];
      }
    }
    else {
      $pubmed_url = ImportRequest::PUBMED_URL;
      $options = ['attributes' => ['target' => '_blank']];
      foreach ($states as $state) {
        $comments = [];
        foreach ($state->comments as $comment) {
          $comment = trim($comment->body);
          if (!empty($comment)) {
            $comments[] = $comment;
          }
        }
        $article_id = $state->article->target_id;
        $pmid = $state->article->entity->source_id->value;
        $rows[] = [
          Link::createFromRoute($article_id, 'ebms_article.article', ['article' => $article_id], $options),
          Link::fromTextAndUrl($pmid, Url::fromUri("$pubmed_url/$pmid", $options)),
          $state->topic->entity->name->value,
          $state->user->entity->name->value,
          implode('; ', $comments),
        ];
      }
    }

    // This report gets only a single table.
    $report['table'] = [
      '#type' => 'table',
      '#caption' => $caption,
      '#header' => $columns,
      '#rows' => $rows,
      '#attributes' => ['class' => ['rejected-from-abstract-table']],
      '#empty' => 'No rejections match the specified filters.',
    ];
    return $report;
  }

  /**
   * Find approvals from abstract review without initial review approval.
   *
   * This is an interesting and useful report, but it doesn't fit very well
   * with the others on this page, which mostly provide statistical summaries,
   * rather than detailed information on individual articles.
   *
   * @param array $values
   *   Values entered by the user on the request form.
   *
   * @return array
   *   Render array for the report.
   */
  private function topicChangesReport(array $values): array {

    // Initialize the report's render array with some header information.
    $start_date = $values['decision-date-start'];
    $end_date = $values['decision-date-end'];
    $dates_display = "Decision Date Range: $start_date through $end_date";
    if (strlen($end_date) === 10) {
      $end_date .= ' 23:59:59';
    }
    $dates = [$start_date, $end_date];
    $report = [
      'heading-wrapper' => [
        '#type' => 'container',
        '#attributes' => ['class' => ['report-header']],
        'heading' => [
          '#theme' => 'item_list',
          '#title' => 'Articles Summary Topic Changes',
          '#items' => [
            'Report Date: ' . date('Y-m-d'),
            $dates_display,
          ],
        ],
      ],
    ];

    // Create an entity query for the articles we're looking for.
    $storage = \Drupal::entityTypeManager()->getStorage('ebms_article');
    $query = $storage->getQuery()->accessCheck(FALSE);
    $query->condition('topics.entity.states.entity.value.entity.field_text_id', 'passed_bm_review');
    $query->condition('topics.entity.states.entity.entered', $dates, 'BETWEEN');
    $query->addTag('skipped_librarian_approval');
    $query->sort('id');

    // Load the articles and extract the data for the report.
    $pubmed_url = ImportRequest::PUBMED_URL;
    $options = ['attributes' => ['target' => '_blank']];
    $rows = [];
    $articles = $storage->loadMultiple($query->execute());
    $storage = \Drupal::entityTypeManager()->getStorage('ebms_state');
    foreach ($articles as $article) {
      $article_id = $article->id();
      $pmid = $article->source_id->value;
      $librarian_topics = [];
      $query = $storage->getQuery()->accessCheck(FALSE);
      $query->condition('article', $article_id);
      $query->condition('value.entity.field_text_id', 'passed_init_review');
      foreach ($storage->loadMultiple($query->execute()) as $state) {
        $librarian_topics[] = $state->topic->entity->name->value;
      }
      sort($librarian_topics);
      $librarian_topics = [
        '#theme' => 'item_list',
        '#items' => $librarian_topics,
        '#attributes' => ['class' => ['usa-list--unstyled']],
      ];
      $librarian_topics = \Drupal::service('renderer')->render($librarian_topics);
      $reviewer_topics = [];
      $query = $storage->getQuery()->accessCheck(FALSE);
      $query->condition('article', $article_id);
      $query->condition('value.entity.field_text_id', 'passed_bm_review');
      foreach ($storage->loadMultiple($query->execute()) as $state) {
        $reviewer_topics[] = $state->topic->entity->name->value;
      }
      sort($reviewer_topics);
      $reviewer_topics = [
        '#theme' => 'item_list',
        '#items' => $reviewer_topics,
        '#attributes' => ['class' => ['usa-list--unstyled']],
      ];
      $reviewer_topics = \Drupal::service('renderer')->render($reviewer_topics);
      $rows[] = [
        Link::createFromRoute($article_id, 'ebms_article.article', ['article' => $article_id], $options),
        Link::fromTextAndUrl($pmid, Url::fromUri("$pubmed_url/$pmid", $options)),
        $librarian_topics,
        $reviewer_topics,
      ];
    }
    // This report gets only a single table.
    $count = count($rows);
    $report['table'] = [
      '#type' => 'table',
      '#caption' => "Article Summary Topic Changes ($count)",
      '#header' => ['EBMS ID', 'PMID', 'After Librarian Review', 'After NCI Review'],
      '#rows' => $rows,
      '#attributes' => ['class' => ['topic-changes-table']],
      '#empty' => 'No topics changed during the specified date span.',
    ];
    return $report;
  }

  /**
   * Show counts of articles automatically rejected by journal title.
   *
   * @param array $values
   *   Values entered by the user on the request form.
   *
   * @return array
   *   Render array for the report.
   */
  private function journalExclusionReport(array $values): array {

    // See whether we have a single cycle or a range.
    if (!empty($values['cycle'])) {
      $cycle = $values['cycle'];
      $cycle_string = Batch::cycleString($cycle);
      $cycle_display = "Review cycle: $cycle_string";
    }
    else {
      $cycle = [$values['cycle-start'], $values['cycle-end']];
      $cycle_start = Batch::cycleString($cycle[0]);
      $cycle_end = Batch::cycleString($cycle[1]);
      $cycle_display = "Review cycle range: $cycle_start through $cycle_end";
    }

    // Initialize the report's render array with some header information.
    $report = [
      'heading-wrapper' => [
        '#type' => 'container',
        '#attributes' => ['class' => ['report-header']],
        'heading' => [
          '#theme' => 'item_list',
          '#title' => 'Articles Rejected By Journal Title',
          '#items' => [
            'Report Date: ' . date('Y-m-d'),
            $cycle_display,
            'If default journal exclusion was applied for more than one topic, the total for the board will be lower than then sum of the topic counts.',
          ],
        ],
      ],
    ];

    // Initialize the array in which we collect the data.
    $all_articles = [];
    if (!empty($values['board'])) {
      $board = Board::load($values['board']);
      $boards = [
        $values['board'] => [
          'name' => $board->name->value,
          'topics' => [],
          'articles' => [],
        ],
      ];
    }
    else {
      $boards = [];
      foreach (Board::boards() as $board_id => $board_name) {
        $boards[$board_id] = [
          'name' => $board_name,
          'topics' => [],
          'articles' => [],
        ];
      }
    }

    // Create an entity query for the states we're looking for.
    $storage = \Drupal::entityTypeManager()->getStorage('ebms_state');
    $query = $storage->getQuery()->accessCheck(FALSE);
    $query->condition('value.entity.field_text_id', 'reject_journal_title');
    if (!empty($values['board'])) {
      $query->condition('board', $values['board']);
    }
    $query->addTag('states_for_cycle');
    $query->addMetaData('cycle', $cycle);
    $query->addMetaData('operator', is_array($cycle) ? 'BETWEEN' : '=');

    // Fetch the state entities and collect the data.
    foreach ($storage->loadMultiple($query->execute()) as $state) {
      $board_id = $state->board->target_id;
      $topic_id = $state->topic->target_id;
      $article_id = $state->article->target_id;
      $boards[$board_id]['articles'][$article_id] = $article_id;
      $all_articles[$article_id] = $article_id;
      if (!array_key_exists($topic_id, $boards[$board_id]['topics'])) {
        $boards[$board_id]['topics'][$topic_id] = [
          'name' => $state->topic->entity->name->value,
          'count' => 1,
        ];
      }
      else {
        $boards[$board_id]['topics'][$topic_id]['count']++;
      }
    }

    // Create a separate table for each board.
    foreach ($boards as &$board) {
      if (!empty($board['articles'])) {
        usort($board['topics'], function(array &$a, array &$b): int {
          return $a['name'] <=> $b['name'];
        });
        $rows = [];
        foreach ($board['topics'] as $topic) {
          $rows[] = [$topic['name'], $topic['count']];
        }
        $rows[] = ['Total', count($board['articles'])];
        $report[] = [
          '#type' => 'table',
          '#caption' => $board['name'],
          '#header' => ['Topic', 'Rejected'],
          '#rows' => $rows,
          '#attributes' => ['class' => ['journal-rejections-table']],
        ];
      }
    }

    // Add a table for the grand total if a single board was not specified.
    if (empty($values['board'])) {
      $report[] = [
        '#type' => 'table',
        '#caption' => 'Grand Total',
        '#rows' => [['All Boards', count($all_articles)]],
        '#attributes' => ['class' => ['journal-rejections-table']],
      ];
    }
    return $report;
  }

  /**
   * Show counts of articles reviewed by a librarian.
   *
   * @param array $values
   *   Values entered by the user on the request form.
   *
   * @return array
   *   Render array for the report.
   */
  private function librarianReviewReport(array $values): array {

    // See whether we have a single cycle or a range.
    if (!empty($values['cycle'])) {
      $cycle = $values['cycle'];
      $cycle_string = Batch::cycleString($cycle);
      $cycle_display = "Review cycle: $cycle_string";
    }
    else {
      $cycle = [$values['cycle-start'], $values['cycle-end']];
      $cycle_start = Batch::cycleString($cycle[0]);
      $cycle_end = Batch::cycleString($cycle[1]);
      $cycle_display = "Review cycle range: $cycle_start through $cycle_end";
    }

    // Initialize the report's render array with some header information.
    $report = [
      'heading-wrapper' => [
        '#type' => 'container',
        '#attributes' => ['class' => ['report-header']],
        'heading' => [
          '#theme' => 'item_list',
          '#title' => 'Articles Reviewed By Medical Librarian',
          '#items' => [
            'Report Date: ' . date('Y-m-d'),
            $cycle_display,
            'If an article was reviewed for more than one topic, the total for the board will be lower than then sum of the topic counts.',
          ],
        ],
      ],
    ];

    // Initialize the array in which we collect the data.
    $all_articles = [];
    if (!empty($values['board'])) {
      $board = Board::load($values['board']);
      $boards = [
        $values['board'] => [
          'name' => $board->name->value,
          'topics' => [],
          'articles' => [],
        ],
      ];
    }
    else {
      $boards = [];
      foreach (Board::boards() as $board_id => $board_name) {
        $boards[$board_id] = [
          'name' => $board_name,
          'topics' => [],
          'articles' => [],
        ];
      }
    }

    // Create an entity query for the states we're looking for.
    $storage = \Drupal::entityTypeManager()->getStorage('ebms_state');
    $query = $storage->getQuery()->accessCheck(FALSE);
    $query->condition('value.entity.field_text_id', ['passed_init_review', 'reject_init_review'], 'IN');
    if (!empty($values['board'])) {
      $query->condition('board', $values['board']);
    }
    $query->addTag('states_for_cycle');
    $query->addMetaData('cycle', $cycle);
    $query->addMetaData('operator', is_array($cycle) ? 'BETWEEN' : '=');

    // Fetch the state entities and collect the data.
    foreach ($storage->loadMultiple($query->execute()) as $state) {
      $board_id = $state->board->target_id;
      $topic_id = $state->topic->target_id;
      $article_id = $state->article->target_id;
      $boards[$board_id]['articles'][$article_id] = $article_id;
      $all_articles[$article_id] = $article_id;
      if (!array_key_exists($topic_id, $boards[$board_id]['topics'])) {
        $boards[$board_id]['topics'][$topic_id] = [
          'name' => $state->topic->entity->name->value,
          'count' => 1,
        ];
      }
      else {
        $boards[$board_id]['topics'][$topic_id]['count']++;
      }
    }

    // Create a separate table for each board.
    foreach ($boards as &$board) {
      if (!empty($board['articles'])) {
        usort($board['topics'], function(array &$a, array &$b): int {
          return $a['name'] <=> $b['name'];
        });
        $rows = [];
        foreach ($board['topics'] as $topic) {
          $rows[] = [$topic['name'], $topic['count']];
        }
        $rows[] = ['Total', count($board['articles'])];
        $report[] = [
          '#type' => 'table',
          '#caption' => $board['name'],
          '#header' => ['Topic', 'Reviewed'],
          '#rows' => $rows,
          '#attributes' => ['class' => ['librarian-review-table']],
        ];
      }
    }

    // Add a table for the grand total if a single board was not specified.
    if (empty($values['board'])) {
      $report[] = [
        '#type' => 'table',
        '#caption' => 'Grand Total',
        '#rows' => [['All Boards', count($all_articles)]],
        '#attributes' => ['class' => ['librarian-review-table']],
      ];
    }
    return $report;
  }

  /**
   * Show counts of articles approved during review from the abstract.
   *
   * @param array $values
   *   Values entered by the user on the request form.
   *
   * @return array
   *   Render array for the report.
   */
  private function abstractApprovedReport(array $values): array {

    // See whether we have a single cycle or a range.
    if (!empty($values['cycle'])) {
      $cycle = $values['cycle'];
      $cycle_string = Batch::cycleString($cycle);
      $cycle_display = "Review cycle: $cycle_string";
    }
    else {
      $cycle = [$values['cycle-start'], $values['cycle-end']];
      $cycle_start = Batch::cycleString($cycle[0]);
      $cycle_end = Batch::cycleString($cycle[1]);
      $cycle_display = "Review cycle range: $cycle_start through $cycle_end";
    }

    // Initialize the report's render array with some header information.
    $report = [
      'heading-wrapper' => [
        '#type' => 'container',
        '#attributes' => ['class' => ['report-header']],
        'heading' => [
          '#theme' => 'item_list',
          '#title' => 'Articles Approved During Review From Abstract',
          '#items' => [
            'Report Date: ' . date('Y-m-d'),
            $cycle_display,
            'If an article was approved for more than one topic, the total for the board will be lower than then sum of the topic counts.',
          ],
        ],
      ],
    ];

    // Initialize the array in which we collect the data.
    $all_articles = [];
    if (!empty($values['board'])) {
      $board = Board::load($values['board']);
      $boards = [
        $values['board'] => [
          'name' => $board->name->value,
          'topics' => [],
          'articles' => [],
        ],
      ];
    }
    else {
      $boards = [];
      foreach (Board::boards() as $board_id => $board_name) {
        $boards[$board_id] = [
          'name' => $board_name,
          'topics' => [],
          'articles' => [],
        ];
      }
    }

    // Create an entity query for the states we're looking for.
    $storage = \Drupal::entityTypeManager()->getStorage('ebms_state');
    $query = $storage->getQuery()->accessCheck(FALSE);
    $query->condition('value.entity.field_text_id', 'passed_bm_review');
    if (!empty($values['board'])) {
      $query->condition('board', $values['board']);
    }
    $query->addTag('states_for_cycle');
    $query->addMetaData('cycle', $cycle);
    $query->addMetaData('operator', is_array($cycle) ? 'BETWEEN' : '=');

    // Fetch the state entities and collect the data.
    foreach ($storage->loadMultiple($query->execute()) as $state) {
      $board_id = $state->board->target_id;
      $topic_id = $state->topic->target_id;
      $article_id = $state->article->target_id;
      $boards[$board_id]['articles'][$article_id] = $article_id;
      $all_articles[$article_id] = $article_id;
      if (!array_key_exists($topic_id, $boards[$board_id]['topics'])) {
        $boards[$board_id]['topics'][$topic_id] = [
          'name' => $state->topic->entity->name->value,
          'count' => 1,
        ];
      }
      else {
        $boards[$board_id]['topics'][$topic_id]['count']++;
      }
    }

    // Create a separate table for each board.
    foreach ($boards as &$board) {
      if (!empty($board['articles'])) {
        usort($board['topics'], function(array &$a, array &$b): int {
          return $a['name'] <=> $b['name'];
        });
        $rows = [];
        foreach ($board['topics'] as $topic) {
          $rows[] = [$topic['name'], $topic['count']];
        }
        $rows[] = ['Total', count($board['articles'])];
        $report[] = [
          '#type' => 'table',
          '#caption' => $board['name'],
          '#header' => ['Topic', 'Approved'],
          '#rows' => $rows,
          '#attributes' => ['class' => ['abstract-approved-table']],
        ];
      }
    }

    // Add a table for the grand total if a single board was not specified.
    if (empty($values['board'])) {
      $report[] = [
        '#type' => 'table',
        '#caption' => 'Grand Total',
        '#rows' => [['All Boards', count($all_articles)]],
        '#attributes' => ['class' => ['abstract-approved-table']],
      ];
    }
    return $report;
  }

}
