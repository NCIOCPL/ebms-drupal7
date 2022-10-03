<?php

namespace Drupal\ebms_import\Entity;

use Drupal\Core\Entity\ContentEntityBase;
use Drupal\Core\Entity\ContentEntityInterface;
use Drupal\Core\Entity\EntityTypeInterface;
use Drupal\Core\Field\BaseFieldDefinition;

/**
 * This entity captures the response from NLM to a PubMed search request.
 *
 * The response, which is expected to be in the "PubMed" format, is parsed
 * by the import software, which extracts the PubMed IDs and fetches the XML
 * documents for the articles represented by those IDs for import into the EBMS.
 * The captured response is sometimes useful after the import has completed, in
 * the event that a problem is reported and we need to troubleshoot.
 *
 * @ingroup ebms
 *
 * @ContentEntityType(
 *   id = "ebms_pubmed_results",
 *   label = @Translation("PubMed Results"),
 *   base_table = "ebms_pubmed_results",
 *   admin_permission = "administer site configuration",
 *   entity_keys = {
 *     "id" = "id"
 *   }
 * )
 */
class PubmedSearchResults extends ContentEntityBase implements ContentEntityInterface {

  /**
   * {@inheritdoc}
   */
  public static function baseFieldDefinitions(EntityTypeInterface $entity_type): array {
    $fields = parent::baseFieldDefinitions($entity_type);

    $fields['submitted'] = BaseFieldDefinition::create('datetime')
      ->setDescription('When the response was handed off to the import software.')
      ->setRequired(TRUE);
    $fields['results'] = BaseFieldDefinition::create('string_long')
      ->setDescription('Search response received from PubMed.')
      ->setRequired(TRUE);

    return $fields;
  }

}
