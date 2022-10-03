<?php

namespace Drupal\ebms_review\Entity;

use Drupal\Core\Entity\ContentEntityBase;
use Drupal\Core\Entity\ContentEntityInterface;
use Drupal\Core\Entity\EntityTypeInterface;
use Drupal\Core\Field\BaseFieldDefinition;
use Drupal\Core\Field\FieldStorageDefinitionInterface;

/**
 * Board member review of an article.
 *
 * The reviewer is constrained to select either the first Disposition value
 * ("Warrants no changes to the summary") or one or more of the other
 * Disposition values. If the "Warrants no changes ..." value is chosen,
 * then none of the RejectionReason options may be selected; otherwise at
 * least one RejectionReason options must be selected.
 *
 * @ingroup ebms
 *
 * @ContentEntityType(
 *   id = "ebms_review",
 *   label = @Translation("Review"),
 *   base_table = "ebms_review",
 *   admin_permission = "access ebms article overview",
 *   entity_keys = {
 *     "id" = "id",
 *   },
 * )
 */
class Review extends ContentEntityBase implements ContentEntityInterface {

  /**
   * Rejection disposition.
   */
  const NO_CHANGES = 'Warrants no changes to the summary';

  /**
   * {@inheritdoc}
   */
  public static function baseFieldDefinitions(EntityTypeInterface $entity_type): array {
    $fields = parent::baseFieldDefinitions($entity_type);
    $fields['reviewer'] = BaseFieldDefinition::create('entity_reference')
      ->setRequired(TRUE)
      ->setDescription('The board member who submitted the review.')
      ->setSetting('target_type', 'user');
    $fields['posted'] = BaseFieldDefinition::create('datetime')
      ->setDescription('When the review was submitted.')
      ->setRequired(TRUE);
    $fields['comments'] = BaseFieldDefinition::create('text_long')
      ->setDescription("Free-text elaboration of how the reviewer feels the article's findings should be incorporated into the PDQ summaries (or why it shouldn't be).");
    $fields['loe_info'] = BaseFieldDefinition::create('string_long')
      ->setDescription("Reviewer's assessment of the levels of evidence found in the article; free text, but following the guidelines used by the board for LOE.");
    $fields['dispositions'] = BaseFieldDefinition::create('entity_reference')
      ->setDescription("One or more values indicating what should be done with the information in this article for the packet's topic.")
      ->setSetting('target_type', 'taxonomy_term')
      ->setSetting('handler_settings',
        ['target_bundles' => ['dispositions' => 'dispositions']])
      ->setCardinality(FieldStorageDefinitionInterface::CARDINALITY_UNLIMITED);
    $fields['reasons'] = BaseFieldDefinition::create('entity_reference')
      ->setDescription('Why the reviewer rejected the article, if applicable.')
      ->setSetting('target_type', 'taxonomy_term')
      ->setSetting('handler_settings',
        ['target_bundles' => ['rejection_reasons' => 'rejection_reasons']])
      ->setCardinality(FieldStorageDefinitionInterface::CARDINALITY_UNLIMITED);
    return $fields;
  }

  /**
   * Find the ID for the rejection disposition.
   */
  public static function getRejectionDisposition() {
    $storage = \Drupal::entityTypeManager()->getStorage('taxonomy_term');
    $ids = $storage->getQuery()->accessCheck(FALSE)
      ->condition('name', self::NO_CHANGES)
      ->condition('vid', 'dispositions')
      ->execute();
    return reset($ids);
  }

}
