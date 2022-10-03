<?php

namespace Drupal\ebms_article\Form;

use Drupal\Core\Entity\EntityTypeManagerInterface;
use Drupal\Core\Form\FormBase;
use Drupal\Core\Form\FormStateInterface;
use Symfony\Component\DependencyInjection\ContainerInterface;

/**
 * Form controller for deleting a board manager topic-level comment.
 *
 * @ingroup ebms
 */
class InternalCommentDeleteForm extends FormBase {

  /**
   * The entity type manager.
   *
   * @var \Drupal\Core\Entity\EntityTypeManagerInterface
   */
  protected EntityTypeManagerInterface $entityTypeManager;

  /**
   * {@inheritdoc}
   */
  public static function create(ContainerInterface $container): InternalCommentDeleteForm {
    // Instantiates this form class.
    $instance = parent::create($container);
    $instance->entityTypeManager = $container->get('entity_type.manager');
    return $instance;
  }

  /**
   * {@inheritdoc}
   */
  public function getFormId(): string {
    return 'ebms_internal_comment_delete_form';
  }

  /**
   * {@inheritdoc}
   */
  public function buildForm(array $form, FormStateInterface $form_state, int $article_id = 0): array {
    $storage = $this->entityTypeManager->getStorage('ebms_article');
    $article = $storage->load($article_id);
    $label = $article->getLabel();
    return [
      'article' => [
        '#type' => 'hidden',
        '#value' => $article_id,
      ],
      '#title' => "Delete internal comment for $label?",
      'submit' => [
        '#type' => 'submit',
        '#value' => 'Confirm Deletion',
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

    $article_id = $form_state->getValue('article');
    $storage = $this->entityTypeManager->getStorage('ebms_article');
    $article = $storage->load($article_id);
    // Delta is passed as 1-based, even though Drupal has them as 0-based,
    // because of PHP's very odd behavior with `empty("0")`.
    $delta = $this->getRequest()->get('delta') - 1;
    unset($article->internal_comments[$delta]);
    $article->save();
    $this->messenger()->addMessage('Internal comment successfully deleted.');

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
    unset($options['query']['article']);
    unset($options['query']['delta']);
    $form_state->setRedirect($route, $parameters, $options);
  }

}
