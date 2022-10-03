<?php

namespace Drupal\ebms_core\Routing;

use Drupal\Core\Routing\RouteSubscriberBase;
use Symfony\Component\Routing\RouteCollection;

/**
 * Listens to the dynamic route events.
 */
class RouteSubscriber extends RouteSubscriberBase {

  /**
   * {@inheritdoc}
   */
  protected function alterRoutes(RouteCollection $collection) {
    if ($route = $collection->get('system.site_information_settings'))
      $route->setDefault('_form', 'Drupal\ebms_core\Form\ExtendedSiteInformationForm');
  }

}
