<?php

namespace Drupal\ebms_summary\Form;

use Drupal\Core\Cache\Cache;
use Drupal\Core\Entity\EntityTypeManagerInterface;
use Drupal\Core\Form\FormBase;
use Drupal\Core\Form\FormStateInterface;
use Symfony\Component\DependencyInjection\ContainerInterface;
use Drupal\ebms_summary\Entity\SummaryPage;

/**
 * Form for adding/editing a summaries page.
 *
 * @ingroup ebms
 */
class SummaryPageForm extends FormBase {

  /**
   * The entity type manager.
   *
   * @var \Drupal\Core\Entity\EntityTypeManagerInterface
   */
  protected EntityTypeManagerInterface $entityTypeManager;

  /**
   * {@inheritdoc}
   */
  public static function create(ContainerInterface $container): SummaryPageForm {
    $instance = parent::create($container);
    $instance->entityTypeManager = $container->get('entity_type.manager');
    return $instance;
  }

  /**
   * {@inheritdoc}
   */
  public function getFormId(): string {
    return 'ebms_summary_page_form';
  }

  /**
  * {@inheritdoc}
  */
  public function buildForm(array $form, FormStateInterface $form_state, $summary_page = NULL): array {
    $board_id = $this->getRequest()->query->get('board');
    $topics = [];
    $storage = $this->entityTypeManager->getStorage('ebms_topic');
    $query = $storage->getQuery()->accessCheck(FALSE);
    $query->condition('board', $board_id);
    $query->sort('name');
    $ids = $query->execute();
    $entities = $storage->loadMultiple($ids);
    foreach ($entities as $topic) {
      $topics[$topic->id()] = $topic->getName();
    }
    $default_topics = [];
    if (empty($summary_page)) {
      $title = 'Add Summary Page';
      $name = '';
      $page_id = '';
    }
    else {
      $title = 'Edit Summary Page';
      $name = $summary_page->name->value;
      $page_id = $summary_page->id();
      foreach ($summary_page->topics as $topic) {
        if (array_key_exists($topic->target_id, $topics)) {
          $default_topics[] = $topic->target_id;
        }
      }
    }
    return [
      '#title' => $title,
      'page-id' => [
        '#type' => 'hidden',
        '#value' => $page_id,
      ],
      'board-id' => [
        '#type' => 'hidden',
        '#value' => $board_id,
      ],
      'name' => [
        '#type' => 'textfield',
        '#title' => 'Page Name',
        '#description' => 'The display name for this summaries page.',
        '#required' => TRUE,
        '#default_value' => $name,
      ],
      'topics' => [
        '#type' => 'checkboxes',
        '#title' => 'Topics',
        '#options' => $topics,
        '#default_value' => $default_topics,
        '#required' => TRUE,
        '#description' => 'Topics associated with this page.',
      ],
      'submit' => [
        '#type' => 'submit',
        '#value' => 'Save',
      ],
      'cancel' => [
        '#type' => 'submit',
        '#value' => 'Cancel',
        '#limit_validation_errors' => [],
        '#submit' => [[$this, 'cancelForm']],
      ],
    ];
  }

  /**
   * {@inheritdoc}
   */
  public function validateForm(array &$form, FormStateInterface $form_state) {
    $trigger = $form_state->getTriggeringElement()['#value'];
    if ($trigger === 'Save') {
      $topics = [];
      foreach ($form_state->getValue('topics') as $key => $value) {
        if (!empty($value)) {
          $topics[] = $key;
        }
      }
      if (empty($topics)) {
        $form_state->setErrorByName('topics', 'At least one topic must be selected.');
      }
      $form_state->setValue('topic-ids', $topics);
    }
  }

  /**
   * Redirect back to the summary page..
   *
   * @param array $form
   *   Ignored.
   * @param FormStateInterface $form_state
   *   Used for the redirection.
   */
  public function cancelForm(array &$form, FormStateInterface $form_state) {
    $query_parameters = $this->getRequest()->query->all();
    $board_id = $query_parameters['board'];
    unset($query_parameters['board']);
    $options = ['query' => $query_parameters];
    $parameters = ['board_id' => $board_id];
    $form_state->setRedirect('ebms_summary.board', $parameters, $options);
  }

  /**
   * {@inheritdoc}
   */
  public function submitForm(array &$form, FormStateInterface $form_state) {

    $board_id = $form_state->getValue('board-id');
    $page_id = $form_state->getValue('page-id');
    $name = $form_state->getValue('name');
    $topics = $form_state->getValue('topic-ids');
    if (empty($page_id)) {
      $page = SummaryPage::create([
        'name' => $name,
        'topics' => $topics,
        'active' => 1,
      ]);
    }
    else {
      $page = SummaryPage::load($page_id);
      $page->name = $name;
      $page->topics = $topics;
    }
    $page->save();
    if (empty($page_id)) {
      $storage = $this->entityTypeManager->getStorage('ebms_board_summaries');
      $query = $storage->getQuery()->accessCheck(FALSE);
      $query->condition('board', $board_id);
      $ids = $query->execute();
      $board_summaries = $storage->load(reset($ids));
      $board_summaries->pages[] = $page->id();
      $board_summaries->save();
    }
    $this->messenger()->addMessage("Saved page '$name'.");
    $route = 'ebms_summary.board';
    $parameters = ['board_id' => $board_id];
    $query_parameters = $this->getRequest()->query->all();
    unset($query_parameters['board']);
    $options = ['query' => $query_parameters];
    $form_state->setRedirect($route, $parameters, $options);
    Cache::invalidateTags(['summary-topic-page-bookmark']);
  }

}
