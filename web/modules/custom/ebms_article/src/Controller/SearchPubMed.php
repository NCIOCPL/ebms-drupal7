<?php

namespace Drupal\ebms_article\Controller;

use Drupal\Core\Controller\ControllerBase;
use Drupal\Core\Routing\TrustedRedirectResponse;
use Drupal\user\Entity\User;

/**
 * Find the article for a given PubMed ID.
 */
class SearchPubMed extends ControllerBase {

  /**
   * Show the article if we find it or fall back on the search form.
   */
  public function search() {
    $pmid =  \Drupal::request()->query->get('pmid');
    $user = User::load($this->currentUser()->id());
    if (!$user->hasPermission('perform full search')) {
      return new TrustedRedirectResponse("https://pubmed.ncbi.nlm.nih.gov/$pmid");
    }
    $storage = $this->entityTypeManager()->getStorage('ebms_article');
    $query = $storage->getQuery()->accessCheck(FALSE);
    $query->condition('source_id', $pmid);
    $ids = $query->execute();
    if (empty($ids)) {
      return $this->redirect('ebms_article.search_form', [], ['query' => ['pmid' => $pmid]]);
    }
    return $this->redirect('ebms_article.article', ['article' => reset($ids)]);
  }

}
