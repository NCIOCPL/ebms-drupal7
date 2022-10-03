<?php

namespace Drupal\ebms_journal\Plugin\Field\FieldType;

use Drupal\Core\Field\FieldItemBase;
use Drupal\Core\Field\FieldStorageDefinitionInterface;
use Drupal\Core\TypedData\DataDefinition;

/**
 * PDQ board journal preferences.
 *
 * Information about a PDQ board which prefers not to review articles from
 * a given journal.
 *
 * @FieldType(
 *   id = "ebms_not_list_board",
 * )
 */
class NotListBoard extends FieldItemBase {

  /**
   * {@inheritdoc}
   */
  public static function propertyDefinitions(FieldStorageDefinitionInterface $field_definition): array {
    return [
      'board' => DataDefinition::create('entity_reference')
        ->setDescription("Board for which this journal's articles should not be included by default in the review process.")
        ->setSetting('target_type', 'ebms_board')
        ->setRequired(TRUE),
      'start' => DataDefinition::create('datetime_iso8601')
        ->setDescription('Date when this setting should take effect for this board.')
        ->setRequired(TRUE),
      'user' => DataDefinition::create('entity_reference')
        ->setDescription('User who assigned this designation for the journal.')
        ->setSetting('target_type', 'user')
        ->setRequired(TRUE),
    ];
  }

  /**
   * {@inheritdoc}
   */
  public static function schema(FieldStorageDefinitionInterface $field_definition): array {
    return [
      'columns' => [
        'board' => ['type' => 'int', 'unsigned' => TRUE],
        'start' => ['type' => 'varchar', 'length' => 20],
        'user' => ['type' => 'int', 'unsigned' => TRUE],
      ],
    ];
  }

}
