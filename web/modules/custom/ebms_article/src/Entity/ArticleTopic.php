<?php

namespace Drupal\ebms_article\Entity;

use Drupal\Core\Entity\ContentEntityBase;
use Drupal\Core\Entity\ContentEntityInterface;
use Drupal\Core\Entity\EntityTypeInterface;
use Drupal\Core\Field\BaseFieldDefinition;
use Drupal\Core\Field\FieldStorageDefinitionInterface;
use Drupal\datetime\Plugin\Field\FieldType\DateTimeItem;

/**
 * Association of an article with a specific review topic.
 *
 * The article passes through a separate series of states for each topic
 * assigned to it.
 *
 * @ingroup ebms
 *
 * @ContentEntityType(
 *   id = "ebms_article_topic",
 *   label = @Translation("Article Topic"),
 *   base_table = "ebms_article_topic",
 *   entity_keys = {
 *     "id" = "id",
 *   },
 * )
 */
class ArticleTopic extends ContentEntityBase implements ContentEntityInterface {

  /**
   * {@inheritdoc}
   */
  public static function baseFieldDefinitions(EntityTypeInterface $entity_type): array {
    $fields = parent::baseFieldDefinitions($entity_type);
    $fields['topic'] = BaseFieldDefinition::create('entity_reference')
      ->setRequired(TRUE)
      ->setLabel('Topic')
      ->setDescription('One of the topics assigned to this article for review.')
      ->setSetting('target_type', 'ebms_topic');
    $fields['cycle'] = BaseFieldDefinition::create('datetime')
      ->setRequired(TRUE)
      ->setDescription('The month for which review of the article is targeted for this topic.')
      ->setSettings(['datetime_type' => DateTimeItem::DATETIME_TYPE_DATE]);
    $fields['states'] = BaseFieldDefinition::create('entity_reference')
      ->setLabel('States')
      ->setDescription('Per-topic states for this article.')
      ->setSetting('target_type', 'ebms_state')
      ->setCardinality(FieldStorageDefinitionInterface::CARDINALITY_UNLIMITED);
    $fields['tags'] = BaseFieldDefinition::create('entity_reference')
      ->setLabel('Tags')
      ->setDescription('Article tags specific to this topic.')
      ->setSetting('target_type', 'ebms_article_tag')
      ->setCardinality(FieldStorageDefinitionInterface::CARDINALITY_UNLIMITED);
    $fields['comments'] = BaseFieldDefinition::create('ebms_modifiable_comment')
      ->setLabel('Comments')
      ->setDescription('Optional comments for the assignment of this topic to the article.')
      ->setCardinality(FieldStorageDefinitionInterface::CARDINALITY_UNLIMITED);
    return $fields;
  }

  /**
   * Find and return the state marked current.
   *
   * @return object|null
   *   Either a `State` object or NULL.
   *
   * @throws \Drupal\Core\TypedData\Exception\MissingDataException
   */
  public function getCurrentState(): ?object {
    $states = $this->get('states');
    $i = count($states);
    while ($i-- > 0) {
      $state = $states->get($i)->entity;
      if ($state->current->value) {
        return $state;
      }
    }
    return NULL;
  }

}
