<?php

namespace Drupal\ebms_review\Form;

require '../vendor/autoload.php';

use Drupal\Core\Form\FormBase;
use Drupal\Core\Form\FormStateInterface;
use Drupal\Core\Url;
use Drupal\ebms_board\Entity\Board;
use Drupal\ebms_core\Entity\SavedRequest;
use Drupal\ebms_review\Entity\Packet;
use Drupal\ebms_review\Form\RecordResponses;
use Drupal\file\Entity\File;

/**
 * Print a review packet.
 *
 * @ingroup ebms
 */
class PrintPacketForm extends FormBase {

  /**
   * {@inheritdoc}
   */
  public function getFormId(): string {
    return 'print_packet_form';
  }

  /**
   * {@inheritdoc}
   */
  public function buildForm(array $form, FormStateInterface $form_state, $request_id = 0): array|Response {

    // If we have a job request, fetch its parameters.
    $parameters = empty($request_id) ? [] : SavedRequest::loadParameters($request_id);

    // Set up starting defaults.
    $member_picklist = [];
    $packet_picklist = [];
    $member_empty = 'Please select a board.';
    $packet_empty = 'Please select a board member.';
    $packet_disabled = $member_disabled = TRUE;

    // Set up the member picklist if we have a board selection.
    $board_id = $form_state->getValue('board') ?: $parameters['board'] ?? '';
    if (!empty($board_id)) {
      $members = RecordResponses::getMembers($board_id);
      $member_disabled = FALSE;
      if (!empty($members)) {
        $member_picklist = $members;
        $member_empty = '- Select -';
      }
      else {
        $member_empty = 'This board has no pending review packets.';
      }
    }

    // Set up the packet picklist if we have a member selection.
    $member_id = $form_state->getValue('member') ?: $parameters['member'] ?? '';
    if (!empty($member_id)) {
      $packets = RecordResponses::getPackets($member_id, $board_id);
      if (!empty($packets)) {
        $packet_picklist = $packets;
        $packet_disabled = FALSE;
        $packet_empty = '- None -';
      }
    }

    // Assemble the render array for the form.
    $form = [
      '#title' => 'Print Packet',
      'board' => [
        '#type' => 'radios',
        '#title' => 'Board',
        '#description' => 'Select the board for which you want to print a packet.',
        '#options' => Board::boards(),
        '#default_value' => $board_id,
        '#required' => TRUE,
        '#ajax' => [
          'callback' => '::boardChangeCallback',
          'wrapper' => 'board-controlled',
          'event' => 'change',
        ],
      ],
      'board-controlled' => [
        '#type' => 'container',
        '#attributes' => ['id' => 'board-controlled'],
        'member' => [
          '#type' => 'select',
          '#title' => 'Board Member',
          '#description' => 'Select the board member for whom you wish to print a packet.',
          '#options' => $member_picklist,
          '#required' => TRUE,
          '#default_value' => $member_id,
          '#disabled' => $member_disabled,
          '#attributes' => ['autocomplete' => 'off'],
          '#empty_option' => $member_empty,
          '#ajax' => [
            'callback' => '::memberChangeCallback',
            'wrapper' => 'member-controlled',
            'event' => 'change',
          ],
        ],
        'member-controlled' => [
          '#type' => 'container',
          '#attributes' => ['id' => 'member-controlled'],
          'packet' => [
            '#type' => 'select',
            '#title' => 'Packet',
            '#required' => TRUE,
            '#description' => "Select the packet you wish to print.",
            '#options' => $packet_picklist,
            '#default_value' => $parameters['packet'] ?? '',
            '#disabled' => $packet_disabled,
            '#attributes' => ['autocomplete' => 'off'],
            '#empty_option' => $packet_empty,
          ],
        ],
      ],
      'options' => [
        '#type' => 'checkboxes',
        '#title' => 'Options',
        '#options' => [
          'articles' => 'Full-text article PDFs',
          'review-sheets' => 'Review sheets',
          'summaries' => 'Summary documents',
        ],
        '#required' => TRUE,
        '#default_value' => array_diff($parameters['options'] ?? ['articles', 'review-sheets', 'summaries'], [0]),
      ],
      'submit' => [
        '#type' => 'submit',
        '#value' => 'Submit',
      ],
      'reset' => [
        '#type' => 'submit',
        '#value' => 'Reset',
        '#submit' => ['::resetSubmit'],
        '#limit_validation_errors' => [],
      ],
    ];

    // Show what will be printed if we have a request.
    if (!empty($request_id)) {
      $route = 'ebms_review.fetch_print_job';
      $parms = ['request_id' => $request_id];
      $opts = ['class' => ['button', 'usa-button']];
      $documents = [];
      $packet = Packet::load($parameters['packet']);
      $topic_id = $packet->topic->target_id;
      $review_sheets = $total = 0;
      foreach ($packet->articles as $packet_article) {
        if (empty($packet_article->entity->dropped->value)) {
          $article = $packet_article->entity->article->entity;
          if (in_array('articles', $parameters['options'])) {
            if (!empty($article->full_text->file)) {
              $file = File::load($article->full_text->file);
              $documents[] = $file->filename->value;
              $total++;
            }
          }
          if (in_array('review-sheets', $parameters['options'])) {
            $current_state = $article->getCurrentState($topic_id);
            if ($current_state->value->entity->field_text_id->value !== 'fyi') {
              $review_sheets++;
              $total++;
            }
          }
        }
      }
      if (in_array('summaries', $parameters['options'])) {
        foreach ($packet->summaries as $summary) {
          $documents[] = $summary->entity->file->entity->filename->value;
          $total++;
        }
      }
      if ($review_sheets > 0) {
        if ($review_sheets === 1) {
          $doccuments[] = '1 review sheet';
        }
        else {
          $documents[] = "$review_sheets review sheets";
        }
      }
      $form['fetch'] = [
        '#theme' => 'print_job',
        '#url' => Url::fromRoute($route, $parms, $opts),
        '#documents' => $documents,
        '#total' => $total,
      ];
      ebms_debug_log(print_r($parameters, TRUE));
      //$fetch_link = Link::createFromRoute('Fetch Job', $route, $parms, $opts)->toString();
    }
    return $form;
  }

  /**
   * When the board changes, the list of board members changes as well.
   */
  public function boardChangeCallback(array &$form, FormStateInterface $form_state): array {
    return $form['board-controlled'];
  }

  /**
   * When the board member changes, the list of packets changes as well.
   */
  public function memberChangeCallback(array &$form, FormStateInterface $form_state): array {
    return $form['board-controlled']['member-controlled'];
  }

  /**
   * {@inheritdoc}
   */
  public function validateForm(array &$form, FormStateInterface $form_state) {}

  /**
   * Create a fresh form.
   */
  public function resetSubmit(array &$form, FormStateInterface $form_state) {
    $form_state->setRedirect('ebms_review.print_packet');
  }

  /**
   * {@inheritdoc}
   */
  public function submitForm(array &$form, FormStateInterface $form_state) {
    $parameters = $form_state->getValues();
    $request = SavedRequest::saveParameters('print job', $parameters);
    $parms = ['request_id' => $request->id()];
    $form_state->setRedirect('ebms_review.print_packet', $parms);
  }

}
