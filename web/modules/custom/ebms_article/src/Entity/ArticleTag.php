<?php

namespace Drupal\ebms_article\Entity;

use Drupal\Core\Entity\ContentEntityBase;
use Drupal\Core\Entity\ContentEntityInterface;
use Drupal\Core\Entity\EntityTypeInterface;
use Drupal\Core\Field\BaseFieldDefinition;
use Drupal\Core\Field\FieldStorageDefinitionInterface;

/**
 * Association of an article with a specific tag.
 *
 * Some of the tags assigned to an article are specific to one of the
 * topics assigned to the article, and the `ArticleTag` entity references
 * will live in the `ArticleTopic` entity for that article/topic combination.
 * Those tags which are not topic-specific will be referenced directly from
 * the `Article` entity. The taxonomy term entity for each tag carries the
 * information about which of these two types of tag assignment the tag is
 * available for (some tags can be used either way).
 *
 * @ingroup ebms
 *
 * @ContentEntityType(
 *   id = "ebms_article_tag",
 *   label = @Translation("Article Tag"),
 *   base_table = "ebms_article_tag",
 *   entity_keys = {
 *     "id" = "id",
 *   },
 * )
 */
class ArticleTag extends ContentEntityBase implements ContentEntityInterface {

  /**
   * {@inheritdoc}
   */
  public static function baseFieldDefinitions(
    EntityTypeInterface $entity_type
  ): array {
    $fields = parent::baseFieldDefinitions($entity_type);
    $fields['tag'] = BaseFieldDefinition::create('entity_reference')
      ->setRequired(TRUE)
      ->setSetting('target_type', 'taxonomy_term')
      ->setSetting('handler_settings',
                   ['target_bundles' => ['article_tags' => 'article_tags']]);
    $fields['user'] = BaseFieldDefinition::create('entity_reference')
      ->setSetting('target_type', 'user');
    $fields['assigned'] = BaseFieldDefinition::create('datetime')
      ->setRequired(TRUE);
    $fields['comments'] = BaseFieldDefinition::create('ebms_comment')
      ->setCardinality(FieldStorageDefinitionInterface::CARDINALITY_UNLIMITED);
    $fields['active'] = BaseFieldDefinition::create('boolean')
      ->setDefaultValue(TRUE);
    return $fields;
  }

  /**
   * Add a comment to the tag.
   *
   * @param string $body
   *   Text content of the comment.
   * @param string $entered
   *   When the comment was entered.
   * @param int $user
   *   ID of the user who added the comment.
   */
  public function addComment(string $body, string $entered = '', int $user = 0) {
    if (empty($entered)) {
      $entered = date('Y-m-d H:i:s');
    }
    if (empty($user)) {
      $user = \Drupal::currentUser()->id();
    }
    $this->comments[] = [
      'user' => $user,
      'entered' => $entered,
      'body' => $body,
    ];
  }

}
