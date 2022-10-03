<?php

namespace Drupal\ebms_article\Form;

use Drupal\Core\Entity\EntityTypeManagerInterface;
use Drupal\Core\Form\FormBase;
use Drupal\Core\Form\FormStateInterface;
use Drupal\Core\Session\AccountProxyInterface;
use Drupal\ebms_article\Entity\Article;
use Symfony\Component\DependencyInjection\ContainerInterface;

/**
 * Form controller for adding a comment to an article tag.
 *
 * @ingroup ebms
 */
class AddArticleTagCommentForm extends FormBase {

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
  public static function create(ContainerInterface $container): AddArticleTagCommentForm {
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
    return 'ebms_add_article_tag_comment_form';
  }

  /**
   * {@inheritdoc}
   */
  public function buildForm(array $form, FormStateInterface $form_state, int $tag_id = 0): array {

    $form_state->setValue('tag', $tag_id);
    $storage = $this->entityTypeManager->getStorage('ebms_article_tag');
    $article_tag = $storage->load($tag_id);
    $tag_name = $article_tag->get('tag')->entity->getName();
    $article_id = $this->getRequest()->get('article');
    $article = Article::load($article_id);

    return [
      'tag' => [
        '#type' => 'hidden',
        '#value' => $tag_id,
      ],
      '#title' => "Add Comment To '$tag_name' Tag",
      'article-info' => [
        '#theme' => 'article_info',
        '#title' => $article->title->value,
        '#citation' => $article->getLabel(),
      ],
      'comment' => [
        '#type' => 'textfield',
        '#title' => 'Tag Comment',
        '#description' => 'Enter a new comment to be stored with the tag.',
        '#required' => TRUE,
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

    // Save the new comment.
    $tag_id = $form_state->getValue('tag');
    $storage = $this->entityTypeManager->getStorage('ebms_article_tag');
    $article_tag = $storage->load($tag_id);
    $article_tag->comments[] = [
      'user' => $this->account->id(),
      'entered' => date('Y-m-d H:i:s'),
      'body' => $form_state->getValue('comment'),
    ];
    $article_tag->save();
    $this->messenger()->addMessage('New comment successfully added.');

    // If we got here from a review queue, that's where we'll return.
    $queue_id = $this->getRequest()->get('queue');
    if (!empty($queue_id)) {
      $this->returnToQueue($queue_id, $form_state);
    }
    else {
      // Back to the article page.
      $article_id = $this->getRequest()->get('article');
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
    $options = ['query' => $this->getRequest()->query->all()];
    unset($options['query']['queue']);
    $form_state->setRedirect($route, $parameters, $options);
  }

}
