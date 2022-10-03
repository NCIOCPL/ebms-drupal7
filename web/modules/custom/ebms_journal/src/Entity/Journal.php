<?php

namespace Drupal\ebms_journal\Entity;

use Drupal\Core\Entity\ContentEntityBase;
use Drupal\Core\Entity\ContentEntityInterface;
use Drupal\Core\Entity\EntityTypeInterface;
use Drupal\Core\Entity\Query\QueryInterface as QueryQueryInterface;
use Drupal\Core\Entity\QueryInterface;
use Drupal\Core\Field\BaseFieldDefinition;
use Drupal\Core\Field\FieldStorageDefinitionInterface;

/**
 * Information retrieved from NLM about medical journals.
 *
 * Enhanced with information about which journals are preferred ("core")
 * journals, and which are flagged by specific boards as journals whose
 * articles we prefer not to include in the review process by default.
 *
 * @ingroup ebms
 *
 * @ContentEntityType(
 *   id = "ebms_journal",
 *   label = @Translation("Journal"),
 *   base_table = "ebms_journal",
 *   admin_permission = "administer site configuration",
 *   entity_keys = {
 *     "id" = "id"
 *   }
 * )
 */
class Journal extends ContentEntityBase implements ContentEntityInterface {

  /**
   * Where we get the journal information.
   */
  const URL ='ftp://ftp.ncbi.nih.gov/pubmed/J_Medline.gz';

  /**
   * {@inheritdoc}
   */
  public static function baseFieldDefinitions(EntityTypeInterface $entity_type): array {
    $fields = parent::baseFieldDefinitions($entity_type);

    $fields['source'] = BaseFieldDefinition::create('string')
      ->setRequired(TRUE)
      ->setDescription('Source of the journal information (typically Pubmed).')
      ->setSetting('max_length', 32);
    $fields['source_id'] = BaseFieldDefinition::create('string')
      ->setRequired(TRUE)
      ->setDescription('Unique identifier assigned by the source of the journal information.')
      ->setSetting('max_length', 32);
    $fields['title'] = BaseFieldDefinition::create('string')
      ->setRequired(TRUE)
      ->setDescription('Full official title of the journal.')
      ->setSetting('max_length', 512);
    $fields['brief_title'] = BaseFieldDefinition::create('string')
      ->setRequired(TRUE)
      ->setDescription('Short display title for the journal.')
      ->setSetting('max_length', 127);
    $fields['core'] = BaseFieldDefinition::create('boolean')
      ->setDescription('Is this journal one of the preferred journals in the EBMS?');
    $fields['not_lists'] = BaseFieldDefinition::create('ebms_not_list_board')
      ->setDescription('Boards which by default prefer not to include articles from this journal in the review process.')
      ->setCardinality(FieldStorageDefinitionInterface::CARDINALITY_UNLIMITED);

    return $fields;
  }

  /**
   * Create the entity query for the maintenance pages.
   *
   * @param array $values
   *   Parameter values to be incorporated into the query.
   *
   * @return QueryInterface
   *   Entity query for filtering journals.
   */
  public static function createQuery(array $values): QueryQueryInterface {
    $storage = \Drupal::entityTypeManager()->getStorage('ebms_journal');
    $query = $storage->getQuery()->accessCheck(FALSE);
    $brief_title = $values['brief-title'] ?? '';
    $full_title = $values['full-title'] ?? '';
    $journal_id = $values['journal-id'] ?? '';
    $inclusion_exclusion = $values['inclusion-exclusion'] ?? 'all';
    if (!empty($brief_title)) {
      $query->condition('brief_title', "%$brief_title%", 'LIKE');
    }
    if (!empty($full_title)) {
      $query->condition('title', "%$full_title%", 'LIKE');
    }
    if (!empty($journal_id)) {
      $query->condition('source_id', "%$journal_id%", 'LIKE');
    }
    if ($inclusion_exclusion === 'excluded') {
      $query->condition('not_lists.board', $values['board']);
    }
    elseif ($inclusion_exclusion === 'included') {
      $query->addMetaData('board_id', $values['board']);
      $query->addTag('journal_included');
    }
    return $query;
  }

  /**
   * Update the `Journal` entities.
   *
   * Fetches the latest list of journals from the National Library of
   * Medicine, updating titles for journals we already have and adding
   * journals which are new. Does not drop rows for journals which have
   * disappeared from NLM's list. Writes to the ebms_journal table.
   * Records the date/time the refresh was done in a configuration
   * variable.
   *
   * @param bool $force
   *   If `TRUE` ignore the once-per-day limitation on journal refreshes
   *
   * @return array
   *   Array with counts, indexed by the name of the count:
   *     - fetched => how many journals NLM reported
   *     - checked => how many journals we already had
   *     - updated => how many existing entities we changed
   *     - added => how many entities we added for new journals
   *   Also contains elapsed time in number of seconds and an error string.
   */
  public static function refresh($force = FALSE): array {

    // Keep a record of what we do.
    $logger = \Drupal::logger('ebms_journal');

    // Start with an empty report.
    $start = microtime(TRUE);
    $report = [
      'fetched' => 0,
      'checked' => 0,
      'updated' => 0,
      'added' => 0,
      'elapsed' => 0,
      'error' => '',
    ];

    // Don't do this more than once a day unless we're explicitly asked to.
    if (!$force) {
      $last_refresh = \Drupal::config('ebms_journal.settings')->get('last_refresh');
      if (!empty($last_refresh)) {
        $last_refresh = new \DateTime($last_refresh);
        $yesterday = new \DateTime();
        $interval = new \DateInterval('P1D');
        $yesterday->sub($interval);
        if ($last_refresh > $yesterday) {
          $last_refresh = $last_refresh->format('Y-m-d H:i:s');
          $report['elapsed'] = microtime(TRUE) - $start;
          $report['error'] = "Skipping journal refresh, last performed $last_refresh.";
          $logger->warning($report['error']);
          return $report;
        }
      }
    }

    // Find out what NLM has.
    try {
      $nlm_journals = self::fetchJournals();
      ebms_debug_log('Journal::refresh() fetched ' . count($nlm_journals) . ' journals.');
      $report['fetched'] = count($nlm_journals);
    }
    catch (\Exception $e) {
      $message = $e->getMessage();
      $report['error'] = "Fetching journals from NLM: $message.";
      $logger->error($report['error']);
      $report['elapsed'] = microtime(TRUE) - $start;
      return $report;
    }

    // Find out what we have.
    $ids_we_have = [];
    $query = \Drupal::database()->select('ebms_journal', 'journal');
    $query->fields('journal', ['id', 'source_id', 'title', 'brief_title']);
    $results = $query->execute();
    foreach ($results as $result) {
      $nlm_id = $result->source_id;
      $ids_we_have[] = $nlm_id;
      if (array_key_exists($nlm_id, $nlm_journals)) {
        $nlm_journal = $nlm_journals[$nlm_id];
        $full = $nlm_journal['title'];
        $brief = $nlm_journal['brief_title'];
        if ($full !== $result->title || $brief !== $result->brief_title) {
          $journal = Journal::load($result->id);
          if ($full !== $result->title) {
            $journal->set('title', $full);
            ebms_debug_log("Updated title for journal $nlm_id to $full", 1);
          }
          if ($brief !== $result->brief_title) {
            $journal->set('brief_title', $brief);
            ebms_debug_log("Updated brief title for journal $nlm_id to $brief", 1);
          }
          $journal->save();
          $report['updated']++;
        }
      }
    }
    ebms_debug_log('Journal::refresh() checked ' . count($ids_we_have) . ' journals.');
    $report['checked'] = count($ids_we_have);

    // Add the ones we didn't have.
    foreach ($nlm_journals as $nlm_id => $values) {
      if (!in_array($nlm_id, $ids_we_have)) {
        $full = $values['title'];
        $brief = $values['brief_title'];
        Journal::create([
          'title' => $values['title'],
          'brief_title' => $values['brief_title'],
          'source_id' => $nlm_id,
        ])->save();
        ebms_debug_log("Added new journal $full ($nlm_id)", 1);
        $ids_we_have[] = $nlm_id;
        $report['added']++;
      }
    }

    // Remember when this happened.
    $config = \Drupal::configFactory()->getEditable('ebms_journal.settings');
    $config->set('last_refresh', date('Y-m-d H:i:s'));
    $config->save();

    // Finish and return the report.
    $elapsed = microtime(TRUE) - $start;
    $logger->info("Refreshed journal information from NLM in $elapsed seconds.");
    $report['elapsed'] = $elapsed;
    return $report;
  }

  /**
   * Retrieve journal information from NLM.
   *
   * @return array
   *   Titles for each journal indexed by its PubMed ID.
   */
  private static function fetchJournals(): array {

    // Fetch the compressed file of journal information.
    $ch = curl_init(self::URL);
    curl_setopt($ch, CURLOPT_FRESH_CONNECT, TRUE);
    curl_setopt($ch, CURLOPT_RETURNTRANSFER, TRUE);
    curl_setopt($ch, CURLOPT_TIMEOUT, 15);
    curl_setopt($ch, CURLOPT_SSL_VERIFYPEER, FALSE);
    curl_setopt($ch, CURLOPT_SSL_VERIFYHOST, FALSE);
    $compressed_response = curl_exec($ch);
    curl_close($ch);

    // Throw failure exception if no files retrieved
    if (!$compressed_response) {
      throw new \Exception('Unable to retrieve journals from NLM');
    }

    // Uncompress the file and parse it.
    $response = gzdecode($compressed_response);
    $lines = preg_split("/(\r\n|\n|\r)/", $response);
    $pmid = $title = $brief = NULL;
    $journals = [];
    foreach ($lines as $line) {
      $matches = [];
      if (substr($line, 0, 5) == '-----')
        $pmid = $title = $brief = null;
      elseif (preg_match('/NlmId: (.*)/', $line, $matches))
        $pmid = $matches[1];
      elseif (preg_match('/JournalTitle: (.*)/', $line, $matches))
        $title = $matches[1];
      elseif (preg_match('/MedAbbr: (.*)/', $line, $matches))
        $brief = $matches[1];
      if ($pmid && $title) {
        $journals[$pmid] = [
          'title' => $title,
          'brief_title' => $brief ?: $title,
        ];
        $pmid = $title = $brief = null;
      }
    }

    // Should be well over 30,000 journals.
    $count = count($journals);
    if ($count < 30000) {
      throw new \Exception("Only received $count journals from NLM.");
    }

    // We have what we came for.
    return $journals;
  }

}
