<?php

namespace Drupal\ebms_summary\Plugin\Field\FieldType;

use Drupal\Core\Field\FieldItemBase;
use Drupal\Core\Field\FieldStorageDefinitionInterface;
use Drupal\Core\TypedData\DataDefinition;

/**
 * Wrapper around the `Doc` entity.
 *
 * This multi-valued field provides support for notes and activation
 * information.
 *
 * @FieldType(
 *   id = "ebms_doc_usage",
 *   description = @Translation("Summary document for a Summaries page."),
 * )
 */
class DocUsage extends FieldItemBase {

  /**
   * {@inheritdoc}
   */
  public static function propertyDefinitions(
    FieldStorageDefinitionInterface $field_definition
  ): array {
    return [
      'doc' => DataDefinition::create('entity_reference')
        ->setDescription('Reference to the Doc entity for this summary document.')
        ->setRequired(TRUE)
        ->setSetting('target_type', 'ebms_doc'),
      'notes' => DataDefinition::create('string')
        ->setDescription('Optional notes on this document as used on this summary page.'),
      'active' => DataDefinition::create('boolean')
        ->setRequired(TRUE)
        ->setDescription('If FALSE, display of the link to this document is suppressed.'),
    ];
  }

  /**
   * {@inheritdoc}
   */
  public static function schema(FieldStorageDefinitionInterface $field_definition): array {
    return [
      'columns' => [
        'doc' => ['type' => 'int', 'unsigned' => TRUE],
        'notes' => ['type' => 'text'],
        'active' => ['type' => 'int', 'size' => 'tiny'],
      ],
    ];
  }

}
