<?php

namespace Drupal\ebms_report\Controller;

use Drupal\Core\Controller\ControllerBase;
use Drupal\ebms_board\Entity\Board;
use Drupal\ebms_core\Entity\SavedRequest;
use Drupal\ebms_meeting\Entity\Meeting;
use Drupal\ebms_message\Entity\Message;
use Drupal\file\Entity\File;
use Symfony\Component\HttpFoundation\Response;

/**
 * Controller for the Report Module.
 */
class RecentActivityReport extends ControllerBase {

  /**
   * Message types meeting activity included on this report.
   */
  const MEETING_ACTIVITY = [
    Message::AGENDA_PUBLISHED,
    Message::MEETING_CHANGED,
    Message::MEETING_PUBLISHED,
    Message::MEETING_TYPE_CHANGED,
  ];

  public function display($report_id) {

    // Collect the values we'll need for the entity queries.
    $params = SavedRequest::loadParameters($report_id);
    $start = $params['date-start'];
    $end = $params['date-end'];
    $title = "Recent Activity ($start through $end)";
    $end .= ' 23:59:59';
    $board_ids = array_diff($params['boards'], [0]);

    // Initialize the nested arrays for holding the activity information.
    $boards = [];
    foreach ($board_ids as $board_id) {
      $board = Board::load($board_id);
      $boards[$board_id] = [
        'name' => $board->name->value,
        'groups' => [],
      ];
    }

    // Check each possible activity type.
    $types = ['literature', 'document', 'meeting'];
    foreach ($types as $type) {

      // Only process this type if we've been asked to.
      if (!empty($params['types'][$type])) {

        // Prepare the arrays for the events of this type.
        // Note that if we haven't been asked to group the report by activity
        // type, we collect all of the events into a single unnamed group
        // for each board.
        $group = empty($params['options']['group']) ? '' : $type;
        foreach (array_keys($boards) as $board_id) {
          if (!empty($group) || empty($boards[$board_id]['groups'])) {
            $boards[$board_id]['groups'][$group] = [
              'name' => ucfirst($group),
              'events' => [],
            ];
          }
        }

        // Get the reviews posted during the specified time period.
        if ($type === 'literature') {
          $storage = $this->entityTypeManager()->getStorage('ebms_packet');
          $query = $storage->getQuery()->accessCheck(FALSE);
          $query->condition('topic.entity.board', $board_ids, 'IN');
          $query->condition('articles.entity.reviews.entity.posted', [$start, $end], 'BETWEEN');
          foreach ($storage->loadMultiple($query->execute()) as $packet) {
            foreach ($packet->articles as $packet_article) {
              foreach ($packet_article->entity->reviews as $review) {
                $posted = $review->entity->posted->value;
                if ($posted < $start || $posted > $end) {
                  continue;
                }
                $article = $packet_article->entity->article->entity;
                $dispositions = [];
                foreach ($review->entity->dispositions as $disposition) {
                  $dispositions[] = $disposition->entity->name->value;
                }
                $reasons = [];
                foreach ($review->entity->reasons as $reason) {
                  $reasons[] = $reason->entity->name->value;
                }
                $full_text_url = '';
                if (!empty($article->full_text->file)) {
                  $file = File::load($article->full_text->file);
                  $full_text_url = $file->createFileUrl();
                }
                $boards[$packet->topic->entity->board->target_id]['groups'][$group]['events'][] = [
                  'type' => 'literature',
                  'when' => $posted,
                  'user' => $review->entity->reviewer->entity->name->value,
                  'packet_title' => $packet->title->value,
                  'authors' => $article->getAuthors(3) ?: ['[No authors listed]'],
                  'article_title' => $article->title->value,
                  'publication' => $article->getLabel(),
                  'pmid' => $article->source_id->value,
                  'dispositions' => $dispositions,
                  'reasons' => $reasons,
                  'comments' => $review->entity->comments->value,
                  'loe_info' => $review->entity->loe_info->value,
                  'full_text_url' => $full_text_url,
                ];
              }
            }
          }
        }

        // Find the summary documents posted during the specified date range.
        elseif ($type === 'document') {
          $storage = $this->entityTypeManager()->getStorage('ebms_message');
          $query = $storage->getQuery()->accessCheck(FALSE);
          $query->condition('boards', $board_ids, 'IN');
          $query->condition('message_type', Message::SUMMARY_POSTED);
          $query->condition('posted', [$start, $end], 'BETWEEN');
          foreach ($storage->loadMultiple($query->execute()) as $message) {
            $values = $message->getExtraValues();
            $event = [
              'type' => 'document',
              'when' => $message->posted->value,
              'user' => $message->user->entity->name->value,
              'url' => $values->summary_url ?? '',
              'notes' => $values->notes ?? '',
              'title' => $values->title ?? '',
            ];
            foreach ($message->boards as $board) {
              $board_id = $board->target_id;
              if (array_key_exists($board_id, $boards)) {
                $boards[$board_id]['groups'][$group]['events'][] = $event;
              }
            }
          }
        }

        // Find the meeting creation/modification events.
        elseif ($type === 'meeting') {
          $storage = $this->entityTypeManager()->getStorage('ebms_message');
          $query = $storage->getQuery()->accessCheck(FALSE);
          $query->condition('message_type', self::MEETING_ACTIVITY, 'IN');
          $query->condition('posted', [$start, $end], 'BETWEEN');
          $or_group = $query->orConditionGroup()
            ->condition('boards', $board_ids, 'IN')
            ->condition('groups.entity.boards', $board_ids, 'IN');
          $query->condition($or_group);
          foreach ($storage->loadMultiple($query->execute()) as $message) {
            $values = $message->getExtraValues();
            $meeting = Meeting::load($values->meeting_id);
            $message_type = $message->message_type->value;
            $what = 'added to calendar';
            if ($message_type === Message::AGENDA_PUBLISHED) {
              $what = 'agenda published';
            }
            elseif ($message_type === Message::MEETING_CHANGED) {
              $what = 'date/time changed';
            }
            elseif ($message_type === Message::MEETING_TYPE_CHANGED) {
              $what = 'type changed';
            }
            $event = [
              'type' => 'meeting',
              'when' => $message->posted->value,
              'user' => $message->user->entity->name->value,
              'what' => $what,
              'meeting_name' => $meeting->name->value,
              'meeting_date' => substr($meeting->dates->value, 0, 10),
              'meeting_type' => $meeting->type->entity->name->value,
            ];
            $meeting_boards = [];
            foreach ($message->boards as $message_board) {
              $board_id = $message_board->target_id;
              if (array_key_exists($board_id, $boards)) {
                $meeting_boards[$board_id] = $board_id;
              }
            }
            foreach ($message->groups as $message_group) {
              foreach ($message_group->entity->boards as $board) {
                $board_id = $board->target_id;
                if (array_key_exists($board_id, $boards)) {
                  $meeting_boards[$board_id] = $board_id;
                }
              }
            }
            foreach ($meeting_boards as $board_id) {
              $boards[$board_id]['groups'][$group]['events'][] = $event;
            }
          }
        }
      }
    }

    // Sort the events with the most recent at the top in each group.
    foreach ($boards as &$board) {
      foreach ($board['groups'] as &$group) {
        usort($group['events'], function(&$a, &$b) {
          return $b['when'] <=> $a['when'];
        });
      }
    }

    // Render and return the page.
    $render_array = [
      '#cache' => ['max-age' => 0],
      '#theme' => 'recent_activity_report',
      '#title' => $title,
      '#boards' => $boards,
    ];
    $page = \Drupal::service('renderer')->render($render_array);
    $response = new Response($page);
    return $response;
  }

}
