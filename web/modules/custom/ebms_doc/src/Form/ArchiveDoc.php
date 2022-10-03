<?php

namespace Drupal\ebms_doc\Form;

use Drupal\Core\Entity\ContentEntityConfirmFormBase;
use Drupal\Core\Form\FormStateInterface;
use Drupal\Core\Url;

/**
 * Form for archiving an EBMS document.
 */
class ArchiveDoc extends ContentEntityConfirmFormBase {

  /**
   * {@inheritdoc}
   */
  public function getQuestion() {
    return 'Archive this document?';
  }

  /**
   * {@inheritdoc}
   */
  public function getDescription() {
    $target = htmlspecialchars($this->entity->file->entity->filename->value);
    return "Archiving document $target. This action cannot be undone.";
  }

  /**
   * {@inheritdoc}
   */
  public function getConfirmText() {
    return 'Archive';
  }

  /**
   * {@inheritdoc}
   */
  public function getCancelUrl(): Url {
    $query = $this->getRequest()->query->all();
    $opts = ['query' => $query];
    return Url::fromRoute('ebms_doc.list', [], $opts);
  }

  /**
   * {@inheritdoc}
   */
  public function submitForm(array &$form, FormStateInterface $form_state) {
    $this->entity->dropped = TRUE;
    $this->entity->save();
    $this->messenger()->addMessage('Document successfully archived.');
    $form_state->setRedirectUrl($this->getCancelUrl());
  }

}
