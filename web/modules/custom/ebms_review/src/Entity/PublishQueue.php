<?php

namespace Drupal\ebms_review\Entity;

use Drupal\Core\Entity\ContentEntityBase;
use Drupal\Core\Entity\ContentEntityInterface;
use Drupal\Core\Entity\EntityTypeInterface;
use Drupal\Core\Field\BaseFieldDefinition;
use Drupal\Core\Field\FieldStorageDefinitionInterface;

/**
 * Track article/state combinations in the article publish queue.
 *
 * Ephemeral entity used to keep track of article/state combinations which
 * have been flagged for publication (see the notes on the "Published" state
 * below) by the librarian as she navigates across the multiple pages of her
 * review queue. Whenever the librarian modifies the queue filter parameters
 * and applies them a new queue is created, without saving any of the choices
 * made for the previous queue.
 *
 * The "Published" state value is used for articles which are ready for a
 * board manager to review from the articles' abstracts for the specified
 * topic. This allows the librarians to approve individual article/topic
 * combinations one by one without having them added to the board manager's
 * abstract review queue as the approvals are made, but instead mark a batch
 * for "publication" to that queue all at once. It is an odd name, but the
 * users got used to it when using the original Visual Basic application,
 * and requested that the name be retained for this state.
 *
 * @ingroup ebms
 *
 * @ContentEntityType(
 *   id = "ebms_publish_queue",
 *   label = @Translation("Publish Queue"),
 *   base_table = "ebms_publish_queue",
 *   admin_permission = "administer site configuration",
 *   entity_keys = {
 *     "id" = "id"
 *   }
 * )
 */
class PublishQueue extends ContentEntityBase implements ContentEntityInterface {

  /**
   * {@inheritdoc}
   */
  public static function baseFieldDefinitions(EntityTypeInterface $entity_type): array {
    $fields = parent::baseFieldDefinitions($entity_type);

    $fields['created'] = BaseFieldDefinition::create('datetime')
      ->setDescription('When the queue was initiated.')
      ->setLabel('Created')
      ->setRequired(TRUE);
    $fields['user'] = BaseFieldDefinition::create('entity_reference')
      ->setLabel('Reviewer')
      ->setRequired(TRUE)
      ->setDescription('User who initiated the publication review queue.')
      ->setSetting('target_type', 'user');
    $fields['filter'] = BaseFieldDefinition::create('string_long')
      ->setDescription('JSON-encoded parameters for the queue filtering.')
      ->setLabel('Filter')
      ->setRequired(TRUE);
    $fields['states'] = BaseFieldDefinition::create('entity_reference')
      ->setLabel('States')
      ->setSetting('target_type', 'ebms_state')
      ->setDescription('Identification of each article/state combination through its current State entity.')
      ->setCardinality(FieldStorageDefinitionInterface::CARDINALITY_UNLIMITED);

    return $fields;
  }


}
