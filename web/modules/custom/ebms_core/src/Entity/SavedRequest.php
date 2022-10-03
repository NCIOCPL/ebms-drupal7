<?php

namespace Drupal\ebms_core\Entity;

use Drupal\Core\Entity\ContentEntityBase;
use Drupal\Core\Entity\ContentEntityInterface;
use Drupal\Core\Entity\EntityTypeInterface;
use Drupal\Core\Field\BaseFieldDefinition;

/**
 * Persisted field values from a form.
 *
 * A request's parameteres are stored so that they can be retrieved when the form
 * script is re-invoked following a submit.
 *
 * @ingroup ebms
 *
 * @ContentEntityType(
 *   id = "saved_request",
 *   label = @Translation("Saved Request"),
 *   base_table = "saved_request",
 *   entity_keys = {
 *      "id" = "id",
 *   }
 * )
 */
class SavedRequest extends ContentEntityBase implements ContentEntityInterface {

  /**
   * {@inheritdoc}
   */
  public static function baseFieldDefinitions(EntityTypeInterface $entity_type) {

    $fields['id'] = BaseFieldDefinition::create('integer')
      ->setDescription('The ID of the request.')
      ->setRequired(TRUE);

    $fields['request_type'] = BaseFieldDefinition::create('string')
      ->setDescription('The name of the request type (for example, "journal queue").')
      ->setRequired(TRUE);

    $fields['user'] = BaseFieldDefinition::create('entity_reference')
      ->setDescription('The user who submitted the request.')
      ->setSetting('target_type', 'user')
      ->setRequired(TRUE);

    $fields['submitted'] = BaseFieldDefinition::create('datetime')
      ->setDescription('When the request was submitted.')
      ->setRequired(TRUE);

    $fields['parameters'] = BaseFieldDefinition::create('string_long')
      ->setDescription('JSON-encoded parameters for the request.')
      ->setRequired(TRUE);

    return $fields;
  }

  /**
   * Fill in the `user` and `submitted` field values to create a new entity.
   *
   * @param string $request_type
   *   Identification of which type of request we are creating (for example,
   *   "search").
   * @param array $parameters
   *   Values to be serialized as JSON.
   *
   * @return SavedRequest
   *   Instance of this entity type.
   */
  public static function saveParameters(string $request_type, array $parameters): SavedRequest {
    $request = static::create([
      'request_type' => $request_type,
      'user' => \Drupal::currentUser()->id(),
      'submitted' => date('Y-m-d H:i:s'),
      'parameters' => json_encode($parameters),
    ]);
    $request->save();
    return $request;
  }

  /**
   * Give the caller the deserialized values for the request.
   *
   * @return array
   *   Deserialized array of parameter values submitted for the request.
   */
  public function getParameters(): array {
    return json_decode($this->parameters->value, TRUE);
  }

  /**
   * Load the request entity and return the deserialized values.
   *
   * @param int $id
   *   ID of the request entity.
   *
   * @return array
   *   Deserialized array of parameter values submitted for the request.
   */
  public static function loadParameters(int $id): array {
    $request = self::load($id);
    return $request->getParameters();
  }
}
