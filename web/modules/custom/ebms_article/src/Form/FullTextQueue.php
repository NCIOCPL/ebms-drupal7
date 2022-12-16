<?php

namespace Drupal\ebms_article\Form;

use Drupal\Core\Form\FormBase;
use Drupal\Core\Form\FormStateInterface;
use Drupal\Core\Url;
use Drupal\ebms_article\Entity\Article;
use Drupal\ebms_board\Entity\Board;
use Drupal\ebms_core\Entity\SavedRequest;
use Drupal\ebms_import\Entity\Batch;
use Drupal\ebms_topic\Entity\Topic;
use Drupal\file\Entity\File;

/**
 * Form controller for managing bulk loading of full-text PDFs.
 *
 * @ingroup ebms
 */
class FullTextQueue extends FormBase {

  const FILTER = 'Filter';
  const UPLOAD = 'Upload';

  /**
   * {@inheritdoc}
   */
  public function getFormId(): string {
    return 'full_text_queue';
  }

  /**
   * {@inheritdoc}
   */
  public function buildForm(array $form, FormStateInterface $form_state, int $queue_id = 0): array {

    // Get the parameters.
    $values = empty($queue_id) ? [] : SavedRequest::loadParameters($queue_id);

    // Find the articles needing PDFs.
    $storage = \Drupal::entityTypeManager()->getStorage('ebms_article');
    $query = $storage->getQuery()->accessCheck(FALSE);
    $query->condition('topics.entity.states.entity.value.entity.field_text_id', 'passed_bm_review');
    $query->condition('topics.entity.states.entity.current', 1);
    if (!empty($values['preliminary']) && $values['preliminary'] === 'with') {
      $query->condition('full_text.file', NULL, 'IS NOT NULL');
      $query->condition('tags.entity.tag.entity.field_text_id', 'preliminary');
    }
    else {
      $query->condition('full_text.file', NULL, 'IS NULL');
    }
    $or = $query->orConditionGroup()
      ->condition('full_text.unavailable', FALSE)
      ->condition('full_text.unavailable', NULL, 'IS NULL');
    $query->condition($or);
    if (!empty($values['board'])) {
      $query->condition('topics.entity.topic.entity.board', $values['board']);
    }
    if (!empty($values['topic'])) {
      $query->condition('topics.entity.topic', $values['topic']);
    }
    if (!empty($values['cycle'])) {
      $query->condition('topics.entity.cycle', $values['board']);
    }
    else {
      $query->condition('topics.entity.states.entity.entered', Article::CONVERSION_DATE, '>');
    }
    $count_query = clone $query;
    $count = $count_query->count()->execute();
    $per_page = $values['per-page'] ?? 10;
    // $start = 1; Can't use this — see note below on Drupal bugs.
    if ($per_page !== 'all') {
      $query->pager($per_page);
      // $page = $this->getRequest()->get('page') ?: 0;
      // $start += $page * $per_page;
    }
    $articles = $storage->loadMultiple($query->execute());

    // Build the list of render arrays for the articles.
    $items = [];
    foreach ($articles as $article) {
      $article_boards = [];
      foreach ($article->topics as $article_topic) {
        $article_board = $article_topic->entity->topic->entity->board->entity->name->value;
        $article_boards[$article_board] = $article_board;
      }
      sort($article_boards);
      $article_id = $article->id();
      $authors = $article->getAuthors();
      if (empty($authors)) {
        $authors = ['[No authors listed.]'];
      }
      elseif (count($authors) > 3) {
        $authors = array_splice($authors, 0, 3);
        $authors[] = 'et al.';
      }
      $related = [];
      foreach ($article->getRelatedArticles() as $other_article) {
        $related[] = $other_article->getLabel();
      }
      if (!empty($related)) {
        $related = [
          '#theme' => 'item_list',
          '#items' => $related,
          '#title' => 'Related Articles',
        ];
      }
      $items[] = [
        '#type' => 'container',
        "article-$article_id" => [
          '#theme' => 'full_text_queue_article',
          '#article' => [
            'id' => $article_id,
            'url' => Url::fromRoute('ebms_article.article', ['article' => $article_id]),
            'authors' => implode('; ', $authors),
            'title' => $article->title->value,
            'publication' => $article->getLabel(),
            'pmid' => $article->source_id->value,
            'related' => $related,
            'boards' => $article_boards,
          ],
        ],
        "full-text-$article_id" => [
          '#type' => 'file',
          '#attributes' => [
            'class' => ['usa-file-input'],
            'accept' => ['.pdf'],
          ],
        ],
      ];
    }

    // Create the render array for the entire form page.
    $form = [
      '#attached' => ['library' => ['ebms_article/full-text-queue']],
      '#cache' => ['max-age' => 0],
      '#title' => 'Full Text Retrieval Queue',
      'queue-id' => [
        '#type' => 'hidden',
        '#value' => $queue_id,
      ],
      'filters' => [
        '#type' => 'details',
        '#title' => 'Filters',
        'board' => [
          '#type' => 'select',
          '#title' => 'Editorial Board',
          '#description' => 'Optionally narrow the queue to those associated with a specific board.',
          '#options' => Board::boards(),
          '#default_value' => $values['board'] ?? '',
          '#empty_value' => '',
          '#ajax' => [
            'callback' => '::getTopicsCallback',
            'wrapper' => 'board-controlled',
            'event' => 'change',
          ],
        ],
        'topic-wrapper' => [
          '#type' => 'container',
          '#attributes' => ['id' => 'board-controlled'],
          'topic' => [
            '#type' => 'select',
            '#title' => 'Summary Topic',
            '#description' => 'Optionally narrow the queue to those assigned to a specific review topic. Select a board to populate the picklist for topics.',
            '#options' => empty($values['board']) ? [''] : Topic::topics($values['board']),
            '#default_value' => $values['topic'] ?? '',
            '#empty_value' => '',
          ],
        ],
        'cycle' => [
          '#type' => 'select',
          '#title' => 'Review Cycle',
          '#description' => 'Optionally restrict the queue to articles assigned to a specific review cycle.',
          '#options' => Batch::cycles(),
          '#default_value' => $values['cycle'],
          '#empty_value' => '',
        ],
        'preliminary' => [
          '#type' => 'radios',
          '#title' => 'With or Without PDFs',
          '#options' => [
            'without' => 'Without PDFs',
            'with' => 'With Preliminary PDFs',
          ],
          '#default_value' => $values['preliminary'] ?? 'without',
        ],
      ],
      'options' => [
        '#type' => 'details',
        '#title' => 'Options',
        'per-page' => [
          '#type' => 'radios',
          '#title' => 'Articles Per Page',
          '#description' => 'PDF files which you have queued for uploading will no longer be queued if you navigate to a different page before clicking the Upload button.',
          '#options' => [
            10 => 10,
            25 => 25,
            50 => 50,
            100 => 100,
            'all' => 'All on one page',
          ],
          '#default_value' => $per_page,
        ],
      ],
      'filter' => [
        '#type' => 'submit',
        '#value' => self::FILTER,
      ],
      'reset' => [
        '#type' => 'submit',
        '#value' => 'Reset',
        '#submit' => ['::resetSubmit'],
        '#limit_validation_errors' => [],
      ],
      'apply' => [
        '#type' => 'submit',
        '#value' => self::UPLOAD,
      ],
      'articles' => [
        '#type' => 'container',
        // This is lame, but we don't have much choice until the Drupal bugs
        // for the form/render array muddle are resolved. See, for example,
        // https://www.drupal.org/project/ideas/issues/2702061,
        // https://www.drupal.org/project/drupal/issues/1382350, and
        // https://www.drupal.org/project/drupal/issues/3246825. These bugs
        // prevent us from using an ordered list to present the articles
        // needing full-text PDFs, but when we do, the file upload fields
        // don't get hooked into the form processing. We could bypass
        // Drupal's form processing altogether, and use low-level PDF
        // code to get the uploaded files and save them, but that's an
        // even more unattractive hack than this. This also means we need
        // to create our HTML markup for the title showing how many
        // articles meet the query conditions here in the PDF code. I'm
        // not going to create a TWIG template for a single H2 element.
        '#markup' => "<h2>Article which require PDFs ($count)</h2>",
        /*
        '#theme' => 'item_list',
        '#list_type' => 'ol',
        '#title' => "Article needing PDFs ($count)",
        '#items' => $items,
        '#attributes' => ['start' => $start],
        */
        $items,
      ],
      'apply-bottom' => [
        '#type' => 'submit',
        '#value' => self::UPLOAD,
      ],
    ];
    if ($per_page !== 'all') {
      $form['pager'] = ['#type' => 'pager'];
    }
    return $form;
  }

  /**
   * Create a fresh version of the page.
   *
   * @param array $form
   *   Form settings.
   * @param \Drupal\Core\Form\FormStateInterface $form_state
   *   Form values.
   */
  public function resetSubmit(array &$form, FormStateInterface $form_state) {
    $form_state->setRedirect('ebms_article.full_text_queue');
  }

  /**
   * {@inheritdoc}
   */
  public function submitForm(array &$form, FormStateInterface $form_state) {

    // If we've been asked to store the files, do so.
    $trigger = $form_state->getTriggeringElement()['#value'];
    if ($trigger === self::UPLOAD) {
      $validators = ['file_validate_extensions' => ['pdf']];
      $file_usage = \Drupal::service('file.usage');
      $logger = \Drupal::logger('ebms_article');
      $files = $this->getRequest()->files->get('files', []);
      foreach ($files as $key => $uploaded_file) {
        if (!empty($uploaded_file)) {
          $article_id = str_replace('full-text-', '', $key);
          $name = $uploaded_file->getClientOriginalName();
          $file = file_save_upload($key, $validators, 'public://', 0);
          if (empty($file)) {
            $message = "Unable to save $name for article $article_id.";
            $this->messenger()->addWarning($message);
            $logger->error($message);
          }
          else {
            $article = Article::load($article_id);
            if (empty($article)) {
              $message = "Unable to load article $article_id for attaching $name.";
              $this->messager()->addWarning($message);
              $logger->error($message);
            }
            else {
              $file->setPermanent();
              $file->save();
              $values = ['file' => $file->id(), 'unavailable' => FALSE];
              $article->set('full_text', $values);
              $article->save();
              $file_usage->add($file, 'ebms_article', 'ebms_article', $article_id);
              $this->messenger()->addMessage("Posted $name for article $article_id.");
            }
          }
        }
      }
    }

    // In any case, generate a fresh queue set.
    $values = $form_state->getValues();
    $request = SavedRequest::saveParameters('full-text queue', $values);
    $parms = ['queue_id' => $request->id()];
    $opts = ['query' => $this->getRequest()->query->all()];
    unset($opts['query']['page']);
    $form_state->setRedirect('ebms_article.full_text_queue', $parms, $opts);
  }

  /**
   * Adjust picklist for topics when board changes.
   *
   * @param array $form
   *   Render array we are adjusting.
   * @param FormStateInterface $form_state
   *   Access to the form's values.
   *
   * @return array
   *   Modified portion of the form.
   */
  public function getTopicsCallback(array &$form, FormStateInterface $form_state): array {
    $board_id = $form_state->getValue('board');
    $topics = empty($board_id) ? [] : Topic::topics($board_id);
    // Shouldn't have to do this, but Drupal is broken here
    // (https://www.drupal.org/project/drupal/issues/3180011).
    // @todo remove next line when the bug is fixed.
    $topics = ['' => '- None -'] + $topics;
    $form['filters']['topic-wrapper']['topic']['#options'] = $topics;
    return $form['filters']['topic-wrapper'];
  }

}
