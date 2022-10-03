<?php

/**
 * Refresh EBMS articles from NLM's PubMed XML.
 *
 * See https://tracker.nci.nih.gov/browse/OCEEBMS-87.
 */

use Drupal\ebms_article\Entity\Article;
use Drupal\ebms_import\Entity\Batch;

/**
 * Email the report to the usual suspects.
 *
 * @param string|array $information
 *   Report lines array or error message string.
 * @param float $start
 *   When the job started.
 */
function send_report(string|array $information, float $start) {

  @ebms_debug_log('Starting Article XML Refresh report', 1);
  $to = \Drupal::config('ebms_core.settings')->get('dev_notif_addr');
  if (empty($to)) {
    \Drupal::logger('ebms_review')->error('No recipients for article XML refresh report.');
    @ebms_debug_log('Aborting Article XML Refresh report: no recipients registered.', 1);
    return;
  }
  $server = php_uname('n');
  $subject = "EBMS Article XML Refresh ($server)";
  $message = '';
  if (is_array($information)) {
    foreach ($information as $line) {
      $message .= '<p>' . htmlspecialchars($line) . "</p>\n";
    }
  }
  else {
    $message = "<p style=\"color: red; font-size: 1rem; font-weight: bold\">$information</p>\n";
    $subject .= ' [FAILURE]';
  }
  $elapsed = microtime(TRUE) - $start;
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
    \Drupal::logger('ebms_review')->error('Unable to send Article XML Refresh report.');
    @ebms_debug_log('Failure sending report.', 1);
  }
  @ebms_debug_log('Finished Article XML Refresh report', 1);
}

/**
 * Processing logic.
 *
 *  1. Collect information about all the articles we have.
 *  2. Find out from NLM which articles have been updated recently.
 *  3. Update articles whose XML has changed.
 *  4. Report what we did.
 */
function main() {

  try {

    // 1. Collect information about all the articles we have.
    $start = microtime(TRUE);
    $articles = [];
    $latest_mod = '2022-01-01';
    ebms_debug_log('Starting article XML refresh job');
    $query = \Drupal::database()->select('ebms_article', 'article');
    $query->fields('article', ['id', 'source_id', 'import_date', 'update_date', 'data_mod', 'data_checked']);
    $results = $query->execute();
    ebms_debug_log('back from first $query->execute()', 3);
    $record = $results->fetchObject();
    ebms_debug_log('back from first $results->fetchObject()', 3);
    while ($record !== FALSE) {
      $pmid = trim(preg_replace('/\s+/', ' ', $record->source_id));
      if (!empty($pmid)) {
        $latest = $record->import_date;
        if (!empty($record->update_date) && $record->update_date > $latest) {
          $latest = $record->update_date;
        }
        if (!empty($record->data_checked) && $record->data_checked > $latest) {
          $latest = $record->data_checked;
        }
        $latest = substr($latest, 0, 10);
        if (!empty($record->data_mod) && $record->data_mod > $latest_mod) {
          $latest_mod = $record->data_mod;
        }
        // Storing the three values as an array takes more memory than packing
        // the values into a string and unpacking them as we need them, and
        // causes the job to fail when running with PHP's memory_limit value
        // set to 512M. I have asked CBIIT to increase the value to 1024M for
        // the CLI PHP, leaving the value in Apache's php.ini at 512M, so that
        // we don't hit the wall even with the more conservative storage of
        // the values as we accumulate more articles over time.
        // $articles[$pmid] = [$record->id, $latest, $record->data_mod];
        $articles[$pmid] = "{$record->id}|$latest|{$record->data_mod}";
        if (count($articles) % 100 === 0) {
          ebms_debug_log(count($articles) . ' so far ...', 3);
        }
      }
      $record = $results->fetchObject();
    }
    ebms_debug_log('assembled dates for ' . count($articles) . ' articles', 1);

    // 2. Find out from NLM which articles have been updated recently.
    $first = new \DateTime($latest_mod);
    $last = new \DateTime($latest_mod);
    $date = new \DateTime($latest_mod);
    $stop = new \DateTime('-1 week');
    ebms_debug_log("latest mod is $latest_mod; stop is " . $stop->format('Y-m-d'));
    $url = Batch::EUTILS . '/esearch.fcgi';
    $updated = 0;
    while ($date < $stop) {
      $last = clone $date;
      $date_string = $date->format('Y-m-d');
      $parms = 'db=pubmed&retmax=50000000&term=' . $date->format('Y/m/d') . '[MDAT]';
      ebms_debug_log("opening $url?$parms", 3);
      $ch = Batch::getCurlHandle($parms, $url);
      $results = \curl_exec($ch);
      $lines = preg_split("/\r\n|\n|\r/", $results);
      foreach ($lines as $line) {
        if (str_contains($line, '<ERROR>')) {
          $error = 'Failure fetching MDAT information';
          if (preg_match('#<ERROR>(.*)</ERROR>#', $line, $match)) {
            $error .= ': ' . $match[1];
          }
          \Drupal::logger('ebms_article')->error($error);
          ebms_debug_log($error, 1);
          send_report($error, $start);
          exit(1);
        }
        if (preg_match('#<Id>(\d+)</Id>#', $line, $match)) {
          $pmid = $match[1];
          if (array_key_exists($pmid, $articles)) {
            //list($id, $latest, $data_mod) = $articles[$pmid];
            list($id, $latest, $data_mod) = explode('|', $articles[$pmid]);
            if ($data_mod !== $date_string) {
              $article = Article::load($id);
              $article->set('data_mod', $date_string);
              $article->save();
              ebms_debug_log("Set 'date_mod' for article $id (PMID $pmid) to $date_string");
              ++$updated;
            }
          }
        }
      }
      $date->modify('+1 day');
    }
    // Free up this big chunk of memory which we no longer need.
    unset($articles);
    if ($first >= $stop) {
      $first = $first->format('Y-m-d');
      $stop = $stop->format('Y-m-d');
      ebms_debug_log("first=$first stop=$stop", 3);
      $report = ["The 'data_mod' column is up to date (as of $first)."];
    }
    else {
      $first = $first->format('Y-m-d');
      $last = $last->format('Y-m-d');
      $report = ["Modified 'date_mod' for $updated articles modified ($first--$last)."];
    }

    // 3. Update articles whose XML has changed.
    $query = \Drupal::database()->select('ebms_article', 'article');
    $query->isNotNull('article.data_mod');
    $query->where('article.data_checked IS NULL OR article.data_checked < article.data_mod');
    $query->addField('article', 'source_id');
    $pmids = $query->execute()->fetchCol();
    ebms_debug_log(count($pmids) . ' articles queued for XML refresh', 1);
    if (empty($pmids)) {
      $report[] = 'No articles need refresh from changed XML.';
    }
    else {
      $storage = \Drupal::entityTypeManager()->getStorage('taxonomy_term');
      $query = $storage->getQuery()->accessCheck(FALSE);
      $query->condition('vid', 'import_dispositions');
      $query->condition('field_text_id', 'error');
      $ids = $query->execute();
      $error_id = reset($ids);
      $today = date('Y-m-d');
      $offset = $count = 0;
      $batch_size = 100;
      while ($offset < count($pmids)) {
        $subset = array_slice($pmids, $offset, $batch_size);
        $offset += $batch_size;
        $request = [
          'article-ids' => $subset,
          'import-comments' => 'BATCH REPLACEMENT OF UPDATED ARTICLES FROM PUBMED',
        ];
        ebms_debug_log("processing slice beginning at offset $offset", 2);
        $batch = Batch::process($request);
        if (empty($batch->success->value)) {
          if ($batch->messages->count() < 1) {
            $report[] = 'Import of fresh XML failed for unspecified reasons.';
            ebms_debug_log('Import of fresh XML failed for unspecified reasons.', 1);
          }
          else {
            foreach ($batch->messages as $message) {
              $report[] = $message->value;
              ebms_debug_log($message->value, 1);
              break;
            }
          }
        }
        $imported = [];
        foreach ($batch->actions as $action) {
          if ($action->disposition != $error_id && !empty($action->article)) {
            if (empty($action->article)) {
              ebms_debug_log("no article ID for PMID {$action->source_id} with disposition ID {$action->disposition}", 1);
            }
            else {
              $imported[$action->article] = $action->source_id;
            }
          }
        }
        foreach ($imported as $id => $pmid) {
          $article = Article::load($id);
          $article->set('data_checked', $today);
          $article->save();
          ebms_debug_log("Updated article $id (PMID $pmid) from fresh XML", 3);
          $count++;
        }
      }
      if (count($report) === 1) {
        $report[] = "Refreshed $count articles.";
      }
    }

    // 4. Report what we did.
    send_report($report, $start);
  }
  catch (\Exception $e) {
    send_report("failure: $e", $start);
  }
}

main();
?>
