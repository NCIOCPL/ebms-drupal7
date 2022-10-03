<?php

namespace Drupal\ebms_summary\Form;

use Drupal\Core\Form\FormBase;
use Drupal\Core\Form\FormStateInterface;
use Drupal\ebms_doc\Entity\Doc;
use Drupal\ebms_message\Entity\Message;
use Drupal\ebms_summary\Entity\SummaryPage;

/**
 * Form for adding a supporting document to a board summaries page.
 *
 * @ingroup ebms
 */
class SummaryManagerDocForm extends FormBase {

  /**
   * {@inheritdoc}
   */
  public function getFormId(): string {
    return 'ebms_summary_manager_doc_form';
  }

  /**
  * {@inheritdoc}
  */
  public function buildForm(array $form, FormStateInterface $form_state, $summary_page = NULL): array {
    $page_name = $summary_page->name->value;
    $eligible_docs = $summary_page->eligibleDocs();
    return [
      '#title' => "Post NCI Document for $page_name Page",
      'page-id' => [
        '#type' => 'hidden',
        '#value' => $summary_page->id(),
      ],
      'doc' => [
        '#type' => 'radios',
        '#title' => 'Document',
        '#options' => $eligible_docs,
        '#description' => "Select NCI document to be linked from this summaries page.",
        '#required' => TRUE,
      ],
      'notes' => [
        '#type' => 'textarea',
        '#title' => 'Notes',
        '#description' => "Add optional notes to be displayed with the document's link.",
      ],
      'submit' => [
        '#type' => 'submit',
        '#value' => 'Save',
      ],
      'cancel' => [
        '#type' => 'submit',
        '#value' => 'Cancel',
        '#limit_validation_errors' => [],
        '#submit' => ['::cancelSubmit'],
      ],
    ];
  }

  /**
   * Return to the summary page.
   *
   * @param array $form
   *   Ignored.
   * @param FormStateInterface $form_state
   *   Used for the redirection.
   */
  public function cancelSubmit(array &$form, FormStateInterface $form_state) {
    $route_match = $this->getRouteMatch();
    $parms = ['summary_page' => $route_match->getRawParameter('summary_page')];
    $options = ['query' => $this->getRequest()->query->all()];
    $form_state->setRedirect('ebms_summary.page', $parms, $options);
  }

  /**
   * {@inheritdoc}
   */
  public function submitForm(array &$form, FormStateInterface $form_state) {

    // Save the document.
    $page_id = $form_state->getValue('page-id');
    $doc_id = $form_state->getValue('doc');
    $notes = $form_state->getValue('notes');
    $values = [
      'doc' => $doc_id,
      'notes' => $notes,
      'active' => 1,
    ];
    $summary_page = SummaryPage::load($page_id);
    $summary_page->manager_docs[] = $values;
    $summary_page->save();
    $this->messenger()->addMessage('Added NCI document.');

    // Create a notification message for the home page.
    $storage = \Drupal::entityTypeManager()->getStorage('ebms_board_summaries');
    $ids = $storage->getQuery()
      ->accessCheck(FALSE)
      ->condition('pages', $page_id)
      ->execute();
    if (!empty($ids)) {
      $board_id = $storage->load(reset($ids))->board->target_id;
      $doc = Doc::load($doc_id);
      $file = $doc->file->entity;
      $title = $doc->description->value;
      if (empty($title)) {
        $title = $file->filename->value;
      }
      Message::create([
        'message_type' => Message::SUMMARY_POSTED,
        'user' => $this->currentUser()->id(),
        'posted' => date('Y-m-d H:i:s'),
        'boards' => [$board_id],
        'extra_values' => json_encode([
          'summary_url' => $file->createFileUrl(),
          'title' => $title,
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

}
