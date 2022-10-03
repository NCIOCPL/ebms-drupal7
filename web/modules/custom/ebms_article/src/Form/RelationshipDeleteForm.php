<?php

namespace Drupal\ebms_article\Form;

use Drupal\Core\Entity\ContentEntityConfirmFormBase;
use Drupal\Core\Form\FormStateInterface;
use Drupal\Core\Url;

/**
 * Form for deleting a relationship between two articles.
 *
 * The user interface presents this as a deletion, but behind the scenes
 * the software retains the entity and marks it as deactivated. The users
 * indicated they might want to see when relationships were "deleted" and
 * by whom at some point in the future.
 */
class RelationshipDeleteForm extends ContentEntityConfirmFormBase {

  /**
   * {@inheritdoc}
   */
  public function getQuestion() {
    return 'Delete this relationship?';
  }

  /**
   * {@inheritdoc}
   */
  public function getDescription() {
    $related = htmlspecialchars($this->entity->related->entity->getLabel());
    $related_to = htmlspecialchars($this->entity->related_to->entity->getLabel());
    return "Deleting relationship between <em>$related</em> and <em>$related_to</em>. This action cannot be undone.";
  }

  /**
   * {@inheritdoc}
   */
  public function getConfirmText() {
    return 'Delete';
  }

  /**
   * {@inheritdoc}
   */
  public function getCancelUrl(): Url {
    $query = $this->getRequest()->query->all();
    $article_id = $query['article'];
    unset($query['article']);
    $opts = ['query' => $query];
    $parms = ['article' => $article_id];
    return Url::fromRoute('ebms_article.article', $parms, $opts);
  }

  /**
   * {@inheritdoc}
   */
  public function submitForm(array &$form, FormStateInterface $form_state) {
    $related = $this->entity->related->entity->getLabel();
    $related_to = $this->entity->related_to->entity->getLabel();
    $this->entity->inactivated = date('Y-m-d H:i:s');
    $this->entity->inactivated_by = $this->currentUser()->id();
    $this->entity->save();
    $this->messenger()->addMessage($this->t('Deleted relationship between @related and @related_to.', [
      '@related' => $related,
      '@related_to' => $related_to,
    ]));
    $form_state->setRedirectUrl($this->getCancelUrl());
  }

}
