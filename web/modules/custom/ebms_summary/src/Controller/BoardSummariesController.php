<?php

namespace Drupal\ebms_summary\Controller;

use Drupal\Core\Controller\ControllerBase;
use Drupal\Core\Link;
use Drupal\Core\Url;
use Drupal\ebms_board\Entity\Board;
use Drupal\ebms_doc\Entity\Doc;
use Drupal\ebms_summary\Entity\BoardSummaries;
use Drupal\user\Entity\User;
use Symfony\Component\DependencyInjection\ContainerInterface;
use Symfony\Component\HttpFoundation\RequestStack;

/**
 * Show a menu of summary pages.
 */
class BoardSummariesController extends ControllerBase {

  /**
   * Request stack.
   *
   * @var RequestStack
   */
  public $request;

  /**
   * Inject our own request stack service property.
   *
   * @param RequestStack $request
   *   Request stack.
   */
  public function __construct(RequestStack $request) {
    $this->request = $request->getCurrentRequest();
  }

  /**
   * {@inheritdoc}
   */
  public static function create(ContainerInterface $container) {
    return new static($container->get('request_stack'));
  }

  /**
   * Allow the user to navigate to one of the summary pages.
   *
   * If a board has not been specified, and the user it not linked
   * to a single board, then ask the user to select a board.
   */
  public function display(int $board_id = 0): array {

    // Make sure we have a single board.
    $user = User::load($this->currentUser()->id());
    if (empty($board_id)) {
      $board_ids = [];
      foreach ($user->boards as $board) {
        $board_ids[] = $board->target_id;
      }
      if (count($board_ids) === 1) {
        $board_id = reset($board_ids);
      }
      else {
        return $this->boardMenu($board_ids);
      }
    }
    $board = Board::load($board_id);

    // Make sure we have a BoardSummaries entity.
    $query_parameters = $this->request->query->all();
    $storage = $this->entityTypeManager()->getStorage('ebms_board_summaries');
    $query = $storage->getQuery()->accessCheck(FALSE);
    $query->condition('board', $board_id);
    $ids = $query->execute();
    if (empty($ids)) {
      $values = ['board' => $board_id];
      $board_summaries = BoardSummaries::create($values);
      $board_summaries->save();
    }
    else {
      // We already have an entity. Handle requested actions.
      $board_summaries = BoardSummaries::load(reset($ids));
      $delta = $this->request->get('archive');
      if (is_numeric($delta)) {
        $board_summaries->docs[$delta]->active = '0';
        $board_summaries->save();
        unset($query_parameters['archive']);
      }
      $delta = $this->request->get('revive');
      if (is_numeric($delta)) {
        $board_summaries->docs[$delta]->active = '1';
        $board_summaries->save();
        unset($query_parameters['revive']);
      }
    }

    // Create the links for the summaries pages.
    $manager = $user->hasPermission('manage summaries');
    $member = $user->hasPermission('review literature');
    $options = ['query' => $query_parameters];
    unset($options['query']['archive']);
    unset($options['query']['revive']);
    $options['query']['board'] = $board_id;
    $pages = [];
    foreach ($board_summaries->pages as $page) {
      $summary_page = $page->entity;
      if (!empty($summary_page->active->value)) {
        $parms = ['summary_page' => $summary_page->id()];
        $values = [
          'name' => $summary_page->name->value,
          'url' => Url::fromRoute('ebms_summary.page', $parms),
        ];
        if ($manager) {
          $values['edit'] = Url::fromRoute('ebms_summary.edit_page', $parms, $options);
          $values['delete'] = Url::fromRoute('ebms_summary.delete_page', ['ebms_summary_page' => $summary_page->id()], $options);
        }
        $pages[] = $values;
      }
    }
    $name = array_column($pages, 'name');
    array_multisort($name, SORT_ASC, $pages);
    $page = [
      '#title' => $board->name->value,
      '#cache' => ['max-age' => 0],
      'pages' => [
        '#theme' => 'summary_pages',
        '#member' => $member,
        '#pages' => $pages,
      ],
    ];

    // Add a button for a new summaries page.
    if ($manager) {
      $page['add-page-button'] = [
        '#theme' => 'links',
        '#links' => [
          [
            'url' => Url::fromRoute('ebms_summary.add_page', [], $options),
            'title' => 'Add New Summaries Page',
            'attributes' => ['class' => ['button', 'usa-button']],
          ],
        ],
      ];
    }

    // Add the supporting documents.
    $show_archived_docs = $this->request->get('archived-docs') === 'show';
    $docs = [];
    $route = 'ebms_summary.board';
    $parms = ['board_id' => $board_id];
    foreach ($board_summaries->docs as $delta => $doc_usage) {
      $active = $doc_usage->active;
      if ($active || $manager && $show_archived_docs) {
        $doc = Doc::load($doc_usage->doc);
        $file = $doc->file->entity;
        $text = $doc->description->value ?: $file->filename->value;
        $url = Url::fromUri($file->createFileUrl(FALSE));
        $values = [
          'text' => $text,
          'url' => $url,
          'notes' => $doc_usage->notes,
          'user' => $file->uid->entity->name->value,
          'date' => substr($doc->posted->value, 0, 10),
          'archived' => !$active,
        ];
        if ($manager) {
          $options = ['query' => $query_parameters];
          $action = $active ? 'archive' : 'revive';
          $options['query'][$action] = $delta;
          $url = Url::fromRoute($route, $parms, $options)->toString();
          $onclick = "location.href='$url'";
          $values['onclick'] = $onclick;
        }
        $docs[] = $values;
      }
    }
    $header = ['File Name', 'Notes', 'Uploaded By', 'Date'];
    if ($manager) {
      $header[] = 'Archived';
    }
    $page['docs'] = [
      '#theme' => 'doc_table',
      '#caption' => 'Supporting Documents',
      '#header' => $header,
      '#rows' => array_reverse($docs),
      '#empty' => 'No supporting documents have been posted for this board yet.',
    ];

    // Add buttons at the bottom if the user can manage summary pages.
    if ($manager) {
      $eligible_docs = $board_summaries->eligibleDocs();
      if (!empty($eligible_docs) || !empty($board_summaries->docs)) {
        $page['doc-buttons'] = [
          '#theme' => 'links',
          '#links' => [],
        ];
        if (!empty($eligible_docs)) {
          $route = 'ebms_summary.add_board_doc';
          $action = $show_archived_docs ? 'show' : 'hide';
          $options = ['query' => ['archived-docs' => $action]];
          $page['doc-buttons']['#links'][] = [
            'url' => Url::fromRoute($route, $parms, $options),
            'title' => 'Post Document',
            'attributes' => ['class' => ['button', 'usa-button']],
          ];
        }
        if (!empty($board_summaries->docs)) {
          $route = 'ebms_summary.board';
          $action = $show_archived_docs ? 'hide' : 'show';
          $options = ['query' => ['archived-docs' => $action]];
          $page['doc-buttons']['#links'][] = [
            'url' => Url::fromRoute($route, $parms, $options),
            'title' => ucfirst($action) . ' Archived Documents',
            'attributes' => ['class' => ['button', 'usa-button']],
          ];
        }
      }
    }
    return $page;
  }

  /**
   * Let the user pick a board.
   *
   * @param array $board_ids
   *   IDs of the boards linked to the user.
   */
  private function boardMenu(array $board_ids) {
    if (empty($board_ids)) {
      $boards = Board::boards();
    }
    else {
      $boards = [];
      foreach ($board_ids as $board_id) {
        $board = Board::load($board_id);
        $boards[$board_id] = $board->name->value;
      }
      natcasesort($boards);
    }
    $links = [];
    $route = 'ebms_summary.board';
    foreach ($boards as $id => $name) {
      $parms = ['board_id' => $id];
      $links[] = Link::createFromRoute($name, $route, $parms);
    }
    return [
      '#title' => 'Summaries',
      'image' => [
        '#theme' => 'image',
        '#attributes' => [
          'src' => '/themes/custom/ebms/images/typewriter.jpg',
        ],
      ],
      'boards' => [
        '#theme' => 'item_list',
        '#title' => 'Select a board.',
        '#list_type' => 'ul',
        '#items' => $links,
      ],
    ];
  }

}
