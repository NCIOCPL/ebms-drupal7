<?php

namespace Drupal\ebms_core\Controller;

use Drupal\Core\Controller\ControllerBase;
use Symfony\Component\HttpFoundation\RedirectResponse;

/**
 * External login module for NIH SSO.
 */
class SingleSignOn extends ControllerBase {

  /**
   * Identify ourselves to the externalauth services.
   */
  const PROVIDER = 'ebms_core';

  /**
   * Back to the starting gate on failure.
   */
  const LOGIN = '/login';

  /**
   * Log in the user with SSO.
   */
  public function login(): RedirectResponse {

    // Collect the Apache headers.
    $headers = [];
    $raw_values = [];
    $externalauth = \Drupal::service('externalauth.externalauth');
    foreach (apache_request_headers() as $name => $value) {
      $key = strtolower($name);
      if (str_starts_with($key, 'sm_') || $key === 'user_email') {
        $raw_values[$name] = $value;
        $headers[strtolower($key)] = $value;
      }
    }
    ebms_debug_log("Apache headers:\n" . print_r($raw_values, TRUE), 3);

    // Make sure the /ssologin URL is protected by SiteMinder.
    if (!array_key_exists('sm_authtype', $headers) || $headers['sm_authtype'] !== 'Form') {
      $this->messenger()->addWarning('NIH SSO login integration is not configured correctly.');
      \Drupal::logger(self::PROVIDER)->error('Login failed: page not protected by SSO.');
      return new RedirectResponse(self::LOGIN);
    }

    // Find out if the SiteMinder user name matches an EBMS account.
    if (array_key_exists('sm_user', $headers)) {
      $sm_user = $headers['sm_user'];
      $account = $externalauth->load($sm_user, self::PROVIDER);

      // If not, see if the email address for the account SiteMinder approved
      // matches the email placeholder for a federated Google account.
      if (empty($account)) {
        if (array_key_exists('user_email', $headers)) {
          $user_email = $headers['user_email'];
          $mail_key = "mail:$user_email";
          $account = $externalauth->load($mail_key, self::PROVIDER);

          // If so, replace the placeholder with the SiteMinder user name.
          if (!empty($account)) {
            $authmap = \Drupal::service('externalauth.authmap');
            $authmap->save($account, self::PROVIDER, $sm_user);
          }
        }
      }

      // If we still don't have a valid account, send the user back to /login.
      if (empty($account)) {
        ebms_debug_log("$sm_user is not a valid user account.", 1);
        $this->messenger()->addWarning('Not a valid user account.');
        \Drupal::logger(self::PROVIDER)->error("$sm_user is not a valid user account.");
        setcookie('NIHSMSESSION', '', 1, '/', '.nih.gov');
        setcookie('NIHSMPROFILE', '', 1, '/', '.nih.gov');
        return new RedirectResponse(self::LOGIN);
      }

      // Check to see if the account has been disabled.
      elseif (empty($account->status)) {
        ebms_debug_log("$sm_user is a disabled user account.", 1);
        $this->messenger()->addWarning('Not a valid user account.');
        \Drupal::logger(self::PROVIDER)->error("$sm_user is not an active user account.");
        setcookie('NIHSMSESSION', '', 1, '/', '.nih.gov');
        setcookie('NIHSMPROFILE', '', 1, '/', '.nih.gov');
        return new RedirectResponse(self::LOGIN);
      }

      // If we make it to here, we have a usable account, so log it in.
      else {
        $user = $externalauth->login($sm_user, self::PROVIDER);
        $name = $user->name->value;
        ebms_debug_log("SM user $sm_user logged in as $name");
        return $this->redirect('<front>');
      }
    }

    // Very unlikely to have SM_AUTHTYPE without SM_USER, but check anyway.
    else {
      ebms_debug_log('SM_USER variable not found', 1);
      $this->messenger()->addWarning('NIH SSO login integration is not configured correctly.');
      \Drupal::logger(self::PROVIDER)->error('SM_USER variable not found.');
      return new RedirectResponse(self::LOGIN);
    }
  }

}
