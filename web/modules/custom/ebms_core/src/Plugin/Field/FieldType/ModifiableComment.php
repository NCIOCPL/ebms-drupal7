<?php

namespace Drupal\ebms_core\Plugin\Field\FieldType;

use Drupal\Core\Field\FieldItemBase;
use Drupal\Core\Field\FieldStorageDefinitionInterface;
use Drupal\Core\TypedData\DataDefinition;

/**
 * Comment with tracking when and by whom it was last modified.
 *
 * Somewhat more complicated than the custom SimpleComment field (also
 * created as part of this module), but still much less heavy weight
 * than the built-in Comment entity type. Used for board manager
 * topic-specific comments, shown to board members on their packet
 * review pages.
 *
 * @FieldType(
 *   id = "ebms_modifiable_comment",
 *   description = @Translation("Modifiable comment"),
 * )
 */
class ModifiableComment extends FieldItemBase {

  /**
   * {@inheritdoc}
   */
  public static function propertyDefinitions(
    FieldStorageDefinitionInterface $field_definition
  ) {
    return [
      'user' => DataDefinition::create('entity_reference')
        ->setRequired(TRUE)
        ->setSetting('target_type', 'user'),
      'entered' => DataDefinition::create('datetime_iso8601')
        ->setRequired(TRUE),
      'comment' => DataDefinition::create('string')
        ->setRequired(TRUE),
      'modified' => DataDefinition::create('datetime_iso8601'),
      'modified_by' => DataDefinition::create('entity_reference')
        ->setSetting('target_type', 'user'),
    ];
  }

  /**
   * {@inheritdoc}
   */
  public static function schema(
    FieldStorageDefinitionInterface $field_definition
  ) {
    return [
      'columns' => [
        'user' => ['type' => 'int', 'unsigned' => TRUE],
        'entered' => ['type' => 'varchar', 'length' => 20],
        'comment' => ['type' => 'text'],
        'modified' => ['type' => 'varchar', 'length' => 20],
        'modified_by' => ['type' => 'int', 'unsigned' => TRUE],
      ],
    ];
  }

}
