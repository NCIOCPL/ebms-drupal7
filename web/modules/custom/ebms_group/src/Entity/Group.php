<?php

namespace Drupal\ebms_group\Entity;

use Drupal\Core\Entity\ContentEntityBase;
use Drupal\Core\Entity\ContentEntityInterface;
use Drupal\Core\Entity\EntityTypeInterface;
use Drupal\Core\Field\BaseFieldDefinition;
use Drupal\Core\Field\FieldStorageDefinitionInterface;

/**
 * Defines the group entity.
 *
 * Groups are more flexible and less formal than boards. Their primary
 * purpose is to make it easier to schedule meetings for subsets of a
 * board's members. Membership in a group is stored in the entity for
 * the user's account, not in this entity. Association of a group with
 * a board is optional, and more than one board can be linked to a group.
 *
 * @ingroup ebms
 *
 * @ContentEntityType(
 *   id = "ebms_group",
 *   label = @Translation("Group"),
 *   handlers = {
 *     "view_builder" = "Drupal\Core\Entity\EntityViewBuilder",
 *     "list_builder" = "Drupal\ebms_group\ListBuilder",
 *
 *     "form" = {
 *       "default" = "Drupal\ebms_group\Form\GroupForm",
 *       "add" = "Drupal\ebms_group\Form\GroupForm",
 *       "edit" = "Drupal\ebms_group\Form\GroupForm",
 *       "delete" = "Drupal\Core\Entity\ContentEntityDeleteForm",
 *     },
 *     "route_provider" = {
 *       "html" = "Drupal\Core\Entity\Routing\AdminHtmlRouteProvider",
 *     }
 *   },
 *   base_table = "ebms_group",
 *   admin_permission = "administer site configuration",
 *   entity_keys = {
 *     "id" = "id",
 *     "label" = "name",
 *   },
 *   links = {
 *     "canonical" = "/admin/config/ebms/group/{ebms_group}",
 *     "add-form" = "/admin/config/ebms/group/add",
 *     "edit-form" = "/admin/config/ebms/group/{ebms_group}/edit",
 *     "delete-form" = "/admin/config/ebms/group/{ebms_group}/delete",
 *     "collection" = "/admin/config/ebms/group",
 *   }
 * )
 */
class Group extends ContentEntityBase implements ContentEntityInterface {

  /**
   * Get the name of the group.
   */
  public function getName(): string {
    return $this->get('name')->value;
  }

  /**
   * Set the name of the group.
   */
  public function setName(string $name): Group {
    $this->set('name', $name);
    return $this;
  }

  /**
   * {@inheritdoc}
   */
  public static function baseFieldDefinitions(
    EntityTypeInterface $entity_type
  ): array {

    $fields = parent::baseFieldDefinitions($entity_type);

    $fields['name'] = BaseFieldDefinition::create('string')
      ->setLabel('Name')
      ->setDescription('The name of the group.')
      ->setRevisionable(FALSE)
      ->setTranslatable(FALSE)
      ->setSettings([
        'max_length' => 255,
        'text_processing' => 0,
      ])
      ->setDefaultValue('')
      ->setDisplayOptions('view', [
        'label' => 'above',
        'type' => 'string',
        'weight' => -4,
      ])
      ->setDisplayOptions('form', [
        'type' => 'string_textfield',
        'weight' => -4,
      ])
      ->setRequired(TRUE);

    $fields['boards'] = BaseFieldDefinition::create('entity_reference')
      ->setLabel('PDQ® Editorial Boards')
      ->setSetting('target_type', 'ebms_board')
      ->setCardinality(FieldStorageDefinitionInterface::CARDINALITY_UNLIMITED)
      ->setDisplayOptions('view', [
        'weight' => -2,
        'label' => 'above',
      ])
      ->setDisplayOptions('form', [
        'weight' => -2,
        'type' => 'entity_reference_autocomplete',
      ])
      ->setDescription('Board(s) associated with this group.')
      ->setRevisionable(FALSE)
      ->setTranslatable(FALSE);

    return $fields;
  }

  /**
   * Get the array of group names, indexed by ID.
   *
   * @return array
   *   Array of group names indexed by their entity IDs.
   */
  public static function groups(): array {
    $storage = \Drupal::entityTypeManager()->getStorage('ebms_group');
    $query = $storage->getQuery()->accessCheck(FALSE);
    $query->sort('name');
    $entities = $storage->loadMultiple($query->execute());
    $groups = [];
    foreach ($entities as $entity) {
      $groups[$entity->id()] = $entity->getName();
    }
    return $groups;
  }
}
