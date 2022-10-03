<?php

namespace Drupal\ebms_article\Form;

use Drupal\Core\Entity\EntityTypeManagerInterface;
use Drupal\Core\Form\FormBase;
use Drupal\Core\Form\FormStateInterface;
use Drupal\Core\Session\AccountProxyInterface;
use Drupal\ebms_article\Entity\Article;
use Symfony\Component\DependencyInjection\ContainerInterface;

/**
 * Form controller for adding a comment to an article topic.
 *
 * @ingroup ebms
 */
class ManagerCommentForm extends FormBase {

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
  public static function create(ContainerInterface $container): ManagerCommentForm {
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
    return 'ebms_manager_comment_form';
  }

  /**
   * {@inheritdoc}
   */
  public function buildForm(array $form, FormStateInterface $form_state, int $article_topic_id = 0): array {

    $storage = $this->entityTypeManager->getStorage('ebms_article_topic');
    $article_topic = $storage->load($article_topic_id);
    $topic = $article_topic->topic->entity;
    $topic_name = $topic->getName();
    $article_id = $this->getRequest()->get('article');
    $article = Article::load($article_id);

    // Delta is passed as 1-based, even though Drupal has them as 0-based,
    // because of PHP's very odd behavior with `empty("0")`.
    $delta = $this->getRequest()->get('delta');
    if (empty($delta)) {
      $title = "Post board manager comment for topic $topic_name";
      $comment = '';
    }
    else {
      $title = "Edit board manager comment for topic $topic_name";
      $comment = $article_topic->comments[$delta - 1]->comment;
    }

    return [
      'article-topic' => [
        '#type' => 'hidden',
        '#value' => $article_topic->id(),
      ],
      'topic' => [
        '#type' => 'hidden',
        '#value' => $topic_name,
      ],
      'delta' => [
        '#type' => 'hidden',
        '#value' => $delta,
      ],
      '#title' => $title,
      'article-info' => [
        '#theme' => 'article_info',
        '#title' => $article->title->value,
        '#citation' => $article->getLabel(),
      ],
      'comment' => [
        '#type' => 'textarea',
        '#title' => 'Comment',
        '#description' => 'This comment will be shown to board members in their literature packets.',
        '#required' => TRUE,
        '#default_value' => $comment,
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
    $article_id = $this->getRequest()->get('article');
    $this->returnToArticle($article_id, $form_state);
  }

  /**
   * {@inheritdoc}
   */
  public function submitForm(array &$form, FormStateInterface $form_state) {

    $article_topic_id = $form_state->getValue('article-topic');
    $storage = $this->entityTypeManager->getStorage('ebms_article_topic');
    $article_topic = $storage->load($article_topic_id);
    $delta = $form_state->getValue('delta');
    $body = $form_state->getValue('comment');
    if (empty($delta)) {
      $article_topic->comments[] = [
        'user' => $this->account->id(),
        'entered' => date('Y-m-d H:i:s'),
        'comment' => $body,
      ];
      $message = 'New comment successfully added.';
    }
    else {
      $delta--;
      $article_topic->comments[$delta]->modified = date('Y-m-d H:i:s');
      $article_topic->comments[$delta]->modified_by = $this->account->id();
      $article_topic->comments[$delta]->comment = $body;
      $message = 'Comment successfully updated.';
    }
    $article_topic->save();
    $this->messenger()->addMessage($message);

    // Back to the article page.
    $article_id = $this->getRequest()->get('article');
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
    $options = ['query' => $this->getRequest()->query->all()];
    unset($options['query']['topic']);
    unset($options['query']['delta']);
    $form_state->setRedirect($route, $parameters, $options);
  }

}
