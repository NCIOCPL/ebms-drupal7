<?php

namespace Drupal\ebms_breadcrumb\Breadcrumb;

use Drupal\Core\Breadcrumb\Breadcrumb;
use Drupal\Core\Breadcrumb\BreadcrumbBuilderInterface;
use Drupal\Core\Routing\RouteMatchInterface;
use Drupal\Core\Link;
use Drupal\ebms_article\Entity\Article;
use Drupal\ebms_review\Entity\Packet;
use Drupal\ebms_review\Entity\PacketArticle;
use Drupal\user\Entity\User;
use Symfony\Component\HttpFoundation\Request;
use Symfony\Component\HttpFoundation\RequestStack;

/**
 * Take control of the site's breadcrumbs.
 */
class Builder implements BreadcrumbBuilderInterface {

  /**
   * Routes with breadcrumb information (label/route pairs).
   */
  const ROUTES = [

    // The About page.
    'ebms_core.about' => [['About', '<none>']],

    // The Articles pages.
    'ebms_article.search_form' => [['Articles', '<none>']],
    'ebms_article.search_results' => [
      ['Articles', 'ebms_article.search_form'],
      ['Search Results', '<none>'],
    ],
    'ebms_article.article' => [
      ['Articles', 'ebms_article.search_form'],
      ['Full Article History', '<none>'],
    ],
    'ebms_import.import_form' => [
      ['Articles', 'ebms_article.search_form'],
      ['Import', '<none>'],
    ],
    'ebms_article.add_article_relationship' => [
      ['Articles', 'ebms_article.search_form'],
      ['Article', 'ebms_article.article'],
      ['Add Relationship', '<none>'],
    ],
    'ebms_article.edit_article_relationship' => [
      ['Articles', 'ebms_article.search_form'],
      ['Article', 'ebms_article.article'],
      ['Edit Relationship', '<none>'],
    ],
    'ebms_article.delete_article_relationship' => [
      ['Articles', 'ebms_article.search_form'],
      ['Article', 'ebms_article.article'],
      ['Delete Relationship', '<none>'],
    ],
    'ebms_article.add_article_tag' => [
      ['Articles', 'ebms_article.search_form'],
      ['Article', 'ebms_article.article'],
      ['Add Article Tag', '<none>'],
    ],
    'ebms_article.add_article_tag_comment' => [
      ['Articles', 'ebms_article.search_form'],
      ['Article', 'ebms_article.article'],
      ['Add Article Tag Comment', '<none>'],
    ],
    'ebms_article.add_article_topic' => [
      ['Articles', 'ebms_article.search_form'],
      ['Article', 'ebms_article.article'],
      ['Add Article Topic', '<none>'],
    ],
    'ebms_article.add_new_state' => [
      ['Articles', 'ebms_article.search_form'],
      ['Article', 'ebms_article.article'],
      ['Add Article State', '<none>'],
    ],
    'ebms_article.add_state_comment' => [
      ['Articles', 'ebms_article.search_form'],
      ['Article', 'ebms_article.article'],
      ['Add State Comment', '<none>'],
    ],
    'ebms_article.add_manager_topic_comment' => [
      ['Articles', 'ebms_article.search_form'],
      ['Article', 'ebms_article.article'],
      ['Add Manager Topic Comment', '<none>'],
    ],
    'ebms_article.edit_manager_topic_comment' => [
      ['Articles', 'ebms_article.search_form'],
      ['Article', 'ebms_article.article'],
      ['Edit Manager Topic Comment', '<none>'],
    ],
    'ebms_article.delete_manager_topic_comment' => [
      ['Articles', 'ebms_article.search_form'],
      ['Article', 'ebms_article.article'],
      ['Delete Manager Topic Comment', '<none>'],
    ],
    'ebms_article.add_full_text' => [
      ['Articles', 'ebms_article.search_form'],
      ['Article', 'ebms_article.article'],
      ['Add Article Full Text', '<none>'],
    ],
    'ebms_review.review_queue' => [
      ['Articles', 'ebms_article.search_form'],
      ['Review Queue', '<none>'],
    ],
    'ebms_review.add_review_topic' => [
      ['Articles', 'ebms_article.search_form'],
      ['Review Queue', 'ebms_review.review_queue'],
      ['Add Review Topic', '<none>'],
    ],
    'ebms_review.publish' => [
      ['Articles', 'ebms_article.search_form'],
      ['Batch Publish', '<none>'],
    ],
    'ebms_article.full_text_queue' => [
      ['Articles', 'ebms_article.search_form'],
      ['Full-Text Retrieval', '<none>'],
    ],

    // Internal article pages.
    'ebms_article.internal_articles' => [
      ['Articles', 'ebms_article.search_form'],
      ['Internal Articles', '<none>'],
    ],
    'ebms_import.import_internal_articles' => [
      ['Articles', 'ebms_article.search_form'],
      ['Import Internal Articles', '<none>'],
    ],
    'ebms_article.add_internal_comment' => [
      ['Articles', 'ebms_article.search_form'],
      ['Article', 'ebms_article.article'],
      ['Add Internal Comment', '<none>'],
    ],
    'ebms_article.edit_internal_comment' => [
      ['Articles', 'ebms_article.search_form'],
      ['Article', 'ebms_article.article'],
      ['Edit Internal Comment', '<none>'],
    ],
    'ebms_article.delete_internal_comment' => [
      ['Articles', 'ebms_article.search_form'],
      ['Article', 'ebms_article.article'],
      ['Delete Internal Comment', '<none>'],
    ],
    'ebms_article.internal_tags' => [
      ['Articles', 'ebms_article.search_form'],
      ['Article', 'ebms_article.article'],
      ['Internal Tags', '<none>'],
    ],

    // Documents.
    'ebms_doc.list' => [['Documents', '<none>']],
    'ebms_doc.create' => [
      ['Documents', 'ebms_doc.list'],
      ['Post Document', '<none>'],
    ],
    'ebms_doc.edit' => [
      ['Documents', 'ebms_doc.list'],
      ['Edit Document', '<none>'],
    ],
    'ebms_doc.archive' => [
      ['Documents', 'ebms_doc.list'],
      ['Archive Document', '<none>'],
    ],

    // Journals.
    'ebms_journal.maintenance' => [['Journals', '<none>']],

    // Calendar.
    'ebms_meeting.calendar' => [['Calendar', '<none>']],
    'ebms_meeting.calendar_month' => [['Calendar', '<none>']],
    'ebms_meeting.add_meeting' => [
      ['Calendar', 'ebms_meeting.calendar'],
      ['Add Meeting', '<none>'],
    ],
    'ebms_meeting.meeting' => [
      ['Calendar', 'ebms_meeting.calendar'],
      ['Meeting', '<none>'],
    ],
    'ebms_meeting.edit_meeting' => [
      ['Calendar', 'ebms_meeting.calendar'],
      ['Meeting', 'ebms_meeting.meeting'],
      ['Edit Meeting', '<none>'],
    ],
    'ebms_meeting.calendar_month' => [
      ['Calendar', 'ebms_meeting.calendar'],
      ['Month', '<none>'],
    ],

    // Packets/reviews.
    'ebms_review.packets' => [['Packets', '<none>']],
    'ebms_review.assigned_packets' => [['Assigned Packets', '<none>']],
    'ebms_review.add_review' => [
      ['Packets', 'ebms_review.packets'],
      ['Assigned Packets', 'ebms_review.assigned_packets'],
      ['Record Responses', 'ebms_review.record_responses'],
      ['Packets For User', 'ebms_review.record_assigned_packets'],
      ['Packet', 'ebms_review.assigned_packet'],
      ['Add Review', '<none>'],
    ],
    'ebms_review.assigned_packet' => [
      ['Packets', 'ebms_review.packets'],
      ['Assigned Packets', 'ebms_review.assigned_packets'],
      ['Record Responses', 'ebms_review.record_responses'],
      ['Packets For User', 'ebms_review.record_assigned_packets'],
      ['Packet', '<none>'],
    ],
    'ebms_review.completed_packets' => [
      ['Completed Packets', '<none>'],
    ],
    'ebms_review.completed_packet' => [
      ['Completed Packets', 'ebms_review.completed_packets'],
      ['Completed Packet', '<none>'],
    ],
    'ebms_review.details' => [
      ['Packets', 'ebms_review.packets'],
      ['Reviewed Packets', 'ebms_review.reviewed_packets'],
      ['Packet', 'ebms_review.reviewed_packet'],
      ['Details', '<none>'],
    ],
    'ebms_review.fyi_packet' => [
      ['FYI Packets', 'ebms_review.fyi_packets'],
      ['Packet', '<none>'],
    ],
    'ebms_review.fyi_packets' => [
      ['FYI Packets', '<none>'],
    ],
    'ebms_review.other_reviews' => [
      ['Packets', 'ebms_review.packets'],
      ['Assigned Packets', 'ebms_review.assigned_packets'],
      ['Record Responses', 'ebms_review.record_responses'],
      ['Packets For User', 'ebms_review.record_assigned_packets'],
      ['Packet', 'ebms_review.assigned_packet'],
      ['Article', 'ebms_review.add_review'],
      ['Other Reviews', '<none>']
    ],
    'ebms_review.packet_form' => [
      ['Packets', 'ebms_review.packets'],
      ['Create Packet', '<none>'],
    ],
    'ebms_review.packet_edit_form' => [
      ['Packets', 'ebms_review.packets'],
      ['Edit Packet', '<none>'],
    ],
    'ebms_review.record_assigned_packets' => [
      ['Packets', 'ebms_review.packets'],
      ['Record Responses', 'ebms_review.record_responses'],
      ['Packets For User', '<none>'],
    ],
    'ebms_review.record_responses' => [
      ['Packets', 'ebms_review.packets'],
      ['Record Responses', '<none>'],
    ],
    'ebms_review.reviewed_packet' => [
      ['Packets', 'ebms_review.packets'],
      ['Reviewed Packets', 'ebms_review.reviewed_packets'],
      ['Packet', '<none>'],
    ],
    'ebms_review.reviewed_packets' => [
      ['Packets', 'ebms_review.packets'],
      ['Reviewed Packets', '<none>'],
    ],
    'ebms_review.reviewer_doc_form' => [
      ['Assigned Packets', 'ebms_review.assigned_packets'],
      ['Assigned Packet', 'ebms_review.assigned_packet'],
      ['Post Document', '<none>'],
    ],
    'ebms_review.unreviewed_packet' => [
      ['Packets', 'ebms_review.packets'],
      ['Unreviewed Packets', 'ebms_review.unreviewed_packets'],
      ['Packet', '<none>'],
    ],
    'ebms_review.unreviewed_packets' => [
      ['Packets', 'ebms_review.packets'],
      ['Unreviewed Packets', '<none>'],
    ],
    'ebms_review.archive_packet' => [
      ['Packets', 'ebms_review.packets'],
      ['Reviewed Packets', 'ebms_review.reviewed_packets'],
      ['Packet', 'ebms_review.reviewed_packet'],
      ['Archive Packet', '<none>'],
    ],

    // Reports.
    'ebms_report.landing_page' => [['Reports', '<none>']],
    'ebms_report.abandoned_articles' => [
      ['Reports', 'ebms_report.landing_page'],
      ['Invalid PubMed IDs', '<none>'],
    ],
    'ebms_report.articles' => [
      ['Reports', 'ebms_report.landing_page'],
      ['Article Statistics', '<none>'],
    ],
    'ebms_report.articles_by_status' => [
      ['Reports', 'ebms_report.landing_page'],
      ['Articles By Status', '<none>'],
    ],
    'ebms_report.articles_by_tag' => [
      ['Reports', 'ebms_report.landing_page'],
      ['Articles By Tag', '<none>'],
    ],
    'ebms_report.articles_without_responses' => [
      ['Reports', 'ebms_report.landing_page'],
      ['Articles Without Responses', '<none>'],
    ],
    'ebms_report.board_members' => [
      ['Reports', 'ebms_report.landing_page'],
      ['Board Membership', '<none>'],
    ],
    'ebms_report.board_member_logins' => [
      ['Reports', 'ebms_report.landing_page'],
      ['Board Member Logins', '<none>'],
    ],
    'ebms_report.documents' => [
      ['Reports', 'ebms_report.landing_page'],
      ['Documents', '<none>'],
    ],
    'ebms_report.hotel_requests' => [
      ['Reports', 'ebms_report.landing_page'],
      ['Hotel Requests', '<none>'],
    ],
    'ebms_report.import' => [
      ['Reports', 'ebms_report.landing_page'],
      ['Import', '<none>'],
    ],
    'ebms_report.literature_reviews' => [
      ['Reports', 'ebms_report.landing_page'],
      ['Literature Reviews', '<none>'],
    ],
    'ebms_report.meeting_dates' => [
      ['Reports', 'ebms_report.landing_page'],
      ['Meeting Dates', '<none>'],
    ],
    'ebms_report.recent_activity' => [
      ['Reports', 'ebms_report.landing_page'],
      ['Recent Activity', '<none>'],
    ],
    'ebms_report.recent_activity_report' => [
      ['Reports', 'ebms_report.landing_page'],
      ['Recent Activity', '<none>'],
    ],
    'ebms_report.reimbursement_requests' => [
      ['Reports', 'ebms_report.landing_page'],
      ['Reimbursement Requests', '<none>'],
    ],
    'ebms_report.responses_by_reviewer' => [
      ['Reports', 'ebms_report.landing_page'],
      ['Responses By Reviewer', '<none>'],
    ],
    'ebms_report.statistics' => [
      ['Reports', 'ebms_report.landing_page'],
      ['Statistics', '<none>'],
    ],
    'ebms_report.topic_reviewers' => [
      ['Reports', 'ebms_report.landing_page'],
      ['Topic Reviewers', '<none>'],
    ],
    'ebms_topic.reviewers' => [
      ['Reports', 'ebms_report.landing_page'],
      ['Topic Reviewers', 'ebms_report.topic_reviewers'],
      ['Edit', '<none>'],
    ],

    // Summaries.
    'ebms_summary.board' => [
      ['Summaries', '<none>'],
    ],
    'ebms_summary.add_board_doc' => [
      ['Summaries', 'ebms_summary.board'],
      ['Add Board Document', '<none>'],
    ],
    'ebms_summary.add_page' => [
      ['Summaries', 'ebms_summary.board'],
      ['Add Page', '<none>'],
    ],
    'ebms_summary.edit_page' => [
      ['Summaries', 'ebms_summary.board'],
      ['Edit Page', '<none>'],
    ],
    'ebms_summary.page' => [
      ['Summaries', 'ebms_summary.board'],
      ['Summary Topic Page', '<none>'],
    ],
    'ebms_summary.add_summary_link' => [
      ['Summaries', 'ebms_summary.board'],
      ['Summary Topic Page', 'ebms_summary.page'],
      ['Add Summary Link', '<none>'],
    ],
    'ebms_summary.edit_summary_link' => [
      ['Summaries', 'ebms_summary.board'],
      ['Summary Topic Page', 'ebms_summary.page'],
      ['Edit Summary Link', '<none>'],
    ],
    'ebms_summary.delete_summary_link' => [
      ['Summaries', 'ebms_summary.board'],
      ['Summary Topic Page', 'ebms_summary.page'],
      ['Delete Summary Link', '<none>'],
    ],
    'ebms_summary.delete_page' => [
      ['Summaries', 'ebms_summary.board'],
      ['Summary Topic Page', 'ebms_summary.page'],
      ['Delete Page', '<none>'],
    ],
    'ebms_summary.add_manager_doc' => [
      ['Summaries', 'ebms_summary.board'],
      ['Summary Topic Page', 'ebms_summary.page'],
      ['Add Manager Document', '<none>'],
    ],
    'ebms_summary.add_member_doc' => [
      ['Summaries', 'ebms_summary.board'],
      ['Summary Topic Page', 'ebms_summary.page'],
      ['Add Board Member Document', '<none>'],
    ],

    // Travel.
    'ebms_travel.landing_page' => [['Travel', '<none>']],
    'ebms_travel.configuration' => [
      ['Travel', 'ebms_travel.landing_page'],
      ['Configuration', '<none>'],
    ],
    'ebms_travel.directions' => [
      ['Travel', 'ebms_travel.landing_page'],
      ['Directions', '<none>'],
    ],
    'ebms_travel.hotel_request' => [
      ['Travel', 'ebms_travel.landing_page'],
      ['Hotel Request', '<none>'],
    ],
    'ebms_travel.policies_and_procedures' => [
      ['Travel', 'ebms_travel.landing_page'],
      ['Policies And Procedures', '<none>'],
    ],

    // Help pages.
    'ebms_help.help' => [['Help', '<none>']],
    'ebms_help.login' => [
      ['Help', 'ebms_core.help'],
      ['Login/Logout', '<none>'],
    ],
    'ebms_help.home' => [
      ['Help', 'ebms_core.help'],
      ['Home', '<none>'],
    ],
    'ebms_help.search' => [
      ['Help', 'ebms_core.help'],
      ['Article Search', '<none>'],
    ],
    'ebms_help.calendar' => [
      ['Help', 'ebms_core.help'],
      ['Calendar', '<none>'],
    ],
    'ebms_help.packets' => [
      ['Help', 'ebms_core.help'],
      ['Packets', '<none>'],
    ],
    'ebms_help.summaries' => [
      ['Help', 'ebms_core.help'],
      ['Summaries', '<none>'],
    ],
    'ebms_help.travel' => [
      ['Help', 'ebms_core.help'],
      ['Travel', '<none>'],
    ],

    // Admin.
    // @todo Maybe leave these out so our admin breadcrumbs conform to the Drupal pattern?
    'ebms_core.admin_config_ebms' => [
      ['Administration', 'system.admin'],
      ['Configuration', 'system.admin_config'],
      ['EBMS', '<none>'],
    ],
    'ebms_core.publication_type_hierarchy' => [
      ['Administration', 'system.admin'],
      ['Configuration', 'system.admin_config'],
      ['EBMS', 'ebms_core.admin_config_ebms'],
      ['MeSH Publication Type Hierarchy', '<none>'],
    ],
    'ebms_journal.refresh' => [
      ['Administration', 'system.admin'],
      ['Configuration', 'system.admin_config'],
      ['EBMS', 'ebms_core.admin_config_ebms'],
      ['Journal Refresh', '<none>'],
    ],
    'entity.ebms_meeting.collection' => [
      ['Administration', 'system.admin'],
      ['Configuration', 'system.admin_config'],
      ['EBMS', 'ebms_core.admin_config_ebms'],
      ['Meetings', '<none>'],
    ],
  ];

  /**
   * Breadcrumb object we're building.
   */
  private Breadcrumb $breadcrumb;

  /**
   * Currently logged-on user.
   */
  private User $user;

  /**
   * Name of the current request's route.
   */
  private string $route;

  /**
   * Current request.
   */
  private Request $request;

  /**
   * Stack for the current request.
   */
  private RequestStack $stack;

  /**
   * {@inheritdoc}
   */
  public function __construct(RequestStack $request_stack) {
    $this->stack = $request_stack;
    $this->request = $this->stack->getCurrentRequest();
  }

  /**
   * {@inheritdoc}
   */
  public function applies(RouteMatchInterface $route_match): bool {
    $route = $route_match->getRouteName();
    ebms_debug_log("breadcrumbs route is $route");
    if ($route === 'entity.node.canonical' || in_array($route, array_keys(self::ROUTES))) {
      return TRUE;
    }
    return FALSE;
  }

  /**
   * {@inheritdoc}
   */
  public function build(RouteMatchInterface $route_match): Breadcrumb {

	  // Establish some instance properties needed further down the call stack.
    $this->breadcrumb = new Breadcrumb();
    $this->route = $route_match->getRouteName();
  	$this->user = User::load(\Drupal::currentUser()->id());

	  // All the pages we've as covered by our class have at least a home-page breadcrumb.
    $this->breadcrumb->addLink(Link::createFromRoute('Home', '<front>'));

	  // Special handling for routes created by Drupal core (for nodes).
    if ($this->route === 'entity.node.canonical') {
      $current_path = $this->request->getRequestUri();
      if (str_starts_with($current_path, '/travel')) {
        if ($current_path === '/travel') {
          $this->breadcrumb->addLink(Link::createFromRoute('Travel', '<none>'));
        }
        else {
          $this->breadcrumb->addLink(Link::createFromRoute('Travel', 'ebms_travel.landing_page'));
          switch ($current_path) {
            case '/travel/directions':
              $this->breadcrumb->addLink(Link::createFromRoute('Directions', '<none>'));
              break;
            case '/travel/policies-and-procedures':
              $this->breadcrumb->addLink(Link::createFromRoute('Policies and Procedures', '<none>'));
              break;
          }
        }
      }
      if (str_starts_with($current_path, '/help')) {
        if ($current_path === '/help') {
          $this->breadcrumb->addLink(Link::createFromRoute('Help', '<none>'));
        }
        else {
          $this->breadcrumb->addLink(Link::createFromRoute('Help', 'ebms_help.help'));
          switch ($current_path) {
            case '/help/login':
              $this->breadcrumb->addLink(Link::createFromRoute('Login/Logout', '<none>'));
              break;
            case '/help/home':
              $this->breadcrumb->addLink(Link::createFromRoute('Home Page', '<none>'));
              break;
            case '/help/search':
              $this->breadcrumb->addLink(Link::createFromRoute('Article Search', '<none>'));
              break;
            case '/help/calendar':
              $this->breadcrumb->addLink(Link::createFromRoute('Calendar', '<none>'));
              break;
            case '/help/packets':
              $this->breadcrumb->addLink(Link::createFromRoute('Packets', '<none>'));
              break;
            case '/help/summaries':
              $this->breadcrumb->addLink(Link::createFromRoute('Summaries', '<none>'));
              break;
            case '/help/travel':
              $this->breadcrumb->addLink(Link::createFromRoute('Travel', '<none>'));
              break;
            case '/help/profile':
              $this->breadcrumb->addLink(Link::createFromRoute('Profile', '<none>'));
              break;
          }
        }
      }
      if (str_starts_with($current_path, '/ncihelp')) {
        if ($current_path === '/ncihelp') {
          $this->breadcrumb->addLink(Link::createFromRoute('Help', '<none>'));
        }
        else {
          $this->breadcrumb->addLink(Link::createFromRoute('Help', 'ebms_help.ncihelp'));
          switch ($current_path) {
            case '/ncihelp/login':
              $this->breadcrumb->addLink(Link::createFromRoute('Login/Logout', '<none>'));
              break;
            case '/ncihelp/home':
              $this->breadcrumb->addLink(Link::createFromRoute('Home Page', '<none>'));
              break;
            case '/ncihelp/search':
              $this->breadcrumb->addLink(Link::createFromRoute('Article Search', '<none>'));
              break;
            case '/ncihelp/calendar':
              $this->breadcrumb->addLink(Link::createFromRoute('Calendar', '<none>'));
              break;
            case '/ncihelp/packets':
              $this->breadcrumb->addLink(Link::createFromRoute('Packets', '<none>'));
              break;
            case '/ncihelp/summaries':
              $this->breadcrumb->addLink(Link::createFromRoute('Summaries', '<none>'));
              break;
            case '/ncihelp/travel':
              $this->breadcrumb->addLink(Link::createFromRoute('Travel', '<none>'));
              break;
            case '/ncihelp/profile':
              $this->breadcrumb->addLink(Link::createFromRoute('Profile', '<none>'));
              break;
          }
        }
      }
      ebms_debug_log("request URI is $current_path");
    }

	  // Breadcrumbs for routes we created ourselves.
    elseif (array_key_exists($this->route, self::ROUTES)) {
      foreach (self::ROUTES[$this->route] as list($label, $route)) {
		    $this->addLink($route_match, $label, $route);
	    }
    }

  	// Make caching work correctly and return the breadcrumb object.
    $this->breadcrumb->addCacheContexts(['route', 'user.roles']);
    return $this->breadcrumb;
  }

  /**
   * Add a breadcrumb to our object.
   *
   * @param RouteMatchInterface $route_match
   *  Information about the current request's route.
   * @param string $label
   *  Default display for the breadcrumb (overridden in some cases).
   * @param string $route
   *  Name of the route for this breadcrumb (not to be confused with the
   *  route for the current request, which is in $this->route).
   */
  private function addLink(RouteMatchInterface $route_match, $label, $route) {
    $parms = [];
    $opts = [];
    switch ($route) {
      case '<none>':
        switch ($this->route) {
          case 'ebms_article.article':
            $article = $route_match->getParameter('article');
            $pmid = $article->source_id->value;
            $label = "PMID $pmid";
            break;
          case 'ebms_review.add_review':
            $packet_article = PacketArticle::load($route_match->getRawParameter('packet_article_id'));
            $pmid = $packet_article->article->entity->source_id->value;
            $label = "Review of PMID $pmid";
            break;
          case 'ebms_review.assigned_packet':
          case 'ebms_review.completed_packet':
          case 'ebms_review.fyi_packet':
          case 'ebms_review.packet_edit_form':
          case 'ebms_review.reviewed_packet':
          case 'ebms_review.unreviewed_packet':
            $packet = Packet::load($route_match->getRawParameter('packet_id'));
            $label = $packet->title->value;
            break;
          case 'ebms_review.record_assigned_packets':
            $obo = $this->request->query->get('obo');
            if (!empty($obo)) {
              $label = User::load($obo)->name->value;
            }
            break;
          case 'ebms_summary.page':
            $this->breadcrumb->addCacheTags(['summary-topic-page-bookmark']);
            $page = $route_match->getParameter('summary_page');
            $label = $page->name->value;
            break;
        }
        break;
      case 'ebms_article.article':
        $article_id = $route_match->getRawParameter('article_id');
        if (empty($article_id)) {
          $article_id = $route_match->getRawParameter('article');
        }
        if (empty($article_id)) {
          $article_id = $this->request->query->get('article');
        }
        $article = Article::load($article_id);
        $pmid = $article->source_id->value;
        $label = "PMID $pmid";
        $parms['article'] = $article_id;
        break;
      case 'ebms_meeting.meeting':
        $parms['meeting'] = $route_match->getRawParameter('meeting');
        break;
      case 'ebms_review.add_review':
        $packet_id = $route_match->getRawParameter('packet_id');
        $packet_article_id = $route_match->getRawParameter('packet_article_id');
        $packet_article = PacketArticle::load($packet_article_id);
        $pmid = $packet_article->article->entity->source_id->value;
        $label = "Review of PMID $pmid";
        $parms['packet_id'] = $packet_id;
        $parms['packet_article_id'] = $packet_article_id;
        $obo = $this->request->query->get('obo');
        if (!empty($obo)) {
          $opts['obo'] = $obo;
        }
        break;
      case 'ebms_review.assigned_packet':
      case 'ebms_review.reviewed_packet':
      case 'ebms_review.unreviewed_packet':
        $packet_id = $route_match->getRawParameter('packet_id');
        if (empty($packet_id)) {
          $packet_id = $route_match->getRawParameter('ebms_packet');
        }
        $packet = Packet::load($packet_id);
        $label = $packet->title->value;
        $parms['packet_id'] = $packet_id;
        $filter_id = $this->request->query->get('filter-id');
        if (!empty($filter_id)) {
          $opts['filter-id'] = $filter_id;
        }
        $obo = $this->request->query->get('obo');
        if (!empty($obo)) {
          $opts['obo'] = $obo;
        }
        if ($route === 'ebms_review.reviewed_packet' && !empty($this->request->query->get('unreviewed'))) {
          $route = 'ebms_review.unreviewed_packet';
        }
        break;
      case 'ebms_review.assigned_packets':
        if (!$this->user->hasRole('board_member')) {
          return;
        }
        break;
      case 'ebms_review.packets':
        if ($this->user->hasRole('board_member')) {
          return;
        }
        $filter_id = $this->request->query->get('filter-id');
        if (!empty($filter_id)) {
          $parms['request_id'] = $filter_id;
        }
        $page = $this->request->query->get('page');
        if (!empty($page)) {
          $opts['page'] = $page;
        }
        break;
      case 'ebms_review.record_assigned_packets':
        if ($this->user->hasRole('board_member')) {
          return;
        }
        $obo = $this->request->query->get('obo');
        if (!empty($obo)) {
          $label = User::load($obo)->name->value;
          $opts['obo'] = $obo;
        }
        break;
      case 'ebms_review.record_responses':
        if ($this->user->hasRole('board_member')) {
          return;
        }
        break;
      case 'ebms_review.review_queue':
        $queue_id = $this->request->query->get('queue');
        $parms['queue_id'] = $queue_id;
        break;
      case 'ebms_review.reviewed_packets':
      case 'ebms_review.unreviewed_packets':
        $filter_id = $this->request->query->get('filter-id');
        if (!empty($filter_id)) {
          $parms['filter_id'] = $filter_id;
        }
        if ($route === 'ebms_review.reviewed_packets' && !empty($this->request->query->get('unreviewed'))) {
          $route = 'ebms_review.unreviewed_packets';
          $label = 'Unreviewed Packets';
        }
        break;
      case 'ebms_summary.page':
        $page = $route_match->getParameter('summary_page');
        if (empty($page)) {
          $page = $route_match->getParameter('ebms_summary_page');
        }
        $parms['summary_page'] = $page->id();
        $label = $page->name->value;
        $this->breadcrumb->addCacheTags(['summary-topic-page-bookmark']);
        break;
    }
	  $this->breadcrumb->addLink(Link::createFromRoute($label, $route, $parms, ['query' => $opts]));
  }

}
