<?php

namespace Drupal\ebms_article\Form;

use Drupal\Core\Entity\EntityTypeManagerInterface;
use Drupal\Core\Form\FormBase;
use Drupal\Core\Form\FormStateInterface;
use Drupal\Core\Session\AccountProxyInterface;
use Drupal\ebms_article\Entity\Article;
use Symfony\Component\DependencyInjection\ContainerInterface;

/**
 * Form controller for adding a comment to an article/topic state.
 *
 * @ingroup ebms
 */
class AddStateCommentForm extends FormBase {

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
  public static function create(ContainerInterface $container): AddStateCommentForm {
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
    return 'ebms_state_comment_form';
  }

  /**
   * {@inheritdoc}
   */
  public function buildForm(array $form, FormStateInterface $form_state, int $state_id = 0): array {
    $storage = $this->entityTypeManager->getStorage('ebms_state');
    $state = $storage->load($state_id);
    $state_name = $state->value->entity->getName();
    $article_id = $this->getRequest()->get('article');
    $article = Article::load($article_id);
    return [
      '#title' => "Add comment for $state_name state",
      'article-info' => [
        '#theme' => 'article_info',
        '#title' => $article->title->value,
        '#citation' => $article->getLabel(),
      ],
      'state' => ['#type' => 'hidden', '#value' => $state_id],
      'comment' => [
        '#type' => 'textarea',
        '#title' => 'Comment',
        '#description' => 'Add some information relevant to this state.',
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
    $article_id = $this->getRequest()->get('article');
    $this->returnToArticle($article_id, $form_state);
  }

  /**
   * {@inheritdoc}
   */
  public function submitForm(array &$form, FormStateInterface $form_state) {
    $state_id = $form_state->getValue('state');
    $storage = $this->entityTypeManager->getStorage('ebms_state');
    $state = $storage->load($state_id);
    $state->comments[] = [
      'user' => $this->account->id(),
      'entered' => date('Y-m-d H:i:s'),
      'body' => $form_state->getValue('comment'),
    ];
    $state->save();
    $this->messenger()->addMessage('Comment successfully added.');
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
    unset($options['query']['article']);
    $form_state->setRedirect($route, $parameters, $options);
  }

}
