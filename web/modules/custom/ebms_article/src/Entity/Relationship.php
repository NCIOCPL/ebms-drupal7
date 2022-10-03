<?php

namespace Drupal\ebms_article\Entity;

use Drupal\Core\Entity\ContentEntityBase;
use Drupal\Core\Entity\ContentEntityInterface;
use Drupal\Core\Entity\EntityTypeInterface;
use Drupal\Core\Field\BaseFieldDefinition;

/**
 * Captures the relationship of one article to another.
 *
 * Any pair of articles can have more than one relationship.
 *
 * Note that the terminology describing the articles participating in a
 * relationship can be confusing. In the Drupal 7 incarnation of the EBMS,
 * when the user was on what was called the "Full Citation" page for a
 * given article, and clicked the link to bring up the "link related
 * citations" form, the field for the IDs of articles related to the article
 * which the user had just been viewing was labeled "Related Article ID(s),"
 * but the database stored those IDs in the "to_id" column of the database
 * table, and the ID of the original article "to which" they were related
 * was stored in the "from_id" column, which reversed the sense of the
 * relationship. The migration software stores the values in the original
 * "to_id" column in the "related" column of the table in the Drupal 9
 * database (matching the label on the field in which the user identifies
 * the articles related to the article they had been viewing) and the values
 * in the old "from_id" are in the "related_to" column of the new database
 * table. So the right way to think of the column names is to realize that
 * when the user brings up the "Link Related Article(s)" form, they will be
 * identifying other articles which are "related" in some way to the article
 * (whose ID is in the "related_to" column) the article whose page they have
 * been viewing. [Comment reworded to avoid phpcs displeasure with what
 * it calls "gendered" language.]
 *
 * @ingroup ebms
 *
 * @ContentEntityType(
 *   id = "ebms_article_relationship",
 *   label = "Article Relationship",
 *   handlers = {
 *     "form" = {
 *       "deactivate" = "Drupal\ebms_article\Form\RelationshipDeleteForm",
 *     },
 *   },
 *   base_table = "ebms_article_relationship",
 *   admin_permission = "manage articles",
 *   entity_keys = {
 *     "id" = "id",
 *   },
 * )
 */
class Relationship extends ContentEntityBase implements ContentEntityInterface {

  /**
   * {@inheritdoc}
   */
  public static function baseFieldDefinitions(EntityTypeInterface $entity_type): array {
    $fields = parent::baseFieldDefinitions($entity_type);
    $fields['related'] = BaseFieldDefinition::create('entity_reference')
      ->setRequired(TRUE)
      ->setSetting('target_type', 'ebms_article');
    $fields['related_to'] = BaseFieldDefinition::create('entity_reference')
      ->setRequired(TRUE)
      ->setSetting('target_type', 'ebms_article');
    $fields['type'] = BaseFieldDefinition::create('entity_reference')
      ->setRequired(TRUE)
      ->setSetting('target_type', 'taxonomy_term')
      ->setSetting('handler_settings', [
        'target_bundles' => [
          'relationship_types' => 'relationship_types',
        ],
      ]);
    $fields['recorded'] = BaseFieldDefinition::create('datetime')
      ->setRequired(TRUE);
    $fields['recorded_by'] = BaseFieldDefinition::create('entity_reference')
      ->setRequired(TRUE)
      ->setSetting('target_type', 'user');
    $fields['comment'] = BaseFieldDefinition::create('string');
    $fields['inactivated'] = BaseFieldDefinition::create('datetime');
    $fields['inactivated_by'] = BaseFieldDefinition::create('entity_reference')
      ->setSetting('target_type', 'user');
    $fields['suppress'] = BaseFieldDefinition::create('boolean');
    return $fields;
  }

}
