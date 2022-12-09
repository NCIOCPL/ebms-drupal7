<?php

namespace Drupal\ebms_meeting\Controller;

use Drupal\Core\Controller\ControllerBase;
use Drupal\Core\Url;
use Drupal\ebms_meeting\Entity\Meeting;
use Drupal\user\Entity\User;

/**
 * Custom calendar display.
 *
 * The original version of the EBMS used the contributed modules for handling
 * calendar UI. This turned out to be a serious mistake, as the sole maintainer
 * for those modules walked away without any announcement, and bugs piled up
 * and were ignored for years. It turned out to be much easier to implement
 * our own calendar display than the work we incurred in wrestling with those
 * bugs.
 */
class Month extends ControllerBase {

  /**
   * Assemble the render array for a calendar month.
   *
   * @param string $month
   *   Month to be displayed in the form YYYY-MM (defaults to current month).
   *
   * @return array
   *   Render array for the calendar page.
   */
  public function display($month = ''): array {

    // Determine which month we're displaying.
    $today = date('Y-m-d');
    if (empty($month)) {
      $month = date('n');
      $year = date('Y');
    }
    else {
      list($year, $month) = explode('-', $month);
    }
    $this_month = sprintf('%04d-%02d-01', $year, $month);
    $date = new \DateTime($this_month);

    // Find the meetings scheduled for that month.
    $user = User::load($this->currentUser()->id());
    if ($month == 12) {
      $next_month = sprintf('%04d-01-01', $year + 1);
    }
    else {
      $next_month = sprintf('%04d-%02d-01', $year, $month + 1);
    }
    if ($month == 1) {
      $prev_month = sprintf('%04d-12-01', $year - 1);
    }
    else {
      $prev_month = sprintf('%04d-%02d-01', $year, $month - 1);
    }
    $storage = $this->entityTypeManager()->getStorage('ebms_meeting');
    $query = $storage->getQuery()->accessCheck(FALSE);
    $query->condition('dates', $this_month, '>=');
    $query->condition('dates', $next_month, '<');
    $query->sort('dates');
    Meeting::applyMeetingFilters($query, $user);
    $meetings = [];
    foreach ($storage->loadMultiple($query->execute()) as $meeting) {
      if (preg_match('/\d{4}-\d\d-(\d\d)/', $meeting->dates->value, $matches)) {
        $meetings[intval($matches[1])][] = $meeting;
      }
    }

    // Assemble the rows for the month's weeks.
    $options = ['query' => \Drupal::request()->query->all()];
    $dow = $date->format('w');
    $title = $date->format('F Y');
    $n = $date->format('t');
    $rows = [];
    $row = [];
    for ($day = 1; $day <= $n; $day++) {
      if ($day === 1) {
        for ($i = 0; $i < $dow; $i++) {
          $row[] = NULL;
        }
      }
      else if (count($row) === 7) {
        $rows[] = $row;
        $row = [];
      }
      $date = sprintf('%04d-%02d-%02d', $year, $month, $day);
      $day_meetings = [];
      if (array_key_exists($day, $meetings)) {
        foreach ($meetings[$day] as $meeting) {
          $meeting_start = new \DateTime($meeting->dates->value);
          $hour = $meeting_start->format('g');
          $am_pm = $meeting_start->format('a');
          $minutes = $meeting_start->format('i');
          $start = $hour;
          if ($minutes !== '00') {
            $start .= ":$minutes";
          }
          $start .= "$am_pm Eastern Time";
          $url = Url::fromRoute('ebms_meeting.meeting', ['meeting' => $meeting->id()], $options);
          // @todo Think about adding color attribute to meeting.
          $day_meetings[] = [
            'name' => $meeting->name->value,
            'start' => $start,
            'url' => $url,
            'canceled' => $meeting->status->entity->name->value === Meeting::CANCELED,
            'color' => $this->getMeetingColor($meeting),
          ];
        }
      }
      $row[] = [
        'day' => $day,
        'today' => $date === $today,
        'meetings' => $day_meetings,
      ];
    }
    while (count($row) < 7) {
      $row[] = NULL;
    }
    $rows[] = $row;

    // Create navigation rows for the previous and next months.
    $route = 'ebms_meeting.calendar_month';
    $options = ['query' => \Drupal::request()->query->all()];
    $buttons = [
      [
        'url' => Url::fromRoute($route, ['month' => substr($prev_month, 0, 7)], $options),
        'label' => 'Previous',
      ],
      [
        'url' => Url::fromRoute($route, ['month' => substr($next_month, 0, 7)], $options),
        'label' => 'Next',
      ],
    ];

    // Some users get some additional buttons.
    if ($user->hasPermission('manage meetings')) {
      $options['query']['month'] = substr($this_month, 0, 7);
      $buttons[] = [
        'url' => Url::fromRoute('ebms_meeting.add_meeting', [], $options),
        'label' => 'Create Meeting',
      ];
    }
    if ($user->hasPermission('view all meetings')) {
      if (!$user->boards->isEmpty()) {
        $label = 'Show All Boards';
        $options = ['query' => \Drupal::request()->query->all()];
        if ($options['query']['boards'] === 'all') {
          unset($options['query']['boards']);
          $label = 'Restrict To My Boards';
        }
        else {
          $options['query']['boards'] = 'all';
        }
        $buttons[] = [
          'url' => Url::fromRoute($route, ['month' => substr($this_month, 0, 7)], $options),
          'label' => $label,
        ];
      }
      $travel_manager = $user->id() > 1 && $user->hasPermission('manage travel');
      $default = $travel_manager ? 'board' : 'all';
      $label = 'Show All Meeting Categories';
      $options = ['query' => \Drupal::request()->query->all()];
      $meetings = $options['query']['meetings'] ?? $default;
      if ($meetings === 'all') {
        $label = 'Show Just Board Meetings';
        $meetings = 'board';
      }
      else {
        $meetings = 'all';
      }
      if ($meetings === $default) {
        unset($options['query']['meetings']);
      }
      else {
        $options['query']['meetings'] = $meetings;
      }
      $buttons[] = [
        'url' => Url::fromRoute($route, ['month' => substr($this_month, 0, 7)], $options),
        'label' => $label,
      ];
    }

    // Assemble and return the render array.
    return [
      '#title' => $title,
      'buttons' => [
        '#theme' => 'ebms_buttons',
        '#buttons' => $buttons,
      ],
      'calendar' => [
        '#theme' => 'month',
        '#attached' => ['library' => ['ebms_meeting/calendar-month']],
        '#rows' => $rows,
      ],
      'upcoming' => Meeting::upcomingMeetings($user),
    ];
  }

  /**
   * Find out which background color this meeting should use.
   *
   * @todo Talk to the users about whether they want different colors.
   *
   * @param Meeting $meeting
   *   Meeting to be displayed on the calendar.
   *
   * @return string
   *   Possibly empty CSS color string.
   */
  private function getMeetingColor(Meeting $meeting): string {
    $colors = [
      1 => 'red',
      2 => 'navy',
      3 => 'sienna',
      4 => 'darkgreen',
      5 => 'darkmagenta',
      6 => 'darkorange',
    ];
    if (!empty($meeting->color->value)) {
      return $meeting->color->value;
    }
    if ($meeting->boards->count() === 1) {
      return $colors[$meeting->boards[0]->target_id] ?? '';
    }
    $boards = [];
    foreach ($meeting->groups as $group) {
      foreach ($group->entity->boards as $board) {
        $boards[$board->target_id] = $board->target_id;
      }
    }
    if (count($boards) === 1) {
      return $colors[reset($boards)] ?? '';
    }
    return '';
  }

}
