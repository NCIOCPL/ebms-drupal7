<?php

namespace Drupal\ebms_review\Controller;

use Drupal\Core\Ajax\AjaxResponse;
use Drupal\Core\Ajax\OpenModalDialogCommand;
use Drupal\Core\Controller\ControllerBase;
use Drupal\Core\Form\FormBuilder;
use Drupal\ebms_review\Entity\PacketArticle;
use Symfony\Component\DependencyInjection\ContainerInterface;

/**
 * Opens the modal form for an expedited article rejection.
 */
class QuickRejectController extends ControllerBase {

  /**
   * The form builder.
   *
   * @var \Drupal\Core\Form\FormBuilder
   */
  protected $formBuilder;

  /**
   * The ModalFormExampleController constructor.
   *
   * @param \Drupal\Core\Form\FormBuilder $formBuilder
   *   The form builder.
   */
  public function __construct(FormBuilder $formBuilder) {
    $this->formBuilder = $formBuilder;
  }

  /**
   * {@inheritdoc}
   *
   * @param \Symfony\Component\DependencyInjection\ContainerInterface $container
   *   The Drupal service container.
   *
   * @return static
   */
  public static function create(ContainerInterface $container) {
    return new static($container->get('form_builder'));
  }

  /**
   * Callback for opening the modal form.
   */
  public function openQuickRejectForm(int $packet_id, int $packet_article_id) {

    // Get the modal form using the form builder.
    $modal_form = $this->formBuilder->getForm('Drupal\ebms_review\Form\QuickReject', $packet_id, $packet_article_id);

    // Construct the form's title.
    $packet_article = PacketArticle::load($packet_article_id);
    $article = $packet_article->article->entity;
    $journal = $article->brief_journal_title->value;
    $pmid = $article->source_id->value;
    $year = $article->year->value;
    $author = '';
    foreach ($article->authors as $author) {
      if (!empty($author->last_name)) {
        $author = ' ' . $author->last_name;
        break;
      }
    }
    $title = "Rejecting article$author $journal $year PMID $pmid";

    // Add an AJAX command to open a modal dialog with the form as the content.
    $response = new AjaxResponse();
    $response->addCommand(new OpenModalDialogCommand($title, $modal_form, ['width' => '90%']));

    return $response;
  }

}
