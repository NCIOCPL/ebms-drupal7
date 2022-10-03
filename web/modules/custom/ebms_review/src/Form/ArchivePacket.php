<?php

namespace Drupal\ebms_review\Form;

use Drupal\Core\Entity\ContentEntityConfirmFormBase;
use Drupal\Core\Form\FormStateInterface;
use Drupal\Core\Url;

/**
 * Form for archiving an EBMS review packet.
 */
class ArchivePacket extends ContentEntityConfirmFormBase {

  /**
   * {@inheritdoc}
   */
  public function getQuestion() {
    return 'Archive this packet?';
  }

  /**
   * {@inheritdoc}
   */
  public function getDescription() {
    return 'Archiving this packet will remove it from the queues of any reviewers who have not responded.';
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
    $route = 'ebms_review.reviewed_packet';
    if (!empty($query['unreviewed'])) {
      $route = 'ebms_review.unreviewed_packet';
      unset($query['unreviewed']);
    }
    $opts = ['query' => $query];
    return Url::fromRoute($route, ['packet_id' => $this->entity->id()], $opts);
  }

  /**
   * {@inheritdoc}
   */
  public function submitForm(array &$form, FormStateInterface $form_state) {
    $filter_id = $this->getRequest()->query->get('filter-id');
    $opts = ['query' => []];
    if (!empty($filter_id)) {
      $opts['query']['filter-id'] = $filter_id;
    }
    $this->entity->set('active', 0);
    $this->entity->save();
    $this->messenger()->addMessage('Packet successfully archived.');
    $route = $this->getRequest()->query->get('unreviewed') ? 'ebms_review.unreviewed_packets' : 'ebms_review.reviewed_packets';
    $form_state->setRedirect($route, [], $opts);
  }

}
