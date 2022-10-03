<?php

namespace Drupal\ebms_article\Form;

use Drupal\Core\Entity\EntityTypeManagerInterface;
use Drupal\Core\Form\FormBase;
use Drupal\Core\Form\FormStateInterface;
use Drupal\Core\Session\AccountProxyInterface;
use Drupal\ebms_article\Entity\Article;
use Drupal\file\Entity\File;
use Symfony\Component\DependencyInjection\ContainerInterface;

/**
 * Form controller for attaching the full-text PDF file to an article.
 *
 * @ingroup ebms
 */
class FullTextForm extends FormBase {

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
   * {@inheritdoc}
   */
  public static function create(ContainerInterface $container): FullTextForm {
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
    return 'ebms_post_full_text_form';
  }

  /**
   * {@inheritdoc}
   */
  public function buildForm(array $form, FormStateInterface $form_state, int $article_id = 0): array {

    // Fetch the information we need about the article.
    $form_state->setValue('article', $article_id);
    $article = Article::load($article_id);

    // Fetch the information we need about the full-text PDF.
    $storage = $this->entityTypeManager->getStorage('file');
    $ft_desc = 'Select a PDF file smaller than 20 MB to be uploaded.';
    $title = 'Post Full-Text PDF File';
    if (!empty($article->full_text->file)) {
      $file = $storage->load($article->full_text->file);
      $ft_filename = $file->getFilename();
      $ft_user = $file->uid->entity->getDisplayName();
      $ft_date = date('Y-m-d', $file->created->value);
      $title = 'Replace Full-Text PDF File';
      $ft_desc .= " This will replace $ft_filename uploaded $ft_date by $ft_user";
    }

    return [
      '#title' => $title,
      'article-info' => [
        '#theme' => 'article_info',
        '#title' => $article->title->value,
        '#citation' => $article->getLabel(),
      ],
      'article' => [
        '#type' => 'hidden',
        '#value' => $article_id,
      ],
      'full-text' => [
        '#title' => 'Full-text PDF File',
        '#type' => 'file',
        '#attributes' => [
          'class' => ['usa-file-input'],
          'accept' => ['.pdf'],
        ],
        '#description' => $ft_desc,
      ],
      'unavailable' => [
        '#type' => 'checkbox',
        '#title' => 'Full-text file unavailable',
        '#description' => 'Check if attempts to obtain the full text for the article have not succeeded (and are not likely to in the future).',
      ],
      'comment' => [
        '#type' => 'textfield',
        '#title' => 'Comment',
        '#description' => 'Only stored if full-text file is unavailable.',
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
   * {@inheritdoc}
   */
  public function validateForm(array &$form, FormStateInterface $form_state) {
    $trigger = $form_state->getTriggeringElement()['#value'];
    if ($trigger === 'Cancel') {
      return;
    }
    $files = $this->getRequest()->files->get('files', []);
    $unavailable = $form_state->getValue('unavailable');
    $message = '';
    if (empty($files['full-text']) && empty($unavailable)) {
      $message = 'You must choose a PDF file or indicate that the file is unavailable (preferably with an explanation in the latter case).';
    }
    elseif (!empty($files['full-text']) && !empty($unavailable)) {
      $message = 'Full text PDF cannot be simultaneously available and unavailable.';
    }
    if (!empty($message)) {
      $form_state->setErrorByName('full-text', $message);
    }
  }

  /**
   * Return directly to the article page.
   *
   * @param array $form
   *   Form settings.
   * @param \Drupal\Core\Form\FormStateInterface $form_state
   *   Form values.
   */
  public function cancelSubmit(array &$form, FormStateInterface $form_state) {
    $article_id = $this->getRequest()->get('article');
    $this->returnToArticle($article_id, $form_state);
  }

  /**
   * {@inheritdoc}
   */
  public function submitForm(array &$form, FormStateInterface $form_state) {

    // Collect the form's values.
    $article_id = $form_state->getValue('article');
    $uid = $this->account->id();
    $now = date('Y-m-d H:i:s');
    $message = 'Full-Text PDF file stored.';
    if (empty($form_state->getValue('unavailable'))) {
      $validators = ['file_validate_extensions' => ['pdf']];
      $file = file_save_upload('full-text', $validators, 'public://', 0);
      $file->setPermanent();
      $file->save();
      $file_usage = \Drupal::service('file.usage');
      $file_usage->add($file, 'ebms_article', 'ebms_article', $article_id);
      $values = [
        'file' => $file->id(),
        'unavailable' => FALSE,
      ];
    }
    else {
      $values = [
        'unavailable' => TRUE,
        'flagged_as_unavailable' => $now,
        'flagged_by' => $uid,
      ];
      $notes = trim($form_state->getValue('comment') ?? '');
      if (!empty($notes)) {
        $values['notes'] = $notes;
      }
      $message = 'Full text PDF recorded as unavailable.';
    }

    // Plug them into the Article entity.
    $article = Article::load($article_id);
    $article->set('full_text', $values);
    $article->save();
    $this->messenger()->addMessage($message);

    // Back to the article page.
    $this->returnToArticle($article_id, $form_state);
  }

  /**
   * Return directly to the article page.
   *
   * @param int $article_id
   *   ID of article entity to whose page we are returning.
   * @param \Drupal\Core\Form\FormStateInterface $form_state
   *   Form values.
   */
  private function returnToArticle(int $article_id, FormStateInterface $form_state) {
    $route = 'ebms_article.article';
    $parameters = ['article' => $article_id];
    $options = ['query' => $this->getRequest()->query->all()];
    $form_state->setRedirect($route, $parameters, $options);
  }

}
