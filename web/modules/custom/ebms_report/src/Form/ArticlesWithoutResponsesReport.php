<?php

namespace Drupal\ebms_report\Form;

use Drupal\Core\Form\FormBase;
use Drupal\Core\Form\FormStateInterface;
use Drupal\Core\Url;
use Drupal\ebms_article\Entity\Article;
use Drupal\ebms_board\Entity\Board;
use Drupal\ebms_core\Entity\SavedRequest;
use Drupal\ebms_topic\Entity\Topic;
use Drupal\user\Entity\User;
use Symfony\Component\HttpFoundation\Response;

/**
 * Show articles assigned for review which have no responses yet.
 *
 * @ingroup ebms
 */
class ArticlesWithoutResponsesReport extends FormBase {

  /**
   * {@inheritdoc}
   */
  public function getFormId(): string {
    return 'articles_without_responses_report_form';
  }

  /**
   * {@inheritdoc}
   */
  public function buildForm(array $form, FormStateInterface $form_state, int $report_id = 0): array|Response {

    // Show the board member version if requested.
    $params = empty($report_id) ? [] : SavedRequest::loadParameters($report_id);
    $version = $this->getRequest()->query->get('version');
    if (!empty($params) && $version === 'member') {
      $report = $this->report($params, TRUE);
      $page = \Drupal::service('renderer')->render($report);
      $response = new Response($page);
      return $response;
    }

    // Collect some values for the report request.
    $user = User::load($this->currentUser()->id());
    $board = $form_state->getValue('board', $params['board'] ?? Board::defaultBoard($user));
    $topics = empty($board) ? [] : Topic::topics($board);
    $selected_topics = $params['topics'] ?? [];
    if (!empty($selected_topics)) {
      foreach ($selected_topics as $topic) {
        if (!array_key_exists($topic, $topics)) {
          $selected_topics = [];
          break;
        }
      }
    }
    $reviewers = empty($board) ? [] : $this->reviewers($board);
    $selected_reviewers = $params['reviewers'] ?? [];
    if (!empty($selected_reviewers)) {
      foreach ($selected_reviewers as $reviewer) {
        if (!array_key_exists($reviewer, $reviewers)) {
          $selected_reviewers = [];
          break;
        }
      }
    }

    // Assemble the fields for the form.
    $form = [
      '#title' => 'Articles Without Responses',
      '#attached' => ['library' => ['ebms_report/no-responses']],
      '#cache' => ['max-age' => 0],
      'filters' => [
        '#type' => 'details',
        '#title' => 'Filters',
        'board' => [
          '#type' => 'select',
          '#title' => 'Editorial Board',
          '#description' => 'Select the board for which the report is to be generated.',
          '#required' => TRUE,
          '#options' => Board::boards(),
          '#default_value' => $board,
          '#empty_value' => '',
          '#ajax' => [
            'callback' => '::boardChangeCallback',
            'wrapper' => 'board-controlled',
            'event' => 'change',
          ],
        ],
        'board-controlled' => [
          '#type' => 'container',
          '#attributes' => ['id' => 'board-controlled'],
          'topics' => [
            '#type' => 'select',
            '#title' => 'Summary Topic(s)',
            '#description' => 'Optionally select one or more topics to narrow the report.',
            '#options' => $topics,
            '#multiple' => TRUE,
            '#default_value' => $selected_topics,
            '#empty_value' => '',
          ],
          'reviewers' => [
            '#type' => 'select',
            '#title' => 'Board Member(s)',
            '#description' => 'Optionally select one or more reviewers to narrow the report.',
            '#options' => $reviewers,
            '#multiple' => TRUE,
            '#default_value' => $selected_reviewers,
            '#empty_value' => '',
          ],
        ],
        'packet-assigned-date' => [
          '#type' => 'container',
          '#attributes' => ['class' => ['inline-fields']],
          '#title' => 'Packet Assigned Date Range',
          '#description' => 'Only show statistics for reviews assigned during the specified date range.',
          'assigned-start' => [
            '#type' => 'date',
            '#default_value' => $params['assigned-start'] ?? '',
          ],
          'assigned-end' => [
            '#type' => 'date',
            '#default_value' => $params['assigned-end'] ?? '',
          ],
        ],
        'tags' => [
          '#type' => 'checkboxes',
          '#title' => 'Additional Filtering',
          '#options' => [
            'high-priority' => 'High priority',
          ],
          '#default_value' => $params['tags'],
          '#description' => 'Restrict the report to article-topic combinations which have been tagged <em>high priority</em>.',
        ],
      ],
      'option-box' => [
        '#type' => 'details',
        '#title' => 'Display Options',
        'sort' => [
          '#type' => 'radios',
          '#title' => 'Order By',
          '#description' => 'Choose the sequence in which the report information should be presented.',
          '#options' => [
            'author' => 'First article author',
            'topic' => 'Topic',
            'journal' => 'Journal brief title (core journals first)',
          ],
          '#default_value' => $params['sort'] ?? 'author',
        ],
      ],
      'submit' => [
        '#type' => 'submit',
        '#value' => 'Submit',
      ],
      'reset' => [
        '#type' => 'submit',
        '#value' => 'Reset',
        '#submit' => ['::resetSubmit'],
        '#limit_validation_errors' => [],
      ],
    ];

    // If we have a report request, generate and add it.
    if (empty($form_state->getValues()) && !empty($params)) {
      $opts = ['query' => ['version' => 'member']];
      $form['member-version-button'] = [
        '#type' => 'link',
        '#url' => Url::fromRoute('ebms_report.articles_without_responses', ['report_id' => $report_id], $opts),
        '#title' => 'Member Version',
        '#attributes' => ['class' => ['usa-button'], 'target' => '_blank'],
      ];
      $form['report'] = $this->report($params);
    }
    return $form;
  }

  /**
   * {@inheritdoc}
   */
  public function submitForm(array &$form, FormStateInterface $form_state) {
    $params = $form_state->getValues();
    $request = SavedRequest::saveParameters('articles without responses report', $params);
    $form_state->setRedirect('ebms_report.articles_without_responses', ['report_id' => $request->id()]);
  }

  /**
   * Create a fresh form.
   */
  public function resetSubmit(array &$form, FormStateInterface $form_state) {
    $form_state->setRedirect('ebms_report.articles_without_responses');
  }

  /**
   * Fill in the portion of the form driven by board selection.
   *
   * @param array $form
   *   Render array we are adjusting.
   * @param FormStateInterface $form_state
   *   Access to the form's values.
   */
  public function boardChangeCallback(array &$form, FormStateInterface $form_state) {
    return $form['filters']['board-controlled'];
  }

  /**
   * Show articles for which reviews are still needed.
   *
   * @param array $values
   *   Values entered by the user on the request form.
   * @param string $member_version
   *   If `TRUE` produced a stripped-down version of the report.
   *
   * @return array
   *   Render array for the report.
   */
  private function report(array $params, bool $member_version = FALSE): array {

    // Not suitable for Drupal's entity query API.
    ebms_debug_log('top of ArticlesWithoutResponses::report()');
    $high_priority_id = $this->high_priority_id();
    $high_priority_only = FALSE;
    $query = \Drupal::database()->select('ebms_packet', 'packet');
    $query->join('ebms_topic', 'topic', 'topic.id = packet.topic');
    $query->join('ebms_packet__articles', 'articles', 'articles.entity_id = packet.id');
    $query->join('ebms_packet_article', 'packet_article', 'packet_article.id = articles.articles_target_id');
    $query->join('ebms_article', 'article', 'article.id = packet_article.article');
    $query->join('ebms_article__topics', 'topics', 'topics.entity_id = article.id');
    $query->join('ebms_article_topic', 'article_topic', 'article_topic.id = topics.topics_target_id AND article_topic.topic = topic.id');
    $query->join('ebms_state', 'current_state', 'current_state.article = article.id AND current_state.topic = topic.id');
    $query->join('ebms_packet__reviewers', 'reviewers', 'reviewers.entity_id = packet.id');
    $query->leftJoin('ebms_packet_article__reviews', 'reviews', 'reviews.entity_id = packet_article.id');
    $query->leftJoin('ebms_article__authors', 'author', 'author.entity_id = article.id AND author.delta = 0');

    // Apply the filtering.
    $query->condition('packet.active', 1);
    if (!empty($params['tags']['high-priority'])) {
      $query->join('ebms_article_topic__tags', 'tags' ,'tags.entity_id = article_topic.id');
      $query->join('ebms_article_tag', 'article_tag', 'article_tag.id = tags.tags_target_id');
      $query->condition('article_tag.tag', $high_priority_id);
      $high_priority_only = TRUE;
    }
    if (!empty($params['assigned-start'])) {
      $query->condition('packet.created', $params['assigned-start'], '>=');
    }
    if (!empty($params['assigned-end'])) {
      $end = $params['assigned-end'];
      if (strlen($end) === 10) {
        $end .= ' 23:59:59';
      }
      $query->condition('packet.created', $end, '<=');
    }
    if (!empty($params['topics'])) {
      $query->condition('packet.topic', $params['topics'], 'IN');
    }
    else {
      $query->condition('topic.board', $params['board']);
    }
    if (!empty($params['reviewers'])) {
      $query->join('ebms_packet__reviewers', 'reviewers', 'reviewers.entity_id = packet.id');
      $query->condition('reviewers.reviewers_target_id', $params['reviewers'], 'IN');
    }
    $query->condition('packet_article.dropped', 0);
    $query->isNull('reviews.reviews_target_id');
    $query->condition('current_state.current', 1);
    $query->condition('current_state.value', $this->excluded_states(), 'NOT IN');

    // Add the fields we need in the results set.
    $query->addField('topic', 'name', 'topic_name');
    $query->addField('author', 'authors_display_name');
    $query->addField('article', 'title');
    $query->addField('article', 'id', 'article_id');
    $query->addField('packet', 'id', 'packet_id');
    $query->addField('packet', 'created', 'assigned');
    $query->distinct();

    // Apply the appropriate sort order for the rows.
    $topic_primary_sort = FALSE;
    switch ($params['sort']) {
      case 'topic':
        $query->orderBy('topic.name');
        $topic_primary_sort = TRUE;
        $query->addField('topic', 'id', 'topic_id');
        $topic_ids = [];
        break;
      case 'journal':
        $query->leftJoin('ebms_journal', 'journal', 'journal.source_id = article.source_journal_id');
        $query->addField('journal', 'core');
        $query->addField('article', 'journal_title');
        $query->orderBy('journal.core', 'DESC');
        $query->orderBy('article.journal_title');
        break;
    }
    $query->orderBy('author.authors_display_name');
    $query->orderBy('article.title');
    if (!$topic_primary_sort) {
      $query->orderBy('topic.name');
    }

    // Collect the data from the query.
    $article_ids = [];
    $packet_reviewers = [];
    $rows = [];
    ebms_debug_log('query: ' . (string) $query);
    foreach ($query->execute() as $row) {
      if ($topic_primary_sort) {
        $topic_ids[$row->topic_id] = $row->topic_id;
        if (!array_key_exists($row->topic_id, $rows)) {
          $rows[$row->topic_id] = [
            'name' => $row->topic_name,
            'articles' => [],
          ];
        }
        if (!array_key_exists($row->article_id, $rows[$row->topic_id]['articles'])) {
          $rows[$row->topic_id]['articles'][$row->article_id] = [
            'article_id' => $row->article_id,
            'packets' => [],
          ];
        }
        $rows[$row->topic_id]['articles'][$row->article_id]['packets'][] = [
          'id' => $row->packet_id,
          'assigned' => substr($row->assigned, 0, 10),
        ];
      }
      else {
        if (!array_key_exists($row->article_id, $rows)) {
          $rows[$row->article_id] = [
            'article_id' => $row->article_id,
            'packets' => [],
          ];
        }
        $rows[$row->article_id]['packets'][] = [
          'id' => $row->packet_id,
          'assigned' => substr($row->assigned, 0, 10),
          'topic' => $row->topic_name,
          'topic_id' => $row->topic_id,
        ];
      }
      $article_ids[$row->article_id] = $row->article_id;
      $packet_reviewers[$row->packet_id] = [];
    }

    // Find out which reviewers were assigned to each packet.
    $reviewer_ids = [];
    if (!empty($packet_reviewers)) {
      $query = \Drupal::database()->select('users_field_data', 'user');
      $query->join('ebms_packet__reviewers', 'reviewer', 'reviewer.reviewers_target_id = user.uid');
      $query->condition('reviewer.entity_id', array_keys($packet_reviewers), 'IN');
      $query->addField('user', 'name', 'reviewer_name');
      $query->addField('reviewer', 'entity_id', 'packet_id');
      $query->addField('user', 'uid', 'reviewer_id');
      $query->orderBy('user.name');
      foreach ($query->execute() as $row) {
        $packet_reviewers[$row->packet_id][] = $row->reviewer_name;
        $reviewer_ids[$row->reviewer_id] = $row->reviewer_id;
      }
    }

    // Get the article values.
    $articles = $this->loadArticles($article_ids, !$member_version);
    $high_priority = [];
    if (!$high_priority_only && !empty($articles)) {
      $query = \Drupal::database()->select('ebms_article_topic', 'article_topic');
      $query->join('ebms_article__topics', 'topics', 'topics.topics_target_id = article_topic.id');
      $query->join('ebms_article_topic__tags', 'tags' ,'tags.entity_id = article_topic.id');
      $query->join('ebms_article_tag', 'article_tag', 'article_tag.id = tags.tags_target_id');
      $query->condition('topics.entity_id', $article_ids, 'IN');
      $query->condition('article_tag.tag', $high_priority_id);
      $query->addField('topics', 'entity_id', 'article_id');
      $query->addField('article_topic', 'topic', 'topic_id');
      foreach ($query->execute() as $row) {
        if (!array_key_exists($row->article_id, $high_priority)) {
          $high_priority[$row->article_id] = [];
        }
        $high_priority[$row->article_id][$row->topic_id] = TRUE;
      }
    }

    // Assemble the render array for the report.
    $reviewer_count = count($reviewer_ids);
    $article_count = count($article_ids);
    $packet_count = count($packet_reviewers);
    $reviewer_s = $reviewer_count === 1 ? '' : 's';
    $packet_s = $packet_count === 1 ? '' : 's';
    $article_s = $article_count === 1 ? '' : 's';
    $title = "$article_count Article$article_s in $packet_count Packet$packet_s Assigned to $reviewer_count Reviewer$reviewer_s";
    if ($topic_primary_sort) {
      $topic_count = count($topic_ids);
      $topic_s = $topic_count === 1 ? '' : 's';
      $title = "$topic_count Topic$topic_s for $title";
    }
    $report = [
      '#theme' => $member_version ? 'no_responses_board_member_version' : 'articles_without_reviews',
      '#title' => $title,
      '#articles' => [],
      '#topics' => [],
    ];
    foreach ($rows as $key => $row) {
      if ($topic_primary_sort) {
        $topic_articles = [];
        foreach ($row['articles'] as $topic_article) {
          $article_id = $topic_article['article_id'];
          $article = $articles[$article_id];
          $article['high_priority'] = $high_priority_only || !empty($high_priority[$article_id][$key]);
          $packets = [];
          foreach ($topic_article['packets'] as $packet) {
            $packets[] = [
              'assigned' => $packet['assigned'],
              'reviewers' => $packet_reviewers[$packet['id']],
            ];
          }
          $article['packets'] = $packets;
          $topic_articles[] = $article;
        }
        $report['#topics'][] = [
          'name' => $row['name'],
          'articles' => $topic_articles,
        ];
      }
      else {
        $article_id = $row['article_id'];
        $article = $articles[$article_id];
        $packets = [];
        foreach ($row['packets'] as $packet) {
          $packets[] = [
            'topic' => $packet['topic'],
            'high_priority' => $high_priority_only || !empty($high_priority[$article_id][$key]),
            'assigned' => $packet['assigned'],
            'reviewers' => $packet_reviewers[$packet['id']],
          ];
        }
        $article['packets'] = $packets;
        $report['#articles'][] = $article;
      }
    }
    ebms_debug_log('report assembled');
    return $report;
  }

  /**
   * Create the picklist options for reviewer belonging to a specific board.
   *
   * @param int $board_id
   *   Entity ID for the board whose members we're looking for.
   *
   * @return array
   *   Sorted reviewer names indexed by user ID.
   */
  private function reviewers(int $board_id): array {
    $storage = \Drupal::entityTypeManager()->getStorage('user');
    $query = $storage->getQuery()->accessCheck(FALSE);
    $query->condition('boards', $board_id);
    $query->condition('roles', 'board_member');
    $query->sort('name');
    $reviewers = [];
    foreach ($storage->loadMultiple($query->execute()) as $reviewer) {
      $reviewers[$reviewer->id()] = $reviewer->name->value;
    }
    return $reviewers;
  }

  /**
   * Find the states we want to exclude from the report.
   *
   * @return array
   *   Entity IDs for states we avoid when selecting unreviewed aritlces.
   */
  private function excluded_states(): array {
    $query = \Drupal::database()->select('taxonomy_term_data', 'term');
    $query->join('taxonomy_term__field_text_id', 'term_text_id', 'term_text_id.entity_id = term.tid');
    $query->join('taxonomy_term__field_sequence', 'sequence', 'sequence.entity_id = term.tid');
    $query->condition('term.vid', 'states');
    $query->condition('term_text_id.field_text_id_value', 'fyi');
    $query->addField('term', 'tid', 'tid');
    $query->addField('sequence', 'field_sequence_value', 'sequence');
    $fyi = $query->execute()->fetchObject();
    $query = \Drupal::database()->select('taxonomy_term_data', 'term');
    $query->join('taxonomy_term__field_sequence', 'sequence', 'sequence.entity_id = term.tid');
    $query->condition('term.vid', 'states');
    $query->condition('sequence.field_sequence_value', $fyi->sequence, '>');
    $query->addField('term', 'tid', 'tid');
    $excluded_states = [$fyi->tid];
    foreach ($query->execute() as $row) {
      $excluded_states[] = $row->tid;
    }
    return $excluded_states;
  }

  /**
   * Find the article tag for high-priority article/topic combinations.
   */
  private function high_priority_id(): int {
    $query = \Drupal::database()->select('taxonomy_term_data', 'term');
    $query->join('taxonomy_term__field_text_id', 'term_text_id', 'term_text_id.entity_id = term.tid');
    $query->condition('term.vid', 'article_tags');
    $query->condition('term_text_id.field_text_id_value', 'high_priority');
    $query->addField('term', 'tid');
    return $query->execute()->fetchField();
  }

  /**
   * Load the articles and fetch the values needed for the render array.
   *
   * @param array $articles_ids
   *   Entity IDs for the articles waiting for review.
   * @param bool $include_urls
   *   Don't include the article full history URLs if `FALSE`.
   *
   * @return array
   *   Values needed by the template for rendering the articles on the report.
   */
  private function loadArticles(array $article_ids, bool $include_urls): array {
    $articles = [];
    if (!empty($article_ids)) {
      foreach (Article::loadMultiple($article_ids) as $article) {
        $articles[$article->id()] = [
          'id' => $article->id(),
          'title' => $article->title->value,
          'publication' => $article->getLabel(),
          'url' => $include_urls ? Url::fromRoute('ebms_article.article', ['article' => $article->id()]) : '',
          'authors' => implode(', ', $article->getAuthors(3)) ?: '[No authors named]',
          'pmid' => $article->source_id->value,
        ];
      }
    }
    return $articles;
  }

}
