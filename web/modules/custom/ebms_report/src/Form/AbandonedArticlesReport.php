<?php

namespace Drupal\ebms_report\Form;

use Drupal\Core\Form\FormBase;
use Drupal\Core\Form\FormStateInterface;
use Drupal\Core\Link;
use Drupal\ebms_article\Entity\Article;
use Drupal\ebms_core\Entity\SavedRequest;
use Drupal\ebms_import\Entity\Batch;

/**
 * Find articles which NLM used to have but claim they can no longer find.
 *
 * @ingroup ebms
 */
class AbandonedArticlesReport extends FormBase {

  /**
   * Regular expression used to extract PubMed IDs.
   */
  const PATTERN = '#<Id>(\d+)</Id>#';

  /**
   * Address for API used to find which articles NLM still has.
   */
  const URL = Batch::EUTILS . '/esearch.fcgi';

  /**
   * {@inheritdoc}
   */
  public function getFormId(): string {
    return 'abandoned_articles_report_form';
  }

  /**
   * {@inheritdoc}
   */
  public function buildForm(array $form, FormStateInterface $form_state): array {

    // Create the form and add the fields.
    $form = [
      '#title' => 'Invalid PubMed IDs',
      'options' => [
        '#type' => 'details',
        '#title' => 'Optional Controls',
        'batch' => [
          '#type' => 'number',
          '#title' => 'Batch Size',
          '#description' => 'The number of articles to check in a single request sent to PubMed.',
          '#min' => 1000,
          '#max' => 100000,
          '#step' => 1000,
          '#default_value' => 10000,
        ],
        'delay' => [
          '#type' => 'number',
          '#title' => 'Delay',
          '#description' => 'Number of seconds to wait between requests, to avoid overwhelming the service and being locked out.',
          '#min' => .5,
          '#max' => 15,
          '#default_value' => .5,
          '#step' => .5,
        ],
        'all' => [
          '#type' => 'checkbox',
          '#title' => 'Check all articles',
          '#description' => 'By default only the active articles are checked. Checking all articles in the EBMS takes a long time, and risks timing out the report.',
        ],
      ],
      'submit' => [
        '#type' => 'submit',
        '#value' => 'Submit',
      ],
    ];

    // Add the report if we have a request.
    $values = $form_state->getValues();
    if (!empty($values)) {
      SavedRequest::saveParameters('abandoned articles report', $values);
      $report = self::report($values['all'], $values['batch'], $values['delay']);
      if (empty($values['all'])) {
        $checked = 'Checked ' . $report['checked'] . ' Active Articles';
      }
      else {
        $checked = 'Checked All ' . $report['checked'] . ' Articles';
      }
      $missing_count = count($report['missing']);
      $missing_s = $missing_count === 1 ? '' : 's';
      $items = [];
      foreach ($report['missing'] as $id => $pmid) {
        $text = "$pmid (EBMS ID $id)";
        $items[] = Link::createFromRoute($text, 'ebms_article.article', ['article' => $id], ['attributes' => ['target' => '_blank']]);
      }
      $form['report'] = [
        '#theme' => 'item_list',
        '#title' => "$missing_count Invalid PubMed ID$missing_s ($checked)",
        '#items' => $items,
        '#empty' => 'No invalid PubMed IDs were found.',
      ];

    }
    return $form;
  }

  /**
   * {@inheritdoc}
   */
  public function submitForm(array &$form, FormStateInterface $form_state) {
    $form_state->setRebuild();
  }

  /**
   * Determine the values needed for the report.
   *
   * @param bool $all
   *   If `TRUE` check all articles. Otherwise (the default) check only
   *   articles which are still active.
   * @param int $batch_size
   *   Number of articles to ask about in a single request.
   * @param float $delay
   *   Number of seconds to wait between requests.
   *
   * @return array
   *   Report data, including number of articles checked and identified
   *   missing articles.
   */
  public static function report(bool $all = FALSE, int $batch_size = 10000, float $delay = .5) {

    // Find out which articles we need to check.
    if (!$all) {
      $articles = Article::active();
    }
    else {
      $query = \Drupal::entityQueryAggregate('ebms_article');
      $query->groupBy('id');
      $query->groupBy('source_id');
      $ids = $query->execute();
      $articles = [];
      foreach ($ids as $article_ids) {
        $articles[$article_ids['id']] = $article_ids['source_id'];
      }
    }

    // Find out what NLM still has, so we can identify what's missing.
    $found = self::find($articles, $batch_size, $delay);

    // Add in the render array for the articles which have disappeared.
    return [
      'checked' => count($articles),
      'missing' => array_diff($articles, $found),
    ];
  }

  /**
   * Find out which of the articles we have NLM can still find.
   *
   * This is the batching wrapper which slices the job up into
   * smaller batches, so we don't overwhelm NLM's server.
   *
   * @param array $pmids
   *   List of PubMed IDs for the articles we got from PubMed.
   * @param int $batch_size
   *   Number of articles to ask about in a single request.
   * @param int $delay
   *   Number of seconds to wait between requests.
   *
   * @return array
   *   PubMed IDs reported as found by NLM.
   */
  private static function find(array $pmids, int $batch_size, float $delay): array {

    // Batch the requests so we don't overwhelm the service.
    $batch_size = $batch_size ?: count($pmids);
    $offset = 0;
    $found = [];
    while ($offset < count($pmids)) {
      if (!empty($offset) && !empty($delay)) {
        usleep($delay * 1000000);
      }
      $slice = array_slice($pmids, $offset, $batch_size);
      $verified = self::verify($slice);
      $found = array_merge($found, $verified);
      $offset += $batch_size;
    }
    return $found;
  }

  /**
   * Ask NLM to confirm which of this set of articles it can still find.
   */
  private static function verify($pmids) {

    // Ask NLM which of these it still has.
    $max = count($pmids);
    $parms = "db=pubmed&retmax=$max&term=" . implode(',', $pmids) . '[UID]';
    $ch = Batch::getCurlHandle($parms, self::URL);
    $results = curl_exec($ch);
    curl_close($ch);

    // Check for problems.
    $error = [];
    if (preg_match('#<ERROR>(.*)</ERROR>#Us', $results, $error)) {
      throw new \Exception('PMID ERROR: ' . $error[1]);
    }
    if (!preg_match('#<IdList#', $results)) {
      throw new \Exception('PMID FAILURE: ' . $results);
    }

    // Extract the IDs into an array.
    $offset = 0;
    $verified = [];
    while (TRUE) {
      $matches = [];
      $found = preg_match(self::PATTERN, $results, $matches, PREG_OFFSET_CAPTURE, $offset);
      if (empty($found)) {
        return $verified;
      }
      $verified[] = trim($matches[1][0]);
      $offset = $matches[1][1] + strlen($matches[1][0]);
    }
  }

}
