<?php

namespace Drupal\ebms_review\Form;

use Drupal\Core\Entity\EntityTypeManagerInterface;
use Drupal\Core\Form\FormBase;
use Drupal\Core\Form\FormStateInterface;
use Drupal\Core\Url;
use Drupal\Core\Session\AccountProxyInterface;
use Drupal\ebms_board\Entity\Board;
use Drupal\ebms_core\Entity\SavedRequest;
use Drupal\ebms_review\Entity\Packet;
use Drupal\ebms_topic\Entity\Topic;
use Drupal\user\Entity\User;
use Symfony\Component\DependencyInjection\ContainerInterface;

/**
 * Filtered view of packet list.
 *
 * @ingroup ebms
 */
class Packets extends FormBase {

  /**
   * The entity type manager.
   *
   * @var \Drupal\Core\Entity\EntityTypeManagerInterface
   */
  protected EntityTypeManagerInterface $entityTypeManager;

  /**
   * {@inheritdoc}
   */
  public static function create(ContainerInterface $container): Packets {
    // Instantiates this form class.
    $instance = parent::create($container);
    $instance->entityTypeManager = $container->get('entity_type.manager');
    return $instance;
  }

  /**
   * {@inheritdoc}
   */
  public function getFormId(): string {
    return 'ebms_packets_form';
  }

  /**
   * {@inheritdoc}
   */
  public function buildForm(array $form, FormStateInterface $form_state, $request_id = 0): array {

    // Handle requested actions.
    // @todo The original form had a very confusing and redundant mechanism
    // for bulk archiving, which was turned on even for packets which were
    // already archived. We might implement a more straightforward, logical
    // version of bulk processing, but we should talk to the users first to
    // discuss what it should really look like.
    $archive = $this->getRequest()->query->get('archive') ?: 0;
    if (!empty($archive)) {
      $packet = Packet::load($archive);
      $packet->set('active', 0);
      $packet->save();
      $this->getRequest()->query->remove('archive');
      $this->messenger()->addMessage("Arcived packet #$archive.");
    }
    $reactivate = $this->getRequest()->query->get('reactivate') ?: 0;
    if (!empty($reactivate)) {
      $packet = Packet::load($reactivate);
      $packet->set('active', 1);
      $packet->save();
      $this->getRequest()->query->remove('reactivate');
      $this->messenger()->addMessage("Reactivated packet #$reactivate.");
    }

    // Get the options for the page.
    $user = User::load($this->currentUser()->id());
    $parms = empty($request_id) ? [] : SavedRequest::loadParameters($request_id);
    $user_boards = [];
    foreach ($user->boards as $board) {
      $user_boards[] = $board->target_id;
    }
    if (empty($parms)) {
      $selected_boards = $user_boards;
      $selected_topics = [];
      $per_page = 10;
      $sort = 'created';
      $packet_statuses = ['active'];
      $review_statuses = ['reviewed', 'unreviewed'];
    }
    else {
      $selected_boards = array_values(array_diff($parms['boards'], [0]));
      $selected_topics = array_values(array_diff($parms['topics'], [0]));
      $packet_statuses = array_values(array_diff($parms['packet-statuses'], [0]));
      $review_statuses = array_values(array_diff($parms['review-statuses'], [0]));
      $per_page = $parms['per-page'];
      $sort = $parms['sort'];
    }

    // Create the picklists.
    $boards = Board::boards();
    $topics = empty($selected_boards) ? [0 => 'Select a board'] : Topic::topics($selected_boards);
    if (!empty($selected_topics)) {
      foreach ($selected_topics as $topic_id) {
        if (!array_key_exists($topic_id, $topics)) {
          $selected_topics = [];
        }
      }
    }

    // Fetch the packets which match the search criteria.
    $items = [];
    $num_packets = 0;
    $start = 1;
    if (!empty($packet_statuses) && !empty($review_statuses)) {
      $storage = $this->entityTypeManager->getStorage('ebms_packet');
      $query = $storage->getQuery()->accessCheck(FALSE);
      if (!empty($selected_boards)) {
        $query->condition('topic.entity.board', $selected_boards, 'IN');
      }
      if (!empty($selected_topics)) {
        $query->condition('topic', $selected_boards, 'IN');
      }
      if (!empty($parms['packet-name'])) {
        $query->condition('title', $parms['packet-name'], 'LIKE');
      }
      if (!empty($parms['creation-start'])) {
        $query->condition('created', $parms['creation-start'], '>=');
      }
      if (!empty($parms['creation-end'])) {
        $end = $parms['creation-end'];
        if (strlen($end) === 10) {
          $end .= ' 23:59:59';
        }
        $query->condition('created', $end, '<=');
      }
      if (count($packet_statuses) === 1) {
        $query->condition('active', reset($packet_statuses) === 'active' ? 1 : 0);
      }
      if (count($review_statuses) === 1) {
        $condition = reset($review_statuses) === 'reviewed' ? 'exists' : 'notExists';
        $query->$condition('articles.entity.reviews');
      }
      $count_query = clone($query);
      $num_packets = (int) $count_query->count()->execute();
      $query->sort($sort, $sort === 'created' ? 'DESC' : 'ASC');
      if ($sort !== 'created') {
        $query->sort('created', 'DESC');
      }
      if ($per_page !== 'all') {
        $query->pager($per_page);
        $page = $this->getRequest()->query->get('page') ?: 0;
        $start += $page * $per_page;
      }
      $packets = $storage->loadMultiple($query->execute());

      // Assemble the render arrays for the packets.
      foreach ($packets as $packet) {
        $action = empty($packet->active->value) ? 'reactivate' : 'archive';
        $edit_opts = [];
        $action_opts = ['query' => [$action => $packet->id()]];
        if (!empty($request_id)) {
          $edit_opts['query'] = ['filter-id' => $request_id];
          $action_opts['query']['filter-id'] = $request_id;
        }
        if (!empty($page)) {
          $action_opts['query']['page'] = $edit_opts['query']['page'] = $page;
        }
        $values = [
          'name' => $packet->title->value,
          'id' => $packet->id(),
          'edit_url' => Url::fromRoute('ebms_review.packet_edit_form', ['packet_id' => $packet->id()], $edit_opts),
          'action_url' => Url::fromRoute('ebms_review.packets', ['request_id' => $request_id], $action_opts),
          'action_label' => empty($packet->active->value) ? 'Reactivate' : 'Archive',
          'board' => $packet->topic->entity->board->entity->name->value,
          'posted' => $packet->created->value,
          'user' => $packet->created_by->entity->name->value,
        ];
        $items[] = [
          '#theme' => 'packet_list_packet',
          '#packet' => $values,
        ];
      }
    }

    // Assemble and return the render array for the page.
    return [
      '#attached' => ['library' => ['ebms_review/packets']],
      '#title' => 'Literature Review Packets',
      '#cache' => ['max-age' => 0],
      'filters' => [
        '#type' => 'details',
        '#title' => 'Filtering Options',
        'boards' => [
          '#type' => 'select',
          '#title' => 'Board',
          '#description' => 'Narrow the list of packets to those created for these boards.',
          '#options' => $boards,
          '#default_value' => $selected_boards,
          '#multiple' => TRUE,
          '#ajax' => [
            'callback' => '::getTopicsCallback',
            'wrapper' => 'board-controlled',
            'event' => 'change',
          ],
        ],
        'board-controlled' => [
          '#type' => 'container',
          '#attributes' => ['id' => 'board-controlled'],
          'topics' => [
            '#type' => 'select',
            '#title' => 'Topic',
            '#description' => 'Show packets created for these topics.',
            '#options' => $topics,
            '#default_value' => $selected_topics,
            '#multiple' => TRUE,
            '#validated' => TRUE,
          ],
        ],
        'packet-name' => [
          '#type' => 'textfield',
          '#title' => 'Packet Name',
          '#description' => 'Show packets whose names match this value. Use wildcards for partial name matching.',
          '#default_value' => $parms['packet-name'] ?? '',
        ],
        'creation-date' => [
          '#type' => 'container',
          '#attributes' => ['class' => ['inline-fields']],
          '#title' => 'Creation Date Range',
          '#description' => 'Only show packets created during the specified date range.',
          'creation-start' => [
            '#type' => 'date',
            '#default_value' => $parms['creation-start'] ?? '',
          ],
          'creation-end' => [
            '#type' => 'date',
            '#default_value' => $parms['creation-end'] ?? '',
          ],
        ],
        'packet-statuses' => [
          '#type' => 'checkboxes',
          '#title' => 'Packet Statuses To Include',
          '#description' => 'Filter packet list based on whether the packets are active or archived.',
          '#options' => [
            'active' => 'Active',
            'archived' => 'Archived',
          ],
          '#default_value' => $packet_statuses,
        ],
        'review-statuses' => [
          '#type' => 'checkboxes',
          '#title' => 'Review Statuses To Include',
          '#description' => 'Filter packet list based on whether the articles have been reviewed.',
          '#options' => [
            'reviewed' => 'Reviewed (packets that have at least one review for any article)',
            'unreviewed' => 'Unreviewed (packets that have no reviews for any article)',
          ],
          '#default_value' => $review_statuses,
        ],
      ],
      'display-options' => [
        '#type' => 'details',
        '#title' => 'Display Options',
        'sort' => [
          '#type' => 'radios',
          '#title' => 'Sort By',
          '#required' => TRUE,
          '#options' => [
            'title' => 'Packet name',
            'topic.entity.board.entity.name' => 'Packet board',
            'created_by.entity.name' => 'Packet creator',
            'created' => 'Date of creation (most recent first)',
          ],
          '#default_value' => $sort,
          '#description' => 'Select sorting method for search results.',
        ],
          'per-page' => [
          '#type' => 'radios',
          '#title' => 'Per Page',
          '#required' => TRUE,
          '#options' => [
            '10' => '10',
            '25' => '25',
            '50' => '50',
            'all' => 'View All',
          ],
          '#default_value' => $per_page,
          '#description' => 'Specify how many packets should be shown per page.',
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
      'add' => [
        '#title' => 'Create New Packet',
        '#type' => 'link',
        '#url' => Url::fromRoute('ebms_review.packet_form'),
        '#attributes' => ['class' => ['button', 'usa-button']],
      ],
      'packets' => [
        '#theme' => 'item_list',
        '#list_type' => 'ol',
        '#title' => "Packets ($num_packets)",
        '#items' => $items,
        '#empty' => 'No packets which match the filtering criteria were found.',
        '#attributes' => ['start' => $start],
      ],
      'pager' => [
        '#type' => 'pager',
      ],
    ];
  }

  /**
   * {@inheritdoc}
   */
  public function submitForm(array &$form, FormStateInterface $form_state) {
    $values = $form_state->getValues();
    $request = SavedRequest::saveParameters('packets list', $values);
    $form_state->setRedirect('ebms_review.packets', ['request_id' => $request->id()]);
  }

  /**
   * Create a fresh search form with default values.
   *
   * @param array $form
   *   Form settings.
   * @param \Drupal\Core\Form\FormStateInterface $form_state
   *   Form values.
   */
  public function resetSubmit(array &$form, FormStateInterface $form_state) {
    $form_state->setRedirect('ebms_review.packets');
  }

  /**
   * Fill in the portion of the form driven by board selection.
   *
   * @param array $form
   *   Render array we are adjusting.
   * @param FormStateInterface $form_state
   *   Access to the form's values.
   */
  public function getTopicsCallback(array &$form, FormStateInterface $form_state) {
    $selected_boards = $form_state->getValue('boards');
    $options = empty($selected_boards) ? [0 => 'Select a board'] : Topic::topics($selected_boards);
    $form['filters']['board-controlled']['topics']['#options'] = $options;
    return $form['filters']['board-controlled'];
  }

}
