<?php

namespace Drupal\ebms_article\Form;

use Drupal\Core\Entity\EntityTypeManagerInterface;
use Drupal\Core\Form\FormBase;
use Drupal\Core\Form\FormStateInterface;
use Drupal\Core\Session\AccountProxyInterface;
use Drupal\ebms_board\Entity\Board;
use Drupal\ebms_import\Entity\Batch;
use Symfony\Component\DependencyInjection\ContainerInterface;

/**
 * Form controller for adding a new topic to an article.
 *
 * @ingroup ebms
 */
class AddArticleTopicForm extends FormBase {

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
   * Available initial states.
   *
   * @var array
   */
  protected array $states = [];

  /**
   * {@inheritdoc}
   */
  public static function create(ContainerInterface $container): AddArticleTopicForm {
    // Instantiates this form class.
    $instance = parent::create($container);
    $instance->account = $container->get('current_user');
    $instance->entityTypeManager = $container->get('entity_type.manager');
    $termLookup = $container->get('ebms_core.term_lookup');
    $states = [
      'published',
      'passed_bm_review',
      'passed_full_review',
      'fyi',
    ];
    foreach ($states as $text_id) {
      $state = $termLookup->getState($text_id);
      $instance->states[$text_id] = $state->getName();
    }
    return $instance;
  }

  /**
   * {@inheritdoc}
   */
  public function getFormId(): string {
    return 'ebms_add_article_topic_form';
  }

  /**
   * {@inheritdoc}
   */
  public function buildForm(array $form, FormStateInterface $form_state, int $article_id = 0): array {

    // dpm($article_id);
    $form_state->setValue('article', $article_id);
    $storage = $this->entityTypeManager->getStorage('ebms_article');
    $article = $storage->load($article_id);
    $label = $article->getLabel();
    $cycles = Batch::cycles();
    $boards = Board::boards();
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
      'topic-comment' => [
        '#type' => 'textfield',
        '#title' => 'Topic Comment',
        '#description' => 'Optionally add a comment to be stored with the new topic.',
      ],
      'cycle' => [
        '#type' => 'select',
        '#title' => 'Review cycle',
        '#options' => $cycles,
        '#required' => TRUE,
        '#description' => 'Select the review cycle for which this topic is assigned.',
      ],
      'state' => [
        '#type' => 'radios',
        '#title' => 'Initial State',
        '#options' => $this->states,
        '#description' => 'Pick an initial state for the new topic.',
        '#required' => TRUE,
        '#default_value' => 'published',
      ],
      'state-comment' => [
        '#type' => 'textfield',
        '#title' => 'State Comment',
        '#description' => 'Optionally add a comment to be stored with the initial state.',
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
   * Find the topics matching the currently-selected board.
   *
   * @param array $form
   *   Access to the existing values on the form.
   * @param \Drupal\Core\Form\FormStateInterface $form_state
   *   Access to the new values on the form.
   *
   * @return array
   *   Updated values for board-controlled portion of the form.
   */
  public function getTopicsCallback(array &$form, FormStateInterface $form_state): array {
    $board = $form_state->getValue('board');
    $article_id = $form_state->getValue('article');
    $options = empty($board) ? ['' => 'Select a board'] : $this->getTopics($board, $article_id);
    $form['board-controlled']['topic']['#options'] = $options;
    return $form['board-controlled'];
  }

  /**
   * Get the array of topics matching the current board.
   *
   * @param int $board_id
   *   ID of the currently-selected board.
   * @param int $article_id
   *   ID of the article whose tag we are editing.
   *
   * @return array
   *   Topics for this board, indexed by ID.
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
    $query->condition('active', TRUE);
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
   * Return directly to the article page.
   *
   * @param array $form
   *   Form settings.
   * @param \Drupal\Core\Form\FormStateInterface $form_state
   *   Form values.
   */
  public function cancelSubmit(array &$form, FormStateInterface $form_state) {
    // dpm('cancelSubmit()');
    $article_id = $this->getRequest()->get('article');
    $this->returnToArticle($article_id, $form_state);
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
    $comment = trim($parameters['state-comment'] ?? '');
    $cycle = $parameters['cycle'];
    if (empty($comment)) {
      $comment = NULL;
    }
    $article_id = $form_state->getValue('article');

    // Load the article entity.
    $storage = $this->entityTypeManager->getStorage('ebms_article');
    $article = $storage->load($article_id);
    $article->addState($state_id, $topic_id, $uid, $entered, $cycle, $comment);
    $article->save();

    // Add the topic comment, if there is one.
    $comment = trim($parameters['topic-comment'] ?? '');
    if (!empty($comment)) {
      $article_topic = $article->getTopic($topic_id);
      $article_topic->comments[] = [
        'comment' => $comment,
        'user' => $uid,
        'entered' => $entered,
      ];
      $article_topic->save();
    }

    // Tell the user what we did.
    $storage = $this->entityTypeManager->getStorage('ebms_topic');
    $topic = $storage->load($topic_id);
    $name = $topic->getName();
    $message = "Topic '$name' has been assigned to the article.";
    $this->messenger()->addMessage($message);

    // Back to Sorrento.
    $this->returnToArticle($article_id, $form_state);
  }

  /**
   * Return directly to the article page.
   *
   * @param int $article_id
   *   ID of article entity to whose page we are returning.
   * @param \Drupal\Core\Form\FormStateInterface $form_state
   *   Form values.
   */
  private function returnToArticle(int $article_id, FormStateInterface $form_state) {
    $route = 'ebms_article.article';
    $parameters = ['article' => $article_id];
    $options = ['query' => $this->getRequest()->query->all()];
    $form_state->setRedirect($route, $parameters, $options);
  }

}
