<?php

namespace Drupal\ebms_home\Controller;

use Drupal\Core\Controller\ControllerBase;
use Drupal\Core\Url;
use Drupal\ebms_message\Entity\Message;
use Drupal\ebms_meeting\Entity\Meeting;
use Drupal\ebms_review\Entity\Packet;
use Drupal\user\Entity\User;

/**
 * Create a customized page for each user rôle.
 */
class HomePage extends ControllerBase {

  /**
   * Images for the home page.
   *
   * Shown for users which aren't shown alert/activity cards on the home page.
   */
  const LIBRARY_IMAGES = [
    'library-8.jpg',
    'library-40.jpg',
    'library-5.jpg',
    'library-16.jpg',
    'library-41.jpg',
    'library-45.jpg',
    'library-46.jpg',
  ];

  /**
   * Assemble the cards for the home page.
   */
  public function display(): array {

    // If the user doesn't see activity cards on the home page, show image.
    $user = User::load($this->currentUser()->id());
    if (!$user->hasPermission('view alerts')) {
      return [
        '#theme' => 'image',
        '#attributes' => [
          'src' => '/themes/custom/ebms/images/' . self::LIBRARY_IMAGES[random_int(0, count(self::LIBRARY_IMAGES) - 1)],
          'class' => ['margin-top-5'],
        ],
        '#cache' => ['max-age' => 0],
      ];
    }

    // The first card contains any available alerts.
    $cards = [$this->makeAlertCard($user)];

    // Add the activity cards.
    foreach (Message::GROUPS as $name => list($_message_types, $days)) {
      $query = Message::createQuery($name);
      $query->range(0, 6);
      $ids = $query->execute();
      ebms_debug_log('HomePage::display(): ' . $name . ' found ' . count($ids) . ' messages.');
      $url = '';
      if (count($ids) > 5) {
        $url = Url::fromRoute('ebms_message.recent_activity', ['group' => $name]);
        $ids = array_slice($ids, 0, 5);
      }
      $messages = [];
      foreach ($ids as $id) {
        $message = Message::load($id);
        $messages[] = [
          '#theme' => 'ebms_message',
          '#message' => $message,
        ];
      }
      $capped_name = ucfirst($name);
      $cards[] = [
        'title' => "$capped_name Activity",
        'img' => "$name-activity.jpg",
        'items' => $messages,
        'url' => $url,
        'empty' => "There is no $name activity to display from the past $days days.",
      ];
    }
    return [
      '#title' => '',
      '#attached' => ['library' => ['ebms_home/home']],
      'messages' => [
        '#theme' => 'activity_cards',
        '#cache' => ['max-age' => 0],
        '#cards' => $cards,
      ],
    ];
  }

  /**
   * Assemble the render array for the alert card.
   *
   * @param User $user
   *   User for whom we are creating the home page.
   *
   * @return array
   *   Render array for the card.
   */
  private function makeAlertCard(User $user): array {
    $boards = [];
    foreach ($user->boards as $board) {
      $boards[] = $board->target_id;
    }
    $items = [$this->nextMeeting($user, $boards)];
    if ($user->hasPermission('view review alerts')) {
      $items[] = $this->reviewAlert($user);
    }
    if ($user->hasPermission('view hotel request alerts') && !empty($boards)) {
      $items[] = $this->hotelRequestsAlert($user, $boards);
    }
    if ($user->hasPermission('view posted summaries alerts') && !empty($boards)) {
      $items[] = $this->postedSummariesAlert($boards);
    }
    return [
      'title' => 'Alerts',
      'img' => 'alert.jpg',
      'items' => $items,
    ];
  }

  /**
   * Assemble the render array for the alert card.
   *
   * @param User $user
   *   User for whom we are creating the home page.
   * @param array $board_ids
   *   IDs of boards to which this user belongs.
   *
   * @return array
   *   Render array for the alert.
   */
  private function nextMeeting(User $user, array $board_ids): array {
    $storage = \Drupal::entityTypeManager()->getStorage('ebms_meeting');
    $query = $storage->getQuery()->accessCheck(FALSE);
    $query->condition('dates.end_value', date('Y-m-d H:i:s'), '>=');
    $query->range(0, 1);
    $query->sort('dates');
    if (!empty($board_ids)) {
      Meeting::applyMeetingFilters($query, $user);
    }
    $ids = $query->execute();
    if (empty($ids)) {
      return ['#theme' => 'home_page_next_meeting'];
    }
    $meeting = Meeting::load(reset($ids));
    $when = new \DateTime($meeting->dates->value);
    $hour = $when->format('g');
    $minutes = $when->format('i');
    $am_pm = $when->format('a');
    $minutes = $minutes === '00' ? '' : ":$minutes";
    $date = $when->format('Y-m-d');
    $when = "$hour$minutes$am_pm, $date";
    $parameters = ['meeting' => $meeting->id()];
    return [
      '#theme' => 'home_page_next_meeting',
      '#meeting' => [
        'url' => Url::fromRoute('ebms_meeting.meeting', $parameters),
        'name' => $meeting->name->value,
        'when' => $when,
        'type' => $meeting->type->entity->name->value,
        'agenda_posted' => !empty($meeting->agenda->value) && !empty($meeting->agenda_published->value),
      ],
    ];
  }

  /**
   * Assemble the render array for the review alert.
   *
   * For board members, we show them the number of articles which have been
   * assigned to them for review. For board managers, we show the number of
   * reviews which have been posted to their packets since they last looked
   * at each packet.
   *
   * @param User $user
   *   User for whom we are creating the home page.
   *
   * @return array
   *   Render array for the alert.
   */
  private function reviewAlert(User $user): array {
    if ($user->hasPermission('review literature') && !$user->hasPermission('manage review packets')) {
      return [
        '#theme' => 'home_page_assigned_for_review',
        '#count' => Packet::getReviewerArticleCount($user->id()),
        '#url' => Url::fromRoute('ebms_review.assigned_packets'),
      ];
    }
    $storage = \Drupal::entityTypeManager()->getStorage('ebms_review');
    $query = $storage->getQuery()->accessCheck(FALSE);
    $query->addTag('unseen_reviews');
    $query->addMetaData('uid', $user->id());
    return [
      '#theme' => 'home_page_unseen_reviews',
      '#count' => $query->count()->execute(),
      '#url' => Url::fromRoute('ebms_review.reviewed_packets'),
    ];
  }

  /**
   * Assemble the render array for the hotel requests alert.
   *
   * Tell the board manager how many hotel requests have been submitted by
   * the members of her board(s) in the past 60 days.
   *
   * @param User $user
   *   User for whom we are creating the home page.
   * @param array $board_ids
   *   IDs of boards to which this user belongs.
   *
   * @return array
   *   Render array for the alert.
   */
  private function hotelRequestsAlert(User $user, array $board_ids) {
    $cutoff = new \DateTime();
    $cutoff->sub(new \DateInterval('P60D'));
    $storage = \Drupal::entityTypeManager()->getStorage('ebms_hotel_request');
    $query = $storage->getQuery()->accessCheck(FALSE);
    $query->condition('user.entity.boards', $board_ids, 'IN');
    $query->condition('submitted', $cutoff->format('Y-m-d'), '>=');
    return [
      '#theme' => 'home_page_hotel_requests',
      '#count' => $query->count()->execute(),
      '#url' => Url::fromRoute('ebms_report.hotel_requests'),
    ];
  }

  /**
   * Assemble the render array for the posted summaries alert.
   *
   * This shows the board managers how many updated summaries have been posted
   * by their board members in the past 30 days.
   *
   * @param array $board_ids
   *   IDs of boards to which this user belongs.
   *
   * @return array
   *   Render array for the alert.
   */
  private function postedSummariesAlert(array $board_ids): array {
    $storage = \Drupal::entityTypeManager()->getStorage('ebms_doc');
    $query = $storage->getQuery()->accessCheck(FALSE);
    $query->addTag('posted_summaries');
    $query->addMetaData('boards', $board_ids);
    if (count($board_ids) === 1) {
      $url = Url::fromRoute('ebms_summary.board', ['board_id' => $board_ids[0]]);
    }
    else {
      $url = Url::fromRoute('ebms_summary.board');
    }
    return [
      '#theme' => 'home_page_posted_summaries',
      '#count' => $query->count()->execute(),
      '#url' => $url,
      '#board_count' => count($board_ids),
    ];
  }

}
