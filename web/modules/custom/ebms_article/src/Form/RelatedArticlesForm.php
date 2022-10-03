<?php

namespace Drupal\ebms_article\Form;

use Drupal\Core\Entity\EntityTypeManagerInterface;
use Drupal\Core\Form\FormBase;
use Drupal\Core\Form\FormStateInterface;
use Drupal\Core\Session\AccountProxyInterface;
use Drupal\ebms_article\Entity\Article;
use Drupal\ebms_article\Entity\Relationship;
use Symfony\Component\DependencyInjection\ContainerInterface;

/**
 * Form controller for specifying relationships between articles.
 *
 * @ingroup ebms
 */
class RelatedArticlesForm extends FormBase {

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
  public static function create(ContainerInterface $container): RelatedArticlesForm {
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
    return 'ebms_related_articles_form';
  }

  /**
   * {@inheritdoc}
   */
  public function buildForm(array $form, FormStateInterface $form_state, int $relationship_id = 0): array {

    // Different approaches for new versus existing relationships.
    $suppress = $comments = '';
    $type = NULL;
    if (!empty($relationship_id)) {
      $relationship = Relationship::load($relationship_id);
      $comments = $relationship->comment->value;
      $type = $relationship->type->target_id;
      $suppress = $relationship->suppress->value;
      $related_id = $relationship->related->target_id;
      $related = ['#type' => 'hidden', '#value' => $related_id];
      $related_to_id = $relationship->related_to->target_id;
      $related_to = ['#type' => 'hidden', '#value' => $related_to_id];
      $title = 'Edit Article Relationship';
      $article_info = [
        '#theme' => 'related_articles_info',
        '#title' => $relationship->related_to->entity->title->value,
        '#citation' => $relationship->related_to->entity->getLabel(),
        '#related_title' => $relationship->related->entity->title->value,
        '#related_citation' => $relationship->related->entity->getLabel(),
      ];
    }
    else {
      $article_id = $this->getRequest()->query->get('article');
      $article = Article::load($article_id);
      $article_info = [
        '#theme' => 'article_info',
        '#title' => $article->title->value,
        '#citation' => $article->getLabel(),
      ];
      $label = $article->getLabel();
      $related_to = ['#type' => 'hidden', '#value' => $article_id];
      $related = [
        '#title' => 'Related article ID(s)',
        '#type' => 'textfield',
        '#description' => 'EBMS ID(s) of related article(s), separated by commas and/or spaces.',
        '#required' => TRUE,
      ];
      $title = 'Link Related Articles';
    }

    // Build the picklist for available relationship types.
    $storage = $this->entityTypeManager->getStorage('taxonomy_term');
    $query = $storage->getQuery()->condition('vid', 'relationship_types');
    $terms = $storage->loadMultiple($query->execute());
    $types = [];
    foreach ($terms as $term) {
      $name = $term->getName();
      $desc = trim($term->get('description')->value ?? '');
      if (!empty($desc)) {
        $name .= " - $desc";
      }
      $types[$term->id()] = $name;
    }

    return [
      '#title' => $title,
      'article-info' => $article_info,
      'relationship-id' => ['#type' => 'hidden', '#value' => $relationship_id],
      'related' => $related,
      'related-to' => $related_to,
      'type' => [
        '#type' => 'radios',
        '#title' => 'Relationship type',
        '#description' => 'Identify the nature of the relationship.',
        '#options' => $types,
        '#default_value' => $type,
        '#required' => TRUE,
      ],
      'comments' => [
        '#type' => 'textarea',
        '#title' => 'Comments',
        '#description' => 'Optional notes about the relationship.',
        '#default_value' => $comments,
      ],
      'options' => [
        '#type' => 'checkboxes',
        '#title' => 'Options',
        '#options' => ['suppress' => "Suppress (don't show relationship for packets)"],
        '#default_value' => ['suppress' => $suppress ? 'suppress' : ''],
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
   * {@inheritdoc}
   */
  public function validateForm(array &$form, FormStateInterface $form_state) {
    $trigger = $form_state->getTriggeringElement()['#value'];
    if ($trigger === 'Cancel') {
      return;
    }
    $target_id = $form_state->getValue('related-to');
    $ids = preg_split("/[\s,.]+/", $form_state->getValue('related'));
    if (empty($ids)) {
      $form_state->setErrorByName('related', 'No related articles identified.');
    }
    $related = [];
    foreach ($ids as $id) {
      if (!is_numeric($id)) {
        $form_state->setErrorByName('related', 'Invalid article ID syntax.');
        break;
      }
      if ($id == $target_id) {
        $form_state->setErrorByName('related', 'Article cannot be related to itself.');
      }
      $article = Article::load($id);
      if (empty($article)) {
        $form_state->setErrorByName('related', "$id is not a valid article ID.");
        break;
      }
      $related[$id] = $article->getLabel();
    }
    if (empty($form_state->getValue('type'))) {
      $form_state->setErrorByName('type', 'Relationship type not selected.');
    }
    $form_state->setValue('related-articles', $related);
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
    $this->returnToArticle($form_state);
  }

  /**
   * {@inheritdoc}
   */
  public function submitForm(array &$form, FormStateInterface $form_state) {

    // Collect the form's values.
    $values = $form_state->getValues();
    $suppress = !empty($values['options']['suppress']);
    $type = $form_state->getValue('type');
    $comment = trim($form_state->getValue('comments') ?? '') ?: NULL;
    $relationship_id = $form_state->getValue('relationship-id');

    // If we're editing an existing relationship, re-save it.
    if (!empty($relationship_id)) {
      $storage = $this->entityTypeManager->getStorage('ebms_article_relationship');
      $relationship = $storage->load($relationship_id);
      $relationship->suppress = $suppress;
      $relationship->comment = $comment;
      $relationship->type = $type;
      $relationship->recorded = date('Y-m-d H:i:s');
      $relationship->recorded_by = $this->account->id();
      $relationship->save();
      $this->messenger()->addMessage('Relationship updated.');
    }
    else {
      $values = [
        'related_to' => $form_state->getValue('related-to'),
        'type' => $type,
        'recorded' => date('Y-m-d H:i:s'),
        'recorded_by' => $this->account->id(),
        'comment' => $form_state->getValue('comments'),
        'suppress' => $suppress,
      ];
      $related = $form_state->getValue('related-articles');
      foreach ($related as $id => $label) {
        $values['related'] = $id;
        $relationship = Relationship::create($values);
        $relationship->save();
        $this->messenger()->addMessage("Relationship with $label created.");
      }
    }

    // Back to the article page.
    $this->returnToArticle($form_state);
  }

  /**
   * Return directly to the article page.
   *
   * @param \Drupal\Core\Form\FormStateInterface $form_state
   *   Form values.
   */
  private function returnToArticle(FormStateInterface $form_state) {
    $route = 'ebms_article.article';
    $query = $this->getRequest()->query->all();
    $article_id = $query['article'];
    unset($query['article']);
    $parameters = ['article' => $article_id];
    $options = ['query' => $query];
    $form_state->setRedirect($route, $parameters, $options);
  }

}
