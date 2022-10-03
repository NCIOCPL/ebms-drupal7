<?php

namespace Drupal\ebms_article\Controller;

use Drupal\Core\Controller\ControllerBase;
use Drupal\ebms_article\Entity\Article;

/**
 * Display an article's abstract (for a popup).
 */
class ShowAbstract extends ControllerBase {

  /**
   * Display article abstract.
   *
   * @param \Drupal\ebms_article\Entity\Article $article
   *   Article whose abstract is to be displayed.
   *
   * @return array
   *   Render array for the page.
   */
  public function display(Article $article): array {
    $abstract = [];
    foreach ($article->abstract as $paragraph) {
      $abstract[] = [
        'label' => $paragraph->paragraph_label,
        'text' => $paragraph->paragraph_text,
      ];
    }
    $publication = $article->getLabel();
    return [
      '#title' => "Viewing Abstract for $publication",
      '#cache' => ['max-age' => 0],
      'citation' => [
        '#theme' => 'show_abstract',
        '#abstract' => $abstract,
        '#pmid' => $article->source_id->value,
      ],
    ];
  }

}
