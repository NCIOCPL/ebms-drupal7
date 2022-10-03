<?php

namespace Drupal\ebms_article\Controller;

use Drupal\Core\Controller\ControllerBase;
use Drupal\Core\Link;
use Drupal\Core\Url;
use Drupal\ebms_article\Search;
use Drupal\ebms_article\Entity\Article;
use Drupal\file\Entity\File;
use Symfony\Component\DependencyInjection\ContainerInterface;

/**
 * Show articles matching the user's search.
 */
class SearchResultsController extends ControllerBase {

  const PER_PAGE = 10;

  /**
   * The `Search` service.
   *
   * @var \Drupal\ebms_article\Search
   */
  protected Search $searchService;

  /**
   * {@inheritdoc}
   */
  public static function create(ContainerInterface $container): SearchResultsController {
    // Instantiates this form class.
    $instance = parent::create($container);
    $instance->searchService = $container->get('ebms_article.search');
    return $instance;
  }

  /**
   * Display article information.
   */
  public function display(int $request_id): array {
    $parameters = $this->searchService->loadParameters($request_id);
    $restricted = !empty($parameters['restricted']);
    $query = $this->searchService->buildQuery($parameters);
    $count_query = clone($query);

    // It's a bit lame that Drupal's count query returns a string. 😒 Right?
    $num_articles = (int) $count_query->count()->execute();
    $s = $num_articles === 1 ? '' : 's';
    $title = "$num_articles Article$s Found";
    $items = [];
    $per_page = $parameters['per-page'] ?? self::PER_PAGE;
    if ($per_page !== 'all') {
      $query->pager($per_page);
    }
    $articles = Article::loadMultiple($query->execute());
    $query = \Drupal::request()->query->all();
    $query['search'] = $request_id;
    $opts = ['query' => $query];
    $route = 'ebms_article.article';
    foreach ($articles as $article) {
      $parms = ['article' => $article->id()];
      $full_text_url = '';
      if (!empty($article->full_text->file)) {
        $file = File::load($article->full_text->file);
        $full_text_url = $file->createFileUrl();
      }
      $items[] = [
        '#theme' => 'article_search_result',
        '#article' => [
          'title' => $article->title->value,
          'pmid' => $article->source_id->value,
          'id' => $article->id(),
          'legacy_id' => $article->legacy_id->value,
          'publication' => $article->getLabel(),
          'authors' => $article->getAuthors(),
          'topics' => $article->getTopics(),
          'url' => Url::fromRoute($route, $parms, $opts),
          'full_text_url' => $full_text_url,
        ],
        '#restricted' => $restricted,
      ];
    }
    $start = 1;
    if ($per_page !== 'all') {
      $page = \Drupal::request()->get('page') ?: 0;
      $start += $page * $per_page;
    }
    return [
      '#attached' => ['library' => ['ebms_article/search-result']],
      'local-actions' => [
        '#theme' => 'ebms_local_actions',
        '#actions' => [
          [
            'url' => Url::fromRoute('ebms_article.search_form', ['search_id' => $request_id]),
            'label' => 'Refine Search',
            'attributes' => ['title' => 'Return to the current search form.'],
          ],
        ],
      ],
      'articles' => [
        '#theme' => 'item_list',
        '#list_type' => 'ol',
        '#title' => $title,
        '#items' => $items,
        '#attributes' => ['start' => $start],
        '#cache' => ['max-age' => 0],
      ],
      'bottom-pager' => [
        '#type' => 'pager',
      ],
    ];
  }

}
