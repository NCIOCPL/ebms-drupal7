<?php

namespace Drupal\ebms_article\Entity;

use Drupal\Core\Entity\ContentEntityBase;
use Drupal\Core\Entity\ContentEntityInterface;
use Drupal\Core\Entity\EntityTypeInterface;
use Drupal\Core\Field\BaseFieldDefinition;
use Drupal\Core\Field\FieldStorageDefinitionInterface;
use Drupal\ebms_article\ArticlePublication;
use Drupal\ebms_state\Entity\State;
use Drupal\ebms_article\Plugin\Field\FieldType\Author;

/**
 * Article to be reviewed by PDQ boards.
 *
 * The EBMS identifies published articles which may contain useful information
 * for the PDQ cancer information summaries. These articles represent the core
 * of the EBMS and the primary reason for its existence.
 *
 * @ingroup ebms
 *
 * @ContentEntityType(
 *   id = "ebms_article",
 *   label = @Translation("Article"),
 *   handlers = {
 *     "view_builder" = "Drupal\ebms_article\Controller\ArticleController",
 *     "list_builder" = "Drupal\ebms_article\ArticleListBuilder",
 *     "form" = {
 *       "default" = "Drupal\ebms_article\Form\ArticleForm",
 *       "edit" = "Drupal\ebms_article\Form\ArticleForm",
 *     },
 *     "route_provider" = {
 *       "html" = "Drupal\Core\Entity\Routing\AdminHtmlRouteProvider",
 *     },
 *     "views_data" = "Drupal\views\EntityViewsData",
 *   },
 *   base_table = "ebms_article",
 *   admin_permission = "access ebms article overview",
 *   translatable = FALSE,
 *   entity_keys = {
 *     "id" = "id",
 *     "label" = "source_id",
 *     "published" = "active",
 *   },
 *   links = {
 *     "canonical" = "/ebms_article/{ebms_article}",
 *     "edit-form" = "/ebms_article/{ebms_article}/edit",
 *     "collection" = "/admin/content/ebms_article",
 *   }
 * )
 */
class Article extends ContentEntityBase implements ContentEntityInterface {

  /**
   * The date the EBMS originally went live.
   */
  const CONVERSION_DATE = '2013-03-18';

  /**
   * {@inheritdoc}
   */
  public static function baseFieldDefinitions(EntityTypeInterface $entity_type): array {
    $fields = parent::baseFieldDefinitions($entity_type);
    $fields['title'] = BaseFieldDefinition::create('string')
      ->setLabel('Title')
      ->setRequired(TRUE)
      ->setDescription('Official title of the article.')
      ->setSettings(['max_length' => 5000]);
    $fields['search_title'] = BaseFieldDefinition::create('string')
      ->setLabel('Search Title')
      ->setRequired(TRUE)
      ->setDescription('Portion of title converted to plain ASCII for searching.')
      ->setSettings(['max_length' => 512]);
    $fields['authors'] = BaseFieldDefinition::create('ebms_author')
      ->setLabel('Authors')
      ->setDescription('Individuals and/or corporate entities who wrote the article.')
      ->setCardinality(FieldStorageDefinitionInterface::CARDINALITY_UNLIMITED);
    $fields['source'] = BaseFieldDefinition::create('string')
      ->setLabel('Source')
      ->setRequired(TRUE)
      ->setSettings(['max_length' => 32])
      ->setDescription('Source of information about the article.');
    $fields['source_id'] = BaseFieldDefinition::create('string')
      ->setLabel('Source ID')
      ->setRequired(TRUE)
      ->setSettings(['max_length' => 32])
      ->setDescription('Identifier for the article, unique for the source.');
    $fields['source_journal_id'] = BaseFieldDefinition::create('string')
      ->setLabel('Journal ID')
      ->setRequired(TRUE)
      ->setSettings(['max_length' => 32])
      ->setDescription("Source ID for the article's journal.");
    $fields['source_status'] = BaseFieldDefinition::create('string')
      ->setLabel('Status')
      ->setSettings(['max_length' => 32])
      ->setDescription('Status of the article in the source system.');
    $fields['journal_title'] = BaseFieldDefinition::create('string')
      ->setLabel('Journal Title')
      ->setRequired(TRUE)
      ->setSettings(['max_length' => 512])
      ->setDescription('Full title of the journal as given in this article.');
    $fields['brief_journal_title'] = BaseFieldDefinition::create('string')
      ->setLabel('Journal Abbreviation')
      ->setSettings(['max_length' => 127])
      ->setDescription('Shortened version of the journal title as given in this article.');
    $fields['volume'] = BaseFieldDefinition::create('string')
      ->setLabel('Journal Volume')
      ->setSettings(['max_length' => 127])
      ->setDescription('Volume of the journal in which this article appears.');
    $fields['issue'] = BaseFieldDefinition::create('string')
      ->setLabel('Journal Issue')
      ->setSettings(['max_length' => 127])
      ->setDescription('Issue of the journal in which this article appears.');
    $fields['pagination'] = BaseFieldDefinition::create('string')
      ->setLabel('Pagination')
      ->setSettings(['max_length' => 127])
      ->setDescription('Pages on which this article appears.');

    // This approach is an enhancement to the original EBMS, which stored the
    // abstract as a single string, failing to preserve paragraph divisions.
    $fields['abstract'] = BaseFieldDefinition::create('ebms_abstract_paragraph')
      ->setLabel('Abstract')
      ->setDescription("Paragraphs for the article's summary.")
      ->setCardinality(FieldStorageDefinitionInterface::CARDINALITY_UNLIMITED);

    $fields['pub_date'] = BaseFieldDefinition::create('ebms_pub_date')
      ->setLabel('Publication Date')
      ->setRequired(TRUE)
      ->setDescription("Date of the article's publication.");
    $fields['year'] = BaseFieldDefinition::create('integer')
      ->setLabel('Publication Year')
      ->setDisplayOptions('view', ['label' => 'inline'])
      ->setDescription('Year in which the article was published.');
    $fields['imported_by'] = BaseFieldDefinition::create('entity_reference')
      ->setLabel('Imported By')
      ->setRequired(TRUE)
      ->setDescription('User who first imported this article.')
      ->setSetting('target_type', 'user');
    $fields['import_date'] = BaseFieldDefinition::create('datetime')
      ->setLabel('Import Date')
      ->setRequired(TRUE)
      ->setDescription('When the article information was first retrieved from NLM.');
    $fields['update_date'] = BaseFieldDefinition::create('datetime')
      ->setLabel('Update Date')
      ->setDescription('When the article information was last refreshed from NLM.');
    $fields['data_mod'] = BaseFieldDefinition::create('datetime')
      ->setLabel('Article Data Modified by NLM')
      ->setSettings(['datetime_type' => 'date'])
      ->setDescription('When the article information was last modified at NLM.');
    $fields['data_checked'] = BaseFieldDefinition::create('datetime')
      ->setLabel('When We Last Checked with NLM for Updates')
      ->setSettings(['datetime_type' => 'date'])
      ->setDescription('When we last checked that we have the most recent XML from NLM.');
    $fields['full_text'] = BaseFieldDefinition::create('ebms_full_text')
      ->setLabel('Full Text')
      ->setDescription('Information about the PDF for the full text of the article.');
    $fields['topics'] = BaseFieldDefinition::create('entity_reference')
      ->setRequired(TRUE)
      ->setLabel('Topics')
      ->setDescription('Topics assigned to this article.')
      ->setSetting('target_type', 'ebms_article_topic')
      ->setCardinality(FieldStorageDefinitionInterface::CARDINALITY_UNLIMITED);
    $fields['tags'] = BaseFieldDefinition::create('entity_reference')
      ->setLabel('Tags')
      ->setDescription('Tags which have been assigned to this article.')
      ->setSetting('target_type', 'ebms_article_tag')
      ->setCardinality(FieldStorageDefinitionInterface::CARDINALITY_UNLIMITED);
    $fields['internal_tags'] = BaseFieldDefinition::create('ebms_internal_tag')
      ->setLabel('Internal Tags')
      ->setDescription('Articles with these tags are not reviewed.')
      ->setCardinality(FieldStorageDefinitionInterface::CARDINALITY_UNLIMITED);
    $fields['internal_comments'] = BaseFieldDefinition::create('ebms_comment')
      ->setLabel('Internal Comments')
      ->setDescription('Optional notes on article of internal interest only.')
      ->setCardinality(FieldStorageDefinitionInterface::CARDINALITY_UNLIMITED);
    $fields['types'] = BaseFieldDefinition::create('string')
      ->setLabel('Types')
      ->setDescription('Article types which have been assigned by NLM.')
      ->setCardinality(FieldStorageDefinitionInterface::CARDINALITY_UNLIMITED);
    $fields['legacy_id'] = BaseFieldDefinition::create('integer')
      ->setLabel('Legacy ID')
      ->setDescription('Article ID from the original Visual Basic system, if appropriate');

    // This is a workaround for a limitation in QueryInterface::condition,
    // which is incapable of searching for a specific value in the last
    // instance of a multivalued field. This is a better solution than
    // giving up on the Drupal entity query interface and resorting to
    // SQL, I suppose.
    $fields['last_author_name'] = BaseFieldDefinition::create('string')
      ->setLabel('Last Author')
      ->setDescription('Support for article searching by last author name.')
      ->setSettings(['max_length' => 767]);

    // Add the calculated publication field.
    $fields['publication'] = BaseFieldDefinition::create('string')
      ->setComputed(TRUE)
      ->setName('publication')
      ->setClass(ArticlePublication::class)
      ->setDescription("Information about the article's publication")
      ->setLabel('Publication');

    return $fields;
  }

  /**
   * Add a start to the article for a specific topic.
   *
   * Creates and stores a new `State` entity, attaches it to the appropriate
   * `ArticleTopic` (creating that if this is the first state for its topic),
   * and adds the target ID for the new entity to the `states` field, but does
   * not save the `Article` entity itself. That's up to the caller, which
   * might want to make other modifications to the `Article` entity.
   *
   * Note that dependency injection is not available for a custom entity,
   * according to "Clive" (https://drupal.stackexchange.com/users/2800/clive).
   * See https://drupal.stackexchange.com/questions/259784.
   *
   * @param string $text_id
   *   Text ID for the state value.
   * @param int $topic_id
   *   Topic identifier, always required.
   * @param int|null $user_id
   *   User setting the state.  If `NULL`` use the current user's ID.
   * @param string|null $entered
   *   Date/time when state was entered.  If `NULL`, use the current date/time.
   * @param string|null $cycle
   *   Review cycle for this state change.
   * @param string|null $comment
   *   Optional user or program supplied comment.
   *
   * @return object
   *   Reference to the new `State` entity.
   *
   * @throws \Exception
   *   If invalid parameters are submitted.
   */
  public function addState(
    string $text_id,
    int $topic_id,
    int $user_id = NULL,
    string $entered = NULL,
    string $cycle = NULL,
    string $comment = NULL
  ): object {

    // We have decided that no state more advanced than 'Published' is
    // allowed without also having a Published state.
    $term_lookup = \Drupal::service('ebms_core.term_lookup');
    static $published_state = NULL;
    if (empty($published_state)) {
      $published_state = $term_lookup->getState('published');
    }

    // Get everything we know about this state.
    $state_value = $term_lookup->getState($text_id);
    if (empty($state_value)) {
      throw new \Exception("addState(): Unknown state '$text_id'");
    }
    $sequence = $state_value->field_sequence->value;
    $need_published = $sequence > $published_state->field_sequence->value;

    // Get default user from global if not provided.
    if (empty($user_id)) {
      $user_id = \Drupal::currentUser()->id();
    }

    // Get the board to which the topic belongs.
    $topics = \Drupal::entityTypeManager()->getStorage('ebms_topic');
    $topic = $topics->load($topic_id);
    $board_id = $topic->get('board')->target_id;

    // Use the current time if date/time not provided.
    if (empty($entered)) {
      $entered = date('Y-m-d H:i:s');
    }

    // Find the `ArticleTopic` entity.
    $article_topic = NULL;
    foreach ($this->topics as $candidate) {
      if ($candidate->entity->topic->target_id == $topic_id) {
        $article_topic = $candidate->entity;
        break;
      }
    }

    // Create a new one if it's missing.
    $new_topic = FALSE;
    if (empty($article_topic)) {
      if (empty($cycle)) {
        $cycle = new \DateTime($entered);
        $cycle->modify('first day of next month');
        $cycle = $cycle->format('Y-m-d');
      }
      $new_topic = TRUE;
      $values = [
        'topic' => $topic_id,
        'cycle' => $cycle,
      ];
      $article_topic = ArticleTopic::create($values);
    }

    // Otherwise, make any necessary tweaks to the topic's existing states.
    else {
      foreach ($article_topic->states as $state) {
        $state = $state->entity;
        $modified = FALSE;
        if (!empty($state->current->value)) {
          $state->set('current', FALSE);
          $modified = TRUE;
        }
        if ($state->value->entity->field_sequence->value >= $sequence) {
          if (!empty($state->active->value)) {
            $state->set('active', FALSE);
            $state->comments->appendItem([
              'user' => $user_id,
              'entered' => $entered,
              'body' => "State inactivated by setting '$text_id' at $entered",
            ]);
            $modified = TRUE;
          }
        }
        if ($modified) {
          $state->save();
        }
        if ($need_published && $state->value->entity->field_text_id->value === 'published') {
          $need_published = FALSE;
        }
      }
    }

    // Prepare to create the new `State` entity.
    $values = [
      'article' => $this->id(),
      'board' => $board_id,
      'topic' => $topic_id,
      'user' => $user_id,
      'entered' => $entered,
      'active' => TRUE,
    ];

    // If we need a 'published' state, use these value to add one.
    if ($need_published) {
      $values['value'] = $published_state->id();
      $values['current'] = FALSE;
      $values['comments'][] = [
        'user' => $user_id,
        'entered' => $entered,
        'body' => "Published state added as a result of setting the state for this article/topic to $text_id",
      ];
      $state = State::create($values);
      $state->save();
      $article_topic->states->appendItem($state->id());
      unset($values['comments']);
    }

    // Create the state the caller requested.
    $values['value'] = $state_value->id();
    $values['current'] = TRUE;
    if (!empty($comment)) {
      $values['comments'][] = [
        'user' => $user_id,
        'entered' => $entered,
        'body' => $comment,
      ];
    }
    $state = State::create($values);
    $state->save();
    $article_topic->states[] = $state->id();
    $article_topic->save();
    if ($new_topic) {
      $this->topics[] = $article_topic->id();
    }

    // The caller gets a reference to our new `State` entity.
    return $state;
  }

  /**
   * Add a tag to the article.
   *
   * Does not save the `Article` entity itself. That's up to the caller,
   * which might want to make other modifications to the `Article` entity.
   *
   * @param string $text_id
   *   Stable string ID for the tag.
   * @param int $topic
   *   Optional topic for the tag.
   * @param int $user
   *   ID for user adding the tag.
   * @param string $date
   *   Date/time when the tag was assigned.
   * @param string $comment
   *   Optional notes on the tag assignment.
   *
   * @return object
   *   Reference to `ArticleTag` object.
   *
   * @throws \Exception
   *   For failures bubbled up from the stack.
   */
  public function addTag(string $text_id, int $topic = 0, int $user = 0, string $date = '', string $comment = ''): object {

    // Get the taxonomy term entity for this tag.
    $storage = \Drupal::entityTypeManager()->getStorage('taxonomy_term');
    $query = $storage->getQuery()->accessCheck(FALSE);
    $query->condition('vid', 'article_tags');
    $query->condition('field_text_id', $text_id);
    $ids = $query->execute();
    if (empty($ids)) {
      throw new \Exception(("Unknown tag '$text_id'"));
    }
    if (count($ids) !== 1) {
      throw new \Exception("Ambiguous tag name '$text_id'");
    }
    $tag_term = $storage->load(reset($ids));

    // Find out where the tag should go (article or topic).
    $topic_allowed = $tag_term->field_topic_allowed->value;
    $topic_required = $tag_term->field_topic_required->value;
    if (empty($topic)) {
      if ($topic_required) {
        throw new \Exception("Topic is required for article tag '$text_id'.");
      }
      $parent = $this;
    }
    else {
      if (!$topic_allowed) {
        throw new \Exception("Topic cannot be specified for article tag '$text_id'.");
      }
      $parent = NULL;
      foreach ($this->topics as $article_topic) {
        $article_topic = $article_topic->entity;
        if ($article_topic->topic->target_id == $topic) {
          $parent = $article_topic;
          break;
        }
      }
      if (empty($parent)) {
        $topics = \Drupal::entityTypeManager()->getStorage('ebms_topic');
        $topic_name = $topics->load($topic)->getName();
        $id = $this->id();
        throw new \Exception("Topic '$topic_name' not assigned to article $id.");
      }
    }

    // Fill in any missing pieces.
    if (empty($user)) {
      $user = \Drupal::currentUser()->id();
    }
    if (empty($date)) {
      $date = date('Y-m-d H:i:s');
    }
    $comment = trim($comment ?? '');
    if (!empty($comment)) {
      $comment = [
        'user' => $user,
        'entered' => $date,
        'body' => $comment,
      ];
    }

    // See if we already have this tag assignment.
    $existing_tag = NULL;
    foreach ($parent->tags as $tag) {
      $entity = $tag->entity;
      if ($entity->tag->target_id == $tag_term->id()) {
        $existing_tag = $entity;
        break;
      }
    }
    if ($existing_tag) {
      if (!empty($comment)) {
        $existing_tag->comment[] = $comment;
      }
      return $existing_tag;
    }

    // We need to create a new `ArticleTag` entity.
    $values = [
      'tag' => $tag_term->id(),
      'user' => $user,
      'assigned' => $date,
      'active' => TRUE,
    ];
    if (!empty($comment)) {
      $values['comments'] = [$comment];
    }
    $article_tag = ArticleTag::create($values);
    $article_tag->save();
    $parent->tags[] = $article_tag->id();
    $parent->save();
    return $article_tag;
  }

  /**
   * Compare values for a complex field.
   *
   * @param string $name
   *   Name of the field.
   * @param array $properties
   *   Names of the field's properties.
   * @param array $values
   *   New values for the `Article` entity.
   * @param bool $multiple
   *   `TRUE` if the field can hold multiple occurrences, else `FALSE`.
   *
   * @return bool
   *   `TRUE` if the values differ, else `FALSE`
   */
  private function changed(string $name, array $properties, array $values, bool $multiple): bool {
    $old_values = [];
    foreach ($this->get($name) as $value) {
      $old_values[] = $value->toArray();
    }
    if (!empty($values[$name])) {
      $new_values = $values[$name];
      if (!$multiple) {
        $new_values = [$new_values];
      }
    }
    else {
      $new_values = [];
    }
    $old_count = count($old_values);
    $new_count = count($new_values);
    if ($old_count !== $new_count) {
      return TRUE;
    }
    for ($i = 0; $i < $old_count; $i++) {
      foreach ($properties as $property) {
        $old_value = $old_values[$i][$property] ?? '';
        if ($property === 'value') {
          $new_value = $new_values[$i];
        }
        else {
          $new_value = $new_values[$i][$property] ?? '';
        }
        if ($old_value != $new_value) {
          return TRUE;
        }
      }
    }
    return FALSE;
  }

  /**
   * Get article by source ID.
   *
   * @param string $source
   *   So far, 'Pubmed' has been the only supported value.
   * @param string $source_id
   *   The unique ID in the source system (e.g., the PubMed ID).
   *
   * @return object|null
   *   The matching article or NULL if none found.
   */
  public static function getArticleBySourceId(string $source, string $source_id): ?object {
    $storage = \Drupal::entityTypeManager()->getStorage('ebms_article');
    $query = $storage->getQuery()->accessCheck(FALSE);
    $query->condition('source', $source);
    $query->condition('source_id', $source_id);
    $ids = $query->execute();
    if (count($ids) > 1) {
      $ids = implode(', ', $ids);
      throw new \Exception("$source ID $source_id matches multiple articles (EBMS IDs: $ids).");
    }
    return empty($ids) ? NULL : $storage->load(reset($ids));
  }

  /**
   * Collect the display names of the article's authors.
   *
   * @param int $max
   *   If greater than zero, replace excess authors with "et al."
   *
   * @return array
   *   Array of strings for the authors' names.
   */
  public function getAuthors(int $max = 0): array {
    $authors = [];
    foreach ($this->authors as $author) {
      $name = $author->display_name;
      if (empty($name)) {
        $last_name = $author->last_name;
        if ($last_name) {
          $name = $last_name;
          $initials = $author->initials;
          if ($initials) {
            $name = "$name $initials";
          }
        }
        else {
          $name = $author->collective_name;
        }
      }
      if (!empty($name)) {
        $authors[] = $name;
      }
    }
    if ($max > 0 && count($authors) > $max) {
      $authors = array_slice($authors, 0, $max);
      $authors[] = 'et al.';
    }
    return $authors;
  }

  /**
   * Get the current state for a given topic.
   *
   * @param int $topic
   *   ID for the `Topic` entity.
   *
   * @return \Drupal\ebms_state\Entity\State|null
   *   Requested `State` entity if found.
   */
  public function getCurrentState(int $topic): ?State {
    if (empty($topic)) {
      return NULL;
    }
    foreach ($this->topics as $article_topic) {
      $article_topic = $article_topic->entity;
      if ($article_topic->topic->target_id == $topic) {
        return $article_topic->getCurrentState();
      }
    }
    return NULL;
  }

  /**
   * Collect the review cycles for this article.
   *
   * @return array
   *   Array of strings for the article's unique cycles.
   *
   * @throws \Exception
   *   Pass on exceptions thrown from nested calls.
   */
  public function getCycles(): array {
    $dates = [];
    foreach ($this->topics as $topic) {
      $date = $topic->entity->cycle->value;
      if (!in_array($date, $dates)) {
        $dates[] = $date;
      }
    }
    sort($dates);
    $cycles = [];
    foreach ($dates as $date) {
      $datetime = new \DateTime($date);
      $cycles[] = $datetime->format('F Y');
    }
    return $cycles;
  }

  /**
   * Construct a citation for the article, usable as a label.
   *
   * @return string
   *   Escaped string for the article's label.
   */
  public function getLabel(): string {
    $label = trim($this->brief_journal_title->value ?? '');
    $volume = trim($this->volume->value ?? '');
    $issue = trim($this->issue->value ?? '');
    $pagination = trim($this->pagination->value ?? '');
    $year = trim($this->year->value ?? '');
    if (!empty($issue)) {
      $volume .= '(' . $issue . ')';
    }
    if (!empty($volume)) {
      $label .= " $volume";
    }
    if (!empty($pagination)) {
      $label .= ": $pagination";
    }
    if (!empty($year)) {
      $label .= ", $year";
    }
    return $label;
  }

  /**
   * Find the articles related to this one.
   *
   * Relationships can go in both directions. Find all of them.
   *
   * @param boolean $skip_suppressed
   *   If `TRUE` don't include relationships marked as "suppress."
   * @param boolean $ids_only
   *   If `FALSE` return the `Article` entities, not just the IDs.
   *
   * @return array
   *   Array of `Article` entity objects or integer IDs.
   */
  public function getRelatedArticles(bool $skip_suppressed = TRUE, bool $ids_only = FALSE): array {
    $article_id = $this->id();
    $storage = \Drupal::entityTypeManager()->getStorage('ebms_article_relationship');
    $query = $storage->getQuery()->accessCheck(FALSE);
    $group = $query->orConditionGroup()
      ->condition('related', $article_id)
      ->condition('related_to', $article_id);
    $query->condition($group);
    $query->condition('inactivated', NULL, 'IS NULL');
    if ($skip_suppressed) {
      $query->condition('suppress', 0);
    }
    $ids = $query->execute();
    $relationships = $storage->loadMultiple($ids);
    $related_ids = [];
    foreach ($relationships as $relationship) {
      $related = $relationship->related->target_id;
      $related_to = $relationship->related_to->target_id;
      $related_ids[] = $related == $article_id ? $related_to : $related;
    }
    if ($ids_only) {
      return $related_ids;
    }
    return Article::loadMultiple($related_ids);
  }

  /**
   * Get the entity for a topic assigned to this article.
   *
   * @param int $topic_id
   *   The entity ID for the `Topic` requested.
   *
   * @return object|null
   *   Reference to an `ArticleTopic` entity if found, else `NULL`.
   */
  public function getTopic(int $topic_id): ?object {
    foreach ($this->topics as $topic) {
      $article_topic = $topic->entity;
      if ($article_topic->topic->target_id == $topic_id) {
        return $article_topic;
      }
    }
    return NULL;
  }

  /**
   * Assemble the list of topics assigned to this article.
   *
   * @return array
   *   Array of strings for the topics' names.
   */
  public function getTopics() {
    $names = [];
    foreach ($this->topics as $topic) {
      $names[] = $topic->entity->topic->entity->name->value;
    }
    sort($names);
    return $names;
  }

  /**
   * Determine whether the article was published in a core journal.
   *
   * @return bool
   *   `TRUE` if the article was published in a core journal, else `FALSE`.
   */
  public function inCoreJournal(): bool {
    static $journals = [];
    $journal_id = $this->source_journal_id->value;
    if (!array_key_exists($journal_id, $journals)) {
      $storage = \Drupal::entityTypeManager()->getStorage('ebms_journal');
      $journals[$journal_id] = FALSE;
      $query = $storage->getQuery()->accessCheck(FALSE);
      $query->condition('source_id', $journal_id);
      $ids = $query->execute();
      if (count($ids) === 1) {
        $journal = $storage->load(reset($ids));
        $journals[$journal_id] = $journal->get('core')->value;
      }
    }
    return $journals[$journal_id];
  }

  /**
   * Extract the values we need from XML fetched from NLM.
   *
   * @param string $xml
   *   XML extracted from the PubMed service.
   *
   * @return array
   *   Keyed values needed for an `Article` entity.
   *
   * @throws \Exception
   *   Ensure that the document structure matches our expectations.
   */
  public static function parse(string $xml): array {
    $root = new \SimpleXMLElement($xml);
    $tag = $root->getName();
    if ($tag !== 'PubmedArticle') {
      throw new \Exception("Expected 'PubmedArticle' but got '$tag' instead.");
    }
    $citation = $root->MedlineCitation;
    if (empty($citation)) {
      throw new \Exception('MedlineCitation block not found.');
    }
    $article = $citation->Article;
    if (empty($article)) {
      throw new \Exception("Article block not found.");
    }
    $journal = $article->Journal;
    if (empty($journal)) {
      throw new \Exception('Journal block not found.');
    }
    $journal_info = $citation->MedlineJournalInfo;
    if (empty($journal_info)) {
      throw new \Exception('MedlineJournalInfo block not found.');
    }
    $title = trim($article->ArticleTitle ?? '');
    $search_title = substr(self::normalize($title), 0, 512);
    $authors = [];
    $last_author_name = NULL;
    if (!empty($article->AuthorList->Author)) {
      foreach ($article->AuthorList->Author as $author) {
        $collective_name = trim($author->CollectiveName ?? '');
        $last_name = trim($author->LastName ?? '');
        $first_name = trim($author->ForeName ?? '');
        $initials = trim($author->Initials ?? '');
        $author_values = [];
        $display_name = '';
        if (!empty($last_name)) {
          $display_name = $author_values['last_name'] = $last_name;
        }
        if (!empty($first_name)) {
          $author_values['first_name'] = $first_name;
        }
        if (!empty($initials)) {
          $author_values['initials'] = $initials;
          $display_name .= " $initials";
        }
        if (!empty($collective_name)) {
          $author_values['collective_name'] = $collective_name;
          if (empty($display_name)) {
            $display_name = $collective_name;
          }
        }
        $author_values['display_name'] = $display_name;
        $author_values['search_name'] = $last_author_name = self::normalize($display_name);
        $authors[] = $author_values;
      }
    }

    $abstract = [];
    if (!empty($article->Abstract->AbstractText)) {
      foreach ($article->Abstract->AbstractText as $node) {
        $text = trim($node ?? '');
        if (!empty($text)) {
          $paragraph = ['paragraph_text' => $text];
          $label = trim($node['Label'] ?? '');
          if (!empty($label)) {
            $paragraph['paragraph_label'] = $label;
          }
          $abstract[] = $paragraph;
        }
      }
    }

    $journal_issue = $journal->JournalIssue;
    $pub_date = $journal_issue->PubDate;
    $pub_year = trim($pub_date->Year ?? '');
    $pub_month = trim($pub_date->Month ?? '');
    $pub_day = trim($pub_date->Day ?? '');
    $pub_season = trim($pub_date->Season ?? '');
    $medline_date = trim($pub_date->MedlineDate ?? '');
    $date_values = [];
    $year = NULL;
    if (!empty($pub_year)) {
      if (preg_match('/^\d\d\d\d$/', $pub_year)) {
        $year = (int) $pub_year;
      }
      $date_values['year'] = $pub_year;
    }
    if (!empty($medline_date)) {
      $date_values['medline_date'] = $medline_date;
      if (empty($year)) {
        $words = preg_split('/[\s,]+/', $medline_date);
        $matches = [];
        if (preg_match('/^\d{4}-(\d{4})$/', $words[0], $matches)) {
          $year = (int) $matches[1];
        }
        elseif (preg_match('/\d{4}$/', $words[0])) {
          $year = (int) $words[0];
        }
        elseif (count($words) > 1 && preg_match('/\d{4}$/', end($words))) {
          $year = (int) end($words);
        }
      }
    }
    if (!empty($pub_month)) {
      $date_values['month'] = $pub_month;
    }
    if (!empty($pub_day)) {
      $date_values['day'] = $pub_day;
    }
    if (!empty($pub_season)) {
      $date_values['season'] = $pub_season;
    }

    $types = [];
    if (!empty($article->PublicationTypeList->PublicationType)) {
      foreach ($article->PublicationTypeList->PublicationType as $type) {
        $type = trim($type ?? '');
        if (!empty($type)) {
          $types[] = $type;
        }
      }
    }

    $comments_corrections = [];
    if (!empty($citation->CommentsCorrectionsList->CommentsCorrections->PMID)) {
      foreach ($citation->CommentsCorrectionsList->CommentsCorrections->PMID as $pmid) {
        $pmid = trim($pmid ?? '');
        if (!empty($pmid)) {
          $comments_corrections[] = $pmid;
        }
      }
    }

    $values = [
      'title' => $title,
      'search_title' => $search_title,
      'authors' => $authors,
      'source' => 'Pubmed',
      'source_id' => trim($citation->PMID ?? ''),
      'source_journal_id' => trim($journal_info->NlmUniqueID ?? ''),
      'source_status' => trim($citation['Status'] ?? ''),
      'journal_title' => trim($journal->Title ?? ''),
      'brief_journal_title' => trim($journal_info->MedlineTA ?? ''),
      'volume' => trim($journal_issue->Volume ?? ''),
      'issue' => trim($journal_issue->Issue ?? ''),
      'pagination' => trim($article->Pagination->MedlinePgn ?? ''),
      'abstract' => $abstract,
      'pub_date' => $date_values,
      'year' => $year,
      'types' => $types,
      'comments_corrections' => $comments_corrections,
      'last_author_name' => $last_author_name,
    ];
    if (empty($values['source_id'])) {
      throw new \Exception('Missing PMID.');
    }
    return $values;
  }

  /**
   * Update the `Article` entity with fresh values from the source.
   *
   * We don't save the entity. That's up to the caller, which may
   * want to do other things first.
   *
   * @param array $values
   *   Fresh set of keyed values.
   * @param string $when
   *   Date/time the refresh happened.
   *
   * @return bool
   *   'TRUE` if any of the field values were updated, else `FALSE`.
   */
  public function refresh(array $values, string $when): bool {

    // Check the easy, single-valued fields.
    $changed = FALSE;
    $simple = [
      'title', 'source', 'source_id', 'source_journal_id', 'source_status',
      'journal_title', 'brief_journal_title', 'volume', 'issue', 'pagination',
      'year',
    ];
    foreach ($simple as $name) {
      $old = $this->get($name)->value;
      $new = $values[$name];
      if (!empty($old) || !empty($new)) {
        if ($old != $new) {
          $this->set($name, $new);
          $changed = TRUE;
          if ($name === 'title') {
            $this->set('search_title', '');
          }
        }
      }
    }

    // Check the more complicated fields.
    $name_properties = [
      'last_name',
      'first_name',
      'initials',
      'collective_name',
    ];
    $complex = [
      ['authors', $name_properties, TRUE],
      ['abstract', ['paragraph_text', 'paragraph_label'], TRUE],
      ['pub_date', ['year', 'month', 'day', 'season', 'medline_date'], FALSE],
      ['types', ['value'], TRUE],
    ];
    foreach ($complex as list($name, $properties, $multiple)) {
      if ($this->changed($name, $properties, $values, $multiple)) {
        $this->set($name, $values[$name]);
        $changed = TRUE;
        if ($name === 'authors') {
          $this->set('last_author_name', '');
        }
      }
    }

    // If anything changed, remember when it happened.
    if ($changed) {
      $this->set('update_date', $when);
    }
    return $changed;
  }

  /**
   * Normalize an author or title string for searching.
   *
   * Remove non-ASCII characters and normalize spaces.
   */
  public static function normalize($string): string {
    $old_locale = setlocale(LC_CTYPE, 0);
    setlocale(LC_CTYPE, 'en_US.utf8');
    $normalized = preg_replace('/\s+/', ' ', trim($string ?? ''));
    $normalized = iconv('UTF-8', 'ASCII//TRANSLIT', $normalized);
    setlocale(LC_CTYPE, $old_locale);
    return $normalized;
  }

  /**
   * Assemble the display name for one of the authors.
   */
  public static function getAuthorDisplayName(Author $author): string {
    if (!empty($author->last_name)) {
      $name = $author->last_name;
      if (!empty($author->initials)) {
        $name .= ' ' . $author->initials;
      }
    }
    else {
      $name = $author->collective_name;
    }
    return $name;
  }

  /**
   * Find an `ArticleTopic` entity using its topic and board names.
   *
   * @param string $board_name
   *   Name of the board which owns this topic.
   * @param string $topic_name
   *   Name of the topic we're looking for.
   *
   * @return ArticleTopic|null
   *   Object representing one of the topics associated with this article.
   */
  public function findArticleTopic(string $board_name, string $topic_name): ?ArticleTopic {
    foreach ($this->topics as $topic) {
      $article_topic = $topic->entity;
      $topic = $article_topic->topic->entity;
      if ($topic->getName() === $topic_name) {
        if ($topic->board->entity->getName() === $board_name) {
          return $article_topic;
        }
      }
    }
    return NULL;
  }

  /**
   * Find articles for which at least one topic has a chance of being used.
   *
   * @return array
   *   PubMed IDs indexed by the `Article` entity IDs.
   */
  public static function active(): array {

    // Find the states we don't want.
    $rejection_states_text_ids = [
      'reject_journal_title',
      'reject_init_review',
      'reject_bm_review',
      'reject_full_review',
      'full_end',
    ];
    $storage = \Drupal::entityTypeManager()->getStorage('taxonomy_term');
    $query = $storage->getQuery()->accessCheck(FALSE);
    $query->condition('vid', 'states');
    $query->condition('field_text_id', $rejection_states_text_ids, 'IN');
    $rejection_state_ids = $query->execute();

    // Find the decisions we don't want.
    $unwanted_decisions = [
      'Not cited',
      'Cited (citation only)',
      'Cited (legacy)',
      'Text approved',
    ];
    $query = $storage->getQuery()->accessCheck(FALSE);
    $query->condition('vid', 'board_decisions');
    $query->condition('name', $unwanted_decisions, 'IN');
    $unwanted_decision_ids = $query->execute();

    // Create the query.
    $query = \Drupal::database()->select('ebms_article', 'article');
    $query->addField('article', 'id');
    $query->addExpression('CONVERT(article.source_id, UNSIGNED INTEGER)', 'pmid');
    $query->join('ebms_state', 'state', 'state.article = article.id');
    $query->condition('state.current', 1);
    $query->condition('state.value', $rejection_state_ids, 'NOT IN');
    $query->join('taxonomy_term__field_text_id', 'state_text_id', 'state_text_id.entity_id = state.value');
    $query->leftJoin('ebms_state__decisions', 'state_decisions', 'state_decisions.entity_id = state.id');
    $group = $query->orConditionGroup()
      ->condition('state_decisions.decisions_decision', $unwanted_decision_ids, 'NOT IN')
      ->isNull('state_decisions.decisions_decision');
    $query->condition($group);
    $query->orderBy('pmid');
    $active = [];
    foreach ($query->execute() as $row) {
      $active[$row->id] = $row->pmid;
    }
    return $active;
  }
}
