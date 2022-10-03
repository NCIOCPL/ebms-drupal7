<?php

namespace Drupal\ebms_travel\Entity;

use Drupal\Core\Entity\ContentEntityBase;
use Drupal\Core\Entity\ContentEntityInterface;
use Drupal\Core\Entity\EntityTypeInterface;
use Drupal\Core\Field\BaseFieldDefinition;
use Drupal\Core\Field\FieldStorageDefinitionInterface;

/**
 * Request for reimbursement of travel expenses.
 *
 * @ingroup ebms
 *
 * @ContentEntityType(
 *   id = "ebms_reimbursement_request",
 *   label = @Translation("Reimbursement Request"),
 *   base_table = "ebms_reimbursement_request",
 *   admin_permission = "administer site configuration",
 *   entity_keys = {
 *     "id" = "id",
 *   },
 * )
 */
class ReimbursementRequest extends ContentEntityBase implements ContentEntityInterface {

  /**
   * {@inheritdoc}
   */
  public static function baseFieldDefinitions(EntityTypeInterface $entity_type) {

    $fields = parent::baseFieldDefinitions($entity_type);

    $fields['user'] = BaseFieldDefinition::create('entity_reference')
      ->setLabel('Board Member')
      ->setSetting('target_type', 'user')
      ->setDescription('Board member to be reimbursed.')
      ->setRequired(TRUE);
    $fields['meeting'] = BaseFieldDefinition::create('entity_reference')
      ->setRequired(TRUE)
      ->setLabel('Meeting')
      ->setSetting('target_type', 'ebms_meeting')
      ->setDescription('The meeting the board member was attending when the expenses were incurred.');
    $fields['submitted'] = BaseFieldDefinition::create('datetime')
      ->setLabel('Submitted')
      ->setDescription('When the request was submitted.')
      ->setRequired(TRUE);
    $fields['remote_address'] = BaseFieldDefinition::create('string')
      ->setLabel('Remote Address')
      ->setDescription('The IP address of the client machine from which the request was submitted.')
      ->setRequired(TRUE);
    $fields['arrival'] = BaseFieldDefinition::create('datetime')
      ->setLabel('Arrival')
      ->setSettings(['datetime_type' => 'date'])
      ->setDescription('The day the board member arrived for the meeting.')
      ->setRequired(TRUE);
    $fields['departure'] = BaseFieldDefinition::create('datetime')
      ->setLabel('Departure')
      ->setSettings(['datetime_type' => 'date'])
      ->setDescription('The day the board member returned from the meeting.')
      ->setRequired(TRUE);
    $fields['transportation'] = BaseFieldDefinition::create('ebms_transportation_expense')
      ->setLabel('Transportation Expenses')
      ->setDescription('Reimbursable expenses incurred for travel to/from the meeting.')
      ->setCardinality(FieldStorageDefinitionInterface::CARDINALITY_UNLIMITED);
    $fields['parking_and_tolls'] = BaseFieldDefinition::create('ebms_parking_or_toll_expense')
      ->setLabel('Parking and Toll Expenses')
      ->setDescription('Parking and/or toll expenses incurred while attending the meeting.')
      ->setCardinality(FieldStorageDefinitionInterface::CARDINALITY_UNLIMITED);
    $fields['hotel_payment'] = BaseFieldDefinition::create('entity_reference')
      ->setRequired(TRUE)
      ->setLabel('Hotel Payment')
      ->setDescription('Field recording who payed for the hotel.')
      ->setSetting('target_type', 'taxonomy_term')
      ->setSetting('handler_settings', ['target_bundles' => ['hotel_payment_methods' => 'hotel_payment_methods']]);
    $fields['nights_stayed'] = BaseFieldDefinition::create('integer')
      ->setLabel('Nights Stayed')
      ->setDescription('Number of nights the board member stayed at the hotel.');
    $fields['hotel_amount'] = BaseFieldDefinition::create('decimal')
      ->setLabel('Hotel Amount')
      ->setDescription('Number of nights the board member stayed at the hotel.');
    $fields['meals_and_incidentals'] = BaseFieldDefinition::create('entity_reference')
      ->setRequired(TRUE)
      ->setLabel('Meals and Incidentals')
      ->setDescription('Value specifying whether a per diem has been requested or that the board member is ineligible for a per diem.')
      ->setSetting('target_type', 'taxonomy_term')
      ->setSetting('handler_settings', ['target_bundles' => ['meals_and_incidentals' => 'meals_and_incidentals']]);
    $fields['honorarium_requested'] = BaseFieldDefinition::create('boolean')
      ->setRequired(TRUE)
      ->setDescription('Whether the board member has requested to be paid an honorarium for attending the meeting.')
      ->setDefaultValue(FALSE);
    $fields['reimburse_to'] = BaseFieldDefinition::create('entity_reference')
      ->setRequired(TRUE)
      ->setLabel('Reimbursement')
      ->setDescription("Whether the reimbursement should be sent to the board member's home, work, or other location.")
      ->setSetting('target_type', 'taxonomy_term')
      ->setSetting('handler_settings', ['target_bundles' => ['reimbursement_to' => 'reimbursement_to']]);
    $fields['total_amount'] = BaseFieldDefinition::create('decimal')
      ->setLabel('Total Amount')
      ->setDescription('Total amount requested for reimbursement.');
    $fields['comments'] = BaseFieldDefinition::create('string_long')
      ->setLabel('Comments')
      ->setDescription('Optional additional information about the request.');
    $fields['certified'] = BaseFieldDefinition::create('boolean')
      ->setLabel('Certified')
      ->setRequired(TRUE)
      ->setDescription('Confirmation that the requester has certified that the information in the form is correct.')
      ->setDefaultValue(FALSE);
    $fields['confirmation_email'] = BaseFieldDefinition::create('email')
      ->setLabel('Confirmation E-mail')
      ->setDescription('The email address where confirmation of the request is to be sent. Also used if there are questions about the request.')
      ->setRequired(TRUE);

    return $fields;
  }

}
