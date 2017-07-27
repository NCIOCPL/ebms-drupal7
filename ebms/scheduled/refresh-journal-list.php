<?php

/**
 * Get fresh information about journals at NLM.
 *
 * JIRA::OCEEBMS-408
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
    $path = '/home/drupal/logs/refresh-journals.log';
    $now = date('c');
    @file_put_contents($path, "$now $what\n", FILE_APPEND);
    echo "$what\n";
}

/**
 * Email the report to the usual suspects.
 */
function send_report($report) {
    $to = variable_get('dev_notif_addr');
    if (empty($to))
        fail("No recipient addresses for developer notifications");
    $headers = "From: ebms@nci.nih.gov\r\nTo: $to\r\n";
    $subject = 'EBMS journals refreshed (' . php_uname('n') . ')';
    mail($to, $subject, $report, $headers);
}

/**
 * Refresh the journals and report the statistics.
 */
function main() {
    $start = time();
    $result = Ebms\Journal::refresh();
    $report = array();
    foreach ($result as $action => $count) {
        $line = "$count journals $action";
        log_write($line);
        $report[] = $line;
    }
    send_report(implode("\n", $report));
    $elapsed = time() - $start;
    log_write("processing time: $elapsed seconds");
}

main();
?>
