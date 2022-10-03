<?php

namespace Drupal\ebms_article\Form;

use Drupal\Core\Entity\EntityTypeManagerInterface;
use Drupal\Core\Form\FormBase;
use Drupal\Core\Form\FormStateInterface;
use Drupal\Core\Session\AccountProxyInterface;
use Drupal\ebms_article\Entity\Article;
use Drupal\ebms_core\TermLookup;
use Symfony\Component\DependencyInjection\ContainerInterface;

/**
 * Form controller for adding a new state to an article.
 *
 * @ingroup ebms
 */
class NewStateForm extends FormBase {

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
   * The term_lookup service.
   *
   * @var \Drupal\ebms_core\TermLookup
   */
  protected TermLookup $termLookup;

  /**
   * {@inheritdoc}
   */
  public static function create(ContainerInterface $container): NewStateForm {
    // Instantiates this form class.
    $instance = parent::create($container);
    $instance->account = $container->get('current_user');
    $instance->entityTypeManager = $container->get('entity_type.manager');
    $instance->termLookup = $container->get('ebms_core.term_lookup');
    return $instance;
  }

  /**
   * {@inheritdoc}
   */
  public function getFormId(): string {
    return 'ebms_new_state_form';
  }

  /**
   * {@inheritdoc}
   */
  public function buildForm(array $form, FormStateInterface $form_state, int $article_id = 0, int $article_topic_id = 0): array {

    // Extract what we need from the article-topic entity.
    $description = 'New state for this topic.';
    $storage = $this->entityTypeManager->getStorage('ebms_article_topic');
    $article_topic = $storage->load($article_topic_id);
    $cycle = $article_topic->cycle->value;
    $topic = $article_topic->topic->entity->getName();
    $topic_id = $article_topic->topic->target_id;
    $board_id = $article_topic->topic->entity->board->target_id;
    foreach ($article_topic->states as $state) {
      if ($state->entity->current->value) {
        $name = $state->entity->value->entity->getName();
        $description .= " The current state is <em>$name</em>.";
        break;
      }
    }

    // Get the state options.
    $published = $this->termLookup->getState('published');
    $storage = $this->entityTypeManager->getStorage('taxonomy_term');
    $query = $storage->getQuery()->accessCheck(FALSE);
    $query->condition('vid', 'states');
    $query->condition('field_sequence', $published->field_sequence->value, '>');
    $query->sort('field_sequence');
    $query->sort('name');
    $entities = $storage->loadMultiple($query->execute());
    $states = [];
    foreach ($entities as $entity) {
      $states[$entity->field_text_id->value] = $entity->getName();
    }

    // Populate the decision picklist.
    $query = $storage->getQuery()->accessCheck(FALSE);
    $query->condition('vid', 'board_decisions');
    $query->sort('name');
    $entities = $storage->loadMultiple($query->execute());
    $decisions = [];
    $decision = '';
    foreach ($entities as $entity) {
      $decisions[$entity->id()] = $entity->getName();
      if (empty($decision)) {
        $decision = $entity->id();
      }
    }

    // Populate the meeting picklist.
    $now = date('c');
    $storage = $this->entityTypeManager->getStorage('ebms_meeting');
    $query = $storage->getQuery()->accessCheck(FALSE);
    $query->condition('published', 1);
    $group = $query->orConditionGroup()
      ->condition('boards', $board_id)
      ->condition('groups.entity.boards', $board_id);
    $query->condition($group);
    $query->sort('dates.value', 'DESC');
    $ids = $query->execute();
    $meetings = [];
    $have_placeholder = FALSE;
    $entities = $storage->loadMultiple($ids);
    foreach ($entities as $meeting) {
      $date = substr($meeting->dates->value, 0, 10);
      if (!$have_placeholder && strcmp($now, $date) > 0) {
        $meetings[] = '';
        $have_placeholder = TRUE;
      }
      $name = $meeting->name->value;
      $meetings[$meeting->id()] = "$name - $date";
    }
    if (!$have_placeholder) {
      $meetings[] = '';
    }

    // Get the users for this board.
    $users = [];
    $storage = $this->entityTypeManager->getStorage('user');
    $query = $storage->getQuery()->accessCheck(FALSE);
    $query->condition('boards', $board_id);
    $entities = $storage->loadMultiple($query->execute());
    foreach ($entities as $user) {
      $users[$user->id()] = $user->getDisplayName();
    }
    natcasesort($users);

    // Load the article entity.
    $article = Article::load($article_id);
    return [
      '#title' => "Add a new state for $topic",
      'article-info' => [
        '#theme' => 'article_info',
        '#title' => $article->title->value,
        '#citation' => $article->getLabel(),
      ],
      'article' => ['#type' => 'hidden', '#value' => $article_id],
      'cycle' => ['#type' => 'hidden', '#value' => $cycle],
      'topic' => ['#type' => 'hidden', '#value' => $topic_id],
      'state' => [
        '#type' => 'radios',
        '#title' => 'State',
        '#description' => $description,
        '#options' => $states,
        '#required' => TRUE,
      ],
      'meeting' => [
        '#type' => 'select',
        '#title' => 'Meeting',
        '#options' => $meetings,
        '#states' => [
          'visible' => [':input[name="state"]' => ['value' => 'on_agenda']],
          // 'required' => [':input[name="state"]' => ['value' => 'on_agenda']],
        ],
        '#default_value' => '',
      ],
      'decision' => [
        '#type' => 'radios',
        '#title' => 'Decision',
        '#options' => $decisions,
        '#states' => [
          'visible' => [':input[name="state"]' => ['value' => 'final_board_decision']],
          // This is broken, so we pick the first button as the default.
          // See https://www.drupal.org/project/drupal/issues/3267246.
          'required' => [':input[name="state"]' => ['value' => 'final_board_decision']],
          'default_value' => $decision,
        ],
      ],
      'users' => [
        '#type' => 'select',
        '#multiple' => TRUE,
        '#title' => 'Board Members',
        '#options' => $users,
        '#states' => [
          'visible' => [':input[name="state"]' => ['value' => 'final_board_decision']],
        ],
      ],
      'discussed' => [
        '#type' => 'checkbox',
        '#title' => 'Discussed?',
        '#states' => [
          'visible' => [':input[name="state"]' => ['value' => 'final_board_decision']],
        ],
      ],
      'comment' => [
        '#type' => 'textarea',
        '#title' => 'Comment',
        '#description' => 'Optional notes about the new state.',
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
   * Return directly to the article page.
   *
   * @param array $form
   *   Form settings.
   * @param \Drupal\Core\Form\FormStateInterface $form_state
   *   Form values.
   */
  public function cancelSubmit(array &$form, FormStateInterface $form_state) {
    $route_match = $this->getRouteMatch();
    $article_id = $route_match->getParameter('article_id');
    $this->returnToArticle($article_id, $form_state);
  }

  /**
   * {@inheritdoc}
   */
  public function submitForm(array &$form, FormStateInterface $form_state) {

    // Collect the form's values.
    $article_id = $form_state->getValue('article');
    $state_id = $form_state->getValue('state');
    $topic_id = $form_state->getValue('topic');
    $comment = $form_state->getValue('comment');

    // Load the article entity.
    $storage = $this->entityTypeManager->getStorage('ebms_article');
    $article = $storage->load($article_id);

    // Create the new state entity.
    $state = $article->addState($state_id, $topic_id, NULL, NULL, NULL, $comment);
    $article->save();

    // Add extra values needed, depending on the state.
    if ($state_id === 'final_board_decision') {
      $state->decisions[] = [
        'decision' => $form_state->getValue('decision'),
        'meeting_date' => $form_state->getValue('cycle'),
        'discussed' => (bool) $form_state->getValue('discussed'),
      ];
      $state->deciders = $form_state->getValue('users');
      $state->save();
    }
    elseif ($state_id === 'on_agenda') {
      $state->meetings[] = $form_state->getValue('meeting');
      $state->save();
    }

    // Back to the article page.
    $this->returnToArticle($article_id, $form_state);
  }

  /**
   * Redirect back to the article page.
   *
   * @param int $article_id
   *   ID of article entity to whose page we are returning.
   * @param \Drupal\Core\Form\FormStateInterface $form_state
   *   Object used to redirect back to the article's page.
   */
  private function returnToArticle(int $article_id, FormStateInterface $form_state) {
    $route = 'ebms_article.article';
    $parameters = ['article' => $article_id];
    $query = $this->getRequest()->query->all();
    $options = ['query' => $query];
    $form_state->setRedirect($route, $parameters, $options);
  }

}
