<?php

namespace Drupal\ebms_state\Plugin\Field\FieldType;

use Drupal\Core\Field\FieldItemBase;
use Drupal\Core\Field\FieldStorageDefinitionInterface;
use Drupal\Core\TypedData\DataDefinition;

/**
 * PDQ board decision made for an article/topic state.
 *
 * This field is populated for `State` entities whose state value is
 * "Final board decision."
 *
 * The `meeting_date` property is carried over from the original
 * implementation but it is really a denormalization, duplicating the value
 * of the cycle date assigned to the combination of the article/topic for
 * which this decision is made, and the value is never actually used anywhere
 * (the name of the field is a bit of a misnomer, being the topic's cycle and
 * not the date of an actual meeting--in fact there may never have been a
 * meeting for this decision).
 *
 * The schema for the `State` entity allows for multiple decisions to be
 * attached for a single entity, but at present there is no support in the
 * user interface for adding more than one decision to a `State` entity (and
 * in fact the decisions field is optional).
 *
 * @FieldType(
 *   id = "ebms_board_decision",
 *   description = @Translation("Board decision"),
 * )
 */
class BoardDecision extends FieldItemBase {

  /**
   * {@inheritdoc}
   */
  public static function propertyDefinitions(FieldStorageDefinitionInterface $field_definition): array {
    return [
      'decision' => DataDefinition::create('entity_reference')
        ->setDescription('The vocabulary term for the value of the decision made.')
        ->setRequired(TRUE)
        ->setSetting('target_type', 'taxonomy_term'),
      'meeting_date' => DataDefinition::create('datetime_iso8601')
        ->setDescription('Cycle for which the decision was made.'),
      'discussed' => DataDefinition::create('boolean')
        ->setDescription('Whether the decision was discussed at a meeting.'),
    ];
  }

  /**
   * {@inheritdoc}
   */
  public static function schema(FieldStorageDefinitionInterface $field_definition): array {
    return [
      'columns' => [
        'decision' => ['type' => 'int', 'unsigned' => TRUE],
        'meeting_date' => ['type' => 'varchar', 'length' => 20],
        'discussed' => ['type' => 'int', 'size' => 'tiny'],
      ],
    ];
  }

}
