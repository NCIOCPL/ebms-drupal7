<?php

namespace Drupal\ebms_user\Controller;

use Drupal\Core\Controller\ControllerBase;
use Drupal\Core\Url;
use Drupal\user\Entity\User;

/**
 * EBMS user account information.
 */
class Profile extends ControllerBase {

  /**
   * Show information for an EBMS user account.
   */
  public function display(User $user = NULL): array {
    $current_user = User::load($this->currentUser()->id());
    if (empty($user)) {
      $user = $current_user;
    }
    $can_edit = $user->id() == $current_user->id() || $current_user->hasPermission('administer users');
    $roles = [];
    $boards = [];
    $groups = [];
    $topics = [];
    $default_board = '';
    foreach ($user->roles as $role) {
      $roles[] = $role->entity->get('label');
    }
    foreach ($user->boards as $board) {
      $boards[] = $board->entity->name->value;
    }
    foreach ($user->groups as $group) {
      $groups[] = $group->entity->name->value;
    }
    foreach ($user->topics as $topic) {
      $topics[] = $topic->entity->name->value;
    }
    if (!empty($user->board->target_id)) {
      $default_board = $user->board->entity->name->value;
    }
    return [
      '#title' => $user->name->value,
      '#attached' => ['library' => ['ebms_user/user-profile']],
      '#cache' => ['max-age' => 0],
      'user' => [
        '#theme' => 'ebms_user_profile',
        '#profile' => [
          'joined' => date('Y-m-d', $user->created->value),
          'login' => $user->login->value ? date('Y-m-d H:i:s', $user->login->value) : '',
          'access' => $user->access->value ? date('Y-m-d H:i:s', $user->access->value) : '',
          'status' =>  $user->status->value ? 'Active' : 'Inactive',
          'roles' => $roles,
          'boards' => $boards,
          'board' => $default_board,
          'groups' => $groups,
          'topics' => $topics,
        ],
      ],
    ];
  }

}
