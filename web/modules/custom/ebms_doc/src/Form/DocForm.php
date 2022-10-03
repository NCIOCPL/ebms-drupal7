<?php

namespace Drupal\ebms_doc\Form;

use Drupal\Core\Form\FormBase;
use Drupal\Core\Form\FormStateInterface;
use Drupal\Core\Url;
use Drupal\ebms_board\Entity\Board;
use Drupal\ebms_doc\Entity\Doc;

/**
 * Form for requesting an import job.
 *
 * @ingroup ebms
 */
class DocForm extends FormBase {

  /**
   * {@inheritdoc}
   */
  public function getFormId(): string {
    return 'ebms_doc_form';
  }

  /**
   * {@inheritdoc}
   */
  public function buildForm(array $form, FormStateInterface $form_state, ?Doc $doc = NULL): array {

    // Get the picklists for the tags and the boards.
    $boards = Board::boards();
    $summary_tag_id = 0;
    $tags = [];
    $storage = \Drupal::entityTypeManager()->getStorage('taxonomy_term');
    $ids = $storage->getQuery()->accessCheck(FALSE)
      ->condition('vid', 'doc_tags')
      ->condition('status', 1)
      ->sort('name')
      ->execute();
    foreach ($storage->loadMultiple($ids) as $term) {
      $tags[$term->id()] = $term->name->value;
      if ($term->field_text_id->value === 'summary') {
        $summary_tag_id = $term->id();
      }
    }

    // Find out what the user has changed on the form.
    $values = $form_state->getValues();
    if (!empty($values)) {
      $selected_boards = [];
      foreach ($values['boards'] as $key => $value) {
        if (!empty($value)) {
          $selected_boards[] = $key;
        }
      }
      $selected_tags = [];
      foreach ($values['tags'] as $key => $value) {
        if (!empty($value)) {
          $selected_tags[] = $key;
        }
      }
      $selected_topics = [];
      foreach ($values['topics'] as $key => $value) {
        if (!empty($value)) {
          $selected_topics[] = $key;
        }
      }
    }

    // If we're creating a new document, start with a clean slate.
    if (empty($doc)) {
      $doc_id = '';
      $title = 'Post New Document';
      if (empty($values)) {
        $description = '';
        $selected_boards = $selected_tags = $selected_topics = [];
      }
    }

    // Otherwise, get the values from the existing entity.
    else {
      $doc_id = $doc->id();
      $title = 'Modify ' . $doc->file->entity->filename->value;
      if (empty($values)) {
        $description = $doc->description->value;
        $selected_boards = [];
        foreach ($doc->boards as $board) {
          $selected_boards[] = $board->target_id;
        }
        $selected_tags = [];
        foreach ($doc->tags as $tag) {
          $selected_tags[] = $tag->target_id;
        }
        $selected_topics = [];
        foreach ($doc->topics as $topic) {
          $selected_topics[] = $topic->target_id;
        }
      }
    }

    // Get the form started.
    $form = [
      '#title' => $title,
      'doc-id' => [
        '#type' => 'hidden',
        '#value' => $doc_id,
      ],
    ];

    // The user only uploads the file for new Doc entities.
    if (empty($doc)) {
      $form['file'] = [
        '#title' => 'File',
        '#type' => 'file',
        '#attributes' => [
          'class' => ['usa-file-input', 'required'],
          'accept' => ['.pdf,.rtf,.doc,.docx'],
        ],
        '#description' => 'Find the file for the document on your computer.',
        '#required' => TRUE,
        // USWDS file widgets and Drupal file widgets aren't 100% compatable,
        // so we have to do our own validation.
        '#validated' => TRUE,
      ];
    }

    // Add the fields which are always present.
    $form['description'] = [
      '#type' => 'textfield',
      '#title' => 'Description',
      '#description' => 'This will be used as the title of the document in the EBMS.',
      '#default_value' => $description,
      '#required' => TRUE,
    ];
    $form['boards'] = [
      '#type' => 'checkboxes',
      '#title' => 'Boards',
      '#description' => 'You may select more than one board.',
      '#options' => $boards,
      '#default_value' => $selected_boards,
      '#ajax' => [
        'callback' => '::boardChangeCallback',
        'wrapper' => 'board-controlled',
        'event' => 'change',
      ],
    ];
    $form['board-controlled'] = [
      '#type' => 'container',
      '#attributes' => ['id' => 'board-controlled'],
      'tags' => [
        '#type' => 'checkboxes',
        '#title' => 'Tags',
        '#description' => 'You may select more than one tag.',
        '#options' => $tags,
        '#default_value' => $selected_tags,
        '#ajax' => [
          'callback' => '::tagChangeCallback',
          'wrapper' => 'tag-controlled',
          'event' => 'change',
        ],
      ],
      'tag-controlled' => [
        '#type' => 'container',
        '#attributes' => ['id' => 'tag-controlled'],
      ],
    ];

    // If we have any boards and the 'summary' tag is applied, add topics.
    $summary_tag_checked = in_array($summary_tag_id, $selected_tags);
    if ($summary_tag_checked && !empty($selected_boards)) {
      $storage = \Drupal::entityTypeManager()->getStorage('ebms_topic');
      $ids = $storage->getQuery()->accessCheck(FALSE)
        ->condition('board', $selected_boards, 'IN')
        ->condition('active', 1)
        ->sort('name')
        ->execute();
      $topics = [];
      foreach ($storage->loadMultiple($ids) as $topic) {
        $topics[$topic->id()] = $topic->name->value;
      }
      $form['board-controlled']['tag-controlled']['topics'] = [
        '#type' => 'checkboxes',
        '#title' => 'Topics',
        '#description' => 'You may select more than one topic.',
        '#options' => $topics,
        '#default_value' => array_intersect($selected_topics, array_keys($topics)),
      ];
    }
    $form['submit'] = [
      '#type' => 'submit',
      '#value' => 'Submit',
    ];
    $form['cancel'] = [
      '#type' => 'submit',
      '#value' => 'Cancel',
      '#submit' => ['::cancelSubmit'],
      '#limit_validation_errors' => [],
    ];
    return $form;
  }

  /**
   * {@inheritdoc}
   */
  public function validateForm(array &$form, FormStateInterface $form_state) {
    if (empty($form_state->getValue('doc-id'))) {
      $files = $this->getRequest()->files->get('files', []);
      if (empty($files['file'])) {
        $form_state->setErrorByName('file', 'No file selected.');
      }
    }
  }

  /**
   * Skip validation and redirect as appropriate.
   *
   * @param array $form
   *   Form settings.
   * @param \Drupal\Core\Form\FormStateInterface $form_state
   *   Form values.
   */
  public function cancelSubmit(array &$form, FormStateInterface $form_state) {
    $this->doRedirect($form_state);
  }

  /**
   * {@inheritdoc}
   */
  public function submitForm(array &$form, FormStateInterface $form_state) {

    // Collect the referenced entity IDs.
    $board_ids = [];
    foreach ($form_state->getValue('boards') as $key => $value) {
      if (!empty($value)) {
        $board_ids[] = $key;
      }
    }
    $tag_ids = [];
    foreach ($form_state->getValue('tags') as $key => $value) {
      if (!empty($value)) {
        $tag_ids[] = $key;
      }
    }
    $topic_ids = [];
    foreach ($form_state->getValue('topics') as $key => $value) {
      if (!empty($value)) {
        $topic_ids[] = $key;
      }
    }
    $doc_id = $form_state->getValue('doc-id');
    if (empty($doc_id)) {
      $validators = ['file_validate_extensions' => ['pdf rtf doc docx']];
      $file = file_save_upload('file', $validators, 'public://', 0);
      $file->setPermanent();
      $file->save();
      $doc = Doc::create([
        'description' => $form_state->getValue('description'),
        'file' => $file->id(),
        'posted' => date('Y-m-d H:i:s'),
        'boards' => $board_ids,
        'tags' => $tag_ids,
        'topics' => $topic_ids,
      ]);
      $doc->save();
      $file_usage = \Drupal::service('file.usage');
      $file_usage->add($file, 'ebms_doc', 'ebms_doc', $this->currentUser()->id());
    }
    else {
      $doc = Doc::load($doc_id);
      $doc->set('description', $form_state->getValue('description'));
      $doc->set('boards', $board_ids);
      $doc->set('tags', $tag_ids);
      $doc->set('topics', $topic_ids);
      $doc->save();
    }
    $this->doRedirect($form_state);
  }

  /**
   * Plug the board's topics into the form.
   *
   * @param array $form
   *   Form settings.
   * @param \Drupal\Core\Form\FormStateInterface $form_state
   *   Form values.
   *
   * @return array
   *   The portion of the form's render array controlled by this callback.
   */
  public function boardChangeCallback(array &$form, FormStateInterface $form_state): array {
    return $form['board-controlled'];
  }

  /**
   * Topics are only available if the entity has the 'summary' tag and boards.
   *
   * @param array $form
   *   Form settings.
   * @param \Drupal\Core\Form\FormStateInterface $form_state
   *   Form values.
   *
   * @return array
   *   The portion of the form's render array controlled by this callback.
   */
  public function tagChangeCallback(array &$form, FormStateInterface $form_state): array {
    return $form['board-controlled']['tag-controlled'];
  }

  /**
   * Take the user to the appropriate next page.
   *
   * @param \Drupal\Core\Form\FormStateInterface $form_state
   *   Form values.
   */
  private function doRedirect(FormStateInterface $form_state) {
    $request = $this->getRequest();
    $options = ['query' => $request->query->all()];
    if (!empty($options['query']['caller'])) {
      $base = $request->getSchemeAndHttpHost();
      $url = $options['query']['caller'];
      $form_state->setRedirectUrl(Url::fromUri("$base/$url"));
    }
    else {
      $form_state->setRedirect('ebms_doc.list', [], $options);
    }
  }
}
