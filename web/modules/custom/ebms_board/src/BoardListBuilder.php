<?php

namespace Drupal\ebms_board;

use Drupal\Core\Entity\EntityInterface;
use Drupal\Core\Entity\EntityListBuilder;
use Drupal\Core\Link;

/**
 * Defines a class to build a listing of PDQ Board entities.
 *
 * @ingroup ebms
 */
class BoardListBuilder extends EntityListBuilder {

  /**
   * {@inheritdoc}
   */
  public function buildHeader(): array {
    $header['id'] = $this->t('PDQ Board ID');
    $header['name'] = $this->t('Name');
    return $header + parent::buildHeader();
  }

  /**
   * {@inheritdoc}
   */
  public function getTitle(): string {
    // Broken, I think. https://www.drupal.org/project/drupal/issues/2892334.
    return $this->t('PDQ Boards');
  }

  /**
   * {@inheritdoc}
   */
  public function buildRow(EntityInterface $entity): array {
    /** @var \Drupal\ebms_board\Entity\Board $entity */
    $row['id'] = $entity->id();
    $row['name'] = Link::createFromRoute(
      $entity->label(),
      'entity.ebms_board.edit_form',
      ['ebms_board' => $entity->id()]
    );
    return $row + parent::buildRow($entity);
  }

}
