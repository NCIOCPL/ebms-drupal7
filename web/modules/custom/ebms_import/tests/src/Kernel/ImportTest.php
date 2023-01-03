<?php

namespace Drupal\Tests\ebms_import\Kernel;

use Drupal\ebms_article\Entity\Article;
use Drupal\ebms_board\Entity\Board;
use Drupal\ebms_import\Entity\Batch;
use Drupal\ebms_journal\Entity\Journal;
use Drupal\ebms_meeting\Entity\Meeting;
use Drupal\ebms_topic\Entity\Topic;
use Drupal\KernelTests\KernelTestBase;
use Drupal\taxonomy\Entity\Term;
use Drupal\user\Entity\User;

/**
 * Test the import scenarios.
 *
 * @group ebms
 */
class ImportTest extends KernelTestBase {

  const TAG_MAP = [
    'special-search' => 'i_specialsearch',
    'core-journals-search' => 'i_core_journals',
    'hi-priority' => 'high_priority',
    'fast-track' => 'i_fasttrack',
  ];
  const NOT_LIST_TEST = 'Not-list test';
  const DUPLICATE_IMPORT_TEST = 'Duplicate import test';
  const ORIGINAL_IMPORT_COMMENT = 'Original import request';
  const INTERNAL_IMPORT_TEST = 'Internal import test';
  const IMPORT_REFRESH_TEST = 'BATCH REPLACEMENT OF UPDATED ARTICLES FROM PUBMED';
  const RELATED_ARTICLE_TEST = 'Related article test';

  /**
   * Lookup table for import disposition IDs.
   */
  private $dispositions = [];

  /**
   * Lookup table for import type IDs.
   */
  private $importTypes = [];

  /**
   * {@inheritdoc}
   */
  protected static $modules = [
    'ckeditor5',
    'datetime',
    'datetime_range',
    'editor',
    'ebms_article',
    'ebms_board',
    'ebms_core',
    'ebms_group',
    'ebms_import',
    'ebms_journal',
    'ebms_meeting',
    'ebms_message',
    'ebms_state',
    'ebms_topic',
    'field',
    'file',
    'filter',
    'linkit',
    'taxonomy',
    'text',
    'user',
  ];

  /**
   * {@inheritdoc}
   */
  protected function setUp(): void {

    // Install the tables and configuration we'll need.
    parent::setUp();
    $this->installConfig(['ebms_core']);
    $this->installEntitySchema('ebms_board');
    $this->installEntitySchema('ebms_topic');
    $this->installEntitySchema('ebms_article_tag');
    $this->installEntitySchema('ebms_article_topic');
    $this->installEntitySchema('ebms_article');
    $this->installEntitySchema('ebms_journal');
    $this->installEntitySchema('ebms_meeting');
    $this->installEntitySchema('ebms_message');
    $this->installEntitySchema('ebms_state');
    $this->installEntitySchema('ebms_import_batch');
    $this->installEntitySchema('taxonomy_term');
    $this->installEntitySchema('user');

    // Create the taxonomy terms needed by the tests.
    $states = [
      'ready_init_review' => 10,
      'reject_journal_title' => 20,
      'published' => 40,
      'passed_bm_review' => 50,
      'passed_full_review' => 60,
      'agenda_future_change' => 70,
      'on_agenda' => 80,
      'final_board_decision' => 90,
    ];
    foreach ($states as $key => $sequence) {
      Term::create([
        'vid' => 'states',
        'field_text_id' => $key,
        'name' => strtolower(str_replace('_', ' ', $key)),
        'field_sequence' => $sequence,
      ])->save();
    }
    $tags = [
      'i_core_journals' => 'Core journals search',
      'high_priority' => 'High priority',
      'i_fasttrack' => 'Import fast track',
      'i_specialsearch' => 'Import special search',
    ];
    foreach ($tags as $key => $name) {
      Term::create([
        'vid' => 'article_tags',
        'field_text_id' => $key,
        'name' => $name,
      ])->save();
    }
    $import_types = ['Regular', 'Fast-Track', 'Data', 'Special', 'Internal'];
    foreach ($import_types as $name) {
      $text_id = substr($name, 0, 1);
      $values = [
        'vid' => 'import_types',
        'name' => $name,
        'field_text_id' => $text_id,
      ];
      $term = Term::create($values);
      $term->save();
      $this->importTypes[$text_id] = $term->id();
    }
    $dispositions = [
      'Duplicate',
      'Error',
      'Imported',
      'Not Listed',
      'Review Ready',
      'Replaced',
      'Topic Added',
    ];
    foreach ($dispositions as $name) {
      $text_id = strtolower(str_replace(' ', '_', $name));
      $values = [
        'vid' => 'import_dispositions',
        'name' => $name,
        'field_text_id' => $text_id,
      ];
      $term = Term::create($values);
      $term->save();
      $this->dispositions[$text_id] = $term->id();
    }

    // Create a board, a couple of topics, and a meeting.
    $board = Board::create([
      'id' => 1,
      'name' => 'Test Board',
      'auto_imports' => TRUE,
    ]);
    $board->save();
    for ($i = 1; $i <= 2; $i++) {
      $values = [
        'id' => $i,
        'name' => "Topic $i",
        'board' => 1,
      ];
      Topic::create($values)->save();
    }
    Meeting::create(['id' => 1]);

    // No need to instatiate this for every batch.
    $entity_type_manager = $this->container->get('entity_type.manager');
    $this->storage = $entity_type_manager->getStorage('ebms_article');
  }

  /**
   * Find articles for a specific cancer type.
   *
   * @param string $cancer_type
   *   Which cancer type to use for finding articles.
   * @param int $year
   *   Which year of publication to search for.
   *
   * @return array
   *   List of PubMed IDs.
   */
  private function findArticles($cancer_type, $year = 2008) {
    usleep(500000);
    static $fetched = [];
    $url = Batch::EUTILS . '/esearch.fcgi';
    $parms = "db=pubmed&term=science[journal]+AND+$cancer_type+cancer+AND+{$year}[pdat]";
    $ch = Batch::getCurlHandle($parms, $url);
    $response = curl_exec($ch);
    $code = curl_getinfo($ch, CURLINFO_RESPONSE_CODE);
    $this->assertEquals(200, $code);
    curl_close($ch);
    usleep(500000);
    $root = new \SimpleXMLElement($response);
    $this->assertEquals('eSearchResult', $root->getName());
    $count = (int) $root->Count;
    $pmids = [];
    foreach ($root->IdList->Id as $id) {
      if (!empty($id)) {
        $pmids[] = trim($id ?? '');
      }
    }
    $this->assertNotEmpty($pmids);
    $this->assertCount($count, $pmids);
    $pmids = array_diff($pmids, $fetched);
    $this->assertNotEmpty($pmids);
    $fetched = array_merge($fetched, $pmids);
    return $pmids;
  }

  /**
   * Submit the import batch request and verify the results.
   *
   * @param array $request
   *   Values in the import request.
   *
   * @return Batch
   */
  private function checkBatch($request): Batch {

    // Check the batch-level values.
    $batch = Batch::process($request);
    $actions = [];
    foreach ($batch->actions as $action) {
      $actions[$action->article][] = $action;
    }
    $this->assertTrue($batch->success->value);
    $expected_import_type = Batch::IMPORT_TYPE_REGULAR;
    $comment = $request['import-comments'] ?? '';
    if ($comment === self::INTERNAL_IMPORT_TEST) {
      $expected_import_type = Batch::IMPORT_TYPE_INTERNAL;
    }
    elseif (empty($request['topic'])) {
      $expected_import_type = Batch::IMPORT_TYPE_DATA_REFRESH;
    }
    elseif (!empty($request['fast-track'])) {
      $expected_import_type = Batch::IMPORT_TYPE_FAST_TRACK;
    }
    $this->assertEquals($expected_import_type, $batch->import_type->entity->field_text_id->value);
    $this->assertCount($batch->article_count->value, $request['article-ids']);
    if (empty($request['topic'])) {
      $this->assertEmpty($batch->target);
      $this->assertEmpty($batch->cycle->value);
    }
    else {
      $this->assertEquals($request['topic'], $batch->topic->target_id);
      $this->assertEquals($request['cycle'], $batch->cycle->value);
    }
    if (!empty($comment)) {
      $this->assertEquals($comment, $batch->comment->value);
    }

    // Determine expected values for all articles.
    $expected_states = ['ready_init_review'];
    if (!empty($request['core-journals-search'])) {
      $expected_states[] = 'published';
    }
    if ($comment === self::NOT_LIST_TEST) {
      $expected_states[] = 'reject_journal_title';
    }
    if (!empty($request['fast-track'])) {
      $expected_states[] = 'published';
      $placement = $request['placement'];
      if ($placement === 'bma') {
        $placement = $request['bma-disposition'];
      }
      $expected_states[] = $placement;
    }
    $expected_tags = [];
    if (empty($request['topic'])) {
      $expected_states = [];
    }
    else {
      foreach (self::TAG_MAP as $key => $tag) {
        if (!empty($request[$key])) {
          $expected_tags[] = $tag;
        }
      }
    }

    // Check each of the individual imported articles.
    foreach ($request['article-ids'] as $pmid) {
      $article = Article::getArticleBySourceId('Pubmed', $pmid);
      $this->assertNotEmpty($article);
      if (empty($request['topic'])) {
        //if ($comment !== self::IMPORT_REFRESH_TEST) {
          $this->assertEmpty($article->topics);
        //}
      }
      else {
        $this->assertCount($request['topic'], $article->topics);
        // $article_topic = $article->topics[0]->entity;
        $article_topic = $article->getTopic($request['topic']);
        $this->assertNotEmpty($article_topic);
        $this->assertEquals($request['topic'], $article_topic->topic->target_id);
        if (!empty($request['mgr-comment'])) {
          $this->assertCount(1, $article_topic->comments);
          $this->assertEquals($request['mgr-comment'], $article_topic->comments[0]->comment);
        }
        else {
          $this->assertEmpty($article_topic->comments);
        }
        $this->assertCount(count($expected_tags), $article_topic->tags);
        if (!empty($expected_tags)) {
          $tags = [];
          foreach ($article_topic->tags as $tag) {
            $tags[] = $tag->entity->tag->entity->field_text_id->value;
          }
          foreach ($expected_tags as $tag) {
            $this->assertContains($tag, $tags);
          }
        }
      }
      if (!empty($expected_states)) {
        $this->assertCount(count($expected_states), $article_topic->states);
      }
      foreach ($expected_states as $i => $text_id) {
        $last_state = $i === count($expected_states) - 1;
        $state = $article_topic->states[$i]->entity;
        $this->assertEquals($text_id, $state->value->entity->field_text_id->value);
        $this->assertEquals($request['board'], $state->board->target_id);
        $this->assertEquals($request['topic'], $state->topic->target_id);
        $this->assertNotEmpty($state->active->value);
        if ($last_state) {
          $this->assertNotEmpty($state->current->value);
        }
        else {
          $this->assertEmpty($state->current->value);
        }
        if ($i === 0 && !empty($comment)) {
          $this->assertCount(1, $state->comments);
          if ($comment === self::DUPLICATE_IMPORT_TEST) {
            $this->assertEquals(self::ORIGINAL_IMPORT_COMMENT, $state->comments->body);
          }
          else {
            $this->assertEquals($comment, $state->comments->body);
          }
        }
        elseif ($text_id === 'published') {
          $this->assertCount(1, $state->comments);
        }
        elseif (!empty($request['fast-track']) && !empty($request['fast-track-comments']) && $last_state) {
          $this->assertCount(1, $state->comments);
          $this->assertEquals($request['fast-track-comments'], $state->comments->body);
        }
        elseif ($comment === self::NOT_LIST_TEST) {
          $this->assertCount(1, $state->comments);
          $this->assertEquals($comment, $state->comments->body);
        }
        else {
          $this->assertEmpty($state->comments);
        }
        if ($text_id === 'final_board_decision') {
          $this->assertCount(1, $state->decisions);
          $this->assertEquals($request['disposition'], $state->decisions[0]->decision);
        }
        else {
          $this->assertEmpty($state->decisions);
        }
        $this->assertEmpty($state->deciders);
        if ($text_id === 'on_agenda' && !empty($request['meeting'])) {
          $this->assertCount(1, $state->meetings);
          $this->assertEquals($request['meeting'], $state->meetings[0]->target_id);
        }
        else {
          $this->assertEmpty($state->meetings);
        }
      }
      foreach ($actions[$article->id()] as $action) {
        $this->assertEquals($action->source_id, $pmid);
      }
      if ($comment === self::DUPLICATE_IMPORT_TEST) {
        $this->assertCount(1, $actions[$article->id()]);
        $this->assertEquals($this->dispositions['duplicate'], $actions[$article->id()][0]->disposition);
      }
      elseif ($comment === self::INTERNAL_IMPORT_TEST) {
        $this->assertCount(1, $actions[$article->id()]);
        $this->assertEquals($this->dispositions['imported'], $actions[$article->id()][0]->disposition);
      }
      elseif (!empty($request['topic']) && $request['topic'] == 2) {
        $this->assertCount(2, $actions[$article->id()]);
        $this->assertEquals($this->dispositions['topic_added'], $actions[$article->id()][0]->disposition);
        $this->assertEquals($this->dispositions['review_ready'], $actions[$article->id()][1]->disposition);
      }
      elseif ($comment === self::NOT_LIST_TEST) {
        $this->assertCount(3, $actions[$article->id()]);
        $this->assertEquals($this->dispositions['imported'], $actions[$article->id()][0]->disposition);
        $this->assertEquals($this->dispositions['review_ready'], $actions[$article->id()][1]->disposition);
        $this->assertEquals($this->dispositions['not_listed'], $actions[$article->id()][2]->disposition);
      }
      elseif ($comment === self::IMPORT_REFRESH_TEST) {
        $this->assertCount(2, $actions[$article->id()]);
        $this->assertEquals($this->dispositions['replaced'], $actions[$article->id()][0]->disposition);
        $this->assertEquals($this->dispositions['duplicate'], $actions[$article->id()][1]->disposition);
      }
      elseif (empty($request['topic'])) {
        $this->assertCount(1, $actions[$article->id()]);
        $this->assertEquals($this->dispositions['duplicate'], $actions[$article->id()][0]->disposition);
      }
      else {
        $this->assertCount(2, $actions[$article->id()]);
        $this->assertEquals($this->dispositions['imported'], $actions[$article->id()][0]->disposition);
        $this->assertEquals($this->dispositions['review_ready'], $actions[$article->id()][1]->disposition);
      }
    }
    return $batch;
  }

  /**
   * Test import of articles from PubMed.
   */
  public function testImport() {

    // First try a bare-bones regular import batch.
    $request = [
      'article-ids' => $this->findArticles('liver'),
      'board' => 1,
      'topic' => 1,
      'cycle' => '2010-01-01',
    ];
    $this->checkBatch($request);

    // Add some bells and whistles (comments, tags, etc.).
    $import_comment = 'Yada, yada';
    $topic_comment = 'Dada';
    $pmids = $this->findArticles('lung');
    $request['article-ids'] = $pmids;
    $request['import-comments'] = $import_comment;
    $request['mgr-comment'] = $topic_comment;
    $request['special-search'] = TRUE;
    $request['core-journals-search'] = TRUE;
    $request['hi-priority'] = TRUE;
    $this->checkBatch($request);

    // Add a second topic to the same documents.
    $request['topic'] = 2;
    $this->checkBatch($request);

    // Test a batch setting a meeting for the "on agenda" state.
    $request = [
      'article-ids' => $this->findArticles('breast'),
      'board' => 1,
      'topic' => 1,
      'cycle' => '2020-01-01',
      'fast-track' => TRUE,
      'fast-track-comments' => "Let's get this baby on the road!",
      'placement' => 'on_agenda',
      'meeting' => 1,
    ];
    $this->checkBatch($request);

    // Test a batch setting a disposition for the "final_board_decision" state.
    $decision = Term::create(['vid' => 'board_decisions', 'name' => 'Cited']);
    $decision->save();
    $request['article-ids'] = $this->findArticles('brain');
    $request['placement'] = 'final_board_decision';
    $request['disposition'] = $decision->id();
    unset($request['meeting']);
    $this->checkBatch($request);

    // Test with a "Board Manager Action" state.
    $request['article-ids'] = $this->findArticles('skin');
    $request['placement'] = 'bma';
    $request['bma-disposition'] = 'agenda_future_change';
    $request['import-comments'] = self::ORIGINAL_IMPORT_COMMENT;
    unset($request['disposition']);
    $this->checkBatch($request);

    // Test a duplicate import.
    $request['import-comments'] = self::DUPLICATE_IMPORT_TEST;
    $this->checkBatch($request);

    // Test the "not-list" mechanism.
    $pmids = $this->findArticles('prostate');
    $ch = Batch::getCurlHandle(Batch::PARMS . reset($pmids));
    $response = curl_exec($ch);
    $code = curl_getinfo($ch, CURLINFO_RESPONSE_CODE);
    $this->assertEquals(200, $code);
    curl_close($ch);
    usleep(500000);
    $root = new \SimpleXMLElement($response);
    $journal_id = trim($root->PubmedArticle->MedlineCitation->MedlineJournalInfo->NlmUniqueID);
    $user = User::create(['name' => 'Joe Notlist']);
    $user->save();
    $values = [
      'source' => 'Pubmed',
      'source_id' => $journal_id,
      'title' => 'Science',
      'brief_title' => 'Science',
      'not_lists' => [
        'board' => 1,
        'start' => '2000-01-01',
        'user' => $user->id(),
      ],
    ];
    Journal::create($values)->save();
    $this->checkBatch([
      'article-ids' => $pmids,
      'board' => 1,
      'topic' => 1,
      'cycle' => '2020-01-01',
      'import-comments' => self::NOT_LIST_TEST,
    ]);

    // Now suppress the "not-list" filter.
    $pmids = $this->findArticles('uterine');
    $this->checkBatch([
      'article-ids' => $pmids,
      'board' => 1,
      'topic' => 1,
      'cycle' => '2020-01-01',
      'override-not-list' => TRUE,
    ]);

    // Try a data refresh import, forcing data change.
    $pmids = $this->findArticles('breast', 2009);
    foreach ($pmids as $pmid) {
      Article::create([
        'source' => 'Pubmed',
        'source_id' => $pmid,
        'title' => 'Original title',
      ])->save();
    }
    $this->checkBatch([
      'article-ids' => $pmids,
      'import-comments' => self::IMPORT_REFRESH_TEST,
    ]);

    // Refresh again, but this time with no changes.
    $this->checkBatch(['article-ids' => $pmids]);

    // Test an internal import.
    $this->checkBatch([
      'article-ids' => $this->findArticles('pancreatic'),
      'import-type' => $this->importTypes[Batch::IMPORT_TYPE_INTERNAL],
      'import-comments' => self::INTERNAL_IMPORT_TEST,
    ]);
  }

  /**
   * Make sure a regular import without a topic fails.
   *
   * @return void
   */
  public function testMissingTopic() {
    $this->expectException('Exception');
    Batch::process([
      'article-ids' => ['12345678'],
      'import-type' => $this->importTypes[Batch::IMPORT_TYPE_REGULAR],
      'board' => 1,
      'cycle' => '2000-01-01',
    ]);
  }

  /**
   * Make sure a regular import without a cycle fails.
   *
   * @return void
   */
  public function testMissingCycle() {
    $this->expectException('Exception');
    Batch::process([
      'article-ids' => ['12345678'],
      'import-type' => $this->importTypes[Batch::IMPORT_TYPE_REGULAR],
      'board' => 1,
      'topic' => 1,
    ]);
  }

  /**
   * Test identification of related articles.
   *
   * As often happens, the test is doing more work than the software being
   * tested. Picking arbitrary PubMed IDs to test would leave us at the
   * mercy of NLM, who might eliminate one of the articles, or assign one
   * of them a different ID (it happens), which would break our test. So
   * we walk through a set of candidate pairs of articles to find one which
   * involves only a single article related to the primary article we will
   * import, each of which are from the same journal. And them we make sure
   * the journal is marked as a "core" journal (otherwise the related article
   * will be skipped).
   */
  public function testRelated() {

    // Find some article retractions in the journal Science.
    $url = Batch::EUTILS . '/esearch.fcgi';
    $terms = [
      'science[journal]'.
      'retraction+of+publication[Publication+Type]',
    ];
    $parms = 'db=pubmed&term=' . implode('+AND+', $terms);
    $ch = Batch::getCurlHandle($parms, $url);
    $response = curl_exec($ch);
    $code = curl_getinfo($ch, CURLINFO_RESPONSE_CODE);
    $this->assertEquals(200, $code);
    curl_close($ch);
    usleep(500000);
    $root = new \SimpleXMLElement($response);
    $this->assertEquals('eSearchResult', $root->getName());
    $pmids = [];
    foreach ($root->IdList->Id as $id) {
      $id = trim($id ?? '');
      if (!empty($id)) {
        $pmids[] = $id;
      }
    }
    $this->assertNotEmpty($pmids);

    // Fetch the article XML.
    $parms = Batch::PARMS . implode(',', $pmids);
    $ch = Batch::getCurlHandle($parms);
    $response = curl_exec($ch);
    $code = curl_getinfo($ch, CURLINFO_RESPONSE_CODE);
    $this->assertEquals(200, $code);
    curl_close($ch);
    usleep(500000);

    // Pick the first pair which has only one related article.
    $pair = [];
    $articles = Batch::split($response);
    $science_prefix = 'Science.';
    $science_len = strlen($science_prefix);
    $journal_id = '';
    foreach ($articles as $xml) {
      $root = new \SimpleXMLElement($xml);
      $this->assertEquals('PubmedArticle', $root->getName());
      $citation = $root->MedlineCitation;
      if (!empty($citation)) {
        $journal_id = trim($citation->MedlineJournalInfo->NlmUniqueID);
        $pmid = trim($citation->PMID ?? '');
        if (!empty($pmid) && !empty($journal_id)) {
          $journal_ta = trim($citation->MedlineJournalInfo->MedlineTA ?? '');
          if ($journal_ta === 'Science') {
            if (!empty($citation->CommentsCorrectionsList->CommentsCorrections)) {
              $matches = [];
              foreach ($citation->CommentsCorrectionsList->CommentsCorrections as $cc) {
                $cc_pmid = trim($cc->PMID ?? '');
                if (!empty($cc_pmid)) {
                  $ref_source = substr(trim($cc->RefSource ?? ''), 0, $science_len);
                  if ($ref_source === $science_prefix) {
                    $matches[] = $cc_pmid;
                  }
                }
              }
              if (!empty($pmid) && count($matches) === 1) {
                $pair = [$pmid, $matches[0]];
                break;
              }
            }
          }
        }
      }
    }
    $this->assertNotEmpty($pair);

    // Tell the system that the journal is a core journal.
    $values = [
      'source' => 'Pubmed',
      'source_id' => $journal_id,
      'title' => 'Science',
      'brief_title' => 'Science',
      'core' => TRUE,
    ];
    Journal::create($values)->save();

    // Run the import and check for the related ID.
    list($pmid, $related) = $pair;
    $batch = $this->checkBatch([
      'article-ids' => [$pmid],
      'board' => 1,
      'topic' => 1,
      'cycle' => '2015-01-01',
      'import-comments' => self::RELATED_ARTICLE_TEST,
    ]);
    $followup = $batch->getFollowup();
    $this->assertCount(1, $followup);
    $this->assertEquals($related, $followup[0]);
  }

}
