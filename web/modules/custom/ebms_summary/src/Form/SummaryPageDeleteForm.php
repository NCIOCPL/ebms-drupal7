<?php

namespace Drupal\ebms_summary\Form;

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
class SummaryPageDeleteForm extends ContentEntityConfirmFormBase {

  /**
   * {@inheritdoc}
   */
  public function getQuestion() {
    return 'Delete this summary page?';
  }

  /**
   * {@inheritdoc}
   */
  public function getDescription() {
    $entity = $this->entity;
    $name = $entity->name->value;
    return "Deleting summary page <em>$name</em>. This action cannot be undone.";
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
    $board_id = $query['board'];
    unset($query['board']);
    $opts = ['query' => $query];
    $parms = ['board_id' => $board_id];
    return Url::fromRoute('ebms_summary.board', $parms, $opts);
  }

  /**
   * {@inheritdoc}
   */
  public function submitForm(array &$form, FormStateInterface $form_state) {
    $entity = $this->entity;
    $name = $entity->name->value;
    $entity->set('active', 0);
    $entity->save();
    $this->messenger()->addMessage($this->t('Deleted summary page <em>@name</em>.', [
      '@name' => $name,
    ]));
    $form_state->setRedirectUrl($this->getCancelUrl());
  }

}
