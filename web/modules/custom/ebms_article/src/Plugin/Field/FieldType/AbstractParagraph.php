<?php

namespace Drupal\ebms_article\Plugin\Field\FieldType;

use Drupal\Core\Field\FieldItemBase;
use Drupal\Core\Field\FieldStorageDefinitionInterface;
use Drupal\Core\TypedData\DataDefinition;

/**
 * Implements an abstract paragraph with a label.
 *
 * @FieldType(
 *   id = "ebms_abstract_paragraph",
 *   label = @Translation("Abstract Paragraph"),
 *   description = @Translation("Portion of an EBMS article abstract"),
 *   translatable = FALSE
 * )
 */
class AbstractParagraph extends FieldItemBase {

  /**
   * {@inheritdoc}
   */
  public static function propertyDefinitions(
    FieldStorageDefinitionInterface $field_definition
  ): array {
    return [
      'paragraph_text' => DataDefinition::create('string')
        ->setLabel('Paragraph Text'),
      'paragraph_label' => DataDefinition::create('string')
        ->setLabel('Paragraph Label'),
    ];
  }

  /**
   * {@inheritdoc}
   */
  public static function schema(
    FieldStorageDefinitionInterface $field_definition
  ): array {
    return [
      'columns' => [
        'paragraph_text' => [
          'description' => 'Body of the abstract text',
          'type' => 'text',
        ],
        'paragraph_label' => [
          'description' => 'Type of information in the paragraph',
          'type' => 'varchar',
          'length' => 1024,
        ],
      ],
    ];
  }

}
