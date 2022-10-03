<?php

namespace Drupal\ebms_summary\Entity;

use Drupal\Core\Entity\ContentEntityBase;
use Drupal\Core\Entity\ContentEntityInterface;
use Drupal\Core\Entity\EntityTypeInterface;
use Drupal\Core\Field\BaseFieldDefinition;
use Drupal\Core\Field\FieldStorageDefinitionInterface;

/**
 * Collection of a PDQ® board's summary pages.
 *
 * An entity of this type represents the list of summary page links for a
 * given board. Below this list are links to documents generally specific
 * to that board, rather than any specific summary page (typically level-
 * of-evidence usage guidelines for that board).
 *
 * @ingroup ebms
 *
 * @ContentEntityType(
 *   id = "ebms_board_summaries",
 *   label = @Translation("Board Summary Pages"),
 *   base_table = "ebms_board_summaries",
 *   entity_keys = {
 *     "id" = "id",
 *   },
 * )
 */
class BoardSummaries extends ContentEntityBase implements ContentEntityInterface {

  /**
   * {@inheritdoc}
   */
  public static function baseFieldDefinitions(EntityTypeInterface $entity_type): array {
    $fields = parent::baseFieldDefinitions($entity_type);
    $fields['board'] = BaseFieldDefinition::create('entity_reference')
      ->setDescription('The board to which this set of pages belongs.')
      ->setSetting('target_type', 'ebms_board')
      ->setRequired(TRUE);
    $fields['pages'] = BaseFieldDefinition::create('entity_reference')
      ->setDescription('Information about each of the summary pages for this board.')
      ->setRequired(TRUE)
      ->setSetting('target_type', 'ebms_summary_page')
      ->setCardinality(FieldStorageDefinitionInterface::CARDINALITY_UNLIMITED);
    $fields['docs'] = BaseFieldDefinition::create('ebms_doc_usage')
      ->setDescription('Board-specific documents to be linked below the list of links to the summary pages.')
      ->setCardinality(FieldStorageDefinitionInterface::CARDINALITY_UNLIMITED);
    return $fields;
  }

  /**
   * Find documents for the supporting summary docs picklist.
   *
   * @return array
   *   Supporting documents for the board which aren't already in use.
   */
  public function eligibleDocs(): array {
    $used_docs = [];
    foreach ($this->docs as $doc_usage) {
      $used_docs[] = $doc_usage->doc;
    }
    $storage = \Drupal::entityTypeManager()->getStorage('ebms_doc');
    $query = $storage->getQuery()->accessCheck(FALSE);
    $query->condition($query->andConditionGroup()->condition('tags.entity.field_text_id', 'summary'));
    $query->condition($query->andConditionGroup()->condition('tags.entity.field_text_id', 'support'));
    $query->condition('boards', $this->board->target_id);
    $query->condition('dropped', 0);
    $query->sort('description');
    $eligible_docs = [];
    $ids = $query->execute();
    foreach ($storage->loadMultiple($ids) as $doc) {
      if (!in_array($doc->id(), $used_docs)) {
        $label = $doc->description->value;
        if (empty($label)) {
          $label = $doc->file->entity->filename->value;
        }
        $eligible_docs[$doc->id()] = $label;
      }
    }
    return $eligible_docs;
  }

}
