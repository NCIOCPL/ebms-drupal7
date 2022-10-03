<?php

namespace Drupal\ebms_review\Form;

use Drupal\Core\Form\FormBase;
use Drupal\Core\Form\FormStateInterface;
use Drupal\ebms_board\Entity\Board;

/**
 * Choose a packet in which to record reviews on behalf of a board member.
 *
 * @ingroup ebms
 */
class RecordResponses extends FormBase {

  /**
   * {@inheritdoc}
   */
  public function getFormId(): string {
    return 'record_responses';
  }

  /**
   * {@inheritdoc}
   */
  public function buildForm(array $form, FormStateInterface $form_state): array {

    // Set up starting defaults.
    $member_picklist = [];
    $packet_picklist = [];
    $member_empty = 'Please select a board.';
    $packet_empty = 'Please select a board member.';
    $packet_disabled = $member_disabled = TRUE;

    // Set up the member picklist if we have a board selection.
    $board_id = $form_state->getValue('board');
    if (!empty($board_id)) {
      $members = self::getMembers($board_id);
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
    $member_id = $form_state->getValue('member');
    if (!empty($member_id)) {
      $packets = self::getPackets($member_id, $board_id);
      if (!empty($packets)) {
        $packet_picklist = $packets;
        $packet_disabled = FALSE;
        $packet_empty = '- None -';
      }
    }

    // Assemble the render array for the form.
    return [
      '#title' => 'Record Responses',
      'board' => [
        '#type' => 'radios',
        '#title' => 'Board',
        '#description' => 'Select the board for which you want to enter review responses.',
        '#options' => Board::boards(),
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
          '#description' => 'Select the board member on whose behalf you wish to record review responses.',
          '#options' => $member_picklist,
          '#required' => TRUE,
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
            '#description' => "Select a packet to navigate directly to that specific set of articles. Otherwise you will be taken to the board member's Assigned Packets page.",
            '#options' => $packet_picklist,
            '#disabled' => $packet_disabled,
            '#attributes' => ['autocomplete' => 'off'],
            '#empty_option' => $packet_empty,
          ],
        ],
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
   * Find board members with articles to review.
   *
   * Drupal's entity query API won't cut it for this logic.
   *
   * @param int $board_id
   *   ID of the board whose members we're looking for.
   *
   * @return array
   *   Members of the board with unreviewed packets, indexed by user IDs.
   */
  public static function getMembers(int $board_id): array {
    $query = \Drupal::database()->select('ebms_packet', 'packet');
    $query->join('ebms_topic', 'topic', 'topic.id = packet.topic');
    $query->join('ebms_packet__reviewers', 'reviewers', 'reviewers.entity_id = packet.id');
    $query->join('users_field_data', 'user', 'user.uid = reviewers.reviewers_target_id');
    $query->join('ebms_packet__articles', 'articles', 'articles.entity_id = packet.id');
    $query->join('ebms_packet_article', 'article', 'article.id = articles.articles_target_id');
    $query->join('ebms_state', 'state', 'state.article = article.article AND state.topic = topic.id');
    $query->join('taxonomy_term__field_text_id', 'state_text_id', 'state_text_id.entity_id = state.value');
    $query->leftJoin('ebms_packet_article__reviews', 'reviews', 'reviews.entity_id = article.id');
    $query->leftJoin('ebms_review', 'review', 'review.id = reviews.reviews_target_id AND review.reviewer = reviewers.reviewers_target_id');
    $query->condition('packet.active', 1);
    $query->condition('article.dropped', 0);
    $query->condition('topic.board', $board_id);
    $query->condition('state.current', 1);
    $query->condition('state_text_id.field_text_id_value', ['fyi', 'final_board_decision'], 'NOT IN');
    $query->isNull('review.reviewer');
    $query->distinct();
    $query->fields('user', ['uid', 'name']);
    $query->orderBy('user.name');
    $results = $query->execute();
    $members = [];
    foreach ($results as $result) {
      $members[$result->uid] = $result->name;
    }
    return $members;
  }

  /**
   * Find packets with articles to review for a board member.
   *
   * @param int $member_id
   *   ID of the board member's EBMS user account
   * @param int $board_id
   *   ID of the board whose packets we're looking for.
   *
   * @return array
   *   Packets with unreviewed articles, indexed by packet entity IDs.
   */
  public static function getPackets(int $member_id, int $board_id): array {
    $storage = \Drupal::entityTypeManager()->getStorage('ebms_packet');
    $query = $storage->getQuery()->accessCheck(FALSE);
    $query->condition('active', 1);
    $query->condition('reviewers', $member_id);
    $query->condition('articles.entity.dropped', 0);
    $query->condition('topic.entity.board', $board_id);
    $query->addTag('packets_with_unreviewed_articles');
    $query->sort('created', 'DESC');
    $entities = $storage->loadMultiple($query->execute());
    $packets = [];
    foreach ($entities as $packet) {
      $packets[$packet->id()] = $packet->title->value;
    }
    return $packets;
  }

  /**
   * {@inheritdoc}
   */
  public function validateForm(array &$form, FormStateInterface $form_state) {}

  /**
   * Create a fresh form.
   */
  public function resetSubmit(array &$form, FormStateInterface $form_state) {
    $form_state->setRedirect('ebms_review.record_responses');
  }

  /**
   * {@inheritdoc}
   */
  public function submitForm(array &$form, FormStateInterface $form_state) {
    $member_id = $form_state->getValue('member');
    $packet_id = $form_state->getValue('packet');
    $options = ['query' => ['obo' => $member_id]];
    if (!empty($packet_id)) {
      $form_state->setRedirect('ebms_review.assigned_packet', ['packet_id' => $packet_id], $options);
    }
    else {
      $form_state->setRedirect('ebms_review.record_assigned_packets', [], $options);
    }
  }

}
