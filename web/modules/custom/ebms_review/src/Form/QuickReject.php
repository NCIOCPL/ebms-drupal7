<?php

namespace Drupal\ebms_review\Form;

use Drupal\Core\Form\FormBase;
use Drupal\Core\Form\FormStateInterface;
use Drupal\ebms_review\Entity\PacketArticle;
use Drupal\ebms_review\Entity\Review;
use Drupal\user\Entity\User;

/**
 * Fast path to a rejection review.
 */
class QuickReject extends FormBase {

  /**
   * {@inheritdoc}
   */
  public function getFormId(): string {
    return 'quick_reject';
  }

  /**
   * {@inheritdoc}
   */
  public function buildForm(array $form, FormStateInterface $form_state, $packet_id = NULL, $packet_article_id = NULL): array {

    // Get the values for the rejection reasons field.
    $storage = \Drupal::entityTypeManager()->getStorage('taxonomy_term');
    $query = $storage->getQuery()->accessCheck(FALSE)
      ->sort('weight')
      ->condition('status', 1)
      ->condition('vid', 'rejection_reasons');
    $reasons = [];
    foreach ($storage->loadMultiple($query->execute()) as $term) {
      $display = htmlspecialchars($term->name->value);
      if (!empty($term->description->value)) {
        $description = htmlspecialchars(rtrim($term->description->value, '.'));
        $display .= " (<em>$description</em>)";
      }
      $reasons[$term->id()] = $display;
    }

    // Assemble the render array for the form.
    return [
      '#attached' => ['library' => ['ebms_review/review-form']],
      'packet-id' => [
        '#type' => 'hidden',
        '#value' => $packet_id,
      ],
      'packet-article' => [
        '#type' => 'hidden',
        '#value' => $packet_article_id,
      ],
      'reasons' => [
        '#type' => 'checkboxes',
        '#multiple' => TRUE,
        '#title' => 'Reason(s) for Exclusion From PDQ® Summary',
        '#options' => $reasons,
        '#description' => 'Please indicate which of these reasons led to your decision to exclude the article. You may choose more than one reason.',
        '#required' => TRUE,
      ],
      'comment' => [
        '#type' => 'text_format',
        '#format' => 'board_member_html',
        '#title' => 'Comments',
        '#description' => 'If you have additional comments, please add them here.',
      ],
      'submit' => [
        '#type' => 'submit',
        '#value' => 'Submit',
      ],
    ];
  }

  /**
   * {@inheritdoc}
   */
  public function submitForm(array &$form, FormStateInterface $form_state) {
    $packet_article_id = $form_state->getValue('packet-article');
    $packet_article = PacketArticle::load($packet_article_id);
    $comment = $form_state->getValue('comment');
    $obo = $this->getRequest()->query->get('obo');
    $uid = $this->currentUser()->id();
    if (!empty($obo)) {
      $user = User::load($obo);
      $obo_name = $user->name->value;
      $user = User::load($uid);
      $user_name = $user->name->value;
      $comment['value'] .= "<p><i>Recorded by $user_name on behalf of $obo_name.</i></p>";
      $uid = $obo;
    }
    $values = [
      'reviewer' => $uid,
      'posted' => date('Y-m-d H:i:s'),
      'comments' => $comment,
      'dispositions' => [Review::getRejectionDisposition()],
      'reasons' => array_values(array_diff($form_state->getValue('reasons'), [0])),
    ];
    $review = Review::create($values);
    $review->save();
    $packet_article->reviews[] = $review->id();
    $packet_article->save();
    $this->messenger()->addMessage('Review successfully stored.');
    $parms = ['packet_id' => $form_state->getValue('packet-id')];
    $options = ['query' => $this->getRequest()->query->all()];
    $form_state->setRedirect('ebms_review.assigned_packet', $parms, $options);
  }

}
