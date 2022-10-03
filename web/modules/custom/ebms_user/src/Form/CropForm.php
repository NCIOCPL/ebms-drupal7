<?php

namespace Drupal\ebms_user\Form;

use Drupal\Core\Form\FormBase;
use Drupal\Core\Form\FormStateInterface;
use Drupal\file\Entity\File;
use Drupal\user\Entity\User;

/**
 * Get the user's crop for the profile picture and save it.
 *
 * @ingroup ebms
 */
class CropForm extends FormBase {

  /**
   * Width and height of a profile picture.
   */
  const PICTURE_SIZE = 135;

  /**
   * {@inheritdoc}
   */
  public function getFormId(): string {
    return 'user_profile_picture_crop_form';
  }

  /**
   * {@inheritdoc}
   */
  public function buildForm(array $form, FormStateInterface $form_state, User $user = NULL, File $file = NULL): array {

    $uri = $file->getFileUri();
    $image = \Drupal::service('image.factory')->get($uri);
    $form = [
      '#title' => 'Crop Image',
      '#id' => 'picture-crop-form',
      '#attached' => ['library' => ['ebms_user/picture-crop']],
      'wrapper' => [
        '#type' => 'container',
        'picture' => [
          '#theme' => 'image',
          '#alt' => 'Profile picture to be cropped',
          '#title' => 'Profile Picture',
          '#width' => '500px',
          '#uri' => $uri,
          '#id' => 'profile-image',
        ],
      ],
      'x' => ['#type' => 'hidden'],
      'y' => ['#type' => 'hidden'],
      'w' => ['#type' => 'hidden'],
      'h' => ['#type' => 'hidden'],
      'width' => [
        '#type' => 'hidden',
        '#value' => $image->getWidth(),
      ],
      'height' => [
        '#type' => 'hidden',
        '#value' => $image->getHeight(),
      ],
      'fid' => [
        '#type' => 'hidden',
        '#value' => $file->id(),
      ],
      'uid' => [
        '#type' => 'hidden',
        '#value' => $user->id(),
      ],
      'submit' => [
        '#type' => 'submit',
        '#value' => 'Apply Crop',
      ],
      'cancel' => [
        '#type' => 'submit',
        '#value' => 'Cancel',
        '#submit' => ['::cancelSubmit'],
        '#limit_validation_errors' => [],
        '#suffix' => '<span id="min-width-message"></span>'
      ],
    ];
    return $form;
  }

  /**
   * Go back to the profile page.
   */
  public function cancelSubmit(array &$form, FormStateInterface $form_state) {
    $uid = $this->getRouteMatch()->getRawParameter('user');
    $form_state->setRedirect('ebms_user.profile', ['user' => $uid]);
  }

  /**
   * {@inheritdoc}
   */
  public function submitForm(array &$form, FormStateInterface $form_state) {
    $x = $form_state->getValue('x');
    $y = $form_state->getValue('y');
    $w = round($form_state->getValue('w'));
    $h = round($form_state->getValue('h'));
    $original_width = $form_state->getValue('width');
    $original_height = $form_state->getValue('height');
    $uid = $form_state->getValue('uid');
    ebms_debug_log("CropForm submit: w=$w (original $original_width) h=$h (original $original_height) x=$x y=$y");
    if ($w == $h && $w >= self::PICTURE_SIZE) {
      $user = User::load($uid);
      $fid = $form_state->getValue('fid');
      $file = File::load($fid);
      $file->setPermanent();
      $file->save();
      $image = \Drupal::service('image.factory')->get($file->getFileUri());
      ebms_debug_log('image width: ' . $image->getWidth() . ' height: ' . $image->getHeight());
      $modified = FALSE;
      if ($x || $y || $w < $original_width || $h < $original_height) {
        $image->crop($x, $y, $w, $h);
        $modified = TRUE;
      }
      if ($w > self::PICTURE_SIZE) {
        $image->scale(self::PICTURE_SIZE, self::PICTURE_SIZE);
        $modified = TRUE;
      }
      if ($modified) {
        $image->save();
      }
      $user->user_picture = $fid;
      $user->save();
      $file_usage = \Drupal::service('file.usage');
      $file_usage->add($file, 'user', 'user', $uid);
      $this->messenger()->addMessage('Profile picture successfully registered.');
    }
    $form_state->setRedirect('ebms_user.profile', ['user' => $uid]);
  }

}
