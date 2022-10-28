<?php

namespace Drupal\ebms_topic;

use Drupal\Core\Entity\EntityInterface;
use Drupal\Core\Entity\EntityListBuilder;
use Drupal\Core\Link;

/**
 * Defines a class to build a listing of EBMS topics.
 *
 * @ingroup ebms
 */
class ListBuilder extends EntityListBuilder {

  /**
   * {@inheritdoc}
   */
  public function buildHeader(): array {
    $header['name'] = $this->t('Name');
    $header['board'] = $this->t('Board');
    return $header + parent::buildHeader();
  }

  /**
   * {@inheritdoc}
   */
  public function buildRow(EntityInterface $entity): array {
    $row['name'] = Link::createFromRoute(
      $entity->label(),
      'entity.ebms_topic.edit_form',
      ['ebms_topic' => $entity->id()]
    );
    $row['board'] = $entity->board->entity->name->value;
    return $row + parent::buildRow($entity);
  }

  /**
   * {@inheritdoc}
   */
  public function getEntityIds() {
    $storage = $this->getStorage();
    $query = $storage->getQuery()->accessCheck(FALSE);
    $query->sort('name', 'ASC');
    $parms = \Drupal::request()->query;
    return $query->execute();
  }

}
