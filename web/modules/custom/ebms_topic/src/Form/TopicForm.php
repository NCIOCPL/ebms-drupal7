<?php

namespace Drupal\ebms_topic\Form;

use Drupal\Core\Entity\ContentEntityForm;
use Drupal\Core\Form\FormStateInterface;
use Drupal\Core\Session\AccountProxyInterface;
use Symfony\Component\DependencyInjection\ContainerInterface;

/**
 * Form controller for PDQ Topic edit forms.
 *
 * @ingroup ebms
 */
class TopicForm extends ContentEntityForm {

  /**
   * The current user account.
   *
   * @var \Drupal\Core\Session\AccountProxyInterface
   */
  protected AccountProxyInterface $account;

  /**
   * {@inheritdoc}
   */
  public static function create(ContainerInterface $container): TopicForm {
    // Instantiates this form class.
    $instance = parent::create($container);
    $instance->account = $container->get('current_user');
    return $instance;
  }

  /**
   * {@inheritdoc}
   */
  public function buildForm(array $form, FormStateInterface $form_state): array {
    $form = parent::buildForm($form, $form_state);
    if ($this->entity->isNew()) {
      $form['#title'] = $this->t('Add PDQ topic');
    }
    else {
      $form['#title'] = $this->t('Edit PDQ topic');
    }
    return $form;
  }

  /**
   * {@inheritdoc}
   */
  public function save(array $form, FormStateInterface $form_state) {
    $entity = $this->entity;

    $status = parent::save($form, $form_state);

    switch ($status) {
      case SAVED_NEW:
        $this->messenger()->addMessage($this->t('Created the %label PDQ topic.', [
          '%label' => $entity->label(),
        ]));
        break;

      default:
        $this->messenger()->addMessage($this->t('Saved the %label PDQ topic.', [
          '%label' => $entity->label(),
        ]));
    }
    $form_state->setRedirect('entity.ebms_topic.canonical', ['ebms_topic' => $entity->id()]);
  }

}
