<?php

namespace Drupal\ebms_article\Controller;

use Drupal\Core\Controller\ControllerBase;
use Drupal\Core\Routing\TrustedRedirectResponse;
use Drupal\ebms_article\Entity\Article;
use Drupal\file\Entity\File;
use Drupal\user\Entity\User;

/**
 * Find the article for a given PubMed ID.
 */
class SearchPubMed extends ControllerBase {

  const NOT_FOUND = '<p>A PDF of this article is not available in the EBMS. E-mail your Board manager to request a copy of this article.';

  /**
   * Show the article if we find it or fall back on the import form.
   */
  public function search() {
    $user = User::load($this->currentUser()->id());
    $pmid =  \Drupal::request()->query->get('pmid');
    $storage = $this->entityTypeManager()->getStorage('ebms_article');
    $query = $storage->getQuery()->accessCheck(FALSE);
    $query->condition('source_id', $pmid);
    $ids = $query->execute();
    if (empty($ids)) {
      if ($user->hasPermission('import articles')) {
        return $this->redirect('ebms_import.import_form', [], ['query' => ['pmid' => $pmid]]);
      }
      return [
        '#title' => 'Not Found',
        '#markup' => self::NOT_FOUND,
      ];
    }
    if ($user->hasPermission('perform full search')) {
      return $this->redirect('ebms_article.article', ['article' => reset($ids)]);
    }
    $article = Article::load(reset($ids));
    if (empty($article->full_text->file)) {
      return [
        '#title' => 'Not Found',
        '#markup' => self::NOT_FOUND,
      ];
    }
    $file = File::load($article->full_text->file);
    return new TrustedRedirectResponse($file->createFileUrl());
  }

}
