<?php

namespace Drupal\ebms_travel\Form;

use Drupal\Core\Entity\EntityTypeManagerInterface;
use Drupal\Core\Form\FormBase;
use Drupal\Core\Form\FormStateInterface;
use Symfony\Component\DependencyInjection\ContainerInterface;

/**
 * Manage the instructions blocks for the travel forms.
 *
 * @ingroup ebms
 */
class ConfigurationForm extends FormBase {

  /**
   * The entity type manager.
   *
   * @var \Drupal\Core\Entity\EntityTypeManagerInterface
   */
  protected EntityTypeManagerInterface $entityTypeManager;

  /**
   * {@inheritdoc}
   */
  public static function create(ContainerInterface $container): ConfigurationForm {
    $instance = parent::create($container);
    $instance->entityTypeManager = $container->get('entity_type.manager');
    return $instance;
  }

  /**
   * {@inheritdoc}
   */
  public function getFormId(): string {
    return 'ebms_travel_configuration_form';
  }

  /**
   * {@inheritdoc}
   */
  public function buildForm(array $form, FormStateInterface $form_state): array {
    $hotel_instructions = $this->config('ebms_travel.instructions')->get('hotel');
    $reimbursement_instructions = $this->config('ebms_travel.instructions')->get('reimbursement');
    $travel_manager = $this->config('ebms_travel.email')->get('travel_manager');
    $developers = $this->config('ebms_travel.email')->get('developers');
    return [
      '#title' => 'Travel Configuration',
      'hotel' => [
        '#type' => 'text_format',
        '#title' => 'Hotel Instructions',
        '#description' => 'Rich-text paragraphs to be displayed at the top of the hotel reservation request form.',
        '#default_value' => $hotel_instructions,
        '#format' => 'filtered_html',
        '#required' => TRUE,
      ],
      'reimbursement' => [
        '#type' => 'text_format',
        '#title' => 'Reimbursement Instructions',
        '#description' => 'Rich-text paragraphs to be displayed at the top of the travel reimbursement request form.',
        '#default_value' => $reimbursement_instructions,
        '#format' => 'filtered_html',
        '#required' => TRUE,
      ],
      'travel-manager' => [
        '#type' => 'textfield',
        '#title' => 'Travel Manager Email Address(es)',
        '#description' => 'Plain (sally@example.com) or with display name (Sally &lt;sally@example.com&gt;). Separate multiple addresses with commas.',
        '#default_value' => $travel_manager,
      ],
      'developers' => [
        '#type' => 'textfield',
        '#title' => 'Developer/Tester Email Address(es)',
        '#description' => 'Used for sending email messages from non-production servers. Same formatting as for the travel manager(s).',
        '#default_value' => $developers,
      ],
      'submit' => [
        '#type' => 'submit',
        '#value' => 'Save',
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
    if ($trigger === 'Save') {
      $config = $this->configFactory()->getEditable('ebms_travel.instructions');
      $config->set('hotel', $form_state->getValue('hotel')['value']);
      $config->set('reimbursement', $form_state->getValue('reimbursement')['value']);
      $config->save();
      $config = $this->configFactory()->getEditable('ebms_travel.email');
      $config->set('travel_manager', $form_state->getValue('travel-manager'));
      $config->set('developers', $form_state->getValue('developers'));
      $config->save();
      $this->messenger()->addMessage('Saved travel configuration.');
    }
    $form_state->setRedirect('ebms_travel.landing_page');
  }

}
