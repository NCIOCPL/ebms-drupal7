<?php

namespace Drupal\ebms_review\Form;

use Drupal\Core\Database\Query\PagerSelectExtender;
use Drupal\Core\Database\Query\SelectInterface;
use Drupal\Core\Entity\Query\QueryInterface;
use Drupal\Core\Form\FormBase;
use Drupal\Core\Form\FormStateInterface;
use Drupal\Core\Url;
use Drupal\ebms_article\Entity\Article;
use Drupal\ebms_article\Entity\ArticleTopic;
use Drupal\ebms_board\Entity\Board;
use Drupal\ebms_core\Entity\SavedRequest;
use Drupal\ebms_import\Entity\Batch;
use Drupal\ebms_topic\Entity\Topic;
use Symfony\Component\HttpFoundation\RedirectResponse;

/**
 * Interface for batch "publication" of articles.
 *
 * The "Published" state value is used for articles which are ready for a
 * board manager to review from the articles' abstracts for the specified
 * topic. This allows the librarians to approve individual article/topic
 * combinations one by one without having them added to the board manager's
 * abstract review queue as the approvals are made, but instead mark a batch
 * for "publication" to that queue all at once. It is an odd name, but the
 * users got used to it when using the original Visual Basic application,
 * and requested that the name be retained for this state.
 *
 * @ingroup ebms
 */
class PublicationForm extends FormBase {

  /**
   * {@inheritdoc}
   */
  public function getFormId(): string {
    return 'article_publication';
  }

  /**
   * {@inheritdoc}
   */
  public function buildForm(array $form, FormStateInterface $form_state, $queue_id = 0): array | RedirectResponse {

    // If we don't have an object for tracking this queue, create one.
    if (empty($queue_id)) {
      $queue = SavedRequest::saveParameters('publish queue', ['sort' => 'author', 'per-page' => 10]);
      $route = 'ebms_review.publish';
      $parms = ['queue_id' => $queue->id()];
      return new RedirectResponse(Url::fromRoute($route, $parms)->toString());
    }

    // Make the picklists.
    $cycles = Batch::cycles();
    $boards = Board::boards();
    $topics = Topic::topics();

    // Build up the query to identify the queue's articles.
    $values = SavedRequest::loadParameters($queue_id);
    $per_page = $values['per-page'] ?? 10;
    $queued_json = $values['queued'] ?? '[]';
    $queued = json_decode($queued_json, TRUE);
    $query = $this->makeQuery($values);
    $count_query = $query->countQuery();
    $count = $count_query->execute()->fetchField();
    if ($values['sort'] === 'author') {
      $query->leftJoin('ebms_article__authors', 'author', 'author.entity_id = article.id AND author.delta = 0');
      $query->orderBy('author.authors_search_name');
      $query->orderBy('article.title');
      $query->addField('author', 'authors_search_name');
      $query->addField('article', 'title');
    }
    else {
      $query->leftJoin('ebms_journal', 'journal', 'journal.source_id = article.source_journal_id');
      $query->orderBy('journal.core', 'DESC');
      $query->orderBy('article.journal_title');
      $query->orderBy('article.title');
      $query->addField('journal', 'core');
      $query->addField('article', 'journal_title');
      $query->addField('article', 'title');
    }
    if ($per_page === 'all') {
      $start = 1;
    }
    else {
      $query = $query->extend(PagerSelectExtender::class);
      $query->limit($per_page);
      $page = $this->getRequest()->get('page') ?? 0;
      $start = 1 + $page * $per_page;
    }

    // Load the articles and prepare them for rendering in the queue.
    $articles = Article::loadMultiple($query->execute()->fetchCol());
    $items = [];
    foreach ($articles as $article) {
      $items[] = $this->createArticleRenderArray($article, $values, $queued);
    }

    // Assemble the form.
    $form = [
      '#attached' => ['library' => ['ebms_review/unpublished-articles']],
      '#cache' => ['max-age' => 0],
      '#title' => 'Publish Articles',
      'queue-id' => [
        '#type' => 'hidden',
        '#value' => $queue_id,
      ],
      'filtering' => [
        '#type' => 'details',
        '#title' => 'Filtering',
        'cycle' => [
          '#type' => 'select',
          '#title' => 'Review Cycle',
          '#description' => 'Restrict the articles to those in the selected review cycle.',
          '#options' => $cycles,
          '#default_value' => $values['cycle'] ?? '',
          '#empty_value' => '',
        ],
        'board' => [
          '#type' => 'select',
          '#title' => 'Editorial Board',
          '#description' => 'Restrict the articles to those to be reviewed for the selected PDQ® editorial board.',
          '#options' => $boards,
          '#default_value' => $values['board'] ?? '',
          '#empty_value' => '',
        ],
        'topic' => [
          '#type' => 'select',
          '#title' => 'Summary Topic',
          '#description' => 'Restrict the articles to those to be reviewed for the selected topic.',
          '#options' => $topics,
          '#default_value' => $values['topic'] ?? '',
          '#empty_value' => '',
        ],
      ],
      'options' => [
        '#type' => 'details',
        '#title' => 'Options',
        'sort' => [
          '#type' => 'radios',
          '#title' => 'Sort Order',
          '#required' => TRUE,
          '#description' => 'Specify in which order the articles should be displayed.',
          '#options' => [
            'author' => "By first author's name",
            'journal' => 'By journal title, core journals first',
          ],
          '#default_value' => $values['sort'] ?? 'author',
        ],
        'per-page' => [
          '#type' => 'radios',
          '#title' => 'Articles Per Page',
          '#description' => 'Publication selections will be remembered across pages.',
          '#required' => TRUE,
          '#options' => [
            '10' => '10',
            '25' => '25',
            '50' => '50',
            '100' => '100',
            'all' => 'All',
          ],
          '#default_value' => $per_page,
        ],
      ],
      'filter' => [
        '#type' => 'submit',
        '#value' => 'Filter',
      ],
      'reset' => [
        '#type' => 'submit',
        '#value' => 'Reset',
        '#submit' => ['::cancelSubmit'],
        '#limit_validation_errors' => [],
      ],
      'select all' => [
        '#type' => 'submit',
        '#value' => 'Select All',
      ],
      'clear all' => [
        '#type' => 'submit',
        '#value' => 'Clear All',
      ],
      'batch publish' => [
        '#type' => 'submit',
        '#value' => 'Batch Publish',
        '#states' => [
          'invisible' => [
            ':input[name="queued"]' => ['value' => '[]'],
          ],
        ],
    ],
      'queued' => [
        '#type' => 'textfield',
        '#attributes' => ['class' => ['hidden'], 'maxlength' => ''],
        '#value' => $queued_json,
        '#ajax' => [
          'callback' => '::changesCallback',
          'event' => 'change',
        ],
      ],
      'queue-wrapper' => [
        '#type' => 'container',
        '#attributes' => ['id' => 'queue-list'],
        'queue' => [
          '#theme' => 'item_list',
          '#title' => "Unpublished Articles ($count)",
          '#empty' => 'No unpublished articles match the filtering criteria.',
          '#list_type' => 'ol',
          '#items' => $items,
          '#attributes' => ['start' => $start],
        ]
      ],
    ];

    // Add pager if appropriate and return the form's render array.
    if ($per_page !== 'all') {
      $form['pager'] = ['#type' => 'pager'];
    }
    return $form;
  }

  /**
   * {@inheritdoc}
   */
  public function submitForm(array &$form, FormStateInterface $form_state) {

    // Find out which button was clicked.
    $trigger = $form_state->getTriggeringElement()['#value'];

    // Handle requests to update all of the button settings.
    if ($trigger === 'Select All' || $trigger === 'Clear All') {
      $queue_id = $form_state->getValue('queue-id');
      $queue = SavedRequest::load($queue_id);
      $values = $queue->getParameters();
      $queued = [];
      if ($trigger === 'Select All') {
        $query = $this->makeQuery($values);
        $articles = Article::loadMultiple($query->execute()->fetchCol());
        foreach ($articles as $article_id => $article) {
          foreach ($this->getUnpublishedTopics($article) as $article_topic_id => $article_topic) {
            $queued[] = "$article_id-$article_topic_id";
          }
        }
      }

      // Update the saved queue values and reload the page.
      $values['queued'] = json_encode($queued);
      $queue->set('parameters', json_encode($values));
      $queue->save();
      $form_state->setRedirect('ebms_review.publish', ['queue_id' => $queue->id()]);
    }

    // Re-build the queue, apply publishing decisions if so requested.
    if ($trigger === 'Filter' || $trigger === 'Batch Publish') {
      $values = $form_state->getValues();
      unset($values['queue-id']);
      if ($trigger === 'Batch Publish') {
        $uid = $this->currentUser()->id();
        $now = date('Y-m-d H:i:s');
        $queued = json_decode($values['queued'], TRUE);
        $articles = [];
        foreach ($queued as $key) {
          list($article_id, $article_topic_id) = explode('-', $key);
          $articles[$article_id][] = $article_topic_id;
        }
        foreach ($articles as $article_id => $article_topic_ids) {
          $article = Article::load($article_id);
          foreach ($article_topic_ids as $article_topic_id) {
            $article_topic = ArticleTopic::load($article_topic_id);
            $topic_id = $article_topic->topic->target_id;
            $article->addState('published', $topic_id, $uid, $now);
          }
        }
      }

      // Create a new queue entity and reload the page.
      $values['queued'] = '[]';
      $queue = SavedRequest::saveParameters('publish queue', $values);
      $form_state->setRedirect('ebms_review.publish', ['queue_id' => $queue->id()]);
    }
  }

  /**
   * Reset the form to the default values
   *
   * @param array $form
   *   Form settings.
   * @param \Drupal\Core\Form\FormStateInterface $form_state
   *   Form values.
   */
  public function cancelSubmit(array &$form, FormStateInterface $form_state) {
    $this->redirect('ebms_review.publish');
  }

  /**
   * Update the queued publications.
   *
   * We have to pull the queued changes directly from the field because
   * Drupal hasn't yet updated the processed values array.
   *
   * @param array $form
   *   An associative array containing the structure of the form.
   * @param \Drupal\Core\Form\FormStateInterface $form_state
   *   The object tracking the current state of the form.
   */
  public function changesCallback(array &$form, FormStateInterface $form_state) {
    $queued_json = $form_state->getUserInput()['queued'] ?? '[]';
    $queue_id = $form_state->getValue('queue-id');
    $request = SavedRequest::load($queue_id);
    $parameters = $request->getParameters();
    $parameters['queued'] = $queued_json;
    $parameters_json = json_encode($parameters);
    $request->set('parameters', $parameters_json);
    $request->save();
  }

  /**
   * Create the render array for an article and its unpublished topics.
   *
   * @param Article $article
   *   The `Article` we need to display.
   * @param array $values
   *   Array with filtering options for the queue.
   * @param array $queued
   *   Array of strings holding article ID/article topic ID pairs
   */
  private function createArticleRenderArray(Article $article, array $values, array $queued) {

    // Find the common article information we need to assemble for all topics.
    $article_id = $article->id();
    $authors = $article->getAuthors(3);

    // Find the unpublished topics which meet the filtering criteria.
    $topics = [];
    foreach ($this->getUnpublishedTopics($article) as $article_topic_id => $article_topic) {
      if (!empty($values['cycle']) && $values['cycle'] != $article_topic->cycle->value) {
        continue;
      }
      if (!empty($values['topic']) && $values['topic'] != $article_topic->topic->target_id) {
        continue;
      }
      if (!empty($values['board']) && $values['board'] != $article_topic->topic->entity->board->target_id) {
        continue;
      }
      $topic = $article_topic->topic->entity;
      $key = "$article_id-$article_topic_id";
      $checkbox_id = "unpublished-topic-$key";
      $name = $topic->name->value;
      $board = $topic->board->entity->name->value;
      $topics[$checkbox_id] = [
        '#type' => 'checkbox',
        '#checked' => in_array($key, $queued),
        '#id' => $checkbox_id,
        '#title' => "$name ($board)",
      ];
    }

    // Sort the topics by name.
    usort($topics, function($a, $b) {
      return $a['#title'] <=> $b['#title'];
    });

    // Assemble and eturn the render array.
    return [
      '#theme' => 'unpublished_article',
      '#article' => [
        'id' => $article->id(),
        'authors' => implode(', ', $authors),
        'title' => $article->title->value,
        'publication' => $article->getLabel(),
        'pmid' => $article->source_id->value,
        'topics' => $topics,
      ],
    ];
  }

  /**
   * Find the topics in the article which have passed the librarian's review.
   *
   * @param Article $article
   *   The entity whose topics we need to examine.
   *
   * @return array
   *   Nested array of ArticleTopic entity IDs indexed by Article entity IDs.
   */
  private function getUnpublishedTopics(Article $article): array {
    $article_topics = [];
    foreach ($article->topics as $article_topic) {
      $article_topic = $article_topic->entity;
      $topic_state = $article_topic->getCurrentState();
      if ($topic_state->value->entity->field_text_id->value === 'passed_init_review') {
        $article_topics[$article_topic->id()] = $article_topic;
      }
    }
    return $article_topics;
  }

  /**
   * Create the basic query for finding unpublished topics.
   *
   * Way too slow using the entity query API with a fully loaded system ,
   * so this has been rewritten to use the database API.
   *
   * @param array $values
   *   Array with filtering options for the queue.
   *
   * @return SelectInterface
   *   The base query for finding articles for the queue.
   */
  private function makeQuery(array $values): SelectInterface {
    $query = \Drupal::database()->select('taxonomy_term__field_text_id', 'text_id');
    $query->condition('bundle', 'states');
    $query->condition('field_text_id_value', 'passed_init_review');
    $query->addField('text_id', 'entity_id');
    $state_id = $query->execute()->fetchField();
    $query = \Drupal::database()->select('ebms_state', 'state');
    $query->join('ebms_article', 'article', 'article.id = state.article');
    $query->condition('state.current', 1);
    $query->condition('state.value', $state_id);
    if (!empty($values['topic'])) {
      $query->condition('state.topic', $values['topic']);
    }
    elseif (!empty($values['board'])) {
      $query->condition('state.board', $values['board']);
    }
    if (!empty($values['cycle'])) {
      $query->join('ebms_article__topics', 'topics', 'topics.entity_id = article.id');
      $query->join('ebms_article_topic', 'topic', 'topic.id = topics.topics_target_id');
      $query->condition('topic.cycle', $values['cycle']);
    }
    else {
      $query->condition('state.entered', Article::CONVERSION_DATE, '>');
    }
    $query->addField('article', 'id', 'article_id');
    $query->distinct();
    return $query;
  }

}
