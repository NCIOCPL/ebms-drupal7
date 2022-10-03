<?php

namespace Drupal\ebms_core\Form;

use Drupal\Core\Form\FormStateInterface;
use Drupal\system\Form\SiteInformationForm;

/**
 * Add our own fields.
 */
class ExtendedSiteInformationForm extends SiteInformationForm {

  /**
   * Custom configuration value names.
   */
  const CUSTOM_CONFIG = [
    'debug_level',
    'dev_notif_addr',
    'pubmed_missing_article_report_recips',
  ];

  /**
   * {@inheritdoc}
   */
  public function buildForm(array $form, FormStateInterface $form_state): array {
    $site_config = $this->config('ebms_core.settings');
    $form =  parent::buildForm($form, $form_state);
    foreach ($form as &$section) {
      if (is_array($section)) {
        if (!empty($section['#type']) && $section['#type'] === 'details') {
          if (!empty($section['#open'])) {
            $section['#open'] = FALSE;
          }
        }
      }
    }
    $form['ebms_logging'] = [
      '#type' => 'details',
      '#title' => 'Debug logging',
      'debug_level' => [
        '#type' => 'select',
        '#title' => 'Debug Level',
        '#options' => [
          0 => 'No debug logging',
          1 => 'Basic activity logging',
          2 => 'Standard debug logging',
          3 => 'Verbose debug logging',
        ],
        '#default_value' => $site_config->get('debug_level') ?: 0,
        '#description' => 'How much debug logging should we do? This is separate logging to the file system, to prevent cluttering up the watchdog logs.',
      ],
    ];
    $form['ebms_email_addresses'] = [
      '#type' => 'details',
      '#title' => 'Report recipients',
      'dev_notif_addr' => [
        '#type' => 'textfield',
        '#title' => 'Developer notification address',
        '#description' => 'Used primarily as a fallback when no other suitable address is available, or for non-production tier testing. Separate multiple addresses with commas.',
        '#default_value' => $site_config->get('dev_notif_addr'),
      ],
      'pubmed_missing_article_report_recips' => [
        '#type' => 'textfield',
        '#title' => 'Missing article report recipients',
        '#description' => 'Email addresses for the report of articles which NLM sent us in the past but can no longer find. Separate multiple addresses with commas.',
        '#default_value' => $site_config->get('pubmed_missing_article_report_recips'),
      ],
    ];
    return $form;
  }

  /**
   * {@inheritdoc}
   */
  public function submitForm(array &$form, FormStateInterface $form_state) {
    $site_config = $this->configFactory()->getEditable('ebms_core.settings');
    foreach (self::CUSTOM_CONFIG as $name) {
      $site_config->set($name, $form_state->getValue($name));
    }
    $site_config->save();
    parent::submitForm($form, $form_state);
  }

}
