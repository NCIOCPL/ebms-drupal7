<?php

namespace Drupal\ebms_core;

use Drupal\Core\Entity\EntityStorageInterface;
use Drupal\Core\Entity\EntityTypeManagerInterface;

/**
 * Service for looking up taxonomy values.
 */
class TermLookup {

  /**
   * The taxonomy_term entity storage handler.
   *
   * @var \Drupal\Core\Entity\EntityStorageInterface
   */
  protected EntityStorageInterface $termStorage;

  /**
   * Constructs a new \Drupal\ebms_core\TermLookup object.
   *
   * @param \Drupal\Core\Entity\EntityTypeManagerInterface $entity_type_manager
   *   The entity type manager.
   */
  public function __construct(EntityTypeManagerInterface $entity_type_manager) {
    $this->termStorage = $entity_type_manager->getStorage('taxonomy_term');
  }

  /**
   * Find `State` from its text ID.
   *
   * @param string $text_id
   *   Stable string ID for the state.
   *
   * @return object|null
   *   State object.
   */
  public function getState(string $text_id): ?object {
    $query = $this->termStorage->getQuery()->accessCheck(FALSE);
    $query->condition('vid', 'states');
    $query->condition('field_text_id', $text_id);
    $ids = $query->execute();
    if (empty($ids)) {
      return NULL;
    }
    $tid = reset($ids);
    return $this->termStorage->load($tid);
  }

}
