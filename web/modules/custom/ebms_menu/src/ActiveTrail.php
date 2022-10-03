<?php

namespace Drupal\ebms_menu;

use Drupal\Core\Cache\CacheBackendInterface;
use Drupal\Core\Menu\MenuActiveTrail;
use Drupal\Core\Menu\MenuLinkManagerInterface;
use Drupal\Core\Routing\RouteMatchInterface;
use Drupal\Core\Lock\LockBackendInterface;
use Symfony\Component\HttpFoundation\RequestStack;

/**
 * Overrides the core service for figuring out which menu item is active.
 */
class ActiveTrail extends MenuActiveTrail {

  /**
   * Menu items for which Drupal doesn't get the active menu item right.
   */
  const EBMS_MENUS = [
    'main' => [
      '/article' => 'ebms_article.articles',
      '/assigned' => 'ebms_review.assigned_packets',
      '/calendar' => 'ebms_meeting.calendar',
      '/docs' => 'ebms_doc.collection',
      '/import' => 'ebms_import.request',
      '/packets' => 'ebms_review.packets',
      '/review/publish' => 'ebms_review.publish',
      '/review' => 'ebms_review.review_queue',
      '/search' => 'ebms_article.search',
      '/summaries' => 'ebms_summary.summaries',
      '/travel' => 'ebms_travel.overview',
      '/help' => 'ebms_help.help',
      '/ncihelp' => 'ebms_help.ncihelp',
    ],
    'travel' => [
      '/travel' => 'ebms_travel.landing_page',
      '/travel/directions' => 'ebms_travel.directions',
      '/travel/policies-and-procedures' => 'ebms_travel.policies_and_procedures',
      '/travel/hotel-request' => 'ebms_travel.hotel_request',
      '/travel/reiumbursement-request' => 'ebms_travel.reiumbursement_request',
      '/travel/manage-configuration' => 'ebms_travel.configuration',
    ],
    'help' => [
      '/help/login' => 'ebms_help.login',
      '/help/home' => 'ebms_help.home',
      '/help/search' => 'ebms_help.search',
      '/help/calendar' => 'ebms_help.calendar',
      '/help/packets' => 'ebms_help.packets',
      '/help/summaries' => 'ebms_help.summaries',
      '/help/travel' => 'ebms_help.travel',
      '/help/profile' => 'ebms_help.profile',
    ],
    'ncihelp' => [
      '/ncihelp/login' => 'ebms_help.nci_login',
      '/ncihelp/home' => 'ebms_help.nci_home',
    ],
  ];

  /**
   * The path for the currently-requested page.
   *
   * @var string
   */
  private $currentPath;

  /**
   * ActiveTrail constructor.
   * @param MenuLinkManagerInterface $menu_link_manager
   * @param RouteMatchInterface $route_match
   * @param CacheBackendInterface $cache
   * @param LockBackendInterface $lock
   * @param RequestStack $request_stack
   */
  public function __construct(MenuLinkManagerInterface $menu_link_manager, RouteMatchInterface $route_match, CacheBackendInterface $cache, LockBackendInterface $lock, RequestStack $request_stack) {
    parent::__construct($menu_link_manager, $route_match, $cache, $lock);
    $this->currentPath = $request_stack->getCurrentRequest()->getRequestUri();
  }

  /**
   * {@inheritdoc}
   */
  protected function doGetActiveTrailIds($menu_name) {
    $item = $this->lookupMenuItem($menu_name);
    if (!empty($item)) {
      return [$item => $item, '' => ''];
    }
    return parent::doGetActiveTrailIds($menu_name);
  }

  /**
   * See if this is a menu item we need to handle ourselves.
   *
   * @param string $menu_name
   *   For example, 'main' or 'travel'.
   *
   * @return string
   *   The ID of a menu item, or an empty string if none found.
   */
  private function lookupMenuItem($menu_name) {
    if ($menu_name === 'main') {
      foreach (self::EBMS_MENUS['main'] as $path => $item) {
        if (str_starts_with($this->currentPath, $path)) {
          return $item;
        }
      }
    }
    return self::EBMS_MENUS[$menu_name][$this->currentPath] ?? '';
  }

}
