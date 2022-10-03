<?php

namespace Drupal\ebms_import\Entity;

use Drupal\Core\Entity\ContentEntityBase;
use Drupal\Core\Entity\ContentEntityInterface;
use Drupal\Core\Entity\EntityTypeInterface;
use Drupal\file\Entity\File;
use Drupal\Core\Field\BaseFieldDefinition;
use Drupal\Core\Field\FieldStorageDefinitionInterface;
use Drupal\Core\Site\Settings;
use Drupal\datetime\Plugin\Field\FieldType\DateTimeItem;
use Drupal\ebms_article\Entity\Article;
use Drupal\ebms_board\Entity\Board;

/**
 * Batch of articles requested to be imported into the EBMS.
 *
 * Articles are imported into the EBMS in batches. We track the history of
 * these batch requests. The word "import" has more than one meaning in this
 * context, which can be confusing. Sometimes it is used to mean bringing in
 * an article which has never been in the system before. Sometimes it is used
 * to refer to the process of adding a review topic to an article which was
 * previously imported for a different topic. Sometimes it is used to mean
 * refreshing the information for an article we already have in the system,
 * using updated values retrieved from NLM. These various meanings reflect
 * the way the users think about and describe the system.
 *
 * The processing flow goes like this (in the absence of failures), at a
 * high level:
 *  1. The user submits the import request form.
 *  2. The form's submit handler invokes Batch::process with the form values.
 *  3. The static process() method creates a Batch entity.
 *  4. The Batch entity is used to fetch and save the articles.
 *  5. The Batch entity is saved for use by reports.
 *
 * @ingroup ebms
 *
 * @ContentEntityType(
 *   id = "ebms_import_batch",
 *   label = @Translation("Import Batch"),
 *   base_table = "ebms_import_batch",
 *   admin_permission = "administer site configuration",
 *   entity_keys = {
 *     "id" = "id"
 *   }
 * )
 */
class Batch extends ContentEntityBase implements ContentEntityInterface {

  /**
   * Import type constants.
   */
  const IMPORT_TYPE_REGULAR = 'R';
  const IMPORT_TYPE_FAST_TRACK = 'F';
  const IMPORT_TYPE_SPECIAL_SEARCH = 'S';
  const IMPORT_TYPE_DATA_REFRESH = 'D';
  const IMPORT_TYPE_INTERNAL = 'I';

  /**
   * Don't wear out our welcome with NLM's PubMed service.
   */
  const PUBMED_BATCH_SIZE = 100;

  /**
   * Prefix for the NLM EUTILS API.
   */
  const EUTILS = 'https://eutils.ncbi.nlm.nih.gov/entrez/eutils';

  /**
   * Base URL for fetching article records from PubMed.
   */
  const URL = self::EUTILS . '/efetch.fcgi';

  /**
   * Front of parameter string for requests to PubMed for article records.
   */
  const PARMS = 'db=pubmed&rettype=medline&retmode=xml&id=';

  /**
   * Regex for error responses on request as a whole.
   */
  const E_FETCH_RESULT = '/<eFetchResult.*>(?P<result>.*)<\/eFetchResult>/smuU';

  /**
   * Regex for error inside an eFetchResponse.
   */
  const ERR_MSG = '/<ERROR>(?P<errMsg>.*)<\/ERROR>/smuU';

  /**
   * How we can tell if we got a results set.
   */
  const PUBMED_ARTICLE_SET = '/<!DOCTYPE PubmedArticleSet/m';

  /**
   * Used during processing of the requests for creating state entities.
   *
   * @var int
   */
  protected int $board = 0;

  /**
   * Keyed map of import dispositions.
   *
   * @var array
   */
  protected array $dispositions = [];

  /**
   * List of journals from which we will not import articles.
   *
   * @var array
   */
  protected array $notList = [];

  /**
   * Was this import request just a practice run, instead of a live job?
   *
   * @var bool
   */
  protected bool $test = FALSE;

  /**
   * List of IDs for articles which should be imported in a followup import job.
   *
   * @var array
   */
  protected array $followup = [];

  /**
   * List of articles which are marked as ready for review.
   *
   * @var array
   */
  protected array $readyForReview = [];

  /**
   * List of unique PubMed IDs.
   *
   * @var array
   */
  private array $uniqueIds = [];

  /**
   * Access method for test mode flag.
   *
   * @return bool
   *   TRUE if this was just a test run.
   */
  public function isTest(): bool {
    return $this->test;
  }

  /**
   * Get articles which should be imported by a followup job.
   *
   * See OCEEBMS-568 and OCEEBMS-600.
   *
   * @return array
   *   List of PubMed IDs for related articles we should import.
   */
  public function getFollowup(): array {
    return $this->followup;
  }

  /**
   * {@inheritdoc}
   */
  public static function baseFieldDefinitions(EntityTypeInterface $entity_type): array {
    $fields = parent::baseFieldDefinitions($entity_type);

    $fields['topic'] = BaseFieldDefinition::create('entity_reference')
      ->setDescription('Every regular import batch is associated with a single topic.')
      ->setSetting('target_type', 'ebms_topic');
    $fields['source'] = BaseFieldDefinition::create('string')
      ->setDescription('At this time all of the articles in the system have been imported from PubMed.')
      ->setRequired(TRUE)
      ->setSetting('max_length', 32);
    $fields['imported'] = BaseFieldDefinition::create('datetime')
      ->setDescription('When the import happened.');
    $fields['cycle'] = BaseFieldDefinition::create('datetime')
      ->setDescription('The month for which review of the article in this batch is targeted for this topic.')
      ->setSettings(['datetime_type' => DateTimeItem::DATETIME_TYPE_DATE]);
    $fields['user'] = BaseFieldDefinition::create('entity_reference')
      ->setDescription('User requesting this import batch.')
      ->setRequired(TRUE)
      ->setSetting('target_type', 'user');
    $fields['not_list'] = BaseFieldDefinition::create('boolean')
      ->setDescription('If TRUE, suppress import of articles published in less desirable journals.');
    $fields['import_type'] = BaseFieldDefinition::create('entity_reference')
      ->setDescription('What type of import request is this (e.g., fast_track).')
      ->setRequired(TRUE)
      ->setSetting('target_type', 'taxonomy_term')
      ->setSetting('handler_settings',
                   ['target_bundles' => ['import_types' => 'import_types']]);
    $fields['article_count'] = BaseFieldDefinition::create('integer')
      ->setDescription('Number of unique articles in this batch.')
      ->setRequired(TRUE);
    $fields['comment'] = BaseFieldDefinition::create('string')
      ->setDescription('Optional notes on this import request.')
      ->setSetting('max_length', 2048);
    $fields['messages'] = BaseFieldDefinition::create('string_long')
      ->setDescription('System messages from any batch-level failures (article-specific import failures are recorded elsewhere).')
      ->setCardinality(FieldStorageDefinitionInterface::CARDINALITY_UNLIMITED);
    $fields['success'] = BaseFieldDefinition::create('boolean')
      ->setDescription('If TRUE the batch as a whole succeeded, though some articles may have failed import; otherwise the batch as a whole failed, though individual articles may have been imported.');
    $fields['actions'] = BaseFieldDefinition::create('ebms_import_action')
      ->setDescription('What we did for each article in the batch import request.')
      ->setCardinality(FieldStorageDefinitionInterface::CARDINALITY_UNLIMITED);

    return $fields;
  }

  /**
   * Process an import request.
   *
   * Note that dependency injection is not available for a custom entity,
   * according to "Clive" (https://drupal.stackexchange.com/users/2800/clive).
   * See https://drupal.stackexchange.com/questions/259784.
   *
   * We fetch articles from PubMed in batches, and we pause for a second
   * between batches so that we don't wear out our welcome at NLM.
   *
   * @param array $request
   *   Dictionary of request options.
   *
   * @return Batch
   *   Batch entity recording what happened in the import job.
   *
   * @throws \Exception
   *   Extra protection against violation of assumptions.
   */
  public static function process(array $request): Batch {

    // Get the request values needed for the import job.
    $logger = \Drupal::logger('ebms_import');
    if (empty($request['article-ids'])) {
      throw new \Exception('No articles specified in import request.');
    }
    $import_type = $request['import-type'] ?? '';
    $storage = \Drupal::entityTypeManager()->getStorage('taxonomy_term');
    if (empty($import_type)) {
      if (empty($request['topic'])) {
        $text_id = self::IMPORT_TYPE_DATA_REFRESH;
      }
      elseif (empty($request['fast-track'])) {
        $text_id = self::IMPORT_TYPE_REGULAR;
      }
      else {
        $text_id = self::IMPORT_TYPE_FAST_TRACK;
      }
      $query = $storage->getQuery()->accessCheck(FALSE);
      $query->condition('vid', 'import_types');
      $query->condition('field_text_id', $text_id);
      $ids = $query->execute();
      if (count($ids) !== 1) {
        throw new \Exception("Can't find import type '$text_id'!");
      }
      $import_type = reset($ids);
    }
    elseif (empty($request['topic'])) {
      $term = $storage->load($import_type);
      $text_id = $term->field_text_id->value;
      if ($text_id !== self::IMPORT_TYPE_DATA_REFRESH && $text_id !== self::IMPORT_TYPE_INTERNAL) {
        throw new \Exception('Topic is required for this import type.');
      }
    }
    $pmids = array_unique($request['article-ids'], SORT_NUMERIC);
    $values = [];
    $values['topic'] = $topic_id = $request['topic'] ?? 0;
    $values['source'] = 'Pubmed';
    $values['imported'] = $now = date('Y-m-d H:i:s');
    $values['cycle'] = $cycle = $request['cycle'] ?? '';
    $values['user'] = $user = $request['user'] ?? \Drupal::currentUser()->id();
    $values['not_list'] = empty($request['override-not-list']) && !empty($topic_id);
    $values['import_type'] = $import_type;
    $values['article_count'] = 0;
    $values['comment'] = $request['import-comments'] ?? NULL;
    $values['messages'] = NULL;
    $values['success'] = TRUE;
    $values['actions'] = [];
    if (!empty($topic_id) && empty($values['cycle'])) {
      throw new \Exception('Cycle must be specified for topic-specific import.');
    }

    // Create the `Batch` entity.
    $batch = Batch::create($values);
    $batch->test = !empty($request['test-mode']);
    if (!empty($topic_id)) {
      $topics = \Drupal::entityTypeManager()->getStorage('ebms_topic');
      $topic = $topics->load($topic_id);
      $batch->board = $board_id = $topic->board->target_id;
      if (!empty($values['not_list'])) {
        $not_list_service = \Drupal::service('ebms_journal.not_list');
        $batch->notList = $not_list_service->getNotList($batch->board);
      }
    }

    // Process each unique PubMed ID.
    if (!$batch->isTest()) {
      $core_comment = 'Published because of import from core journals';
      $fast_track_comment = $request['fast-track-comments'] ?? NULL;
      $placement = $request['placement'] ?? '';
      if ($placement === 'bma') {
        $placement = $request['bma-disposition'] ?? 0;
      }
      $meeting = $request['meeting'] ?? 0;
      $decision = NULL;
      if (!empty($request['disposition'])) {
        $decision = [
          'decision' => $request['disposition'],
          'meeting_date' => $cycle,
          'discussed' => FALSE,
        ];
      }
      $topic_comment = $request['mgr-comment'] ?? '';
      if (!empty($topic_comment)) {
        $topic_comment = [
          'user' => $user,
          'entered' => $now,
          'comment' => $topic_comment,
        ];
      }
    }
    $fetched = [];
    $offset = 0;
    $slice = array_slice($pmids, $offset, self::PUBMED_BATCH_SIZE);
    $debugging = Settings::get('APP_ENV') === 'dev';
    while (!empty($slice)) {
      $response = $batch->fetch($slice);
      if (empty($batch->success->value)) {
        $slice = '';
      }
      else {
        $docs = $batch->split($response);
        foreach ($docs as $doc) {
          static $done = FALSE;
          if (!$done && $debugging) {
            file_put_contents('/tmp/doc.xml', $doc, FILE_APPEND);
          }
          try {
            $values = Article::parse($doc);
            if ($debugging) {
              @file_put_contents('/tmp/values.json', json_encode($values, JSON_PRETTY_PRINT), FILE_APPEND);
            }
          }
          catch (\Exception $e) {
            $err = $e->getMessage();
            $message = "Error parsing article XML: $err";
            $logger->error($message);
            $batch->addErrorMessage($message);
            break;
          }
          $done = TRUE;
          $pmid = $values['source_id'];
          if (empty($pmid)) {
            throw new \Exception(json_encode($values, JSON_PRETTY_PRINT));
          }
          if (in_array($pmid, $fetched)) {
            $message = 'Article already received from NLM for this batch.';
            $batch->addAction($pmid, 'error', NULL, $message);
            continue;
          }
          $fetched[] = $pmid;
          if (!in_array($pmid, $pmids)) {
            $message = 'Received unrequested article.';
            $batch->addAction($pmid, 'error', NULL, $message);
            continue;
          }
          try {
            $article = $batch->import($values);
            if (empty($article)) {
              ebms_debug_log("skipping failed article with PMID $pmid", 1);
              continue;
            }
            $article_id = $article->id();
          }
          catch (\Exception $e) {
            $err = $e->getMessage();
            $message = "Error importing article: $err";
            $logger->error($message);
            $batch->addErrorMessage($message);
            break;
          }
          if (!$batch->isTest()) {
            $article_changed = FALSE;
            if (!empty($request['full-text-id'])) {
              $fid = $request['full-text-id'];
              $file = File::load($fid);
              $file_usage = \Drupal::service('file.usage');
              $file_usage->add($file, 'ebms_article', 'ebms_article', $article_id);
              $article->full_text = [
                'file' => $fid,
                'unavailable' => FALSE,
              ];
              $article_changed = TRUE;
            }
            if (!empty($request['special-search'])) {
              $article->addTag('i_specialsearch', $topic_id, $user, $now);
              $article_changed = TRUE;
            }
            if (!empty($request['hi-priority'])) {
              $article->addTag('high_priority', $topic_id, $user, $now);
              $article_changed = TRUE;
            }
            if (!empty($request['core-journals-search'])) {
              $article->addTag('i_core_journals', $topic_id, $user, $now);
              if (empty($request['fast-track'])) {
                $article->addState('published', $topic_id, $user, $now, $cycle, $core_comment);
              }
              $article_changed = TRUE;
            }
            if (!empty($request['fast-track']) && in_array($article_id, $batch->readyForReview)) {
              $article->addTag('i_fasttrack', $topic_id, $user, $now);
              $state = $article->addState($placement, $topic_id, $user, $now, $cycle, $fast_track_comment);
              $article_changed = TRUE;
              $state_changed = FALSE;
              if ($placement === 'on_agenda' && !empty($meeting)) {
                $state->meetings[] = $meeting;
                $state_changed = TRUE;
              }
              elseif ($placement === 'final_board_decision' && !empty($decision)) {
                $state->decisions[] = $decision;
                $state_changed = TRUE;
              }
              if ($state_changed) {
                $state->save();
              }
            }
            if (!empty($topic_comment)) {
              $article_topic = $article->getTopic($topic_id);
              if (!empty($article_topic)) {
                $article_topic->comments[] = $topic_comment;
                $article_topic->save();
              }
            }
            if (!empty($request['internal-tags'])) {
              $already_added = [];
              foreach ($article->internal_tags as $internal_tag) {
                $already_added[] = $internal_tag->tag;
              }
              foreach ($request['internal-tags'] as $internal_tag_id) {
                if (!in_array($internal_tag_id, $already_added)) {
                  $article->internal_tags[] = [
                    'tag' => $internal_tag_id,
                    'added' => $now,
                  ];
                  $article_changed = TRUE;
                }
              }
            }
            if (!empty($request['internal-comment'])) {
              $article->internal_comments[] = [
                'user' => $user,
                'entered' => $now,
                'body' => $request['internal-comment'],
              ];
              $article_changed = TRUE;
            }
            if ($article_changed) {
              $article->save();
            }
          }
        }
        $offset += self::PUBMED_BATCH_SIZE;
        $slice = array_slice($pmids, $offset, self::PUBMED_BATCH_SIZE);
        if (!empty($slice)) {
          usleep(500000);
        }
      }
    }

    // Record any requested articles which NLM did not give us.
    $missing = array_diff($pmids, $fetched);
    $message = 'No article with this Pubmed ID was returned by Pubmed';
    foreach ($missing as $pmid) {
      $batch->addAction($pmid, 'error', NULL, $message);
    }

    // Clean up the list of articles we want to get in a followup job.
    if (!empty($batch->followup)) {
      $followup = array_unique($batch->followup);
      $batch->followup = array_diff($followup, $pmids);
    }

    // Save the batch if this is a live import job.
    if (!$batch->test) {
      $batch->save();
    }
    return $batch;
  }

  /**
   * Record an error message and mark the batch as failed.
   *
   * @param string $message
   *   Error message to be recorded.
   */
  protected function addErrorMessage(string $message) {
    $this->messages->appendItem($message);
    $this->set('success', FALSE);
  }

  /**
   * Retrieve and parse article records from NLM.
   *
   * @param array $pmids
   *   Sequence of one or more PubMed IDs.
   *
   * @return string
   *   Response to our request for PubMed articles.
   */
  protected function fetch(array $pmids): string {

    // Record problems as they arise.
    $logger = \Drupal::logger('ebms_import');

    // Initialize a curl object for a POST request.
    $parms = self::PARMS . implode(',', $pmids);
    try {
      $ch = self::getCurlHandle($parms);
    }
    catch (\Exception $e) {
      $error = $e->getMessage();
      $message = "Unable to connect to NLM: $error";
      $logger->error($message);
      $this->addErrorMessage($message);
      return '';
    }

    // Post it.
    $response = curl_exec($ch);

    // Failed?
    if (!$response) {
      $error = curl_error($ch);
      $message = "Unable to retrieve data from NLM: $error";
      $logger->error($message);
      $this->addErrorMessage($message);
    }
    else {
      $error = '';
      $matches = [];
      if (preg_match(self::E_FETCH_RESULT, $response, $matches)) {
        $text = trim($matches['result'] ?? '');
        $matches = [];
        if (preg_match(self::ERR_MSG, $text, $matches)) {
          $error = trim($matches['errMsg'] ?? '');
        }
        else {
          $error = $text;
        }
      }
      elseif (!preg_match(self::PUBMED_ARTICLE_SET, $response)) {
        $error = $response;
      }
      if (!empty($error)) {
        $code = curl_getinfo($ch, CURLINFO_RESPONSE_CODE);
        $msg = "Request error returned by NLM (HTTP CODE $code): $error";
        $logger->error($msg);
        $this->addErrorMessage($msg);
      }
    }

    curl_close($ch);
    return $response;
  }

  /**
   * Split the response into separate XML documents for each article.
   *
   * The responses are so large in some cases that the parser runs
   * out of memory, so we have to use string processing to split
   * out the separate sub-documents instead of parsing the entire
   * response. A little unfortunate, but it has always worked correctly,
   * and we are using a real XML parser to parse the extracted documents.
   *
   * @param string $response
   *   Response from the request for article records from NLM.
   *
   * @return array
   *   Separate XML document for each article.
   */
  public static function split(string $response) : array {
    $docs = [];
    list($open, $close) = ['<PubmedArticle>', '</PubmedArticle>'];
    $start = strpos($response, $open);
    while ($start !== FALSE) {
      $end = strpos($response, $close, $start);
      if ($end === FALSE) {
        $start = FALSE;
      }
      else {
        $end += strlen($close);
        $docs[] = substr($response, $start, $end - $start);
        $start = strpos($response, $open, $end);
      }
    }
    return $docs;
  }

  /**
   * Record an action on an article for this import job.
   *
   * The valid disposition text IDs are: imported, review_ready, not_listed,
   * duplicate, topic_added, replaced, and error.
   *
   * @param string $pmid
   *   Source ID for the article.
   * @param string $disposition
   *   Text ID for the disposition to be recorded.
   * @param int|null $article
   *   ID for the EBMS `Article` entity, if available.
   * @param string $message
   *   Optional message with details about the disposition.
   *
   * @throws \Exception
   *   If unhandled failures occur further down the call stack.
   */
  public function addAction(string $pmid, string $disposition, ?int $article, string $message = '') {
    $this->uniqueIds[$pmid] = $pmid;
    $this->set('article_count', count($this->uniqueIds));
    if (empty($this->dispositions)) {
      $storage = \Drupal::entityTypeManager()->getStorage('taxonomy_term');
      $query = $storage->getQuery()->accessCheck(FALSE);
      $query->condition('vid', 'import_dispositions');
      $ids = $query->execute();
      foreach ($storage->loadMultiple($ids) as $term) {
        $this->dispositions[$term->get('field_text_id')->value] = $term->id();
      }
    }
    $values = [
      'source_id' => $pmid,
      'disposition' => $this->dispositions[$disposition],
    ];
    if (!empty($article)) {
      $values['article'] = $article;
    }
    if (!empty($message)) {
      $values['message'] = $message;
    }
    $this->actions->appendItem($values);
  }

  /**
   * Import a new article or update an existing one.
   *
   * @param array $values
   *   Keyed array of values for the `Article` entity's fields.
   *
   * @throws \Exception
   *   Bubble up exceptions from the stack.
   */
  protected function import(array $values) {
    $logger = \Drupal::logger('ebms_import');
    $comments_corrections = $values['comments_corrections'];
    unset($values['comments_corrections']);
    $now = $this->imported->value;
    $source = $values['source'];
    $pmid = $values['source_id'];
    $journal_id = $values['source_journal_id'];
    $board = $this->board;
    $topic = $board ? $this->topic->target_id : 0;
    $cycle = $board ? $this->cycle->value : '';
    $user = $this->user->target_id;
    $not_listed = in_array($journal_id, $this->notList);
    $comment = $this->comment->value;
    try {
      $article = Article::getArticleBySourceId($source, $pmid);
    }
    catch (\Exception $e) {
      $message = $e->getMessage();
      $logger->error($message);
      $this->addAction($pmid, 'error', NULL, $message);
      return;
    }

    // Do we already have this article?
    if (empty($article)) {

      // This is a new article. Save it if this isn't a test run.
      $values['imported_by'] = $user;
      $values['import_date'] = $now;
      $article = Article::create($values);
      if (!$this->isTest()) {
        $article->save();
        $article_id = $article->id();
      }
      else {
        $article_id = NULL;
      }

      // Record actions and the new state as appropriate.
      $this->addAction($pmid, 'imported', $article_id);
      if (!$this->isTest() && !empty($topic)) {
        $article->addState('ready_init_review', $topic, $user, $now, $cycle, $comment);
      }
      if (!empty($topic)) {
        $this->addAction($pmid, 'review_ready', $article_id);
        $this->readyForReview[] = $article_id;
      }
    }
    else {

      // We already have this article.
      $article_id = $article->id();

      // Record actions and the new state as appropriate.
      // @todo Ask the users if they agree we should streamline this.
      // For an existing record, we should probably only add a 'duplicate'
      // action if we're not adding a new topic for the article, and
      // not replacing old values. See OCEEBMS-613.
      // 2022-03-25: The users have agreed to this proposal.
      if ($article->refresh($values, $now)) {
        $this->addAction($pmid, 'replaced', $article_id);
      }
      $current_state = $article->getCurrentState($topic);
      if (!empty($topic) && empty($current_state)) {
        $this->addAction($pmid, 'topic_added', $article_id);
        $this->addAction($pmid, 'review_ready', $article_id);
        $this->readyForReview[] = $article_id;
        if (!$this->isTest()) {
          $article->addState('ready_init_review', $topic, $user, $now, $cycle, $comment);
        }
      }
      else {
        $this->addAction($pmid, 'duplicate', $article_id);
      }

      // See if we should reject the article now.
      if ($not_listed) {
        if (!empty($current_state)) {
          $text_id = $current_state->value->entity->field_text_id;
          if ($text_id !== 'reject_journal_title') {
            $not_listed = FALSE;
          }
        }
      }
    }

    // Is the article in a journal we don't usually import from for this board?
    if ($not_listed) {
      $this->addAction($pmid, 'not_listed', $article_id);
      if (!$this->isTest()) {
        $article->addState('reject_journal_title', $topic, $user, $now, $cycle, $comment);
      }
    }

    // Find out if there are any related articles which should also be imported.
    if (!empty($topic) && !empty($comments_corrections) && $article->inCoreJournal()) {
      if (!empty(Board::load($board)->auto_imports->value)) {
        $journal_id = $article->source_journal_id->value;
        foreach ($comments_corrections as $other_pmid) {
          try {
            $other_article = Article::getArticleBySourceId('Pubmed', $other_pmid);

            // Don't bother if we already have the article.
            if (empty($other_article)) {

              // Skip it if it's in a different journal.
              $other_journal_id = $this->getPubmedArticleJournalId($other_pmid);
              if ($other_journal_id === $journal_id) {
                $this->followup[] = $other_pmid;
              }
            }
          }
          catch (\Exception $e) {
            $message = 'Checking for related articles: ' . $e->getMessage();
            $logger->error($message);
            $this->addAction($pmid, 'error', NULL, $message);
          }
        }
      }
    }

    // Store the changes to the article if we're not just testing.
    if (!$this->isTest()) {
      $article->save();
    }

    // Let the caller do any necessary followup processing on the article.
    return $article;
  }

  /**
   * Find out from Pubmed which journal an article is published in.
   *
   * We use this for articles we don't already have to determine whether
   * we should consider importing them in a followup job.
   *
   * @param string $pmid
   *   PubMed ID for an article we don't have.
   *
   * @return string
   *   Journal ID for the article.
   *
   * @throws \Exception
   *   If unhandled failures occur further down the call stack.
   */
  public function getPubmedArticleJournalId(string $pmid): string {

    // Fetch the article document.
    try {
      $ch = self::getCurlHandle(self::PARMS . $pmid);
    }
    catch (\Exception $e) {
      $logger = \Drupal::logger('ebms_import');
      $error = $e->getMessage();
      $message = "Unable to connect to NLM: $error";
      $logger->error($message);
      $this->addErrorMessage($message);
      return '';
    }
    $results = curl_exec($ch);
    curl_close($ch);
    usleep(500000);

    // Parse the document and extract the journal ID.
    if (empty($results)) {
      return '';
    }
    $article_set = new \SimpleXMLElement($results);
    $article = $article_set->PubmedArticle;
    return $article->MedlineCitation->MedlineJournalInfo->NlmUniqueID;
  }

  /**
   * Create a connection for posting HTTP requests.
   *
   * @param string $parms
   *   Parameters for the request.
   * @param string $url
   *   Base URL for the request (defaults to PubMed fetch URL).
   *
   * @return \CurlHandle
   *   Handle to the connection object.
   *
   * @throws \Exception
   *   If unable to create a connection object.
   */
  public static function getCurlHandle(string $parms, string $url = self::URL): \CurlHandle {
    $ch = curl_init();
    if (empty($ch)) {
      throw new \Exception('Unable to create an HTTP connection object.');
    }
    curl_setopt($ch, CURLOPT_URL, $url);
    curl_setopt($ch, CURLOPT_POST, TRUE);
    curl_setopt($ch, CURLOPT_POSTFIELDS, $parms);
    curl_setopt($ch, CURLOPT_SSL_VERIFYPEER, FALSE);
    curl_setopt($ch, CURLOPT_SSL_VERIFYHOST, FALSE);

    // This one puts the return in $results instead of stdout.
    curl_setopt($ch, CURLOPT_RETURNTRANSFER, TRUE);
    return $ch;
  }

  /**
   * Create an array of review cycles, most recent first.
   *
   * @return array
   *   Cycle labels (e.g., 'January 2020') indexed by first days.
   */
  public static function cycles() {
    $cycles = [];
    $month = new \DateTime('first day of next month');
    $first = new \DateTime('2002-06-01');
    while ($month >= $first) {
      $cycles[$month->format('Y-m-d')] = $month->format('F Y');
      $month->modify('previous month');
    }
    return $cycles;
  }

  /**
   * Convert ISO date string to Month Year format.
   *
   * @param string $date
   *
   * @return string
   *   For example, passing '2020-12-01' returns 'December 2020'.
   */
  public static function cycleString(string $date): string {
    $date_time = new \DateTime($date);
    return $date_time->format('F Y');
  }

}
