<?php

/**
 *
 * Find EBMS Pubmed records which have been dropped by NLM, and report them.
 *
 * JIRA::OCEEBMS-270
 */

/**
 * Record a fatal error and bail.
 */
function fail($why) {
    log_write($why);
    exit(1);
}

/**
 * Record what's happening.
 */
function log_write($what) {
    $path = '/home/drupal/logs/pubmed-drops.log';
    $now = date('c');
    @file_put_contents($path, "$now $what\n", FILE_APPEND);
    echo "$what\n";
}

/*
 * Email the report to the usual suspects.
 */
function send_report($checked, $missing, $elapsed) {
    $lines = array(
        "Checked $checked",
        '',
        count($missing) . ' articles missing from NLM',
        '======================',
    );
    foreach ($missing as $m)
        $lines[] = "$m missing";
    $lines[] = '';
    $lines[] = "Processing time: $elapsed seconds";
    $message = implode("\r\n", $lines) . "\r\n";
    $default_recips = variable_get('dev_notif_addr');
    $to = variable_get('pubmed_missing_article_report_recips', $default_recips);
    $headers = "From: ebms@nci.nih.gov\r\nTo: $to\r\n";
    $subject = 'PMIDs missing from NLM (' . php_uname('n') . ')';
    mail($to, $subject, $message, $headers);
}

/**
 * Top-level processing logic.
 *
 *  1. Parse the command-line arguments.
 *  2. Find out which PMIDs NLM has lost.
 *  3. Send out the report.
 */
function main() {
    module_load_include('inc', 'ebms', 'reports');
    // 1. Parse the command-line arguments.
    $start = time();
    $opts = getopt('', array('all', 'batch-size:'));
    $active_only = !isset($opts['all']);

    // 2. Find out which PMIDs NLM has lost.
    if (empty($opts['batch-size']))
        $report = EbmsReports::lost_by_nlm($active_only);
    else
        $report = EbmsReports::lost_by_nlm($active_only, $opts['batch-size']);

    // 3. Send out the report.
    if ($active_only)
        $checked = $report->checked . ' active Pubmed IDs';
    else
        $checked = 'all ' . $report->checked . ' Pubmed IDs';
    log_write("Checked $checking");
    log_write(count($report->missing) . ' articles dropped');
    foreach ($missing as $m)
        log_write("$m dropped");
    $elapsed = time() - $start;
    log_write("processing time: $elapsed seconds");
    send_report($checked, $report->missing, $elapsed);
}

main();
?>
