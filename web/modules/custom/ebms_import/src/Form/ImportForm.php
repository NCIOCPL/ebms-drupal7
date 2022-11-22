<?php

namespace Drupal\ebms_import\Form;

use Drupal\Core\Entity\EntityTypeManagerInterface;
use Drupal\Core\Form\FormBase;
use Drupal\Core\Form\FormStateInterface;
use Drupal\Core\Session\AccountProxyInterface;
use Drupal\ebms_core\TermLookup;
use Drupal\ebms_import\Entity\Batch;
use Drupal\ebms_import\Entity\ImportRequest;
use Drupal\ebms_import\Entity\PubmedSearchResults;
use Symfony\Component\DependencyInjection\ContainerInterface;

/**
 * Form for requesting an import job.
 *
 * @ingroup ebms
 */
class ImportForm extends FormBase {

  /**
   * Pattern used for extracting PubMed IDs from search results.
   */
  const MEDLINE_PMID_PAT = '/^PMID- (\d{2,8})/m';

  /**
   * The current user account.
   *
   * @var \Drupal\Core\Session\AccountProxyInterface
   */
  protected AccountProxyInterface $account;

  /**
   * The entity type manager.
   *
   * @var \Drupal\Core\Entity\EntityTypeManagerInterface
   */
  protected EntityTypeManagerInterface $entityTypeManager;

  /**
   * The term_lookup service.
   *
   * @var \Drupal\ebms_core\TermLookup
   */
  protected TermLookup $termLookup;

  /**
   * {@inheritdoc}
   */
  public function buildForm(array $form, FormStateInterface $form_state, $request_id = NULL): array {

    // The only default we provide is for the PubMed IDs field.
    $request = empty($request_id) ? NULL : ImportRequest::load($request_id);
    $pmids = $this->getRequest()->get('pmid') ?: '';
    if (!empty($request)) {
      $params = json_decode($request->params->value, TRUE);
      if (!empty($params['followup-pmids'])) {
        $pmids = implode(' ', $params['followup-pmids']);
      }
    }

    // Populate the picklists.
    $storage = $this->entityTypeManager->getStorage('ebms_board');
    $query = $storage->getQuery()->accessCheck(FALSE);
    $query->sort('name');
    $entities = $storage->loadMultiple($query->execute());
    $boards = [];
    foreach ($entities as $entity) {
      $boards[$entity->id()] = $entity->name->value;
    }
    $storage = $this->entityTypeManager->getStorage('taxonomy_term');
    $dispositions = [];
    $query = $storage->getQuery()->accessCheck(FALSE);
    $query->condition('vid', 'board_decisions');
    $query->sort('name');
    $entities = $storage->loadMultiple($query->execute());
    foreach ($entities as $entity) {
      $dispositions[$entity->id()] = $entity->name->value;
    }
    $on_hold = $this->termLookup->getState('on_hold');
    $bma_dispositions = [];
    $query = $storage->getQuery()->accessCheck(FALSE);
    $query->condition('vid', 'states');
    $query->condition('field_sequence', $on_hold->field_sequence->value);
    $query->sort('name');
    $entities = $storage->loadMultiple($query->execute());
    foreach ($entities as $entity) {
      $bma_dispositions[$entity->field_text_id->value] = $entity->name->value;
    }
    $placements = [
      'published' => 'Published',
      'passed_bm_review' => 'Passed abstract review',
      'passed_full_review' => 'Passed full text review',
      'bma' => 'Board Manager Action',
      'on_agenda' => 'On agenda',
      'final_board_decision' => 'Editorial Board decision',
    ];

    // Populate the meeting picklist.
    $now = date('c');
    $storage = $this->entityTypeManager->getStorage('ebms_meeting');
    $query = $storage->getQuery()->accessCheck(FALSE);
    $query->condition('published', 1);
    $query->sort('dates.value', 'DESC');
    $ids = $query->execute();
    $meetings = [];
    $entities = $storage->loadMultiple($ids);
    foreach ($entities as $entity) {
      $date = substr($entity->dates->value, 0, 10);
      $name = $entity->name->value;
      $meetings[$entity->id()] = "$name - $date";
    }

    // Populate the cycle picklist.
    $cycles = [];
    $month = new \DateTime('first day of next month');
    $first = new \DateTime('2002-06-01');
    while ($month >= $first) {
      $cycles[$month->format('Y-m-d')] = $month->format('F Y');
      $month->modify('previous month');
    }

    // Assemble the form.
    $form = [
      '#title' => 'Import Articles from PubMed',
      '#attached' => ['library' => ['ebms_import/import-form']],
      'board' => [
        '#type' => 'select',
        '#title' => 'Board',
        '#required' => TRUE,
        '#description' => 'Select a board to populate the Topic picklist.',
        '#options' => $boards,
        '#empty_value' => '',
        '#ajax' => [
          'callback' => '::getTopicsCallback',
          'wrapper' => 'board-controlled',
          'event' => 'change',
        ],
      ],
      'board-controlled' => [
        '#type' => 'container',
        '#attributes' => ['id' => 'board-controlled'],
        'topic' => [
          '#type' => 'select',
          '#title' => 'Topic',
          '#required' => TRUE,
          '#description' => 'The topic assigned to articles imported in this batch.',
          '#options' => [],
          '#empty_value' => '',

          // See http://drupaldummies.blogspot.com/2012/01/solved-illegal-choice-has-been-detected.html.
          '#validated' => TRUE,
        ],
      ],
      'cycle' => [
        '#type' => 'select',
        '#title' => 'Cycle',
        '#options' => $cycles,
        '#required' => TRUE,
        '#description' => 'Review cycle for which these articles are to be imported.',
        '#empty_value' => '',
      ],
      'pmids' => [
        '#type' => 'textfield',
        '#title' => 'PubMed IDs',
        '#description' => 'Enter article IDs here, separated by space, or post PubMed search results below.',
        '#default_value' => $pmids,
      ],
      'import-comments' => [
        '#type' => 'textfield',
        '#title' => 'Import Comment',
        '#description' => 'Notes about the import job.',
      ],
      'mgr-comment' => [
        '#type' => 'textfield',
        '#title' => 'Manager Comments',
        '#description' => 'Information stored with each article for its topic assignment.',
      ],
      'options' => [
        '#type' => 'fieldset',
        '#title' => 'Options',
        'override-not-list' => [
          '#type' => 'checkbox',
          '#title' => 'Override NOT List',
          '#description' => "Don't reject articles even from journals we don't usually accept for the selected board.",
        ],
        'test-mode' => [
          '#type' => 'checkbox',
          '#title' => 'Test Mode',
          '#description' => 'If checked, only show what we would have imported.',
        ],
        'fast-track' => [
          '#type' => 'checkbox',
          '#title' => 'Fast Track',
          '#description' => 'Skip some of the earlier reviews.',
        ],
        'special-search' => [
          '#type' => 'checkbox',
          '#title' => 'Special Search',
          '#description' => 'Mark these articles as the result of a custom search.',
        ],
        'core-journals-search' => [
          '#type' => 'checkbox',
          '#title' => 'Core Journals',
          '#description' => 'Importing articles from a PubMed search of the "core" journals.',
        ],
        'hi-priority' => [
          '#type' => 'checkbox',
          '#title' => 'High Priority',
          '#description' => 'Tag the articles in this import batch as high-priority articles.',
        ],
        'fast-track-fieldset' => [
          '#type' => 'fieldset',
          '#title' => 'Fast Track Options',
          '#states' => [
            'visible' => [':input[name="fast-track"]' => ['checked' => TRUE]],
          ],
          'placement' => [
            '#type' => 'select',
            '#title' => 'Placement Level',
            '#states' => [
              'required' => [':input[name="fast-track"]' => ['checked' => TRUE]],
            ],
            '#description' => 'Assign this state to the imported articles.',
            '#options' => $placements,
            '#empty_value' => '',
          ],
          'disposition' => [
            '#type' => 'select',
            '#title' => 'Disposition',
            '#options' => $dispositions,
            '#empty_value' => '',
            '#states' => [
              'visible' => [':input[name="placement"]' => ['value' => 'final_board_decision']],
              'required' => [
                ':input[name="fast-track"]' => ['checked' => TRUE],
                // Placate PHStorm's silly lint rules.
                0 => 'and',
                ':input[name="placement"]' => ['value' => 'final_board_decision'],
              ],
            ],
          ],
          'bma-disposition' => [
            '#type' => 'select',
            '#title' => 'Disposition',
            '#options' => $bma_dispositions,
            '#empty_value' => '',
            '#states' => [
              'visible' => [':input[name="placement"]' => ['value' => 'bma']],
              'required' => [
                ':input[name="fast-track"]' => ['checked' => TRUE],
                // Appease the ridiculous PHStorm lint deities.
                0 => 'and',
                ':input[name="placement"]' => ['value' => 'bma'],
              ],
            ],
          ],
          'meeting' => [
            '#type' => 'select',
            '#title' => 'Meeting',
            '#options' => $meetings,
            '#empty_value' => '',
            '#description' => 'Select the meeting for the on-agenda placement state.',
            '#states' => [
              'visible' => [
                ':input[name="fast-track"]' => ['checked' => TRUE],
                // Placate PHStorm's silly lint rules.
                0 => 'and',
                ':input[name="placement"]' => ['value' => 'on_agenda'],
              ],
              'required' => [
                ':input[name="fast-track"]' => ['checked' => TRUE],
                // Placate PHStorm's silly lint rules.
                0 => 'and',
                ':input[name="placement"]' => ['value' => 'on_agenda'],
              ],
            ],
          ],
          'fast-track-comments' => [
            '#type' => 'textfield',
            '#title' => 'Fast Track Comments',
            '#description' => 'Enter notes to be attached to the "fast-track" tag.',
          ],
        ],
      ],
      'file' => [
        '#title' => 'PubMed Search Results',
        '#type' => 'file',
        '#attributes' => ['class' => ['usa-file-input']],
        '#description' => 'Articles found in the uploaded PUBMED-formatted search results will be retrieved.',
      ],
      'full-text' => [
        '#title' => 'Full Text',
        '#type' => 'file',
        '#attributes' => [
          'class' => ['usa-file-input'],
          'accept' => ['.pdf'],
        ],
        '#description' => 'Only suitable for single-article import requests.',
      ],
      'submit' => [
        '#type' => 'submit',
        '#value' => 'Submit',
      ],
      'reset' => [
        '#type' => 'submit',
        '#value' => 'Reset',
        '#submit' => ['::resetSubmit'],
        '#limit_validation_errors' => [],
      ],
    ];

    // Append the report, if we have one (and this is not an AJAX request).
    if (!empty($request) && empty($form_state->getValue('board'))) {
      $form['report'] = $request->getReport('Statistics');
    }

    return $form;
  }

  /**
   * {@inheritdoc}
   */
  public static function create(ContainerInterface $container): ImportForm {
    // Instantiates this form class.
    $instance = parent::create($container);
    $instance->account = $container->get('current_user');
    $instance->entityTypeManager = $container->get('entity_type.manager');
    $instance->termLookup = $container->get('ebms_core.term_lookup');
    return $instance;
  }

  /**
   * Parse Pubmed IDs out of a Pubmed search result.
   *
   * @param string $results
   *   Search results in PUBMED format.
   *
   * @return array
   *   Array of Pubmed IDs as ASCII digit strings.
   *
   * @throws \Exception
   *   If bad filename, parms, out of memory, etc.
   */
  public function findPubmedIds(string $results): array {

    // Save what we got (OCEEBMS-313).
    $values = [
      'submitted' => date('Y-m-d H:i:s'),
      'results' => $results,
    ];
    PubmedSearchResults::create($values)->save();

    // Find the IDs.
    $matches = [];
    preg_match_all(self::MEDLINE_PMID_PAT, $results, $matches);
    return $matches[1];
  }

  /**
   * {@inheritdoc}
   */
  public function getFormId(): string {
    return 'ebms_import_form';
  }

  /**
   * Find topics for a given board.
   */
  private function getTopics($board): array {
    $storage = $this->entityTypeManager->getStorage('ebms_topic');
    $query = $storage->getQuery()->accessCheck(FALSE);
    $query->condition('board', $board);
    $query->condition('active', 1);
    $query->sort('name');
    $ids = $query->execute();
    $topics = $storage->loadMultiple($ids);
    $options = [];
    foreach ($topics as $topic) {
      $options[$topic->id()] = $topic->name->value;
    }
    return $options;
  }

  /**
   * Plug the board's topics into the form.
   */
  public function getTopicsCallback(array &$form, FormStateInterface $form_state): array {
    $board = $form_state->getValue('board');
    $options = empty($board) ? [] : $this->getTopics($board);

    // The #empty_option setting is broken in AJAX.
    // See https://www.drupal.org/project/drupal/issues/3180011.
    $form['board-controlled']['topic']['#options'] = ['' => '- Select -'] + $options;
    return $form['board-controlled'];
  }

  /**
   * {@inheritdoc}
   */
  public function submitForm(array &$form, FormStateInterface $form_state) {

    // Prepare navigation.
    $route = 'ebms_import.import_form';
    $parameters = [];

    // Submit the import request, catching failures.
    $request = $form_state->getValues();
    try {
      $batch = Batch::process($request);
    }
    catch (\Exception $e) {
      $error = $e->getMessage();
      $message = "Import failure: $error";
      $logger = \Drupal::logger('ebms_import');
      $logger->error($message);
      $this->messenger()->addError($message);
      $batch = NULL;
    }

    // Keep going if we didn't go up in flames.
    if (!empty($batch)) {

      // Save the statistical report information, even if this is a test run.
      $report = $batch->toArray();
      $report['batch'] = $batch->id();
      $request['followup-pmids'] = $batch->getFollowup();
      $values = [
        'batch' => $batch->id(),
        'params' => json_encode($request),
        'report' => json_encode($report),
      ];
      $import_request = ImportRequest::create($values);
      $import_request->save();
      $parameters = ['request_id' => $import_request->id()];
    }

    // Navigate back to the form.
    $form_state->setRedirect($route, $parameters);
  }

  /**
   * {@inheritdoc}
   */
  public function validateForm(array &$form, FormStateInterface $form_state) {
    $route = 'ebms_import.import_form';
    $trigger = $form_state->getTriggeringElement()['#value'];
    if ($trigger === 'Reset') {
      $form_state->setRedirect($route);
    }
    elseif ($trigger === 'Submit') {
      parent::validateForm($form, $form_state);
      $pmids = trim($form_state->getValue('pmids') ?? '');
      $files = $this->getRequest()->files->get('files', []);
      if (empty($pmids)) {
        if (empty($files['file'])) {
          $message = 'You must enter a list of PubMed IDs or post a PubMed search results file.';
          $form_state->setErrorByName('pmids', $message);
        }
        else {
          $validators = ['file_validate_extensions' => ''];
          $file = file_save_upload('file', $validators, FALSE, 0);
          if (empty($file)) {
            $name = $files['file']->getClientOriginalName();
            $form_state->setErrorByName('file', "Unable to save $name.");
          }
          $search_results = file_get_contents($file->getFileUri());
          $pmids = $this->findPubmedIds($search_results);
          if (empty($pmids)) {
            $form_state->setErrorByName('file', 'No PubMed IDs found.');
          }
        }
      }
      elseif (!empty($files['file'])) {
        $message = 'List of IDs and PubMed search results both submitted.';
        $form_state->setErrorByName('file', $message);
      }
      else {
        $pmids = preg_split('/[\s,]+/', $pmids, -1, PREG_SPLIT_NO_EMPTY);
        foreach ($pmids as $pmid) {
          if (!preg_match('/^\d{1,8}$/', $pmid)) {
            $form_state->setErrorByName('pmids', 'Invalid Pubmed ID format.');
            break;
          }
        }
      }
      $form_state->setValue('article-ids', $pmids);
      $form_state->setValue('full-text-id', NULL);

      if (!empty($files['full-text'])) {
        if (count($pmids) > 1) {
          $message = 'Full-text PDF can only be supplied when importing a single article';
          $form_state->setErrorByName('full-text', $message);
        }
        elseif (empty($form_state->getValue('test-mode'))) {
          $validators = ['file_validate_extensions' => ['pdf']];
          $file = file_save_upload('full-text', $validators, 'public://', 0);
          $file->setPermanent();
          $file->save();
          $form_state->setValue('full-text-id', $file->id());
        }
      }

      if (empty($form_state->getValue('fast-track'))) {
        if (empty($form_state->getValue('special-search'))) {
          $text_id = Batch::IMPORT_TYPE_REGULAR;
        }
        else {
          $text_id = Batch::IMPORT_TYPE_SPECIAL_SEARCH;
        }
      }
      else {
        $text_id = Batch::IMPORT_TYPE_FAST_TRACK;
      }
      $storage = $this->entityTypeManager->getStorage('taxonomy_term');
      $query = $storage->getQuery()->accessCheck(FALSE);
      $query->condition('vid', 'import_types');
      $query->condition('field_text_id', $text_id);
      $ids = $query->execute();
      if (count($ids) !== 1) {
        throw new \Exception("Can't find import type '$text_id'!");
      }
      $import_type = reset($ids);
      $form_state->setValue('import-type', $import_type);
      $form_state->setValue('user', $this->account->id());
    }
  }

  /**
   * Create a version of the form with default values.
   *
   * @param array $form
   *   Form settings.
   * @param \Drupal\Core\Form\FormStateInterface $form_state
   *   Form values.
   */
  public function resetSubmit(array &$form, FormStateInterface $form_state) {
    $form_state->setRedirect('ebms_import.import_form');
  }

}
