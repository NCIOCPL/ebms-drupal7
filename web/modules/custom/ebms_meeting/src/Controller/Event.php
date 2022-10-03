<?php

namespace Drupal\ebms_meeting\Controller;

use Drupal\Core\Controller\ControllerBase;
use Drupal\Core\Url;
use Symfony\Component\HttpFoundation\Response;
use Drupal\ebms_meeting\Entity\Meeting;

/**
 * Create a calendar event for the meeting.
 */
class Event extends ControllerBase {

  /**
   * Let the user add the meeting to her personal calendar.
   *
   * @param \Drupal\ebms_meeting\Entity\Meeting $meeting
   *   Entity for the meeting to be added to a calendar.
   *
   * @return Response
   *   A custom non-HTML response.
   */
  public function send($meeting): Response {
    $meeting_id = $meeting->id();
    $status = 'CONFIRMED';
    $category = 'MEETING';
    if ($meeting->status->entity->name->value === Meeting::CANCELED) {
      $status = 'CANCELED';
    }
    $route = 'ebms_meeting.meeting';
    $parms = ['meeting' => $meeting_id];
    $options = ['absolute' => TRUE];
    $url = Url::fromRoute($route, $parms, $options)->toString();
    if (!empty($meeting->category->entity->name->value)) {
      $category = strtoupper($meeting->category->entity->name->value) . ' MEETING';
    }
    $location = $meeting->type->entity->name->value;
    if ($location === 'In Person') {
      $location = '9609 Medical Center Drive, Rockville MD 20850';
    }
    $start = new \DateTime($meeting->dates->value);
    $end = new \DateTime($meeting->dates->end_value);
    $start = $start->format('Ymd\THis');
    $end = $end->format('Ymd\THis');
    $properties = [
      'BEGIN:VCALENDAR',
      'VERSION:2.0',
      'PRODID:-//National Cancer Institute//NONSGML EBMS Meeting//EN',
      'BEGIN:VEVENT',
      "UID:$url",
      "URL:$url",
      "SUMMARY:{$meeting->name->value}",
      "LOCATION:$location",
      "DTSTART;TZID=US-Eastern:$start",
      "DTEND;TZID=US-Eastern:$end",
      "STATUS:$status",
      "CATEGORIES:$category",
      'END:VEVENT',
      'END:VCALENDAR',
    ];
    $event = implode("\r\n", $properties) . "\r\n";
    $filename = "nci-meeting-$meeting_id-$start.ics";
    $response = new Response($event);
    $response->headers->set('Content-type', 'text/calendar');
    $response->headers->set('Content-disposition', 'inline;filename="' . $filename . '"');
    return $response;
  }

}
