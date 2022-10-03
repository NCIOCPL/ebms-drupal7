<?php

namespace Drupal\ebms_article\Form;

use Drupal\Core\Entity\EntityTypeManagerInterface;
use Drupal\Core\Form\FormBase;
use Drupal\Core\Form\FormStateInterface;
use Drupal\Core\Session\AccountProxyInterface;
use Symfony\Component\DependencyInjection\ContainerInterface;

/**
 * Form controller for adding a comment to an article topic.
 *
 * @ingroup ebms
 */
class InternalCommentForm extends FormBase {

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
  public static function create(ContainerInterface $container): InternalCommentForm {
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
    return 'ebms_internal_comment_form';
  }

  /**
   * {@inheritdoc}
   */
  public function buildForm(array $form, FormStateInterface $form_state, int $article_id = 0): array {
    $storage = $this->entityTypeManager->getStorage('ebms_article');
    $article = $storage->load($article_id);

    // Delta is passed as 1-based, even though Drupal has them as 0-based,
    // because of PHP's very odd behavior with `empty("0")`.
    $delta = $this->getRequest()->get('delta');
    if (empty($delta)) {
      $title = 'Add internal comment';
      $comment = '';
    }
    else {
      $title = 'Edit internal comment';
      $comment = $article->internal_comments[$delta - 1]->body;
    }

    return [
      '#title' => $title,
      'article-info' => [
        '#theme' => 'article_info',
        '#title' => $article->title->value,
        '#citation' => $article->getLabel(),
      ],
      'article' => [
        '#type' => 'hidden',
        '#value' => $article->id(),
      ],
      'delta' => [
        '#type' => 'hidden',
        '#value' => $delta,
      ],
      'comment' => [
        '#type' => 'textarea',
        '#title' => 'Comment',
        '#description' => 'Provide notes about how this article may be of interest to internal PDQ staff.',
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
    $storage = $this->entityTypeManager->getStorage('ebms_article');
    $article_id = $this->getRequest()->get('article');
    $article = $storage->load($article_id);
    $delta = $form_state->getValue('delta');
    $body = trim($form_state->getValue('comment') ?? '');
    if (empty($delta)) {
      $article->internal_comments[] = [
        'user' => $this->account->id(),
        'entered' => date('Y-m-d H:i:s'),
        'body' => $body,
      ];
      $message = 'New internal comment successfully added.';
    }
    else {
      $delta--;
      $article->internal_comments[$delta]->entered = date('Y-m-d H:i:s');
      $article->internal_comments[$delta]->user = $this->account->id();
      $article->internal_comments[$delta]->body = $body;
      $message = 'Internal comment successfully updated.';
    }
    $article->save();
    $this->messenger()->addMessage($message);

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
    $options = ['query' => $this->getRequest()->query->all()];
    unset($options['query']['delta']);
    $form_state->setRedirect($route, $parameters, $options);
  }

}
