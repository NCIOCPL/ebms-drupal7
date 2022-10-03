<?php

namespace Drupal\ebms_help\Controller;

use Drupal\Core\Controller\ControllerBase;
use Drupal\node\Entity\Node;
use Symfony\Component\HttpFoundation\Response;

/**
 * Provide information about how to use the EBMS.
 */
class BoardMemberManualSinglePage extends ControllerBase {

  const SECTIONS = [
    'Board Members User Manual — Login/Logout',
    'Board Members User Manual — Home Page',
    'Board Members User Manual — Article Search',
    'Board Members User Manual — Calendar',
    'Board Members User Manual — Packets',
    'Board Members User Manual — Summaries',
    'Board Members User Manual — Travel',
    'Board Members User Manual — Profile',
  ];

  /**
   * Show the board members' user manual as a single page.
   */
  public function display(): Response {
    $sections = [];
    $storage = $this->entityTypeManager()->getStorage('node');
    foreach (self::SECTIONS as $title) {
      $query = $storage->getQuery()->accessCheck(FALSE);
      $query->condition('title', $title);
      $ids = $query->execute();
      if (!empty($ids)) {
        $node = Node::load(reset($ids));
        $values = [
          'title' => str_replace('Board Members User Manual — ', '', $title),
          'body' => $node->body->value,
        ];
        $sections[] = $values;
      }
    }
    $manual = [
      '#theme' => 'user_manual',
      '#title' => "Board Member’s Guide to Using the EBMS",
      '#sections' => $sections,
    ];
    $page = \Drupal::service('renderer')->render($manual);
    $response = new Response($page);
    return $response;
}

}
