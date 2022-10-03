<?php

namespace Drupal\ebms_review\Form;

use Drupal\Core\Entity\EntityTypeManagerInterface;
use Drupal\Core\Form\FormBase;
use Drupal\Core\Form\FormStateInterface;
use Drupal\Core\Session\AccountProxyInterface;
use Drupal\ebms_article\Entity\Article;
use Drupal\ebms_core\Entity\SavedRequest;
use Symfony\Component\DependencyInjection\ContainerInterface;
use Symfony\Component\HttpKernel\Exception\BadRequestHttpException;

/**
 * Add a new topic to an article in the review queue.
 *
 * It was tempting to use the form we already have for adding a new topic
 * to an article, which is loaded with a button on the full article display
 * page, but this form doesn't need all the fields that one does, and
 * worse, it would have introduced a circular dependency. So we're creating
 * a second form for adding a topic to an article in the review queue, and
 * then returning to that queue.
 *
 * @ingroup ebms
 */
class AddReviewTopic extends FormBase {

  /**
   * State used to populate the queue for each review queue type.
   */
  const STATES = [
    'Librarian Review' => 'passed_init_review',
    'Abstract Review' => 'published',
    'Full Text Review' => 'passed_bm_review',
    'On Hold Review' => 'on_hold',
  ];

  /**
   * The current user account.
   *
   * @var \Drupal\Core\Session\AccountProxyInterface
   */
  protected AccountProxyInterface $account;

  /**
   * The entity type manager.
   *
   * @var \Drupal\Core\Entity\EntityTypeManagerInterface
   */
  protected EntityTypeManagerInterface $entityTypeManager;

  /**
   * {@inheritdoc}
   */
  public static function create(ContainerInterface $container): AddReviewTopic {
    // Instantiates this form class.
    $instance = parent::create($container);
    $instance->account = $container->get('current_user');
    $instance->entityTypeManager = $container->get('entity_type.manager');
    return $instance;
  }

  /**
   * {@inheritdoc}
   */
  public function getFormId(): string {
    return 'ebms_add_topic_for_review_form';
  }

  /**
   * {@inheritdoc}
   */
  public function buildForm(array $form, FormStateInterface $form_state, int $article_id = 0): array {

    // Load the article for which we are adding a topic.
    $article = Article::load($article_id);
    $label = $article->getLabel();

    // Fetch the queue's current tracking object.
    $queue_id = $this->getRequest()->get('queue');
    if (empty($queue_id)) {
      throw new BadRequestHttpException('missing queue ID parameter');
    }
    $params = SavedRequest::loadParameters($queue_id);
    $queue_type = $params['type'];
    $state = self::STATES[$queue_type];

    // @todo Refactor this into a service.
    $cycles = ['' => ''];
    $month = new \DateTime('first day of next month');
    $first = new \DateTime('2002-06-01');
    while ($month >= $first) {
      $cycles[$month->format('Y-m-d')] = $month->format('F Y');
      $month->modify('previous month');
    }

    // @todo Refactor this as a service (same for search form).
    $storage = $this->entityTypeManager->getStorage('ebms_board');
    $query = $storage->getQuery()->accessCheck(FALSE);
    $query->sort('name');
    $entities = $storage->loadMultiple($query->execute());
    $boards = [];
    foreach ($entities as $entity) {
      $id = $entity->id();
      $boards[$id] = $entity->getName();
    }
    $topics = ['' => 'Select a board'];
    $board = $form_state->getValue('board');
    if (!empty($board)) {
      $topics = $this->getTopics($board, $article_id);
    }
    $topic = $form_state->getValue('topic');
    if (!empty($topic) && !array_key_exists($topic, $topics)) {
      $topic = '';
    }
    return [
      'article' => [
        '#type' => 'hidden',
        '#value' => $article_id,
      ],
      'queue' => [
        '#type' => 'hidden',
        '#value' => $queue_id,
      ],
      'state' => [
        '#type' => 'hidden',
        '#value' => $state,
      ],
      '#title' => 'Assign Topic',
      'article-info' => [
        '#theme' => 'article_info',
        '#title' => $article->title->value,
        '#citation' => $label,
      ],
      'board' => [
        '#type' => 'select',
        '#title' => 'Board',
        '#description' => 'Select a board to populate the Topic picklist.',
        '#options' => $boards,
        '#required' => TRUE,
        '#default_value' => $board,
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
          '#title' => 'Topic',
          '#description' => 'Select topic to be assigned.',
          '#options' => $topics,
          '#validated' => TRUE,
          '#required' => TRUE,
          '#default_value' => $topic,
        ],
      ],
      'cycle' => [
        '#type' => 'select',
        '#title' => 'Review cycle',
        '#options' => $cycles,
        '#required' => TRUE,
        '#default_value' => $form_state->getValue('cycle'),
        '#description' => 'Select the review cycle for which this topic is assigned.',
      ],
      'submit' => [
        '#type' => 'submit',
        '#value' => 'Submit',
      ],
      'cancel' => [
        '#type' => 'submit',
        '#value' => 'Cancel',
        '#submit' => ['::cancelSubmit'],
        '#limit_validation_errors' => [],
      ],
    ];
  }

  /**
   * Package a fresh set of topics for the form.
   */
  public function getTopicsCallback(array &$form, FormStateInterface $form_state): array {
    $board = $form_state->getValue('board');
    $article_id = $form_state->getValue('article');
    $options = empty($board) ? ['' => 'Select a board'] : $this->getTopics($board, $article_id);
    $form['board-controlled']['topic']['#options'] = $options;
    return $form['board-controlled'];
  }

  /**
   * Find the topics links to the specified board.
   */
  private function getTopics(int $board_id, int $article_id): array {

    // Load the article entity.
    $storage = $this->entityTypeManager->getStorage('ebms_article');
    $article = $storage->load($article_id);
    $existing_topics = [];
    foreach ($article->topics as $article_topic) {
      $existing_topics[] = $article_topic->entity->topic->target_id;
    }
    $storage = $this->entityTypeManager->getStorage('ebms_topic');
    $query = $storage->getQuery()->accessCheck(FALSE);
    $query->condition('board', $board_id);
    $query->sort('name');
    $ids = $query->execute();
    $topics = $storage->loadMultiple($ids);
    $options = [0 => 'Select a board'];
    foreach ($topics as $topic) {
      if (in_array('Select a board', $options)) {
        $options = [0 => '- Select a topic -'];
      }
      if (!in_array($topic->id(), $existing_topics)) {
        $options[$topic->id()] = $topic->getName();
      }
    }
    if (count($options) === 1) {
      $options = [0 => '** All topics for this board have already been assigned. **'];
    }
    return $options;
  }

  /**
   * {@inheritdoc}
   */
  public function validateForm(array &$form, FormStateInterface $form_state) {
    $trigger = $form_state->getTriggeringElement()['#value'];
    if ($trigger === 'Submit') {
      $topic = $form_state->getValue('topic');
      if (empty($topic)) {
        $form_state->setErrorByName('topic', 'A topic must be selected.');
      }
    }
  }

  /**
   * Take the user back to the review queue page.
   */
  public function cancelSubmit(array &$form, FormStateInterface $form_state) {
    $queue_id = $this->getRequest()->get('queue');
    $this->returnToReview($queue_id, $form_state);
  }

  /**
   * {@inheritdoc}
   */
  public function submitForm(array &$form, FormStateInterface $form_state) {
    $parameters = $form_state->getValues();
    $state_id = $parameters['state'];
    $topic_id = $parameters['topic'];
    $uid = $this->account->id();
    $entered = date('Y-m-d H:i:s');
    $cycle = $parameters['cycle'];
    $queue_id = $form_state->getValue('queue');
    $article_id = $form_state->getValue('article');

    // Load the article entity.
    $storage = $this->entityTypeManager->getStorage('ebms_article');
    $article = $storage->load($article_id);
    $article->addState($state_id, $topic_id, $uid, $entered, $cycle);
    $article->save();

    // Tell the user what we did.
    $storage = $this->entityTypeManager->getStorage('ebms_topic');
    $topic = $storage->load($topic_id);
    $name = $topic->getName();
    $publication = $article->getLabel();
    $message = "Topic '$name' has been added to article $article_id ($publication).";
    $this->messenger()->addMessage($message);

    // Back to Sorrento.
    $this->returnToReview($queue_id, $form_state);
  }

  /**
   * Redirect the user to the review queue.
   */
  private function returnToReview($queue_id, FormStateInterface $form_state) {
    $route = 'ebms_review.review_queue';
    $parameters = ['queue_id' => $queue_id];
    $options = ['query' => $this->getRequest()->query->all()];
    $form_state->setRedirect($route, $parameters, $options);
  }

}
