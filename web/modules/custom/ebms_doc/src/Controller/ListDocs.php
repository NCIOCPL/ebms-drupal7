<?php

namespace Drupal\ebms_doc\Controller;

use Drupal\Core\Controller\ControllerBase;
use Drupal\Core\Url;
//use Drupal\ebms_doc\Entity\Doc;

/**
 * Show individual document information.
 */
class ListDocs extends ControllerBase {

  /**
   * Display document information.
   */
  public function display(): array {
    $header = [
      'name' => [
        'data' => 'File Name',
        'field' => 'file.entity.filename',
        'specifier' => 'file.entity.filename',
      ],
      'uploaded' => [
        'data' => 'Uploaded',
        'field' => 'posted',
        'specifier' => 'posted',
        'sort' => 'desc',
      ],
      'user' => [
        'data' => 'By',
        'field' => 'file.entity.uid.entity.name',
        'specifier' => 'file.entity.uid.entity.name',
      ],
      'actions' => [
        'data' => 'Action Buttons',
      ],
    ];
    $storage = $this->entityTypeManager()->getStorage('ebms_doc');
    $query = $storage->getQuery()
      ->accessCheck(FALSE)
      ->condition('dropped', '1', '<>')
      ->tableSort($header);
    $rows = [];
    $opts = ['query' => \Drupal::request()->query->all()];
    foreach ($storage->loadMultiple($query->execute()) as $doc) {
      $edit = [
        '#type' => 'link',
        '#url' => Url::fromRoute('ebms_doc.edit', ['doc' => $doc->id()], $opts),
        '#title' => 'Edit',
        '#attributes' => ['class' => ['button', 'usa-button', 'inline-button']],
      ];
      $delete = [
        '#type' => 'link',
        '#url' => Url::fromRoute('ebms_doc.archive', ['ebms_doc' => $doc->id()], $opts),
        '#title' => 'Archive',
        '#attributes' => ['class' => ['button', 'usa-button', 'inline-button']],
      ];
      $row = [
        $doc->file->entity->filename->value,
        substr($doc->posted->value, 0, 10),
        $doc->file->entity->uid->entity->name->value,
        [
          'data' => [
            '#type' => 'container',
            'edit' => $edit,
            'delete' => $delete,
          ],
        ],
      ];
      $rows[] = $row;
    }
    return [
      '#title' => 'Documents',
      '#attached' => ['library' => ['ebms_doc/documents']],
      'add' => [
        '#title' => 'Post Document',
        '#type' => 'link',
        '#url' => Url::fromRoute('ebms_doc.create'),
        '#attributes' => ['class' => ['button', 'usa-button']],
      ],
      'table' => [
        '#theme' => 'table',
        '#attributes' => ['id' => 'ebms-document-table'],
        '#header' => $header,
        '#rows' => $rows,
        '#cache' => ['max-age' => 0],
      ],
    ];
  }

}
