<?php

namespace Drupal\ebms_user\Form;

use Drupal\Core\Form\FormBase;
use Drupal\Core\Form\FormStateInterface;
use Drupal\file\Entity\File;
use Drupal\user\Entity\User;

/**
 * Choose an image for a user profile picture.
 *
 * In some edge cases, a JPEG image will be stored in the wrong orientation,
 * sometimes with an EXIF property indicating how the rendering application
 * should rotate the image before displaying it. When that happens, the
 * cropping library will report the wrong data. Photos taken from an iPhone
 * seem to be the most common culprits, according to what I've read in GitHub
 * issues reporting the problem. The same problem was experienced by the
 * original EBMS implementation, which used a completely different cropping
 * library. The workaround is to bring the image up in a viewer like Mac's
 * Finder Preview and export it to a new image file (just copying the file
 * won't work). I considered adding some information about the problem to
 * this form, but couldn't come up with wording that wasn't overly technical.
 * As far as I know, none of the EBMS users have ever reported the problem,
 * so it doesn't seem worth worrying too much about.
 *
 * @ingroup ebms
 */
class ProfilePictureForm extends FormBase {

  /**
   * {@inheritdoc}
   */
  public function getFormId(): string {
    return 'ebms_user_profile_picture_form';
  }

  /**
   * {@inheritdoc}
   */
  public function buildForm(array $form, FormStateInterface $form_state, User $user = NULL): array {

    // @todo Ask the users what the "User Agreement" refers to.
    $form = [
      '#title' => $user->name->value,
      'uid' => [
        '#type' => 'hidden',
        '#value' => $user->id(),
      ],
      'hidden' => [
        '#type' => 'container',
        '#attributes' => ['class' => ['hidden']],
        'js-flag' => [
          '#type' => 'hidden',
          '#default_value' => 0,
          '#attributes' => ['id' => 'js-flag']
        ],
      ],
      '#attached' => ['library' => ['ebms_user/profile-picture']],
      'picture' => [
        '#type' => 'managed_file',
        '#title' => 'Profile Picture',
        '#upload_location' => 'public://pictures/',
        '#upload_validators' => [
          'file_validate_extensions' => ['jpg jpeg gif png'],
          'file_validate_size' => [8 * 1024 * 1024],
          'file_validate_is_image' => [],
        ],
        '#description' => 'You can upload a JPG, GIF or PNG file. By clicking "Use Image," you certify that you have the right to distribute this photo and that it does not violate the User Agreement. The image must be a minimum of ' . CropForm::PICTURE_SIZE . ' pixels square.',
      ],
      'submit' => [
        '#type' => 'submit',
        '#value' => 'Use Image',
      ],
      'cancel' => [
        '#type' => 'submit',
        '#value' => 'Cancel',
        '#submit' => ['::cancelSubmit'],
        '#limit_validation_errors' => [],
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
  public function validateForm(array &$form, FormStateInterface $form_state) {
    parent::validateForm($form, $form_state);
    $file_ids = $form_state->getValue('picture');
    if (empty($file_ids)) {
      $form_state->setErrorByName('file', 'No image selected.');
    }
    else {
      $file = File::load($file_ids[0]);
      if (empty($file)) {
        $form_state->setErrorByName('file', 'Failure uploading image.');
      }
      else {
        $image = \Drupal::service('image.factory')->get($file->getFileUri());
        $width = $image->getWidth();
        $height = $image->getHeight();
        if ($width < CropForm::PICTURE_SIZE || $height < CropForm::PICTURE_SIZE) {
          $form_state->setErrorByName('picture', 'The profile picture must be a minimum of ' . CropForm::PICTURE_SIZE . ' pixels square.');
        }
        else {
          $file->setPermanent();
          $file->save();
          $form_state->setValue('image', $image);
          $form_state->setValue('file', $file);
        }
      }
    }
  }

  /**
   * {@inheritdoc}
   */
  public function submitForm(array &$form, FormStateInterface $form_state) {
    $image = $form_state->getValue('image');
    $file = $form_state->getValue('file');
    $uri = $file->uri->value;
    $filemime = $file->filemime->value;
    $filesize = $file->filesize->value;
    $status = $file->status->value;
    ebms_debug_log("profile picture uri=$uri filemime=$filemime filesize=$filesize status=$status");
    $uid = $form_state->getValue('uid');
    $js = $form_state->getValue('js-flag');
    $width = $image->getWidth();
    $height = $image->getHeight();
    $ready = TRUE;
    if ($width != CropForm::PICTURE_SIZE || $height != CropForm::PICTURE_SIZE) {
      if (empty($js)) {
        self::scaleAndCrop($image);
      }
      else {
        $form_state->setRedirect('ebms_user.crop', ['user' => $uid, 'file' => $file->id()]);
        $ready = FALSE;
      }
    }
    if ($ready) {
      $user = User::load($uid);
      $user->user_picture = $file->id();
      $user->save();
      $file_usage = \Drupal::service('file.usage');
      $file_usage->add($file, 'user', 'user', $uid);
      $this->messenger()->addMessage('Profile picture successfully registered.');
      $form_state->setRedirect('ebms_user.profile', ['user' => $uid]);
    }
  }

  /**
   * Scale and crop an image for the user's profile picture.
   *
   * If the user has disabled Javascript we crop and scale the uploaded
   * picture image to a square shape of the required dimensions without any
   * input from the user. Cropping is done by centering the portion of the
   * image retained.
   *
   * @param Image $image
   *   Image we modify in place.
   */
  public static function scaleAndCrop($image) {
    $width = $image->getWidth();
    $height = $image->getHeight();
    if ($width > $height) {
      $dim = $height;
      $extra = $width - $dim;
      $x = $extra / 2;
      $image->crop($x, 0, $dim, $dim);
    }
    elseif ($width < $height) {
      $dim = $width;
      $extra = $height - $dim;
      $y = $extra / 2;
      $image->crop(0, $y, $dim, $dim);
    }
    if ($dim > CropForm::PICTURE_SIZE) {
      $image->scale(CropForm::PICTURE_SIZE);
    }
    $image->save();
  }

}
