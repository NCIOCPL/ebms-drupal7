<?php

namespace Drupal\ebms_report\Controller;

require '../vendor/autoload.php';

use PhpOffice\PhpSpreadsheet\Spreadsheet;
use PhpOffice\PhpSpreadsheet\Writer\Xlsx;
use Drupal\Core\Controller\ControllerBase;
use Drupal\ebms_board\Entity\Board;
use Symfony\Component\HttpFoundation\Response;

/**
 * Report on the last time each board member logged in.
 */
class BoardMemberLastLogins extends ControllerBase {

  /**
   * Create an Excel workbook for the report (no form needed).
   */
  public function report() {

    // Examine the active packets, counting article waiting for review.
    $full_text_approval = \Drupal::service('ebms_core.term_lookup')->getState('passed_full_review');
    $sequence_threshold = $full_text_approval->field_sequence->value;
    $unreviewed_counts = [];
    $storage = $this->entityTypeManager()->getStorage('ebms_packet');
    $query = $storage->getQuery()->accessCheck(FALSE);
    $query->condition('active', 1);
    foreach ($storage->loadMultiple($query->execute()) as $packet) {

      // Who's assigned to the packet?
      $reviewers = [];
      foreach ($packet->reviewers as $reviewer) {
        $reviewers[] = $reviewer->target_id;
      }
      foreach ($packet->articles as $packet_article) {

        // If the article has been dropped, it's not waiting for reviews.
        if (!empty($packet_article->entity->dropped->value)) {
          continue;
        }

        // We don't expect reviews for FYI articles.
        $current_state = $packet_article->entity->article->entity->getCurrentState($packet->topic->target_id);
        if ($current_state->value->entity->field_text_id->value  === 'fyi') {
          continue;
        }

        // If the article has already moved on for this topic, skip past it.
        if ($current_state->value->entity->field_sequence->value > $sequence_threshold) {
          continue;
        }

        // Who has already reviewed the article?
        $reviewed_by = [];
        foreach ($packet_article->reviews->entity->reviewers as $reviewer) {
          $reviewed_by[] = $reviewer->target_id;
        }

        // See who was assigned but hasn't yet reviewed the article.
        foreach (array_diff($reviewers, $reviewed_by) as $reviewer_id) {
          if (!array_key_exists($reviewer_id, $unreviewed_counts)) {
            $unreviewed_counts[$reviewer_id] = 1;
          }
          else {
            $unreviewed_counts[$reviewer_id]++;
          }
        }
      }
    }

    // Find all the active board member users.
    $storage = $this->entityTypeManager()->getStorage('user');
    $query = $storage->getQuery()->accessCheck(FALSE);
    $query->condition('status', 1);
    $query->condition('roles', 'board_member');
    $query->sort('name');
    $board_members = [];
    $boards = [];
    $no_board = [];

    // Collect the information needed for each board member.
    foreach ($storage->loadMultiple($query->execute()) as $user) {
      $uid = $user->id();
      $board_members[$uid] = [
        'name' => $user->name->value,
        'login' => empty($user->login->value) ? '' : date('Y-m-d', $user->login->value),
        'unreviewed' => $unreviewed_counts[$uid] ?? 0,
      ];

      // Add the user ID to each board's array of members.
      foreach ($user->boards as $board) {
        if (!array_key_exists($board->target_id, $boards)) {
          $boards[$board->target_id] = [$uid];
        }
        else {
          $boards[$board->target_id][] = $uid;
        }
      }

      // We'll create a separate worksheet for board members with no board.
      if (empty($user->boards->count())) {
        $no_board[] = $uid;
      }
    }

    // Create a new workbook and clear out any default worksheets.
    $book = new Spreadsheet();
    while ($book->getSheetCount() > 0) {
      $book->removeSheetByIndex(0);
    }

    // Create a worksheet for each board.
    $board_names = Board::boards();
    foreach ($board_names as $board_id => $board_name) {
      if ($board_name === 'Integrative, Alternative, and Complementary Therapies') {
        $board_name = 'IACT';
      }
      $this->addSheet($book, $board_name, $boards[$board_id], $board_members);
    }

    // Add a final worksheet for those not yet assigned a board.
    $this->addSheet($book, 'No Boards', $no_board, $board_members);

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
        'Content-disposition' => 'attachment; filename="board-members-last-login-' . $stamp . '.xlsx"',
      ],
    );
  }

  /**
   * Add a sheet to the workbook and populate it.
   */
  private function addSheet($book, $board_name, &$user_ids, &$board_members) {

    // Create the sheet and configure its settings.
    $sheet = $book->createSheet();
    $sheet->setTitle($board_name);
    $sheet->setCellValue('A1', 'Name');
    $sheet->setCellValue('B1', 'AuthName');
    $sheet->setCellValue('C1', 'Last Login');
    $sheet->setCellValue('D1', 'Outstanding Reviews');
    $sheet->getStyle('A1:D1')->applyFromArray([
      'fill' => [
        'fillType' => 'solid',
        'color' => ['argb' => 'FF0101DF'],
      ],
      'font' => [
        'bold' => TRUE,
        'color' => ['argb' => 'FFFFFFFF'],
      ],
      'alignment' => [
        'horizontal' => 'center',
      ],
    ]);
    $sheet->getColumnDimension('A')->setAutoSize(TRUE);
    $sheet->getColumnDimension('B')->setAutoSize(TRUE);
    $sheet->getColumnDimension('C')->setAutoSize(TRUE);
    $sheet->getColumnDimension('D')->setAutoSize(TRUE);

    // Add the user rows.
    $row = 2;
    foreach ($user_ids as $uid) {
      $sheet->setCellValue("A$row", $board_members[$uid]['name']);
      $sheet->setCellValue("B$row", 'SSO login not implemented yet');
      $sheet->setCellValue("C$row", $board_members[$uid]['login']);
      $sheet->setCellValue("D$row", $board_members[$uid]['unreviewed']);
      ++$row;
    }

    // Not sure why this cell was picked, but that's what the original did.
    $sheet->setSelectedCell('A2');
  }

}
