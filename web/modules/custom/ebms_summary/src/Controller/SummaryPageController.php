<?php

namespace Drupal\ebms_summary\Controller;

use Drupal\Core\Controller\ControllerBase;
use Drupal\Core\Url;
use Drupal\ebms_doc\Entity\Doc;
use Drupal\ebms_summary\Entity\SummaryPage;
use Drupal\user\Entity\User;
use Symfony\Component\DependencyInjection\ContainerInterface;
use Symfony\Component\HttpFoundation\RequestStack;

/**
 * Show a page of summary links.
 */
class SummaryPageController extends ControllerBase {

  /**
   * Request stack.
   *
   * @var RequestStack
   */
  public $request;

  /**
   * Inject our own request stack service property.
   *
   * @param RequestStack $request
   *   Request stack.
   */
  public function __construct(RequestStack $request) {
    $this->request = $request->getCurrentRequest();
  }

  /**
   * {@inheritdoc}
   */
  public static function create(ContainerInterface $container) {
    return new static($container->get('request_stack'));
  }

  /**
   * Display PDQ® Summary information.
   *
   * Assemble the render array for the requested summaries page.
   * The landing page for summaries shows all of the user's boards
   * (or all boards, for roles not tied to a specific board).  From
   * the landing page the user can navigate to the page showing a
   * specific board, and all of the summary subpages created for
   * that board (as well as any summary supporting documents for
   * the board).  From the board page, the user can drill down
   * into a specific subpage where links to individidual summaries
   * on cancer.gov are shown, as well as summary documents posted
   * by board managers ("documents posted by NCI") and by board
   * members ("documents posted by board members").
   */
  public function display(SummaryPage $summary_page): array {

    // Handle any requested actions.
    $query_parameters = $this->request->query->all();
    $delta = $this->request->get('archive-nci-doc');
    if (is_numeric($delta)) {
      $summary_page->manager_docs[$delta]->active = 0;
      $summary_page->save();
      unset($query_parameters['archive-nci-doc']);
    }
    $delta = $this->request->get('revive-nci-doc');
    if (is_numeric($delta)) {
      $summary_page->manager_docs[$delta]->active = 1;
      $summary_page->save();
      unset($query_parameters['revive-nci-doc']);
    }
    $delta = $this->request->get('archive-member-doc');
    if (is_numeric($delta)) {
      $summary_page->member_docs[$delta]->active = 0;
      $summary_page->save();
      unset($query_parameters['archive-member-doc']);
    }
    $delta = $this->request->get('revive-member-doc');
    if (is_numeric($delta)) {
      $summary_page->member_docs[$delta]->active = 1;
      $summary_page->save();
      unset($query_parameters['revive-member-doc']);
    }

    // Create the summary links.
    $user = User::load($this->currentUser()->id());
    $manager = $user->hasPermission('manage summaries');
    $member = $user->hasPermission('review literature');
    $options = ['query' => $query_parameters];
    $parms = ['summary_page' => $summary_page->id()];
    $links = [];
    foreach ($summary_page->links as $delta => $link) {
      $values = ['url' => $link->uri, 'text' => $link->title];
      if ($manager) {
        $parms['delta'] = $delta;
        $values['edit'] = Url::fromRoute('ebms_summary.edit_summary_link', $parms, $options);
        $values['delete'] = Url::fromRoute('ebms_summary.delete_summary_link', $parms, $options);
      }
      $links[] = $values;
    }
    usort($links, function($a, $b) {
      return $a['text'] <=> $b['text'];
    });
    $page = [
      '#title' => $summary_page->name->value,
      '#cache' => ['max-age' => 0],
      'summaries' => [
        '#theme' => 'summary_links',
        '#links' => $links,
      ],
    ];

    // Add a button for creating a new link.
    if ($manager) {
      $page['add-link-button'] = [
        '#theme' => 'links',
        '#links' => [
          [
            'url' => Url::fromRoute('ebms_summary.add_summary_link', ['summary_page' => $summary_page->id()], $options),
            'title' => 'Add New Summary Link',
            'attributes' => ['class' => ['button', 'usa-button']],
          ],
        ],
      ];
    }

    // Add the table for the documents posted by NCI staff.
    $show_archived_nci_docs = $this->request->get('archived-nci-docs') === 'show';
    $nci_docs = [];
    $route = 'ebms_summary.page';
    $parms = ['summary_page' => $summary_page->id()];
    foreach ($summary_page->manager_docs as $delta => $doc_usage) {
      $active = $doc_usage->active;
      if ($active || $manager && $show_archived_nci_docs) {
        $doc = Doc::load($doc_usage->doc);
        $file = $doc->file->entity;
        $text = $doc->description->value ?: $file->filename->value;
        $url = Url::fromUri($file->createFileUrl(FALSE));
        $values = [
          'text' => $text,
          'url' => $url,
          'notes' => $doc_usage->notes,
          'user' => $file->uid->entity->name->value,
          'date' => substr($doc->posted->value, 0, 10),
          'archived' => !$active,
        ];
        if ($manager) {
          $options = ['query' => $query_parameters];
          $action = $active ? 'archive-nci-doc' : 'revive-nci-doc';
          $options['query'][$action] = $delta;
          $url = Url::fromRoute($route, $parms, $options)->toString();
          $onclick = "location.href='$url'";
          $values['onclick'] = $onclick;
        }
        $nci_docs[] = $values;
      }
    }
    $header = ['File Name', 'Notes', 'Uploaded By', 'Date'];
    if ($manager) {
      $header[] = 'Archived';
    }
    $page['nci-docs'] = [
      '#theme' => 'doc_table',
      '#caption' => 'Documents Posted by NCI',
      '#header' => $header,
      '#rows' => array_reverse($nci_docs),
      '#empty' => 'No documents have been posted by NCI for this page yet.',
    ];

    // Add buttons below the table if appropriate.
    $buttons = [];
    if ($manager) {
      $eligible_docs = $summary_page->eligibleDocs();
      if (!empty($eligible_docs)) {
        $options = ['query' => $query_parameters];
        $buttons[] = [
          'url' => Url::fromRoute('ebms_summary.add_manager_doc', $parms, $options),
          'title' => 'Post Document',
          'attributes' => ['class' => ['button', 'usa-button']],
        ];
      }
    }
    if ($manager && !empty($summary_page->manager_docs)) {
      $action = $show_archived_nci_docs ? 'hide' : 'show';
      $options = ['query' => ['archived-nci-docs' => $action]];
      $buttons[] = [
        'url' => Url::fromRoute($route, $parms, $options),
        'title' => ucfirst($action) . ' Archived NCI Documents',
        'attributes' => ['class' => ['button', 'usa-button']],
      ];
    }
    if (!empty($buttons)) {
      $page['nci-doc-buttons'] = [
        '#theme' => 'links',
        '#links' => $buttons,
      ];
    }

    // Add the table for the documents posted by the board members.
    $show_archived_member_docs = $this->request->get('archived-member-docs') === 'show';
    $member_docs = [];
    foreach ($summary_page->member_docs as $delta => $doc_usage) {
      $active = $doc_usage->active;
      if ($active || $manager && $show_archived_member_docs) {
        $doc = Doc::load($doc_usage->doc);
        $file = $doc->file->entity;
        $text = $doc->description->value ?: $file->filename->value;
        $url = Url::fromUri($file->createFileUrl(FALSE));
        $values = [
          'text' => $text,
          'url' => $url,
          'notes' => $doc_usage->notes,
          'user' => $file->uid->entity->name->value,
          'date' => substr($doc->posted->value, 0, 10),
          'archived' => !$active,
        ];
        if ($manager) {
          $options = ['query' => $query_parameters];
          $action = $active ? 'archive-member-doc' : 'revive-member-doc';
          $options['query'][$action] = $delta;
          $url = Url::fromRoute($route, $parms, $options)->toString();
          $onclick = "location.href='$url'";
          $values['onclick'] = $onclick;
        }
        $member_docs[] = $values;
      }
    }
    $page['reviewer-docs'] = [
      '#theme' => 'doc_table',
      '#caption' => 'Documents Posted by Board Members',
      '#header' => $header,
      '#rows' => array_reverse($member_docs),
      '#empty' => 'No documents have been posted by board members for this page yet.',
    ];

    // Add buttons below the table if appropriate.
    $buttons = [];
    if ($member) {
      $options = ['query' => $query_parameters];
      $buttons[] = [
        'url' => Url::fromRoute('ebms_summary.add_member_doc', $parms, $options),
        'title' => 'Post Document',
        'attributes' => ['class' => ['button', 'usa-button']],
      ];
    }
    if ($manager && !empty($summary_page->member_docs)) {
      $action = $show_archived_member_docs ? 'hide' : 'show';
      $options = ['query' => ['archived-member-docs' => $action]];
      $buttons[] = [
        'url' => Url::fromRoute($route, $parms, $options),
        'title' => ucfirst($action) . ' Archived Member Documents',
        'attributes' => ['class' => ['button', 'usa-button']],
      ];
    }
    if (!empty($buttons)) {
      $page['member-doc-buttons'] = [
        '#theme' => 'links',
        '#links' => $buttons,
      ];
    }
    return $page;
  }

}
