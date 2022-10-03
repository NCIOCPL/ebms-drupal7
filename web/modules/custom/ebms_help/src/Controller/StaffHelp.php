<?php

namespace Drupal\ebms_help\Controller;

use Drupal\Core\Controller\ControllerBase;

/**
 * Provide information about how to use the EBMS.
 */
class StaffHelp extends ControllerBase {

  /**
   * Stub for now.
   */
  public function display(): array {
    return [
      '#title' => "NCI Staff Guide to Using the EBMS",
      '#markup' => '<p><img src="/themes/custom/ebms/images/help.jpg" alt="Help"><br>Help is on the way!!! 🚑</p><p>Use the links on the left (below on mobile devices) to find instructions for using the different parts of the web site.</p><p>To view the board member help pages, visit <a href="/help">https://ebms.nci.nih.gov/help</a></p>',
    ];
  }

}
