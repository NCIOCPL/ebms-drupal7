<?php /** @noinspection ALL */

namespace Drupal\ebms_article;

use Drupal\Core\Entity\EntityTypeManagerInterface;
use Drupal\Core\Database\Connection;
use Drupal\ebms_article\Entity\Article;

class SearchQuery {

  /**
   * The taxonomy_term entity storage handler.
   *
   * @var \Drupal\Core\Entity\EntityStorageInterface
   */
  protected $articleStorage;

  /**
   * The entity type manager.
   *
   * @var EntityTypeManagerInterface
   */
  protected $typeManager;

  /**
   * Flag indicating whether we need to filter on boards.
   *
   * @var bool
   */
  protected $needBoards;

  /**
   * Board IDs (needed in more than one place).
   *
   * @var array
   */
  protected $boards = [];

  /**
   * Topic IDs (needed in more than one place).
   *
   * @var array
   */
  protected $topics = [];

  /**
   * Connection to the SQL database.
   *
   * @var Connection
   */
  protected $db = NULL;

  /**
   * Parameters used for building the search query.
   *
   * @var array
   */
  protected $parms;

  /**
   * The query we are building.
   *
   * @var \Drupal\Core\Entity\Query\QueryInterface
   */
  protected $query;

  /**
   * Flag indicating whether we have applied filtering by states.
   *
   * @var bool
   */
  protected $stateFilters = FALSE;

  /**
   * Lookup table of state information from text IDs.
   *
   * @var array
   */
  protected $stateMap = [];

  /**
   * States which are under the librarians' control.
   *
   * @var array
   */
  protected $earlyStates = [];

  /**
   * States which follow the "published" state.
   *
   * @var array
   */
  protected $laterStates = [];

  /**
   * Is this a full or restricted search?
   *
   * @var bool
   */
  private $restricted = FALSE;

  /**
   * Constructs a new \Drupal\ebms_core\TermLookup object.
   *
   * @param EntityTypeManagerInterface $entity_type_manager
   *   The entity type manager.
   * @param Connection $db
   *   Connection to the SQL database.
   * @param array $parms
   *   Values used for building the search query.
   */
  public function __construct(EntityTypeManagerInterface $entity_type_manager, Connection $db, array $parms) {
    $this->typeManager = $entity_type_manager;
    $this->articleStorage = $entity_type_manager->getStorage('ebms_article');
    $this->db = $db;
    $this->parms = $parms;
    $this->query = $this->articleStorage->getQuery()->accessCheck(FALSE);
    if (!empty($parms['boards'])) {
      $this->boards = $parms['boards'];
    }
    if (!empty($parms['topics'])) {
      $this->topics = $parms['topics'];
    }
    $this->restricted = !empty($parms['restricted']);
    if (!$this->restricted) {
      $storage = $entity_type_manager->getStorage('taxonomy_term');
      $query = $storage->getQuery()->accessCheck(FALSE);
      $query->condition('vid', 'states');
      $states = $storage->loadMultiple($query->execute());
      foreach ($states as $state) {
        $this->stateMap[$state->field_text_id->value] = (object) [
          'id' => $state->id(),
          'sequence' => $state->field_sequence->value,
        ];
      }
    }
    $this->needBoards = $this->needBoardSearch();
  }

  /**
   * Apply all of the requested search criteria.
   *
   * Some of the later criteria filters depend on work done in previous
   * filters.  Some will check the results of earlier filters and short
   * circuit tests that are already done as side effects of earlier
   * filters. Do not change the order of filters without considering this
   * issue.
   *
   * @return \Drupal\Core\Entity\Query\QueryInterface
   *   Query constructed from the user's search criteria.
   */
  public function build(): object {

    $this->searchIds();
    $this->searchStates();
    $this->searchTopicsOrBoards();
    $this->searchAuthors();
    $this->searchArticleTitle();
    $this->searchFullTextRetrieved();
    $this->searchJournal();
    $this->searchPubDate();
    $this->searchCycle();
    $this->searchArticleReviewer();
    $this->searchReviewerResponse();
    $this->searchComments();
    $this->searchTags();
    $this->searchImportDate();
    $this->searchModifiedDate();
    $this->searchOnAgenda();
    $this->searchDecision();
    $this->excludeInternalArticles();

    // Add sorting and return the query.
    $this->searchOrder();
    return $this->query;
  }

  /**
   * Search unique IDs.
   */
  private function searchIds() {
    $pmid = trim($this->parms['pmid'] ?? '');
    if (!empty($pmid)) {
      $this->query->condition('source', 'Pubmed');
      $this->query->condition('source_id', $pmid);
    }
    if (!$this->restricted) {
      $ebms_id = trim($this->parms['ebms_id'] ?? '');
      if (!empty($ebms_id)) {
        $this->query->condition('id', $ebms_id);
      }
    }
  }

  /**
   * Apply conditions to narrow the search by article states.
   *
   * The default behavior returns all articles which have reached the
   * Published state.  When the user identifies states later than Published,
   * we narrow the search to include only articles which have all of these
   * states, and we don't have to bother checking for the Published state
   * (because the presence of any of the later states guarantees that the
   * Published state will be present as well).
   *
   * If no states later than Published are specified, but the user checks any
   * of the boxes to include articles which have states earlier than
   * Published, even if they don't also have the Published state, we broaden
   * the net to add those articles, but still include the ones that do have
   * the Published state.  In other words, specifying later states narrows the
   * search, and specifying only earlier states broadens the search.
   *
   * The states we're looking for are collected in the constructor.
   *
   * 2014-08-01 (OCEEBMS-203): we've added options for filtering down to just
   * articles whose current states are early states. See details at the top of
   * this method.
   */
  private function searchStates() {

    // Only perform this filtering for unrestricted searching.
    if ($this->restricted) {
      return;
    }

    // Collect the set of states which are acceptable for inclusion in the
    // result set. By default the users only want to see articles which have
    // the Published state, but sometimes they'll relax the criteria to also
    // include articles which were rejected (by the librarian) or excluded (by
    // having been published in a journal from which the board in question
    // doesn't generally want to see any of the articles) or which just
    // haven't gotten around to the initial review step. If the user sets the
    // flag for "Include Unpublished Articles" we don't exclude anything in
    // the early filtering step. In that case we leave the earlyStates array
    // empty.
    if (empty($this->parms['filters']['unpublished'])) {
      $this->earlyStates = ['published'];
      if (!empty($this->parms['filters']['not-listed'])) {
        $this->earlyStates[] = 'reject_journal_title';
      }
      if (!empty($this->parms['filters']['rejected'])) {
        $this->earlyStates[] = 'reject_init_review';
      }
    }

    // Now collect the states which reflect activity after the 'Published'
    // state. Unlike the early states, where any one will do to let an article
    // into the set of result candidates, each of these is checked and
    // required separately. The two decision fields checked below carry the
    // state text ID for a specific accept or reject decision state. The third
    // field is just a boolean represented by by a checkbox on the UI. Note
    // that the form for the search request has the state_text_id strings as
    // the values for the "Yes" and "No" radio buttons for the first two
    // fields here, so we don't have to know what those values are.
    if (!empty($this->parms['abstract-decision'])) {
      $this->laterStates[] = $this->parms['abstract-decision'];
    }
    if (!empty($this->parms['full-text-decision'])) {
      $this->laterStates[] = $this->parms['full-text-decision'];
    }
    if (!empty($this->parms['fyi']) && $this->parms['fyi'] === 'yes') {
      $this->laterStates[] = 'fyi';
    }

    // If any "on_agenda" criteria are specified, ebms_article_state rows must
    // also match the on_agenda article state.
    $meeting_category = $this->parms['meeting-category'] ?? '';
    $meeting_start = $this->parms['meeting-start'] ?? '';
    $meeting_end = $this->parms['meeting-end'] ?? '';
    if ($meeting_category || $meeting_start && $meeting_end) {
      $this->laterStates[] = 'on_agenda';
    }

    // Librarians can now limit the search results to include only articles
    // which have been rejected at an early stage, either by the NOT list, or
    // by the librarian initial review, or articles that have not yet been
    // reviewed.
    if (!empty($this->parms['filters']['only-not-listed'])) {
      $this->matchEarlyStates('reject_journal_title');
    }
    elseif (!empty($this->parms['filters']['only-rejected'])) {
      $this->matchEarlyStates('reject_init_review');
    }
    elseif (!empty($this->parms['filters']['only-unpublished'])) {
      $this->matchEarlyStates();
    }

    // If the matchEarlyStates() method set stateFilters, we're done here.
    if ($this->stateFilters) {
      return;
    }

    // Look for the later states first. The reason for doing it this way is
    // that the downstream (that is, after Published) status values guarantee
    // that the Published state is also present (which means we don't have to
    // bother checking the early states if we filter on the later ones).
    if (!empty($this->laterStates)) {

      // Each state mentioned must be present.
      foreach ($this->laterStates as $state) {
        $state_filter = $state === 'fyi' ? 'any' : 'active';
        $state = $this->stateMap[$state]->id;
        $this->matchStates($state, $state_filter);
      }
    }

    // Only have to check for Published and earlier states if no later states
    // were asked for.
    elseif (!empty($this->earlyStates)) {

      // Map the text IDs to the more efficient integer IDs.
      $states = [];
      foreach ($this->earlyStates as $state) {
        $states[] = $this->stateMap[$state]->id;
      }

      // Optimize for the case of a single state.
      if (count($states) === 1) {
        $states = $states[0];
      }

      // For these we check all the states at once.
      $this->matchStates($states, 'active');
    }
  }

  /**
   * Match current early states.
   *
   * New method to support OCEEBMS-203.
   *
   * Called by srchStates().
   *
   * The librarians can now search for only articles which have early states.
   * If a state name is passed, we match only articles with that state.
   * Otherwise, we match any state which is earlier than 'Published'. If
   * topics or boards were selected, we handle narrowing by those criteria,
   * too.
   *
   * The principal difference between what we're doing here and state
   * filtering we do elsewhere, is that we're only accepting article states
   * which are the current state for the article.
   *
   * @param string $state
   *   Optional text ID for a specific early state.
   */
  private function matchEarlyStates(string $state = '') {

    // Are we looking for one state or multiple?
    if ($state) {
      $states = $this->stateMap[$state]->id;
    }
    else {
      $pub_sequence = $this->stateMap['published']->sequence;
      $states = [];
      foreach ($this->stateMap as $state) {
        if ($state->sequence < $pub_sequence) {
          $states[] = $state->id;
        }
      }
    }

    // Add the state condition to the query.
    $this->matchStates($states, 'current');
  }

  /**
   * Common code to check for specified article states.
   *
   * Called separately for later (post-published) states, where all the states
   * specified must be present, earlier (pre-published) states, where the
   * presence any of the states specified cause an article to be included, and
   * a special set of new searches, which find articles with an early state as
   * the current (not just active) state.
   *
   * @param int|array $states
   *   Either an array of state IDs, or an integer for a single state ID.
   *   Up to this point we've been dealing with the mnemonic state text ids.
   *   Now those have been mapped to the more efficient integer IDs in an
   *   effort to avoid bringing the database server to its knees.
   * @param string $state_filter
   *   One of three values:
   *     'current': only match current states
   *     'active': match any state row which is still valid
   *     'any': no state status filtering.
   */
  private function matchStates($states, string $state_filter) {

    // Try the most granular (topics) first.
    if (!empty($this->topics)) {

      // "OR" means we can do all the topics in one join.
      $topic_logic = $this->parms['topic-logic'] ?? 'or';
      if ($topic_logic === 'or' && count($this->topics) > 1) {
        $group = $this->addStateConditions($states, $state_filter);
        $group->condition('topics.entity.topic', $this->topics, 'IN');
      }

      // Otherwise, user wants multiple topics all to be represented.
      else {
        foreach ($this->topics as $topic) {
          $group = $this->addStateConditions($states, $state_filter);
          $group->condition('topics.entity.topic', $topic);
        }
      }
    }

    // Boards needed?
    if ($this->needBoards) {

      // Boards are always checked in separate joins.
      foreach ($this->boards as $board) {
        $group = $this->addStateConditions($states, $state_filter);
        $group->condition("topics.entity.topic.entity.board", $board);
      }
    }

    // Test for the state(s) without regard to topic or board, but only if we
    // didn't create any joins earlier in this method.
    if (empty($this->stateFilters)) {
      $this->addStateConditions($states, $state_filter);
    }
  }

  /**
   * Add a group to the ebms_article_state table to match one or more states.
   *
   * Can also add a join to restrict the state rows to those which belong to a
   * specific cycle. See notes on matchStates() for an explanation of the
   * parameters. As noted there, $states can be an array or a single integer.
   *
   * Unlike the `Database` API, the entity query condition() method does not
   * automatically use the correct operator based on the type of the value, so
   * we do that ourselves.
   *
   * See OCEEBMS-599: support searching by cycle range.
   *
   * @param mixed $states
   *   Either an array of state IDs, or an integer for a single state ID.
   * @param string $state_filter
   *   One of three values:
   *     'current': only match current states
   *     'active': match any state row which is still valid
   *     'any': no state status filtering.
   *
   * @return \Drupal\Core\Entity\Query\ConditionInterface
   *   Set of conditions to be added to the query as a group.
   */
  private function addStateConditions($states, $state_filter): object {

    // Create the conditions group.
    $operator = is_array($states) ? 'IN' : '=';
    $group = $this->query->andConditionGroup()
                  ->condition('topics.entity.states.entity.value', $states, $operator);

    // Apply any state filters requested.
    if ($state_filter === 'current') {
      $group->condition("topics.entity.states.entity.current", 1);
    }
    elseif ($state_filter == 'active') {
      $group->condition('topics.entity.states.entity.active', 1);
    }

    // Narrow by cycle(s) if requested.
    if (!empty($this->parms['cycle'])) {
      $group->condition('topics.entity.cycle', $this->parms['cycle']);
    }
    else {
      if (!empty($this->parms['cycle-start'])) {
        $group->condition('topics.entity.cycle', $this->parms['cycle-start'], '>=');
      }
      if (!empty($this->parms['cycle-end'])) {
        $group->condition('topics.entity.cycle', $this->parms['cycle-end'], '<=');
      }
    }

    // Remember that we've filtered by state.
    $this->stateFilters = TRUE;

    // Plug the group into the query and return it.
    $this->query->condition($group);
    return $group;
  }

  /**
   * Search for articles with specified topics or boards assigned.
   *
   * Only need boards if no topics assigned.
   *
   * We rarely have to do anything here, because all of the state checks take
   * care of looking for boards and topics requested, and the same is true for
   * searching by cycle.
   */
  private function searchTopicsOrBoards() {

    // Only perform this filtering for unrestricted searching.
    if ($this->restricted) {
      return;
    }

    // Make sure this isn't taken care of elsewhere. Note that checking for
    // whether articles have two or more topic is independent of this test,
    // as that check doesn't care which topics they are.
    if ($this->needTopicOrBoardSearch()) {

      // Check topics first.
      if (!empty($this->topics)) {

        // Single topic is the easiest case.
        $topic_logic = $this->parms['topic-logic'] ?? 'or';
        if (count($this->topics) === 1) {
          $this->query->condition('topics.entity.topic', $this->topics[0]);
        }

        // Searching for any of a set is almost as easy.
        elseif ($topic_logic === 'or') {
          $this->query->condition('topics.entity.topic', $this->topics, 'IN');
        }

        // Harder case, needing separate conditions group for each topic.
        else {
          foreach ($this->topics as $topic) {
            $group = $this->query->andConditionGroup()
                          ->condition('topics.entity.topic', $topic);
            $this->query->condition($group);
          }
        }
      }

      // No topics specified.  Any boards?
      elseif ($this->needBoards) {

        // Separate join required for each board.
        foreach ($this->boards as $board) {
          $group = $this->query->andConditionGroup()
                        ->condition('topics.entity.topic.entity.board', $board);
          $this->query->condition($group);
        }
      }
    }

    // Look for articles with more than one topic. Use a separate group, so we
    // don't mess up other topic-related criteria.
    if (!empty($this->parms['filters']['topics-added'])) {
      $group = $this->query->andConditionGroup()
                    ->condition('topics.%delta', 0, '>');
      $this->query->condition($group);
    }
  }

  /**
   * Restrict search results to articles written by specific authors.
   *
   * Expects semicolon separated authors.
   *   e.g., "Smith AH; Jones B"
   * Expects last name [space] initials.
   *   e.g., "Smith AH"
   * Does no wildcard matching unless specified with percent signs.
   * If multiple authors, article must have all of them (A AND B).
   */
  private function searchAuthors() {
    $authors =
      $authors = explode(';', $this->parms['authors'] ?? '');
    $field = '';
    foreach ($authors as $author) {
      $author = trim($author ?? '');
      if (!empty($author)) {
        $author = Article::normalize($author);
        // Have to use like because of a bug in the testing harness.
        // $operator = preg_match('/[%_]/', $author) ? 'LIKE' : '=';
        $operator = 'LIKE';
        if (empty($field)) {
          $position = $this->parms['author-position'] ?? '';
          $field = match ($position) {
            'first' => 'authors.0.search_name',
            'last' => 'last_author_name',
            default => 'authors.search_name',
          };
        }
        $group = $this->query->andConditionGroup()
                      ->condition($field, $author, $operator);
        $this->query->condition($group);
      }
    }
  }

  /**
   * Filter by article title (wildcards supported).
   */
  private function searchArticleTitle() {
    if (!empty($this->parms['title'])) {
      $title = Article::normalize($this->parms['title']);
      // Always use LIKE to work around a bug in the phpunit database.
      //$operator = preg_match('/[%_]/', $title) ? 'LIKE' : '=';
      $operator = 'LIKE';
      $this->query->condition('search_title', $title, $operator);
    }
  }

  /**
   * Filter based on whether we have the full text PDF for the article.
   *
   * Looking for articles with full text retrieved will not pick up any
   * articles converted from the legacy system (except for any that the users
   * have gone back and plugged in after the fact). Similarly, if the users
   * check "NO" for this field, they'll get all the legacy articles,
   * regardless of whether the full text was obtained in the old system.
   * There's no provision in the user interface for finding only articles for
   * which the full text cannot be obtained.
   */
  private function searchFullTextRetrieved() {
    $full_text = $this->parms['full-text'] ?? '';
    switch ($full_text) {
      case 'yes':
        $this->query->condition('full_text.file', NULL, 'IS NOT NULL');
        break;

      case 'no':
        $this->query->condition('full_text.file', NULL, 'IS NULL');
        break;
    }
  }

  /**
   * Narrow search resuolts by journal.
   */
  private function searchJournal() {
    if (!empty($this->parms['journal'])) {
      $title = trim(preg_replace('/\s+/', ' ', $this->parms['journal'] ?? ''));
      // Always use LIKE to work around a bug in the phpunit database.
      //$operator = preg_match('/[%_]/', $title) ? 'LIKE' : '=';
      $operator = 'LIKE';
      $this->query->condition('journal_title', $title, $operator);
    }

    // Board members don't have access to the core-journals field.
    if (!$this->restricted && !empty($this->parms['core-journals'])) {
      $storage = $this->typeManager->getStorage('ebms_journal');
      $query = $storage->getQuery()->accessCheck(FALSE);
      $query->condition('core', 1);
      $journals = $storage->loadMultiple($query->execute());
      $core_ids = [];
      foreach ($journals as $journal) {
        $core_ids[] = $journal->source_id->value;
      }
      $core = $this->parms['core-journals'] ?? '';
      switch ($core) {
        case 'yes':
          $this->query->condition('source_journal_id', $core_ids, 'IN');
          break;

        case 'no':
          $this->query->condition('source_journal_id', $core_ids, 'NOT IN');
          break;
      }
    }
  }

  /**
   * Find articles by PubMed publication date.
   *
   * Searching by month is only allowed if a year is specified. PubMed
   * sometimes uses zero-padded integers for months, and sometimes uses
   * English month abbreviations. We look for both.
   */
  private function searchPubDate() {
    if (!empty($this->parms['publication-year'])) {
      $year = trim($this->parms['publication-year'] ?? '');
      if (!empty($year)) {
        $this->query->condition('year', $year);
      }
      if (!empty($this->parms['publication-month'])) {
        $month = $this->parms['publication-month'];
        if ($month >= 1 && $month <= 12) {
          $date = new \DateTime(sprintf("2000-%02d-01", $month));
          $values = [$date->format('m'), $date->format('M')];
          // Not $this->query->condition('pub_date.month', $values, 'IN');
          // until Drupal core bug #1518506 is fixed, because that bug breaks
          // the tests.
          $group = $this->query->orConditionGroup()
                        ->condition('pub_date.month', $values[0], 'LIKE')
                        ->condition('pub_date.month', $values[1], 'LIKE');
          $this->query->condition($group);
        }
      }
    }
  }

  /**
   * Search by review cycle assigned.
   */
  private function searchCycle() {

    // Only perform this filtering for unrestricted searching.
    if ($this->restricted) {
      return;
    }

    // Make sure we have something to do here.
    if (!$this->needCycleSearch()) {
      return;
    }

    // Did the user specify any topics?
    if (!empty($this->topics)) {

      // Can the search match any of a number of different topics?
      $topic_logic = $this->parms['topic-logic'] ?? 'or';
      if ($topic_logic !== 'and' && count($this->topics) > 1) {
        $this->query->condition('topics.entity.topic', $this->topics, 'IN');
        if (!empty($this->parms['cycle'])) {
          $this->query->condition('topics.entity.cycle', $this->parms['cycle']);
        }
        else {
          if (!empty($this->parms['cycle-start'])) {
            $this->query->condition('topics.entity.cycle', $this->parms['cycle-start'], '>=');
          }
          if (!empty($this->parms['cycle-end'])) {
            $this->query->condition('topics.entity.cycle', $this->parms['cycle-end'], '<=');
          }
        }
      }

      // Otherwise, wrap each topic in a separate group.
      else {
        foreach ($this->topics as $topic) {
          $group = $this->query->andConditionGroup()
                        ->condition('topics.entity.topic', $topic);
          if (!empty($this->parms['cycle'])) {
            $group->condition('topics.entity.cycle', $this->parms['cycle']);
          }
          else {
            if (!empty($this->parms['cycle-start'])) {
              $group->condition('topics.entity.cycle', $this->parms['cycle-start'], '>=');
            }
            if (!empty($this->parms['cycle-end'])) {
              $group->condition('topics.entity.cycle', $this->parms['cycle-end'], '<=');
            }
          }
          $this->query->condition($group);
        }
      }
    }

    // How about boards? Boards always use AND, never OR.
    elseif (!empty($this->boards)) {
      foreach ($this->boards as $board) {
        $group = $this->query->andConditionGroup()
                      ->condition('topics.entity.topic.entity.board', $board);
        if (!empty($this->parms['cycle'])) {
          $group->condition('topics.entity.cycle', $this->parms['cycle']);
        }
        else {
          if (!empty($this->parms['cycle-start'])) {
            $group->condition('topics.entity.cycle', $this->parms['cycle-start'], '>=');
          }
          if (!empty($this->parms['cycle-end'])) {
            $group->condition('topics.entity.cycle', $this->parms['cycle-end'], '<=');
          }
        }
        $this->query->condition($group);
      }
    }

    // Easy case: no topics or boards specified.
    else {
      if (!empty($this->parms['cycle'])) {
        $this->query->condition('topics.entity.cycle', $this->parms['cycle']);
      }
      else {
        if (!empty($this->parms['cycle-start'])) {
          $this->query->condition('topics.entity.cycle', $this->parms['cycle-start'], '>=');
        }
        if (!empty($this->parms['cycle-end'])) {
          $this->query->condition('topics.entity.cycle', $this->parms['cycle-end'], '<=');
        }
      }
    }
  }

  /**
   * Find articles assigned to a specific reviewer.
   *
   * If an article was assigned to our reviewer, but subsequently dropped from
   * the packet, don't include the article UNLESS the reviewer had already
   * reviewed it.
   *
   * The information we need here isn't available from the article entity, so
   * we talk to the database directly to get the list of articles which match
   * this criterion, and feed them back into a new condition on our article
   * entity query.
   *
   * Note that this check is done independently of any topic, state, cycle,
   * or board checks. This was true in the original, Drupal 7 version of the
   * system, so we're keeping it that way (at least for now). Wouldn't be that
   * difficult to combine the logic so users could narrow the results down to
   * articles for which a specific review made a certain recommendation.
   */
  private function searchArticleReviewer() {

    // Only perform this filtering for unrestricted searching.
    if ($this->restricted) {
      return;
    }

    // Make sure this test has been requested.
    $reviewer = $this->parms['reviewer'] ?? 0;
    if (empty($reviewer)) {
      return;
    }

    // Find out which articles this reviewer has reviewed.
    $reviewed = $this->db->select('ebms_packet_article', 'packet_article')
                     ->fields('packet_article', ['article']);
    $reviewed->join('ebms_packet_article__reviews', 'reviews',
                    'reviews.entity_id = packet_article.id');
    $reviewed->join('ebms_review', 'review',
                    'review.id = reviews.reviews_target_id');
    $reviewed->condition('review.reviewer', $reviewer);

    // Find out which articles were assigned to this reviewer.
    $assigned = $this->db->select('ebms_packet_article', 'packet_article')
                     ->fields('packet_article', ['article']);
    $assigned->join('ebms_packet__articles', 'packet_articles',
                    'packet_articles.articles_target_id = packet_article.id');
    $assigned->join('ebms_packet__reviewers', 'packet_reviewers',
                    'packet_reviewers.entity_id = packet_articles.entity_id');
    $assigned->condition('packet_reviewers.reviewers_target_id', $reviewer);
    $assigned->condition('packet_article.dropped', 0);

    // A union of the two queries gives us the article IDs we need.
    $articles = $assigned->union($reviewed)->execute()->fetchCol();

    // This is a bit crude, but the best we can do, given the limitations of
    // the entity query API. Make sure we force an empty set if our reviewer
    // hasn't been assigned any articles yet.
    if (empty($articles)) {
      $articles = [0];
    }
    $this->query->condition('id', $articles, 'IN');
  }

  /**
   * Find articles with a specific recommendation from board member review.
   *
   * This check is independent of the test for a specific reviewer above. if
   * both checks are requested, we don't require that the specified response
   * come from the specified reviewer, only that some reviewer made the
   * recommendation.
   *
   * As above for the reviewer check, the information we need to use is not
   * available in the article entity, but is buried in the packet and review
   * entities. So we take the same approach we used for the reviewer check,
   * querying the database tables directly, collecting the resulting article
   * IDs, and feeding them back to a new condition on the article entity
   * query.
   */
  private function searchReviewerResponse() {

    // Only perform this filtering for unrestricted searching.
    if ($this->restricted) {
      return;
    }

    // Make sure this test has been requested.
    $disposition = $this->parms['disposition'] ?? 0;
    if (empty($disposition)) {
      return;
    }

    // Create the database query.
    $query = $this->db->select('ebms_packet_article', 'packet_article')
                  ->fields('packet_article', ['article']);
    $query->join('ebms_packet_article__reviews', 'reviews',
                 'reviews.entity_id = packet_article.id');
    $query->join('ebms_review__dispositions', 'disposition',
                 'disposition.entity_id = reviews.reviews_target_id');
    $query->condition('disposition.dispositions_target_id', $disposition);
    $query->condition('packet_article.dropped', 0);

    // Fold the article IDs into the entity query.
    $articles = $query->execute()->fetchCol();
    if (empty($articles)) {
      $articles = [0];
    }
    $this->query->condition('id', $articles, 'IN');
  }

  /**
   * Search by state comments and/or topic comments.
   *
   * If state conditions are also specified in the same search we're not
   * limiting comments to those appearing in the requested state(s),
   * preserving the behavior of the original Drupal 7 site's logic. We do,
   * however test for topics and boards (though as in the original
   * implementation, we ignore the topic logic field and always use "OR"
   * logic for topics here, and use "OR" for multiple board instead of the
   * usual "AND"), and if both comment and comment date(s) are specified, make
   * sure that we're matching on the same comment.
   *
   * Tag comments are not searched (though users may want that in the future).
   *
   * In most cases comment searches will only be useful if the user includes
   * wildcards (% or _ as appropriate). Otherwise the comment must match
   * exactly what the user enters in the comment search field.
   */
  private function searchComments() {

    // Only perform this filtering for unrestricted searching.
    if ($this->restricted) {
      return;
    }

    // Collect the values for searching state comments.
    $comment = $this->parms['comment'] ?? '';
    $start = $this->parms['comment-start'] ?? '';
    $end = $this->parms['comment-end'] ?? '';

    // See if searchModifiedDate() will take care of checking comment dates.
    if (!empty($this->parms['modified-start']) || !empty($this->parms['modified-end'])) {
      $start = $end = '';
    }

    // See if we have comment and/or comment dates.
    if (!empty($comment) || !empty($start) || !empty($end)) {

      // Start a new group.
      $group = $this->query->andConditionGroup();

      // If a comment (possibly with wildcard(s) was specified, look for it.
      if (!empty($comment)) {
        // Not $operator = preg_match('/[%_]/', $comment) ? 'LIKE' : '=';
        // until Drupal core bug #1518506 is fixed, because that bug breaks
        // the tests.
        $operator = 'LIKE';
        $group->condition('topics.entity.states.entity.comments.body', $comment, $operator);
      }

      // Check for dates if requested.
      if (!empty($start)) {
        $group->condition('topics.entity.states.entity.comments.entered', $start, '>=');
      }
      if (!empty($end)) {
        if (strlen($end) == 10) {
          $end .= ' 23:59:59';
        }
        $group->condition('topics.entity.states.entity.comments.entered', $end, '<=');
      }

      // Test for topic(s) and/or board(s).
      if (!empty($this->topics)) {
        $group->condition('topics.entity.topic', $this->topics, 'IN');
      }
      elseif (!empty($this->boards)) {
        $group->condition('topics.entity.topic.entity.board', $this->boards, 'IN');
      }

      // Plug the group into the main query.
      $this->query->condition($group);
    }

    // Checking for topic comments is simpler. They don't have to be in the
    // Same topic as the state comment we just searched for.
    if (!empty($this->parms['board-manager-comment'])) {
      $comment = $this->parms['board-manager-comment'];
      $operator = 'LIKE';
      // @todo Use this instead when Drupal core bug #1518506 is fixed.
      // $operator = preg_match('/[%_]/', $comment) ? 'LIKE' : '=';
      // See https://www.drupal.org/project/drupal/issues/1518506.
      $group = $this->query->andConditionGroup()
                    ->condition('topics.entity.comments.comment', $comment, $operator);
      if (!empty($this->topics)) {
        $group->condition('topics.entity.topic', $this->topics, 'IN');
      }
      elseif (!empty($this->boards)) {
        $group->condition('topics.entity.topic.entity.board', $this->boards, 'IN');
      }
      $this->query->condition($group);
    }
  }

  /**
   * Search for tagged articles.
   *
   * Article tags can be associated with a specific topic, or directly with
   * the article, so we need to "OR" two groups together in order to achieve
   * the desired result.
   */
  private function searchTags() {

    // Board mebers don't have access to these fields.
    if (!$this->restricted) {

      // See if we have anything to search.
      $tag = $this->parms['article-tag'] ?? '';
      $start = $this->parms['tag-start'] ?? '';
      $end = $this->parms['tag-end'] ?? '';
      if ($tag || $start || $end) {

        // Make sure we check a date range to the very last second.
        if (!empty($end) && strlen($end) === 10) {
          $end .= ' 23:59:59';
        }

        // Create a group for finding tags assigned directly to the articles.
        $direct = $this->query->andConditionGroup();
        if (!empty($tag)) {
          $direct->condition('tags.entity.tag', $tag);
        }
        if (!empty($start)) {
          $direct->condition('tags.entity.assigned', $start, '>=');
        }
        if (!empty($end)) {
          $direct->condition('tags.entity.assigned', $end, '<=');
        }

        // And a second group to find topic-specific tags.
        $by_topic = $this->query->andConditionGroup();
        if (!empty($tag)) {
          $by_topic->condition('topics.entity.tags.entity.tag', $tag);
        }
        if (!empty($start)) {
          $by_topic->condition('topics.entity.tags.entity.assigned', $start, '>=');
        }
        if (!empty($end)) {
          $by_topic->condition('topics.entity.tags.entity.assigned', $end, '<=');
        }
        if (!empty($this->topics)) {
          $by_topic->condition('topics.entity.topic', $this->topics, 'IN');
        }
        elseif (!empty($this->boards)) {
          $by_topic->condition('topics.entity.topic.entity.board', $this->boards, 'IN');
        }

        // Plug both groups into the query.
        $combo = $this->query->orConditionGroup()
                      ->condition($direct)
                      ->condition($by_topic);
        $this->query->condition($combo);
      }
    }
  }

  /**
   * Search by date of original article import.
   */
  private function searchImportDate() {

    // Board members don't have access to these fields.
    if (!$this->restricted) {
      if (!empty($this->parms['import-start'])) {
        $this->query->condition('import_date', $this->parms['import-start'], '>=');
      }
      if (!empty($this->parms['import-end'])) {
        $end = $this->parms['import-end'];
        if (strlen($end) === 10) {
          $end .= ' 23:59:59';
        }
        $this->query->condition('import_date', $end, '<=');
      }
    }
  }

  /**
   * Search for modifications made to the articles by the users.
   *
   * Does not include modifications NLM reports having been made to the PubMed
   * records, nor any refreshes of the data performed by pulling in those
   * changes. That should be a separate test field, should the users ever
   * decide that it would be useful.
   */
  private function searchModifiedDate() {

    // Only perform this filtering for unrestricted searching.
    if ($this->restricted) {
      return;
    }

    // See if we have anything to check.
    if (!$this->needModifiedDateSearch()) {
      return;
    }
    $start = $this->parms['modified-start'] ?? '';
    $end = $this->parms['modified-end'] ?? '';
    if (!empty($end) && strlen($end) === 10) {
      $end .= ' 23:59:59';
    }

    // Do the real work at a lower level. otherwise the query will never
    // finish.
    $this->query->addTag('search_modified_date');
    $this->query->addMetaData('search_modified_date_start', $start);
    $this->query->addMetaData('search_modified_date_end', $end);
    $this->query->addMetaData('search_modified_topics', $this->topics);
    $this->query->addMetaData('search_modified_boards', $this->boards);
  }

  /**
   * Add a topic or board condition to a search group.
   *
   * Called by `$this->searchModifiedDate()` and `$this->searchOnAgenda()`.
   *
   * @param object $group
   *   Group of conditions to be possibly augmented.
   */
  private function addTopicOrBoardCondition(object $group) {
    if (!empty($this->topics)) {
      $group->condition('topics', $this->topics, 'IN');
    }
    elseif (!empty($this->boards)) {
      $group->condition('topics.entity.topic.entity.board', $this->boards, 'IN');
    }
  }

  /**
   * Find articles on the agenda by meeting type and/or date.
   */
  private function searchOnAgenda() {

    // Only perform this filtering for unrestricted searching.
    if ($this->restricted) {
      return;
    }

    // See if we have anything to search.
    if (!$this->needOnAgendaSearch()) {
      return;
    }

    // Collect the user's values.
    $type = $this->parms['meeting-category'] ?? 0;
    $start = $this->parms['meeting-start'] ?? '';
    $end = $this->parms['meeting-end'] ?? '';
    $group = $this->query->andConditionGroup();

    // Apply them.
    if (!empty($type)) {
      $group->condition('topics.entity.states.entity.meetings.entity.category', $type);
    }
    if (!empty($start)) {
      $group->condition('topics.entity.states.entity.meetings.entity.dates.value', $start, '>=');
    }
    if (!empty($end)) {
      if (strlen($end) === 10) {
        $end .= 'T23:59:59';
      }
      $group->condition('topics.entity.states.entity.meetings.entity.dates.value', $end, '<=');
    }

    // Narrow to topic(s) or board(s) if requested.
    $this->addTopicOrBoardCondition($group);

    // Plug in the new group.
    $this->query->condition($group);
  }

  /**
   * Search by final board decision.
   */
  private function searchDecision() {
    if (!$this->restricted && !empty($this->parms['decision'])) {
      $decision = $this->parms['decision'];
      $group = $this->query->andConditionGroup()
                    ->condition('topics.entity.states.entity.decisions.decision', $decision)
                    ->condition('topics.entity.states.entity.active', TRUE);
      $this->addTopicOrBoardCondition($group);
      $this->query->condition($group);
    }
  }

  /**
   * Make sure we don't pick up articles intended for internal staff only.
   *
   * See OCEEBMS-509. An "internal" article in this context is one which has
   * no topics associated with it. It is possible for an article to disappear
   * both from the internal article queue page and from the search system, if
   * an article is imported as internal, is never assigned any topics, and
   * someone subsequently deletes all of its internal tags. In that case, the
   * way to get it to reappear would be to re-import it, either internally
   * with new internal tags, or using the standard import page, assigning a
   * board and topic.
   *
   * All we need to do is see if any of the search criteria have already
   * ensured that a topic is present, as will frequently be the case. If not,
   * we add a condition to take care of the requirement that at least one
   * topic be found.
   */
  private function excludeInternalArticles() {
    if (!$this->needTopicOrBoardSearch()) {
      $this->query->exists('topics');
    }
  }

  /**
   * Sort the articles as requested.
   */
  private function searchOrder() {
    $default = $this->restricted ? 'pmid' : 'ebms-id';
    $sort_order = $this->parms['sort'] ?? $default;
    if ($sort_order === 'pmid') {
      $this->query->sort('source_id');
    }
    elseif ($sort_order === 'title') {
      $this->query->sort('title');
    }
    elseif ($sort_order === 'author') {
      $this->query->sort('authors.0.display_name');
    }
    elseif ($sort_order === 'journal') {
      $this->query->sort('journal_title');
    }
    elseif ($sort_order === 'core') {
      $this->query->addTag('core_journal_sort');
    }
    else {
      $this->query->sort('id');
    }
  }

  /**
   * Find out if we need to search by topic or board.
   *
   * @return bool
   *   `FALSE` if any other tests have already checked for boards and topics.
   */
  private function needTopicOrBoardSearch() {
    if (empty($this->topics) && empty($this->boards)) {
      return FALSE;
    }
    if ($this->stateFilters && $this->needCycleSearch()) {
      return FALSE;
    }
    if ($this->needModifiedDateSearch()) {
      return FALSE;
    }
    if ($this->needOnAgendaSearch()) {
      return FALSE;
    }
    if (!empty($this->parms['comment']) || !empty($this->parms['comment-start']) || !empty($this->parms['comment-end'])) {
      return FALSE;
    }
    if (!empty($this->parms['article-tag']) || !empty($this->parms['tag-start']) || !empty($this->parms['tag-end'])) {
      return FALSE;
    }
    if (!empty($this->parms['decision'])) {
      return FALSE;
    }
    return TRUE;
  }

  /**
   * Check to see if we need to consider boards in a search.
   *
   * We don't if no board was specified or if the single selected
   * board is automatically implied by the topic(s).
   *
   * ASSUMPTIONS:
   *   $this->boards and $this->topics have already been set.
   *
   * @return bool
   *   `TRUE` = Must check boards.  Else `FALSE`.
   */
  private function needBoardSearch(): bool {

    if (count($this->boards) === 0) {
      // No boards to check.
      return FALSE;
    }

    // At least one board found.
    if (count($this->topics) === 0) {
      // No topics to imply boards.  Must check the specified board(s).
      return TRUE;
    }

    // At least one board + one topic found.
    if (count($this->boards) === 1) {
      // Checking the topic(s) implies the one board.
      return FALSE;
    }

    // Multiple boards specified.  Assume the user really needed them.
    return TRUE;
  }

  /**
   * Determine whether any of the cycle fields on the search form has a value.
   *
   * @return bool
   *   `TRUE` if we need to filter by a cycle or a cycle range.
   */
  private function needCycleSearch(): bool {
    if ($this->restricted) {
      return FALSE;
    }
    if ($this->stateFilters) {
      return FALSE;
    }
    if (!empty($this->parms['cycle'])) {
      return TRUE;
    }
    if (!empty($this->parms['cycle-start'])) {
      return TRUE;
    }
    if (!empty($this->parms['cycle-end'])) {
      return TRUE;
    }
    return FALSE;
  }

  /**
   * Find out if we need to search by date of article modification.
   */
  private function needModifiedDateSearch() {
    if ($this->restricted) {
      return FALSE;
    }
    if (!empty($this->parms['modified-start'])) {
      return TRUE;
    }
    if (!empty($this->parms['modified-end'])) {
      return TRUE;
    }
    return FALSE;
  }

  /**
   * Find out if we need to look for articles on an agenda.
   */
  private function needOnAgendaSearch(): bool {
    if ($this->restricted) {
      return FALSE;
    }
    if (!empty($this->parms['meeting-category'])) {
      return TRUE;
    }
    if (!empty($this->parms['meeting-start'])) {
      return TRUE;
    }
    if (!empty($this->parms['meeting-end'])) {
      return TRUE;
    }
    return FALSE;
  }

}
