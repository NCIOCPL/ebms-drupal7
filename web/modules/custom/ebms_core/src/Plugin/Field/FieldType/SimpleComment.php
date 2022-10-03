<?php

namespace Drupal\ebms_core\Plugin\Field\FieldType;

use Drupal\Core\Field\FieldItemBase;
use Drupal\Core\Field\FieldStorageDefinitionInterface;
use Drupal\Core\TypedData\DataDefinition;

/**
 * Unthreaded, unversioned, untranslatable comment.
 *
 * The Drupal core Comment entity type is much more complex than is needed
 * for these comments, as it supports threading, versioning, translation,
 * rich text, etc. Used for comments attached to entities like articles,
 * tags, or states.
 *
 * @FieldType(
 *   id = "ebms_comment",
 *   description = @Translation("Simple comment"),
 * )
 */
class SimpleComment extends FieldItemBase {

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
      'body' => DataDefinition::create('string')
        ->setRequired(TRUE),
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
        'body' => ['type' => 'text'],
      ],
    ];
  }

}
