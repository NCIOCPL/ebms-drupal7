<?php

namespace Drupal\ebms_user\Form;

use Drupal\Core\Form\FormBase;
use Drupal\Core\Access\AccessResult;
use Drupal\Core\Form\FormStateInterface;
use Drupal\Core\Session\AccountInterface;
use Drupal\ebms_core\Controller\SingleSignOn;
use Drupal\user\Entity\User;

/**
 * Switch a native Drupal account to NIH SSO.
 *
 * @ingroup ebms
 */
class Authname extends FormBase {

  /**
   * Explain what to enter for the authname.
   *
   * We use this elsewhere, so nice to maintain it in one place.
   */
  const DESCRIPTION = "The user's NIH SSO Username. You can find this information by going to https://ned.nih.gov and searching for the user and viewing the account's details. On the details page this is the <strong>NIH Login Username</strong> field. Try to make the Drupal username match this value if possible.<br><br>Alternatively, enter the user's OpenID identifier. This identifier must be provided by the user. Google identifiers are Google account email addresses (which may or may not be gmail addresses). If the identifier is an email address, prefix it with <strong>mail:</strong>, <em>e.g.</em>, <strong>mail:jjsmith@gmail.com</strong>.";

  /**
   * {@inheritdoc}
   */
  public function getFormId(): string {
    return 'ebms_user_authname_form';
  }

  /**
   * {@inheritdoc}
   */
  public function buildForm(array $form, FormStateInterface $form_state, User $user = NULL): array {

    return [
      '#title' => 'Convert User to NIH SSO Authentication',
      'uid' => [
        '#type' => 'hidden',
        '#value' => $user->id(),
      ],
      'authname' => [
        '#type' => 'textfield',
        '#title' => 'NIH SSO Username',
        '#maxlength' => 128,
        '#description' => self::DESCRIPTION,
        '#required' => TRUE,
      ],
      'submit' => [
        '#type' => 'submit',
        '#value' => 'Submit',
      ],
      'cancel' => [
        '#type' => 'submit',
        '#value' => 'Cancel',
        '#submit' => ['::cancelSubmit'],
        '#limit_validation_errors' => [],
      ],
    ];
  }

  /**
   * Retreat to the user editing page page.
   */
  public function cancelSubmit(array &$form, FormStateInterface $form_state) {
    $uid = $this->getRouteMatch()->getRawParameter('user');
    $form_state->setRedirect('entity.user.edit_form', ['user' => $uid]);
  }

  /**
   * {@inheritdoc}
   */
  public function submitForm(array &$form, FormStateInterface $form_state) {
    $uid = $form_state->getValue('uid');
    $authname = $form_state->getValue('authname');
    $user = User::load($uid);
    $user->set('pass', NULL);
    $user->save();
    $authmap = \Drupal::service('externalauth.authmap');
    $authmap->save($user, SingleSignOn::PROVIDER, $authname);
    ebms_debug_log("Saved SSO authname $authname");
    $form_state->setRedirect('entity.user.edit_form', ['user' => $uid]);
  }

  /**
   * Custom access method for this form.
   *
   * @param AccountInterface $account
   *   The account we check to see if it is already an SSO account.
   *
   * @return AccessResult
   *   The outcome of our investigation.
   */
  public function access(AccountInterface $account): AccessResult {
    if (!$account->hasPermission('administer users')) {
      $answer = AccessResult::forbidden();
    }
    elseif (!\Drupal::moduleHandler()->moduleExists('externalauth')) {
      $answer = AccessResult::forbidden();
    }
    else {
      $uid = $this->getRouteMatch()->getRawParameter('user');
      ebms_debug_log('checking to see if user ' . $uid . ' has an authname');
      $authname = \Drupal::database()->select('authmap', 'a')
        ->condition('a.uid', $uid)
        ->condition('a.provider', 'ebms_core')
        ->fields('a', ['authname'])
        ->execute()
        ->fetchField();
      ebms_debug_log("authname is $authname");
      $answer = empty($authname) ? AccessResult::allowed() : AccessResult::forbidden();
    }
    $answer->setCacheMaxAge(0);
    return $answer;
  }

}
