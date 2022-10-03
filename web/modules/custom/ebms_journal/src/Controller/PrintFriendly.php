<?php

namespace Drupal\ebms_journal\Controller;

use Drupal\Core\Controller\ControllerBase;
use Drupal\ebms_board\Entity\Board;
use Drupal\ebms_core\Entity\SavedRequest;
use Drupal\ebms_journal\Entity\Journal;
use Symfony\Component\HttpFoundation\Response;

/**
 * Create a print-friendly list of journals.
 */
class PrintFriendly extends ControllerBase {

  /**
   * For displaying which filtering was used.
   */
  const FILTER_NAMES = [
    'brief-title' => 'brief title',
    'full-title' => 'full title',
    'journal-id' => 'journal ID',
  ];

  /**
   * Show the journals.
   *
   * @param SavedRequest $saved_request
   *   Values used for filtering the journals.
   *
   * @return Response
   *   Provides simpler HTML which bypasses our normal theme.
   */
  public function show(SavedRequest $saved_request): Response {
    $parameters = $saved_request->getParameters();
    $query = Journal::createQuery($parameters);
    $query->sort('brief_title');
    $ids = $query->execute();
    $count = count($ids);
    $board = Board::load($parameters['board']);
    $name = $board->name->value;
    $which = ucfirst($parameters['inclusion-exclusion']);
    $filters = [];
    foreach (self::FILTER_NAMES as $field => $display) {
      if (!empty($parameters[$field])) {
        $value = $parameters[$field];
        $filters[] = "With $display containing '$value'";
      }
    }
    if (!empty($parameters['brief-title'])) {}
    $journals = [];
    foreach (Journal::loadMultiple($ids) as $journal) {
      $values = [
        'brief' => $journal->brief_title->value,
        'full' => $journal->title->value,
        'id' => $journal->source_id->value,
      ];
      if ($which === 'All') {
        $excluded = 'No';
        foreach ($journal->not_lists as $not_list) {
          if ($not_list->board == $board->id()) {
            $excluded = 'Yes';
            break;
          }
        }
        $values['excluded'] = $excluded;
      }
      $journals[] = $values;
    }
    $render_array = [
      '#theme' => 'print_friendly_journals',
      '#title' => "$which $name Journals ($count)",
      '#filters' => $filters,
      '#all' => $which === 'All',
      '#journals' => $journals,
    ];
    $page = \Drupal::service('renderer')->render($render_array);
    $response = new Response($page);
    return $response;
  }
}
