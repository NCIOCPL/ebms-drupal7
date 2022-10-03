<?php

namespace Drupal\ebms_review\Form;

use Drupal\Core\Form\FormBase;
use Drupal\Core\Form\FormStateInterface;
use Drupal\file\Entity\File;
use Drupal\ebms_doc\Entity\Doc;
use Drupal\ebms_message\Entity\Message;
use Drupal\ebms_review\Entity\Packet;
use Drupal\ebms_review\Entity\ReviewerDoc;
use Drupal\ebms_summary\Entity\SummaryPage;

/**
 * Document posted by a board member to a packet.
 *
 * @ingroup ebms
 */
class ReviewerDocForm extends FormBase {

  /**
   * {@inheritdoc}
   */
  public function getFormId(): string {
    return 'ebms_reviewer_doc_form';
  }

  /**
   * {@inheritdoc}
   */
  public function buildForm(array $form, FormStateInterface $form_state, $packet_id = 0): array {
    $packet = Packet::load($packet_id);
    $title = $packet->title->value;
    return [
      '#title' => "Post document for $title",
      'packet-id' => [
        '#type' => 'hidden',
        '#value' => $packet_id,
      ],
      'doc' => [
        '#title' => 'Document',
        '#type' => 'file',
        '#required' => TRUE,
        '#validated' => TRUE, // We have to do this ourselves.
        '#attributes' => ['class' => ['usa-file-input']],
        '#description' => 'Select the document you would like to upload. To complete the file upload, you must click the Upload File button below.',
      ],
      'notes' => [
        '#type' => 'textarea',
        '#title' => 'Notes',
        '#description' => 'Add any optional notes you would like to have displayed in the list of posted documents for this packet.',
      ],
      'submit' => [
        '#type' => 'submit',
        '#value' => 'Upload File',
      ],
    ];
  }

  /**
   * {@inheritdoc}
   */
  public function submitForm(array &$form, FormStateInterface $form_state) {
    $packet_id = $form_state->getValue('packet-id');
    $file = File::load($form_state->getValue('file-id'));
    $now = date('Y-m-d H:i:s');
    $uid = $this->getRequest()->query->get('obo') ?: $this->currentUser()->id();
    $notes = $form_state->getValue('notes');
    $fid = $file->id();
    $values = [
      'file' => $fid,
      'reviewer' => $uid,
      'posted' => $now,
      'description' => $notes,
    ];
    $doc = ReviewerDoc::create($values);
    $doc->save();
    $file_usage = \Drupal::service('file.usage');
    $file_usage->add($file, 'ebms_reviewer_doc', 'ebms_reviewer_doc', $doc->id());
    $packet = Packet::load($packet_id);
    $packet->reviewer_docs[] = $doc->id();
    $packet->save();

    // Remember this for reports and the home page alerts.
    $board_id = $packet->topic->entity->board->target_id;
    Message::create([
      'message_type' => Message::SUMMARY_POSTED,
      'user' => $uid,
      'posted' => $now,
      'boards' => [$board_id],
      'extra_values' => json_encode([
        'summary_url' => $file->createFileUrl(),
        'title' => $file->getFilename(),
        'notes' => $notes,
      ]),
    ])->save();

    // Attach the document to any summary pages with the same topic.
    $topic_id = $packet->topic->target_id;
    $storage = \Drupal::entityTypeManager()->getStorage('ebms_summary_page');
    $query = $storage->getQuery()->accessCheck(FALSE);
    $query->condition('topics', $packet->topic->target_id);
    $query->condition('active', 1);
    $ids = $query->execute();
    if (!empty($ids)) {
      $filename = $file->getFilename();
      $description = pathinfo($filename)["filename"];
      $doc = Doc::create([
        'file' => $fid,
        'posted' => $now,
        'description' => $description,
      ]);
      $doc->save();
      $values = [
        'doc' => $doc->id(),
        'notes' => $notes,
        'active' => 1,
      ];
      foreach (SummaryPage::loadMultiple($ids) as $page) {
        $page->member_docs[] = $values;
        $page->save();
      }
    }
    $this->messenger()->addMessage('Successfully posted the document.');
    $options = ['query' => $this->getRequest()->query->all()];
    $route = $options['query']['referer'] ?? 'ebms_review.assigned_packet';
    unset($options['query']['referer']);
    $parms = ['packet_id' => $packet_id];
    $form_state->setRedirect($route, $parms, $options);
  }

  /**
   * {@inheritdoc}
   */
  public function validateForm(array &$form, FormStateInterface $form_state) {
    $validators = ['file_validate_extensions' => ''];
    $file = file_save_upload('doc', $validators, 'public://', 0);
    if (empty($file)) {
      $form_state->setErrorByName('doc', 'File is required.');
    }
    else {
      $file->setPermanent();
      $file->save();
      $form_state->setValue('file-id', $file->id());
    }
  }

}
