<?php

namespace Drupal\ebms_report\Form;

use Drupal\Core\Form\FormBase;
use Drupal\Core\Form\FormStateInterface;
use Drupal\Core\Url;
use Drupal\ebms_article\Entity\Article;
use Drupal\ebms_board\Entity\Board;
use Drupal\ebms_core\Entity\SavedRequest;
use Drupal\ebms_import\Entity\Batch;
use Drupal\ebms_review\Entity\Packet;
use Drupal\ebms_review\Entity\Review;
use Drupal\ebms_state\Entity\State;
use Drupal\ebms_topic\Entity\Topic;
use Drupal\file\Entity\File;
use Drupal\taxonomy\Entity\Term;

/**
 * Reports for articles in various stages of the processing flow.
 *
 * These reports are all driven, to some extent, by the current states
 * of the articles for each of their assigned topics. This is in contrast
 * with the "Article Statistics" reports, which are more focused on
 * historical activity, ignoring which states are current at the time
 * the report is requested.
 *
 * @ingroup ebms
 */
class ArticlesByStatusReports extends FormBase {

  /**
   * Names of the sub reports.
   */
  const ABSTRACT_DECISION = 'Abstract Decision';
  const FULL_TEXT_RETRIEVED = 'Full Text Retrieved';
  const FULL_TEXT_DECISION = 'Full Text Decision';
  const ASSIGNED_FOR_REVIEW = 'Assigned For Review';
  const BOARD_MEMBER_RESPONSES = 'Board Member Responses';
  const BOARD_MANAGER_ACTION = 'Board Manager Action';
  const ON_AGENDA = 'On Agenda';
  const EDITORIAL_BOARD_DECISION = 'Editorial Board Decision';
  const REPORTS = [
    ArticlesByStatusReports::ABSTRACT_DECISION,
    ArticlesByStatusReports::FULL_TEXT_RETRIEVED,
    ArticlesByStatusReports::FULL_TEXT_DECISION,
    ArticlesByStatusReports::ASSIGNED_FOR_REVIEW,
    ArticlesByStatusReports::BOARD_MEMBER_RESPONSES,
    ArticlesByStatusReports::BOARD_MANAGER_ACTION,
    ArticlesByStatusReports::ON_AGENDA,
    ArticlesByStatusReports::EDITORIAL_BOARD_DECISION,
  ];

  /**
   * {@inheritdoc}
   */
  public function getFormId(): string {
    return 'articles_by_status_reports';
  }

  /**
   * {@inheritdoc}
   */
  public function buildForm(array $form, FormStateInterface $form_state, int $report_id = 0): array {

    // Collect some values for the report request.
    $params = empty($report_id) ? [] : SavedRequest::loadParameters($report_id);
    $board = $form_state->getValue('board', $params['board'] ?? '');
    $topics = empty($board) ? [] : Topic::topics($board);
    $topic = $params['topic'] ?? '';
    if (!empty($topic) && !array_key_exists($topic, $topics)) {
      $topic = '';
    }
    $report = $form_state->getValue('report', $params['report'] ?? '');
    $dispositions_multiple = FALSE;
    $default_disposition = '';
    $disposition_description = 'Narrow the report to a single outcome.';
    if (in_array($report, [self::BOARD_MANAGER_ACTION, self::EDITORIAL_BOARD_DECISION])) {
      $dispositions_multiple = TRUE;
      $default_disposition = [];
      $disposition_description = 'Narrow the report to one or more specific outcomes.';
    }
    $dispositions = $this->getDispositions($report);
    $disposition = $form_state->getValue('disposition', $params['disposition'] ?? $default_disposition);
    if (!empty($disposition)) {
      if (is_array($disposition)) {
        foreach ($disposition as $name) {
          if (!array_key_exists($name, $dispositions)) {
            $disposition = [];
            break;
          }
        }
      }
      elseif (!array_key_exists($disposition, $dispositions)) {
        $disposition = '';
      }
    }
    if ($report === self::FULL_TEXT_RETRIEVED && empty($disposition)) {
      $disposition = 'retrieved';
    }
    $report_controlled = [
      '#type' => 'container',
      '#attributes' => ['id' => 'report-controlled'],
    ];
    if ($report !== self::ASSIGNED_FOR_REVIEW && $report !== self::ON_AGENDA) {
      $report_controlled['disposition'] = [
        '#type' => 'select',
        '#title' => 'Disposition',
        '#description' => $disposition_description,
        '#options' => $dispositions,
        '#default_value' => $disposition,
        '#multiple' => $dispositions_multiple,
        '#required' => $report === self::FULL_TEXT_RETRIEVED,
      ];
    }

    // Assemble the render array for the form's fields.
    $form = [
      '#title' => 'Articles By Status',
      '#attached' => ['library' => ['ebms_report/articles-by-status']],
      'filters' => [
        '#type' => 'details',
        '#title' => 'Filters',
        'report' => [
          '#type' => 'select',
          '#title' => 'Report',
          '#description' => 'Select the report to be created.',
          '#options' => array_combine(self::REPORTS, self::REPORTS),
          '#required' => TRUE,
          '#default_value' => $report,
          '#empty_value' => '',
          '#attributes' => ['name' => 'report'],
          '#ajax' => [
            'callback' => '::reportChangeCallback',
            'wrapper' => 'report-controlled',
            'event' => 'change',
          ],
        ],
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
          'topic' => [
            '#type' => 'select',
            '#title' => 'Summary Topic',
            '#description' => 'Optionally select a topic to further narrow the report.',
            '#options' => $topics,
            '#default_value' => $topic,
            '#empty_value' => '',
          ],
        ],
        'report-controlled' => $report_controlled,
        'cycles' => [
          '#type' => 'select',
          '#title' => 'Review Cycle(s)',
          '#description' => 'Restrict the report by cycle for which the topic was assigned.',
          '#options' => Batch::cycles(),
          '#multiple' => TRUE,
          '#default_value' => $params['cycles'] ?? [],
          '#states' => [
            'invisible' => [
              ':input[name="report"]' => ['value' => self::BOARD_MEMBER_RESPONSES],
            ],
          ],
        ],
        'review-dates' => [
          '#type' => 'container',
          '#attributes' => ['class' => ['inline-fields']],
          '#title' => 'Review Date Range',
          '#description' => 'Show reviews submitted during the specified date range.',
          '#states' => [
            'visible' => [
              ':input[name="report"]' => ['value' => self::BOARD_MEMBER_RESPONSES],
            ],
          ],
          'review-start' => [
            '#type' => 'date',
            '#default_value' => $params['review-start'] ?? '',
          ],
          'review-end' => [
            '#type' => 'date',
            '#default_value' => $params['review-end'] ?? '',
          ],
        ],
        'meeting-fields' => [
          '#type' => 'container',
          '#states' => [
            'visible' => [
              ':input[name="report"]' => [
                ['value' => self::ON_AGENDA],
                'or',
                ['value' => self::EDITORIAL_BOARD_DECISION],
              ],
            ],
          ],
          'meeting-category' => [
            '#type' => 'select',
            '#title' => 'Meeting Category',
            '#description' => 'Restrict the report by the attendance scope for the meeting.',
            '#options' => self::getTerms('meeting_categories'),
            '#default_value' => $params['meeting-category'] ?? '',
            '#empty_value' => '',
          ],
          'meeting-dates' => [
            '#type' => 'container',
            '#attributes' => ['class' => ['inline-fields']],
            '#title' => 'Meeting Date Range',
            '#description' => 'Restrict the report to articles associated with meetings scheduled during the specified date range.',
            'meeting-start' => [
              '#type' => 'date',
              '#default_value' => $params['meeting-start'] ?? '',
            ],
            'meeting-end' => [
              '#type' => 'date',
              '#default_value' => $params['meeting-end'] ?? '',
            ],
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

    // Tweak the field which is multi-valued for some of the reports.
    if (!$dispositions_multiple && $report !== self::FULL_TEXT_RETRIEVED) {
      $form['filters']['report-controlled']['disposition']['#empty_value'] = '';
    }

    // Add the report if we're ready (and this is not an AJAX call).
    if (!empty($params['report']) && empty($form_state->getValues())) {
      switch ($params['report']) {
        case self::ABSTRACT_DECISION:
          $form['report'] = $this->abstractDecisionReport($params);
          break;
        case self::FULL_TEXT_RETRIEVED:
          $form['report'] = $this->fullTextRetrievalReport($params);
          break;
        case self::FULL_TEXT_DECISION:
          $form['report'] = $this->fullTextDecisionReport($params);
          break;
        case self::ASSIGNED_FOR_REVIEW:
          $form['report'] = $this->assignedForReviewReport($params);
          break;
        case self::BOARD_MEMBER_RESPONSES:
          $form['report'] = $this->boardMemberResponsesReport($params);
          break;
        case self::BOARD_MANAGER_ACTION:
          $form['report'] = $this->boardManagerActionReport($params);
          break;
        case self::ON_AGENDA:
          $form['report'] = $this->onAgendaReport($params);
          break;
        case self::EDITORIAL_BOARD_DECISION:
          $form['report'] = $this->boardDecisionReport($params);
          break;
      }
    }
    return $form;
  }

  /**
   * {@inheritdoc}
   */
  public function submitForm(array &$form, FormStateInterface $form_state) {
    $request = SavedRequest::saveParameters('articles by status report', $form_state->getValues());
    $form_state->setRedirect('ebms_report.articles_by_status', ['report_id' => $request->id()]);
  }

  /**
   * Create a fresh form.
   */
  public function resetSubmit(array &$form, FormStateInterface $form_state) {
    $form_state->setRedirect('ebms_report.articles_by_status');
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
   * Fill in the portion of the form driven by the report selection.
   *
   * @param array $form
   *   Render array we are adjusting.
   * @param FormStateInterface $form_state
   *   Access to the form's values.
   */
  public function reportChangeCallback(array &$form, FormStateInterface $form_state) {
    return $form['filters']['report-controlled'];
  }

  /**
   * Get the picklist of available review decisions.
   *
   * @param string $report
   *   Name of the report.
   *
   * @return array
   *   Array of value => display string combinations.
   */
  private function getDispositions($report) {
    switch ($report) {
      case self::ABSTRACT_DECISION:
        return [
          'passed_bm_review' => 'Approved',
          'reject_bm_review' => 'Rejected',
        ];
      case self::FULL_TEXT_RETRIEVED:
        return [
          'retrieved' => 'Retrieved',
          'unavailable' => 'Unavailable',
        ];
      case self::FULL_TEXT_DECISION:
        return [
          'passed_full_review' => 'Approved',
          'reject_full_review' => 'Rejected',
          'fyi' => 'Flagged as FYI',
        ];
      case self::BOARD_MEMBER_RESPONSES:
        return self::getTerms('dispositions');
      case self::BOARD_MANAGER_ACTION:
        return self::getBoardManagerActions();
      case self::EDITORIAL_BOARD_DECISION:
        return self::getTerms('board_decisions');
      default:
        return [];
    }
  }

  /**
   * Get the comment strings for a State entity.
   *
   * @param State $state
   *   The object capturing information about an article-topic decision.
   *
   * @return array
   *   Possibly empty array of comment strings.
   */
  private function getComments(State $state): array {
    $comments = [];
    foreach ($state->comments as $comment) {
      $body = trim($comment->body);
      if (!empty($body)) {
        $comments[] = $body;
      }
    }
    return $comments;
  }

  /**
   * Get a picklist from a specific vocabulary.
   *
   * @param string $vid
   *   Vocabulary ID.
   *
   * @return array
   *   Array of terms indexed by their term IDs.
   */
  public static function getTerms(string $vid): array {
    $storage = \Drupal::entityTypeManager()->getStorage('taxonomy_term');
    $query = $storage->getQuery()->accessCheck(FALSE);
    $query->condition('vid', $vid);
    $query->sort('weight');
    $terms = [];
    foreach ($storage->loadMultiple($query->execute()) as $term) {
      $terms[$term->id()] = $term->getName();
    }
    return $terms;
  }

  /**
   * Get the state names for board manager decisions, indexed by ID.
   *
   * @return array
   *   State display names indexed by Term ID.
   */
  public static function getBoardManagerActions(): array {
    $storage = \Drupal::entityTypeManager()->getStorage('taxonomy_term');
    $query = $storage->getQuery()->accessCheck(FALSE);
    $query->condition('vid', 'states');
    $query->condition('field_text_id', 'on_hold');
    $ids = $query->execute();
    if (count($ids) !== 1) {
      return [0 => 'INTERNAL ERROR--UNABLE TO FIND "ON HOLD" STATE'];
    }
    $term = Term::load(reset($ids));
    $sequence = $term->field_sequence->value;
    $query = $storage->getQuery()->accessCheck(FALSE);
    $query->condition('vid', 'states');
    $query->condition('field_sequence', $sequence);
    $query->sort('name');
    $states = [];
    foreach ($storage->loadMultiple($query->execute()) as $state) {
      $states[$state->id()] = $state->name->value;
    }
    return $states;
  }

  /**
   * Get the display name for the cycle for which the topic is assigned.
   *
   * @param Article $article
   *   Article entity for which the topic was assigned.
   * @param int $topic_id
   *   ID for the Topic entity.
   *
   * @return string
   *   For example, August 2022.
   */
  private function getCycleForState(Article $article, int $topic_id): string {
    foreach ($article->topics as $article_topic) {
      if ($article_topic->entity->topic->target_id == $topic_id) {
        return Batch::cycleString($article_topic->entity->cycle->value);
      }
    }
    return '[CYCLE NOT FOUND]';
  }

  /**
   * Find the reviews who recommeded that the article be discussed.
   *
   * @param int $article_id
   *   ID of the Article entity in the report.
   * @param int $topic_id
   *   ID of the topic for which the reviewer recommends discussion.
   *
   * @return array
   *   Sorted array of reviewers recommending discussion for the article.
   */
  private function discussionProponents(int $article_id, int $topic_id) {
    $query = \Drupal::database()->select('users_field_data', 'user');
    $query->join('ebms_review', 'review', 'review.reviewer = user.uid');
    $query->join('ebms_packet_article__reviews', 'reviews', 'reviews.reviews_target_id = review.id');
    $query->join('ebms_packet_article', 'article', 'article.id = reviews.entity_id');
    $query->join('ebms_packet__articles', 'articles', 'articles.articles_target_id = article.id');
    $query->join('ebms_packet', 'packet', 'packet.id = articles.entity_id');
    $query->join('ebms_review__dispositions', 'dispositions', 'dispositions.entity_id = review.id');
    $query->join('taxonomy_term_field_data', 'disposition', 'disposition.tid = dispositions.dispositions_target_id');
    $query->condition('article.article', $article_id);
    $query->condition('packet.topic', $topic_id);
    $query->condition('disposition.vid', 'dispositions');
    $query->condition('disposition.name', 'Merits discussion');
    $query->addField('user', 'name', 'name');
    $query->distinct();
    $query->orderBy('name');
    return $query->execute()->fetchCol();
  }

  /**
   * Assemble the render array for displaying an article on the report.
   *
   * @param Article $article
   *   Entity object for the article to be displayed.
   *
   * @return array
   *   Render array with information needed by the report template.
   */
  public static function getArticleValues(Article $article): array {
    return [
      'id' => $article->id(),
      'url' => Url::fromRoute('ebms_article.article', ['article' => $article->id()]),
      'authors' => implode(', ', $article->getAuthors(3)) ?: '[No authors named]',
      'title' => $article->title->value,
      'publication' => $article->getLabel(),
      'pmid' => $article->source_id->value,
    ];
  }

  /**
   * Show decisions made during review of articles from their abstracts.
   *
   * @param array $values
   *   Values entered by the user on the request form.
   *
   * @return array
   *   Render array for the report.
   */
  private function abstractDecisionReport(array $values): array {

    // Construct a query to find the states which the report needs.
    $storage = \Drupal::entityTypeManager()->getStorage('ebms_state');
    $query = $storage->getQuery()->accessCheck(FALSE);
    $query->condition('current', TRUE);
    if (!empty($values['topic'])) {
      $query->condition('topic', $values['topic']);
    }
    else {
      $query->condition('board', $values['board']);
    }
    if (!empty($values['cycles'])) {
      $query->addTag('states_for_cycle');
      if (count($values['cycles']) === 1) {
        $query->addMetaData('cycle', reset($values['cycles']));
        $query->addMetaData('operator', '=');
      }
      else {
        $query->addMetaData('cycle', $values['cycles']);
        $query->addMetaData('operator', 'IN');
      }
    }
    if (!empty($values['disposition'])) {
      $query->condition('value.entity.field_text_id', $values['disposition']);
    }
    else {
      $query->condition('value.entity.field_text_id', ['passed_bm_review', 'reject_bm_review'], 'IN');
    }
    $query->sort('article.entity.authors.0.display_name');
    $query->sort('article.entity.title');
    $query->sort('entered');

    // Walk through the decisions building the render arrays for each article.
    $article_count = $decision_count = 0;
    $articles = [];
    foreach ($storage->loadMultiple($query->execute()) as $state) {
      $decision_count++;
      $article_id = $state->article->target_id;
      $article = $state->article->entity;
      if (!array_key_exists($article_id, $articles)) {
        $article_count++;
        $articles[$article_id] = self::getArticleValues($article);
        $articles[$article_id]['decisions'] = [];
      }
      $articles[$article_id]['decisions'][] = [
        'label' => $state->value->entity->field_text_id->value === 'passed_bm_review' ? 'Approved' : 'Rejected',
        'topic' => $state->topic->entity->name->value,
        'cycle' => $this->getCycleForState($article, $state->topic->target_id),
        'user' => $state->user->entity->name->value,
        'when' => $state->entered->value,
        'comments' => $this->getComments($state),
      ];
    }

    // Put together the render array for the complete report.
    $article_s = $article_count === 1 ? '' : 's';
    $decision_s = $decision_count === 1 ? '' : 's';
    return [
      '#theme' => 'articles_by_status',
      '#title' => "Abstract Review Decisions ($article_count Article$article_s, $decision_count Decision$decision_s)",
      '#articles' => $articles,
    ];
  }

  /**
   * Show which articles are ready for review from full text.
   *
   * @param array $values
   *   Values entered by the user on the request form.
   *
   * @return array
   *   Render array for the report.
   */
  private function fullTextRetrievalReport(array $values): array {

    // Construct a query to find the states which the report needs.
    $storage = \Drupal::entityTypeManager()->getStorage('ebms_state');
    $query = $storage->getQuery()->accessCheck(FALSE);
    $query->condition('current', TRUE);
    $query->condition('value.entity.field_text_id', 'passed_bm_review');
    if (!empty($values['topic'])) {
      $query->condition('topic', $values['topic']);
    }
    else {
      $query->condition('board', $values['board']);
    }
    if (!empty($values['cycles'])) {
      $query->addTag('states_for_cycle');
      if (count($values['cycles']) === 1) {
        $query->addMetaData('cycle', reset($values['cycles']));
        $query->addMetaData('operator', '=');
      }
      else {
        $query->addMetaData('cycle', $values['cycles']);
        $query->addMetaData('operator', 'IN');
      }
    }
    if ($values['disposition'] === 'retrieved') {
      $title = 'Full Text Retrieved';
      $query->exists('article.entity.full_text.file');
    }
    else {
      $title = 'Full Text Unavailable';
      $query->condition('article.entity.full_text.unavailable', 1);
    }
    $query->sort('article.entity.authors.0.display_name');
    $query->sort('article.entity.title');

    // Walk through the decisions building the render arrays for each article.
    $articles = [];
    foreach ($storage->loadMultiple($query->execute()) as $state) {
      $article_id = $state->article->target_id;
      $article = $state->article->entity;
      if (!array_key_exists($article_id, $articles)) {
        $filename = $retrieved = $comment = '';
        if ($values['disposition'] === 'retrieved') {
          $file = File::load($article->full_text->file);
          $filename = $file->filename->value;
          $retrieved = date('Y-m-d', $file->created->value);
        }
        else {
          $comment = $article->full_text->notes;
        }
        $articles[$article_id] = self::getArticleValues($article);
        $articles[$article_id]['topics'] = [];
        $articles[$article_id]['filename'] = $filename;
        $articles[$article_id]['retrieved'] = $retrieved;
        $articles[$article_id]['comment'] = $comment;
      }
      $articles[$article_id]['topics'][] = [
        'name' => $state->topic->entity->name->value,
        'cycle' => $this->getCycleForState($article, $state->topic->target_id),
      ];
    }

    // Put together the render array for the complete report.
    $count = count($articles);
    return [
      '#theme' => 'articles_by_status',
      '#title' => "$title ($count)",
      '#articles' => $articles,
    ];
  }

  /**
   * Show decisions made during review of articles from their full text.
   *
   * @param array $values
   *   Values entered by the user on the request form.
   *
   * @return array
   *   Render array for the report.
   */
  private function fullTextDecisionReport(array $values): array {

    // Construct a query to find the states which the report needs.
    $storage = \Drupal::entityTypeManager()->getStorage('ebms_state');
    $query = $storage->getQuery()->accessCheck(FALSE);
    $query->condition('current', TRUE);
    if (!empty($values['topic'])) {
      $query->condition('topic', $values['topic']);
    }
    else {
      $query->condition('board', $values['board']);
    }
    if (!empty($values['cycles'])) {
      $query->addTag('states_for_cycle');
      if (count($values['cycles']) === 1) {
        $query->addMetaData('cycle', reset($values['cycles']));
        $query->addMetaData('operator', '=');
      }
      else {
        $query->addMetaData('cycle', $values['cycles']);
        $query->addMetaData('operator', 'IN');
      }
    }
    if (!empty($values['disposition'])) {
      $query->condition('value.entity.field_text_id', $values['disposition']);
    }
    else {
      $query->condition('value.entity.field_text_id', ['passed_full_review', 'reject_full_review', 'fyi'], 'IN');
    }
    $query->sort('article.entity.authors.0.display_name');
    $query->sort('article.entity.title');
    $query->sort('entered');

    // Walk through the decisions building the render arrays for each article.
    $article_count = $decision_count = 0;
    $articles = [];
    foreach ($storage->loadMultiple($query->execute()) as $state) {
      $decision_count++;
      $article_id = $state->article->target_id;
      $article = $state->article->entity;
      if (!array_key_exists($article_id, $articles)) {
        $article_count++;
        $articles[$article_id] = self::getArticleValues($article);
        $articles[$article_id]['decisions'] = [];
      }
      switch ($state->value->entity->field_text_id->value) {
        case 'passed_full_review':
          $label = 'Approved';
          break;
        case 'reject_full_review':
          $label = 'Rejected';
          break;
        case 'fyi':
          $label = 'Flagged as FYI';
          break;
        default:
          $value = $state->value->entity->field_text_id->value;
          $label = "[UNRECOGNIZED VALUE '$value']";
          break;
      }
      $articles[$article_id]['decisions'][] = [
        'label' => $label,
        'topic' => $state->topic->entity->name->value,
        'cycle' => $this->getCycleForState($article, $state->topic->target_id),
        'user' => $state->user->entity->name->value,
        'when' => $state->entered->value,
        'comments' => $this->getComments($state),
      ];
    }

    // Put together the render array for the complete report.
    $article_s = $article_count === 1 ? '' : 's';
    $decision_s = $decision_count === 1 ? '' : 's';
    return [
      '#theme' => 'articles_by_status',
      '#title' => "Full-Text Review Decisions ($article_count Article$article_s, $decision_count Decision$decision_s)",
      '#articles' => $articles,
    ];
  }

  /**
   * Show articles in review packets but without any reviews.
   *
   * @param array $values
   *   Values entered by the user on the request form.
   *
   * @return array
   *   Render array for the report.
   */
  private function assignedForReviewReport(array $values): array {

    // There's no acceptable way to do this with an entity query.
    $query = \Drupal::database()->select('ebms_packet', 'packet');
    $query->join('ebms_packet__articles', 'articles', 'articles.entity_id = packet.id');
    $query->join('ebms_packet_article', 'packet_article', 'packet_article.id = articles.articles_target_id');
    $query->join('ebms_article', 'article', 'article.id = packet_article.article');
    $query->join('ebms_article__topics', 'topics', 'topics.entity_id = article.id');
    $query->join('ebms_article_topic', 'article_topic', 'article_topic.id = topics.topics_target_id AND article_topic.topic = packet.topic');
    $query->join('ebms_state', 'state', 'state.article = article.id AND state.topic = article_topic.topic');
    $query->join('taxonomy_term__field_text_id', 'term', 'term.entity_id = state.value');
    $query->leftJoin('ebms_packet_article__reviews', 'reviews', 'reviews.entity_id = packet_article.id');
    $query->leftJoin('ebms_article__authors', 'authors', 'authors.entity_id = article.id AND authors.delta = 0');
    $query->condition('packet.active', 1);
    $query->condition('packet_article.dropped', 0);
    $query->condition('state.current', 1);
    $query->condition('term.field_text_id_value', 'passed_full_review');
    if (!empty($values['topic'])) {
      $query->condition('state.topic', $values['topic']);
    }
    else {
      $query->condition('state.board', $values['board']);
    }
    if (!empty($values['cycles'])) {
      if (count($values['cycles']) === 1) {
        $query->condition('article_topic.cycle', reset($values['cycles']));
      }
      else {
        $query->condition('article_topic.cycle', $values['cycles'], 'IN');
      }
    }
    $query->isNull('reviews.reviews_target_id');
    $query->addField('packet', 'id', 'packet_id');
    $query->addField('packet', 'created', 'created');
    $query->addField('article', 'id', 'article_id');
    $query->addField('article', 'search_title', 'title');
    $query->addField('authors', 'authors_search_name', 'author');
    $query->addField('article_topic', 'cycle');
    $query->distinct();
    $query->orderBy('author');
    $query->orderBy('title');
    $query->orderBy('created');

    // Roll up the array of articles.
    $articles = [];
    $assignments = 0;
    foreach ($query->execute() as $row) {
      $assignments++;
      $article_id = $row->article_id;
      $packet_id = $row->packet_id;
      if (!array_key_exists($article_id, $articles)) {
        $article = Article::load($article_id);
        $articles[$article_id] = self::getArticleValues($article);
        $articles[$article_id]['packets'] = [];
      }
      $packet = Packet::load($packet_id);
      $reviewers = [];
      foreach ($packet->reviewers as $reviewer) {
        $reviewers[] = $reviewer->entity->name->value;
      }
      sort($reviewers);
      $articles[$article_id]['packets'][] = [
        'topic' => $packet->topic->entity->name->value,
        'cycle' => Batch::cycleString($row->cycle),
        'assigned' => $packet->created->value,
        'user' => $packet->created_by->entity->name->value,
        'reviewers' => $reviewers,
      ];
    }

    // Put together the render array for the complete report.
    $article_count = count($articles);
    $article_s = $article_count === 1 ? '' : 's';
    $assignment_s = $assignments === 1 ? '' : 's';
    return [
      '#theme' => 'articles_by_status',
      '#title' => "Articles Assigned For Review ($article_count Article$article_s, $assignments Assignment$assignment_s)",
      '#articles' => $articles,
    ];
  }

  /**
   * Show articles with reviews but without a later state.
   *
   * @param array $values
   *   Values entered by the user on the request form.
   *
   * @return array
   *   Render array for the report.
   */
  private function boardMemberResponsesReport(array $values): array {

    // Again, the entity query API isn't going to cut it.
    $query = \Drupal::database()->select('ebms_packet', 'packet');
    $query->join('ebms_packet__articles', 'articles', 'articles.entity_id = packet.id');
    $query->join('ebms_packet_article', 'packet_article', 'packet_article.id = articles.articles_target_id');
    $query->join('ebms_article', 'article', 'article.id = packet_article.article');
    $query->join('ebms_state', 'state', 'state.article = article.id AND state.topic = packet.topic');
    $query->join('taxonomy_term__field_text_id', 'term', 'term.entity_id = state.value');
    $query->join('ebms_topic', 'topic', 'topic.id = state.topic');
    $query->join('ebms_packet_article__reviews', 'reviews', 'reviews.entity_id = packet_article.id');
    $query->join('ebms_review', 'review', 'review.id = reviews.reviews_target_id');
    $query->leftJoin('ebms_article__authors', 'authors', 'authors.entity_id = article.id AND authors.delta = 0');
    $query->condition('packet.active', 1);
    $query->condition('packet_article.dropped', 0);
    $query->condition('state.current', 1);
    $query->condition('term.field_text_id_value', 'passed_full_review');
    if (!empty($values['topic'])) {
      $query->condition('state.topic', $values['topic']);
    }
    else {
      $query->condition('state.board', $values['board']);
    }
    if (!empty($values['review-start'])) {
      $query->condition('review.posted', $values['review-start'], '>=');
    }
    if (!empty($values['review-end'])) {
      $end = $values['review-end'];
      if (strlen($end) === 10) {
        $end .= ' 23:59:59';
      }
      $query->condition('review.posted', $end, '<=');
    }
    if (!empty($values['disposition'])) {
      $query->join('ebms_review__dispositions', 'dispositions', 'dispositions.entity_id = review.id');
      $query->condition('dispositions.dispositions_target_id', $values['disposition']);
    }
    $query->addField('article', 'id', 'article_id');
    $query->addField('review', 'id', 'review_id');
    $query->addField('review', 'posted', 'posted');
    $query->addField('topic', 'name', 'topic');
    $query->addField('article', 'search_title', 'title');
    $query->addField('authors', 'authors_search_name', 'author');
    $query->distinct();
    $query->orderBy('author');
    $query->orderBy('title');
    $query->orderBy('posted');

    // Roll up the array of articles.
    $articles = [];
    $reviews = 0;
    foreach ($query->execute() as $row) {
      $reviews++;
      $article_id = $row->article_id;
      $review_id = $row->review_id;
      if (!array_key_exists($article_id, $articles)) {
        $article = Article::load($article_id);
        $articles[$article_id] = self::getArticleValues($article);
        $articles[$article_id]['reviews'] = [];
      }
      $review = Review::load($review_id);
      $dispositions = [];
      foreach ($review->dispositions as $disposition) {
        $dispositions[] = $disposition->entity->name->value;
      }
      $comments = [];
      foreach ($review->comments as $comment) {
        $comments[] = $comment->value;
      }
      $articles[$article_id]['reviews'][] = [
        'topic' => $row->topic,
        'reviewer' => $review->reviewer->entity->name->value,
        'reviewed' => $review->posted->value,
        'dispositions' => $dispositions,
        'comments' => $comments,
      ];
    }

    // Put together the render array for the complete report.
    $article_count = count($articles);
    $article_s = $article_count === 1 ? '' : 's';
    $review_s = $reviews === 1 ? '' : 's';
    return [
      '#theme' => 'articles_by_status',
      '#title' => "Reviewer Responses ($article_count Article$article_s, $reviews Review$review_s)",
      '#articles' => $articles,
    ];
  }

  /**
   * Show decisions made by the board manager.
   *
   * @param array $values
   *   Values entered by the user on the request form.
   *
   * @return array
   *   Render array for the report.
   */
  private function boardManagerActionReport(array $values): array {

    // Construct a query to find the states which the report needs.
    $storage = \Drupal::entityTypeManager()->getStorage('ebms_state');
    $query = $storage->getQuery()->accessCheck(FALSE);
    $query->condition('current', TRUE);
    if (!empty($values['topic'])) {
      $query->condition('topic', $values['topic']);
    }
    else {
      $query->condition('board', $values['board']);
    }
    if (!empty($values['cycles'])) {
      $query->addTag('states_for_cycle');
      if (count($values['cycles']) === 1) {
        $query->addMetaData('cycle', reset($values['cycles']));
        $query->addMetaData('operator', '=');
      }
      else {
        $query->addMetaData('cycle', $values['cycles']);
        $query->addMetaData('operator', 'IN');
      }
    }
    if (!empty($values['disposition'])) {
      $query->condition('value.target_id', $values['disposition'], 'IN');
    }
    else {
      $disposition_ids = array_keys(self::getBoardManagerActions());
      $query->condition('value.target_id', $disposition_ids, 'IN');
    }
    $query->sort('article.entity.authors.0.display_name');
    $query->sort('article.entity.title');
    $query->sort('entered');

    // Walk through the dispositions building the render arrays for each article.
    $article_count = $action_count = 0;
    $articles = [];
    foreach ($storage->loadMultiple($query->execute()) as $state) {
      $action_count++;
      $article_id = $state->article->target_id;
      $article = $state->article->entity;
      if (!array_key_exists($article_id, $articles)) {
        $article_count++;
        $articles[$article_id] = self::getArticleValues($article);
        $articles[$article_id]['actions'] = [];
      }
      $articles[$article_id]['actions'][] = [
        'name' => $state->value->entity->name->value,
        'topic' => $state->topic->entity->name->value,
        'cycle' => $this->getCycleForState($article, $state->topic->target_id),
        'user' => $state->user->entity->name->value,
        'when' => $state->entered->value,
        'comments' => $this->getComments($state),
        'discussion_proponents' => $this->discussionProponents($article_id, $state->topic->target_id),
      ];
    }

    // Put together the render array for the complete report.
    $article_s = $article_count === 1 ? '' : 's';
    $action_s = $action_count === 1 ? '' : 's';
    return [
      '#theme' => 'articles_by_status',
      '#title' => "Board Manager Actions ($article_count Article$article_s, $action_count Action$action_s)",
      '#articles' => $articles,
    ];
  }

  /**
   * Show articles which are in the "On Agenda" state.
   *
   * The original report omitted articles which were in the "on agenda"
   * state, but for which the state had no meetings attached. We can
   * restore that logic if the users prefer, but it seems more useful
   * to show the articles which say they're on an agenda, but the meetings
   * whose agenda they're supposed to be on arent't there.
   *
   * @param array $values
   *   Values entered by the user on the request form.
   *
   * @return array
   *   Render array for the report.
   */
  private function onAgendaReport(array $values): array {

    // Construct a query to find the states which the report needs.
    $storage = \Drupal::entityTypeManager()->getStorage('ebms_state');
    $query = $storage->getQuery()->accessCheck(FALSE);
    $query->condition('current', TRUE);
    $query->condition('value.entity.field_text_id', 'on_agenda');
    if (!empty($values['topic'])) {
      $query->condition('topic', $values['topic']);
    }
    else {
      $query->condition('board', $values['board']);
    }
    if (!empty($values['cycles'])) {
      $query->addTag('states_for_cycle');
      if (count($values['cycles']) === 1) {
        $query->addMetaData('cycle', reset($values['cycles']));
        $query->addMetaData('operator', '=');
      }
      else {
        $query->addMetaData('cycle', $values['cycles']);
        $query->addMetaData('operator', 'IN');
      }
    }
    if (!empty($values['meeting-start'])) {
      $query->condition('meetings.entity.dates.value', $values['meeting-start'], '>=');
    }
    if (!empty($values['meeting-end'])) {
      $end = $values['meeting-start'];
      if (strlen($end) === 10) {
        $end .= ' 23:59:59';
      }
      $query->condition('meetings.entity.dates.value', $end, '<=');
    }
    if (!empty($values['meeting-category'])) {
      $query->condition('meetings.entity.category', $values['meeting-category']);
    }
    $query->sort('article.entity.authors.0.display_name');
    $query->sort('article.entity.title');
    $query->sort('entered');

    // Walk through the dispositions building the render arrays for each article.
    $article_count = $topic_count = $meeting_count = 0;
    $articles = [];
    foreach ($storage->loadMultiple($query->execute()) as $state) {
      $topic_count++;
      $article_id = $state->article->target_id;
      $article = $state->article->entity;
      if (!array_key_exists($article_id, $articles)) {
        $article_count++;
        $articles[$article_id] = self::getArticleValues($article);
        $articles[$article_id]['agenda_topics'] = [];
      }
      $meetings = [];
      foreach ($state->meetings as $meeting) {
        $meeting_count++;
        $meetings[] = [
          'name' => $meeting->entity->name->value,
          'date' => $meeting->entity->dates->value,
          'category' => $meeting->entity->category->entity->name->value,
        ];
      }
      $articles[$article_id]['agenda_topics'][] = [
        'name' => $state->topic->entity->name->value,
        'cycle' => $this->getCycleForState($article, $state->topic->target_id),
        'comments' => $this->getComments($state),
        'meetings' => $meetings,
      ];
    }

    // Put together the render array for the complete report.
    $article_s = $article_count === 1 ? '' : 's';
    $topic_s = $topic_count === 1 ? '' : 's';
    $meeting_s = $meeting_count === 1 ? '' : 's';
    return [
      '#theme' => 'articles_by_status',
      '#title' => "Board Manager Actions ($article_count Article$article_s, $topic_count Topic$topic_s, $meeting_count Meeting$meeting_s)",
      '#articles' => $articles,
    ];
  }

  /**
   * Show articles which are in the "final board decision" state.
   *
   * This state represents the end of the line for EBMS processing of an
   * article for an assigned topic.
   *
   * @param array $values
   *   Values entered by the user on the request form.
   *
   * @return array
   *   Render array for the report.
   */
  private function boardDecisionReport(array $values): array {

    // Construct a query to find the states which the report needs.
    $storage = \Drupal::entityTypeManager()->getStorage('ebms_state');
    $query = $storage->getQuery()->accessCheck(FALSE);
    $query->condition('current', TRUE);
    $query->condition('value.entity.field_text_id', 'final_board_decision');
    if (!empty($values['topic'])) {
      $query->condition('topic', $values['topic']);
    }
    else {
      $query->condition('board', $values['board']);
    }
    if (!empty($values['cycles'])) {
      $query->addTag('states_for_cycle');
      if (count($values['cycles']) === 1) {
        $query->addMetaData('cycle', reset($values['cycles']));
        $query->addMetaData('operator', '=');
      }
      else {
        $query->addMetaData('cycle', $values['cycles']);
        $query->addMetaData('operator', 'IN');
      }
    }
    if (!empty($values['disposition'])) {
      $query->condition('decisions->entity->decision', $values['disposition'], 'IN');
    }
    if (!empty($values['meeting-start']) || !empty($values['meeting-end']) || !empty($values['meeting-category'])) {
      $query->addTag('meetings_for_board_decision');
      $query->addMetaData('meeting-start', $values['meeting-start'] ?? '');
      $query->addMetaData('meeting-end', $values['meeting-end'] ?? '');
      $query->addMetaData('meeting-category', $values['meeting-category'] ?? '');
    }
    $query->sort('article.entity.authors.0.display_name');
    $query->sort('article.entity.title');
    $query->sort('entered');

    // Walk through the dispositions building the render arrays for each article.
    $article_count = $topic_count = $decision_count = 0;
    $articles = [];
    foreach ($storage->loadMultiple($query->execute()) as $state) {
      $topic_count++;
      $article_id = $state->article->target_id;
      $article = $state->article->entity;
      if (!array_key_exists($article_id, $articles)) {
        $article_count++;
        $articles[$article_id] = self::getArticleValues($article);
        $articles[$article_id]['agenda_topics'] = [];
      }
      $decisions = [];
      foreach ($state->decisions as $state_decision) {
        $decision_count++;
        $term = Term::load($state_decision->decision);
        $decisions[] = $term->name->value;
      }
      $deciders = [];
      foreach ($state->deciders as $decider) {
        $deciders[] = $decider->entity->name->value;
      }
      sort($deciders);
      $articles[$article_id]['board_decision_topics'][] = [
        'name' => $state->topic->entity->name->value,
        'cycle' => $this->getCycleForState($article, $state->topic->target_id),
        'when' => $state->entered->value,
        'decisions' => $decisions,
        'comments' => $this->getComments($state),
        'deciders' => $deciders,
      ];
    }

    // Put together the render array for the complete report.
    $article_s = $article_count === 1 ? '' : 's';
    $topic_s = $topic_count === 1 ? '' : 's';
    $decision_s = $decision_count === 1 ? '' : 's';
    return [
      '#theme' => 'articles_by_status',
      '#title' => "Editorial Board Decisions ($article_count Article$article_s, $topic_count Topic$topic_s, $decision_count Decision$decision_s)",
      '#articles' => $articles,
    ];
  }

}
