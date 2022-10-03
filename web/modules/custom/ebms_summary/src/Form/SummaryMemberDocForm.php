<?php

namespace Drupal\ebms_summary\Form;

use Drupal\Core\Form\FormBase;
use Drupal\Core\Form\FormStateInterface;
use Drupal\ebms_doc\Entity\Doc;
use Drupal\ebms_message\Entity\Message;
use Drupal\ebms_summary\Entity\SummaryPage;
use Drupal\file\Entity\File;

/**
 * Form for adding a supporting document to a board summaries page.
 *
 * @ingroup ebms
 */
class SummaryMemberDocForm extends FormBase {

  /**
   * {@inheritdoc}
   */
  public function getFormId(): string {
    return 'ebms_summary_member_doc_form';
  }

  /**
  * {@inheritdoc}
  */
  public function buildForm(array $form, FormStateInterface $form_state, $summary_page = NULL): array {
    $page_name = $summary_page->name->value;
    return [
      '#title' => 'Board Member Upload',
      'page-id' => [
        '#type' => 'hidden',
        '#value' => $summary_page->id(),
      ],
      'file' => [
        '#title' => 'Choose File',
        '#type' => 'file',
        '#attributes' => [
          'class' => ['usa-file-input'],
          'accept' => ['.doc,.docx'],
        ],
        '#description' => 'Select the document you would like to upload. To complete the file upload, you must click the Upload File button below.',
        '#required' => TRUE,
        '#validated' => TRUE, // Do this ourselves so we can use USWDS widget.
      ],
      'notes' => [
        '#type' => 'textarea',
        '#title' => 'Notes',
        '#description' => "Add optional notes to be displayed with the document's link.",
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

    // Save the document.
    $page_id = $form_state->getValue('page-id');
    $notes = $form_state->getValue('notes');
    $file = File::load($form_state->getValue('file-id'));
    $filename = $file->getFilename();
    $description = pathinfo($filename)["filename"];
    $now = date('Y-m-d H:i:s');
    $doc = Doc::create([
      'file' => $file->id(),
      'posted' => $now,
      'description' => $description,
    ]);
    $doc->save();
    $file_usage = \Drupal::service('file.usage');
    $file_usage->add($file, 'ebms_doc', 'ebms_doc', $doc->id());
    $values = [
      'doc' => $doc->id(),
      'notes' => $notes,
      'active' => 1,
    ];
    $summary_page = SummaryPage::load($page_id);
    $summary_page->member_docs[] = $values;
    $summary_page->save();
    $this->messenger()->addMessage("Posted document {$filename}.");

    // Create a notification message for the home page.
    $storage = \Drupal::entityTypeManager()->getStorage('ebms_board_summaries');
    $ids = $storage->getQuery()
      ->accessCheck(FALSE)
      ->condition('pages', $page_id)
      ->execute();
    if (!empty($ids)) {
      $board_id = $storage->load(reset($ids))->board->target_id;
      Message::create([
        'message_type' => Message::SUMMARY_POSTED,
        'user' => $this->currentUser()->id(),
        'posted' => $now,
        'boards' => [$board_id],
        'extra_values' => json_encode([
          'summary_url' => $file->createFileUrl(),
          'title' => $description,
          'notes' => $notes,
        ]),
      ])->save();
    }

    // Take the user back to the summary page.
    $route = 'ebms_summary.page';
    $parameters = ['summary_page' => $page_id];
    $query_parameters = $this->getRequest()->query->all();
    $options = ['query' => $query_parameters];
    $form_state->setRedirect($route, $parameters, $options);
  }

    /**
   * {@inheritdoc}
   */
  public function validateForm(array &$form, FormStateInterface $form_state) {
    $validators = ['file_validate_extensions' => ['doc docx']];
    $file = file_save_upload('file', $validators, 'public://', 0);
    if (empty($file)) {
      $form_state->setErrorByName('file', 'File is required.');
    }
    else {
      $file->setPermanent();
      $file->save();
      $form_state->setValue('file-id', $file->id());
    }
  }

}
