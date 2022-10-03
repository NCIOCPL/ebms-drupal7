<?php

namespace Drupal\ebms_report\Controller;

use Drupal\Core\Controller\ControllerBase;
use Drupal\Core\Url;
use Drupal\user\Entity\User;

/**
 * Controller for the Report Module.
 */
class ReportRequestController extends ControllerBase {

  /**
   * Information about available reports.
   */
  const REPORTS = [
    'board' => [
      [
        'name' => 'Documents',
        'route' => 'ebms_report.documents',
        'description' => 'Used to locate, edit, or archive documents in the system. Multiple documents can be archived at one time using this report.',
        'permissions' => ['view all reports', 'view librarian reports'],
      ],
      [
        'name' => 'Meeting Dates',
        'route' => 'ebms_report.meeting_dates',
        'description' => 'This report is planned for a future release.',
        'permissions' => ['view all reports', 'view librarian reports'],
      ],
      [
        'name' => 'Hotel Requests',
        'route' => 'ebms_report.hotel_requests',
        'description' => 'Used to see which Board members have submitted a request for a hotel. The report is automatically generated and displayed when the page is opened.',
        'permissions' => ['view all reports', 'view travel reports'],
      ],
      [
        'name' => 'Reimbursement Requests',
        'route' => 'ebms_report.reimbursement_requests',
        'description' => 'Used to see which Board members have submitted a request for reimbursement of meeting expenses. The report is automatically generated and displayed when the page is opened.',
        'permissions' => ['view all reports', 'view travel reports'],
      ],
      [
        'name' => 'Board Membership',
        'route' => 'ebms_report.board_members',
        'description' => 'Used to generate a list of active Board members on an Editorial Board and/or in subgroups.',
        'permissions' => ['view all reports', 'view travel reports'],
      ],
      [
        'name' => 'Board Member Logins',
        'route' => 'ebms_report.board_member_logins',
        'description' => 'Used to determine when Board members last logged into the EBMS. Clicking on the report name from the report menu will generate an Excel report with this information.',
        'permissions' => ['view all reports'],
      ],
      [
        'name' => 'Recent Activity',
        'route' => 'ebms_report.recent_activity',
        'description' => 'Used to summarize recent Board-related activity in the EBMS. It may be useful if you are concerned you missed something or if a Board member has had a period of inactivity and wants to catch up (the Board manager would need to copy and paste the report results to send to the Board member).',
        'permissions' => ['view all reports'],
      ],
    ],
    'article' => [
      [
        'name' => 'Import',
        'route' => 'ebms_report.import',
        'description' => 'Used to display the details of a specific import batch (a set of one or more articles imported to the EBMS for a single topic at a single time). This report is primarily used by the Medical Librarians.',
        'permissions' => ['view all reports', 'view librarian reports'],
      ],
      [
        'name' => 'Article Statistics',
        'route' => 'ebms_report.articles',
        'description' => 'Collection of reports used to generate article statistics, gather more information about a particular reviewing cycle or import session, and identify and troubleshoot problems. There are eight reports in this set, which are primarily used by the Medical Librarians. These reports are more historical than the "Articles By Status" reports below, which are driven by which states are current at the time the report is requested.',
        'permissions' => ['view all reports', 'view librarian reports'],
      ],
      [
        'name' => 'Articles by Status',
        'route' => 'ebms_report.articles_by_status',
        'description' => 'Used to find a single article or a group of articles that is/are at a particular point in the review process. It can be helpful when looking for agenda items or checking to see if final dispositions have been chosen for papers from a specific meeting.',
        'permissions' => ['view all reports'],
      ],
      [
        'name' => 'Articles by Tag',
        'route' => 'ebms_report.articles_by_tag',
        'description' => 'Used to keep track of high priority articles and core journal articles, for example.',
        'permissions' => ['view all reports', 'view librarian reports'],
      ],
      [
        'name' => 'Literature Reviews',
        'route' => 'ebms_report.literature_reviews',
        'description' => 'Used to find articles that have been included in a literature packet and to view several Board member responses at one time.',
        'permissions' => ['view all reports'],
      ],
      [
        'name' => 'Responses by Reviewer',
        'route' => 'ebms_report.responses_by_reviewer',
        'description' => 'Used to assess literature surveillance responses (completed reviews) for each Board member. This report may be worth sharing with the Editors in Chief periodically.',
        'permissions' => ['view all reports'],
      ],
      [
        'name' => 'Articles without Responses',
        'route' => 'ebms_report.articles_without_responses',
        'description' => 'Used to identify which articles were assigned for review but haven\'t received any further attention. The results of this report are often used to help plan meeting agendas and prevent important papers from "falling through the cracks." A Board member version of this report is also available as the results are often shared with Board members. This report may be worth sharing with the Editors in Chief periodically.',
        'permissions' => ['view all reports'],
      ],
      [
        'name' => 'Topic Reviewers',
        'route' => 'ebms_report.topic_reviewers',
        'description' => 'Shows which Board members are assigned to which topics. A print-friendly version of this report is available.',
        'permissions' => ['view all reports'],
      ],
      [
        'name' => 'Invalid PubMed IDs',
        'route' => 'ebms_report.abandoned_articles',
        'description' => 'Used to identify articles that have PubMed IDs that are no longer in PubMed. (Occasionally, the PubMed ID for an article can change when an error such as a duplicate record is caught at NLM.) Bonnie, Robin J, and Bob currently receive this report via e-mail weekly (on Sundays), but it can also be run at any time from the report menus.',
        'permissions' => ['view all reports'],
      ],
      [
        'name' => 'Statistics',
        'route' => 'ebms_report.statistics',
        'description' => 'Excel workbook with one sheet for each Board, providing counts for the number of articles reaching each processing state for the reporting period (which by default is the previous twelve months).',
        'permissions' => ['view all reports'],
      ],
    ],
  ];

  /**
   * Assemble the render array for the available reports.
   *
   * @return array
   *   Render array for the page.
   */
  public function listReports(): array {
    $page = [
      '#attached' => ['library' => ['ebms_report/landing-page']],
      '#title' => 'Reports',
    ];
    $user = User::load($this->currentUser()->id());
    foreach (self::REPORTS as $section => $reports) {
      $available = [];
      foreach ($reports as $report) {
        foreach ($report['permissions'] as $permission) {
          if ($user->hasPermission($permission)) {
            $available[] = $report;
            break;
          }
        }
      }
      if (!empty($available)) {
        sort($available);
        $key = "$section-reports";
        $page[$section] = [
          '#type' => 'details',
          '#title' => ucfirst($section) . ' Management',
          $key => [
            '#theme' => 'report_links',
            '#reports' => [],
          ],
        ];
        foreach ($available as $report) {
          $page[$section][$key]['#reports'][] = [
            'name' => $report['name'],
            'url' => Url::fromRoute($report['route']),
            'description' => $report['description'],
          ];
        }
      }
    }
    return $page;
  }

}
