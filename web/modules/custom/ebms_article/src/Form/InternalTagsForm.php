<?php

namespace Drupal\ebms_article\Form;

use Drupal\Core\Entity\EntityTypeManagerInterface;
use Drupal\Core\Form\FormBase;
use Drupal\Core\Form\FormStateInterface;
use Symfony\Component\DependencyInjection\ContainerInterface;

/**
 * Form controller for adding a tag to an article.
 *
 * @ingroup ebms
 */
class InternalTagsForm extends FormBase {

  /**
   * The entity type manager.
   *
   * @var \Drupal\Core\Entity\EntityTypeManagerInterface
   */
  protected EntityTypeManagerInterface $entityTypeManager;

  /**
   * {@inheritdoc}
   *
   * @noinspection PhpFieldAssignmentTypeMismatchInspection
   */
  public static function create(ContainerInterface $container): InternalTagsForm {
    // Instantiates this form class.
    $instance = parent::create($container);
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
    $storage = $this->entityTypeManager->getStorage('taxonomy_term');
    $query = $storage->getQuery()->accessCheck(FALSE);
    $query->condition('vid', 'internal_tags');
    $query->sort('name');
    $entities = $storage->loadMultiple($query->execute());
    $tags = [];
    foreach ($entities as $entity) {
      $tags[$entity->id()] = $entity->getName();
    }
    $storage = $this->entityTypeManager->getStorage('ebms_article');
    $article = $storage->load($article_id);
    $label = $article->getLabel();
    $selected = [];
    foreach ($article->internal_tags as $internal_tag) {
      $selected[] = $internal_tag->tag;
    }

    return [
      'article' => [
        '#type' => 'hidden',
        '#value' => $article_id,
      ],
      '#title' => 'Internal Tags',
      'article-info' => [
        '#theme' => 'article_info',
        '#title' => $article->title->value,
        '#citation' => $article->getLabel(),
      ],
      'tags' => [
        '#type' => 'checkboxes',
        '#title' => 'Tags',
        '#description' => 'Tags designating articles for internal use rather than board member review.',
        '#options' => $tags,
        '#default_value' => $selected,
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
    $article_id = $form_state->getValue('article');
    $storage = $this->entityTypeManager->getStorage('ebms_article');
    $article = $storage->load($article_id);
    $added = [];
    $now = date('Y-m-d H:i:s');
    foreach ($article->internal_tags as $internal_tag) {
      $added[$internal_tag->tag] = $internal_tag->added;
    }
    $tags = [];
    foreach ($form_state->getValue('tags') as $value) {
      if (!empty($value)) {
        $tags[] = [
          'tag' => $value,
          'added' => $added[$value] ?? $now,
        ];
      }
    }
    $article->internal_tags = $tags;
    $article->save();

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
    $form_state->setRedirect($route, $parameters, $options);
  }

}
