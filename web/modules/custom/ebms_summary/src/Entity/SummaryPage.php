<?php

namespace Drupal\ebms_summary\Entity;

use Drupal\Core\Entity\ContentEntityBase;
use Drupal\Core\Entity\ContentEntityInterface;
use Drupal\Core\Entity\EntityTypeInterface;
use Drupal\Core\Field\BaseFieldDefinition;
use Drupal\Core\Field\FieldStorageDefinitionInterface;
use Drupal\link\LinkItemInterface;

/**
 * Page of links to PDQ® summary documents.
 *
 * There is generally a one-to-one correspondence between a single Topic
 * entity and a SummaryPage but any number of topics (or none) can be
 * associated with a summary page (in fact, the Digestive/Gastrointestinal
 * Cancers page currently has 14 associated topics, and the Head and Neck
 * Cancers page has 9 topics). In addition to the main links to the existing
 * summaries, both NCI staff and the board member reviewers can post
 * Microsoft Word versions of those summaries, and those files are linked at
 * the bottom of the summary page in two separate tables, one for NCI-posted
 * summary documents, and a second table for the summary documents posted by
 * board members.
 *
 * @ingroup ebms
 *
 * @ContentEntityType(
 *   id = "ebms_summary_page",
 *   label = @Translation("Summary Page"),
 *   handlers = {
 *     "form" = {
 *       "deactivate" = "Drupal\ebms_summary\Form\SummaryPageDeleteForm",
 *     },
 *   },
 *   base_table = "ebms_summary_page",
 *   admin_permission = "manage summaries",
 *   entity_keys = {
 *     "id" = "id",
 *   },
 * )
 */
class SummaryPage extends ContentEntityBase implements ContentEntityInterface {

  /**
   * {@inheritdoc}
   */
  public static function baseFieldDefinitions(EntityTypeInterface $entity_type): array {
    $fields = parent::baseFieldDefinitions($entity_type);
    $fields['name'] = BaseFieldDefinition::create('string')
      ->setDescription('The display name for the summary page.')
      ->setSettings([
        'max_length' => 255,
        'text_processing' => 0,
      ])
      ->setRequired(TRUE);
    $fields['topics'] = BaseFieldDefinition::create('entity_reference')
      ->setDescription('Review topics which are associated with this summary page.')
      ->setSetting('target_type', 'ebms_topic')
      ->setCardinality(FieldStorageDefinitionInterface::CARDINALITY_UNLIMITED);
    $fields['links'] = BaseFieldDefinition::create('link')
      ->setDescription('Links to the summaries on cancer.gov.')
      ->setRequired(TRUE)
      ->setSetting('link_type', LinkItemInterface::LINK_EXTERNAL)
      ->setCardinality(FieldStorageDefinitionInterface::CARDINALITY_UNLIMITED);
    $fields['manager_docs'] = BaseFieldDefinition::create('ebms_doc_usage')
      ->setDescription('Microsoft Word versions of summaries posted by board managers.')
      ->setCardinality(FieldStorageDefinitionInterface::CARDINALITY_UNLIMITED);
    $fields['member_docs'] = BaseFieldDefinition::create('ebms_doc_usage')
      ->setDescription('Microsoft Word versions of summaries posted by board member reviewers.')
      ->setCardinality(FieldStorageDefinitionInterface::CARDINALITY_UNLIMITED);
    $fields['active'] = BaseFieldDefinition::create('boolean')
      ->setRequired(TRUE)
      ->setDefaultValue(TRUE)
      ->setDescription('If FALSE display of this page is suppressed, as is the link to it.');
    return $fields;
  }

  /**
   * Find documents for the NCI summary docs picklist.
   *
   * @return array
   *   NCI documents for the board which aren't already in use.
   */
  public function eligibleDocs(): array {
    $used_docs = [];
    foreach ($this->manager_docs as $doc_usage) {
      $used_docs[] = $doc_usage->doc;
    }
    $topics = [];
    foreach ($this->topics as $topic) {
      $topics[] = $topic->target_id;
    }
    if (empty($topics)) {
      return [];
    }
    $storage = \Drupal::entityTypeManager()->getStorage('ebms_doc');
    $query = $storage->getQuery()->accessCheck(FALSE);
    $query->condition('tags.entity.field_text_id', 'summary');
    $query->condition('topics', $topics, 'IN');
    $query->condition('dropped', 0);
    $query->sort('description');
    $eligible_docs = [];
    $ids = $query->execute();
    foreach ($storage->loadMultiple($ids) as $doc) {
      if (!in_array($doc->id(), $used_docs)) {
        $wanted = TRUE;
        foreach ($doc->tags as $tag) {
          if ($tag->entity->field_text_id === 'support') {
            $wanted = FALSE;
            break;
          }
        }
        if ($wanted) {
          $label = $doc->description->value;
          if (empty($label)) {
            $label = $doc->file->entity->filename->value;
          }
          $eligible_docs[$doc->id()] = $label;
        }
      }
    }
    return $eligible_docs;
  }

}
