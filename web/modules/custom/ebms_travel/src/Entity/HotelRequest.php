<?php

namespace Drupal\ebms_travel\Entity;

use Drupal\Core\Entity\ContentEntityBase;
use Drupal\Core\Entity\ContentEntityInterface;
use Drupal\Core\Entity\EntityTypeInterface;
use Drupal\Core\Field\BaseFieldDefinition;

/**
 * Request for a hotel reservation.
 *
 * A board member planning to attend an upcoming board meeting can request
 * that NCI book a hotel room for a specific number of nights.
 *
 * @ingroup ebms
 *
 * @ContentEntityType(
 *   id = "ebms_hotel_request",
 *   label = @Translation("Hotel Request"),
 *   base_table = "ebms_hotel_request",
 *   admin_permission = "administer site configuration",
 *   entity_keys = {
 *     "id" = "id",
 *   },
 * )
 */
class HotelRequest extends ContentEntityBase implements ContentEntityInterface {

  /**
   * {@inheritdoc}
   */
  public static function baseFieldDefinitions(EntityTypeInterface $entity_type) {

    $fields = parent::baseFieldDefinitions($entity_type);

    $fields['user'] = BaseFieldDefinition::create('entity_reference')
      ->setLabel('Board Member')
      ->setSetting('target_type', 'user')
      ->setDescription('Board member for whom the hotel reservation is requested.')
      ->setRequired(TRUE);
    $fields['submitted'] = BaseFieldDefinition::create('datetime')
      ->setLabel('Submitted')
      ->setDescription('When the request was submitted.')
      ->setRequired(TRUE);
    $fields['remote_address'] = BaseFieldDefinition::create('string')
      ->setLabel('Remote Address')
      ->setDescription('The IP address of the client machine from which the request was submitted.')
      ->setRequired(TRUE);
    $fields['meeting'] = BaseFieldDefinition::create('entity_reference')
      ->setLabel('Meeting')
      ->setSetting('target_type', 'ebms_meeting')
      ->setDescription('The meeting the board member will be attending while staying at the hotel.');
    $fields['check_in'] = BaseFieldDefinition::create('datetime')
      ->setLabel('Check-in')
      ->setSettings(['datetime_type' => 'date'])
      ->setDescription('The day the board member plans to check into the hotel.')
      ->setRequired(TRUE);
    $fields['check_out'] = BaseFieldDefinition::create('datetime')
      ->setLabel('Check-out')
      ->setSettings(['datetime_type' => 'date'])
      ->setDescription('The day the board member plans to check out from the hotel.')
      ->setRequired(TRUE);
    $fields['preferred_hotel'] = BaseFieldDefinition::create('entity_reference')
      ->setLabel('Preferred Hotel')
      ->setDescription('The hotel at which the board member would prefer to stay.')
      ->setSetting('target_type', 'taxonomy_term')
      ->setSetting('handler_settings', ['target_bundles' => ['hotels' => 'hotels']]);
    $fields['comments'] = BaseFieldDefinition::create('string_long')
      ->setLabel('Comments')
      ->setDescription('Optional additional information about the request.');

    return $fields;
  }

}
