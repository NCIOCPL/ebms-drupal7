<?php

namespace Drupal\ebms_report\Form;

use Drupal\Core\Form\FormBase;
use Drupal\Core\Form\FormStateInterface;
use Drupal\Core\Url;
use Drupal\ebms_article\Entity\Article;
use Drupal\ebms_article\Entity\ArticleTag;
use Drupal\ebms_article\Entity\ArticleTopic;
use Drupal\ebms_board\Entity\Board;
use Drupal\ebms_core\Entity\SavedRequest;
use Drupal\ebms_state\Entity\State;
use Drupal\ebms_topic\Entity\Topic;
use Drupal\taxonomy\Entity\Term;

/**
 * Show articles with topic-specific tags.
 *
 * @ingroup ebms
 */
class ArticlesByTagReport extends FormBase {

  /**
   * Lowercased starts of comments generated by software.
   *
   * We don't want to include such comments in the report.
   */
  const MACHINE_COMMENTS = [
    'status entered by conversion because the topic/article combination',
    'published state added as a result of setting the state for this',
    'tir 2506 (',
    'published as part of import from core journals',
    'core journals search',
    'delete this journal',
    'state inactivated by setting ',
  ];

  /**
   * {@inheritdoc}
   */
  public function getFormId(): string {
    return 'articles_by_tag_report';
  }

  /**
   * {@inheritdoc}
   */
  public function buildForm(array $form, FormStateInterface $form_state, int $report_id = 0): array {

    // Collect some values for the report request.
    $params = empty($report_id) ? [] : SavedRequest::loadParameters($report_id);
    $board = $form_state->getValue('board', $params['board'] ?? '');
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

    // Create the tags picklist.
    $storage = \Drupal::entityTypeManager()->getStorage('taxonomy_term');
    $query = $storage->getQuery()->accessCheck(FALSE);
    $query->condition('vid', 'article_tags');
    $query->condition('field_topic_allowed', 1);
    $query->condition('status', 1);
    $query->sort('name');
    $tags = [];
    foreach ($storage->loadMultiple($query->execute()) as $tag) {
      $tags[$tag->id()] = $tag->name->value;
    }

    // Assemble the render array for the form's fields.
    $form = [
      '#title' => 'Articles By Tag',
      '#attached' => ['library' => ['ebms_report/articles-by-tag']],
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
            '#description' => 'Optionally select one or more topics to further narrow the report.',
            '#multiple' => TRUE,
            '#options' => $topics,
            '#default_value' => $selected_topics,
            '#empty_value' => '',
          ],
        ],
        'tag' => [
          '#type' => 'select',
          '#title' => 'Tag',
          '#description' => 'Select the tag to be retrieved by the report.',
          '#options' => $tags,
          '#required' => TRUE,
          '#default_value' => $params['tag'] ?? '',
          '#empty_value' => '',
        ],
        'dates' => [
          '#type' => 'container',
          '#attributes' => ['class' => ['inline-fields']],
          '#title' => 'Date Tag Assigned',
          '#description' => 'Optionally specify a data range to narrow the report by when the tags were assigned.',
          'date-start' => [
            '#type' => 'date',
            '#default_value' => $params['date-start'] ?? '',
          ],
          'date-end' => [
            '#type' => 'date',
            '#default_value' => $params['date-end'] ?? '',
          ],
        ],
        'comment' => [
          '#type' => 'textfield',
          '#title' => 'Comment',
          '#description' => 'Optionally narrow the report to tags with comments matching the specified value. Use wildcards for partial match (for example, %financial toxicity%).',
          '#default_value' => $params['comment'] ?? '',
        ],
        'options' => [
          '#type' => 'checkboxes',
          '#title' => 'Options',
          '#options' => ['no-decision' => 'Exclude topics with an editorial board decision.'],
          '#description' => 'Options to narrow the report further.',
          '#default_value' => $params['options'] ?? [],
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

    // Add the report, if so requested (and this is not an AJAX call).
    if (!empty($params) && empty($form_state->getvalues())) {
      $form['report'] = $this->report($params);
    }
    return $form;
  }

  /**
   * {@inheritdoc}
   */
  public function submitForm(array &$form, FormStateInterface $form_state) {
    //dpm($form_state->getValues());
    $request = SavedRequest::saveParameters('articles by tag report', $form_state->getValues());
    $form_state->setRedirect('ebms_report.articles_by_tag', ['report_id' => $request->id()]);
  }

  /**
   * Create a fresh form.
   */
  public function resetSubmit(array &$form, FormStateInterface $form_state) {
    $form_state->setRedirect('ebms_report.articles_by_tag');
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
   * Show articles having tags matching the filter criteria.
   *
   * @param array $values
   *   Values entered by the user on the request form.
   *
   * @return array
   *   Render array for the report.
   */
  private function report(array $params): array {

    // Get some taxonomy terms we'll need later on.
    $storage = \Drupal::entityTypeManager()->getStorage('taxonomy_term');
    $query = $storage->getQuery()->accessCheck(FALSE);
    $query->condition('vid', 'article_tags');
    $query->condition('field_text_id', 'librarian_cmt');
    $ids = $query->execute();
    if (empty($ids)) {
      $this->messenger()->addError('Unable to find librarian comment tag type.');
      return [];
    }
    $librarian_comment_id = reset($ids);
    $query = $storage->getQuery()->accessCheck(FALSE);
    $query->condition('vid', 'states');
    $query->condition('field_text_id', 'passed_full_review');
    $ids = $query->execute();
    if (empty($ids)) {
      $this->messenger()->addError('Unable to find state for Approval from full text review.');
      return [];
    }
    $full_text_approval = $storage->load(reset($ids));

    // The Drupal entity query API is not suitable for this report's logic.
    $term_lookup = \Drupal::service('ebms_core.term_lookup');
    $query = \Drupal::database()->select('ebms_article_tag', 'article_tag');
    $query->join('ebms_article_topic__tags', 'tags', 'tags.tags_target_id = article_tag.id');
    $query->join('ebms_article_topic', 'article_topic', 'article_topic.id = tags.entity_id');
    $query->join('ebms_article__topics', 'topics', 'topics.topics_target_id = article_topic.id');
    $query->join('ebms_article', 'article', 'article.id = topics.entity_id');
    $query->join('ebms_topic', 'topic', 'topic.id = article_topic.topic');
    $query->join('ebms_state', 'state', 'state.topic = topic.id AND state.article = article.id');
    $query->leftJoin('ebms_article__authors', 'author', 'author.entity_id = article.id AND author.delta = 0');
    $query->condition('article_tag.tag', $params['tag']);
    $query->condition('article_tag.active', 1);
    $query->condition('state.current', 1);
    if (!empty($params['topics'])) {
      $query->condition('article_topic.topic', $params['topics'], 'IN');
    }
    else {
      $query->condition('topic.board', $params['board']);
    }
    if (!empty($params['date-start'])) {
      $query->condition('article_tag.assigned', $params['date-start'], '>=');
    }
    if (!empty($params['date-end'])) {
      $end = $params['date-end'];
      if (strlen($end) === 10) {
        $end .= ' 23:59:59';
      }
      $query->condition('article_tag.assigned', $end, '<=');
    }
    if (!empty($params['comment'])) {
      if ($params['tag'] == $librarian_comment_id) {
        $query->join('ebms_article_tag__comments', 'comments', 'comments.entity_id = article_tag.id');
        $query->condition('comments.comments_body', $params['comment'], 'LIKE');
      }
      else {
        $query->join('ebms_article_topic__comments', 'comments', 'comments.entity_id = article_topic.id');
        $query->condition('comments.comments_comment', $params['comment'], 'LIKE');
      }
    }
    if (!empty($params['options']['no-decision'])) {
      $query->join('taxonomy_term__field_text_id', 'state_text_id', 'state_text_id.entity_id = state.value');
      $query->condition('state_text_id.field_text_id_value', 'final_board_decision', '<>');
    }
    $query->addField('topic', 'name', 'topic_name');
    $query->addField('article_topic', 'id', 'article_topic_id');
    $query->addField('article', 'id', 'article_id');
    $query->addField('article_tag', 'id', 'article_tag_id');
    $query->addField('article_tag', 'assigned', 'tag_assigned');
    $query->addField('topic', 'id', 'topic_id');
    $query->addField('state', 'id', 'state_id');
    $query->addField('author', 'authors_display_name', 'author');
    $query->addField('article', 'title', 'article_title');
    $query->distinct();
    $query->orderBy('topic.name');
    $query->orderBy('author.authors_display_name');
    $query->orderBy('article.title');

    // Walk through the results set collecting nested topic/article arrays.
    $topics = [];
    $articles = [];
    foreach ($query->execute() as $row) {
      if (!array_key_exists($row->topic_id, $topics)) {
        $topics[$row->topic_id] = [
          'name' => $row->topic_name,
          'articles' => [],
        ];
      }
      if (!array_key_exists($row->article_id, $topics[$row->topic_id]['articles'])) {
        if (!array_key_exists($row->article_id, $articles)) {
          $article = Article::load($row->article_id);
          $articles[$row->article_id] = [
            'id' => $row->article_id,
            'title' => $row->article_title,
            'publication' => $article->getLabel(),
            'url' => Url::fromRoute('ebms_article.article', ['article' => $row->article_id]),
            'authors' => implode(', ', $article->getAuthors(3)) ?: '[No authors named]',
            'pmid' => $article->source_id->value,
          ];
        }
        $state = State::load($row->state_id);
        $state_description = $state->laterStateDescription($full_text_approval->field_sequence->value);
        if (empty($state_description)) {
          $state_description = $state->value->entity->name->value;
          if ($state->field_text_id === 'passed_full_review') {
            $query = \Drupal::database()->select('ebms_packet', 'packet');
            $query->addField('packet', 'title');
            $query->join('ebms_packet__articles', 'articles', 'articles.entity_id = packet.id');
            $query->join('ebms_packet_article', 'packet_article', 'packet_article.id = articles.articles_target_id');
            $query->condition('packet_article.article', $row->article_id);
            $query->condition('packet_article.dropped', 0);
            $query->condition('packet.active', 1);
            $packets = $query->execute()->fetchCol();
            if (!empty($packets)) {
              $state_description = 'Assigned for review: ' . implode('; ', $packets);
            }
          }
        }
        $article_topic = ArticleTopic::load($row->article_topic_id);
        $topic_comments = [];
        foreach ($article_topic->comments as $topic_comment) {
          $comment = $this->vet_comment($topic_comment->comment);
          if (!empty($comment)) {
            $topic_comments[] = $comment;
          }
        }
        $state_comments = [];
        if ($params['tag'] != $librarian_comment_id) {
          foreach ($article_topic->states as $topic_state) {
            foreach ($topic_state->entity->comments as $state_comment) {
              $comment = $this->vet_comment($state_comment->body);
              if (!empty($comment)) {
                $state_comments[] = $comment;
              }
            }
          }
        }
        $topics[$row->topic_id]['articles'][$row->article_id] = $articles[$row->article_id];
        $topics[$row->topic_id]['articles'][$row->article_id]['state'] = $state_description;
        $topics[$row->topic_id]['articles'][$row->article_id]['topic_comments'] = $topic_comments;
        $topics[$row->topic_id]['articles'][$row->article_id]['state_comments'] = $state_comments;
        $topics[$row->topic_id]['articles'][$row->article_id]['tag_comments'] = [];
        $topics[$row->topic_id]['articles'][$row->article_id]['tag_added'] = [];
      }
      $assigned = substr($row->tag_assigned, 0, 10);
      if (!in_array($assigned, $topics[$row->topic_id]['articles'][$row->article_id]['tag_added'])) {
        $topics[$row->topic_id]['articles'][$row->article_id]['tag_added'][] = $assigned;
      }
      if ($params['tag'] == $librarian_comment_id) {
        $article_tag = ArticleTag::load($row->article_tag_id);
        foreach ($article_tag->comments as $tag_comment) {
          $comment = $this->vet_comment($tag_comment->body);
          if (!empty($tag_comment)) {
            $topics[$row->topic_id]['articles'][$row->article_id]['tag_comments'][] = $comment;
          }
        }
      }
    }

    // Assemble and return the render array for the report.
    $article_count = count($articles);
    $topic_count = count($topics);
    $article_s = $article_count === 1 ? '' : 's';
    $topic_s = $topic_count === 1 ? '' : 's';
    $tag = Term::load($params['tag']);
    $tag_name = $tag->name->value;
    return [
      '#theme' => 'articles_by_tag',
      '#title' => "Tag '$tag_name' assigned for $topic_count Topic$topic_s in $article_count Article$article_s",
      '#topics' => $topics,
    ];
  }

  /**
   * Normalize a comment and weed it if it was software-generated.
   *
   * @param string $comment
   *   The comment found in the database.
   *
   * @return string
   *   Empty string if this was a machine-generated comment, otherwise the
   *   original comment, with leading and trailing whitespace stripped.
   */
  private static function vet_comment(string $comment): string {
    $normalized_comment = trim($comment);
    if (empty($normalized_comment)) {
      return '';
    }
    $lowercased_comment = strtolower($normalized_comment);
    foreach (self::MACHINE_COMMENTS as $machine_comment) {
      if (str_starts_with($lowercased_comment, $machine_comment)) {
        return '';
      }
    }
    return $normalized_comment;
  }

}
