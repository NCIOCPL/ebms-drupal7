<?php

namespace Drupal\ebms_group;

use Drupal\Core\Entity\EntityInterface;
use Drupal\Core\Entity\EntityListBuilder;
use Drupal\Core\Link;

/**
 * Defines a class to build a listing of Group entities.
 *
 * @ingroup ebms
 */
class ListBuilder extends EntityListBuilder {

  /**
   * {@inheritdoc}
   */
  public function buildHeader(): array {
    $header['id'] = $this->t('Group ID');
    $header['name'] = $this->t('Name');
    return $header + parent::buildHeader();
  }

  /**
   * {@inheritdoc}
   */
  public function getTitle(): string {
    // Broken, I think. https://www.drupal.org/project/drupal/issues/2892334.
    return $this->t('Groups');
  }

  /**
   * {@inheritdoc}
   */
  public function buildRow(EntityInterface $entity): array {
    /** @var \Drupal\ebms_group\Entity\Group $entity */
    $row['id'] = $entity->id();
    $row['name'] = Link::createFromRoute(
      $entity->label(),
      'entity.ebms_group.edit_form',
      ['ebms_group' => $entity->id()]
    );
    return $row + parent::buildRow($entity);
  }

}
