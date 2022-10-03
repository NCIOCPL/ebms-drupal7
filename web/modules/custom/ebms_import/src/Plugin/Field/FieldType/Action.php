<?php

namespace Drupal\ebms_import\Plugin\Field\FieldType;

use Drupal\Core\Field\FieldItemBase;
use Drupal\Core\Field\FieldStorageDefinitionInterface;
use Drupal\Core\TypedData\DataDefinition;

/**
 * Action taken for an article in an import request batch.
 *
 * @FieldType(
 *   id = "ebms_import_action",
 * )
 */
class Action extends FieldItemBase {

  /**
   * {@inheritdoc}
   */
  public static function propertyDefinitions(FieldStorageDefinitionInterface $field_definition): array {
    return [
      'source_id' => DataDefinition::create('string')
        ->setDescription('ID used by the data source (usually PubMed) to identify the article.')
        ->setRequired(TRUE),
      'article' => DataDefinition::create('entity_reference')
        ->setDescription('Reference to the Article entity (if there is one).')
        ->setSetting('target_type', 'ebms_article'),
      'disposition' => DataDefinition::create('entity_reference')
        ->setDescription('Identification of the action taken for this article.')
        ->setRequired(TRUE)
        ->setSetting('target_type', 'taxonomy_term'),
      'message' => DataDefinition::create('string')
        ->setDescription('Optional additional information about the action.'),
    ];
  }

  /**
   * {@inheritdoc}
   */
  public static function schema(FieldStorageDefinitionInterface $field_definition): array {
    return [
      'columns' => [
        'source_id' => ['type' => 'varchar', 'length' => 32],
        'article' => ['type' => 'int', 'unsigned' => TRUE],
        'disposition' => ['type' => 'int', 'unsigned' => TRUE],
        'message' => ['type' => 'text'],
      ],
    ];
  }

}
