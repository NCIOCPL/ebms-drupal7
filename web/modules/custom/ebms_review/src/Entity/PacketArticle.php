<?php

namespace Drupal\ebms_review\Entity;

use Drupal\Core\Entity\ContentEntityBase;
use Drupal\Core\Entity\ContentEntityInterface;
use Drupal\Core\Entity\EntityTypeInterface;
use Drupal\Core\Field\BaseFieldDefinition;
use Drupal\Core\Field\FieldStorageDefinitionInterface;

/**
 * Article assigned for review in a Packet.
 *
 * Tracks the reviews submitted for the articles in the packet.
 *
 * @ingroup ebms
 *
 * @ContentEntityType(
 *   id = "ebms_packet_article",
 *   label = @Translation("Packet Article"),
 *   base_table = "ebms_packet_article",
 *   admin_permission = "access ebms article overview",
 *   entity_keys = {
 *     "id" = "id",
 *   },
 * )
 */
class PacketArticle extends ContentEntityBase implements ContentEntityInterface {

  /**
   * {@inheritdoc}
   */
  public static function baseFieldDefinitions(EntityTypeInterface $entity_type): array {
    $fields = parent::baseFieldDefinitions($entity_type);
    $fields['article'] = BaseFieldDefinition::create('entity_reference')
      ->setRequired(TRUE)
      ->setDescription('One of the articles assigned for review as part of this packet.')
      ->setSetting('target_type', 'ebms_article');
    $fields['dropped'] = BaseFieldDefinition::create('boolean')
      ->setRequired(TRUE)
      ->setDescription('If TRUE the article should no longer be included when the packet is presented to the board member for review.')
      ->setDefaultValue(FALSE);
    $fields['archived'] = BaseFieldDefinition::create('datetime')
      ->setDescription('If not NULL the date/time the article was archived for this packet, suppressing it from review display.');
    $fields['reviews'] = BaseFieldDefinition::create('entity_reference')
      ->setSetting('target_type', 'ebms_review')
      ->setDescription('The reviews which the board members have submitted for the article.')
      ->setCardinality(FieldStorageDefinitionInterface::CARDINALITY_UNLIMITED);
    return $fields;
  }

}
