<?php

namespace Drupal\ebms_article\Plugin\Field\FieldType;

use Drupal\Core\Field\FieldItemBase;
use Drupal\Core\Field\FieldStorageDefinitionInterface;
use Drupal\Core\TypedData\DataDefinition;

/**
 * Plugin implementation of the 'ebms_pub_date' field type.
 *
 * At the present time, the year value is the only piece of the PubDate
 * block in the MedLine record which is used by the rest of the system,
 * and that value is parsed out at import (or refresh) time, so the
 * only reason we are preserving the original NLM values is in case
 * there is a later requirement to use them.
 *
 * @FieldType(
 *   id = "ebms_pub_date",
 *   label = @Translation("Pub Date"),
 *   description = @Translation("Articulated Article Publication Date"),
 *   translatable = FALSE
 * )
 */
class PubDate extends FieldItemBase {

  /**
   * {@inheritdoc}
   */
  public static function propertyDefinitions(
    FieldStorageDefinitionInterface $field_definition
  ): array {
    return [
      'year' => DataDefinition::create('string')->setLabel('Year'),
      'month' => DataDefinition::create('string')->setLabel('Month'),
      'day' => DataDefinition::create('string')->setLabel('Day'),
      'season' => DataDefinition::create('string')->setLabel('Season'),
      'medline_date' => DataDefinition::create('string')
        ->setLabel('Medline Date'),
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
        'year' => [
          'description' => 'Publication year',
          'type' => 'varchar',
          'length' => 4,
        ],
        'month' => [
          'description' => 'Publication month',
          'type' => 'varchar',
          'length' => 3,
        ],
        'day' => [
          'description' => 'Publication day',
          'type' => 'varchar',
          'length' => 2,
        ],
        'season' => [
          'description' => 'Publication season',
          'type' => 'varchar',
          'length' => 12,
        ],
        'medline_date' => [
          'description' => 'Custom publication date string',
          'type' => 'varchar',
          'length' => 64,
        ],
      ],
    ];
  }

  /**
   * {@inheritdoc}
   */
  public function isEmpty(): bool {
    $year = $this->get('year')->getValue();
    $date = $this->get('medline_date')->getValue();
    return empty($year) && empty($date);
  }

  /**
   * Convert field value to string.
   */
  public function toString(): string {
    if (!empty($this->medline_date)) {
      return $this->medline_date;
    }
    if (!empty($this->season)) {
      return $this->season . ' ' . $this->year;
    }
    if (!empty($his->day)) {
      if (!empty($this->month)) {
        if (strlen($this->month) === 3) {
          return $this->day . ' ' . $this->month . ' ' . $this->year;
        }
        return $this->year . '-' . $this->month . '-' . $this->day;
      }
    }
    if (!empty($this->month)) {
      if (strlen($this->month) === 3) {
        return $this->month . ' ' . $this->year;
      }
    }
    return trim($this->year ?? '');
  }

}
