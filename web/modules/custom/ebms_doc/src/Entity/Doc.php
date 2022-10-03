<?php

namespace Drupal\ebms_doc\Entity;

use Drupal\Core\Entity\ContentEntityBase;
use Drupal\Core\Entity\ContentEntityInterface;
use Drupal\Core\Entity\EntityTypeInterface;
use Drupal\Core\Field\BaseFieldDefinition;
use Drupal\Core\Field\FieldStorageDefinitionInterface;

/**
 * Document posted to the EBMS.
 *
 * This entity type is basically a wrapper around the `File` entity type,
 * with support for tags, boards, and topic associations. This allows us
 * to select uploaded documents for picklists and other purposes.
 *
 * @ingroup ebms
 *
 * @ContentEntityType(
 *   id = "ebms_doc",
 *   label = @Translation("Document"),
 *   handlers = {
 *     "form" = {
 *       "archive" = "Drupal\ebms_doc\Form\ArchiveDoc",
 *     },
 *   },
 *   base_table = "ebms_doc",
 *   entity_keys = {
 *     "id" = "id",
 *   }
 * )
 */
class Doc extends ContentEntityBase implements ContentEntityInterface {

  /**
   * {@inheritdoc}
   */
  public static function baseFieldDefinitions(EntityTypeInterface $entity_type): array {
    $fields = parent::baseFieldDefinitions($entity_type);

    $fields['description'] = BaseFieldDefinition::create('string')
      ->setDescription('Displayed identification for the document on forms.')
      ->setSettings([
        'max_length' => 1024,
        'text_processing' => 0,
      ])
      ->setRequired(TRUE);

    // Can't use 'file' type.
    // See https://drupal.stackexchange.com/questions/310923 and
    // https://www.drupal.org/project/drupal/issues/3278083.
    $fields['file'] = BaseFieldDefinition::create('entity_reference')
      ->setDescription('Reference to the built-in core File entity type.')
      ->setLabel('File')
      ->setSetting('target_type', 'file')
      ->setRequired(TRUE);

    $fields['posted'] = BaseFieldDefinition::create('datetime')
      ->setLabel('Posted')
      ->setRequired(TRUE)
      ->setDescription('When the document was posted to the EBMS.');

    $fields['dropped'] = BaseFieldDefinition::create('boolean')
      ->setLabel('Dropped')
      ->setRequired(TRUE)
      ->setDefaultValue(FALSE)
      ->setDescription('Check this box to suppress the document from picklists.');

    $fields['tags'] = BaseFieldDefinition::create('entity_reference')
      ->setLabel('Tags')
      ->setDescription('Optional tags designating the document for specific uses.')
      ->setSetting('target_type', 'taxonomy_term')
      ->setSetting('handler_settings',
        ['target_bundles' => ['doc_tags' => 'doc_tags']])
      ->setCardinality(FieldStorageDefinitionInterface::CARDINALITY_UNLIMITED);

    $fields['boards'] = BaseFieldDefinition::create('entity_reference')
      ->setLabel('Boards')
      ->setDescription('Optional list of PDQ boards associated with the document.')
      ->setSetting('target_type', 'ebms_board')
      ->setCardinality(FieldStorageDefinitionInterface::CARDINALITY_UNLIMITED);

    $fields['topics'] = BaseFieldDefinition::create('entity_reference')
      ->setLabel('Topics')
      ->setDescription('Optional list of topics for which this document can be used.')
      ->setSetting('target_type', 'ebms_topic')
      ->setCardinality(FieldStorageDefinitionInterface::CARDINALITY_UNLIMITED);

    return $fields;
  }

}
