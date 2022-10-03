<?php

namespace Drupal\ebms_article\Plugin\Field\FieldType;

use Drupal\Core\Field\FieldItemBase;
use Drupal\Core\Field\FieldStorageDefinitionInterface;
use Drupal\Core\TypedData\DataDefinition;

/**
 * Information on the PDF file for an EBMS article.
 *
 * There are basically three conditions represented by this field.
 *  1. We have the article's full-text PDF file, and `file` is set.
 *  2. We have determined that the PDF file is unavailable, and some
 *     of the other field's properties are set.
 *  3. We don't have the file, and we don't know if it is available,
 *     so none of the field properties have values.
 *
 * @FieldType(
 *   id = "ebms_full_text",
 *   description = @Translation("Information about the article's full text"),
 * )
 */
class FullText extends FieldItemBase {

  /**
   * {@inheritdoc}
   */
  public static function propertyDefinitions(
    FieldStorageDefinitionInterface $field_definition
  ): array {
    return [
      'file' => DataDefinition::create('entity_reference')
        ->setSetting('target_type', 'file'),
      'unavailable' => DataDefinition::create('boolean'),
      'flagged_as_unavailable' => DataDefinition::create('datetime_iso8601'),
      'flagged_by' => DataDefinition::create('entity_reference')
        ->setSetting('target_type', 'user'),
      'notes' => DataDefinition::create('string'),
    ];
  }

  /**
   * {@inheritdoc}
   */
  public static function schema(FieldStorageDefinitionInterface $field_definition): array {
    return [
      'columns' => [
        'file' => ['type' => 'int', 'unsigned' => TRUE],
        'unavailable' => ['type' => 'int', 'size' => 'tiny'],
        'flagged_as_unavailable' => ['type' => 'varchar', 'length' => 20],
        'flagged_by' => ['type' => 'int', 'unsigned' => TRUE],
        'notes' => ['type' => 'text'],
      ],
    ];
  }

}
