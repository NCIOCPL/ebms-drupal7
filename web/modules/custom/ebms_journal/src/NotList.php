<?php

namespace Drupal\ebms_journal;

use Drupal\Core\Entity\EntityStorageInterface;
use Drupal\Core\Entity\EntityTypeManagerInterface;

/**
 * Service for identifying blacklisted journals.
 */
class NotList {

  /**
   * The taxonomy_term entity storage handler.
   *
   * @var \Drupal\Core\Entity\EntityStorageInterface
   */
  protected EntityStorageInterface $storage;

  /**
   * Constructs a new \Drupal\ebms_core\NotList object.
   *
   * @param \Drupal\Core\Entity\EntityTypeManagerInterface $entity_type_manager
   *   The entity type manager.
   */
  public function __construct(EntityTypeManagerInterface $entity_type_manager) {
    $this->storage = $entity_type_manager->getStorage('ebms_journal');
  }

  /**
   * Find the journals this board doesn't want.
   *
   * @param int $board_id
   *   ID of the board whose rejected journals we collect.
   */
  public function getNotList(int $board_id): array {
    $now = date('Y-m-d H:i:s');
    $query = $this->storage->getQuery()->accessCheck(FALSE);
    $query->condition('not_lists.board', $board_id);
    $query->condition('not_lists.start', $now, '<=');
    $ids = $query->execute();
    $journals = $this->storage->loadMultiple($ids);
    $not_list = [];
    foreach ($journals as $journal) {
      $not_list[] = $journal->source_id->value;
    }
    return array_unique($not_list);
  }

}
