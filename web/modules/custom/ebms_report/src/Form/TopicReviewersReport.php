<?php

namespace Drupal\ebms_report\Form;

use Drupal\Core\Form\FormBase;
use Drupal\Core\Form\FormStateInterface;
use Drupal\Core\Link;
use Drupal\Core\Url;
use Drupal\ebms_article\Entity\Article;
use Drupal\ebms_board\Entity\Board;
use Drupal\ebms_core\Entity\SavedRequest;
use Drupal\ebms_topic\Entity\Topic;
use Drupal\user\Entity\User;
use Symfony\Component\HttpFoundation\Response;

/**
 * Show reviewers responsible for a board's topics.
 *
 * @ingroup ebms
 */
class TopicReviewersReport extends FormBase {

  /**
   * {@inheritdoc}
   */
  public function getFormId(): string {
    return 'topic_reviewers_report_form';
  }

  /**
   * {@inheritdoc}
   */
  public function buildForm(array $form, FormStateInterface $form_state, int $report_id = 0): array|Response {

    // Show the board member version if requested.
    $params = empty($report_id) ? [] : SavedRequest::loadParameters($report_id);
    $version = $this->getRequest()->query->get('version');
    if (!empty($params) && $version === 'print') {
      $report = [
        '#theme' => 'topic_reviewers_print_version',
        '#report' => $this->report($params, TRUE),
      ];
      $page = \Drupal::service('renderer')->render($report);
      $response = new Response($page);
      return $response;
    }

    // Collect some values for the report request.
    $user = User::load($this->currentUser()->id());
    $board = $form_state->getValue('board', $params['board'] ?? Board::defaultBoard($user));
    $topics = empty($board) ? [] : Topic::topics($board);
    $selected_topics = $params['topics'] ?? [];
    if (!empty($selected_topics)) {
      foreach ($selected_topics as $topic) {
        if (!array_key_exists($topic, $topics)) {
          $selected_topics = [];
          break;
        }
      }
    }
    $reviewers = empty($board) ? [] : $this->reviewers($board);
    $selected_reviewers = $params['reviewers'] ?? [];
    if (!empty($selected_reviewers)) {
      foreach ($selected_reviewers as $reviewer) {
        if (!array_key_exists($reviewer, $reviewers)) {
          $selected_reviewers = [];
          break;
        }
      }
    }

    // Assemble the fields for the form.
    $form = [
      '#title' => 'Topic Reviewers',
      '#cache' => ['max-age' => 0],
      'filters' => [
        '#type' => 'details',
        '#title' => 'Filters',
        'board' => [
          '#type' => 'select',
          '#title' => 'Editorial Board',
          '#description' => 'Select the board for which the report is to be generated.',
          '#required' => TRUE,
          '#options' => Board::boards(),
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
            '#title' => 'Summary Topic(s)',
            '#description' => 'Optionally select one or more topics to narrow the report.',
            '#options' => $topics,
            '#multiple' => TRUE,
            '#default_value' => $selected_topics,
            '#empty_value' => '',
          ],
          'reviewers' => [
            '#type' => 'select',
            '#title' => 'Board Member(s)',
            '#description' => 'Optionally select one or more reviewers to narrow the report.',
            '#options' => $reviewers,
            '#multiple' => TRUE,
            '#default_value' => $selected_reviewers,
            '#empty_value' => '',
          ],
        ],
      ],
      'option-box' => [
        '#type' => 'details',
        '#title' => 'Display Options',
        'grouping' => [
          '#type' => 'radios',
          '#title' => 'Group By',
          '#description' => 'Choose the grouping with which the report information should be presented.',
          '#options' => [
            'topic' => 'Topic',
            'member' => 'Board member',
          ],
          '#default_value' => $params['grouping'] ?? 'topic',
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

    // If we have a report request, generate and add it.
    if (!empty($params) && empty($form_state->getValue('board'))) {
      $opts = ['query' => ['version' => 'print']];
      $tooltip = 'Create print-friendly version of report below. Will ignore filtering or display options changed since the Submit button was last pressed.';
      $form['member-version-button'] = [
        '#type' => 'link',
        '#url' => Url::fromRoute('ebms_report.topic_reviewers', ['report_id' => $report_id], $opts),
        '#title' => 'Print Version',
        '#attributes' => ['class' => ['usa-button'], 'target' => '_blank', 'title' => $tooltip],
      ];
      $form['report'] = $this->report($params);
    }
    return $form;
  }

  /**
   * {@inheritdoc}
   */
  public function submitForm(array &$form, FormStateInterface $form_state) {
    $params = $form_state->getValues();
    $request = SavedRequest::saveParameters('board and topic lists report', $params);
    $form_state->setRedirect('ebms_report.topic_reviewers', ['report_id' => $request->id()]);
  }

  /**
   * Create a fresh form.
   */
  public function resetSubmit(array &$form, FormStateInterface $form_state) {
    $form_state->setRedirect('ebms_report.topic_reviewers');
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
   * Construct the render array for a table listing topic reviewers.
   *
   * @param array $values
   *   Values entered by the user on the request form.
   * @param string $print_version
   *   If `TRUE` produced a print-friendly version of the report.
   *
   * @return array
   *   Render array for the report.
   */
  private function report(array $params, bool $print_version = FALSE): array {

    // Get the entities for the topic reviewers.
    $storage = \Drupal::entityTypeManager()->getStorage('user');
    $query = $storage->getQuery()->accessCheck(FALSE);
    $query->condition('status', 1);
    if (!empty($params['topics'])) {
      $query->condition('topics', $params['topics'], 'IN');
    }
    else {
      $query->condition('topics.entity.board', $params['board']);
    }
    if (!empty($params['reviewers'])) {
      $query->condition('uid', $params['reviewers'], 'IN');
    }
    $query->sort('name');
    $users = $storage->loadMultiple($query->execute());
    $reviewer_count = count($users);

    // Generate the render array for the table based on the grouping.
    $rows = [];
    $renderer = \Drupal::service('renderer');
    $tooltip = 'Edit this topic.';
    $options = ['attributes' => ['target' => '_blank', 'title' => $tooltip]];
    $route = 'ebms_topic.reviewers';
    $grouping = $params['grouping'] ?? 'topic';
    if ($grouping === 'topic') {
      $header = ['Topic', 'Board Members'];
      $topic_reviewers = [];
      foreach ($users as $user) {
        $name = $user->name->value;
        foreach ($user->topics as $topic) {
          $topic_id = $topic->target_id;
          if (empty($params['topics']) || array_key_exists($topic_id, $params['topics'])) {
            if (!array_key_exists($topic_id, $topic_reviewers)) {
              $topic_reviewers[$topic_id] = [$name];
            }
            else {
              $topic_reviewers[$topic_id][] = $name;
            }
          }
        }
      }
      $storage = \Drupal::entityTypeManager()->getStorage('ebms_topic');
      $query = $storage->getQuery()->accessCheck(FALSE);
      $query->condition('id', array_keys($topic_reviewers), 'IN');
      $query->sort('name');
      foreach ($storage->loadMultiple($query->execute()) as $topic) {
        $reviewers = [
          '#theme' => 'item_list',
          '#items' => $topic_reviewers[$topic->id()],
          '#attributes' => ['class' => ['usa-list--unstyled']],
        ];
        $rows[] = [
          $print_version ? $topic->name->value : Link::createFromRoute($topic->name->value, $route, ['topic_id' => $topic->id()], $options),
          $renderer->render($reviewers),
        ];
      }
      $topic_count = count($rows);
    }
    else {
      $header = ['Board Member', 'Topics'];
      $topic_ids = [];
      foreach ($users as $user) {
        $topics = [];
        foreach ($user->topics as $topic) {
          $topic_id = $topic->target_id;
          if (empty($params['topics']) || array_key_exists($topic_id, $params['topics'])) {
            $topic_name = $topic->entity->name->value;
            $topic_ids[$topic_id] = $topic_id;
            $topics[$topic_name] = $print_version ? $topic_name : Link::createFromRoute($topic_name, $route, ['topic_id' => $topic_id], $options);
          }
        }
        ksort($topics);
        $topics_list = [
          '#theme' => 'item_list',
          '#items' => $topics,
          '#attributes' => ['class' => ['usa-list--unstyled']],
        ];
        $rows[] = [
          $user->name->value,
          $renderer->render($topics_list),
        ];
      }
      $topic_count = count($topic_ids);
    }

    // Assemble the render array for the report's table.
    $topic_s = $topic_count === 1 ? '' : 's';
    $reviewer_s = $reviewer_count === 1 ? '' : 's';
    $board = Board::load($params['board']);
    $board_name = $board->name->value;
    $report['table'] = [
      '#type' => 'table',
      '#caption' => "$board_name Board ($topic_count Topic$topic_s, $reviewer_count Reviewer$reviewer_s)",
      '#header' => $header,
      '#rows' => $rows,
      '#attributes' => ['class' => ['topic-reviewers-table']],
      '#empty' => 'No rows match the selected filter criteria.',
    ];
    return $report;
  }

  /**
   * Create the picklist options for reviewer belonging to a specific board.
   *
   * @param int $board_id
   *   Entity ID for the board whose members we're looking for.
   *
   * @return array
   *   Sorted reviewer names indexed by user ID.
   */
  private function reviewers(int $board_id): array {
    $storage = \Drupal::entityTypeManager()->getStorage('user');
    $query = $storage->getQuery()->accessCheck(FALSE);
    $query->condition('boards', $board_id);
    $query->condition('roles', 'board_member');
    $query->sort('name');
    $reviewers = [];
    foreach ($storage->loadMultiple($query->execute()) as $reviewer) {
      $reviewers[$reviewer->id()] = $reviewer->name->value;
    }
    return $reviewers;
  }

}
