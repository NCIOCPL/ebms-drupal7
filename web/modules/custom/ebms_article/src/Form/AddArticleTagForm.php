<?php

namespace Drupal\ebms_article\Form;

use Drupal\Core\Entity\EntityTypeManagerInterface;
use Drupal\Core\Form\FormBase;
use Drupal\Core\Form\FormStateInterface;
use Drupal\Core\Session\AccountProxyInterface;
use Symfony\Component\DependencyInjection\ContainerInterface;

/**
 * Form controller for adding a tag to an article.
 *
 * @ingroup ebms
 */
class AddArticleTagForm extends FormBase {

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
  public static function create(ContainerInterface $container): AddArticleTagForm {
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
    return 'ebms_add_article_tag_form';
  }

  /**
   * {@inheritdoc}
   */
  public function buildForm(array $form, FormStateInterface $form_state, int $article_id = 0): array {

    $form_state->setValue('article', $article_id);
    $storage = $this->entityTypeManager->getStorage('ebms_article');
    $article = $storage->load($article_id);
    $label = $article->getLabel();

    // Build the picklist for available tags.
    $storage = $this->entityTypeManager->getStorage('taxonomy_term');
    $query = $storage->getQuery()->condition('vid', 'article_tags');
    $topic_id = $this->getRequest()->get('topic');
    if (empty($topic_id)) {
      $query->condition('field_topic_required', FALSE);
      $tag_desc = 'Select tag to be assigned.';
    }
    else {
      $query->condition('field_topic_allowed', TRUE);
      $topic_storage = $this->entityTypeManager->getStorage('ebms_topic');
      $topic = $topic_storage->load($topic_id);
      $topic_name = $topic->getName();
      $tag_desc = "Select tag to be assigned for topic '$topic_name'.";
    }
    $query->sort('name');
    $terms = $storage->loadMultiple($query->execute());
    $tags = [];
    foreach ($terms as $term) {
      $tags[$term->field_text_id->value] = $term->getName();
    }

    return [
      'article' => [
        '#type' => 'hidden',
        '#value' => $article_id,
      ],
      '#title' => 'Assign Tag',
      'article-info' => [
        '#theme' => 'article_info',
        '#title' => $article->title->value,
        '#citation' => $label,
      ],
      'tag' => [
        '#type' => 'select',
        '#title' => 'Tag',
        '#description' => $tag_desc,
        '#options' => $tags,
        '#required' => TRUE,
      ],
      'comment' => [
        '#type' => 'textfield',
        '#title' => 'Tag Comment',
        '#description' => 'Optionally add a comment to be stored with the new tag.',
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
    $queue_id = $this->getRequest()->get('queue');
    if (!empty($queue_id)) {
      $this->returnToQueue($queue_id, $form_state);
    }
    else {
      $article_id = $this->getRequest()->get('article');
      $this->returnToArticle($article_id, $form_state);
    }
  }

  /**
   * {@inheritdoc}
   */
  public function submitForm(array &$form, FormStateInterface $form_state) {
    $parameters = $form_state->getValues();
    $tag_id = $parameters['tag'];
    $uid = $this->account->id();
    $now = date('Y-m-d H:i:s');
    $comment_text = trim($parameters['comment'] ?? '');
    $article_id = $form_state->getValue('article');

    // Load the article entity and add the tag.
    $topic_id = $this->getRequest()->get('topic');
    if (empty($topic_id)) {
      $topic_id = 0;
    }
    $storage = $this->entityTypeManager->getStorage('ebms_article');
    $article = $storage->load($article_id);
    $article->addTag($tag_id, $topic_id, $uid, $now, $comment_text);
    $this->messenger()->addMessage('Tag successfully added.');
    $article->save();

    // If we got here from a review queue, that's where we'll return.
    $queue_id = $this->getRequest()->get('queue');
    if (!empty($queue_id)) {
      $this->returnToQueue($queue_id, $form_state);
    }
    else {
      // Back to the article page.
      $this->returnToArticle($article_id, $form_state);
    }
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
    if (!empty($options['query']['topic'])) {
      unset($options['query']['topic']);
    }
    $form_state->setRedirect($route, $parameters, $options);
  }

  /**
   * Return directly to the queue page.
   *
   * @param int $queue_id
   *   ID of queue search criteria for page to which we are returning.
   * @param \Drupal\Core\Form\FormStateInterface $form_state
   *   Form values.
   */
  private function returnToQueue(int $queue_id, FormStateInterface $form_state) {
    $route = 'ebms_review.review_queue';
    $parameters = ['queue_id' => $queue_id];
    $article_id = $this->getRequest()->get('article_id');
    $options = [
      'query' => $this->getRequest()->query->all(),
      'fragment' => "review-queue-article-$article_id",
    ];
    unset($options['query']['queue']);
    if (!empty($options['query']['topic'])) {
      unset($options['query']['topic']);
    }
    $form_state->setRedirect($route, $parameters, $options);
  }

}
