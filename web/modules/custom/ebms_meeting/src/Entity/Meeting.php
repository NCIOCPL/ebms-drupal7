<?php

namespace Drupal\ebms_meeting\Entity;

use Drupal\Core\Entity\ContentEntityBase;
use Drupal\Core\Entity\ContentEntityInterface;
use Drupal\Core\Entity\EntityTypeInterface;
use Drupal\Core\Entity\Query\QueryInterface;
use Drupal\Core\Field\BaseFieldDefinition;
use Drupal\Core\Field\FieldStorageDefinitionInterface;
use Drupal\Core\Link;
use Drupal\user\Entity\User;

/**
 * Meeting events for PDQ boards and other groups.
 *
 * @ingroup ebms
 *
 * @ContentEntityType(
 *   id = "ebms_meeting",
 *   label = @Translation("Meeting"),
 *   handlers = {
 *     "view_builder" = "Drupal\Core\Entity\EntityViewBuilder",
 *     "list_builder" = "Drupal\ebms_meeting\ListBuilder",
 *     "form" = {
 *       "delete" = "Drupal\Core\Entity\ContentEntityDeleteForm",
 *     },
 *     "route_provider" = {
 *       "html" = "Drupal\Core\Entity\Routing\AdminHtmlRouteProvider",
 *     }
 *   },
 *   base_table = "ebms_meeting",
 *   admin_permission = "administer site configuration",
 *   entity_keys = {
 *     "id" = "id",
 *     "label" = "name",
 *     "published" = "published",
 *   },
 *   links = {
 *     "canonical" = "/meeting/{ebms_meeting}",
 *     "collection" = "/admin/content/meeting",
 *   }
 * )
 */
class Meeting extends ContentEntityBase implements ContentEntityInterface {

  /**
   * Name of the status for a meeting which is scheduled.
   */
  const SCHEDULED = 'Scheduled';

  /**
   * Name of the status for a meeting which has been canceled.
   */
  const CANCELED = 'Canceled';

  /**
   * {@inheritdoc}
   */
  public static function baseFieldDefinitions(EntityTypeInterface $entity_type): array {

    $fields = parent::baseFieldDefinitions($entity_type);

    $fields['user'] = BaseFieldDefinition::create('entity_reference')
      ->setLabel('Entered By')
      ->setDescription('Who scheduled the meeting.')
      ->setDisplayOptions('view', ['label' => 'above'])
      ->setRequired(TRUE)
      ->setSetting('target_type', 'user');

    // Won't show up in default view, possibly due to the bug reported
    // in https://www.drupal.org/node/2489476. See also the item
    // at https://drupal.stackexchange.com/questions/244127.
    $fields['entered'] = BaseFieldDefinition::create('datetime')
      ->setLabel('Entered')
      ->setRequired(TRUE)
      ->setDisplayOptions('view', ['label' => 'above'])
      ->setDescription('When the meeting was entered.');

    $fields['name'] = BaseFieldDefinition::create('string')
      ->setLabel('Name')
      ->setDescription('Display name for the meeting.')
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

    $fields['dates'] = BaseFieldDefinition::create('daterange')
      ->setLabel('Date and Time')
      ->setDescription('When the meeting will take place.')
      ->setRequired(TRUE)
      ->setDisplayOptions('form', ['type' => 'daterange_default'])
      ->setDisplayOptions('view', ['label' => 'above']);

    $fields['type'] = BaseFieldDefinition::create('entity_reference')
      ->setLabel('Meeting Type')
      ->setRequired(TRUE)
      ->setDescription('How the meeting will take place (e.g., in person, via WebEx).')
      ->setSetting('target_type', 'taxonomy_term')
      ->setSetting('handler_settings',
        ['target_bundles' => ['meeting_types' => 'meeting_types']])
      ->setDisplayOptions('view', ['label' => 'above'])
      ->setDisplayOptions('form', ['type' => 'options_buttons']);

    $fields['category'] = BaseFieldDefinition::create('entity_reference')
      ->setLabel('Meeting Category')
      ->setDescription('Which type of group will be meeting (e.g., Board).')
      ->setSetting('target_type', 'taxonomy_term')
      ->setSetting('handler_settings',
        ['target_bundles' => ['meeting_categories' => 'meeting_categories']])
      ->setDisplayOptions('view', ['label' => 'above'])
      ->setDisplayOptions('form', ['type' => 'options_buttons']);

    $fields['status'] = BaseFieldDefinition::create('entity_reference')
      ->setLabel('Meeting Status')
      ->setRequired(TRUE)
      ->setDescription('Whether the meeting is on the calendar or canceled.')
      ->setSetting('target_type', 'taxonomy_term')
      ->setSetting('handler_settings',
        ['target_bundles' => ['meeting_statuses' => 'meeting_statuses']])
      ->setDisplayOptions('view', ['label' => 'above'])
      ->setDisplayOptions('form', ['type' => 'options_buttons']);

    $fields['boards'] = BaseFieldDefinition::create('entity_reference')
      ->setLabel('Boards')
      ->setDescription('Optionally select one or more PDQ boards.')
      ->setSetting('target_type', 'ebms_board')
      ->setDisplayOptions('view', ['label' => 'above'])
      ->setDisplayOptions('form', ['type' => 'options_select'])
      ->setCardinality(FieldStorageDefinitionInterface::CARDINALITY_UNLIMITED);

    $fields['groups'] = BaseFieldDefinition::create('entity_reference')
      ->setLabel('Groups')
      ->setDescription('Optionally select one or more groups.')
      ->setSetting('target_type', 'ebms_group')
      ->setDisplayOptions('view', ['label' => 'above'])
      ->setDisplayOptions('form', ['type' => 'options_select'])
      ->setCardinality(FieldStorageDefinitionInterface::CARDINALITY_UNLIMITED);

    $fields['individuals'] = BaseFieldDefinition::create('entity_reference')
      ->setLabel('Individuals')
      ->setDescription('Optionally select one or more individuals.')
      ->setSetting('target_type', 'user')
      ->setDisplayOptions('view', ['label' => 'above'])
      ->setDisplayOptions('form', ['type' => 'options_select'])
      ->setCardinality(FieldStorageDefinitionInterface::CARDINALITY_UNLIMITED);

    $fields['agenda'] = BaseFieldDefinition::create('text_long')
      ->setLabel('Agenda')
      ->setDescription('Rich text agenda for the meeting.')
      ->setDisplayOptions('form', ['type' => 'text_textarea'])
      ->setDisplayOptions('view', ['label' => 'above']);

    $fields['notes'] = BaseFieldDefinition::create('text_long')
      ->setLabel('Notes')
      ->setDescription('Rich text notes for the meeting.')
      ->setDisplayOptions('form', ['type' => 'text_textarea'])
      ->setDisplayOptions('view', ['label' => 'above']);

    $fields['published'] = BaseFieldDefinition::create('boolean')
      ->setLabel('Publish')
      ->setDescription('If checked this meeting will be shown on the calendar.')
      ->setDisplayOptions('form', ['type' => 'boolean_checkbox']);

    $fields['agenda_published'] = BaseFieldDefinition::create('boolean')
      ->setLabel('Publish Agenda')
      ->setDescription('Unless this is checked the agenda will not be shown to board members.')
      ->setDefaultValue(FALSE)
      ->setDisplayOptions('form', ['type' => 'boolean_checkbox'])
      ->setDisplayOptions('view', ['label' => 'above']);

    $fields['documents'] = BaseFieldDefinition::create('file')
      ->setLabel('File')
      ->setDescription('Optional files disseminated for the meeting.')
      ->setSetting('file_extensions', 'pdf rtf doc docx pptx')
      ->setSetting('max_filesize', '20MB')
      ->setDisplayOptions('form', ['type' => 'file'])
      ->setDisplayOptions('view', ['type' => 'file'])
      ->setCardinality(FieldStorageDefinitionInterface::CARDINALITY_UNLIMITED);

    return $fields;
  }

  /**
   * Get the start date for the meeting.
   */
  public function getDate() {
    return substr($this->dates->value, 0, 10);
  }

  /**
   * Find the files to be downloaded for the meeting.
   *
   * Includes files for documents attached to the meeting entity,
   * as well as managed files linked from the agenda's HTML.
   *
   * We have an option to just find out if there are any files, so
   * we know whether to create the button for downloading them.
   *
   * @param bool $check
   *   If true, return at most one filename.
   *
   * @return array
   *   Possibly empty list of File names in the public:// directory.
   */
  public function getFiles(bool $check = FALSE) {
    $files = [];
    foreach ($this->documents as $document) {
      $uri = $document->entity->uri->value;
      if (str_starts_with($uri, 'public://')) {
        $files[] = substr($uri, 9);
        if ($check) {
          return $files;
        }
      }
    }
    $agenda = $this->agenda->value;
    if (!empty($agenda)) {
      $doc = new \DOMDocument();
      $doc->loadHTML($agenda);
      $nodes = $doc->getElementsByTagName('a');
      foreach ($nodes as $node) {
        $href = $node->getAttribute('href');
        if (preg_match('@/sites/default/files/(.*)@', $href, $matches)) {
          $files[] = urldecode($matches[1]);
          if ($check) {
            return $files;
          }
        }
      }
    }
    return $files;
  }

  /**
   * Narrow the query for meetings as appropriate.
   *
   * @param QueryInterface $query
   *   The entity query we will (possibly) modify.
   * @param User $user
   *   The user for whom we are modifying the query.
   */
  static public function applyMeetingFilters(QueryInterface $query, User $user) {

    // See if we need to narrow the query to find only board meetings.
    $travel_manager = $user->id() > 1 && $user->hasPermission('manage travel');
    $meetings = \Drupal::request()->query->get('meetings');
    if (empty($meetings) && $travel_manager) {
      $meetings = 'board';
    }
    if ($meetings === 'board') {
      $query->condition('category.entity.name', 'Board');
    }

    // The travel manager gets no more filtering.
    if ($travel_manager) {
      return;
    }

    // We're done if the user has elected to see events to which he or she
    // wasn't invited.
    if ($user->hasPermission('view all meetings')) {
      if ($user->boards->isEmpty() || \Drupal::request()->query->get('boards') === 'all') {
        return;
      }
    }

    // Limit the query to meetings to which the user was invited.
    $invitations = $query->orConditionGroup()
      ->condition('individuals', $user->id());
    $boards = [];
    foreach ($user->boards as $board) {
      $boards[] = $board->target_id;
    }
    if (!empty($boards)) {
      $invitations->condition('boards', $boards, 'IN');
    }
    $groups = [];
    foreach ($user->groups as $group) {
      $groups[] = $group->target_id;
    }
    if (!empty($groups)) {
      $invitations->condition('groups', $groups, 'IN');
    }
    $query->condition($invitations);
  }

  /**
   * Fetch a table of upcoming meetings for a given user.
   *
   * We want at most six meetings, earliest first.
   *
   * @param object $user
   *   User for whom messages should be found.
   *
   * @return array
   *   Render array for the table.
   */
  public static function upcomingMeetings($user): array {
    $storage = \Drupal::entityTypeManager()->getStorage('ebms_meeting');
    $query = $storage->getQuery()->accessCheck(FALSE);
    $query->condition('dates.end_value', date('Y-m-d H:i:s'), '>=');
    $query->range(0, 6);
    $query->sort('dates');
    self::applyMeetingFilters($query, $user);
    $meetings = $storage->loadMultiple($query->execute());
    $items = [];
    $route = 'ebms_meeting.meeting';
    $options = ['query' => \Drupal::request()->query->all()];
    foreach ($meetings as $meeting) {
      $meeting_start = new \DateTime($meeting->dates->value);
      $date = $meeting_start->format('Y-m-d');
      $hour = $meeting_start->format('g');
      $am_pm = $meeting_start->format('a');
      $minutes = $meeting_start->format('i');
      $start = $hour;
      if ($minutes !== '00') {
        $start .= ":$minutes";
      }
      $start .= $am_pm;
      $meeting_end = new \DateTime($meeting->dates->end_value);
      $hour = $meeting_end->format('g');
      $am_pm = $meeting_end->format('a');
      $minutes = $meeting_end->format('i');
      $end = $hour;
      if ($minutes !== '00') {
        $end .= ":$minutes";
      }
      $end .= $am_pm;
      $items[] = [
        'when' => "$date $start to $end E.T.",
        'link' => Link::createFromRoute($meeting->name->value, $route, ['meeting' => $meeting->id()], $options),
        'type' => $meeting->type->entity->name->value,
      ];
    }
    return [
      '#theme' => 'upcoming_meetings',
      '#meetings' => $items,
    ];
  }

}
