<?php

namespace Drupal\ebms_doc\Controller;

use Drupal\Core\Controller\ControllerBase;
use Drupal\ebms_doc\Entity\Doc;

/**
 * Show individual document information.
 */
class DocController extends ControllerBase {

  /**
   * Display document information.
   */
  public function view(Doc $doc): array {
    return [
      '#markup' => 'This is a stub',
    ];
  }

}
