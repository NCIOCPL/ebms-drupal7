<?php

namespace Drupal\ebms_meeting;

use Drupal\Core\Entity\EntityInterface;
use Drupal\Core\Entity\EntityListBuilder;
use Drupal\Core\Link;

/**
 * Defines a class to build a listing of scheduled meetings.
 *
 * @ingroup ebms
 */
class ListBuilder extends EntityListBuilder {

  /**
   * {@inheritdoc}
   */
  public function buildHeader(): array {
    $header['id'] = $this->t('ID');
    $header['name'] = $this->t('Name');
    $header['date'] = $this->t('Date');
    $header['type'] = $this->t('Type');
    $header['category'] = $this->t('Category');
    $header['status'] = $this->t('Status');
    return $header;
  }

  /**
   * {@inheritdoc}
   */
  protected function getEntityIds(): array {
    $query = $this->getStorage()->getQuery()->accessCheck(FALSE);
    $query->sort('id', 'DESC');
    $query->pager(25);
    return $query->execute();
  }

  /**
   * {@inheritdoc}
   */
  public function buildRow(EntityInterface $entity): array {
    $start = new \DateTime($entity->dates->value);
    $row['id'] = $entity->id();
    $row['name'] = Link::createFromRoute(
      $entity->name->value,
      'ebms_meeting.edit_meeting',
      ['meeting' => $entity->id()]
    );
    $row['date'] = $start->format('Y-m-d');
    $row['type'] = $entity->type->entity->name->value;
    $row['category'] = $entity->category->entity->name->value;
    $row['status'] = $entity->status->entity->name->value;
    return $row;
  }

}
