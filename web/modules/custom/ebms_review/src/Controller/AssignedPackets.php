<?php

namespace Drupal\ebms_review\Controller;

use Drupal\Core\Controller\ControllerBase;
use Drupal\Core\Url;
use Drupal\user\Entity\User;

/**
 * Provide a list of packets assigned for review to the current user.
 */
class AssignedPackets extends ControllerBase {

  /**
   * Create the render array for the packets list.
   */
  public function display() {

    // Start with some defaults.
    $title = 'Assigned Packets';
    $options = ['query' => \Drupal::request()->query->all()];
    $uid = $this->currentUser()->id();

    // Override defaults if working on behalf of a board member.
    $obo = $options['query']['obo'] ?? '';
    if (!empty($obo)) {
      $uid = $obo;
      $user = User::load($uid);
      $name = $user->name->value;
      $title .= " for $name";
    }

    // Find the review packets for the board member.
    $storage = $this->entityTypeManager()->getStorage('ebms_packet');
    $query = $storage->getQuery()->accessCheck(FALSE);
    $query->condition('active', 1);
    $query->condition('reviewers', $uid);
    $query->condition('articles.entity.dropped', 0);
    $query->addTag('packets_with_unreviewed_articles');
    $query->sort('created', 'DESC');
    $query->pager();
    $packets = $storage->loadMultiple($query->execute());
    $items = [];
    foreach ($packets as $packet) {
      $articles = count($packet->articles);
      $s = $articles === 1 ? '' : 's';
      $items[] = [
        'url' => Url::fromRoute('ebms_review.assigned_packet', ['packet_id' => $packet->id()], $options),
        'name' => $packet->title->value,
        'count' => "$articles article$s",
      ];
    }

    // Assemble the render array for the page.
    return [
      '#title' => $title,
      '#cache' => ['max-age' => 0],
      'packets' => [
        '#theme' => 'assigned_packets',
        '#packets' => $items,
      ],
      'pager' => [
        '#type' => 'pager',
      ],
    ];
  }

}
