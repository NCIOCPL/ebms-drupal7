<?php

namespace Drupal\ebms_core\Controller;

use Drupal\Core\Controller\ControllerBase;

/**
 * Explain how to log into the EBMS.
 */
class Login extends ControllerBase {

  /**
   * Show the login instructions and a button to the real login page.
   */
  public function display(): array {
    $url = \Drupal::moduleHandler()->moduleExists('externalauth') ? '/ssologin' : '/user/login';
    return [
      '#title' => '',
      '#attached' => ['library' => ['ebms_core/login']],
      'instructions' => [
        '#theme' => 'ebms_login',
        '#url' => $url,
      ],
    ];
  }

}
