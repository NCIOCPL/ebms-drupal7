<?php

namespace Drupal\ebms_travel\Form;

use Drupal\Core\Entity\EntityTypeManagerInterface;
use Drupal\Core\Form\FormBase;
use Drupal\Core\Form\FormStateInterface;
use Symfony\Component\DependencyInjection\ContainerInterface;
use Drupal\ebms_meeting\Entity\Meeting;
use Drupal\ebms_travel\Entity\HotelRequest;
use Drupal\taxonomy\Entity\Term;
use Drupal\user\Entity\User;

/**
 * Form for submitting a request for a hotel reservation.
 *
 * @ingroup ebms
 */
class HotelRequestForm extends FormBase {

  /**
   * The entity type manager.
   *
   * @var \Drupal\Core\Entity\EntityTypeManagerInterface
   */
  protected EntityTypeManagerInterface $entityTypeManager;

  /**
   * {@inheritdoc}
   */
  public static function create(ContainerInterface $container): HotelRequestForm {
    $instance = parent::create($container);
    $instance->entityTypeManager = $container->get('entity_type.manager');
    return $instance;
  }

  /**
   * {@inheritdoc}
   */
  public function getFormId(): string {
    return 'ebms_hotel_request_form';
  }

  /**
   * {@inheritdoc}
   */
  public function buildForm(array $form, FormStateInterface $form_state): array {

    // Start the form.
    $form = [
      '#title' => 'Hotel Request',
      'instructions' => ['#markup' => $this->config('ebms_travel.instructions')->get('hotel')],
    ];

    // The next bits depend on who the current user is.
    $meeting_field = [
      '#type' => 'select',
      '#title' => 'Meeting',
      '#description' => 'Meeting for which the hotel reservation is needed.',
      '#required' => TRUE,
      '#empty_value' => '',
    ];
    $user = User::load($this->currentUser()->id());
    if ($user->hasPermission('enter travel requests')) {
      $user_id = $form_state->getValue('user-id');
      if (empty($user_id)) {
        $meeting_field['#options'] = [];
        $meeting_field['#empty_option'] = ' - Select a user -';
      }
      else {
        $user = User::load($user_id);
        $meetings = $this->getMeetings($user);
        $meeting_field['#options'] = $meetings;
        $meeting_id = $form_state->getValue('meeting');
        if (!empty($meeting_id) && array_key_exists($meeting_id, $meetings)) {
          $meeting_field['#default_value'] = $meeting_id;
        }
      }
      $form['user-id'] = [
        '#type' => 'select',
        '#title' => 'Board Member',
        '#description' => 'Select board member on whose behalf you would like to enter a hotel request.',
        '#options' => $this->getBoardMembers(),
        '#default_value' => $user_id,
        '#required' => TRUE,
        '#empty_option' => '',
        '#ajax' => [
          'callback' => '::userChangeCallback',
          'wrapper' => 'user-controlled',
          'event' => 'change',
        ],
      ];
      $form['user-controlled'] = [
        '#type' => 'container',
        '#attributes' => ['id' => 'user-controlled'],
        'meeting' => $meeting_field,
      ];
      $check_in_description = 'The day the board member plans to check into the hotel.';
      $check_out_description = 'The day the board member plans to check out of the hotel.';
      $hotel_description = 'Optionally select the hotel at which the board member would prefer to stay.';
    }
    else {
      $form['user-id'] = [
        '#type' => 'hidden',
        '#value' => $user->id(),
      ];
      $meeting_field['#options'] = $this->getMeetings($user);
      $form['meeting'] = $meeting_field;
      $check_in_description = 'The day you plan to check into the hotel.';
      $check_out_description = 'The day you plan to check out of the hotel.';
      $hotel_description = 'Optionally select the hotel at which you would prefer to stay.';
    }

    // Finish off and return the rest of the form.
    $storage = $this->entityTypeManager->getStorage('taxonomy_term');
    $query = $storage->getQuery()->accessCheck(FALSE);
    $query->condition('vid', 'hotels');
    $ids = $query->execute();
    $terms = $storage->loadMultiple($ids);
    $hotels = ['' => 'No preference'];
    foreach ($terms as $term) {
      $hotels[$term->id()] = $term->name->value;
    }
    return $form + [
      'check-in' => [
        '#type' => 'date',
        '#date_date_format' => 'Y-m-d',
        '#title' => 'Check-In',
        '#description' => $check_in_description,
        '#required' => TRUE,
      ],
      'check-out' => [
        '#type' => 'date',
        '#date_date_format' => 'Y-m-d',
        '#title' => 'Check-Out',
        '#description' => $check_out_description,
        '#required' => TRUE,
      ],
      'hotel' => [
        '#type' => 'radios',
        '#title' => 'Preferred Hotel',
        '#options' => $hotels,
        '#description' => $hotel_description,
        '#default_value' => '',
      ],
      'comments' => [
        '#type' => 'textarea',
        '#title' => 'Comments',
        '#description' => 'Optionally provide any addition information about this request.',
      ],
      'submit' => [
        '#type' => 'submit',
        '#value' => 'Save',
      ],
    ];
  }

  /**
   * {@inheritdoc}
   */
  public function submitForm(array &$form, FormStateInterface $form_state) {
    $values = [
      'user' => $form_state->getValue('user-id'),
      'submitted' => date('Y-m-d H:i:s'),
      'remote_address' => $this->getRequest()->getClientIp(),
      'meeting' => $form_state->getValue('meeting'),
      'check_in' => $form_state->getValue('check-in'),
      'check_out' => $form_state->getValue('check-out'),
      'preferred_hotel' => $form_state->getValue('hotel'),
      'comments' => $form_state->getValue('comments'),
    ];
    $request = HotelRequest::create($values);
    $request->save();
    $this->messenger()->addMessage('Successfully submitted hotel reservation request.');
    $this->sendEmailNotification($values);
    $form_state->setRedirect('ebms_travel.landing_page');
  }

  /**
   * When the user changes, the list of meetings changes as well.
   */
  public function userChangeCallback(array &$form, FormStateInterface $form_state): array {
    return $form['user-controlled'];
  }

  /**
   * Populate the picklist of board members.
   */
  private function getBoardMembers(): array {
    $storage = $this->entityTypeManager->getStorage('user');
    $query = $storage->getQuery()->accessCheck(FALSE);
    $query->condition('roles', 'board_member');
    $query->sort('name');
    $query->condition('status', 1);
    $members = [];
    foreach ($storage->loadMultiple($query->execute()) as $user) {
      $members[$user->id()] = $user->getDisplayName();
    }
    return $members;
  }

  /**
   * Get the upcoming meetings to which this user is invited.
   *
   * @param User $user
   *   User for whom this hotel reservation request is submitted.
   *
   * @return array
   *   Picklist values for meetings.
   */
  private function getMeetings(User $user): array {
    $boards = [];
    foreach ($user->boards as $board) {
      $boards[] = $board->target_id;
    }
    $groups = [];
    foreach ($user->groups as $group) {
      $groups[] = $group->target_id;
    }
    $storage = $this->entityTypeManager->getStorage('ebms_meeting');
    $query = $storage->getQuery()->accessCheck(FALSE);
    if (!empty($boards) || !empty($groups)) {
      $membership = $query->orConditionGroup();
      if (!empty($boards)) {
        $membership->condition('boards', $boards, 'IN');
      }
      if (!empty($groups)) {
        $membership->condition('groups', $groups, 'IN');
      }
      $membership->condition('individuals', $user->id());
      $query->condition($membership);
    }
    else {
      $query->condition('individuals', $user->id());
    }
    $now = date('Y-m-dTH:i:s');
    $query->condition('dates.end_value', $now, '>');
    $query->sort('dates');
    $meetings = [];
    foreach ($storage->loadMultiple($query->execute()) as $meeting) {
      $name = $meeting->name->value;
      $date = substr($meeting->dates->value, 0, 10);
      $meetings[$meeting->id()] = "$name - $date";
    }
    return $meetings;
  }

  /**
   * Notify the travel manager that we have a new hotel request.
   *
   * @param array $values
   *   Values from the form.
   */
  private function sendEmailNotification(array $values) {

    // Make sure we have someone to whom we can send the notification.
    $subject = 'Hotel Request';
    $host = $this->getRequest()->getHost();
    if ($host === 'ebms.nci.nih.gov') {
      $to = $this->config('ebms_travel.email')->get('manager');
    }
    else {
      $to = $this->config('ebms_travel.email')->get('developers');
      $subject .= " ($host)";
    }
    if (empty($to)) {
      $this->messenger->addWarning('No recipients found for sending request notification');
      return;
    }

    // We have at least one recipient; assemble and send the message.
    $meeting = Meeting::load($values['meeting']);
    $preferred_hotel = Term::load($values['preferred_hotel']);
    $request = [
      '#theme' => 'hotel_request_notification',
      '#request' => [
        'user' => User::load($values['user'])->name->value,
        'submitted' => $values['submitted'],
        'meeting' => $meeting->name->value,
        'date' => substr($meeting->dates->value, 0, 10),
        'check_in' => $values['check_in'],
        'check_out' => $values['check_out'],
        'preferred_hotel' => empty($preferred_hotel) ? 'No preference' : $preferred_hotel->name->value,
        'comments' => $values['comments'],
      ],
    ];
    $message = \Drupal::service('renderer')->render($request);
    $site_mail = \Drupal::config('system.site')->get('mail');
    $site_name = \Drupal::config('system.site')->get('name');
    $from = "$site_name <$site_mail>";
    $headers = implode("\r\n", [
      'MIME-Version: 1.0',
      'Content-type: text/html; charset=utf-8',
      "From: $from",
    ]);
    $rc = mail($to, $subject, $message, $headers);
    if (empty($rc)) {
      $this->messenger->addWarning('Unable to send request notification to the EBMS Travel Manager');
    }
  }

}
