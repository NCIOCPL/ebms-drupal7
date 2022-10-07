<?php

namespace Drupal\ebms_review\Form;

use Drupal\Core\Database\Query\PagerSelectExtender;
use Drupal\Core\Entity\EntityTypeManagerInterface;
use Drupal\Core\Form\FormBase;
use Drupal\Core\Form\FormStateInterface;
use Drupal\Core\Link;
use Drupal\Core\Url;
use Drupal\ebms_article\Entity\Article;
use Drupal\ebms_board\Entity\Board;
use Drupal\ebms_core\Entity\SavedRequest;
use Drupal\ebms_import\Entity\Batch;
use Drupal\ebms_state\Entity\State;
use Drupal\ebms_topic\Entity\Topic;
use Drupal\file\Entity\File;
use Drupal\user\Entity\User;
use Symfony\Component\DependencyInjection\ContainerInterface;
use Symfony\Component\HttpFoundation\File\Exception\AccessDeniedException;
use Symfony\Component\HttpFoundation\RedirectResponse;

/**
 * Interface for moving articles through the processing states.
 *
 * @ingroup ebms
 */
class ReviewQueue extends FormBase {

  /**
   * How many articles should appear on the page at most.
   */
  const DEFAULT_PAGE_SIZE = 10;

  /**
   * State used to populate the queue for each queue type.
   */
  const STATES = [
    'Librarian Review' => 'ready_init_review',
    'Abstract Review' => 'published',
    'Full Text Review' => 'passed_bm_review',
    'On Hold Review' => 'on_hold',
  ];

  /**
   * Used to filter which queue options are available for the user.
   */
  const STATE_PERMISSIONS = [
    'Librarian Review' => 'perform initial article review',
    'Abstract Review' => 'perform abstract article review',
    'Full Text Review' => 'perform full text article review',
    'On Hold Review' => 'perform full text article review',
  ];

  /**
   * Used to determine which queue to use by default for a given user.
   */
  const DEFAULT_QUEUES = [
    'perform abstract article review' => 'Abstract Review',
    'perform initial article review' => 'Librarian Review',
  ];

  /**
   * Decisions the reviewer can make about each article-topic combination.
   */
  const DECISIONS = ['None', 'Approve', 'Reject', 'FYI', 'On Hold'];

  /**
   * States used when applying decisions.
   */
  const DECISION_STATES = [
    'Librarian Review' => [
      1 => 'passed_init_review',
      2 => 'reject_init_review',
    ],
    'Abstract Review' => [
      1 => 'passed_bm_review',
      2 => 'reject_bm_review',
    ],
    'Full Text Review' => [
      1 => 'passed_full_review',
      2 => 'reject_full_review',
      3 => 'fyi',
      4 => 'on_hold',
    ],
    'On Hold Review' => [
      1 => 'passed_full_review',
      2 => 'reject_full_review',
    ],
  ];

  /**
   * The entity type manager.
   *
   * @var \Drupal\Core\Entity\EntityTypeManagerInterface
   */
  protected EntityTypeManagerInterface $entityTypeManager;

  /**
   * IDs of boards assigned to the current user.
   *
   * @var array
   */
  protected array $userBoards = [];

  /**
   * IDs of topics assigned to the current user.
   *
   * @var array
   */
  protected array $userTopics = [];

  /**
   * Special permission for the branch managers.
   *
   * @var bool
   */
  private bool $canReviewAllTopics = FALSE;

  /**
   * Array of publication types indexed by ancestors.
   */
  private array $pubTypeHierarchy = [];

  /**
   * {@inheritdoc}
   */
  public static function create(ContainerInterface $container): ReviewQueue {
    $instance = parent::create($container);
    $instance->entityTypeManager = $container->get('entity_type.manager');
    $account = User::load($instance->currentUser()->id());
    foreach ($account->boards as $board) {
      $instance->userBoards[] = $board->target_id;
    }
    foreach ($account->topics as $topic) {
      $instance->userTopics[] = $topic->target_id;
    }
    $instance->canReviewAllTopics = $account->hasPermission('perform all topic reviews');
    $connection = $container->get('database');
    $select = $connection->select('on_demand_config', 'c');
    $select->condition('c.name', 'article-type-ancestors');
    $select->fields('c', ['value']);
    $json = $select->execute()->fetchField();
    $instance->pubTypeHierarchy = json_decode($json, TRUE);
    return $instance;
  }

  /**
   * {@inheritdoc}
   */
  public function getFormId(): string {
    return 'ebms_review_queue';
  }

  /**
   * {@inheritdoc}
   */
  public function buildForm(array $form, FormStateInterface $form_state, ?int $queue_id = NULL): array | RedirectResponse {

    // If we don't have an object for tracking this queue, create one.
    if (empty($queue_id)) {
      $request = $this->createQueue($form_state);
      $route = 'ebms_review.review_queue';
      $parms = ['queue_id' => $request->id()];
      return new RedirectResponse(Url::fromRoute($route, $parms)->toString());
    }

    // Get the queue's current tracking object and get the cached parameters.
    $params = SavedRequest::loadParameters($queue_id);
    $values = $form_state->getValues();
    if (empty($values)) {
      $board = $params['board'];
      $queue_type = $params['type'];
    }
    else {
      $board = $values['board'] ?: '';
      $queue_type = $values['type'];
    }
    $decisions_json = $params['decisions'];
    $decision_items = $this->getQueuedDecisionsListItems($decisions_json);
    $decisions = json_decode($decisions_json, TRUE);
    $topic = $params['topic'];
    $cycle = $params['cycle'];
    $tag = $params['tag'];
    $sort = $params['sort'];
    $format = $params['format'];
    $per_page = $params['per-page'];
    $title = $params['title'];
    $journal = $params['journal'];

    // Create options for the form.
    $boards = Board::boards();
    $cycles = Batch::cycles();
    $queue_types = $this->getQueueTypes();
    if (count($queue_types) === 1) {
      $queue_selection = [
        '#type' => 'container',
        'type' => [
          '#type' => 'hidden',
          '#value' => reset($queue_types),
        ],
      ];
    }
    else {
      // It would be logical to attach an Ajax callback to the queue selection
      // in order to re-calculate the topic counts based on the queue's state.
      // We don't do that because (a) the original Drupal 7 version doesn't;
      // and (b) there's a bug in Drupal's handling of Ajax for radio buttons.
      // See https://www.drupal.org/project/drupal/issues/2758631.
      $queue_selection = [
        '#type' => 'details',
        '#title' => 'Queue Selection',
        'type' => [
          '#type' => 'radios',
          '#title' => 'Select Queue',
          '#required' => TRUE,
          '#options' => $queue_types,
          '#default_value' => $queue_type,
          '#description' => 'Choose the review queue to view.',
        ],
      ];
    }
    $topics = ['' => 'Select a board'];
    $state = self::STATES[$queue_type];
    if (!empty($board)) {
      $topics = $this->getTopics($board, $state);
    }
    ebms_debug_log('topics: ' . print_r($topics, TRUE), 3);
    if (!empty($topic)) {
      if (!empty($topics)) {
        $topic_ids = array_keys($topics);
        if (!is_numeric($topic_ids[0])) {
          $ids = [];
          foreach ($topic_ids as $key) {
            foreach (array_keys($topics[$key]) as $id) {
              $ids[] = $id;
            }
          }
          $topic_ids = $ids;
        }
      }
      foreach ($topic as $topic_id) {
        if (!in_array($topic_id, $topic_ids)) {
          $topic = [];
          break;
        }
      }
    }
    $tags = [];
    $storage = $this->entityTypeManager->getStorage('taxonomy_term');
    $query = $storage->getQuery()->accessCheck(FALSE);
    $query->condition('vid', 'article_tags');
    $query->sort('name');
    $entities = $storage->loadMultiple($query->execute());
    foreach ($entities as $entity) {
      $tags[$entity->id()] = $entity->getName();
    }
    $sorts = [
      'state.article' => 'EBMS ID #',
      'article.source_id' => 'PMID #',
      'author' => 'Author',
      'article.title' => 'Title',
      'article.journal_title' => 'Journal',
      'article.year' => 'Publication Date',
      'core' => 'Core Journals',
    ];

    // Assemble the articles to be reviewed if we're not in an Ajax callback.
    // Original implementation used the entity query API, and with a full
    // data set it took minutes for the queue page to come up. Now it takes
    // a second or less.
    $queue = [];
    if (empty($values)) {
      $query = \Drupal::database()->select('ebms_state', 'state');
      $query->condition('state.value', State::getStateId($state));
      $query->condition('state.current', 1);
      if ($sort === 'state.article') {
        $have_article_join = FALSE;
      }
      else {
        $query->join('ebms_article', 'article', 'article.id = state.article');
        $have_article_join = TRUE;
      }
      if (!empty($topic)) {
        $query->condition('state.topic', $topic, 'IN');
      }
      elseif (!empty($board)) {
        $query->condition('state.board', $board);
      }
      if ($queue_type === 'Full Text Review') {
        if (!$have_article_join) {
          $query->join('ebms_article', 'article', 'article.id = state.article');
          $have_article_join = TRUE;
        }
        $query->isNotNull('article.full_text__file');
      }
      if ($queue_type === 'Librarian Review') {
        if (!empty($title)) {
          if (!$have_article_join) {
            $query->join('ebms_article', 'article', 'article.id = state.article');
            $have_article_join = TRUE;
          }
          $query->condition('article.title', "%$title%", 'LIKE');
        }
        if (!empty($journal)) {
          if (!$have_article_join) {
            $query->join('ebms_article', 'article', 'article.id = state.article');
            $have_article_join = TRUE;
          }
          $query->condition('article.brief_journal_title', "%$journal%", 'LIKE');
        }
      }
      else {
        if (!empty($cycle)) {
          $query->join('ebms_article__topics', 'topics', 'topics.entity_id = state.article');
          $query->join('ebms_article_topic', 'topic', 'topic.id = topics.topics_target_id');
          $query->condition('topic.cycle', $cycle);
        }
        if (!empty($tag)) {
          if (empty($cycle)) {
            $query->join('ebms_article__topics', 'topics', 'topics.entity_id = state.article');
          }
          $query->leftJoin('ebms_article__tags', 'article_tags', 'article_tags.entity_id = state.article');
          $query->leftJoin('ebms_article_tag', 'article_tag', 'article_tag.id = article_tags.tags_target_id');
          $query->leftJoin('ebms_article_topic__tags', 'topic_tags', 'topic_tags.entity_id = topics.topics_target_id');
          $query->leftJoin('ebms_article_tag', 'topic_tag', 'topic_tag.id = topic_tags.tags_target_id');
          $group = $query->orConditionGroup();
          $group->condition('article_tag.tag', $tag);
          $group->condition('topic_tag.tag', $tag);
          $query->condition($group);
        }
      }
      $count_query = $query->countQuery();
      $count = $count_query->execute()->fetchField();
      if ($sort === 'core') {
        $query->leftJoin('ebms_journal', 'journal', 'journal.source_id = article.source_journal_id');
        $query->orderBy('journal.core', 'DESC');
        $query->orderBy('article.journal_title');
        $query->orderBy('article.title');
      }
      elseif ($sort === 'author') {
        $query->leftJoin('ebms_article__authors', 'author', 'author.entity_id = article.id AND author.delta = 0');
        $query->orderBy('author.authors_search_name');
        $query->orderBy('article.title');
      }
      else {
        if (!array_key_exists($sort, $sorts)) {
          $sort = 'state.article';
        }
        $query->orderBy($sort);
      }
      $query->addField('state', 'article');
      $query = $query->extend(PagerSelectExtender::class);
      $query->limit($per_page);
      $ids = $query->execute()->fetchCol();
      $articles = Article::loadMultiple($ids);
      ebms_debug_log("$per_page articles loaded");
      $items = [];
      foreach ($articles as $article) {
        $items[] = $this->createArticleRenderArray($article, $queue_id, $decisions, $format, $state);
      }
      $page = $this->getRequest()->get('page');
      $start = 1 + $page * $per_page;
      $queue = [
        '#theme' => 'item_list',
        '#title' => "Articles Waiting for $queue_type ($count)",
        '#empty' => 'No articles match the filtering criteria.',
        '#list_type' => 'ol',
        '#items' => $items,
        '#attributes' => ['start' => $start],
      ];
    }

    // Assemble and return the form. We use a text field for tracking updates
    // to the queued decision actions because hidden fields don't trigger
    // change events. We hide the text field ourselves.
    return [
      '#title' => "$queue_type Queue",
      '#cache' => ['max-age' => 0],
      '#attached' => [
        'library' => ['ebms_review/review-queue'],
      ],
      'queue-id' => [
        '#type' => 'hidden',
        '#value' => $queue_id,
      ],
      'decisions' => [
        '#type' => 'textfield',
        '#value' => $decisions_json,
        '#ajax' => [
          'callback' => '::decisionsCallback',
          'event' => 'change',
          'wrapper' => 'queued-decisions-list',
        ],
        '#attributes' => ['class' => ['hidden'], 'maxlength' => ''],
      ],
      'queue-selection' => $queue_selection,
      'filters' => [
        '#type' => 'details',
        '#title' => 'Filter Options',
        'board' => [
          '#type' => 'select',
          '#title' => 'Editorial Board',
          '#description' => 'Select a board to populate the Topic picklist.',
          '#options' => $boards,
          '#default_value' => $board,
          '#empty_value' => '',
          '#ajax' => [
            'callback' => '::getTopicsCallback',
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
            '#description' => 'Limit queue to articles with these topics.',
            '#options' => $topics,
            '#default_value' => $topic,
            '#multiple' => TRUE,
            '#validated' => TRUE,
          ],
        ],
        'cycle' => [
          '#type' => 'select',
          '#title' => 'Review Cycle',
          '#options' => $cycles,
          '#description' => 'Include articles assigned to at least one reviewable topic for this review cycle.',
          '#default_value' => $cycle,
          '#empty_value' => '',
          '#states' => [
            'visible' => [
              ':input[name="type"]' => [
                ['value' => 'Abstract Review'],
                'or',
                ['value' => 'Full Text Review'],
                'or',
                ['value' => 'On Hold Review'],
              ],
            ],
          ],
        ],
        'tag' => [
          '#type' => 'select',
          '#title' => 'Tag',
          '#options' => $tags,
          '#description' => 'Include articles to which this tag has been assigned.',
          '#default_value' => $tag,
          '#empty_value' => '',
          '#states' => [
            'visible' => [
              ':input[name="type"]' => [
                ['value' => 'Abstract Review'],
                'or',
                ['value' => 'Full Text Review'],
                'or',
                ['value' => 'On Hold Review'],
              ],
            ],
          ],
        ],
        'title' => [
          '#type' => 'textfield',
          '#title' => 'Title',
          '#description' => 'Limit to articles with this title fragment.',
          '#default_value' => $title,
          '#states' => [
            'visible' => [
              ':input[name="type"]' => ['value' => 'Librarian Review'],
            ],
          ],
        ],
        'journal' => [
          '#type' => 'textfield',
          '#title' => 'Journal Short Title',
          '#default_value' => $journal,
          '#description' => 'Limit to articles whose abbreviated journal title contain this fragment.',
          '#states' => [
            'visible' => [
              ':input[name="type"]' => ['value' => 'Librarian Review'],
            ],
          ],
        ],
      ],
      'display-options' => [
        '#type' => 'details',
        '#title' => 'Display Options',
        'sort' => [
          '#type' => 'select',
          '#title' => 'Sort By',
          '#options' => $sorts,
          '#default_value' => $sort,
          '#description' => 'Select the element to be used for ordering the review queue.',
        ],
        'format' => [
          '#type' => 'radios',
          '#title' => 'Format',
          '#options' => [
            'brief' => 'Brief',
            'abstract' => 'Abstract',
          ],
          '#default_value' => $format,
          '#description' => 'Choose whether to include the article abstracts in the queue display.',
        ],
        'per-page' => [
          '#type' => 'select',
          '#title' => 'Articles Per Page',
          '#options' => [10 => 10, 25 => 25, 50 => 50, 100 => 100],
          '#default_value' => $per_page,
          '#description' => 'Decide how many articles to display on each page.',
        ],
        'apply-options' => [
          '#type' => 'submit',
          '#value' => 'Apply Display Options',
        ],
        'preserve-options' => [
          '#type' => 'submit',
          '#value' => 'Preserve Display Options',
        ],
      ],
      'submit' => [
        '#type' => 'submit',
        '#value' => 'Filter',
      ],
      'cancel' => [
        '#type' => 'submit',
        '#value' => 'Reset',
        '#submit' => ['::resetSubmit'],
        '#limit_validation_errors' => [],
      ],
      'apply-decisions' => [
        '#type' => 'submit',
        '#value' => 'Apply Queued Decisions',
        '#states' => [
          'invisible' => [
            ':input[name="decisions"]' => ['value' => '{}'],
          ],
        ],
      ],
      'queued-decisions-list' => [
        '#type' => 'container',
        '#attributes' => ['id' => 'queued-decisions-list'],
        'decisions-list' => [
          '#theme' => 'item_list',
          '#title' => 'Queued Decisions',
          '#empty' => 'No decisions have been queued.',
          '#list_type' => 'ul',
          '#items' => $decision_items,
        ],
      ],
      'queue' => $queue,
      'pager' => [
        '#type' => 'pager',
      ],
      'apply-decisions-bottom' => [
        '#type' => 'submit',
        '#value' => 'Apply Queued Decisions',
        '#states' => [
          'invisible' => [
            ':input[name="decisions"]' => ['value' => '{}'],
          ],
        ],
      ],
    ];
  }

  /**
   * {@inheritdoc}
   */
  public function submitForm(array &$form, FormStateInterface $form_state) {

    $trigger = $form_state->getTriggeringElement()['#value'];
    if ($trigger === 'Filter' || $trigger === 'Apply Queued Decisions') {
      if ($trigger === 'Apply Queued Decisions') {
        $uid = $this->currentUser()->id();
        $now = date('Y-m-d H:i:s');
        $queue_type = $form_state->getValue('type');
        $decisions_json = $form_state->getUserInput()['decisions'] ?? '{}';
        $decisions = json_decode($decisions_json, TRUE);
        foreach ($decisions as $key => $decision) {
          preg_match('/topic-action-(\d+)\|(\d+)/', $key, $matches);
          $article_id = $matches[1];
          $topic_id = $matches[2];
          $state = self::DECISION_STATES[$queue_type][$decision];
          $article = Article::load($article_id);
          $article->addState($state, $topic_id, $uid, $now);
          $article->save();
          $this->messenger()->addMessage('Queued decisions have been applied.');
        }
      }
      $queue = $this->createQueue($form_state);
      $route = 'ebms_review.review_queue';
      $parameters = ['queue_id' => $queue->id()];
      $form_state->setRedirect($route, $parameters);
    }
    if ($trigger === 'Apply Display Options' || $trigger === 'Preserve Display Options') {
      $queue_id = $form_state->getValue('queue-id');
      $sort = $form_state->getValue('sort');
      $format = $form_state->getValue('format');
      $per_page = $form_state->getValue('per-page');
      $queue = SavedRequest::load($queue_id);
      $params = $queue->getParameters();
      $params['sort'] = $sort;
      $params['format'] = $format;
      $params['per-page'] = $per_page;
      $params_json = json_encode($params);
      $queue->set('parameters', $params_json);
      $queue->save();
      if ($trigger === 'Preserve Display Options') {
        $user = User::load($this->currentUser()->id());
        $user->set('review_format', $format);
        $user->set('review_per_page', $per_page);
        $user->set('review_sort', $sort);
        $user->save();
      }
    }
  }

  /**
   * Create a fresh form.
   */
  public function resetSubmit(array &$form, FormStateInterface $form_state) {
    $form_state->setRedirect('ebms_review.review_queue');
  }

  /**
   * Replace the block for the topics to be displayed on the form.
   */
  public function getTopicsCallback(array &$form, FormStateInterface $form_state): array {
    return $form['filters']['board-controlled'];
  }

  /**
   * Preserve the updated change in the queued review decisions.
   *
   * We have to pull the queued decisions directly from the field because
   * Drupal hasn't yet updated the processed values array.
   *
   * @param array $form
   *   An associative array containing the structure of the form.
   * @param \Drupal\Core\Form\FormStateInterface $form_state
   *   The object tracking the current state of the form.
   *
   * @return array
   *   AJAX response.
   */
  public function decisionsCallback(array &$form, FormStateInterface $form_state): array {
    $decisions_json = $form_state->getUserInput()['decisions'] ?? '{}';
    $queue_id = $form_state->getValue('queue-id');
    $queue = SavedRequest::load($queue_id);
    $params = $queue->getParameters();
    $params['decisions'] = $decisions_json;
    $params_json = json_encode($params);
    $queue->set('parameters', $params_json);
    $queue->save();
    $items = $this->getQueuedDecisionsListItems($decisions_json);
    $form['queued-decisions-list']['decisions-list']['#items'] = $items;
    return $form['queued-decisions-list'];
  }

  /**
   * Create a new entity for tracking the current review queue.
   *
   * We use the `SavedRequest` entity type for tracking the
   * parameters used for querying the articles to be placed in
   * the queue, as well as the decision actions which the user
   * has made.
   *
   * As was true back in Drupal 7, you only get a partially populated
   * user object with `currentUser()`. To get all the fields, you
   * need to call `User::load()`.
   *
   * @param \Drupal\Core\Form\FormStateInterface $form_state
   *   The object tracking the current state of the form.
   *
   * @return \Drupal\ebms_core\Entity\SavedRequest
   *   The newly created entity for tracking the current queue.
   */
  private function createQueue(FormStateInterface $form_state): SavedRequest {
    $user = User::load($this->currentUser()->id());
    $default_sort = $user->review_sort->value;
    $default_format = $user->review_format->value;
    $default_per_page = $user->review_per_page->value;
    $default_sort = $default_sort ?: 'state.article';
    $default_format = $default_format ?: 'brief';
    $default_per_page = $default_per_page ?: self::DEFAULT_PAGE_SIZE;
    $default_queue_type = NULL;
    foreach (self::DEFAULT_QUEUES as $permission => $queue_type) {
      if ($user->hasPermission($permission)) {
        $default_queue_type = $queue_type;
        break;
      }
    }
    if (empty($default_queue_type)) {
      $queue_types = $this->getQueueTypes();
      $default_queue_type = reset($queue_types);
    }
    $parameters = $form_state->getValues();
    ebms_debug_log(print_r($parameters['topic'], TRUE), 3);
    $spec = [
      'board' => $parameters['board'] ?? Board::defaultBoard($user),
      'topic' => $parameters['topic'] ?? [],
      'cycle' => $parameters['cycle'] ?? '',
      'tag' => $parameters['tag'] ?? 0,
      'title' => $parameters['title'] ?? '',
      'journal' => $parameters['journal'] ?? '',
      'sort' => $parameters['sort'] ?? $default_sort,
      'format' => $parameters['format'] ?? $default_format,
      'per-page' => $parameters['per-page'] ?? $default_per_page,
      'decisions' => '{}',
      'type' => $parameters['type'] ?? $default_queue_type,
      'form_id' => 'ebms_review_queue',
    ];
    return SavedRequest::saveParameters('review queue', $spec);
  }

  /**
   * Assemble the values for displaying one article in the review queue.
   *
   * Note that there is a bug in the Drupal core rendering software, which
   * breaks form elements with '#type' = 'radios' (or 'checkboxes') when they
   * are nested inside other render arrays, so we have to use the #children
   * property instead of the #options property the documentation tells us to
   * use. See https://www.drupal.org/project/drupal/issues/3246825. It's
   * possible that if/when the bug gets fixed the fix might break our
   * workaround, in which case we're go back to setting the '#options
   * property the way we should have been able to do all along.
   *
   * We use Javascript to attach change listeners on each of the radio
   * buttons. The listeners update the hidden 'decisions' text field each
   * time a decision button is clicked and then trigger a change event on
   * the text field so the AJAX callback attached to that field is invoked,
   * updating the entity which tracks information about the current queue.
   *
   * @param \Drupal\ebms_article\Entity\Article $article
   *   The entity object for the article whose information is to be rendered.
   * @param int $queue_id
   *   Optional ID to get us back to this review queue.
   * @param array $decisions
   *   Cached decisions carried across paging and other requests.
   * @param string $format
   *   Used to determine whether to show the article's abstract.
   * @param string $state
   *   Used to determine which decisions actions to show.
   *
   * @return array
   *   The keyed values for rendering this article.
   */
  private function createArticleRenderArray(Article $article, int $queue_id, array $decisions, string $format, string $state): array {

    if ($state === 'published' || empty($article->full_text->file)) {
      $full_text_url = NULL;
    }
    else {
      $file = File::load($article->full_text->file);
      $full_text_url = $file->createFileUrl();
    }

    $article_id = $article->id();
    $authors = $article->getAuthors(10);
    $authors = implode(', ', $authors);
    $title = $article->title->value;
    $route = 'ebms_article.article';
    $parameters = ['article' => $article_id];
    $query = $this->getRequest()->query->all();
    $query['queue'] = $queue_id;
    if ($state === 'passed_bm_review') {
      $actions = self::DECISIONS;
    }
    else {
      $actions = array_slice(self::DECISIONS, 0, 3);
    }
    $topics = [];

    // Walk through each of the article's topics.
    foreach ($article->topics as $article_topic) {

      // Only show topics whose current state belongs to this review queue.
      $article_topic = $article_topic->entity;
      $topic_state = $article_topic->getCurrentState();
      if ($topic_state->value->entity->field_text_id->value !== $state) {
        continue;
      }
      $topic = $article_topic->topic->entity;
      $topic_id = $topic->id();
      $board_id = $topic->board->target_id;
      $show_buttons = ($state === 'ready_init_review') || $this->canReviewAllTopics;
      if (!$show_buttons) {
        if (in_array($topic_id, $this->userTopics)) {
          $show_buttons = TRUE;
        }
        elseif (in_array($board_id, $this->userBoards)) {
          $show_buttons = TRUE;
        }
      }
      $field_name = "topic-action-$article_id|$topic_id";
      $checked = (int) ($decisions[$field_name] ?? '0');
      $tags = [];
      foreach ($article_topic->tags as $topic_tag) {
        if (!empty($topic_tag->entity->active->value)) {
          $tags[] = $topic_tag->entity->tag->entity->name->value;
        }
      }
      $tags = implode(', ', $tags);
      $buttons = [];
      if ($show_buttons) {
        foreach ($actions as $value => $label) {
          $buttons[] = [
            'id' => "$field_name-$value",
            'name' => $field_name,
            'label' => $label,
            'value' => $value,
            'checked' => $checked === $value,
          ];
        }
      }
      $article_parm = ['article_id' => $article_id];
      $query_copy = $query;
      $query_copy['topic'] = $topic_id;
      $options = ['query' => $query_copy];
      $add_tag_url = Url::fromRoute('ebms_article.add_article_tag', $article_parm, $options);
      $comments = [];
      foreach ($article_topic->comments as $topic_comment) {
        $comments[] = $topic_comment->comment;
      }
      $topics[] = [
        'name' => $topic->name->value,
        'board' => $topic->board->entity->name->value,
        'buttons' => $buttons,
        'tags' => $tags,
        'add_tag_url' => $add_tag_url,
        'comments' => $comments,
      ];
    }
    usort($topics, function ($a, $b): int {
      if (empty($a['buttons']) && !empty($b['buttons'])) {
        return 1;
      }
      elseif (!empty($a['buttons']) && empty($b['buttons'])) {
        return -1;
      }
      return $a['name'] <=> $b['name'];
    });
    $tags = [];
    foreach ($article->tags as $article_tag) {
      if (!empty($article_tag->entity->active->value)) {
        $tags[] = $article_tag->entity->tag->entity->name->value;
      }
    }
    sort($tags);
    $tags = implode(', ', $tags);
    $types = [];
    foreach ($article->types as $type) {
      $types[] = $type->value;
    }
    if (!empty($types)) {
      $ancestor_set = [];
      foreach ($types as $type) {
        $ancestors = $this->pubTypeHierarchy[strtolower($type)] ?? [];
        foreach ($ancestors as $ancestor) {
          $ancestor_set[$ancestor] = TRUE;
        }
      }
      $filtered_types = [];
      foreach ($types as $type) {
        $key = strtolower($type);
        if (!array_key_exists($key, $ancestor_set)) {
          if ($key !== 'journal article') {
            $filtered_types[] = $type;
          }
        }
      }
      $types = $filtered_types;
    }
    sort($types);
    $types = implode(', ', $types);
    $options = ['query' => $query];
    $abstract = [];
    $abstract_url = NULL;
    if ($format === 'abstract') {
      foreach ($article->abstract as $paragraph) {
        $abstract[] = [
          'label' => $paragraph->paragraph_label,
          'text' => $paragraph->paragraph_text,
        ];
      }
    }
    else {
      $abstract_parms = ['article' => $article_id];
      $abstract_url = Url::fromRoute('ebms_article.show_abstract', $abstract_parms);
    }
    $article_parm = ['article_id' => $article_id];
    $add_topic_url = Url::fromRoute('ebms_review.add_review_topic', $article_parm, $options);
    $add_tag_url = Url::fromRoute('ebms_article.add_article_tag', $article_parm, $options);
    return [
      '#theme' => 'review_queue_article',
      '#article' => [
        'authors' => $authors,
        'link' => Link::createFromRoute($title, $route, $parameters, $options),
        'pmid' => $article->source_id->value,
        'ebms_id' => $article_id,
        'legacy_id' => $article->legacy_id->value,
        'publication' => $article->getLabel(),
        'tags' => $tags,
        'types' => $types,
        'abstract' => $abstract,
        'abstract_url' => $abstract_url,
        'full_text_url' => $full_text_url,
        'add_article_tag_url' => $add_tag_url,
        'add_topic_url' => $add_topic_url,
      ],
      '#topics' => $topics,
    ];
  }

  /**
   * Find out which queue the user is authorized for.
   */
  private function getQueueTypes(): array {
    $user = User::load($this->currentUser()->id());
    $queue_types = [];
    foreach (self::STATE_PERMISSIONS as $type => $permission) {
      if ($user->hasPermission($permission)) {
        $queue_types[$type] = $type;
      }
    }
    if (empty($queue_types)) {
      throw new AccessDeniedException('not an authorized reviewer');
    }
    return $queue_types;
  }

  /**
   * Fetch the array of topics for this board.
   */
  private function getTopics(mixed $board, string $state): array {

    // Load the `Topic` entities which belong to this board.
    ebms_debug_log('top of ReviewQueue::getTopics()');
    $storage = $this->entityTypeManager->getStorage('ebms_topic');
    $query = $storage->getQuery()->accessCheck(FALSE);
    $operator = is_array($board) ? 'IN' : '=';
    $query->condition('board', $board, $operator);
    $query->sort('name');
    $topic_ids = $query->execute();
    $topics = $storage->loadMultiple($topic_ids);
    ebms_debug_log('fetched ' . count($topics) . ' topics');

    // Find the topics for which the current user is the NCI reviewer.
    $uid = $this->currentUser()->id();
    $query = $storage->getQuery()->accessCheck(FALSE);
    $query->condition('nci_reviewer', $uid);
    $reviewer_topics = $query->execute();

    // Build an indexed array with article counts appended to the names.
    // This was originally implemented using the entity query API. That
    // worked OK in the version which just loads up a small set of sample
    // test data for development, but when the production data was loaded
    // This took over a minute for the boards with the smallest number of
    // topics, and three minutes for the boards with lots of topics. So we
    // talk directly to the database (or rather, to the database API), and
    // now this step takes around 5 seconds.
    ebms_debug_log('fetched the integer state ID');
    $query = \Drupal::database()->select('ebms_state', 'state');
    $query->condition('state.value', State::getStateId($state));
    $query->condition('state.topic', $topic_ids, 'IN');
    $query->condition('state.current', 1);
    $query->addExpression('COUNT(*)', 'count');
    $query->addField('state', 'topic');
    $query->groupBy('state.topic');
    if ($state !== 'published') {
      $query->join('ebms_article', 'article', 'article.id = state.article');
      $query->isNotNull('article.full_text__file');
    }
    $rows = $query->execute();
    $counts = [];
    foreach ($rows as $row) {
      $counts[$row->topic] = $row->count;
    }
    ebms_debug_log('fetched counts for ' . count($counts) . ' topics');

    // Assemble the picklist.
    $options = [0 => 'Select a board'];
    $my_topics = [];
    foreach ($topics as $topic) {
      if (in_array('Select a board', $options)) {
        $options = [];
      }
      $tid = $topic->id();
      if (in_array($tid, $reviewer_topics)) {
        $my_topics[] = $tid;
      }
      $count = $counts[$tid] ?? 0;
      $name = $topic->getName();
      $options[$tid] = "$name ($count)";
    }
    ebms_debug_log('assembled the topics picklist');

    // We only divide the picklist into "Mine" and "Other" for the abstract
    // review, and only when the current user is the NCI reviewer for some,
    // but not all the topics.
    if ($state !== 'published' || empty($my_topics) || count($my_topics) == count($options)) {
      return $options;
    }
    $my_count = $other_count = 0;
    $mine = [];
    $other = [];
    foreach ($options as $id => $name) {
      if (in_array($id, $my_topics)) {
        $mine[$id] = $name;
        $my_count += $counts[$id];
      }
      else {
        $other[$id] = $name;
        $other_count += $counts[$id];
      }
    }
    ebms_debug_log('collected the topics into "mine" and "others" piles');
    return [
      "My Topics ($my_count)" => $mine,
      "Other Topics ($other_count)" => $other,
    ];
  }

  /**
   * Collect the strings to display decisions waiting to be saved.
   */
  private function getQueuedDecisionsListItems($decisions_json): array {
    $items = [];
    if ($decisions_json !== '{}') {
      $decisions = json_decode($decisions_json, TRUE);
      foreach ($decisions as $key => $value) {
        preg_match('/topic-action-(\d+)\|(\d+)/', $key, $matches);
        $article_id = $matches[1];
        $topic_id = $matches[2];
        $topic = Topic::load($topic_id);
        $topic_name = $topic->getName();
        $decision = strtolower(self::DECISIONS[$value] ?? 'Unrecognized Decision');
        if ($decision === 'fyi') {
          $decision = 'marked as FYI';
        }
        $items[] = "Article $article_id $decision for $topic_name";
      }
    }
    return $items;
  }

}
