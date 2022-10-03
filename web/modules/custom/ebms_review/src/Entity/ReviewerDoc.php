<?php

namespace Drupal\ebms_review\Entity;

use Drupal\Core\Entity\ContentEntityBase;
use Drupal\Core\Entity\ContentEntityInterface;
use Drupal\Core\Entity\EntityTypeInterface;
use Drupal\Core\Field\BaseFieldDefinition;

/**
 * Document posted to a review packet by a board member.
 *
 * @ingroup ebms
 *
 * @ContentEntityType(
 *   id = "ebms_reviewer_doc",
 *   label = @Translation("Reviewer Document"),
 *   base_table = "ebms_reviewer_doc",
 *   entity_keys = {
 *     "id" = "id",
 *   }
 * )
 */
class ReviewerDoc extends ContentEntityBase implements ContentEntityInterface {

  /**
   * {@inheritdoc}
   */
  public static function baseFieldDefinitions(EntityTypeInterface $entity_type): array {
    $fields = parent::baseFieldDefinitions($entity_type);
    $fields['file'] = BaseFieldDefinition::create('file')
      ->setDescription("The file for the board member's document.")
      ->setLabel('File')
      ->setRequired(TRUE)
      ->setSetting('file_extensions', 'pdf rtf doc docx')
      ->setSetting('max_filesize', '20MB')
      ->setDisplayOptions('form', ['type' => 'file'])
      ->setDisplayOptions('view', ['type' => 'file']);
    $fields['reviewer'] = BaseFieldDefinition::create('entity_reference')
      ->setDescription('The board member who posted the document.')
      ->setSetting('target_type', 'user')
      ->setRequired(TRUE);
    $fields['posted'] = BaseFieldDefinition::create('datetime')
      ->setLabel('Posted')
      ->setRequired(TRUE)
      ->setDescription('When the document was posted to the EBMS.');
    $fields['description'] = BaseFieldDefinition::create('string_long')
      ->setDescription('Optional information about this document.')
      ->setLabel('Add Notes (optional)')
      ->setDisplayOptions('form', ['type' => 'text_textarea'])
      ->setDisplayOptions('view', ['type' => 'string']);
    $fields['dropped'] = BaseFieldDefinition::create('boolean')
      ->setLabel('Dropped')
      ->setDescription('Check this box to suppress the document from picklists/other displays.')
      ->setDisplayOptions('form', ['type' => 'boolean']);
    return $fields;
  }

}
