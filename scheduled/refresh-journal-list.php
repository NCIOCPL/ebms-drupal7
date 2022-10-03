<?php

/**
 * Get fresh information about journals at NLM.
 *
 * See https://tracker.nci.nih.gov/browse/OCEEBMS-408.
 */

use Drupal\ebms_journal\Entity\Journal;

/**
 * Email the report to the usual suspects.
 *
 * @param array $report
 *   Information about the refresh results.
 */
function send_report(array $report) {

  @ebms_debug_log('Starting Journal Refresh report', 1);
  $to = \Drupal::config('ebms_core.settings')->get('dev_notif_addr');
  if (empty($to)) {
    \Drupal::logger('ebms_review')->error('No recipients for journal refresh report.');
    @ebms_debug_log('Aborting Journal Refresh report: no recipients registered.', 1);
    return;
  }
  $server = php_uname('n');
  $subject = "EBMS Journal Refresh ($server)";
  $columns = ['Fetched', 'Checked', 'Updated', 'Added'];
  $row = [];
  foreach ($columns as $column) {
    $row[] = $report[strtolower($column)];
  }
  $render_array = [
    '#theme' => 'journal_refresh_cron_report',
    '#report' => $report,
  ];
  $message = \Drupal::service('renderer')->renderPlain($render_array);
  if (!empty($report['error'])) {
    $subject .= ' [FAILURE]';
  }

  // Send the report.
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
    \Drupal::logger('ebms_review')->error('Unable to send Journal Refresh report.');
    @ebms_debug_log('Failure sending report.', 1);
  }
  @ebms_debug_log('Finished Journal Refresh report', 1);
}

/**
 * Top-level processing logic.
 *
 *  1. Refresh the journals.
 *  2. Send out the report if we have recipients.
 */
function main() {
  $report = Journal::refresh();
  send_report($report);
}

main();
?>
