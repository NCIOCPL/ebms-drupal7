<?php

namespace Drupal\ebms_review\Controller;

use Drupal\Core\Controller\ControllerBase;
use Drupal\ebms_review\Entity\Packet;
use Symfony\Component\HttpFoundation\Response;

/**
 * AJAX callback for starred packets settings.
 */
class PacketStar extends ControllerBase {

  /**
   * Modify the packet's starred setting and return the star's new render array.
   */
  public function update(int $packet_id, int $flag) {
    $packet = Packet::load($packet_id);
    $packet->set('starred', $flag);
    $packet->save();
    $star = [
      '#theme' => 'packet_star',
      '#id' => $packet_id,
      '#starred' => $flag,
    ];
    $html = \Drupal::service('renderer')->render($star);
    return new Response($html);
  }
}
