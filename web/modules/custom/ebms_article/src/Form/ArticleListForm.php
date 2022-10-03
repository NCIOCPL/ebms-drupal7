<?php

namespace Drupal\ebms_article\Form;

use Drupal\Core\Entity\EntityTypeManagerInterface;
use Drupal\Core\Form\FormBase;
use Drupal\Core\Form\FormStateInterface;
use Drupal\Core\Session\AccountProxyInterface;
use Symfony\Component\DependencyInjection\ContainerInterface;

/**
 * Form controller for EBMS article edit forms.
 *
 * @ingroup ebms
 */
class ArticleListForm extends FormBase {


  /**
   * The current user account.
   *
   * @var \Drupal\Core\Session\AccountProxyInterface
   */
  protected AccountProxyInterface $account;

  /**
   * The entity type manage.
   *
   * @var \Drupal\Core\Entity\EntityTypeManagerInterface
   */
  protected EntityTypeManagerInterface $entityTypeManager;

  /**
   * {@inheritdoc}
   */
  public static function create(ContainerInterface $container): ArticleListForm {
    // Instantiates this form class.
    $instance = parent::create($container);
    $instance->account = $container->get('current_user');
    $instance->entityTypeManager = $container->get('entity_type.manager');
    return $instance;
  }

  /**
   * {@inheritdoc}
   */
  public function getFormId(): string {
    return 'ebms_article_list_filter';
  }

  /**
   * {@inheritdoc}
   */
  public function buildForm(array $form, FormStateInterface $form_state): array {
    $storage = $this->entityTypeManager->getStorage('ebms_board');
    $query = $storage->getQuery()->accessCheck(FALSE);
    $query->sort('name');
    $entities = $storage->loadMultiple($query->execute());
    $boards = [''];
    foreach ($entities as $entity) {
      $boards[$entity->id()] = $entity->getName();
    }
    $storage = $this->entityTypeManager->getStorage('taxonomy_term');
    $query = $storage->getQuery()->accessCheck(FALSE);
    $query->condition('vid', 'states');
    $query->sort('field_sequence');
    $query->sort('name');
    $entities = $storage->loadMultiple($query->execute());
    $states = [''];
    foreach ($entities as $entity) {
      $states[$entity->id()] = $entity->getName();
    }
    $parms = $this->getRequest()->query;
    $board = $parms->get('board') ?? '';
    $state = $parms->get('state') ?? '';
    return [
      'filters' => [
        '#type' => 'fieldset',
        '#title' => 'Filters',
        'board' => [
          '#type' => 'select',
          '#title' => 'Board',
          '#description' => 'Only show articles considered for review by this board',
          '#options' => $boards,
          '#default_value' => $board,
        ],
        'state' => [
          '#type' => 'select',
          '#title' => 'State',
          '#description' => 'Show articles with a topic in this state',
          '#options' => $states,
          '#default_value' => $state,
        ],
        'submit' => [
          '#type' => 'submit',
          '#value' => 'Apply',
        ],
        'reset' => [
          '#type' => 'submit',
          '#value' => 'Reset',
        ],
      ],
    ];
  }

  /**
   * {@inheritdoc}
   */
  public function submitForm(array &$form, FormStateInterface $form_state) {
    $route = 'entity.ebms_article.collection';
    $parameters = [];
    $trigger = $form_state->getTriggeringElement()['#value'];
    if ($trigger === 'Reset') {
      $form_state->setRedirect($route);
    }
    else {
      $query = [];
      $value = $form_state->getValue('state');
      if (!empty($value)) {
        $query['state'] = $value;
      }
      $value = $form_state->getValue('board');
      if (!empty($value)) {
        $query['board'] = $value;
      }
      $options = ['query' => $query];
      $form_state->setRedirect($route, $parameters, $options);
    }
  }

}
