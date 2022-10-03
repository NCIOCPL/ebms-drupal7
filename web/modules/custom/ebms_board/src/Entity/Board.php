<?php /** @noinspection ALL */

namespace Drupal\ebms_board\Entity;

use Drupal\Core\Entity\ContentEntityBase;
use Drupal\Core\Entity\ContentEntityInterface;
use Drupal\Core\Entity\EntityTypeInterface;
use Drupal\Core\Field\BaseFieldDefinition;

/**
 * Defines the PDQ Board entity.
 *
 * Each board has a manager (a reference to whom is stored in this entity)
 * and multiple board members, who are specialists in the various aspects
 * of cancer treatment, diagnosis, prevention, care, etc. Board membership
 * is stored as a reference in the `User` entity for the user's account.
 * Each board is responsible for reviewing literature for a (possibly
 * large) set of topics. Identification of which board is responsible for
 * a given topic is stored in the `Topic` entity.
 *
 * @ingroup ebms
 *
 * @ContentEntityType(
 *   id = "ebms_board",
 *   label = @Translation("PDQ Board"),
 *   handlers = {
 *     "view_builder" = "Drupal\Core\Entity\EntityViewBuilder",
 *     "list_builder" = "Drupal\ebms_board\BoardListBuilder",
 *
 *     "form" = {
 *       "default" = "Drupal\ebms_board\Form\BoardForm",
 *       "add" = "Drupal\ebms_board\Form\BoardForm",
 *       "edit" = "Drupal\ebms_board\Form\BoardForm",
 *     },
 *     "route_provider" = {
 *       "html" = "Drupal\Core\Entity\Routing\AdminHtmlRouteProvider",
 *     }
 *   },
 *   base_table = "ebms_board",
 *   admin_permission = "administer site configuration",
 *   entity_keys = {
 *     "id" = "id",
 *     "label" = "name",
 *     "published" = "active",
 *   },
 *   links = {
 *     "canonical" = "/admin/config/ebms/board/{ebms_board}",
 *     "add-form" = "/admin/config/ebms/board/add",
 *     "edit-form" = "/admin/config/ebms/board/{ebms_board}/edit",
 *     "collection" = "/admin/config/ebms/board",
 *   }
 * )
 */
class Board extends ContentEntityBase implements ContentEntityInterface {

  /**
   * {@inheritdoc}
   */
  public function getName() {
    return $this->get('name')->value;
  }

  /**
   * {@inheritdoc}
   */
  public function setName($name) {
    $this->set('name', $name);
    return $this;
  }

  /**
   * {@inheritdoc}
   */
  public static function baseFieldDefinitions(
    EntityTypeInterface $entity_type
  ) {

    $fields = parent::baseFieldDefinitions($entity_type);

    $fields['name'] = BaseFieldDefinition::create('string')
      ->setLabel('Name')
      ->setDescription('The name of the PDQ Board.')
      ->setRevisionable(FALSE)
      ->setTranslatable(FALSE)
      ->setSettings([
        'max_length' => 128,
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

    $fields['manager'] = BaseFieldDefinition::create('entity_reference')
      ->setLabel('Board manager')
      ->setSetting('target_type', 'user')
      ->setDisplayOptions('view', [
        'weight' => -3,
        'label' => 'above',
      ])
      ->setDisplayOptions('form', [
        'weight' => -3,
        'type' => 'entity_reference_autocomplete',
      ])
      ->setDescription('User responsible for coordinating the literature review.')
      ->setRevisionable(FALSE)
      ->setTranslatable(FALSE);

    $fields['loe_guidelines'] = BaseFieldDefinition::create('file')
      ->setLabel('LOE guidelines')
      ->setSetting('file_extensions', 'doc docx pdf')
      ->setDisplayOptions('view', [
        'weight' => -2,
        'label' => 'above',
      ])
      ->setDisplayOptions('form', [
        'weight' => -2,
        'type' => 'file_generic',
      ])
      ->setDescription('Level-of-evidence guidelines file.')
      ->setRevisionable(FALSE)
      ->setTranslatable(FALSE);

    $fields['auto_imports'] = BaseFieldDefinition::create('boolean')
      ->setLabel('Automatic imports')
      ->setDescription('Should the import software look for related articles?')
      ->setDefaultValue(FALSE)
      ->setDisplayOptions('form', [
        'type' => 'boolean_checkbox',
        'weight' => -1,
      ]);

    $fields['active'] = BaseFieldDefinition::create('boolean')
      ->setLabel('Active')
      ->setDescription('A flag indicating whether the PDQ Board is active.')
      ->setDefaultValue(TRUE)
      ->setDisplayOptions('form', [
        'type' => 'boolean_checkbox',
        'weight' => 0,
      ]);

    return $fields;
  }

  /**
   * {@inheritdoc}
   */
  public static function sort(ContentEntityInterface $a, ContentEntityInterface $b) {
    return strcmp($a->getName(), $b->getName());
  }

  /**
   * Find the default board for a given user.
   *
   * @param object $user
   *   User whose default board is needed.
   *
   * @return int
   *   ID of board to use by default for this user.
   */
  public static function defaultBoard(object $user): int {
    if (!empty($user->board->target_id)) {
      return $user->board->target_id;
    }
    if (!empty($user->boards->target_id)) {
      return $user->boards->target_id;
    }
    return 0;
  }

  /**
   * Get the array of board names, indexed by ID.
   *
   * @return array
   *   Array of board names indexed by their entity IDs.
   */
  public static function boards(): array {
    $storage = \Drupal::entityTypeManager()->getStorage('ebms_board');
    $query = $storage->getQuery()->accessCheck(FALSE);
    $query->sort('name');
    $entities = $storage->loadMultiple($query->execute());
    $boards = [];
    foreach ($entities as $entity) {
      $boards[$entity->id()] = $entity->getName();
    }
    return $boards;
  }

  /**
   * Get the board's members.
   *
   * @param int|array $boards
   *   ID(s) of the board(s) whose members we want.
   *
   * @return array
   *   References to `User` entities.
   */
  public static function boardMembers(int|array $boards): array {
    $op = is_array($boards) ? 'IN' : '=';
    $storage = \Drupal::entityTypeManager()->getStorage('user');
    $query = $storage->getQuery()->accessCheck(FALSE);
    $query->condition('status.value', 1);
    $query->condition('boards', $boards, $op);
    $query->condition('roles', 'board_member');
    $query->sort('name');
    return $storage->loadMultiple($query->execute());
  }

}
