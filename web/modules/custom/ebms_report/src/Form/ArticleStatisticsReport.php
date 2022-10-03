<?php

namespace Drupal\ebms_report\Form;

require '../vendor/autoload.php';

use Drupal\Core\Database\Query\SelectInterface;
use Drupal\Core\Form\FormBase;
use Drupal\Core\Form\FormStateInterface;
use Drupal\ebms_board\Entity\Board;
use Drupal\ebms_core\Entity\SavedRequest;
use Drupal\ebms_review\Entity\Review;
use Drupal\ebms_topic\Entity\Topic;
use PhpOffice\PhpSpreadsheet\Spreadsheet;
use PhpOffice\PhpSpreadsheet\Writer\Xlsx;
use Symfony\Component\HttpFoundation\Response;

/**
 * Comprehensive Excel workbook for a statistical report on article activity.
 *
 * @ingroup ebms
 */
class ArticleStatisticsReport extends FormBase {

  /**
   * Columns used for states usually preceding full-text retrieval.
   */
  const EARLY_STATES = [
    'reject_journal_title' => '"Not" Listed',
    'passed_init_review' => 'Librarian Approved',
    'reject_init_review' => 'Librarian Rejection',
    'published' => 'Published',
    'passed_bm_review' => 'Abstract Approval',
    'reject_bm_review' => 'Abstract Rejection',
  ];

  /**
   * Columns for states immediately following full-text retrieval.
   */
  const FULL_TEXT_STATES = [
    'passed_full_review' => 'Full Text Approved',
    'reject_full_review' => 'Full Text Rejection',
    'full_review_hold' => 'On Hold With Full Text',
  ];

  /**
   * Styles to apply to the top rows of each sheet.
   */
  const STYLE = [
    'font' => ['bold' => TRUE],
    'alignment' => ['horizontal' => 'center'],
  ];

  /**
   * State term IDs indexed by machine string ID.
   */
  private array $state_ids = [];

  /**
   * Final board decisions.
   */
  private array $decisions = [];

  /**
   * Beginning of date range for the report.
   */
  private string $start = '';

  /**
   * End of date range for the report.
   */
  private string $end = '';

  /**
   * Reviewer decision that no changes are warranted for an article.
   */
  private int $no_changes_warranted;

  /**
   * Column headers.
   */
  private array $cols;

  /**
   * Letter designation for the final column.
   */
  private string $last_col;

  /**
   * Title to be displayed at the top of each sheet.
   */
  private string $sheet_title;

  /**
   * Board-level counts.
   */
  private array $board_counts;

  /**
   * {@inheritdoc}
   */
  public function getFormId(): string {
    return 'article_statistics_report_form';
  }

  /**
   * {@inheritdoc}
   */
  public function buildForm(array $form, FormStateInterface $form_state, int $report_id = 0): array|Response {

    // Create the report if requested.
    $params = empty($report_id) ? [] : SavedRequest::loadParameters($report_id);
    if (!empty($params)) {
      SavedRequest::saveParameters('statistics report', $params);
      return $this->report($params);
    }

    $form = [
      '#title' => 'Article Statistics Report',
      '#cache' => ['max-age' => 0],
      'filters' => [
        '#type' => 'details',
        '#title' => 'Filters',
        'dates' => [
          '#type' => 'container',
          '#attributes' => ['class' => ['inline-fields']],
          '#title' => 'Report Dates',
          '#description' => 'Optionally specify the date range which the report should cover.',
          'date-start' => [
            '#type' => 'date',
            '#default_value' => $params['date-start'] ?? date('Y-m-d', strtotime('-1 year +1 day')),
          ],
          'date-end' => [
            '#type' => 'date',
            '#default_value' => $params['date-end'] ?? date('Y-m-d'),
          ],
        ],
      ],
      'submit' => [
        '#type' => 'submit',
        '#value' => 'Submit',
      ],
    ];
    return $form;
  }

  /**
   * {@inheritdoc}
   */
  public function submitForm(array &$form, FormStateInterface $form_state) {
    $params = $form_state->getValues();
    $request = SavedRequest::saveParameters('statistics report', $params);
    $form_state->setRedirect('ebms_report.statistics', ['report_id' => $request->id()]);
  }

  /**
   * {@inheritdoc}
   */
  public function validateForm(array &$form, FormStateInterface $form_state) {
    foreach (['start', 'end'] as $field) {
      $name = "date-$field";
      $value = $form_state->getValue($name);
      if (!empty($value) && !preg_match('/\d\d\d\d-\d\d-\d\d/', $value)) {
        $form_state->setErrorByName($name, 'Invalid date');
      }
    }
  }

  /**
   * Create the report workbook and wrap it in a `Response` object.
   *
   * @param array $params
   *   Options selected by the user for the report.
   *
   * @return Response
   *   Non-HTML response for returning the binary payload to the client.
   */
  private function report(array $params): Response {

    // Set up the object's properties used for generating the report.
    $this->sheet_title = 'EBMS Statistics (Complete History)';
    $this->start = $start = $params['date-start'] ?? '';
    $end = $params['date-end'] ?? '';
    if (!empty($end)) {
      $this->end = "$end 23:59:59";
      if (!empty($start)) {
        $this->sheet_title = "EBMS Statistics ($start - $end)";
      }
      else {
        $this->sheet_title = "EBMS Statistics (through $end)";
      }
    }
    elseif (!empty($start)) {
      $this->sheet_title = "EBMS Statistics (since $start)";
    }
    $storage = \Drupal::entityTypeManager()->getStorage('taxonomy_term');
    $query = $storage->getQuery()->accessCheck(FALSE);
    $query->condition('vid', 'board_decisions');
    $query->sort('weight');
    $query->sort('tid');
    foreach ($storage->loadMultiple($query->execute()) as $term) {
      $this->decisions[$term->id()] = $term->name->value;
    }
    $query = $storage->getQuery()->accessCheck(FALSE);
    $query->condition('vid', 'states');
    foreach ($storage->loadMultiple($query->execute()) as $term) {
      $this->state_ids[$term->field_text_id->value] = $term->id();
    }
    $this->no_changes_warranted = Review::getRejectionDisposition();

    // Create a new workbook and clear out any default worksheets.
    $book = new Spreadsheet();
    while ($book->getSheetCount() > 0) {
      $book->removeSheetByIndex(0);
    }

    // Get the column headers.
    $this->cols = ['Board', 'Imported'];
    foreach (self::EARLY_STATES as $label) {
      $this->cols[] = $label;
    }
    $this->cols[] = 'Full Text Retrieved';
    foreach (self::FULL_TEXT_STATES as $label) {
      $this->cols[] = $label;
    }
    $this->cols[] = 'Assigned For Review';
    $this->cols[] = 'Positive Responses';
    $this->cols[] = 'No Changes Warranted';
    $this->cols[] = 'No Reviews Received';
    foreach($this->decisions as $label) {
      $this->cols[] = $label;
    }
    $this->last_col = 'A';
    for ($i = count($this->cols) - 1; $i > 0; $i--)
        $this->last_col++;

    // Create the overview sheet.
    $this->addSheet($book, 'Boards');

    // Create a worksheet for each board.
    $board_names = Board::boards();
    $this->cols[0] = 'Topics';
    foreach ($board_names as $board_id => $board_name) {
      $this->addSheet($book, $board_name, $board_id);
    }

    // Wrap things up and send the workbook to the client.
    $book->setActiveSheetIndex(0);
    $writer = new Xlsx($book);
    ob_start();
    $writer->save('php://output');
    $stamp = date('YmdHis');
    return new Response(
      ob_get_clean(),
      200,
      [
        'Content-type' => 'application/vnd.openxmlformats-officedocument.spreadsheetml.sheet',
        'Content-disposition' => 'attachment; filename="ebms-statistics-' . $stamp . '.xlsx"',
        'Cache-control' => 'must-revalidate, post-check=0, pre-check=0',
        'Content-transfer-encoding' => 'binary',
      ],
    );
  }

  /**
   * Create one of the report's spreadsheets.
   *
   * @param Spreadsheet $book
   *   The Excel workbook object to which we're adding a sheet.
   * @param string $name
   *   The name of the sheet.
   * @param int $board
   *   ID of the board for which the sheet is created.
   */
  private function addSheet($book, $name, $board = 0) {

    // Create the sheet and set up the top rows.
    $sheet = $book->createSheet();
    $sheet->setTitle($this->adjustLongName($name));
    if (empty($board)) {
      $sheet->freezePane('B1');
      $row = 3;
    }
    else {
      $sheet->freezePane('B6');
      $row = 4;
    }
    $sheet->mergeCells("A1:{$this->last_col}1");
    $sheet->mergeCells("A2:{$this->last_col}2");
    $sheet->mergeCells("A$row:M$row");
    $sheet->mergeCells("N$row:P$row");
    $sheet->mergeCells("Q$row:{$this->last_col}$row");
    $sheet->setCellValue("N$row", 'Board Member Responses');
    $sheet->setCellValue("Q$row", 'Editorial Board Decisions');
    $row++;
    $sheet->getStyle("A1:{$this->last_col}$row")->applyFromArray(self::STYLE);
    $sheet->setCellValue('A1', $this->sheet_title);
    $sheet->fromArray($this->cols, NULL, "A$row");
    foreach (range('A', $this->last_col) as $col) {
      $sheet->getColumnDimension($col)->setAutoSize(TRUE);
    }

    // Populate the overview sheet.
    if (empty($board)) {
      $sheet->setCellValue('A5', 'All Boards');
      $values = $this->rowCounts(0, 0);
      $sheet->fromArray($values, NULL, 'B5', TRUE);
      $row = 7;
      foreach (Board::boards() as $board_id => $board_name) {
        if (strpos(strtolower($board_name), 'complementary') !== FALSE) {
          $board_name = 'IACT';
        }
        $values = $this->rowCounts($board_id, 0);
        $sheet->setCellValue("A$row", $this->adjustLongName($board_name));
        $this->board_counts[$board_id] = $values;
        $sheet->fromArray($values, NULL, "B$row", TRUE);
        $row++;
      }
    }

    // Handle a sheet for one of the boards.
    else {
      $sheet->setCellValue('A6', 'All Topics');
      $sheet->fromArray($this->board_counts[$board], NULL, 'B6', TRUE);
      $row = 8;
      foreach (Topic::topics($board) as $topic_id => $topic_name) {
        $sheet->setCellValue("A$row", $topic_name);
        $values = $this->rowCounts(0, $topic_id);
        $sheet->fromArray($values, NULL, "B$row", TRUE);
        $row++;
      }
    }
  }

  /**
   * Get the counts for a single row of the report.
   *
   * @param int $board
   *   Optional ID for narrowing the counts to a specific board.
   * @param int $topic
   *   Optional ID for narrowing the counts to a specific topic.
   *
   * @return array
   *   Array of positive integer values to be inserted in the spreadsheet.
   */
  private function rowCounts(int $board, int $topic): array {

    // Find the count of articles "imported" during the date range.
    $counts = [$this->firstSeen($board, $topic)];

    // Get the counts for the early states.
    foreach (array_keys(self::EARLY_STATES) as $text_id) {
      $counts[] = $this->stateCount($text_id, $board, $topic);
    }

    // For how many articles do we have full-text PDFs?
    $counts[] = $this->fullTextCount($board, $topic);

    // Get the counts for the decisions made from that full text.
    foreach (array_keys(self::FULL_TEXT_STATES) as $text_id) {
      $counts[] = $this->stateCount($text_id, $board, $topic);
    }

    // How many articles were assigned out for review?
    $counts[] = $this->assignedCount($board, $topic);

    // Find out what the board members have come up with in their reviews.
    $counts[] = $this->reviewCount('<>', $board, $topic);
    $counts[] = $this->reviewCount('=', $board, $topic);
    $counts[] = $this->reviewCount('', $board, $topic);

    // Finally, get the counts for the final board decisions.
    foreach (array_keys($this->decisions) as $decision) {
      $counts[] = $this->decisionCount($decision, $board, $topic);
    }

    // We've got what we came for.
    return $counts;
  }

   /**
   * Find out how many articles were "imported" for this scope.
   *
   * "Imported" is a word which has unfortunate overloaded
   * meanings for this system. What it *really* means in this
   * context is that the article was first assigned a state
   * for a particular topic, whether or not the article had
   * already been imported previously from NLM for another topic,
   * and even if for this assigment of the review topic we didn't
   * get anything at all from NLM.
   *
   * @param int $board
   *   Optional ID for narrowing the count to a specific board.
   * @param int $topic
   *   Optional ID for narrowing the count to a specific topic.
   *
   * @return int
   *   Value to be placed in a cell in the spreadsheet.
   */
  private function firstSeen(int $board, int $topic): int {

    // Handle the unusual, but very easy case.
    if (empty($this->start) && empty($this->end)) {
      $query = \Drupal::database()->select('ebms_state', 'state');
      if (!empty($topic)) {
        $query->condition('state.topic', $topic);
      }
      elseif (!empty($board)) {
        $query->condition('state.board', $board);
      }
      $query->addExpression('COUNT(DISTINCT state.article)');
      return $query->execute()->fetchField();
    }

    // Create a subquery to get the first state for each article.
    $subquery = \Drupal::database()->select('ebms_state', 'first');
    $subquery->addField('first', 'article');
    $subquery->addExpression('MIN(first.id)', 'id');
    $subquery->groupBy('first.article');
    if (!empty($topic)) {
      $subquery->condition('first.topic', $topic);
    }
    elseif (!empty($board)) {
      $subquery->condition('first.board', $board);
    }

    // Now the query to find out how many of those state are in the range.
    $query = \Drupal::database()->select('ebms_state', 'state');
    $query->join($subquery, 'first_state', 'first_state.id = state.id');
    $query->addField('state', 'article');
    $query->distinct();
    if (!empty($this->start)) {
      $query->condition('state.entered', $this->start, '>=');
    }
    if (!empty($this->end)) {
      $query->condition('state.entered', $this->end, '<=');
    }
    return $query->countQuery()->execute()->fetchField();
  }

  /**
   * Find out how many articles reached the specified state.
   *
   * @param string $text_id
   *   Machine ID for the state whose count we are calculatiing.
   * @param int $board
   *   Optional ID for narrowing the count to a specific board.
   * @param int $topic
   *   Optional ID for narrowing the count to a specific topic.
   *
   * @return int
   *   Value to be placed in a cell in the spreadsheet.
   */
  private function stateCount(string $text_id, int $board, int $topic): int {
    $query = \Drupal::database()->select('ebms_state', 'state');
    $query->addField('state', 'article');
    $query->distinct();
    $query->condition('state.value', $this->state_ids[$text_id]);
    $this->addConditions($query, $board, $topic);
    return $query->countQuery()->execute()->fetchField();
  }

  /**
   * Find out how many articles we got the full text for.
   *
   * @param int $board
   *   Optional ID for narrowing the count to a specific board.
   * @param int $topic
   *   Optional ID for narrowing the count to a specific topic.
   *
   * @return int
   *   Value to be placed in a cell in the spreadsheet.
   */
  private function fullTextCount(int $board, int $topic) {
    $query = \Drupal::database()->select('ebms_article', 'article');
    $query->addField('article', 'id');
    $query->distinct();
    $query->join('file_managed', 'file', 'file.fid = article.full_text__file');
    if (!empty($topic) || !empty($board)) {
      $query->join('ebms_state', 'state', 'state.article = article.id');
      if (!empty($topic)) {
        $query->condition('state.topic', $topic);
      }
      elseif (!empty($board)) {
        $query->condition('state.board', $board);
      }
    }

    // Start and end date values have been scrubbed in the validation method.
    if (!empty($this->start)) {
      if (!empty($this->end)) {
        // Most common case.
        $query->where("FROM_UNIXTIME(file.created) BETWEEN '{$this->start}' AND '{$this->end}'");
      }
      else {
        $query->where("FROM_UNIXTIME(file.created) >= '{$this->start}'");
      }
    } elseif (!empty($this->end)) {
      $query->where("FROM_UNIXTIME(file.created) <= '{$this->end}'");
    }
    return $query->countQuery()->execute()->fetchField();
  }

  /**
   * Get a count for articles assigned for review.
   *
   * @param int $board
   *   Optional ID for narrowing the count to a specific board.
   * @param int $topic
   *   Optional ID for narrowing the count to a specific topic.
   *
   * @return int
   *   Value to be placed in a cell in the spreadsheet.
   */
  private function assignedCount(int $board, int $topic) {
    $query = \Drupal::database()->select('ebms_packet', 'packet');
    $query->join('ebms_packet__articles', 'articles', 'articles.entity_id = packet.id');
    $query->join('ebms_packet_article', 'packet_article', 'packet_article.id = articles.articles_target_id');
    $query->addField('packet_article', 'article');
    $query->distinct();
    if (!empty($topic)) {
      $query->condition('packet.topic', $topic);
    }
    elseif (!empty($board)) {
      $query->join('ebms_topic', 'topic', 'topic.id = packet.topic');
      $query->condition('topic.board', $board);
    }
    if (!empty($this->start)) {
      $query->condition('packet.created', $this->start, '>=');
    }
    if (!empty($this->end)) {
      $query->condition('packet.created', $this->end, '<=');
    }
    return $query->countQuery()->execute()->fetchField();
  }

  /**
   * Get a count for board member reviews.
   *
   * @param string $operator
   *   Comparison with the "no changes warranted" decision ('=' for rejected
   *   articles, '<>' for other decisions, and an empty string to get the
   *   count of articles which didn't get any reviews).
   * @param int $board
   *   Optional ID for narrowing the count to a specific board.
   * @param int $topic
   *   Optional ID for narrowing the count to a specific topic.
   *
   * @return int
   *   Value to be placed in a cell in the spreadsheet.
   */
  private function reviewCount(string $operator, int $board, int $topic) {

    // Get started with the part of the query common to all three counts.
    $query = \Drupal::database()->select('ebms_packet', 'packet');
    $query->join('ebms_packet__articles', 'articles', 'articles.entity_id = packet.id');
    $query->join('ebms_packet_article', 'packet_article', 'packet_article.id = articles.articles_target_id');
    $query->addField('packet_article', 'article');
    $query->distinct();
    if (!empty($topic)) {
      $query->condition('packet.topic', $topic);
    }
    elseif (!empty($board)) {
      $query->join('ebms_topic', 'topic', 'topic.id = packet.topic');
      $query->condition('topic.board', $board);
    }

    // Handle the request for the count of unreviewed articles.
    if (empty($operator)) {
      $query->leftJoin('ebms_packet_article__reviews', 'reviews', 'reviews.entity_id = packet_article.id');
      $query->isNull('reviews.reviews_target_id');
      if (!empty($this->start)) {
        $query->condition('packet.created', $this->start, '>=');
      }
      if (!empty($this->end)) {
        $query->condition('packet.created', $this->end, '<=');
      }
    }

    // Otherwise, apply the specified operator to the dispositions.
    else {
      $query->join('ebms_packet_article__reviews', 'reviews', 'reviews.entity_id = packet_article.id');
      $query->join('ebms_review__dispositions', 'dispositions', 'dispositions.entity_id = reviews.reviews_target_id');
      $query->condition('dispositions.dispositions_target_id', $this->no_changes_warranted, $operator);
      if (!empty($this->start) || !empty($this->end)) {
        $query->join('ebms_review', 'review', 'review.id = reviews.reviews_target_id');
        if (!empty($this->start)) {
          $query->condition('review.posted', $this->start, '>=');
        }
        if (!empty($this->end)) {
          $query->condition('review.posted', $this->end, '<=');
        }
      }
    }

    // Return the count.
    return $query->countQuery()->execute()->fetchField();
  }

  /**
   * Get a count for articles given a specific final board decision.
   *
   * @param int $decision
   *   ID for the decision we're counting articles for.
   * @param int $board
   *   Optional ID for narrowing the count to a specific board.
   * @param int $topic
   *   Optional ID for narrowing the count to a specific topic.
   *
   * @return int
   *   Value to be placed in a cell in the spreadsheet.
   */
  private function decisionCount(int $decision, int $board, int $topic) {
    $query = \Drupal::database()->select('ebms_state', 'state');
    $query->join('ebms_state__decisions', 'decisions', 'decisions.entity_id = state.id');
    $query->condition('decisions.decisions_decision', $decision);
    $query->addField('state', 'article');
    $query->distinct();
    $this->addConditions($query, $board, $topic);
    return $query->countQuery()->execute()->fetchField();
  }

 /**
   * Add filtering by board, topic, and/or dates.
   *
   * @param SelectInterface $query
   *   Query we are modifying.
   * @param int $board
   *   Optional ID for narrowing the count to a specific board.
   * @param int $topic
   *   Optional ID for narrowing the count to a specific topic.
   */
  private function addConditions(SelectInterface $query, int $board, int $topic) {
    if (!empty($topic)) {
      $query->condition('state.topic', $topic);
    }
    elseif (!empty($board)) {
      $query->condition('state.board', $board);
    }
    if (!empty($this->start)) {
      $query->condition('state.entered', $this->start, '>=');
    }
    if (!empty($this->end)) {
      $query->condition('state.entered', $this->end, '<=');
    }
  }

  /**
   * Replace long board names with initialisms.
   *
   * @param string $name
   *   Original board name string.
   *
   * @return string
   *   Possibly shortened version of board name.
   */
  private function adjustLongName($name): string {
    if (strlen($name) < 50) {
      return $name;
    }
    if (preg_match_all('/[A-Z]/', $name, $matches)) {
      return implode('', $matches[0]);
    }
    return $name;
  }

}
