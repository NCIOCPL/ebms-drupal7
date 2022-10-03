<?php

namespace Drupal\ebms_summary\Form;

use Drupal\Core\Entity\EntityTypeManagerInterface;
use Drupal\Core\Form\FormBase;
use Drupal\Core\Form\FormStateInterface;
use Symfony\Component\DependencyInjection\ContainerInterface;
use Drupal\ebms_summary\Entity\BoardSummaries;
use Drupal\ebms_board\Entity\Board;

/**
 * Form for adding a supporting document to a board summaries page.
 *
 * @ingroup ebms
 */
class SummaryBoardDocForm extends FormBase {

  /**
   * The entity type manager.
   *
   * @var \Drupal\Core\Entity\EntityTypeManagerInterface
   */
  protected EntityTypeManagerInterface $entityTypeManager;

  /**
   * {@inheritdoc}
   */
  public static function create(ContainerInterface $container): SummaryBoardDocForm {
    $instance = parent::create($container);
    $instance->entityTypeManager = $container->get('entity_type.manager');
    return $instance;
  }

  /**
   * {@inheritdoc}
   */
  public function getFormId(): string {
    return 'ebms_summary_board_doc_form';
  }

  /**
  * {@inheritdoc}
  */
  public function buildForm(array $form, FormStateInterface $form_state, $board_id = 0): array {
    $board = Board::load($board_id);
    $board_name = $board->name->value;
    $storage = $this->entityTypeManager->getStorage('ebms_board_summaries');
    $query = $storage->getQuery()->accessCheck(FALSE);
    $query->condition('board', $board_id);
    $ids = $query->execute();
    $board_summaries = $storage->load(reset($ids));
    $eligible_docs = $board_summaries->eligibleDocs();
    return [
      '#title' => "Post Document for $board_name Summaries Page",
      'board-id' => [
        '#type' => 'hidden',
        '#value' => $board_id,
      ],
      'board-summaries-id' => [
        '#type' => 'hidden',
        '#value' => $board_summaries->id(),
      ],
      'doc' => [
        '#type' => 'radios',
        '#title' => 'Document',
        '#options' => $eligible_docs,
        '#description' => "Select supporting document to be linked from the board's summaries page.",
        '#required' => TRUE,
        '#validated' => TRUE,
      ],
      'notes' => [
        '#type' => 'textarea',
        '#title' => 'Notes',
        '#description' => "Add optional notes to be displayed with the document's link.",
      ],
      'submit' => [
        '#type' => 'submit',
        '#value' => 'Save',
      ],
      'cancel' => [
        '#type' => 'submit',
        '#value' => 'Cancel',
        '#limit_validation_errors' => [],
        '#submit' => ['::cancelSubmit'],
      ],
    ];
  }

  /**
   * {@inheritdoc}
   */
  public function validateForm(array &$form, FormStateInterface $form_state) {
    $trigger = $form_state->getTriggeringElement()['#value'];
    if ($trigger === 'Cancel') {
      $query_parameters = $this->getRequest()->query->all();
      $board_id = $query_parameters['board'];
      unset($query_parameters['board']);
      $options = ['query' => $query_parameters];
      $parameters = ['board' => $board_id];
      $form_state->setRedirect('ebms_summary.board', $parameters, $options);
    }
    else {
      $doc = $form_state->getValue('doc');
      if (empty($doc)) {
        $form_state->setErrorByName('doc', 'You must select a document.');
      }
    }
  }

  /**
   * {@inheritdoc}
   */
  public function submitForm(array &$form, FormStateInterface $form_state) {

    $board_id = $form_state->getValue('board-id');
    $board_summaries_id = $form_state->getValue('board-summaries-id');
    $board_summaries = BoardSummaries::load($board_summaries_id);
    $values = [
      'doc' => $form_state->getValue('doc'),
      'notes' => $form_state->getValue('notes'),
      'active' => 1,
    ];
    $board_summaries->docs[] = $values;
    $board_summaries->save();
    $route = 'ebms_summary.board';
    $parameters = ['board_id' => $board_id];
    $this->messenger()->addMessage('Added supporting document.');
    $query_parameters = $this->getRequest()->query->all();
    unset($query_parameters['board']);
    $options = ['query' => $query_parameters];
    $form_state->setRedirect($route, $parameters, $options);
  }

  /**
   * Return to the summary page.
   *
   * @param array $form
   *   Ignored.
   * @param FormStateInterface $form_state
   *   Used for the redirection.
   */
  public function cancelSubmit(array &$form, FormStateInterface $form_state) {
    $route_match = $this->getRouteMatch();
    $parms = ['board_id' => $route_match->getRawParameter('board_id')];
    $options = ['query' => $this->getRequest()->query->all()];
    $form_state->setRedirect('ebms_summary.board', $parms, $options);
  }

}
