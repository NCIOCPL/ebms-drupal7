<?php

namespace Drupal\ebms_review\Form;

use Drupal\Core\Form\FormBase;
use Drupal\Core\Form\FormStateInterface;
use Drupal\Core\Url;
use Drupal\ebms_review\Entity\Packet;
use Drupal\ebms_review\Entity\PacketArticle;
use Drupal\ebms_review\Entity\Review;
use Drupal\file\Entity\File;
use Drupal\user\Entity\User;

/**
 * Form used by the board memebers to assess articles in review packets.
 */
class ReviewForm extends FormBase {

  /**
   * {@inheritdoc}
   */
  public function getFormId(): string {
    return 'ebms_review_form';
  }

  /**
   * {@inheritdoc}
   */
  public function buildForm(array $form, FormStateInterface $form_state, $packet_id = NULL, $packet_article_id = NULL): array {

    // Set the default title.
    $packet = Packet::load($packet_id);
    $title = htmlspecialchars($packet->title->value);

    // Override the default if working on behalf of a board member.
    $obo = $this->getRequest()->query->get('obo');
    if (!empty($obo)) {
      $user = User::load($obo);
      $name = $user->name->value;
      $title .= " (on behalf of $name)";
    }

    // Get the values for the article display.
    $packet_article = PacketArticle::load($packet_article_id);
    $article = $packet_article->article->entity;
    $authors = [];
    foreach ($article->authors as $author) {
      $authors[] = $author->display_name;
    }
    $citation = $article->brief_journal_title->value;
    if (!empty($article->volume->value)) {
      $citation .= " {$article->volume->value}";
    }
    if (!empty($article->issue->value)) {
      $citation .= " ({$article->issue->value})";
    }
    if (!empty($article->pagination->value)) {
      $citation .= ": {$article->pagination->value}";
    }
    $citation .= ", {$article->year->value}";
    $file = File::load($article->full_text->file);
    $full_text_url = empty($file) ? '' : $file->createFileUrl();
    $meetings = '';
    $state = $article->getCurrentState($packet->topic->target_id);
    if ($state->value->entity->field_text_id->value === 'on_agenda') {
      $meetings = [];
      foreach ($state->meetings as $meeting) {
        $entity = $meeting->entity;
        $name = $entity->name->value;
        $date = substr($entity->dates->value, 0, 10);
        $meetings[] = "$name - $date";
      }
      $meetings = implode('; ', $meetings);
      if (empty($meetings)) {
        $meetings = 'NO MEETINGS RECORDED';
      }
    }
    $uid = $this->currentUser()->id();
    $others = 0;
    $other_url = '';
    foreach ($packet_article->reviews as $review) {
      if ($review->entity->reviewer->target_id != $uid) {
        ++$others;
      }
    }
    if (!empty($others)) {
      $opts = [];
      if (!empty($obo)) {
        $opts = ['query' => ['obo' => $obo]];
      }
      $parms = ['packet_id' => $packet_id, 'packet_article_id' => $packet_article_id];
      $other_url = Url::fromRoute('ebms_review.other_reviews', $parms, $opts);
    }

    // Get the values for the dispositions field.
    $storage = \Drupal::entityTypeManager()->getStorage('taxonomy_term');
    $query = $storage->getQuery()->accessCheck(FALSE)
      ->sort('weight')
      ->condition('status', 1)
      ->condition('vid', 'dispositions');
    $dispositions = [];
    $no_changes = NULL;
    foreach ($storage->loadMultiple($query->execute()) as $term) {
      if (is_null($no_changes)) {
        $no_changes = $term->id();
      }
      $display = htmlspecialchars($term->name->value);
      if (!empty($term->description->value)) {
        $description = htmlspecialchars(rtrim($term->description->value, '.'));
        $display .= " (<em>$description</em>)";
      }
      $dispositions[$term->id()] = $display;
    }

    // Get the values for the rejection reasons field.
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

    // Drupal's AJAX is broken, so we roll our own JavaScript.
    // For example, https://www.drupal.org/project/drupal/issues/3207786.
    $form = [
      '#title' => $title,
      '#attached' => [
        'library' => ['ebms_review/review-form'],
      ],
      'packet-id' => [
        '#type' => 'hidden',
        '#value' => $packet_id,
      ],
      'packet-article' => [
        '#type' => 'hidden',
        '#value' => $packet_article_id,
      ],
      'article' => [
        '#theme' => 'packet_review_article',
        '#article' => [
          'pmid' => $article->source_id->value,
          'authors' => implode(', ', $authors),
          'title' => $article->title->value,
          'citation' => $citation,
          'full_text_url' => $full_text_url,
          'meetings' => $meetings,
          'other_url' => $other_url,
        ],
      ],
      'dispositions' => [
        '#type' => 'checkboxes',
        '#multiple' => TRUE,
        '#title' => 'Disposition',
        '#required' => TRUE,
        '#options' => $dispositions,
        '#description' => 'Indicate how the article might affect the summary.',
      ],
      'reasons-wrapper' => [
        '#type' => 'container',
        '#attributes' => ['class' => ['hidden'], 'id' => 'reasons-wrapper'],
        'reasons' => [
          '#type' => 'checkboxes',
          '#multiple' => TRUE,
          '#title' => 'Reason(s) for Exclusion From PDQ® Summary',
          '#options' => $reasons,
          '#description' => 'Please indicate which of these reasons led to your decision to exclude the article. You may choose more than one reason.',
          '#required' => TRUE,
          '#validated' => TRUE,
        ],
      ],
      'comment' => [
        '#type' => 'text_format',
        '#title' => 'Comments and Changes to Summary',
        '#format' => 'board_member_html',
        '#description' => 'Please make your suggested changes directly in the summary using Track Changes and upload the revised summary on the page for this literature packet. If you have additional comments, please add them here.',
      ],
      'loe-wrapper' => [
        '#type' => 'container',
        '#attributes' => ['class' => ['hidden'], 'id' => 'loe-wrapper'],
        'loe' => [
          '#type' => 'textarea',
          '#title' => 'Levels of Evidence Information',
          '#description' => 'Enter the appropriate level of evidence for this article.',
        ],
      ],
    ];
    if (!empty($packet->topic->entity->board->entity->loe_guidelines->entity)) {
      $file = $packet->topic->entity->board->entity->loe_guidelines->entity;
      $form['loe-wrapper']['loe-guidelines'] = [
        '#type' => 'link',
        '#title' => 'Download LOE Guidelines',
        '#url' => Url::fromUri($file->createFileUrl(false)),
        '#attributes' => [
          'class' => ['button', 'usa-button'],
          'id' => 'loe-guidelines',
        ],
        '#weight' => -1,
      ];
    }
    $form['submit'] = [
      '#type' => 'submit',
      '#value' => 'Submit',
    ];
    return $form;
  }

  /**
   * {@inheritdoc}
   */
  public function validateForm(array &$form, FormStateInterface $form_state) {

    // Find out which disposition means "no changes" (it's the first one).
    $storage = \Drupal::entityTypeManager()->getStorage('taxonomy_term');
    $ids = $storage->getQuery()->accessCheck(FALSE)
      ->sort('weight')
      ->condition('vid', 'dispositions')
      ->execute();
    $no_changes = reset($ids);

    // Find out which dispositions were selected.
    $dispositions = [];
    foreach ($form_state->getValue('dispositions') as $key => $value) {
      if (!empty($value)) {
        $dispositions[] = $key;
      }
    }
    $form_state->setValue('disposition-ids', $dispositions);

    // Same for the rejection reasons.
    $reasons = [];
    foreach ($form_state->getValue('reasons') as $key => $value) {
      if (!empty($value)) {
        $reasons[] = $key;
      }
    }
    $form_state->setValue('reason-ids', $reasons);

    // If the reviewer said don't use the article, a reason must be given.
    if ($dispositions == [$no_changes]) {
      if (empty($reasons)) {
        $form_state->setErrorByName('reasons', 'At least one rejection reason must be selected.');
      }
    }
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
      'loe_info' => $form_state->getValue('loe'),
      'dispositions' => $form_state->getValue('disposition-ids'),
      'reasons' => $form_state->getValue('reason-ids'),
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
