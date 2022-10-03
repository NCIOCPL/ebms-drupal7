<?php

namespace Drupal\ebms_article\Plugin\Field\FieldType;

use Drupal\Core\Field\FieldItemBase;
use Drupal\Core\Field\FieldStorageDefinitionInterface;
use Drupal\Core\TypedData\DataDefinition;

/**
 * Implements an articulated author name(s) field.
 *
 * The display_name and search_name fields are calculated at save time
 * in the ebms_article_entity_presave() function (ebms_article.module).
 *
 * @FieldType(
 *   id = "ebms_author",
 *   label = @Translation("Author"),
 *   description = @Translation("Contributer to an article"),
 *   translatable = FALSE
 * )
 */
class Author extends FieldItemBase {

  /**
   * {@inheritdoc}
   */
  public static function propertyDefinitions(
    FieldStorageDefinitionInterface $field_definition
  ): array {
    return [
      'last_name' => DataDefinition::create('string')
        ->setLabel('Last Name'),
      'first_name' => DataDefinition::create('string')
        ->setLabel('First Name'),
      'initials' => DataDefinition::create('string')
        ->setLabel('Forename Initials'),
      'collective_name' => DataDefinition::create('string')
        ->setLabel('Collective Name'),
      'display_name' => DataDefinition::create('string')
        ->setLabel('Display Name'),
      'search_name' => DataDefinition::create('string')
        ->setLabel('Search Name'),
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
        'last_name' => [
          'description' => 'Surname for personal author',
          'type' => 'varchar',
          'length' => 255,
        ],
        'first_name' => [
          'description' => 'Given name(s) for personal author',
          'type' => 'varchar',
          'length' => 128,
        ],
        'initials' => [
          'description' => 'Forename initials for personal author',
          'type' => 'varchar',
          'length' => 128,
        ],
        'collective_name' => [
          'description' => 'Name for corporate author',
          'type' => 'varchar',
          'length' => 767,
        ],
        'display_name' => [
          'description' => 'Assembled version of name for display',
          'type' => 'varchar',
          'length' => 767,
        ],
        'search_name' => [
          'description' => "ASCII version of display name",
          'type' => 'varchar',
          'length' => 767,
        ],
      ],
    ];
  }

  /**
   * {@inheritdoc}
   */
  public function isEmpty(): bool {
    $last_name = $this->get('last_name')->getValue();
    $collective_name = $this->get('collective_name')->getValue();
    return empty($last_name) && empty($collective_name);
  }

}
