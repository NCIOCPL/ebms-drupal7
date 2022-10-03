<?php

/**
 * Tag articles still unreviewed after two years.
 *
 * See https://tracker.nci.nih.gov/browse/OCEEBMS-426.
 */

use Drupal\ebms_review\Controller\UnreviewedArticles;

/**
 * Email the report to the usual suspects.
 *
 * @param string|array $information
 *   Report lines array or error message string.
 */
function send_report(string|array $information) {

  @ebms_debug_log('Starting Unreviewed Articles report', 1);
  $to = \Drupal::config('ebms_core.settings')->get('dev_notif_addr');
  if (empty($to)) {
    \Drupal::logger('ebms_review')->error('No recipients for unreviewed articles report.');
    @ebms_debug_log('Aborting Unreviewed Articles report: no recipients registered.', 1);
    return;
  }
  $start = microtime(TRUE);
  $server = php_uname('n');
  $subject = "Unreviewed EBMS Articles ($server)";
  if (is_array($information)) {
    $items = $information;
    $list = [
      '#theme' => 'item_list',
      '#title' => 'Tagged Articles',
      '#items' => $items,
      '#empty' => 'No unreviewed articles needing to be tagged were found.',
    ];
    $message = \Drupal::service('renderer')->renderPlain($list);
  }
  else {
    $message = "<p style=\"color: red; font-size: 1rem; font-weight: bold\">$information</p>";
    $subject .= ' [FAILURE]';
  }
  $elapsed = microtime(TRUE) - $start;
  $message .= "\n";
  $message .= '<p style="color: green; font-size: .8rem; font-style: italic;">Processing time: ';
  $message .= $elapsed;
  $message .= ' seconds.</p>';

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
    \Drupal::logger('ebms_review')->error('Unable to send Unreviewed Articles report.');
    @ebms_debug_log('Failure sending report.', 1);
  }
  @ebms_debug_log('Finished Unreviewed Articles report', 1);
}

/**
 * Top-level processing logic.
 *
 *  1. Apply the tags.
 *  2. Send out the report if we have recipients.
 */
function main() {

  try {
    $lines = UnreviewedArticles::applyTags();
    send_report($lines);
  }
  catch (\Exception $e) {
    send_report("failure: $e");
  }
}

main();
?>
