<?php

namespace Drupal\ebms_doc;

use Drupal\Core\Entity\EntityInterface;
use Drupal\Core\Entity\EntityListBuilder;
use Drupal\Core\Link;

/**
 * Defines a class to build a listing of EBMS articles.
 *
 * @ingroup ebms
 */
class DocListBuilder extends EntityListBuilder {

  /**
   * {@inheritdoc}
   */
  public function buildHeader(): array {
    $header['id'] = $this->t('ID');
    $header['file'] = $this->t('File');
    $header['posted'] = $this->t('Posted');
    $header['size'] = $this->t('Size');
    $header['type'] = $this->t('Type');
    $header['tags'] = $this->t('Tags');
    return $header;
  }

  /**
   * {@inheritdoc}
   */
  public function buildRow(EntityInterface $entity): array {
    $posted = new \DateTime($entity->posted->value);
    $tags = [];
    foreach ($entity->tags as $tag) {
      $tags[] = $tag->entity->getName();
    }
    sort($tags);
    $row['id'] = $entity->id();
    $row['file'] = Link::createFromRoute(
      $entity->file->entity->getFilename(),
      'entity.ebms_doc.edit_form',
      ['ebms_doc' => $entity->id()]
    );
    $row['posted'] = $posted->format('Y-m-d H:i:s');
    $row['size'] = $entity->file->entity->getSize();
    $row['type'] = $entity->file->entity->getMimeType();
    $row['tags'] = implode(', ', $tags);
    return $row;
  }

}
