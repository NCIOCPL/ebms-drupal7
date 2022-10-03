<?php

namespace Drupal\ebms_article\Plugin\Field\FieldType;

use Drupal\Core\Field\FieldItemBase;
use Drupal\Core\Field\FieldStorageDefinitionInterface;
use Drupal\Core\TypedData\DataDefinition;

/**
 * Marking of an article of internal interest, not for board member review.
 *
 * @FieldType(
 *   id = "ebms_internal_tag",
 *   description = @Translation("Marks an article for internal use only"),
 * )
 */
class InternalTag extends FieldItemBase {

  /**
   * {@inheritdoc}
   */
  public static function propertyDefinitions(
    FieldStorageDefinitionInterface $field_definition
  ): array {
    return [
      'tag' => DataDefinition::create('entity_reference')
        ->setRequired(TRUE)
        ->setSetting('target_type', 'taxonomy_term'),
      'added' => DataDefinition::create('datetime_iso8601')
        ->setRequired(TRUE),
    ];
  }

  /**
   * {@inheritdoc}
   */
  public static function schema(FieldStorageDefinitionInterface $field_definition): array {
    return [
      'columns' => [
        'tag' => ['type' => 'int', 'unsigned' => TRUE],
        'added' => ['type' => 'varchar', 'length' => 20],
      ],
    ];
  }

}
