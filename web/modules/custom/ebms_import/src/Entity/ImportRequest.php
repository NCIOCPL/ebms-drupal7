<?php

namespace Drupal\ebms_import\Entity;

use Drupal\Core\Entity\ContentEntityBase;
use Drupal\Core\Entity\ContentEntityInterface;
use Drupal\Core\Entity\EntityTypeInterface;
use Drupal\Core\Field\BaseFieldDefinition;
use Drupal\Core\Url;

/**
 * Import request parameters and results.
 *
 * An instance of this entity is created by the import form's submit
 * handler, capturing the form values as well as the results of the
 * batch import. The entity is saved, the handler redirects back to
 * the form page, and the form is re-drawn, with the report showing
 * the details of the import job which just completed (using getReport).
 *
 * These entities cannot be purged, as they are used for reporting.
 *
 * @ingroup ebms
 *
 * @ContentEntityType(
 *   id = "ebms_import_request",
 *   label = @Translation("Import Request"),
 *   base_table = "ebms_import_request",
 *   admin_permission = "administer site configuration",
 *   entity_keys = {
 *     "id" = "id"
 *   }
 * )
 */
class ImportRequest extends ContentEntityBase implements ContentEntityInterface {

  /**
   * Used for links to articles on PubMed.
   */
  const PUBMED_URL = 'https://pubmed.ncbi.nlm.nih.gov';

  /**
   * {@inheritdoc}
   */
  public static function baseFieldDefinitions(EntityTypeInterface $entity_type): array {
    $fields = parent::baseFieldDefinitions($entity_type);

    $fields['batch'] = BaseFieldDefinition::create('entity_reference')
      ->setDescription('Batch job which performed this import, if running in live mode.')
      ->setSetting('target_type', 'ebms_import_batch');
    $fields['params'] = BaseFieldDefinition::create('string_long')
      ->setDescription('Values captured from the form, plus the related article IDs found by the batch job.')
      ->setRequired(TRUE);
    $fields['report'] = BaseFieldDefinition::create('string_long')
      ->setDescription('Information used to describe what happened during the report job.')
      ->setRequired(TRUE);

    return $fields;
  }

  /**
   * Generate a render array for the report returned by the batch import job.
   *
   * @param string $title
   *   Title for the report.
   * @param bool $internal
   *   If `TRUE`, generate a report with fewer blocks.
   *
   * @return array
   *   Render array fed to the theme.
   */
  public function getReport(string $title, bool $internal = FALSE): array {

    // Create the mapping of import dispositions.
    $dispositions = [];
    $storage = \Drupal::entityTypeManager()->getStorage('taxonomy_term');
    $query = $storage->getQuery()->accessCheck(FALSE);
    $query->condition('vid', 'import_dispositions');
    $terms = $storage->loadMultiple($query->execute());
    foreach ($terms as $term) {
      $dispositions[$term->id()] = $term->field_text_id->value;
    }

    // Figure out which blocks the report will have.
    if ($internal) {
      $blocks = [
        'unique' => "Unique ID{s} in batch",
        'imported' => 'Article{s} imported',
        'duplicate' => 'Duplicate article{s}',
        'replaced' => 'Article{s} replaced',
        'error' => 'Article{s} with errors',
      ];
    }
    else {
      $blocks = [
        'unique' => [
          'title' => 'Unique ID{s} in batch',
          'internal' => TRUE,
        ],
        'imported' => [
          'title' => 'Article{s} imported',
          'internal' => TRUE,
        ],
        'not_listed' => [
          'title' => 'Article{s} NOT listed',
          'internal' => FALSE,
        ],
        'duplicate' => [
          'title' => 'Duplicate article{s}',
          'internal' => TRUE,
        ],
        'review_ready' => [
          'title' => 'Article{s} ready for review',
          'internal' => FALSE,
        ],
        'topic_added' => [
          'title' => 'Article{s} with topic added',
          'internal' => FALSE,
        ],
        'replaced' => [
          'title' => 'Article{s} replaced',
          'internal' => TRUE,
        ],
        'error' => [
          'title' => 'Article{s} with errors',
          'internal' => TRUE,
        ],
      ];
    }

    // Collect the report data.
    $storage = \Drupal::entityTypeManager()->getStorage('ebms_article');
    $articles = [];
    $labels = [];
    $values = json_decode($this->report->value, TRUE);
    foreach ($values['actions'] as $action) {
      $disposition = $dispositions[$action['disposition']];
      $block = $blocks[$disposition];
      if (!empty($block['internal']) || !$internal) {
        $pmid = $action['source_id'];
        $ebms_id = $action['article'] ?? '';
        $message = $action['message'] ?? '';
        $article = ['id' => $ebms_id, 'message' => $message];
        $articles[$disposition][$pmid] = $article;
        if (!array_key_exists($pmid, $articles['unique'] ?? [])) {
          $articles['unique'][$pmid] = ['id' => $ebms_id];
        }
        if (!empty($ebms_id) && !array_key_exists($ebms_id, $labels)) {
          $entity = $storage->load($ebms_id);
          $labels[$ebms_id] = $entity->getLabel();
        }
      }
    }
    $report = [
      '#theme' => 'import_report',
      '#title' => $title,
    ];
    if (!empty($values['messages'])) {
      $report['#errors'] = [
        '#theme' => 'item_list',
        '#type' => 'ul',
        '#items' => $values['messages'],
      ];
    }

    // Add the blocks.
    $header = ['Source ID', 'EBMS ID', 'Publication', 'Messages'];
    $options = ['attributes' => ['target' => '_blank']];
    $pubmed_url = self::PUBMED_URL;
    foreach ($blocks as $key => $block) {
      if ($block['internal'] || !$internal) {
        $ids = $articles[$key] ?? [];
        $s = count($ids) === 1 ? '' : 's';
        $title = count($ids) . ' ' . str_replace('{s}', $s, $block['title']);
        ksort($ids, SORT_NUMERIC);
        $rows = [];
        foreach ($ids as $pmid => $article) {
          $url = "$pubmed_url/$pmid";
          $pubmed_link = [
            'data' => [
              '#type' => 'link',
              '#url' => Url::fromUri($url, $options),
              '#title' => $pmid,
            ],
          ];
          $ebms_id = $article['id'];
          $label = $labels[$ebms_id] ?? ' ';
          $message = $article['message'] ?? ' ';
          if ($ebms_id) {
            $ebms_link = [
              'data' => [
                '#type' => 'link',
                '#url' => Url::fromRoute('ebms_article.article', ['article' => $ebms_id], $options),
                '#title' => $ebms_id,
              ],
            ];
          }
          else {
            $ebms_link = ' ';
          }
          $rows[] = [$pubmed_link, $ebms_link, $label, $message];
        }
        $block = [
          '#type' => 'details',
          '#title' => $title,
          'articles' => [
            '#type' => 'table',
            '#rows' => $rows,
            '#header' => $header,
          ],
        ];
        $report['#blocks'][$key] = $block;
      }
    }

    // All done.
    return $report;
  }

}
