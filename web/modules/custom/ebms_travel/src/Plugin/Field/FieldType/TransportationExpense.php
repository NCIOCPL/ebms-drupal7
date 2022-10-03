<?php

namespace Drupal\ebms_travel\Plugin\Field\FieldType;

use Drupal\Core\Field\FieldItemBase;
use Drupal\Core\Field\FieldStorageDefinitionInterface;
use Drupal\Core\TypedData\DataDefinition;
use Drupal\taxonomy\Entity\Term;

/**
 * Transportation expense field.
 *
 * @FieldType(
 *   id = "ebms_transportation_expense",
 * )
 */
class TransportationExpense extends FieldItemBase {

  /**
   * {@inheritdoc}
   */
  public static function propertyDefinitions(FieldStorageDefinitionInterface $field_definition): array {
    return [
      'date' => DataDefinition::create('datetime_iso8601')
        ->setDescription('When the expense was incurred.')
        ->setRequired(TRUE),
      'type' => DataDefinition::create('entity_reference')
        ->setDescription('Taxi, for example, or Shuttle.')
        // ->setRequired(TRUE)
        ->setSetting('target_type', 'taxonomy_term'),
      'amount' => DataDefinition::create('decimal')
        ->setDescription('How much the expense was for.'),
      'mileage' => DataDefinition::create('decimal')
        ->setDescription('How much the expense was for.'),
    ];
  }

  /**
   * {@inheritdoc}
   */
  public static function schema(FieldStorageDefinitionInterface $field_definition): array {
    return [
      'columns' => [
        'date' => ['type' => 'varchar', 'length' => 20],
        'type' => ['type' => 'int', 'unsigned' => TRUE],
        'amount' => ['type' => 'numeric', 'precision' => 10, 'scale' => 2],
        'mileage' => ['type' => 'numeric', 'precision' => 10, 'scale' => 2],
      ],
    ];
  }

  /**
   * Convert field value to string.
   */
  public function toString(): string {
    $date = $this->date;
    if ($date == '0000-00-00') {
      $date = 'unspecified date';
    }
    if (empty($this->type)) {
      $type = 'unspecified type';
    }
    else {
      $type = Term::load($this->type)->name->value;
    }
    if (!empty($this->amount)) {
      return "$type - $date - " . '$' . $this->amount . ' (52-04)';
    }
    if (!empty($this->mileage)) {
      return "$type - $date - " . $this->mileage . ' miles (51-00)';
    }
    return "$type - $date - neither amount nor mileage given";
  }

}
