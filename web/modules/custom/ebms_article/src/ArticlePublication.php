<?php

namespace Drupal\ebms_article;

use Drupal\Core\Field\FieldItemList;
use Drupal\Core\Field\FieldItemListInterface;
use Drupal\Core\TypedData\ComputedItemListTrait;

/**
 * Computed string showing the article's publication information.
 */
class ArticlePublication extends FieldItemList implements FieldItemListInterface {

  use ComputedItemListTrait;

  /**
   * {@inheritdoc}
   */
  public function computeValue() {
    // \Drupal\ebms\Entity\Article
    $item = $this->getEntity();
    $this->list[0] = $this->createItem(0, $item->getLabel());
  }

}
