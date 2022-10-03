<?php

namespace Drupal\ebms_summary\Form;

use Drupal\Core\Form\FormBase;
use Drupal\Core\Form\FormStateInterface;
use Drupal\ebms_summary\Entity\SummaryPage;

/**
 * Form for removing a summary link.
 *
 * @ingroup ebms
 */
class SummaryLinkDeletionForm extends FormBase {

  /**
   * {@inheritdoc}
   */
  public function getFormId(): string {
    return 'ebms_summary_link_deletion_form';
  }

  /**
  * {@inheritdoc}
  */
  public function buildForm(array $form, FormStateInterface $form_state, $summary_page = NULL, $delta = NULL): array {
    $name = htmlspecialchars($summary_page->links[$delta]->title);
    $form_state->setValue('foo', 'bar');
    return [
      '#title' => 'Delete this summary link?',
      'page-id' => [
        '#type' => 'hidden',
        '#value' => $summary_page->id(),
      ],
      'delta' => [
        '#type' => 'hidden',
        '#value' => $delta,
      ],
      'instructions' => [
        '#markup' => "<p>Deleting summary link for <em>$name</em>. This action cannot be undone.</p>",
      ],
      'submit' => [
        '#type' => 'submit',
        '#value' => 'Delete',
      ],
      'cancel' => [
        '#type' => 'submit',
        '#value' => 'Cancel',
      ],
    ];
  }

  /**
   * {@inheritdoc}
   */
  public function submitForm(array &$form, FormStateInterface $form_state) {
    $trigger = $form_state->getTriggeringElement()['#value'];
    if ($trigger === 'Delete') {
      $page_id = $form_state->getValue('page-id');
      $delta = $form_state->getValue('delta');
      $page = SummaryPage::load($page_id);
      $name = $page->links[$delta]->title;
      $page->links->removeItem($delta);
      $page->save();
      $this->messenger()->addMessage("Dropped link for '$name'.");
    }
    $page_id = $form_state->getValue('page-id');
    $options = ['query' => $this->getRequest()->query->all()];
    $parms = ['summary_page' => $page_id];
    $form_state->setRedirect('ebms_summary.page', $parms, $options);
  }

}
