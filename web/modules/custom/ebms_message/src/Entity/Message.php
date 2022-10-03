<?php

namespace Drupal\ebms_message\Entity;

use Drupal\Core\Entity\ContentEntityBase;
use Drupal\Core\Entity\ContentEntityInterface;
use Drupal\Core\Entity\EntityTypeInterface;
use Drupal\Core\Entity\Query\QueryInterface;
use Drupal\Core\Field\BaseFieldDefinition;
use Drupal\Core\Field\FieldStorageDefinitionInterface;
use Drupal\user\Entity\User;

/**
 * Messages for notification of recent activity.
 *
 * Extra values for each message type are encoded in the values field.
 *
 * @ingroup ebms
 *
 * @ContentEntityType(
 *   id = "ebms_message",
 *   label = @Translation("Message"),
 *   base_table = "ebms_message",
 *   admin_permission = "administer site configuration",
 *   entity_keys = {
 *     "id" = "id",
 *   }
 * )
 */
class Message extends ContentEntityBase implements ContentEntityInterface {

  /**
   * Meeting whose agenda was just made public.
   *
   * Messages of this type have event ID and title values.
   */
  const AGENDA_PUBLISHED = 'agenda published';

  /**
   * One or more articles were just entered the abstract review queue.
   *
   * Messages of this type have no additional values.
   */
  const ARTICLES_PUBLISHED = 'articles published';

  /**
   * Existing meeting whose status just changed from Scheduled to Canceled.
   *
   * Messages of this type have event ID and title values.
   */
  const MEETING_CANCELED = 'meeting canceled';

  /**
   * Existing scheduled meeting whose date/time values changed.
   *
   * Messages of this type have event ID and title values.
   */
  const MEETING_CHANGED = 'meeting changed';

  /**
   * New published messages or old message with status changed to published.
   *
   * Messages of this type have event ID and title values.
   */
  const MEETING_PUBLISHED = 'meeting published';

  /**
   * Existing meeting whose type changed between remote and in-person.
   *
   * Messages of this type have event ID, title, and meeting-type values.
   */
  const MEETING_TYPE_CHANGED = 'meeting type changed';

  /**
   * A new packet of articles to be review was just created.
   *
   * Messages of this type have packet ID and title values.
   */
  const PACKET_CREATED = 'packet created';

  /**
   * A new file has been added to a summaries page.
   *
   * Messages of this type have title, notes, and summary-URL values.
   */
  const SUMMARY_POSTED = 'summary posted';

  /**
   * Message types for the queue of notifications for new summary documents.
   */
  const DOCUMENT_ACTIVITY = [
    Message::SUMMARY_POSTED,
  ];

  /**
   * Message types for the queue of notifications for new packets/articles.
   */
  const LITERATURE_ACTIVITY = [
    Message::ARTICLES_PUBLISHED,
    Message::PACKET_CREATED,
  ];

  /**
   * Message types for the queue of notifications about meeting activiity.
   */
  const MEETING_ACTIVITY = [
    Message::AGENDA_PUBLISHED,
    Message::MEETING_CANCELED,
    Message::MEETING_CHANGED,
    Message::MEETING_PUBLISHED,
    Message::MEETING_TYPE_CHANGED,
  ];

  /**
   * All of the legal values for message type.
   */
  const TYPES = [
    ...Message::DOCUMENT_ACTIVITY,
    ...Message::LITERATURE_ACTIVITY,
    ...Message::MEETING_ACTIVITY,
  ];

  /**
   * Map of the name of an activity group to its message types and
   * number of days to fetch.
   */
  const GROUPS = [
    'literature' => [Message::LITERATURE_ACTIVITY, 30],
    'document' => [Message::DOCUMENT_ACTIVITY, 60],
    'meeting' => [Message::MEETING_ACTIVITY, 60],
  ];

  /**
   * Don't unpack the JSON for the message's values more than once.
   */
  private ?object $cachedValues = NULL;

  /**
   * {@inheritdoc}
   */
  public static function baseFieldDefinitions(EntityTypeInterface $entity_type): array {

    $fields = parent::baseFieldDefinitions($entity_type);

    $fields['message_type'] = BaseFieldDefinition::create('string')
      ->setLabel('Message Type')
      ->setDescription('Type of activity (one of Message::TYPES).')
      ->setRequired(TRUE);
    $fields['user'] = BaseFieldDefinition::create('entity_reference')
      ->setLabel('User')
      ->setDescription('User who performed the action described in the notification message.')
      ->setSetting('target_type', 'user');
    $fields['posted'] = BaseFieldDefinition::create('datetime')
      ->setLabel('Posted')
      ->setRequired(TRUE)
      ->setDescription('When the message was posted to the notification queue.');
    $fields['boards'] = BaseFieldDefinition::create('entity_reference')
      ->setLabel('Boards')
      ->setDescription('PDQ boards associated with the activity.')
      ->setSetting('target_type', 'ebms_board')
      ->setCardinality(FieldStorageDefinitionInterface::CARDINALITY_UNLIMITED);
    $fields['groups'] = BaseFieldDefinition::create('entity_reference')
      ->setLabel('Groups')
      ->setDescription('Groups associated with the activity.')
      ->setSetting('target_type', 'ebms_group')
      ->setCardinality(FieldStorageDefinitionInterface::CARDINALITY_UNLIMITED);
    $fields['individuals'] = BaseFieldDefinition::create('entity_reference')
      ->setLabel('Individuals')
      ->setDescription('Individuals to be notified about the activity.')
      ->setSetting('target_type', 'user')
      ->setCardinality(FieldStorageDefinitionInterface::CARDINALITY_UNLIMITED);
    $fields['extra_values'] = BaseFieldDefinition::create('string_long')
      ->setLabel('Values')
      ->setDescription("JSON-encoded additional values for the activity, specific to the queue type's template.");

    return $fields;
  }

  /**
   * Fetch the object with per-message-type values.
   *
   * @return object
   *   Unpacked object containing additional values for this message's type.
   */
  public function getExtraValues(): object {
    if (is_null($this->cachedValues)) {
      $json = $this->extra_values->value;
      if (empty($json)) {
        $this->cachedValues = (object)[];
      }
      else {
        $this->cachedValues = json_decode($json, FALSE);
      }
    }
    return $this->cachedValues;
  }

  /**
   * Fetch messages for a specific activity group.
   *
   * @param string $group
   *   One of meeting, literature, or document (see Message::GROUPS).
   *
   * @return QueryInterface
   *   Entity query object.
   */
  public static function createQuery($group): QueryInterface {
    $user = User::load(\Drupal::currentUser()->id());
    $storage = \Drupal::entityTypeManager()->getStorage('ebms_message');
    $query = $storage->getQuery()->accessCheck(FALSE);
    list($message_types, $days) = self::GROUPS[$group];
    if ($user->id() != 1) {

      // Don't show newly published articles to board members.
      $board_member = $user->hasPermission('review literature');
      if (in_array(Message::ARTICLES_PUBLISHED, $message_types) && $board_member) {
        $message_types = array_diff($message_types, [Message::ARTICLES_PUBLISHED]);
      }

      // Only show newly created packets to board members.
      if (in_array(Message::PACKET_CREATED, $message_types) && !$board_member) {
        $message_types = array_diff($message_types, [Message::PACKET_CREATED]);
      }

      $or = $query->orConditionGroup()->condition('individuals', $user->id());
      $boards = [];
      foreach ($user->boards as $board) {
        $boards[] = $board->target_id;
      }
      if (!empty($boards)) {
        $or->condition('boards', $boards, 'IN');
      }
      $groups = [];
      foreach ($user->groups as $group) {
        $groups[] = $group->target_id;
      }
      if (!empty($groups)) {
        $or->condition('groups', $groups, 'IN');
      }
      $query->condition($or);
    }
    $query->condition('message_type', $message_types, 'IN');
    $query->condition('posted', date('Y-m-d', strtotime("-$days days")), '>=');
    $query->sort('posted', 'DESC');
    return $query;
  }

}
