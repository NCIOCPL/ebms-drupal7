<?php

namespace Drupal\ebms_article\Form;

use Drupal\Core\Form\FormBase;
use Drupal\Core\Form\FormStateInterface;
use Drupal\Core\Url;
use Drupal\ebms_core\Entity\SavedRequest;
use Drupal\file\Entity\File;
use Drupal\taxonomy\Entity\Term;
use Drupal\user\Entity\User;

/**
 * Find and display articles tagged for internal use, not for review.
 *
 * @ingroup ebms
 */
class InternalArticles extends FormBase {

  /**
   * {@inheritdoc}
   */
  public function getFormId(): string {
    return 'internal_articles_formoski';
  }

  /**
   * {@inheritdoc}
   */
  public function buildForm(array $form, FormStateInterface $form_state, int $request_id = 0): array {
    $values = empty($request_id) ? [] : SavedRequest::loadParameters($request_id);
    $per_page = $values['per-page'] ?? 10;
    $tags = [];
    $storage = \Drupal::entityTypeManager()->getStorage('taxonomy_term');
    $ids = $storage->getQuery()
      ->accessCheck(FALSE)
      ->condition('vid', 'internal_tags')
      ->condition('status', '1')
      ->sort('name')
      ->execute();
    foreach (Term::loadMultiple($ids) as $id => $term) {
      $tags[$id] = $term->name->value;
    }
    $form = [
      '#title' => 'Internal Articles',
      '#attached' => ['library' => ['ebms_article/internal-articles']],
      'request-id' => [
        '#type' => 'hidden',
        '#value' => $request_id,
      ],
      'filters' => [
        '#type' => 'details',
        '#title' => 'Filters',
        'tags' => [
          '#type' => 'checkboxes',
          '#title' => 'Tags',
          '#description' => 'Only show the articles with the selected tags, if any are checked.',
          '#options' => $tags,
          '#default_value' => $values['tags'] ?? [],
        ],
        'import-date' => [
          '#type' => 'container',
          '#attributes' => ['class' => ['inline-fields']],
          '#title' => 'Import Date Range',
          '#description' => 'Only show articles imported during the specified date range.',
          'import-start' => [
            '#type' => 'date',
            '#default_value' => $values['import-start'] ?? '',
          ],
          'import-end' => [
            '#type' => 'date',
            '#default_value' => $values['import-end'] ?? '',
          ],
        ],
        'tag-date' => [
          '#type' => 'container',
          '#attributes' => ['class' => ['inline-fields']],
          '#title' => 'Tag Date Range',
          '#description' => 'Only show articles tagged during the specified date range.',
          'tag-start' => [
            '#type' => 'date',
            '#default_value' => $values['tag-start'] ?? '',
          ],
          'tag-end' => [
            '#type' => 'date',
            '#default_value' => $values['tag-end'] ?? '',
          ],
        ],
        'comment' => [
          '#type' => 'textfield',
          '#title' => 'Comment',
          '#description' => 'Only show articles with a comment containing the value entered here, if any.',
          '#default_value' => $values['comment'] ?? '',
        ],
      ],
      'options' => [
        '#type' => 'details',
        '#title' => 'Display Options',
        'per-page' => [
          '#type' => 'radios',
          '#title' => 'Articles Per Page',
          '#description' => 'Any value other than "All" will enable paging of the article display.',
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
        '#submit' => ['::reset'],
        '#limit_validation_errors' => [],
      ],
    ];

    // Find the internal articles which match the filtering criteria.
    // I haven't found this documented anywhere, but without this check
    // we end up performing the work of constructing the query, executing
    // it, and assembling the render array for the articles twice, which
    // isn't necessary. Because this isn't documented, if anything breaks,
    // look here first. 😉
    $method = $this->getRequest()->getMethod();
    ebms_debug_log("InternalArticles::buildForm(): request method is $method");
    if ($method !== 'POST') {
      $storage = \Drupal::entityTypeManager()->getStorage('ebms_article');
      $query = $storage->getQuery()->accessCheck(FALSE)->sort('internal_tags.0.added', 'DESC');
      $selected_tags = [];
      if (!empty($values['tags'])) {
        foreach ($values['tags'] as $key => $value) {
          if (!empty($value)) {
            $selected_tags[] = $key;
          }
        }
      }
      if (empty($selected_tags)) {
        $query->exists('internal_tags.tag');
      }
      else {
        $query->condition('internal_tags.tag', $selected_tags, 'IN');
      }
      if (!empty($values['import-start'])) {
        $query->condition('import_date', $values['import-start'], '>=');
      }
      if (!empty($values['import-end'])) {
        $end = $values['import-end'];
        if (strlen($end) === 10) {
          $end .= ' 23:59:59';
        }
        $query->condition('import_date', $end, '<=');
      }
      if (!empty($values['tag-start'])) {
        $query->condition('internal_tags.added', $values['tag-start'], '>=');
      }
      if (!empty($values['tag-end'])) {
        $end = $values['tag-end'];
        if (strlen($end) === 10) {
          $end .= ' 23:59:59';
        }
        $query->condition('internal_tags.added', $end, '<=');
      }
      if (!empty($values['comment'])) {
        $query->condition('internal_comments.body', '%' . $values['comment'] . '%', 'LIKE');
      }
      $count_query = clone $query;
      $count = $count_query->count()->execute();
      $start = 1;
      if ($per_page !== 'all') {
        $query->pager($per_page);
        $page = $this->getRequest()->get('page') ?: 0;
        $start += $page * $per_page;
      }
      $ids = $query->execute();
      $articles = $storage->loadMultiple($ids);

      // Assemble the render arrays.
      $items = [];
      foreach ($articles as $article) {
        $authors = $article->getAuthors();
        if (empty($authors)) {
          $authors = ['[No authors listed.]'];
        }
        elseif (count($authors) > 3) {
          $authors = array_splice($authors, 0, 3);
          $authors[] = 'et al.';
        }
        $internal_tags = [];
        foreach ($article->internal_tags as $internal_tag) {
          $added = substr($internal_tag->added, 0, 10);
          $name = Term::load($internal_tag->tag)->name->value;
          $internal_tags[] = "[$added] $name";
        }
        $comments = [];
        foreach ($article->internal_comments as $internal_comment) {
          $comments[] = [
            'user' => User::load($internal_comment->user)->name->value,
            'body' => $internal_comment->body,
          ];
        }
        $full_text_url = '';
        if (!empty($article->full_text->file)) {
          $file = File::load($article->full_text->file);
          $full_text_url = $file->createFileUrl();
        }
        $items[] = [
          '#theme' => 'internal_article',
          '#article' => [
            'title' => $article->title->value,
            'pmid' => $article->source_id->value,
            'id' => $article->id(),
            'legacy_id' => $article->legacy_id->value,
            'publication' => $article->getLabel(),
            'authors' => $authors,
            'tags' => $internal_tags,
            'comments' => $comments,
            'url' => Url::fromRoute('ebms_article.article', ['article' => $article->id()]),
            'full_text_url' => $full_text_url,
          ],
        ];
      }
      $form['articles'] = [
        '#theme' => 'item_list',
        '#title' => "Articles ($count)",
        '#list_type' => 'ol',
        '#items' => $items,
        '#empty' => 'No articles match the filtering values.',
        '#attributes' => ['start' => $start],
        '#cache' => ['max-age' => 0],
      ];
      $form['bottom-pager'] = [
        '#type' => 'pager',
      ];
    }
    return $form;
  }

  /**
   * {@inheritdoc}
   */
  public function submitForm(array &$form, FormStateInterface $form_state) {
    $request = SavedRequest::saveParameters('internal articles', $form_state->getValues());
    $form_state->setRedirect('ebms_article.internal_articles', ['request_id' => $request->id()]);
  }

  /**
   * Clear the decks.
   *
   * @param array $form
   *   Form settings.
   * @param \Drupal\Core\Form\FormStateInterface $form_state
   *   Form values.
   */
  public function reset(array &$form, FormStateInterface $form_state) {
    $form_state->setRedirect('ebms_article.internal_articles');
  }

}
