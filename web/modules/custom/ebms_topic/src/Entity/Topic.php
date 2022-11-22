<?php

namespace Drupal\ebms_topic\Entity;

use Drupal\Core\Entity\ContentEntityBase;
use Drupal\Core\Entity\ContentEntityInterface;
use Drupal\Core\Entity\EntityTypeInterface;
use Drupal\Core\Field\BaseFieldDefinition;

/**
 * Review topic which can be assigned to an article.
 *
 * Topics are owned by a `Board`, and articles progress through various
 * states separately for each topic assigned. When a board member is
 * asked to review a given article, it is for a specific topic.
 *
 * @ingroup ebms
 *
 * @ContentEntityType(
 *   id = "ebms_topic",
 *   label = @Translation("Topic"),
 *   handlers = {
 *     "view_builder" = "Drupal\Core\Entity\EntityViewBuilder",
 *     "list_builder" = "Drupal\ebms_topic\ListBuilder",
 *
 *     "form" = {
 *       "default" = "Drupal\ebms_topic\Form\TopicForm",
 *       "add" = "Drupal\ebms_topic\Form\TopicForm",
 *       "edit" = "Drupal\ebms_topic\Form\TopicForm",
 *     },
 *     "route_provider" = {
 *       "html" = "Drupal\Core\Entity\Routing\AdminHtmlRouteProvider",
 *     }
 *   },
 *   base_table = "ebms_topic",
 *   admin_permission = "manage topics",
 *   entity_keys = {
 *     "id" = "id",
 *     "label" = "name",
 *   },
 *   links = {
 *     "canonical" = "/admin/config/ebms/topic/{ebms_topic}",
 *     "add-form" = "/admin/config/ebms/topic/add",
 *     "edit-form" = "/admin/config/ebms/topic/{ebms_topic}/edit",
 *     "collection" = "/admin/config/ebms/topic",
 *   }
 * )
 */
class Topic extends ContentEntityBase implements ContentEntityInterface {

  /**
   * Get the `Topic` entity's name.
   */
  public function getName(): string {
    return $this->get('name')->value;
  }

  /**
   * Update the name of the topic (without saving the entity).
   */
  public function setName($name): Topic {
    $this->set('name', $name);
    return $this;
  }

  /**
   * {@inheritdoc}
   */
  public static function baseFieldDefinitions(EntityTypeInterface $entity_type): array {

    $fields = parent::baseFieldDefinitions($entity_type);

    $fields['name'] = BaseFieldDefinition::create('string')
      ->setLabel('Name')
      ->setDescription('The name of the group.')
      ->setSettings([
        'max_length' => 255,
        'text_processing' => 0,
      ])
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

    $fields['board'] = BaseFieldDefinition::create('entity_reference')
      ->setLabel('PDQ® Editorial Board')
      ->setDescription('The board which assigns this topic to articles.')
      ->setRequired(TRUE)
      ->setDisplayOptions('view', [
        'weight' => -3,
        'label' => 'above',
      ])
      ->setDisplayOptions('form', [
        'weight' => -3,
        'type' => 'entity_reference_autocomplete',
      ])
      ->setSetting('target_type', 'ebms_board');

    $fields['nci_reviewer'] = BaseFieldDefinition::create('entity_reference')
      ->setLabel('NCI Reviewer')
      ->setDescription('The default reviewer for the decision made from the abstract for this topic.')
      ->setRequired(TRUE)
      ->setDisplayOptions('view', [
        'weight' => -2,
        'label' => 'above',
      ])
      ->setDisplayOptions('form', [
        'weight' => -2,
        'type' => 'entity_reference_autocomplete',
      ])
      ->setSetting('target_type', 'user');

    $fields['topic_group'] = BaseFieldDefinition::create('entity_reference')
      ->setLabel('Topic Group')
      ->setDescription('Optional string for grouping topics in the UI for boards which have a large number of topics.')
      ->setRequired(FALSE)
      ->setDisplayOptions('view', [
        'weight' => -1,
        'label' => 'above',
      ])
      ->setDisplayOptions('form', [
        'weight' => -1,
        'type' => 'entity_reference_autocomplete',
      ])
      ->setSetting('target_type', 'taxonomy_term')
      ->setSetting('handler_settings', ['target_bundles' => ['topic_groups' => 'topic_groups']]);

    $fields['active'] = BaseFieldDefinition::create('boolean')
      ->setLabel('Active')
      ->setDescription('A flag indicating whether the topic is active.')
      ->setDefaultValue(TRUE)
      ->setDisplayOptions('view', [
        'weight' => 0,
        'label' => 'above',
      ])
      ->setDisplayOptions('form', [
        'type' => 'boolean_checkbox',
        'weight' => 0,
      ]);

    return $fields;
  }

  /**
   * Get the array of topic names, indexed by ID.
   *
   * @param int|array $board_id
   *   Optional board ID to limit topics to a single board.
   *
   * @param bool $active_only
   *   If FALSE (the default) include inactive topics.
   *
   * @return array
   *   Array of topic names indexed by their entity IDs.
   */
  public static function topics(int|array $boards = [], bool $active_only = FALSE): array {
    $storage = \Drupal::entityTypeManager()->getStorage('ebms_topic');
    $query = $storage->getQuery()->accessCheck(FALSE);
    if ($active_only) {
      $query->condition('active', TRUE);
    }
    $query->sort('name');
    if (!empty($boards)) {
      $operator = is_array($boards) ? 'IN' : '=';
      $query->condition('board', $boards, $operator);
    }
    $entities = $storage->loadMultiple($query->execute());
    $topics = [];
    foreach ($entities as $entity) {
      $topics[$entity->id()] = $entity->getName();
    }
    return $topics;
  }

  /**
   * Find the revewers for this topic.
   *
   * @return array
   *   Names of the default reviewers for this topic, indexed by user ID.
   */
  public function reviewers(): array {
    $storage = \Drupal::entityTypeManager()->getStorage('user');
    $query = $storage->getQuery()->accessCheck(FALSE);
    $query->condition('topics', $this->id());
    $query->sort('name');
    $reviewers = [];
    foreach ($storage->loadMultiple($query->execute()) as $user) {
      $reviewers[$user->id()] = $user->name->value;
    }
    return $reviewers;
  }
}
