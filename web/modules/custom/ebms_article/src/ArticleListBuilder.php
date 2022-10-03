<?php

namespace Drupal\ebms_article;

use Drupal\Core\Entity\EntityInterface;
use Drupal\Core\Entity\EntityListBuilder;
use Drupal\Core\Link;

/**
 * Defines a class to build a listing of EBMS articles.
 *
 * @ingroup ebms
 */
class ArticleListBuilder extends EntityListBuilder {

  /**
   * {@inheritdoc}
   */
  public function buildHeader(): array {
    $header['id'] = $this->t('Article');
    $header['source-id'] = $this->t('PMID');
    $header['citation'] = $this->t('Citation');
    $header['import-date'] = $this->t('Imported');
    return $header + parent::buildHeader();
  }

  /**
   * {@inheritdoc}
   */
  public function render(): array {
    $build['form'] = \Drupal::formBuilder()->getForm('Drupal\ebms_article\Form\ArticleListForm');
    $build += parent::render();
    return $build;
  }

  /**
   * {@inheritdoc}
   */
  public function getTitle(): string {
    // Broken, I think. https://www.drupal.org/project/drupal/issues/2892334.
    return $this->t('Articles');
  }

  /**
   * {@inheritdoc}
   */
  public function getEntityIds() {
    $storage = $this->getStorage();
    $query = $storage->getQuery()->accessCheck(FALSE);
    $query->sort('import_date', 'DESC');
    $parms = \Drupal::request()->query;
    $board = $parms->get('board');
    $state = $parms->get('state');
    if (!empty($board) || !empty($state)) {
      $query->condition('topics.entity.states.entity.current', 1);
      if (!empty($board)) {
        $query->condition('topics.entity.states.entity.board', $board);
      }
      if (!empty($state)) {
        $query->condition('topics.entity.states.entity.value', $state);
      }
    }
    if ($this->limit) {
      $query->pager($this->limit);
    }
    return $query->execute();
  }

  /**
   * {@inheritdoc}
   */
  public function buildRow(EntityInterface $entity): array {
    $row['id'] = $entity->id();
    $row['source-id'] = $entity->source_id->value;
    $row['citation'] = Link::createFromRoute(
      $entity->getLabel(),
      // 'entity.ebms_article.canonical',
      'ebms_article.article',
      // ['ebms_article' => $entity->id()]
      ['article' => $entity->id()]
    );
    // $row['citation'] = $entity->getLabel();
    $row['import-date'] = $entity->import_date->value;
    return $row + parent::buildRow($entity);
  }

}
