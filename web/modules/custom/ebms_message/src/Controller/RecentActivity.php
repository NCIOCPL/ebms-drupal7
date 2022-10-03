<?php

namespace Drupal\ebms_message\Controller;

use Drupal\Core\Controller\ControllerBase;
use Drupal\ebms_message\Entity\Message;

/**
 * Show the recent activity for a particular group of messages.
 */
class RecentActivity extends ControllerBase {

  /**
   * Get the messages for the requested group.
   */
  public function display(string $group): array {
    list($_message_types, $days) = Message::GROUPS[$group];
    $query = Message::createQuery($group);
    $ids = $query->pager(10)->execute();
    $messages = [];
    foreach ($ids as $id) {
      $message = Message::load($id);
      $messages[] = [
        '#theme' => 'ebms_message',
        '#message' => $message,
      ];
    }
    $capped_name = ucfirst($group);
    return [
      '#title' => "Recent $capped_name Activity",
      '#attached' => ['library' => ['ebms_message/activity']],
      '#cache' => ['max-age' => 0],
      'messages' => [
        '#theme' => 'item_list',
        '#list_type' => 'ul',
        '#items' => $messages,
        '#empty' => "There is no recent $group activity to display.",
      ],
      'pager' => [
        '#type' => 'pager',
      ],
    ];
  }

}
