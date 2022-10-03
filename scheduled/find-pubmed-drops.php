<?php

/**
 * Find EBMS Pubmed records which have been dropped by NLM, and report them.
 *
 * See https://tracker.nci.nih.gov/browse/OCEEBMS-270.
 */

use Drupal\ebms_report\Form\AbandonedArticlesReport;

/**
 * Email the report to the usual suspects.
 */
function send_report(string $recips) {

  // Fetch the information from NLM.
  ebms_debug_log('Starting Dropped PubMed Articles report', 1);
  $start = microtime(TRUE);
  $report = AbandonedArticlesReport::report();

  // Assemble a rich-text message body.
  $checked = 'Checked ' . $report['checked'] . ' Active Articles';
  $missing_count = count($report['missing']);
  $missing_s = $missing_count === 1 ? '' : 's';
  $items = [];
  foreach ($report['missing'] as $id => $pmid) {
    $items[] = "$pmid (EBMS ID $id)";
  }
  $list = [
    '#theme' => 'item_list',
    '#title' => "$missing_count Invalid PubMed ID$missing_s ($checked)",
    '#items' => $items,
    '#empty' => 'No invalid PubMed IDs were found.',
  ];
  $elapsed = microtime(TRUE) - $start;
  $message = \Drupal::service('renderer')->renderPlain($list);
  $message .= "\n";
  $message .= '<p style="color: green; font-size: .8rem; font-style: italic;">Processing time: ';
  $message .= $elapsed;
  $message .= ' seconds.</p>';

  // Send the report.
  $site_mail = \Drupal::config('system.site')->get('mail');
  $site_name = \Drupal::config('system.site')->get('name');
  $from = "$site_name <$site_mail>";
  $server = php_uname('n');
  $subject = "PMIDs missing from NLM ($server)";
  $headers = implode("\r\n", [
    'MIME-Version: 1.0',
    'Content-type: text/html; charset=utf-8',
    "From: $from",
  ]);
  $rc = mail($recips, $subject, $message, $headers);
  if (empty($rc)) {
    \Drupal::logger('ebms_report')->error('Unable to send Dropped PubMed Articles report.');
    ebms_debug_log('Failure sending report', 1);
  }
  ebms_debug_log('Finished Dropped PubMed Articles report', 1);
}

/**
 * Top-level processing logic.
 *
 *  1. Verify that we have recipients.
 *  2. Send out the report.
 */
function main() {

  // Make sure we have at least one receipient.
  $to = \Drupal::config('ebms_core.settings')->get('pubmed_missing_article_report_recips');
  if (empty($to)) {
    $to = \Drupal::config('ebms_core.settings')->get('dev_notif_addr');
  }
  if (empty($to)) {
    \Drupal::logger('ebms_report')->error('No recipients for dropped articles report.');
  }
  else {
    send_report($to);
  }
}

main();
?>
