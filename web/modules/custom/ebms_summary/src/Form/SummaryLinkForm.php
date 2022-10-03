<?php

namespace Drupal\ebms_summary\Form;

use Drupal\Core\Entity\EntityTypeManagerInterface;
use Drupal\Core\Form\FormBase;
use Drupal\Core\Form\FormStateInterface;
use Symfony\Component\DependencyInjection\ContainerInterface;
use Drupal\ebms_summary\Entity\SummaryPage;

/**
 * Form for adding/editing a summary link.
 *
 * @ingroup ebms
 */
class SummaryLinkForm extends FormBase {

  /**
   * The entity type manager.
   *
   * @var \Drupal\Core\Entity\EntityTypeManagerInterface
   */
  protected EntityTypeManagerInterface $entityTypeManager;

  /**
   * {@inheritdoc}
   */
  public static function create(ContainerInterface $container): SummaryLinkForm {
    $instance = parent::create($container);
    $instance->entityTypeManager = $container->get('entity_type.manager');
    return $instance;
  }

  /**
   * {@inheritdoc}
   */
  public function getFormId(): string {
    return 'ebms_summary_link_form';
  }

  /**
  * {@inheritdoc}
  */
  public function buildForm(array $form, FormStateInterface $form_state, $summary_page = NULL, $delta = NULL): array {
    $name = $summary_page->name->value;
    if (is_null($delta)) {
      $title = "Add $name Summary Link";
      $url = $text = '';
    }
    else {
      $title = "Edit $name Summary Link";
      $url = $summary_page->links[$delta]->uri;
      $text = $summary_page->links[$delta]->title;
    }
    return [
      '#title' => $title,
      'page-id' => [
        '#type' => 'hidden',
        '#value' => $summary_page->id(),
      ],
      'delta' => [
        '#type' => 'hidden',
        '#value' => $delta,
      ],
      'display' => [
        '#type' => 'textfield',
        '#title' => 'Text',
        '#description' => "The display name for this summary's link.",
        '#required' => TRUE,
        '#default_value' => $text,
      ],
      'url' => [
        '#type' => 'textfield',
        '#title' => 'URL',
        '#description' => 'The web address for this summary.',
        '#required' => TRUE,
        '#default_value' => $url,
      ],
      'submit' => [
        '#type' => 'submit',
        '#value' => 'Save',
      ],
      'cancel' => [
        '#type' => 'submit',
        '#value' => 'Cancel',
        '#limit_validation_errors' => [['display', 'url']],
        '#submit' => [[$this, 'nothingToSeeHere']],
      ],
    ];
  }

  /**
   * Make sure Drupal doesn't invoke the real submit handler for cancels.
   *
   * As usual, Drupal's documentation is pretty thin here. Despite the
   * commonly given advice, assigning an empty array to '#submit' in the
   * definition of the Cancel button does not accomplish the goal. Drupal
   * is so weird. 😩
   *
   * @param array $form
   *   Ignored.
   * @param FormStateInterface $form_state
   *   Ignored.
   */
  public function nothingToSeeHere(array &$form, FormStateInterface $form_state) {}

  /**
   * {@inheritdoc}
   */
  public function validateForm(array &$form, FormStateInterface $form_state) {
    $trigger = $form_state->getTriggeringElement()['#value'];
    if ($trigger === 'Cancel') {
      $page_id = $form_state->getValue('page-id');
      $options = ['query' => $this->getRequest()->query->all()];
      $parms = ['summary_page' => $page_id];
      $form_state->setRedirect('ebms_summary.page', $parms, $options);
    }
  }

  /**
   * {@inheritdoc}
   */
  public function submitForm(array &$form, FormStateInterface $form_state) {
    $page_id = $form_state->getValue('page-id');
    $delta = $form_state->getValue('delta');
    $display = $form_state->getValue('display');
    $url = $form_state->getValue('url');
    $page = SummaryPage::load($page_id);
    if (is_numeric($delta)) {
      $page->links[$delta] = [
        'uri' => $url,
        'title' => $display,
      ];
    }
    else {
      $page->links[] = [
        'uri' => $url,
        'title' => $display,
      ];
    }
    $page->save();
    $this->messenger()->addMessage("Saved link '$display'.");
    $options = ['query' => $this->getRequest()->query->all()];
    $parms = ['summary_page' => $page_id];
    $form_state->setRedirect('ebms_summary.page', $parms, $options);
  }

}
