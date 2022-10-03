<?php

namespace Drupal\ebms_travel\Form;

use Drupal\Core\Form\FormBase;
use Drupal\Core\Form\FormStateInterface;
use Drupal\ebms_travel\Entity\ReimbursementRequest;
use Drupal\user\Entity\User;

/**
 * Form for submitting a reimbursement request.
 *
 * @ingroup ebms
 */
class ReimbursementRequestForm extends FormBase {

  /**
   * {@inheritdoc}
   */
  public function getFormId(): string {
    return 'ebms_reimbursement_request_form';
  }

  /**
   * {@inheritdoc}
   */
  public function buildForm(array $form, FormStateInterface $form_state): array {

    // Start the top block of the form.
    $meeting_fields = [
      '#type' => 'details',
      '#open' => TRUE,
      '#title' => 'Meeting Information',
    ];

    // The next bits depend on who the current user is.
    $meeting_field = [
      '#type' => 'select',
      '#title' => 'Select Meeting',
      '#description' => 'Meeting for which the expenses were incurred.',
      '#required' => TRUE,
      '#validated' => TRUE,
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
      $meeting_fields['user-id'] = [
        '#type' => 'select',
        '#title' => 'Board Member',
        '#description' => 'Select board member on whose behalf you would like to enter a reimbursement request.',
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
      $meeting_fields['user-controlled'] = [
        '#type' => 'container',
        '#attributes' => ['id' => 'user-controlled'],
        'meeting' => $meeting_field,
      ];
      $arrival_description = 'The day the board member arrived for the meeting.';
      $departure_description = 'The day the board member departed from the meeting.';
      $honorarium_description = 'Please indicate whether the board member is requesting an honorarium.';
      $reimbursement_to_description = 'Indicate whether the board member would like the reimbursement check to be sent to the work or home address we have on file.';
      $total_amount_description = 'You may enter the total amount the board member is requesting, including reimbursement for all expenses and the honorarium, or you may leave this field blank. We will calculate the mileage and per diem amount (if requested) and add that to the reimbursement.';
      $email_description = 'Enter the email address where the confirmation of this request should be sent. We will also use this email to contact the board member if we have questions about the reimbursement request.';
    }
    else {
      $meeting_fields['user-id'] = [
        '#type' => 'hidden',
        '#value' => $user->id(),
      ];
      $meeting_field['#options'] = $this->getMeetings($user);
      $meeting_fields['meeting'] = $meeting_field;
      $arrival_description = 'The day you arrived for the meeting.';
      $departure_description = 'The day you departed from the meeting.';
      $honorarium_description = 'Please indicate whether you are requesting an honorarium.';
      $reimbursement_to_description = 'Indicate whether you would like your reimbursement check to be sent to the work or home address we have on file.';
      $total_amount_description = 'You may enter the total amount you are requesting, including reimbursement for all expenses and your honorarium, or you may leave this field blank. We will calculate your mileage and per diem amount (if requested) and add that to your reimbursement.';
      $email_description = 'Enter the email address where you would like a confirmation of this request to be sent. We will also use this email to contact you if we have questions about your reimbursement request.';
    }
    $meeting_fields['arrival'] = [
      '#type' => 'date',
      '#date_date_format' => 'Y-m-d',
      '#title' => 'Arrival',
      '#description' => $arrival_description,
      '#required' => TRUE,
    ];
    $meeting_fields['departure'] = [
      '#type' => 'date',
      '#date_date_format' => 'Y-m-d',
      '#title' => 'Departure',
      '#description' => $departure_description,
      '#required' => TRUE,
    ];


    // Create the dynamic block for transportation expense fields.
    $pov_id = $this->transportationExpenseType('private');
    $transportation_expense_count = $form_state->get('transportation-expense-count', 0);
    $transportation_expenses = [
      '#type' => 'details',
      '#open' => TRUE,
      '#title' => 'Transportation Expenses',
      '#description' => '<em>Either a dollar amount or mileage is required for each expense, as are the expense date and type.</em>',
      'expense-fields' => [
        '#type' => 'container',
        '#attributes' => ['id' => 'transportation-expenses'],
        'transportation-expense-count' => [
          '#type' => 'hidden',
          '#value' => $transportation_expense_count,
        ],
        'pov' => [
          '#type' => 'hidden',
          '#value' => $pov_id,
        ],
      ],
    ];

    // Add the expense lines the user has created.
    for ($i = 1; $i <= $transportation_expense_count; ++$i) {
      $expense_type = $form_state->getValue("transportation-type-$i") ?: '';
      $transportation_expenses['expense-fields']["transportation-expense-$i"] = [
        '#type' => 'container',
        '#attributes' => ['class' => ['inline-fields', 'travel-expense-fields']],
        "transportation-date-$i" => [
          '#type' => 'date',
          '#date_date_element' => 'date',
          '#date_date_format' => 'Y-m-d',
          '#required' => TRUE,
          '#default_value' => $form_state->getValue("transportation-date-$i") ?: '',
        ],
        "transportation-type-$i" => [
          '#type' => 'select',
          '#options' => $this->getPicklistValues('transportation_expense_types'),
          '#empty_option' => '- Type -',
          '#required' => TRUE,
          '#default_value' => $expense_type,
          '#attributes' => ['onchange' => "ebms_transportation_expense_type_changed()"],
        ],
        "transportation-amount-$i" => [
          '#type' => 'number',
          '#placeholder' => 'Amount',
          '#step' => .01,
          '#default_value' => $form_state->getValue("transportation-amount-$i") ?: '',
          /* Broken. See https://www.drupal.org/project/drupal/issues/1091852.
          '#states' => [
            'invisible' => [
              ':input[name="transportation-type-' . $i . '"]' => ['value' => $pov_id],
            ],
          ],
          */
        ],
        "transportation-mileage-$i" => [
          '#type' => 'number',
          '#title' => 'Mileage',
          '#placeholder' => 'Miles',
          '#default_value' => $form_state->getValue("transportation-mileage-$i") ?: '',
          /* Broken. https://www.drupal.org/project/drupal/issues/1091852.
          '#states' => [
            'visible' => [
              ':input[name="transportation-type-' . $i . '"]' => ['value' => $pov_id],
            ],
          ],
          */
        ],
      ];
    }

    // Add buttons for adding/removing expense lines.
    $transportation_expenses['expense-fields']['add-transportation-expense'] = [
      '#type' => 'submit',
      '#value' => 'Add Transportation Expense',
      '#submit' => ['::addTransportationExpense'],
      '#limit_validation_errors' => [],
      '#ajax' => [
        'callback' => '::transportationExpenseCallback',
        'wrapper' => 'transportation-expenses',
      ],
    ];
    if (!empty($transportation_expense_count)) {
      $transportation_expenses['expense-fields']['remove-transportation-expense'] = [
        '#type' => 'submit',
        '#value' => 'Remove Last Transportation Expense',
        '#submit' => ['::removeTransportationExpense'],
        '#limit_validation_errors' => [],
        '#ajax' => [
          'callback' => '::transportationExpenseCallback',
          'wrapper' => 'transportation-expenses',
        ],
      ];
    }

    // Create the dynamic block for parking/toll expense fields.
    $parking_and_tolls_expense_count = $form_state->get('parking-and-tolls-expense-count', 0);
    $parking_and_tolls_expenses = [
      '#type' => 'details',
      '#open' => TRUE,
      '#title' => 'Parking and Tolls Expenses',
      '#description' => '<em>The date, type, and amount fields are required for each expense.</em>',
      'expense-fields' => [
        '#type' => 'container',
        '#attributes' => ['id' => 'parking-and-tolls-expenses'],
        'parking-and-tolls-expense-count' => [
          '#type' => 'hidden',
          '#value' => $parking_and_tolls_expense_count,
        ],
      ],
    ];

    // Add the expense lines the user has created.
    for ($i = 1; $i <= $parking_and_tolls_expense_count; ++$i) {
      $parking_and_tolls_expenses['expense-fields']["parking-or-toll-expense-$i"] = [
        '#type' => 'container',
        '#attributes' => ['class' => ['inline-fields', 'travel-expense-fields']],
        "parking-or-toll-date-$i" => [
          '#type' => 'date',
          '#date_date_element' => 'date',
          '#date_date_format' => 'Y-m-d',
          '#required' => TRUE,
          '#default_value' => $form_state->getValue("parking-or-toll-date-$i") ?: '',
        ],
        "parking-or-toll-type-$i" => [
          '#type' => 'select',
          '#options' => $this->getPicklistValues('parking_or_toll_expense_types'),
          '#empty_option' => '- Type -',
          '#required' => TRUE,
          '#default_value' => $form_state->getValue("parking-or-toll-type-$i") ?: '',
        ],
        "parking-or-toll-amount-$i" => [
          '#type' => 'number',
          '#placeholder' => 'Amount',
          '#step' => .01,
          '#required' => TRUE,
          '#default_value' => $form_state->getValue("parking-or-toll-amount-$i") ?: '',
        ],
      ];
    }

    // Add buttons for adding/removing expense lines.
    $parking_and_tolls_expenses['expense-fields']['add-parking-or-toll-expense'] = [
      '#type' => 'submit',
      '#value' => 'Add Parking Or Toll Expense',
      '#submit' => ['::addParkingOrTollExpense'],
      '#limit_validation_errors' => [],
      '#ajax' => [
        'callback' => '::parkingOrTollExpenseCallback',
        'wrapper' => 'parking-and-tolls-expenses',
      ],
    ];
    if (!empty($parking_and_tolls_expense_count)) {
      $parking_and_tolls_expenses['expense-fields']['remove-parking-or-toll-expense'] = [
        '#type' => 'submit',
        '#value' => 'Remove Last Parking Or Toll Expense',
        '#submit' => ['::removeParkingOrTollExpense'],
        '#limit_validation_errors' => [],
        '#ajax' => [
          'callback' => '::parkingOrTollExpenseCallback',
          'wrapper' => 'parking-and-tolls-expenses',
        ],
      ];
    }

    // Assemble and return the render array for the form.
    return [
      '#attached' => ['library' => ['ebms_travel/reimbursement']],
      '#title' => 'Reimbursement Request',
      'instructions' => [
        '#markup' => $this->config('ebms_travel.instructions')->get('reimbursement'),
      ],
      'meeting-fields' => $meeting_fields,
      'transportation-expenses' => $transportation_expenses,
      'parking-and-tolls-expenses' => $parking_and_tolls_expenses,
      'hotel' => [
        '#type' => 'details',
        '#open' => TRUE,
        '#title' => 'Hotel Expenses',
        'hotel-payment-method' => [
          '#type' => 'radios',
          '#title' => 'Payment',
          '#options' => $this->getPicklistValues('hotel_payment_methods'),
          '#required' => TRUE,
          '#description' => 'Specify who paid for the hotel room.',
        ],
        'nights-stayed' => [
          '#type' => 'number',
          '#title' => 'Nights Stayed',
          '#description' => "Indicate the number of nights stayed in a hotel at NCI's expense.",
          '#attributes' => ['step' => 1],
        ],
        'hotel-amount' => [
          '#type' => 'number',
          '#title' => 'Hotel Amount',
          '#description' => 'How much was paid for the hotel room?',
          '#step' => .01,
        ],
      ],
      'options' => [
        '#type' => 'details',
        '#open' => TRUE,
        '#title' => 'Options',
        'meals-and-incidentals' => [
          '#type' => 'radios',
          '#title' => 'Meals and Incidentals',
          '#description' => 'Board members who live less than 50 miles from our building are not eligible to receive a per diem.',
          '#options' => $this->getPicklistValues('meals_and_incidentals'),
          '#required' => TRUE,
        ],
        'honorarium' => [
          '#type' => 'radios',
          '#title' => 'Honorarium',
          '#description' => $honorarium_description,
          '#options' => [
            'requested' => 'Honorarium requested',
            'declined' => 'Honorarium declined',
          ],
          '#required' => TRUE,
        ],
        'reimbursement-to' => [
          '#type' => 'radios',
          '#title' => 'Reimbursement To',
          '#description' => $reimbursement_to_description,
          '#options' => $this->getPicklistValues('reimbursement_to'),
          '#required' => TRUE,
        ],
      ],
      'summary' => [
        '#type' => 'details',
        '#open' => TRUE,
        '#title' => 'Overall',
        'total-amount' => [
          '#type' => 'number',
          '#title' => 'Total Amount Requested',
          '#description' => $total_amount_description,
          '#step' => .01,
        ],
        'comments' => [
          '#type' => 'textarea',
          '#title' => 'Comments',
          '#description' => 'Optionally provide any addition information about this request.',
        ],
        'certification' => [
          '#type' => 'checkboxes',
          '#title' => 'Certify',
          '#required' => TRUE,
          '#options' => ['certified' => 'I certify that the above information is true and correct to the best of my knowledge.'],
          '#description' => 'Checking this box is required in order to submit the reimbursement request.',
        ],
        'email' => [
          '#type' => 'email',
          '#title' => 'Email',
          '#required' => TRUE,
          '#description' => $email_description,
        ],
      ],
      'submit' => [
        '#type' => 'submit',
        '#value' => 'Submit',
      ],
    ];
  }

  /**
   * {@inheritdoc}
   */
  public function validateForm(array &$form, FormStateInterface $form_state) {
    $arrival = $form_state->getValue('arrival');
    $departure = $form_state->getValue('departure');
    if ($departure < $arrival) {
      $form_state->setErrorByName('arrival', 'Time travel is not supported. The arrival date cannot be later than the departure date.');
    }
    $transportation_expense_count = $form_state->getValue('transportation-expense-count');
    $transportation = [];
    $pov_id = $this->transportationExpenseType('private');
    for ($i = 1; $i <= $transportation_expense_count; $i++) {
      $date = $form_state->getValue("transportation-date-$i");
      $type = $form_state->getValue("transportation-type-$i");
      if ($type == $pov_id) {
        $mileage = $form_state->getValue("transportation-mileage-$i");
        if (empty($mileage)) {
          $form_state->setErrorByName("transportation-mileage-$i", 'Mileage not specified.');
        }
        elseif ($mileage < 0) {
          $form_state->setErrorByName("transportation-mileage-$i", 'Mileage cannot be a negative value.');
        }
        else {
          $transportation[] = [
            'date' => $date,
            'type' => $type,
            'mileage' => $mileage,
          ];
        }
      }
      else {
        $amount = $form_state->getValue("transportation-amount-$i");
        if (empty($amount)) {
          $form_state->setErrorByName("transportation-amount-$i", 'Amount not specified.');
        }
        elseif ($amount < 0) {
          $form_state->setErrorByName("transportation-amount-$i", 'Amount cannot be a negative value.');
        }
        else {
          $transportation[] = [
            'date' => $date,
            'type' => $type,
            'amount' => $amount,
          ];
        }
      }
    }
    $form_state->setValue('transportation', $transportation);
    $parking_and_tolls_expense_count = $form_state->getValue('parking-and-tolls-expense-count');
    $parking_and_tolls = [];
    for ($i = 1; $i <= $parking_and_tolls_expense_count; $i++) {
      $date = $form_state->getValue("parking-or-toll-date-$i");
      $type = $form_state->getValue("parking-or-toll-type-$i");
      $amount = $form_state->getValue("parking-or-toll-amount-$i");
      if ($amount <= 0) {
        $form_state->setErrorByName("parking-or-toll-amount-$i", 'Amount cannot be a negative value.');
      }
      else {
        $values = [
          'date' => $date,
          'type' => $type,
          'amount' => $amount,
        ];
        $parking_and_tolls[] = $values;
      }
    }
    $form_state->setValue('parking_and_tolls', $parking_and_tolls);
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
      'arrival' => $form_state->getValue('arrival'),
      'departure' => $form_state->getValue('departure'),
      'transportation' => $form_state->getValue('transportation'),
      'parking_and_tolls' => $form_state->getValue('parking_and_tolls'),
      'hotel_payment' => $form_state->getValue('hotel-payment-method'),
      'nights_stayed' => $form_state->getValue('nights-stayed'),
      'hotel_amount' => $form_state->getValue('hotel-amount'),
      'meals_and_incidentals' => $form_state->getValue('meals-and-incidentals'),
      'honorarium_requested' => $form_state->getValue('honorarium') === 'requested',
      'reimburse_to' => $form_state->getValue('reimbursement-to'),
      'total_amount' => $form_state->getValue('total-amount'),
      'comments' => $form_state->getValue('comments'),
      'certified' => TRUE,
      'confirmation_email' => $form_state->getValue('email'),
    ];
    $request = ReimbursementRequest::create($values);
    $request->save();
    $this->messenger()->addMessage('Successfully submitted reimbursement request.');
    $this->sendEmailNotifications($request);
    $form_state->setRedirect('ebms_travel.landing_page');
  }

  /**
   * Create another block of transportation expense fields.
   *
   * @param array $form
   *   The request form's render array.
   * @param FormStateInterface $form_state
   *   Access to the form's values.
   */
  public function addTransportationExpense(array &$form, FormStateInterface $form_state) {
    $count = $form_state->get('transportation-expense-count') ?: 0;
    $form_state->set('transportation-expense-count',  $count + 1);
    $form_state->setRebuild();
  }

  /**
   * Remove the last block of transportation expense fields.
   *
   * @param array $form
   *   The request form's render array.
   * @param FormStateInterface $form_state
   *   Access to the form's values.
   */
  public function removeTransportationExpense(array &$form, FormStateInterface $form_state) {
    $form_state->set('transportation-expense-count', $form_state->get('transportation-expense-count') - 1);
    $form_state->setRebuild();
  }

  /**
   * Update the blocks of transportation expense fields.
   *
   * @param array $form
   *   The request form's render array.
   * @param FormStateInterface $form_state
   *   Access to the form's values.
   */
  public function transportationExpenseCallback(array &$form, FormStateInterface $form_state) {
    return $form['transportation-expenses']['expense-fields'];
  }

  /**
   * Create another block of parking or toll expense fields.
   *
   * @param array $form
   *   The request form's render array.
   * @param FormStateInterface $form_state
   *   Access to the form's values.
   */
  public function addParkingOrTollExpense(array &$form, FormStateInterface $form_state) {
    $count = $form_state->get('parking-and-tolls-expense-count') ?: 0;
    $form_state->set('parking-and-tolls-expense-count', $count + 1);
    $form_state->setRebuild();
  }

  /**
   * Remove the last block of parking or toll expense fields.
   *
   * @param array $form
   *   The request form's render array.
   * @param FormStateInterface $form_state
   *   Access to the form's values.
   */
  public function removeParkingOrTollExpense(array &$form, FormStateInterface $form_state) {
    $form_state->set('parking-and-tolls-expense-count', $form_state->get('parking-and-tolls-expense-count') - 1);
    $form_state->setRebuild();
  }

  /**
   * Update the blocks of parking or toll expense fields.
   *
   * @param array $form
   *   The request form's render array.
   * @param FormStateInterface $form_state
   *   Access to the form's values.
   */
  public function parkingOrTollExpenseCallback(array &$form, FormStateInterface $form_state) {
    return $form['parking-and-tolls-expenses']['expense-fields'];
  }

  /**
   * When the user changes, the list of meetings changes as well.
   */
  public function userChangeCallback(array &$form, FormStateInterface $form_state): array {
    return $form['meeting-fields']['user-controlled'];
  }

  /**
   * Populate the picklist of board members.
   */
  private function getBoardMembers(): array {
    $storage = \Drupal::entityTypeManager()->getStorage('user');
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
   * Get the past meetings to which this user was invited.
   *
   * @param User $user
   *   User for whom this reimbursement reservation request is submitted.
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
    $storage = \Drupal::entityTypeManager()->getStorage('ebms_meeting');
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
    $query->condition('dates.value', $now, '<');
    $query->sort('dates', 'DESC');
    $meetings = [];
    foreach ($storage->loadMultiple($query->execute()) as $meeting) {
      $name = $meeting->name->value;
      $date = substr($meeting->dates->value, 0, 10);
      $meetings[$meeting->id()] = "$name - $date";
    }
    return $meetings;
  }

  /**
   * Notify the travel manager(s) and the board member.
   *
   * @param ReimbursementRequest $request
   *   The newly-submitted reimbursement request entity.
   */
  private function sendEmailNotifications(ReimbursementRequest $request) {

    // Collect the common SMTP values.
    $subject = 'EBMS Reimbursement Request';
    $site_mail = \Drupal::config('system.site')->get('mail');
    $site_name = \Drupal::config('system.site')->get('name');
    $from = "$site_name <$site_mail>";
    $renderer = \Drupal::service('renderer');

    // See if we have managers to whom we can send the notification.
    $host = $this->getRequest()->getHost();
    if ($host === 'ebms.nci.nih.gov') {
      $to = $this->config('ebms_travel.email')->get('manager');
    }
    else {
      $to = $this->config('ebms_travel.email')->get('developers');
      $subject .= " ($host)";
    }
    if (empty($to)) {
      $this->messenger->addWarning('No managers found for sending request notification');
    }
    else {
      $notification = [
        '#theme' => 'reimbursement_request_notification',
        '#request' => $request,
      ];
      $message = $renderer->render($notification);
      $headers = implode("\r\n", [
        'MIME-Version: 1.0',
        'Content-type: text/html; charset=utf-8',
        "From: $from",
      ]);
      $rc = mail($to, $subject, $message, $headers);
      if (empty($rc)) {
        $this->messenger->addWarning('Unable to send request notification to the EBMS Travel Manager(s)');
      }
    }

    // Send a separate notification to the board member. A fresh render array
    // is needed, as Drupal will have marked the first one as "all done,
    // nothing more to do here."
    $notification = [
      '#theme' => 'reimbursement_request_notification',
      '#request' => $request,
      '#user_notification' => TRUE,
    ];
    $message = $renderer->render($notification);
    $to = $request->confirmation_email->value;
    $headers = implode("\r\n", [
      'MIME-Version: 1.0',
      'Content-type: text/html; charset=utf-8',
      "From: $from",
    ]);
    $rc = mail($to, 'Your Reimbursement Request', $message, $headers);
    if (empty($rc)) {
      $this->messenger->addWarning("Unable to send request notification to $to");
    }
  }

  /**
   * Fetch values for a specific picklist.
   *
   * @param string $vocabulary
   *   Vocabulary ID for the term set.
   *
   * @return array
   *   Display values indexed by term IDs.
   */
  private function getPicklistValues(string $vocabulary): array {
    $storage = \Drupal::entityTypeManager()->getStorage('taxonomy_term');
    $query = $storage->getQuery()->accessCheck(FALSE);
    $query->condition('vid', $vocabulary);
    $query->sort('weight');
    $ids = $query->execute();
    $terms = $storage->loadMultiple($ids);
    $values = [];
    foreach ($terms as $term) {
      $values[$term->id()] = $term->name->value;
    }
    return $values;
  }

  /**
   * Get unique entity ID for a specific transportation expense type.
   *
   * Originally the class used an instance property variable to store tne
   * ID for the "privately owned vehicle" expense type, so we didn't have
   * to find it more than once. But it turned out that there's a bug
   * (https://www.drupal.org/project/drupal/issues/3306899) in Drupal core
   * which results in our methods being invoked on objects which have not
   * been properly initialized. So we need to calculate this value each
   * time one of our methods which needs it is invoked. This bug seems
   * especially pernicious, as it casts into doubt the reliability of
   * values collected by dependency injection, an approach widely touted
   * as a "best practice" in the Drupal and Symfony communities.
   *
   * @param string $text_id
   *   Machine name for the term.
   *
   * @return int
   *   Unique ID for the term entity.
   */
  private function transportationExpenseType(string $text_id): int {
    $storage = \Drupal::entityTypeManager()->getStorage('taxonomy_term');
    $query = $storage->getQuery()->accessCheck(FALSE);
    $query->condition('vid', 'transportation_expense_types');
    $query->condition('field_text_id', $text_id);
    $ids = $query->execute();
    if (empty($ids)) {
      throw new \Exception("Unable to find term for '$text_id' transportation expense type.");
    }
    return reset($ids);
  }

}
