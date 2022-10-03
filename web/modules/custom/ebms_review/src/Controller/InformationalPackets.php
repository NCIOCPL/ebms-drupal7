<?php

namespace Drupal\ebms_review\Controller;

use Drupal\Core\Controller\ControllerBase;
use Drupal\Core\Link;
use Drupal\user\Entity\User;

/**
 * Provide a list of FYI packets for a board member.
 *
 * An "FYI Packet" originally referred to a packet created for a topic
 * for which the board member in question was designated as a default
 * reviewer but was not actually assigned to review the articles in the
 * packet. These packets were shown to the board member solely "for his
 * or her information."
 *
 * A second meaning for the phrase "FYI Packet" arose with Jira ticket
 * OCEEBMS-232, which created a class of packet all of whose articles
 * were tagged as "FYI" (that is not intended for review at all).
 *
 * So this page assembles a list of all of the packets which match the
 * original description for an "FYI Packet" and augments that list with
 * the packets assigned to the current board member and all of whose
 * articles are tagged as "FYI."
 */
class InformationalPackets extends ControllerBase {

  /**
   * Create the render array for the packets list.
   */
  public function display() {

    // We're not interested in packets created more than two years ago.
    $cutoff = new \DateTime();
    $cutoff->sub(new \DateInterval('P2Y'));
    $cutoff = $cutoff->format('Y-m-d');

    // Find the packets whose topics are in this reviewer's wheelhouse,
    // but which aren't assigned to her.
    $storage = $this->entityTypeManager()->getStorage('ebms_packet');
    $uid = $this->currentUser()->id();
    $user = User::load($uid);
    $topics_ids = [];
    foreach ($user->topics as $topic) {
      $topics_ids[] = $topic->target_id;
    }
    $packets = [];
    if (!empty($topics_ids)) {
      $query = $storage->getQuery()->accessCheck(FALSE);
      $query->condition('active', 1);
      $query->condition('created', $cutoff, '>=');
      $query->condition('reviewers', $uid);
      $assigned = array_values($query->execute()) ?: [0];
      $query = $storage->getQuery()->accessCheck(FALSE);
      $query->condition('active', 1);
      $query->condition('created', $cutoff, '>=');
      $query->condition('id', $assigned, 'NOT IN');
      $query->condition('topic', $topics_ids, 'IN');
      foreach ($storage->loadMultiple($query->execute()) as $packet) {
        $packets[] = [
          $packet->title->value,
          $packet->created->value,
          $packet->articles->count(),
          $packet->id(),
        ];
      }
    }

    // Now add the packets which have nothing but FYI articles, and are
    // assigned to this reviewer.
    $query = $storage->getQuery()->accessCheck(FALSE);
    $query->condition('active', 1);
    $query->condition('created', $cutoff, '>=');
    $query->condition('reviewers', $uid);
    $query->addTag('packet_has_non_fyi_article');
    $have_non_fyi = array_values($query->execute()) ?: [0];
    $query = $storage->getQuery()->accessCheck(FALSE);
    $query->condition('active', 1);
    $query->condition('created', $cutoff, '>=');
    $query->condition('reviewers', $uid);
    $query->condition('id', $have_non_fyi, 'NOT IN');
    foreach ($storage->loadMultiple($query->execute()) as $packet) {
      $packets[] = [
        $packet->title->value,
        $packet->created->value,
        $packet->articles->count(),
        $packet->id(),
      ];
    }

    // Sort the rows by hand.
    usort($packets, function(array &$a, array &$b): int {
      return $b[1] <=> $a[1] ?: $a[0] <=> $b[0];
    });

    // Inject links for the first column.
    foreach ($packets as &$row) {
      $name = $row[0];
      $packet_id = $row[3];
      unset($row[3]);
      $row[0] = Link::createFromRoute($name, 'ebms_review.fyi_packet', ['packet_id' => $packet_id]);
    }
    // Assemble the render array for the page.
    return [
      '#title' => 'FYI Packets',
      '#cache' => ['max-age' => 0],
      'packets' => [
        '#theme' => 'table',
        '#rows' => $packets,
        '#header' => ['Packet Name', 'Date Created', 'Number of Articles'],
        '#empty' => 'No FYI packets have been created in the past two years.',
      ],
    ];
  }

}
