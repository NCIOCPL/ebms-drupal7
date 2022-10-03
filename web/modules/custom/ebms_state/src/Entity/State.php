<?php

namespace Drupal\ebms_state\Entity;

use Drupal\Core\Entity\ContentEntityBase;
use Drupal\Core\Entity\ContentEntityInterface;
use Drupal\Core\Entity\EntityTypeInterface;
use Drupal\Core\Field\BaseFieldDefinition;
use Drupal\Core\Field\FieldStorageDefinitionInterface;
use Drupal\taxonomy\Entity\Term;

/**
 * The state an article/topic entered at a given point in time.
 *
 * The EBMS tracks state history separately for each Topic assigned to each
 * article, with exactly one State entity marked as current for that topic,
 * as well as the previous State entities recorded for the article/topic
 * combination.
 *
 * An article's states field contains entity references to State entities,
 * so it is clear from the Article entity which State entities belong to a
 * given article. There are places in the system (for example, the
 * PublishQueue entities, which let the librarians mark multiple article/topic
 * combinations for transition to the "Published" state on the page for the
 * initial review) which save a reference directly to a State entity, so we
 * also record a back-reference from this entity to the Article entity to
 * which it belongs.
 *
 * Note: The board is stored separately for this entity, even though it could
 * be derived from the Topic entity, partly for performance, and partly to
 * capture the board to which the topic belonged at the time the state was
 * entered, in the (unlikely) event that a topic migrates from one board to
 * another at some later time.
 *
 * @ingroup ebms
 *
 * @ContentEntityType(
 *   id = "ebms_state",
 *   label = @Translation("State"),
 *   base_table = "ebms_state",
 *   admin_permission = "administer site configuration",
 *   entity_keys = {
 *     "id" = "id"
 *   }
 * )
 */
class State extends ContentEntityBase implements ContentEntityInterface {

  /**
   * {@inheritdoc}
   */
  public static function baseFieldDefinitions(EntityTypeInterface $entity_type): array {
    $fields = parent::baseFieldDefinitions($entity_type);

    $fields['value'] = BaseFieldDefinition::create('entity_reference')
      ->setLabel('State')
      ->setDescription('Taxonomy value identying the state being entered.')
      ->setRequired(TRUE)
      ->setSetting('target_type', 'taxonomy_term')
      ->setSetting('handler_settings', ['target_bundles' => ['states' => 'states']]);
    $fields['board'] = BaseFieldDefinition::create('entity_reference')
      ->setLabel('Board')
      ->setDescription('PDQ editorial board for which the state has been entered.')
      ->setRequired(TRUE)
      ->setSetting('target_type', 'ebms_board');
    $fields['topic'] = BaseFieldDefinition::create('entity_reference')
      ->setLabel('Topic')
      ->setDescription('The topic for which this state has been entered.')
      ->setRequired(TRUE)
      ->setSetting('target_type', 'ebms_topic');
    $fields['user'] = BaseFieldDefinition::create('entity_reference')
      ->setLabel('User')
      ->setDescription('The user who moved the article to the state for this topic.')
      ->setRequired(TRUE)
      ->setSetting('target_type', 'user');
    $fields['entered'] = BaseFieldDefinition::create('datetime')
      ->setLabel('Entered')
      ->setDescription('When the article entered this state for this topic.');
    $fields['active'] = BaseFieldDefinition::create('boolean')
      ->setLabel('Active')
      ->setDefaultValue(TRUE)
      ->setDescription('Set to false if the state entry was in error.');
    $fields['current'] = BaseFieldDefinition::create('boolean')
      ->setLabel('Current')
      ->setDefaultValue(FALSE)
      ->setDescription('Only one state can be current for each article/topic combination.');
    $fields['comments'] = BaseFieldDefinition::create('ebms_comment')
      ->setLabel('Comments')
      ->setDescription('Notes on how/why this state was entered.')
      ->setCardinality(FieldStorageDefinitionInterface::CARDINALITY_UNLIMITED);
    $fields['decisions'] = BaseFieldDefinition::create('ebms_board_decision')
      ->setLabel('Decisions')
      ->setDescription('Board decisions made for this state.')
      ->setCardinality(FieldStorageDefinitionInterface::CARDINALITY_UNLIMITED);
    $fields['deciders'] = BaseFieldDefinition::create('entity_reference')
      ->setSetting('target_type', 'user')
      ->setLabel('Deciders')
      ->setDescription('Board members participating in the decisions made for this state.')
      ->setCardinality(FieldStorageDefinitionInterface::CARDINALITY_UNLIMITED);
    $fields['meetings'] = BaseFieldDefinition::create('entity_reference')
      ->setSetting('target_type', 'ebms_meeting')
      ->setLabel('Meetings')
      ->setDescription('This article is on the agenda for these meetings for this topic.')
      ->setCardinality(FieldStorageDefinitionInterface::CARDINALITY_UNLIMITED);

    return $fields;
  }

  /**
   * Add a comment to the state.
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

  /**
   * Show details if this is a state toward the end of processing.
   *
   * @param int $sequence
   *   The sequence number for approval based on review of the article's
   *   full text. We're interested only in states whose sequence numbers
   *   are beyond this threshold.
   *
   * @return string
   *   Empty if we find no advanced state; otherwise the state's description.
   */
  public function laterStateDescription(int $sequence): string {
    if ($this->value->entity->field_sequence->value <= $sequence) {
      return '';
    }
    if ($this->value->entity->field_text_id->value === 'final_board_decision') {
      $decisions = [];
      foreach ($this->decisions as $state_decision) {
        $term = Term::load($state_decision->decision);
        $decisions[] = $term->name->value;
      }
      if (empty($decisions)) {
        return 'Editorial Board Decision (NO DECISION RECORDED)';
      }
      sort($decisions);
      return 'Editorial Board Decision (' . implode('; ', $decisions) . ')';
    }
    if ($this->value->entity->field_text_id->value === 'on_agenda') {
      $meetings = [];
      foreach ($this->meetings as $meeting) {
        $name = $meeting->entity->name->value;
        $date = substr($meeting->entity->dates->value, 0, 10);
        $meetings[] = "$name - $date";
      }
      if (empty($meetings)) {
        return 'On Agenda (NO MEETINGS RECORDED)';
      }
      return 'On Agenda (' . implode('; ', $meetings) . ')';
    }
    $name = $this->value->entity->name->value;
    return "Board Manager Action ($name)";
  }

  /**
   * Look up the integer ID for a given state value.
   *
   * @param string $text_id
   *   The stable machine ID string name for the state.
   *
   * @return int
   *   The primary key for the state's terminology entity.
   */
  public static function getStateId(string $text_id) {
    $query = \Drupal::database()->select('taxonomy_term__field_text_id', 'text_id');
    $query->condition('bundle', 'states');
    $query->condition('field_text_id_value', $text_id);
    $query->addField('text_id', 'entity_id');
    return $query->execute()->fetchField();
  }

}
