<?php

namespace Drupal\ebms_meeting\Form;

use Drupal\Core\Form\FormBase;
use Drupal\Core\Entity\EntityTypeManagerInterface;
use Drupal\Core\Form\FormStateInterface;
use Drupal\ebms_board\Entity\Board;
use Drupal\ebms_group\Entity\Group;
use Drupal\ebms_meeting\Entity\Meeting;
use Drupal\ebms_message\Entity\Message;
use Symfony\Component\DependencyInjection\ContainerInterface;

/**
 * Create or edit an EBMS meeting entity.
 *
 * @ingroup ebms
 */
class MeetingForm extends FormBase {

  /**
   * The entity type manager.
   *
   * @var \Drupal\Core\Entity\EntityTypeManagerInterface
   */
  protected EntityTypeManagerInterface $entityTypeManager;

  /**
   * {@inheritdoc}
   */
  public function getFormId(): string {
    return 'ebms_meeting_form';
  }

  /**
   * {@inheritdoc}
   */
  public static function create(ContainerInterface $container): MeetingForm {
    $instance = parent::create($container);
    $instance->entityTypeManager = $container->get('entity_type.manager');
    return $instance;
  }

  /**
   * {@inheritdoc}
   */
  public function buildForm(array $form, FormStateInterface $form_state, $meeting = NULL): array {

    // Set some defaults.
    $name = $category = $type = $status = $meeting_id = $agenda = $notes = '';
    $selected_boards = $selected_groups = $selected_individuals = [];
    $published = TRUE;
    $agenda_published = FALSE;
    $boards = Board::boards();
    $groups = Group::groups();
    $title = 'Add Meeting';
    $date = date('Y-m-d');
    $start = '13:00:00';
    $end = '17:30:00';

    // Create some picklists.
    $storage = $this->entityTypeManager->getStorage('user');
    $ids = $storage->getQuery()
      ->accessCheck(FALSE)
      ->condition('uid', 1, '>')
      ->condition('status', 1)
      ->sort('name')
      ->execute();
    $individuals = [];
    foreach ($storage->loadMultiple($ids) as $entity) {
      $individuals[$entity->id()] = $entity->name->value;
    }
    $storage = $this->entityTypeManager->getStorage('taxonomy_term');
    $ids = $storage->getQuery()
      ->accessCheck(FALSE)
      ->condition('vid', 'meeting_categories')
      ->sort('weight')
      ->execute();
    $categories = [];
    foreach ($storage->loadMultiple($ids) as $term) {
      if (empty($category)) {
        $category = $term->id();
      }
      $categories[$term->id()] = $term->name->value;
    }
    $ids = $storage->getQuery()->accessCheck(FALSE)
      ->condition('vid', 'meeting_types')
      ->sort('weight')
      ->execute();
    $types = [];
    foreach ($storage->loadMultiple($ids) as $term) {
      if (empty($type)) {
        $type = $term->id();
      }
      $types[$term->id()] = $term->name->value;
    }
    $ids = $storage->getQuery()->accessCheck(FALSE)
      ->condition('vid', 'meeting_statuses')
      ->sort('weight')
      ->execute();
    $statuses = [];
    foreach ($storage->loadMultiple($ids) as $term) {
      if (empty($status)) {
        $status = $term->id();
      }
      $statuses[$term->id()] = $term->name->value;
    }

    // Override some defaults if we're editing an existing meeting.
    if (!empty($meeting)) {
      $meeting_id = $meeting->id();
      $title = 'Edit Meeting';
      $name = $meeting->name->value;
      $date = substr($meeting->dates->value, 0, 10);
      $start = substr($meeting->dates->value, 11, 8);
      $end = substr($meeting->dates->end_value, 11, 8);
      $agenda = $meeting->agenda->value;
      $notes = $meeting->notes->value;
      $category = $meeting->category->target_id;
      $type = $meeting->type->target_id;
      $status = $meeting->status->target_id;
      $published = $meeting->published->value;
      $agenda_published = $meeting->agenda_published->value;
      $selected_boards = [];
      foreach ($meeting->boards as $board) {
        $selected_boards[] = $board->target_id;
      }
      $selected_groups = [];
      foreach ($meeting->groups as $group) {
        $selected_groups[] = $group->target_id;
      }
      $selected_individuals = [];
      foreach ($meeting->individuals as $individual) {
        $selected_individuals[] = $individual->target_id;
      }
      $files = [];
      foreach ($meeting->documents as $file) {
        $files[] = $file->target_id;
      }
    }
    return [
      '#title' => $title,
      '#attached' => [
        'library' => ['ebms_meeting/meeting-form'],
      ],
      'meeting-id' => [
        '#type' => 'hidden',
        '#value' => $meeting_id,
      ],
      'name' => [
        '#type' => 'textfield',
        '#title' => 'Name',
        '#description' => 'Display name for the meeting.',
        '#required' => TRUE,
        '#default_value' => $name,
      ],
      'schedule' => [
        '#type' => 'details',
        '#title' => 'Meeting Schedule',
        'date' => [
          '#type' => 'date',
          '#title' => 'Date',
          '#date_date_element' => 'date',
          '#date_date_format' => 'Y-m-d',
          '#required' => TRUE,
          '#default_value' => $date,
        ],
        'start' => [
          '#type' => 'date',
          '#title' => 'Start',
          '#attributes' => [
            'type' => 'time',
            'step' => 60 * 15,
          ],
          '#default_value' => $start,
          '#required' => TRUE,
        ],
        'end' => [
          '#type' => 'date',
          '#title' => 'End',
          '#attributes' => [
            'type' => 'time',
            'step' => 60 * 15,
          ],
          '#default_value' => $end,
          '#required' => TRUE,
        ],
      ],
      'options' => [
        '#type' => 'details',
        '#title' => 'Options',
        'type' => [
          '#type' => 'radios',
          '#title' => 'Meeting Type',
          '#description' => 'Whether the meeting is in-person or remote.',
          '#options' => $types,
          '#default_value' => $type,
          '#required' => TRUE,
        ],
        'status' => [
          '#type' => 'radios',
          '#title' => 'Meeting Status',
          '#description' => 'Whether the meeting is still on the calendar.',
          '#options' => $statuses,
          '#default_value' => $status,
          '#required' => TRUE,
        ],
        'category' => [
          '#type' => 'radios',
          '#title' => 'Meeting Category',
          '#description' => "The nature of the meeting's scope.",
          '#options' => $categories,
          '#default_value' => $category,
        ],
      ],
      'participants' => [
        '#type' => 'details',
        '#title' => 'Participants',
        'boards' => [
          '#type' => 'select',
          '#title' => 'Boards',
          '#description' => 'PDQ board(s) for which the meeting is scheduled.',
          '#options' => $boards,
          '#default_value' => $selected_boards,
          '#multiple' => TRUE,
        ],
        'groups' => [
          '#type' => 'select',
          '#title' => 'Groups',
          '#description' => 'Working group(s) for which the meeting is scheduled.',
          '#options' => $groups,
          '#default_value' => $selected_groups,
          '#multiple' => TRUE,
        ],
        'individuals' => [
          '#type' => 'select',
          '#title' => 'Individuals',
          '#description' => 'Individuals invited to the meeting.',
          '#options' => $individuals,
          '#default_value' => $selected_individuals,
          '#multiple' => TRUE,
        ],
      ],
      'filebox' => [
        '#type' => 'details',
        '#title' => 'Meeting Files',
        'files' => [
          '#type' => 'managed_file',
          '#title' => 'Files',
          '#description' => 'Supporting files for this meeting.',
          '#upload_validators' => [
            'file_validate_extensions' => ['pdf rtf doc docx pptx'],
          ],
          '#default_value' => $files,
          '#multiple' => TRUE,
        ],
      ],
      'publication' => [
        '#type' => 'details',
        '#title' => 'Publication',
        'published' => [
          '#type' => 'checkbox',
          '#title' => 'Published?',
          '#description' => "Don't check this until you are ready for the meeting to appear on the calendar.",
          '#default_value' => $published,
        ],
        'agenda-published' => [
          '#type' => 'checkbox',
          '#title' => 'Agenda Published?',
          '#description' => "Don't check this until you are ready for the agenda to be shown to board members.",
          '#default_value' => $agenda_published,
        ],
      ],
      'agenda' => [
        '#type' => 'text_format',
        '#title' => 'Agenda',
        '#description' => 'Topics which will be covered in the meeting.',
        '#format' => 'filtered_html',
        '#default_value' => $agenda,
      ],
      'notes' => [
        '#type' => 'text_format',
        '#title' => 'Notes',
        '#description' => 'Optional additional information about the meeting.',
        '#format' => 'filtered_html',
        '#default_value' => $notes,
      ],
      'submit' => [
        '#type' => 'submit',
        '#value' => 'Save',
      ],
      'cancel' => [
        '#type' => 'submit',
        '#value' => 'Cancel',
        '#submit' => ['::cancelSubmit'],
        '#limit_validation_errors' => [],
      ],
    ];
  }

  /**
   * {@inheritdoc}
   */
  public function submitForm(array &$form, FormStateInterface $form_state) {
    $meeting_id = $form_state->getValue('meeting-id');
    $trigger = $form_state->getTriggeringElement()['#value'];
    if ($trigger === 'Save') {
      $date = $form_state->getValue('date');
      $start = $form_state->getValue('start');
      $end = $form_state->getValue('end');
      $dates = [
        'value' => "{$date}T{$start}",
        'end_value' => "{$date}T{$end}",
      ];
      $old_values = [];
      if (empty($meeting_id)) {
        $meeting = Meeting::create([
          'user' => $this->currentUser()->id(),
          'entered' => date('Y-m-d H:i:s'),
        ]);
        $message = 'New meeting added.';
      }
      else {
        $meeting = Meeting::load($meeting_id);
        $message = 'Meeting saved.';
        $old_values['dates'] = $meeting->dates;
        $old_values['status'] = $meeting->status->entity;
        $old_values['agenda_published'] = $meeting->agenda_published->value;
        $old_values['published'] = $meeting->published->value;
        $old_values['type'] = $meeting->type->target_id;
      }
      $meeting->set('name', $form_state->getValue('name'));
      $meeting->set('dates', $dates);
      $meeting->set('agenda', $form_state->getValue('agenda'));
      $meeting->set('notes', $form_state->getValue('notes'));
      $meeting->set('type', $form_state->getValue('type'));
      $meeting->set('category', $form_state->getValue('category'));
      $meeting->set('status', $form_state->getValue('status'));
      $meeting->set('boards', $form_state->getValue('boards'));
      $meeting->set('groups', $form_state->getValue('groups'));
      $meeting->set('individuals', $form_state->getValue('individuals'));
      $meeting->set('published', $form_state->getValue('published'));
      $meeting->set('agenda_published', $form_state->getValue('agenda-published'));
      $meeting->set('documents', $form_state->getValue('files'));
      if (!empty($meeting->agenda[0]->value)) {
        $meeting->agenda[0]->set('format', 'filtered_html');
      }
      if (!empty($meeting->notes[0]->value)) {
        $meeting->notes[0]->set('format', 'filtered_html');
      }
      $meeting->save();
      $this->addMessages($meeting, $old_values, $form_state);
      $meeting_id = $meeting->id();
      $this->messenger()->addMessage($message);
    }
    if (!empty($meeting_id)) {
      $form_state->setRedirect('ebms_meeting.meeting', ['meeting' => $meeting_id]);
    }
    else {
      $month = $this->getRequest()->query->get('month');
      if (!empty($month)) {
        $form_state->setRedirect('ebms_meeting.calendar', ['month' => $month]);
      }
      else {
        $form_state->setRedirect('entity.ebms_meeting.collection');
      }
    }
  }

  /**
   * Return to the calendar.
   *
   * @param array $form
   *   Form settings.
   * @param \Drupal\Core\Form\FormStateInterface $form_state
   *   Form values.
   */
  public function cancelSubmit(array &$form, FormStateInterface $form_state) {
    $month = $this->getRequest()->query->get('month');
    if (!empty($month)) {
      $form_state->setRedirect('ebms_meeting.calendar', ['month' => $month]);
    }
    else {
      $form_state->setRedirect('entity.ebms_meeting.collection');
    }
  }

  /**
   * Record information to be shown on the home page activity cards.
   *
   * @param Meeting $meeting
   *   The meeting entity which was just saved.
   * @param array $old_values
   *   Values before the save, if this is not a new entity.
   * @param FormStateInterface $form_state
   *   Access to the values entered on the form.
   */
  private function addMessages(Meeting $meeting, array $old_values, FormStateInterface $form_state) {

    // Only record activity for published entities.
    if (!empty($form_state->getValue('published'))) {

      // Collect the values used for all of the activity messages we create.
      $values = [
        'user' => $this->currentUser()->id(),
        'posted' => date('Y-m-d H:i:s'),
        'boards' => $form_state->getValue('boards'),
        'groups' => $form_state->getValue('groups'),
        'individuals' => $form_state->getValue('individuals'),
        'extra_values' => json_encode([
          'meeting_id' => $meeting->id(),
          'title' => $form_state->getValue('name'),
        ]),
      ];
    }

    // Treat a newly published entity as new.
    if (empty($old_values['published'])) {

      // Record the newly published meeting.
      if ($meeting->status->entity->name->value === Meeting::SCHEDULED) {
        $values['message_type'] = Message::MEETING_PUBLISHED;
        Message::create($values)->save();
      }

      // Do the same for the agenda.
      if ($form_state->getValue('agenda-published')) {
        $values['message_type'] = Message::AGENDA_PUBLISHED;
        Message::create($values)->save();
      }
    }

    // Handle a changed meeting.
    else {

      // Have we flipped between canceled and scheduled?
      $new_status = $meeting->status->entity->name->value;
      $old_status = $old_values['status']->name->value;
      if ($new_status !== $old_status) {
        if ($new_status === Meeting::SCHEDULED) {
          $values['message_type'] = Message::MEETING_PUBLISHED;
          Message::create($values)->save();
        }
        else {
          $values['message_type'] = Message::MEETING_CANCELED;
          Message::create($values)->save();
        }
      }

      // If the agenda was just published, remember that action.
      if (!empty($form_state->getValue('agenda-published')) && empty($old_values['agenda_published'])) {
        $values['message_type'] = Message::AGENDA_PUBLISHED;
        Message::create($values)->save();
      }

      // Record a change in the meeting date and/or times.
      if (!empty($old_values['dates'])) {
        $new_dates = $meeting->dates->value . '--' . $meeting->dates->end_value;
        $old_dates = $old_values['dates']->value . '--' . $old_values['dates']->end_value;
        if ($new_dates !== $old_dates) {
          $values['message_type'] = Message::MEETING_CHANGED;
          Message::create($values)->save();
        }
      }

      // If the meeting type just changed, record that change. Do this one
      // last because we're adding to the 'extra_values' array.
      if (!empty($old_values['type']) && $meeting->type->target_id != $old_values['type']) {
        $values['message_type'] = Message::MEETING_TYPE_CHANGED;
        $values['extra_values']['meeting_type'] = $meeting->type->entity->name->value;
        Message::create($values)->save();
      }
    }
  }

}
