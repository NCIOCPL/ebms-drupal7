<?php

namespace Drupal\ebms_core\EventSubscriber;

use Symfony\Component\EventDispatcher\EventSubscriberInterface;
use Symfony\Component\HttpFoundation\RedirectResponse;
use Symfony\Component\HttpKernel\Event\RequestEvent;
use Symfony\Component\HttpKernel\KernelEvents;

/**
 * Event subscriber subscribing to KernelEvents::REQUEST.
 */
class RedirectAnonymousUser implements EventSubscriberInterface {

  /**
   * {@inheritdoc}
   */
  public static function getSubscribedEvents() {
    return [KernelEvents::REQUEST => 'checkLoginStatus'];
  }

  /**
   * Send the user to the login page if appropriate.
   */
  public function checkLoginStatus(RequestEvent $event) {
    $user = \Drupal::currentUser();
    $current_path = $event->getRequest()->getPathInfo();
    $skip = ['/user/login', '/login', '/ssologin'];
    if ($user->isAnonymous() && !in_array($current_path, $skip)) {
      ebms_debug_log('redirecting to /login');
      $response = new RedirectResponse('/login');
      $response->send();
    }
  }

}
