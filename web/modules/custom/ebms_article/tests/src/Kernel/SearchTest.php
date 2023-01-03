<?php

namespace Drupal\Tests\ebms_article\Kernel;

use Drupal\KernelTests\KernelTestBase;
use Drupal\ebms_article\Entity\Article;
use Drupal\ebms_article\Entity\ArticleTopic;
use Drupal\ebms_article\Search;
use Drupal\ebms_board\Entity\Board;
use Drupal\ebms_journal\Entity\Journal;
use Drupal\ebms_review\Entity\Packet;
use Drupal\ebms_review\Entity\PacketArticle;
use Drupal\ebms_review\Entity\Review;
use Drupal\ebms_topic\Entity\Topic;
use Drupal\ebms_meeting\Entity\Meeting;
use Drupal\taxonomy\Entity\Term;
use Drupal\user\Entity\User;
use Symfony\Component\Yaml\Yaml;

/**
 * Test the article type.
 *
 * @group ebms
 */
class SearchTest extends KernelTestBase {

  /**
   * The search service being tested.
   *
   * @var \Drupal\ebms_article\Search|null
   */
  protected ?Search $service;

  /**
   * Dummy `Board` entities created for testing.
   *
   * @var array|null
   */
  protected ?array $boards;

  /**
   * Dummy `Topic` entities created for testing.
   *
   * @var array|null
   */
  protected ?array $topics;

  /**
   * State `states` terms created for testing.
   *
   * @var array|null
   */
  protected ?array $states;

  /**
   * Test values loaded from YAML file.
   *
   * @var array|null
   */
  protected ?array $tests;

  /**
   * {@inheritdoc}
   */
  protected static $modules = [
    'ckeditor5',
    'datetime_range',
    'editor',
    'ebms_article',
    'ebms_board',
    'ebms_core',
    'ebms_doc',
    'ebms_group',
    'ebms_journal',
    'ebms_meeting',
    'ebms_message',
    'ebms_review',
    'ebms_state',
    'ebms_topic',
    'field',
    'file',
    'filter',
    'linkit',
    'user',
    'datetime',
    'system',
    'taxonomy',
    'text',
  ];

  /**
   * {@inheritdoc}
   */
  protected function setUp(): void {
    parent::setUp();

    $installer = $this->container->get('module_installer');
    $installer->install(['ebms_core', 'taxonomy']);
    $this->installConfig(['ebms_core']);
    $this->installConfig(['ebms_article']);
    $this->installEntitySchema('ebms_article');
    $this->installEntitySchema('ebms_article_tag');
    $this->installEntitySchema('ebms_article_topic');
    $this->installEntitySchema('ebms_board');
    $this->installEntitySchema('ebms_journal');
    $this->installEntitySchema('ebms_meeting');
    $this->installEntitySchema('ebms_message');
    $this->installEntitySchema('ebms_packet');
    $this->installEntitySchema('ebms_packet_article');
    $this->installEntitySchema('ebms_review');
    $this->installEntitySchema('ebms_state');
    $this->installEntitySchema('ebms_topic');
    $this->installEntitySchema('taxonomy_term');
    $this->installEntitySchema('user');
    $this->installSchema('system', ['sequences']);
    $this->enableModules(['ebms_article']);
    $this->service = $this->container->get('ebms_article.search');
    $module = $this->container->get('extension.list.module')->getPath('ebms_article');
    $data = Yaml::parseFile("$module/tests/config/search-test-data.yml");
    foreach ($data['entities']['states'] as $values) {
      $state = Term::create($values);
      $state->save();
      $this->states[] = $state;
    }
    foreach ($data['entities']['boards'] as $values) {
      $board = Board::create($values);
      $board->save();
      $this->boards[] = $board;
    }
    foreach ($data['entities']['topics'] as $values) {
      $topic = Topic::create($values);
      $topic->save();
      $this->topics[] = $topic;
    }
    $this->tests = $data['tests'];
  }

  /**
   * Test searching by agenda.
   */
  public function testAgendaSearch() {

    // Create some meeting categories.
    foreach ($this->tests['agendas']['meeting-categories'] as $values) {
      Term::create($values)->save();
    }

    // Create a test article.
    $article_values = $this->tests['agendas']['article'];
    $article = Article::create(['id' => $article_values['id']]);
    $state = $article->addState('on_agenda', $article_values['topic']);

    // Create a meeting and attach it to the state entity.
    $meeting = Meeting::create($this->tests['agendas']['meeting']);
    $meeting->save();
    $state->meetings[] = $meeting->id();
    $state->save();

    // Add a state for another, decoy topic, and save the article.
    $article->addState('on_agenda', 2);
    $article->save();

    // Test some meeting searches.
    $this->trySearches('agendas');
  }

  /**
   * Test searching by article reviewer.
   */
  public function testArticleReviewerSearch() {

    // Create some reviewers.
    foreach ($this->tests['article-reviewers']['users'] as $values) {
      User::create($values)->save();
    }

    // Create some articles.
    foreach ($this->tests['article-reviewers']['articles'] as $values) {
      $article = Article::create(['id' => $values['id']]);
      $article->addState('passed_full_review', $values['topic']);
      $article->save();
    }

    // Create some reviews.
    foreach ($this->tests['article-reviewers']['reviews'] as $values) {
      Review::create($values)->save();
    }

    // Create some review packets.
    foreach ($this->tests['article-reviewers']['packets'] as $packet) {
      $articles = [];
      foreach ($packet['articles'] as $values) {
        $articles[] = $values['id'];
        PacketArticle::create($values)->save();
      }
      $packet['articles'] = $articles;
      Packet::create($packet)->save();
    }

    // Test some searches.
    $this->trySearches('article-reviewers');
  }

  /**
   * Test searching for article titles.
   */
  public function testArticleTitleSearch() {
    $this->runTests('article-titles');
  }

  /**
   * Test searching by author names.
   */
  public function testAuthorSearch() {
    $this->runTests('authors');
  }

  /**
   * Test searching for articles by comments.
   */
  public function testCommentSearch() {

    // Create some articles.
    User::create(['id' => 1]);
    foreach ($this->tests['comments']['articles'] as $values) {
      $article = Article::create(['id' => $values['id']]);
      foreach ($values['topics'] as $topic_values) {
        $topic_id = $topic_values['topic'];
        if (!empty($topic_values['comments'])) {
          list($comment, $when) = $topic_values['comments'][0];
          $state = $article->addState('published', $topic_id, NULL, $when, NULL, $comment);
          if (count($topic_values['comments']) > 1) {
            $extra_comments = array_slice($topic_values['comments'], 1);
            foreach ($extra_comments as list($comment, $when)) {
              $state->comments[] = [
                'body' => $comment,
                'user' => 1,
                'entered' => $when,
              ];
            }
            $state->save();
          }
        }
        else {
          $article->addState('published', $topic_id);
          if (!empty($topic_values['topic-comment'])) {
            foreach ($article->topics as $article_topic) {
              $article_topic = $article_topic->entity;
              if ($article_topic->topic->target_id == $topic_id) {
                $article_topic->comments[] = $topic_values['topic-comment'];
              }
              $article_topic->save();
              break;
            }
          }
        }
      }
      $article->save();
    }

    // Try some test searches.
    $this->trySearches('comments');
  }

  /**
   * Test search by review cycle.
   */
  public function testCycleSearch() {

    // Create some test articles.
    foreach ($this->tests['cycles']['articles'] as $values) {
      $article = Article::create(['id' => $values['id']]);
      foreach ($values['topics'] as $topic) {
        $article->addState('published', $topic['id'], NULL, NULL, $topic['cycle']);
      }
      $article->save();
    }

    // Test the searches.
    $this->trySearches('cycles');
  }

  /**
   * Test searching by final article decision.
   */
  public function testDecisionSearch() {

    // Create some decisions.
    foreach ($this->tests['decisions']['terms'] as $values) {
      Term::create($values)->save();
    }

    // Create an article with the final board decision state.
    $values = $this->tests['decisions']['article'];
    $article = Article::create(['id' => $values['id']]);
    $state = $article->addState($values['state'], $values['topic']);
    $state->decisions[] = $values['decision'];
    $state->save();
    $article->save();

    // Try some searches.
    $this->trySearches('decisions');
  }

  /**
   * Test searching based on the presence of the articles' full text PDF.
   */
  public function testFullTextRetrievalSearch() {
    $this->runTests('full-text');
  }

  /**
   * Test searching by EBMS article ID or PubMed ID.
   */
  public function testIdSearch() {
    $this->runTests('ids');
  }

  /**
   * Test searching by the date the articles were first imported.
   */
  public function testImportDateSearch() {
    $this->runTests('import-dates');
  }

  /**
   * Make sure that internal articles are not included in search results.
   */
  public function testInternalArticleExclusion() {

    // Create an article with no topics and verify that it's not found..
    $article = Article::create();
    $query = $this->service->buildQuery([
      'filters' => ['unpublished' => TRUE],
    ]);
    $this->assertEmpty($query->execute());

    // Add a topic and verify that the article gets picked up.
    $article_topic = ArticleTopic::create([
      'topic' => $this->topics[0]->id(),
      'cycle' => '2000-01-01',
    ]);
    $article_topic->save();
    $article->topics[] = $article_topic->id();
    $article->save();
    $query = $this->service->buildQuery([
      'filters' => ['unpublished' => TRUE],
    ]);
    $this->assertCount(1, $query->execute());
  }

  /**
   * Test journal searching.
   */
  public function testJournalSearch() {

    // Create some journal entities.
    foreach ($this->tests['journals']['values'] as $values) {
      Journal::create($values)->save();
    }

    // Create some test articles and run some searches against them.
    $this->runTests('journals');
  }

  /**
   * Find articles modified by the users during a specified date range.
   */
  public function testModificationDateSearch() {

    // Create some tags.
    foreach ($this->tests['modification-dates']['tags'] as $values) {
      Term::create($values)->save();
    }

    // Create some articles with different kinds of date changes.
    $cycle = '1999-12-01';
    foreach ($this->tests['modification-dates']['articles'] as $id => $date) {
      $article = Article::create(['id' => $id]);
      switch ($id) {

        // State change.
        case 1:
        case 7:
          $article->addState('published', 1, 0, $date, $cycle);
          break;

        // Comment added to state.
        case 2:
          $state = $article->addState('published', 1, 0, '1999-12-31', $cycle);
          $state->addComment('Test', $date);
          $state->save();
          break;

        // Tag added to article.
        case 3:
          $article->addState('published', 1, 0, '1999-12-31', $cycle);
          $article->addTag('ponies', 0, 0, $date);
          break;

        // Comment added to article tag.
        case 4:
          $article->addState('published', 1, 0, '1999-12-31', $cycle);
          $tag = $article->addTag('puppies', 0, 0, '1999-12-31');
          $tag->addComment('Test', $date);
          $tag->save();
          break;

        // Tag added to article topic.
        case 5:
          $article->addState('published', 1, 0, '1999-12-31', $cycle);
          $article->addTag('ponies', 1, 0, $date);
          break;

        // Comment added to topic-specific tag.
        case 6:
          $article->addState('published', 1, 0, '1999-12-31', $cycle);
          $tag = $article->addTag('ponies', 1, 0, '1999-12-31');
          $tag->addComment('Test', $date);
          $tag->save();
          break;
      }
      $article->save();
    }

    // Run some test searches.
    $this->trySearches('modification-dates');
  }

  /**
   * Test searching by publication year and month.
   */
  public function testPubDateSearch() {
    $this->runTests('publication-dates');
  }

  /**
   * Search by assessments made of articles by the board member reviewers.
   */
  public function testReviewerResponseSearch() {

    // Create some user accounts for the reviewers.
    foreach ($this->tests['responses']['users'] as $values) {
      User::create($values)->save();
    }

    // Create some articles we can assign for review.
    foreach ($this->tests['responses']['articles'] as $values) {
      $article = Article::create(['id' => $values['id']]);
      $article->addState('passed_full_review', $values['topic']);
      $article->save();
    }

    // Create the review response ("disposition") term values.
    foreach ($this->tests['responses']['dispositions'] as $values) {
      Term::create($values)->save();
    }

    // Create some reviews.
    foreach ($this->tests['responses']['reviews'] as $values) {
      Review::create($values)->save();
    }

    // Create the packet-article assignments.
    foreach ($this->tests['responses']['packet-articles'] as $values) {
      PacketArticle::create($values)->save();
    }

    // Create the review assignment packets.
    foreach ($this->tests['responses']['packets'] as $values) {
      Packet::create($values)->save();
    }

    // Run some searches.
    $this->trySearches('responses');
  }

  /**
   * Verify that the sorting of search results is as requested.
   */
  public function testSort() {
    $this->runTests('sorts', FALSE);
  }

  /**
   * Test searching by states.
   */
  public function testStateSearch() {

    // Create some test articles. This test goes to 11!
    foreach ($this->tests['states']['articles'] as $id => $states) {
      $article = Article::create(['id' => $id]);
      foreach ($states as $values) {
        $article->addState($values['state'], $values['topic']);
      }
      $article->save();
    }

    // Take them out for a spin.
    $this->trySearches('states');
  }

  /**
   * Test searching by tag (with or without topics).
   */
  public function testTagSearch() {

    // Create some tags.
    foreach ($this->tests['tags']['values'] as $values) {
      Term::create($values)->save();
    }

    // Create some articles using those tags.
    foreach ($this->tests['tags']['articles'] as $id => $tag) {
      $article = Article::create(['id' => $id]);
      $article->addState('published', 1);
      $topic = $tag['topic-specific'] ? 1 : 0;
      $article->addTag($tag['name'], $topic, 0, $tag['date']);
      $article->save();
    }

    // Try some tag searches.
    $this->trySearches('tags');
  }

  /**
   * Test searching by topic or board.
   */
  public function testTopicOrBoardSearch() {

    // Create some test articles.
    foreach ($this->tests['topics-and-boards']['articles'] as $id => $topics) {
      $article = Article::create(['id' => $id]);
      foreach ($topics as $topic) {
        $article->addState('published', $topic);
      }
      $article->save();
    }

    // Run some test cases.
    $this->trySearches('topics-and-boards');
  }

  /**
   * Create a set of articles and set them to the 'published' state.
   *
   * This will work for many of the tests, though some will require custom
   * processing for the test articles. We chose the 'published' state because
   * by default the search engine ignores articles which do not have at least
   * one topic in that state.
   *
   * @param string $name
   *   Key to the values needed for a particular set of tests.
   */
  private function createArticles(string $name) {
    foreach ($this->tests[$name]['articles'] as $values) {
      $article = Article::create($values);
      $article->addState('published', 1);
      $article->save();
    }
  }

  /**
   * Run a set of test searches and verify that the results are correct.
   *
   * @param string $name
   *   Key to the values needed for a particular set of tests.
   * @param bool $sorted
   *   If `TRUE` (the default), sort the result set before comparison.
   * @param bool $debug
   *   If `TRUE`, show the query when a test fails.
   */
  private function trySearches(string $name, bool $sorted = TRUE, bool $debug = FALSE) {
    foreach ($this->tests[$name]['searches'] as $search) {
      $query = $this->service->buildQuery($search['parms']);
      $ids = array_values($query->execute());
      if ($sorted) {
        sort($ids);
      }
      $message = $search['message'];
      if ($debug) {
        $message .= "\nQUERY:\n" . $query . "\n";
      }
      $this->assertEquals($search['expected'], $ids, $message);
    }
  }

  /**
   * Create some test articles and run a set of searches against them.
   *
   * @param string $name
   *   Key to the values needed for a particular set of tests.
   * @param bool $sorted
   *   If `TRUE` (the default), sort the result set before comparison.
   * @param bool $debug
   *   If `TRUE`, show the query when a test fails.
   */
  private function runTests(string $name, bool $sorted = TRUE, bool $debug = FALSE) {
    $this->createArticles($name);
    $this->trySearches($name, $sorted, $debug);
  }

}
