<?php

namespace Drupal\ebms_travel\Controller;

use Drupal\Core\Controller\ControllerBase;

/**
 * Landing page for travel information/forms.
 */
class MenuCards extends ControllerBase {

  /**
   * Display the cards for the travel landing page.
   */
  public function display(): array {
    return [
      '#title' => '',
      '#attached' => ['library' => ['ebms_travel/landing']],
      'cards' => [
        '#theme' => 'travel_landing_page',
        '#cache' => ['max-age' => 0],
      ],
    ];
  }

}
