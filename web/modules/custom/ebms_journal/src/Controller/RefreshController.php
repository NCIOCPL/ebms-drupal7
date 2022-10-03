<?php

namespace Drupal\ebms_journal\Controller;

use Drupal\Core\Controller\ControllerBase;
use Drupal\ebms_journal\Entity\Journal;

/**
 * Refresh the journal information from NLM.
 */
class RefreshController extends ControllerBase {

  /**
   * Do the refresh and show the results.
   *
   * @return array
   *   Render array for table of processing counts (see Journal::refresh).
   */
  public function refresh(): array {
    $report = Journal::refresh(TRUE);
    if (!empty($report['error'])) {
      $this->messenger()->addError($report['error']);
    }
    return [
      '#theme' => 'table',
      '#caption' => 'Processing completed in ' . round($report['elapsed'], 3) . ' seconds',
      '#header' => ['Journals', 'Count'],
      '#rows' => [
        ['Journals fetched', $report['fetched']],
        ['Journals checked', $report['checked']],
        ['Journals updated', $report['updated']],
        ['Journals added', $report['added']],
      ],
    ];
  }

}
