<?php

namespace Drupal\ebms_article\Controller;

use Drupal\Core\Controller\ControllerBase;
use Drupal\ebms_article\Search;
use Drupal\ebms_article\Entity\Article;
use Drupal\ebms_board\Entity\Board;
use Drupal\ebms_topic\Entity\Topic;
use Drupal\file\Entity\File;
use Drupal\taxonomy\Entity\Term;
use Symfony\Component\DependencyInjection\ContainerInterface;
use Symfony\Component\HttpFoundation\Response;

/**
 * Show articles matching the user's search.
 */
class SimpleSearchResults extends ControllerBase {

  /**
   * The `Search` service.
   *
   * @var \Drupal\ebms_article\Search
   */
  protected Search $searchService;

  /**
   * {@inheritdoc}
   */
  public static function create(ContainerInterface $container): SimpleSearchResults {
    // Instantiates this form class.
    $instance = parent::create($container);
    $instance->searchService = $container->get('ebms_article.search');
    return $instance;
  }

  /**
   * Display article information.
   */
  public function display(int $request_id): Response {

    // Fetch the parameters for this search;
    $parameters = $this->searchService->loadParameters($request_id);
    $filters = [];
    if (!empty($parameters['board'])) {
      $boards = [];
      foreach ($parameters['board'] as $board_id) {
        $board = Board::load($board_id);
        $boards[] = $board->name->value;
      }
      $filters[] = 'Board(s): ' . implode('; ', $boards);
    }
    if (!empty($parameters['topic'])) {
      $boards = [];
      foreach ($parameters['topic'] as $topic_id) {
        $topic = Topic::load($topic_id);
        $topics[] = $topic->name->value;
      }
      $filters[] = 'Topic(s): ' . implode('; ', $topics);
    }
    if (!empty($parameters['publication-year'])) {
      $filters[] = 'Year: ' . $parameters['publication-year'];
    }
    if (!empty($parameters['cycle'])) {
      $cycle = new \DateTime($parameters['cycle']);
      $filters[] = 'Review Cycle: ' . $cycle->format('F Y');
    }
    elseif (!empty($parameters['cycle-start']) || !empty($parameters['cycle-end'])) {
      $filters[] = 'Review Cycle Range: ' . $this->makeRangeString($parameters, 'cycle');
    }
    if (!empty($parameters['abstract-decision'])) {
      $filters[] = 'Abstract Decision: ' . ($parameters['abstract-decision'] === 'passed_bm_review' ? 'Yes' : 'No');
    }
    if (!empty($parameters['full-text-decision'])) {
      $filters[] = 'Full Text Decision: ' . ($parameters['full-text-decision'] === 'passed_full_review' ? 'Yes' : 'No');
    }
    if (!empty($parameters['meeting-category'])) {
      $term = Term::load($parameters['meeting-category']);
      $filters[] = 'Meeting Category: ' . $term->name->value;
    }
    if (!empty($parameters['meeting-start']) || !empty($parameters['meeting-end'])) {
      $filters[] = 'Meeting Date Range: ' . $this->makeRangeString($parameters, 'meeting');
    }
    if (!empty($parameters['decision'])) {
      $term = Term::load($parameters['decision']);
      $filters[] = 'Editorial Board Decision: ' . $term->name->value;
    }
    if (!empty($parameters['comment'])) {
      $filters[] = 'Comment(s): ' . $parameters['comment'];
    }
    if (!empty($parameters['comment-start']) || !empty($parameters['comment-end'])) {
      $filters[] = 'Date Comment(s) Added: ' . $this->makeRangeString($parameters, 'comment');
    }
    if (!empty($parameters['article-tag'])) {
      $term = Term::load($parameters['article-tag']);
      $filters[] = 'Tag(s): ' . $term->name->value;
    }
    if (!empty($parameters['tag-start']) || !empty($parameters['tag-end'])) {
      $filters[] = 'Date Tag(s) Added: ' . $this->makeRangeString($parameters, 'tag');
    }
    if (!empty($parameters['core-journals'])) {
      $filters[] = 'Core Journals: ' . ucfirst($parameters['core-journals']);
    }
    if (!empty($parameters['board-manager-comment'])) {
      $filters[] = 'Board Manager Comment(s): ' . $parameters['board-manager-comment'];
    }

    // Create the render array.
    $render_array = [
      '#theme' => 'simple_search_results',
      '#cache' => ['max-age' => 0],
      '#filters' => $filters,
      '#articles' => [],
    ];
    $query = $this->searchService->buildQuery($parameters);
    $articles = Article::loadMultiple($query->execute());
    foreach ($articles as $article) {
      $values = [
        'title' => $article->title->value,
        'pmid' => $article->source_id->value,
        'id' => $article->id(),
        'legacy_id' => $article->legacy_id->value,
        'publication' => $article->getLabel(),
        'authors' => $article->getAuthors(),
        'topics' => $article->getTopics(),
      ];
      if (!empty($article->full_text->file)) {
        $file = File::load($article->full_text->file);
        $values['full_text_url'] = $file->createFileUrl(FALSE);
      }
      $render_array['#articles'][] = $values;
    }

    // Render and return the page.
    $page = \Drupal::service('renderer')->render($render_array);
    $response = new Response($page);
    return $response;
  }

  /**
   * Convert a data to its "Month Year" format.
   *
   * @param string $date
   *   Date in the ISO format YYYY-MM-DD.
   *
   * @return string
   *   For example, "August 2022".
   */
  private function getCycleName(string $date): string {
    if (empty($date)) {
      return '';
    }
    $datetime = new \DateTime($date);
    return $datetime->format('F Y');
  }

  /**
   * Create a string representing a range of dates.
   *
   * Handle the conditions in which one value is missing.
   *
   * @param array $parms
   *   Parameter values selected for the search
   * @param string $prefix
   *   Prepended to "-start" and "-end" to get the parameter values.
   *
   * @return string
   *   String describing the date range.
   */
  private function makeRangeString(array $parms, string $prefix) {
    $start = $parms["$prefix-start"];
    $end = $parms["$prefix-end"];
    if ($prefix === 'cycle') {
      $start = $this->getCycleName($start);
      $end = $this->getCycleName($end);
    }
    if (!empty($start)) {
      if (!empty($end)) {
        return "$start to $end";
      }
      return "Since $start";
    }
    return "Through $end";
  }

}
