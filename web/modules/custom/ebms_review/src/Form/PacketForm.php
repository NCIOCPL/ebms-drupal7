<?php

namespace Drupal\ebms_review\Form;

use Drupal\Core\Database\Query\SelectInterface;
use Drupal\Core\Entity\EntityTypeManagerInterface;
use Drupal\Core\Form\FormBase;
use Drupal\Core\Form\FormStateInterface;
use Drupal\Core\Session\AccountProxyInterface;
use Drupal\Core\Url;
use Drupal\file\Entity\File;
use Symfony\Component\DependencyInjection\ContainerInterface;
use Drupal\ebms_article\Entity\Article;
use Drupal\ebms_board\Entity\Board;
use Drupal\ebms_doc\Entity\Doc;
use Drupal\ebms_message\Entity\Message;
use Drupal\ebms_review\Entity\Packet;
use Drupal\ebms_review\Entity\PacketArticle;
use Drupal\ebms_state\Entity\State;
use Drupal\ebms_topic\Entity\Topic;
use Drupal\user\Entity\User;

/**
 * Form for requesting reviews of articles for a given topic.
 *
 * @ingroup ebms
 */
class PacketForm extends FormBase {

  /**
   * Sort options for the articles.
   */
  const SORT_BY_AUTHOR = 'authors.0.last_name';
  const SORT_BY_JOURNAL = 'journal_title.value';

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
   * Are board members other than topic specialists selected?
   */
  private bool $nonSpecialists = FALSE;

  /**
   * {@inheritdoc}
   */
  public static function create(ContainerInterface $container): PacketForm {
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
    return 'ebms_packet_form';
  }

  /**
   * {@inheritdoc}
   */
  public function buildForm(array $form, FormStateInterface $form_state, $packet_id = NULL): array {

    // If we're editing an existing packet, board and topic are fixed.
    $method = $this->getRequest()->getMethod();
    ebms_debug_log("top of PacketForm::buildForm(): request method is $method");
    if (!empty($packet_id)) {
      $packet = Packet::load($packet_id);
      $board_id = $packet->topic->entity->board->target_id;
      $topic_id = $current_topic = $packet->topic->target_id;
      $packet_name = $packet->title->value;
      $title = "Edit Packet - {$packet_name}";
    }

    // Otherwise, see if board and topic have been chosen.
    else {
      $title = 'Add New Literature Surveillance Packet';
      $packet_name = $packet = null;
      $boards = Board::boards();
      $board_id = $form_state->getValue('board');
      if (empty($board_id)) {
        $board_id = Board::defaultBoard($this->account) ?: reset(array_keys($boards));
      }
      $topic_ids = $this->topicsForBoard($board_id);
      $topic_options = $this->topicOptions($board_id);
      if (empty($topic_options)) {
        $topic_description = "None of this board's topics have articles ready for packets.";
        $topic_options = [0 => 'Select a different board'];
      }
      else {
        $topic_description = 'Select the topic for this packet.';
      }
      $topic_id = $form_state->getValue('topic') ?: 0;
      $current_topic = $form_state->getValue('current-topic') ?: 0;
      if (!array_key_exists($topic_id, $topic_ids)) {
        $topic_id = NULL;
      }
    }

    // Start the form's render array.
    $form = [
      '#title' => $title,
      '#attached' => [
        'library' => ['ebms_review/packet-form'],
      ],
      'packet-id' => [
        '#type' => 'hidden',
        '#value' => $packet_id,
      ],
      'current-topic' => [
        '#type' => 'hidden',
        '#value' => $topic_id,
      ],
    ];

    // If this is a new packet, display board and topic picklists.
    if (empty($packet_id)) {
      $form['board'] = [
        '#type' => 'select',
        '#title' => 'Board',
        '#required' => TRUE,
        '#description' => 'Select a board to populate the Topic picklist.',
        '#options' => $boards,
        '#default_value' => $board_id,
        '#ajax' => [
          'callback' => '::boardChangeCallback',
          'wrapper' => 'board-controlled',
          'event' => 'change',
        ],
      ];
      $form['board-controlled'] = [
        '#type' => 'container',
        '#attributes' => ['id' => 'board-controlled'],
        'topic' => [
          '#type' => 'select',
          '#title' => 'Topic',
          '#required' => TRUE,
          '#description' => $topic_description,
          '#options' => $topic_options,
          '#ajax' => [
            'callback' => '::topicChangeCallback',
            'wrapper' => 'topic-controlled',
            'event' => 'change',
          ],
        ],
      ];
      if (array_keys($topic_options) == [0]) {
        $form['board-controlled']['topic']['#default_value'] = 0;
      }
    }

    // Otherwise, remember the selected board and topic in hidden fields.
    else {
      $form['board'] = [
        '#type' => 'hidden',
        '#value' => $board_id,
      ];
      $form['board-controlled'] = [
        'topic' => [
          '#type' => 'hidden',
          '#value' => $topic_id,
        ],
      ];
    }

    // Start the portion of the form controlled by the board/topic selections.
    $form['board-controlled']['topic-controlled'] = [
      '#type' => 'container',
      '#attributes' => ['id' => 'topic-controlled'],
    ];
    $topic_controlled =& $form['board-controlled']['topic-controlled'];

    // We only need the rest of the form if we have a board and a topic.
    if (!empty($topic_id)) {

      // Only allow the user to control article sort for new packets.
      if (empty($packet_id)) {
        $sort = $form_state->getValue('sort') ?: self::SORT_BY_AUTHOR;
        $topic_controlled['sort'] = [
          '#type' => 'select',
          '#title' => 'Sort Articles By',
          '#options' => [
            self::SORT_BY_AUTHOR => 'Author',
            self::SORT_BY_JOURNAL => 'Journal',
          ],
          '#default_value' => $sort,
          '#description' => 'Select sorting method for available articles.',
          '#ajax' => [
            'callback' => '::sortChangeCallback',
            'wrapper' => 'sort-controlled',
            'event' => 'change',
          ],
        ];
      }
      else {
        $sort = self::SORT_BY_AUTHOR;
      }
      $topic_controlled['sort-controlled'] = [
        '#type' => 'container',
        '#attributes' => ['id' => 'sort-controlled'],
      ];

      // Get the articles for the reviewable and FYI checkboxes.
      $articles = [];
      $fyi_articles = [];
      $article_defaults = $fyi_defaults = [];
      $query = $this->makeTopicQuery($topic_id, 'passed_full_review', $sort);
      ebms_debug_log('running reviewable articles query');
      $article_ids = $query->execute()->fetchCol();
      ebms_debug_log(count($article_ids) . ' reviewable articles found');
      $entities = Article::loadMultiple($article_ids);
      ebms_debug_log(count($article_ids) . ' reviewable articles loaded');
      if (!empty($packet_id)) {
        $packet_articles = $packet->activeArticles();
        $packet_article_ids = array_keys($packet_articles);
        $entities = $this->addPacketArticles($entities, $packet_articles, $topic_id, FALSE);
      }
      foreach ($entities as $article) {
        $articles[$article->id()] = $this->renderArticle($article, $topic_id);
      }
      $query = $this->makeTopicQuery($topic_id, 'fyi', $sort);
      ebms_debug_log('running fyi query');
      $article_ids = $query->execute()->fetchCol();
      ebms_debug_log(count($article_ids) . ' fyi articles found');
      $entities = Article::loadMultiple($article_ids);
      ebms_debug_log(count($article_ids) . ' fyi articles loaded');
      if (!empty($packet_id)) {
        $entities = $this->addPacketArticles($entities, $packet_articles, $topic_id, TRUE);
      }
      foreach ($entities as $article) {
        $fyi_articles[$article->id()] = $this->renderArticle($article, $topic_id);
      }

      // Only show a picklist if it has articles in it.
      if (!empty($articles)) {
        $article_defaults = array_keys($articles);
        if (!empty($packet_id)) {
          $article_defaults = array_intersect($article_defaults, $packet_article_ids);
        }
        $topic_controlled['sort-controlled']['articles'] = [
          '#type' => 'checkboxes',
          '#title' => 'Articles',
          '#required' => empty($fyi_articles),
          '#description' => 'Selected articles will be included in the packet. You may unselect any articles that you do not want to be included.',
          '#options' => $articles,
          '#default_value' => $article_defaults,
        ];
      }
      if (!empty($fyi_articles)) {
        $fyi_defaults = array_keys($fyi_articles);
        if (!empty($packet_id)) {
          $fyi_defaults = array_intersect($fyi_defaults, $packet_article_ids);
        }
        $topic_controlled['sort-controlled']['fyi-articles'] = [
          '#type' => 'checkboxes',
          '#title' => 'FYI Articles',
          '#required' => empty($articles),
          '#description' => 'These articles will not be reviewed.',
          '#options' => $fyi_articles,
          '#default_value' => $fyi_defaults,
        ];
      }

      // Add the checkboxes for the summaries.
      $summaries = $this->summariesForTopic($topic_id);
      if (!empty($summaries)) {
        $topic_controlled['summaries'] = [
          '#type' => 'checkboxes',
          '#title' => 'Summary Documents (Optional)',
          '#description' => 'You may select more than one summary document.',
          '#options' => $summaries,
        ];
        if (!empty($packet_id)) {
          $summary_defaults = [];
          foreach ($packet->summaries as $summary) {
            if (array_key_exists($summary->target_id, $summaries)) {
              $summary_defaults[] = $summary->target_id;
            }
          }
          if (!empty($summary_defaults)) {
            $topic_controlled['summaries']['#default_value'] = $summary_defaults;
          }
        }
      }

      // Add the checkboxes for packet reviewers.
      $topic_controlled['reviewer-block'] = [
        '#type' => 'container',
        '#attributes' => ['id' => 'reviewer-block'],
      ];
      $board_members = [];
      foreach (Board::boardMembers($board_id) as $user) {
        $board_members[$user->id()] = $user->name->value;
      }
      $reviewer_desc = 'Selected reviewers will receive this packet to review on their Assigned Packets page. Unselected reviewers who are associated with this topic will be able to view the packet on their FYI Packets page.';
      $selected_reviewers = [];
      if ($topic_id == $current_topic) {
        $form_reviewers = $form_state->getValue('reviewers');
        if (!is_null($form_reviewers)) {
          foreach ($form_reviewers as $key => $value) {
            if (!empty($value) && array_key_exists($key, $board_members)) {
              $selected_reviewers[] = $key;
            }
          }
        }
        elseif (!empty($packet_id)) {
          foreach ($packet->reviewers as $reviewer) {
            $selected_reviewers[] = $reviewer->target_id;
          }
        }
      }

      // Add a toggle between all board members and topic specialists.
      // But only there are topic specialists are they're a proper subset
      // of the entire board.
      $show_board_members = $form_state->getValue('show-board-members') ?? FALSE;
      $topic_reviewers = $this->reviewersForTopic($topic_id, $selected_reviewers);
      if (!empty($topic_reviewers) && $topic_reviewers != $board_members) {
        $reviewers = $topic_reviewers;
        if ($show_board_members) {
          $reviewers = $board_members;
        }
        elseif (!$this->nonSpecialists) {
          $reviewer_desc = "These reviewers are associated with this topic in the EBMS. $reviewer_desc  You may also select additional Board members who are not associated with this topic to review this packet.";
        }
        $topic_controlled['show-board-members'] = [
          '#type' => 'checkbox',
          '#title' => 'Show All Board Members',
          '#default_value' => $show_board_members,
          '#ajax' => [
            'callback' => '::toggleReviewersCallback',
            'wrapper' => 'reviewer-block',
            'event' => 'change',
          ],
        ];
      }
      else {
        $reviewers = $board_members;
      }

      // Include even inactive members, if they're included in the packet.
      if (!empty($packet->id)) {
        $changed = FALSE;
        foreach ($packet->reviewers as $reviewer) {
          $uid = $reviewer->target_id;
          if (!array_key_exists($uid, $reviewers)) {
            $user = User::load($uid);
            if (!empty($user)) {
              $reviewers[$uid] = $user->name->value;
              $changed = TRUE;
            }
          }
        }
        if ($changed) {
          natsort($reviewers);
        }
      }

      // Draw the reviewer checkboxes.
      $reviewers_defaults = $selected_reviewers ?: array_keys($topic_reviewers);
      $topic_controlled['reviewer-block']['reviewers'] = [
        '#type' => 'checkboxes',
        '#title' => 'Reviewers',
        '#required' => TRUE,
        '#description' => $reviewer_desc,
        '#options' => $reviewers,
        '#default_value' => $reviewers_defaults,
      ];

      // Add the packet name field. This should change when the topic is
      // changed for a new packet, but for some reason it doesn't. Haven't
      // tracked down whether this is a known bug, but it appears to affect
      // the original EBMS in the same way.
      if (empty($packet_id)) {
        $topic = Topic::load($topic_id);
        $now = date('F Y');
        $packet_name = "{$topic->name->value} ($now)";
        ebms_debug_log("packet name is $packet_name");
      }
      $topic_controlled['name'] = [
        '#type' => 'textfield',
        '#title' => 'Packet Name',
        '#required' => TRUE,
        '#default_value' => $packet_name,
      ];
      $topic_controlled['submit'] = [
        '#type' => 'submit',
        '#value' => 'Submit',
      ];

      // Stash away some values we need for working around Drupal bugs.
      $form_state->setValue('articles-defaults', $article_defaults);
      $form_state->setValue('fyi-articles-defaults', $fyi_defaults);
      $form_state->setValue('reviewers-defaults', $reviewers_defaults);
      $form_state->setValue('reviewer-description', $reviewer_desc);
    }

    // We're done.
    ebms_debug_log('return from PacketForm::buildForm()');
    return $form;
  }

  /**
   * {@inheritdoc}
   */
  public function validateForm(array &$form, FormStateInterface $form_state) {

    // Collect the IDs for the selected articles (FYI or not).
    $fields = ['articles', 'fyi-articles'];
    $error_field = NULL;
    $ids = [];
    foreach ($fields as $name) {
      foreach ($form_state->getValue($name) as $key => $value) {
        if (is_null($error_field)) {
          $error_field = $name;
        }
        if (!empty($value)) {
          $ids[] = $key;
        }
      }
    }
    if (empty($ids)) {
      $form_state->setErrorByName($error_field, 'At least one article must be selected.');
    }
    else {
      $form_state->setValue('article-ids', $ids);
    }

    // Collect the IDs for the selected reviewers.
    $ids = [];
    foreach ($form_state->getValue('reviewers') as $key => $value) {
      if (!empty($value)) {
        $ids[] = $key;
      }
    }
    if (empty($ids)) {
      $form_state->setErrorByName('reviewers', 'At least one reviewer must be selected.');
    }
    else {
      $form_state->setValue('reviewer-ids', $ids);
    }

    // Collect the IDs for the selected summary documents (if any).
    $ids = [];
    foreach ($form_state->getValue('summaries') as $key => $value) {
      if (!empty($value)) {
        $ids[] = $key;
      }
    }
    $form_state->setValue('summary-ids', $ids);
  }

  /**
   * {@inheritdoc}
   */
  public function submitForm(array &$form, FormStateInterface $form_state) {

    // Create and save a new packet if appropriate.
    $title = trim($form_state->getValue('name'));
    $uid = $this->account->id();
    $packet_id = $form_state->getValue('packet-id');
    $topic_id = $form_state->getValue('topic');
    $reviewer_ids = $form_state->getValue('reviewer-ids');
    $now = date('Y-m-d H:i:s');
    if (empty($packet_id)) {
      $articles = [];

      // Create a new `PacketArticle` entity for each selected article.
      foreach ($form_state->getValue('article-ids') as $article_id) {
        $article = PacketArticle::create(['article' => $article_id]);
        $article->save();
        $articles[] = $article->id();
      }

      // Assemble the values, create the `Packet` entity, and save it.
      $values = [
        'articles' => $articles,
        'topic' => $topic_id,
        'summaries' => $form_state->getValue('summary-ids'),
        'reviewers' => $reviewer_ids,
        'title' => $title,
        'created' => $now,
        'created_by' => $this->account->id(),
      ];
      $packet = Packet::create($values);
      $packet->save();

      // Remember this activity for the home page.
      $topic_reviewer_ids = $this->reviewersForTopic($topic_id, []);
      foreach ($topic_reviewer_ids as $reviewer_id) {
        if (!in_array($reviewer_id, $reviewer_ids)) {
          $reviewer_ids[] = $reviewer_id;
        }
      }
      Message::create([
        'message_type' => Message::PACKET_CREATED,
        'user' => $uid,
        'posted' => $now,
        'individuals' => $reviewer_ids,
        'extra_values' => json_encode([
          'packet_id' => $packet->id(),
          'title' => $title,
        ]),
      ])->save();
    }

    // Otherwise, update and save the packet we've been editing.
    else {
      $packet = Packet::load($packet_id);
      $original_reviewer_ids = [];
      foreach ($packet->reviewers as $reviewer) {
        $original_reviewer_ids[] = $reviewer->target_id;
      }
      $packet->set('summaries', $form_state->getValue('summary-ids'));
      $packet->set('reviewers', $reviewer_ids);
      $packet->set('title', $title);
      $already_saved = [];
      $articles = $form_state->getValue('article-ids');

      // Update the article we already had.
      foreach ($packet->articles as $article) {
        $packet_article = $article->entity;
        $article_id = $packet_article->article->target_id;
        $already_saved[] = $article_id;
        $changed = FALSE;
        if (empty($packet_article->dropped->value)) {
          if (!in_array($article_id, $articles)) {
            $packet_article->set('dropped', TRUE);
            $changed = TRUE;
          }
        }
        elseif (in_array($article_id, $articles)) {
          $packet_article->set('dropped', FALSE);
          $changed = TRUE;
        }
        if ($changed) {
          $packet_article->save();
        }

        // If reviewers have been added, let them know on the home page.
        $new_reviewer_ids = array_diff($reviewer_ids, $original_reviewer_ids);
        if (!empty($new_reviewer_ids)) {
          Message::create([
            'message_type' => Message::PACKET_CREATED,
            'user' => $uid,
            'posted' => $now,
            'individuals' => $new_reviewer_ids,
            'extra_value' => json_encode([
              'packet_id' => $packet->id(),
              'title' => $title,
            ]),
          ])->save();
        }
      }

      // Add new `PacketArticle` entities for the ones we added.
      $to_be_saved = array_diff($articles, $already_saved);
      foreach ($to_be_saved as $article_id) {
        $article = PacketArticle::create(['article' => $article_id]);
        $article->save();
        $packet->articles[] = $article->id();
      }
      $packet->save();
    }

    // Report success and return to the packets list.
    $this->messenger()->addMessage("Saved packet $title.");
    $query = $this->getRequest()->query->all();
    $parms = $opts = [];
    if (!empty($query['filter-id'])) {
      $parms = ['request_id' => $query['filter-id']];
      unset($query['filter-id']);
      if (!empty($query)) {
        $opts['query'] = $query;
      }
    }
    $form_state->setRedirect('ebms_review.packets', $parms, $opts);
  }

  /**
   * Add the articles which are currently selected for the packet.
   *
   * Because the entity query APIs are not as flexible as standard
   * database queries, our base query excludes articles which are
   * already in any packet. But for the articles which are in the
   * packet being edited, we obviously need to include those. This
   * requires us to re-sort the articles so the ones in the packet
   * are in the right positions.
   *
   * @param array $entities
   *   Article entities found by the base query.
   * @param array $assigned
   *   Articles currently assigned to the packet being edited.
   * @param integer $topic_id
   *   The topic for the current packet.
   * @param boolean $fyi
   *   Whether we are looking for articles whose current state is FYI.
   *
   * @return array
   *   The expanded array of articles.
   */
  private function addPacketArticles(array $entities, array $assigned, int $topic_id, bool $fyi): array {
    if (empty($assigned)) {
      return $entities;
    }
    foreach ($assigned as $id => $article) {
      if (!array_key_exists($id, $entities)) {
        $state = $article->getCurrentState($topic_id);
        $fyi_article = !empty($state) && $state->value->entity->field_text_id->value === 'fyi';
        if ($fyi === $fyi_article) {
          $entities[$id] = $article;
        }
      }
    }
    uasort($entities, function($a, $b) {
      $a_title = $a->title->value ?? '';
      $b_title = $b->title->value ?? '';
      $a_name = $b_name = 'aaaaa';
      foreach ($a->authors as $author) {
        if (!empty($author->last_name)) {
          $a_name = $author->last_name;
          break;
        }
      }
      foreach ($b->authors as $author) {
        if (!empty($author->last_name)) {
          $b_name = $author->last_name;
        }
      }
      return strnatcasecmp("$a_name|$a_title", "$b_name|$b_title");
    });
    return $entities;
  }

  /**
   * Return the portion of the form which changes when the board changes.
   *
   * @param array $form
   *   Render array for the form.
   * @param FormStateInterface $form_state
   *   Access to the values on the form.
   *
   * @return array
   *   The portion of the form which changes with a different board selection.
   */
  public function boardChangeCallback(array &$form, FormStateInterface $form_state): array {
    return $form['board-controlled'];
  }

  /**
   * Create the query to find review-ready articles for the specified topic.
   *
   * This is invoked from two places, once where we need the count of articles
   * eligible for review for the topic, and the second where we need to fetch
   * the actual `Article` entities.
   *
   * @param integer $topic_id
   *   ID of the `Topic` entity.
   * @param string $state_text_id
   *   ID of the FYI or Full Text Approved state.
   * @param string $sort
   *
   * @return SelectInterface
   *   Object for finding review-ready articles.
   */
  private function makeTopicQuery(int $topic_id, string $state_text_id, string $sort): SelectInterface {
    $state_id = State::getStateId($state_text_id);
    $query = \Drupal::database()->select('ebms_state', 'state');
    $query->join('ebms_article', 'article', 'article.id = state.article');
    $query->addField('article', 'id');
    $query->condition('state.topic', $topic_id);
    $query->condition('state.value', $state_id);
    $query->condition('state.current', 1);
    $query->isNotNull('article.full_text__file');
    if ($state_text_id === 'fyi') {
      $query->condition('state.entered', '2016-02-01', '>=');
    }
    $query->leftJoin('ebms_packet_article', 'packet_article', 'packet_article.article = article.id AND packet_article.dropped = 0');
    $query->leftJoin('ebms_packet__articles', 'packet_articles', 'packet_articles.articles_target_id = packet_article.id');
    $query->leftJoin('ebms_packet', 'packet', 'packet.id = packet_articles.entity_id AND packet.topic = state.topic');
    $query->isNull('packet.id');
    if ($sort === self::SORT_BY_AUTHOR) {
      $query->leftJoin('ebms_article__authors', 'author', 'author.entity_id = article.id AND author.delta = 0');
      $query->orderBy('author.authors_display_name');
    }
    else {
      $query->leftJoin('ebms_journal', 'journal', 'journal.source_id = article.source_journal_id');
      $query->orderBy('journal.core', 'DESC');
      $query->orderBy('article.journal_title');
    }
    $query->orderBy('article.title');
    ebms_debug_log((string) $query, 3);
    return $query;
  }

  /**
   * Assemble what gets displayed for an article on the form's picklist.
   *
   * My understanding of the current model for the Drupal rendering pipeline
   * is that the goal is to defer the actual conversion to HTML (or whatever)
   * as long as possible, keeping the information to be processed in render
   * arrays until then. However, the form API seems to be interfering with
   * that goal in some places because of some rigid, simplistic assumptions
   * made about some of the specific elements. In the case before us here,
   * we're working with a requirement to present the user with a set of
   * checkboxes for published articles to be added to a review packet, and
   * each item should show the article’s citation, with the article’s title
   * wrapped in a link which will open the full article page in a separate
   * tab, a button for showing the article’s full text, and some formatted
   * optional additional information, such as highlighting of high-priority
   * articles, display of board manager comments in a different font, etc.
   * In other words, not your grandmother’s checkbox plus plain string.
   * Following the idealized model, one would think we should assemble render
   * arrays for the information to be displayed, indexed by the IDs of the
   * articles, and assign that indexed array to the #options property of the
   * checkboxes field. But that doesn’t work because according to the
   * documentation (and the behavior) the `Checkboxes` class won’t accept
   * anything but strings as the values of the options array. So I’m holding
   * my nose and calling the rendering service’s render() with each render
   * array so I can meet the dragon’s requirements.
   *
   * That still leaves us with a puzzle about when Drupal accepts marked up
   * HTML and when it treats a string as needing to have markup escaped.
   * Obviously this approach is putting marked-up strings in the #options
   * property, and Drupal isn't escaping them, so this is doing what we need.
   * It would be nice if the documentation were more forthcoming.
   * https://api.drupal.org/api/drupal/core!lib!Drupal!Core!Render!Element!Checkboxes.php/class/Checkboxes
   * only says "... array ... whose values are the labels next to each
   * checkbox."
   *
   * @param Article $article
   *   Reference to the article to be rendered.
   * @param int $topic_id
   *   The topic for which we are rendering the article.
   *
   * @return string
   *   The rendered HTML for the article.
   */
  private function renderArticle(Article $article, $topic_id): string {

    // Start with the first author surname we find.
    $author_surname = '';
    foreach ($article->authors as $author) {
      if (!empty($author->last_name)) {
        $author_surname = $author->last_name;
        break;
      }
    }

    // Find any articles related to this one.
    $related = [];
    foreach ($article->getRelatedArticles() as $related_article) {
      $display = [];
      foreach ($related_article->authors as $author) {
        if (!empty($author->last_name)) {
          $display[] = $author->last_name;
          break;
        }
      }
      $display[] = $related_article->brief_journal_title->value;
      $display[] = $related_article->year->value;
      $values = [
        'citation' => implode(' ', $display),
        'pmid' => $related_article->source_id->value,
        'url' => Url::fromRoute('ebms_article.article', ['article' => $related_article->id()]),
        'id' => $related_article->id(),
      ];
      $related[] = $values;
    }

    // Find out if the article is tagged "high priority" for this topic.
    $article_topic = $article->getTopic($topic_id);
    $high_priority = FALSE;
    foreach ($article_topic->tags as $tag) {
      if ($tag->entity->tag->entity->field_text_id->value === 'high_priority') {
        if (!empty($tag->entity->active->value)) {
          $high_priority = TRUE;
          break;
        }
      }
    }

    // Find any board manager comments for the topic.
    $comments = [];
    foreach ($article_topic->comments as $comment) {
      $comments[] = $comment->comment;
    }

    // Articles in all but the oldest packets have the full text PDF file.
    $file = File::load($article->full_text->file);

    // Apply the theme to the values to get the rendered HTML.
    $element = [
      '#theme' => 'packet_form_article',
      '#author' => $author_surname,
      '#title' => $article->title->value,
      '#article_url' => Url::fromRoute('ebms_article.article', ['article' => $article->id()]),
      '#journal' => $article->brief_journal_title->value ?? '',
      '#pmid' => $article->source_id->value ?? '',
      '#year' => $article->year->value ?? '',
      '#full_text_url' => empty($file) ? '' : $file->createFileUrl(),
      '#related' => $related,
      '#high_priority' => $high_priority,
      '#comments' => $comments,
    ];
    return \Drupal::service('renderer')->render($element);
  }

  /**
   * Find the reviewers who specialize in the packet's topic.
   *
   * We also include the reviewers who are currently assigned to the packet,
   * so they don't get dropped without an explicit decision to do so.
   *
   * @param integer $topic_id
   *   ID of the packet's topic.
   * @param array $selected_reviewers
   *   IDs of the users who are currently assigned as reviewers in the packet.
   *
   * @return array
   *   Sorted names of reviewers indexed by their `User` entity ID.
   */
  private function reviewersForTopic(int $topic_id, array $selected_reviewers): array {
    ebms_debug_log('top of PacketForm::reviewersForTopic()');
    $this->nonSpecialists = FALSE;
    $roles = user_role_names(TRUE, 'review literature');
    $storage = $this->entityTypeManager->getStorage('user');
    $query = $storage->getQuery()->accessCheck(FALSE);
    $query->condition('status.value', 1);
    $query->condition('topics', $topic_id);
    $query->condition('roles', 'board_member');
    $users = $storage->loadMultiple($query->execute());
    $reviewers = [];
    foreach ($users as $user) {
      $reviewers[$user->id()] = $user->name->value;
    }
    if (!empty($selected_reviewers)) {
      foreach ($selected_reviewers as $id) {
        if (!array_key_exists($id, $reviewers)) {
          $this->nonSpecialists = TRUE;
          $user = $storage->load($id);
          $reviewers[$id] = $user->name->value;
        }
      }
    }
    natsort($reviewers);
    ebms_debug_log('return from PacketForm::reviewersForTopic()');
    return $reviewers;
  }

  /**
   * Get the updated part of the form altered by change in article sort.
   *
   * This implementation preserves behavior present in the Drupal 7 version,
   * in which the decisions made before the sort change about which articles
   * to omit from the selection were discarded, and the user would need to
   * un-check those articles again. That may not actually be desirable
   * behavior, but we'll defer that discussion for later.
   *
   * @param array $form
   *   The render array for the form.
   * @param FormStateInterface $form_state
   *   The object which lets us interrogate (and set) the form's values.
   *
   * @return array
   *   The portion of the form which changes with the article sort option.
   */
  public function sortChangeCallback(array &$form, FormStateInterface $form_state): array {
    $subform =& $form['board-controlled']['topic-controlled'];
    $this->workAroundBug1100170($subform, $form_state);
    return $subform['sort-controlled'];
  }

  /**
   * Find the summary documents tagged with the current topic.
   *
   * @param int $topic_id
   *   ID of the current topic.
   *
   * @return array
   *   Summary documents indexed by the entity IDs.
   */
  private function summariesForTopic($topic_id): array {
    ebms_debug_log('top of PacketForm::summariesForTopic()');
    $storage = $this->entityTypeManager->getStorage('ebms_doc');
    $query = $storage->getQuery()->accessCheck(FALSE);
    $query->condition('topics', $topic_id);
    $query->condition('tags.entity.field_text_id', 'summary');
    $query->condition('dropped', 0);
    $ids = $query->execute();
    $summaries = [];
    if (!empty($ids)) {
      foreach ($ids as $id) {
        $summary = Doc::load($id);
        $description = $summary->description->value ?? '';
        if (empty($description)) {
          $description = $summary->file->entity->filename->value;
        }
        $summaries[$id] = $description;
      }
    }
    ebms_debug_log('return from PacketForm::summariesForTopic()');
    return $summaries;
  }

  /**
   * Get the updated part of the form altered by reviewer display changes.
   *
   * @param array $form
   *   The render array for the form.
   * @param FormStateInterface $form_state
   *   The object which lets us interrogate (and set) the form's values.
   *
   * @return array
   *   The portion of the form which changes with the reviewer display option.
   */
  public function toggleReviewersCallback(array &$form, FormStateInterface $form_state): array {
    $subform =& $form['board-controlled']['topic-controlled'];
    $this->workAroundBug1100170($subform, $form_state, TRUE);
    return $subform['reviewer-block'];
  }

  /**
   * Get the updated part of the form altered by topic selection changes.
   *
   * @param array $form
   *   The render array for the form.
   * @param FormStateInterface $form_state
   *   The object which lets us interrogate (and set) the form's values.
   *
   * @return array
   *   The portion of the form which changes with a different topic selection.
   */
  public function topicChangeCallback(array &$form, FormStateInterface $form_state): array {
    $subform =& $form['board-controlled']['topic-controlled'];
    $this->workAroundBug1100170($subform, $form_state);
    return $subform;
  }

  /**
   * Build the picklist for options with articles needing review.
   *
   * Some of the boards have so many topics that they group the topics
   * to make it easier to find specific ones. The returned array uses
   * these topic groupings when they are present.
   *
   * As in the original EBMS, not even the database API is able to handle
   * a query this complicated, with a nested correlated query.
   *
   * @param int $board_id
   *   IDs of the selected board's topics.
   *
   * @return array
   *   Possibly grouped picklist for topics, showing how many articles are
   *   available for each topic.
   */
  private function topicOptions($board_id): array {

    // Collect groups and topics indexed by the sorting strings.
    ebms_debug_log('top of PacketForm::topicOptions()');
    $fyi = State::getStateId('fyi');
    $passed = State::getStateId('passed_full_review');
    $correlated_query = <<<EOT
      SELECT DISTINCT packet_article.article
                 FROM ebms_packet_article packet_article
                 JOIN ebms_packet__articles packet_articles ON packet_articles.articles_target_id = packet_article.id
                WHERE packet_articles.entity_id IN (SELECT id FROM ebms_packet WHERE topic = t.id)
    EOT;
    $query = <<<EOT
      SELECT t.id, COUNT(DISTINCT a.id) AS count
        FROM ebms_topic t
        JOIN ebms_state s ON s.topic = t.id
        JOIN ebms_article a ON a.id = s.article
       WHERE s.current = 1
         AND (s.value = $passed OR (s.value = $fyi AND s.entered >= '2016-02-01'))
         AND t.board = $board_id
         AND a.full_text__file IS NOT NULL
         AND s.article NOT IN ($correlated_query)
    GROUP BY t.id
    EOT;
    ebms_debug_log('PacketForm::topicOptions() query = ' . (string) $query, 3);
    $results = \Drupal::database()->query($query);
    $counts = [];
    foreach ($results as $result) {
      $counts[$result->id] = $result->count;
    }

    // If the board has no packet-eligible articles, nothing more to do.
    if (empty($counts)) {
      return [];
    }

    $query = \Drupal::database()->select('ebms_topic', 'topic');
    $query->leftJoin('taxonomy_term_field_data', 'grp', 'grp.tid = topic.topic_group');
    $query->condition('topic.id', array_keys($counts), 'IN');
    $query->fields('topic', ['id', 'name']);
    $query->addField('grp', 'name', 'group');
    $query->orderBy('topic.name');
    $results = $query->execute();
    $topic_map = [];
    foreach ($results as $result) {
      $id = $result->id;
      $name = $result->name;
      $group = $result->group;
      $count = $counts[$id];
      $display = "$name ($count)";
      if (empty($group)) {
        $topic_map[$name] = "$id|$display";
      }
      else {
        $topic_map[$group][$id] = $display;
      }
    }

    // If the board has no packet-eligible articles, nothing more to do.
    if (empty($topic_map)) {
      return [];
    }

    // Sort the grouped and ungrouped topics into a single sequence. In
    // practice, if a board has any topics grouped, all its topics are
    // grouped. Nevertheless, we support mixing grouped and ungrouped
    // topics.
    ksort($topic_map);
    $topics = [];
    foreach ($topic_map as $key => $value) {
      if (is_array($value)) {
        $topics[$key] = $value;
      }
      else {
        list($id, $display) = explode('|', $value, 2);
        $topics[$id] = $display;
      }
    }
    ebms_debug_log('return from PacketForm::topicOptions()');
    return $topics;
  }

  /**
   * Get the IDs of the `Topic` entities associated with a board.
   *
   * @param int $board_id
   *   ID of the board whose topics we want.
   *
   * @return array
   *   IDs of this board's topics.
   */
  private function topicsForBoard($board_id): array {
    $storage = $this->entityTypeManager->getStorage('ebms_topic');
    $query = $storage->getQuery()->accessCheck(FALSE);
    $query->condition('board', $board_id);
    $query->sort('name');
    return $query->execute();
  }

  /**
   * Work around bug https://drupal.org/node/1100170.
   *
   * This bug (which is over a decade old!) prevents checkbox defaults from
   * working properly. Checkboxes (and radios for that matter) are poorly
   * documented, so this workaround took some heavy-duty reverse engineering.
   *
   * @param array $wrapper
   *   Container in which the article checkboxes live.
   * @param FormStateInterface $form_state
   *   The object which lets us interrogate (and set) the form's values.
   */
  private function workAroundBug1100170(array &$wrapper, FormStateInterface $form_state, bool $reviewers_only = FALSE) {
    $fields = [
      'articles' => 'sort-controlled',
      'fyi-articles' => 'sort-controlled',
      'reviewers' => 'reviewer-block',
    ];
    $wrapper['reviewer-block']['reviewers']['#description'] = $form_state->getValue('reviewer-description');
    foreach ($fields as $name => $block) {
      if ($reviewers_only && $name !== 'reviewers') {
        continue;
      }
      $defaults = $form_state->getValue($name . '-defaults') ?: [];
      if (!empty($wrapper[$block][$name])) {
        foreach ($wrapper[$block][$name] as $k => $v) {
          if (isset($v['#type']) && $v['#type'] === 'checkbox') {
            $checked = in_array($k, $defaults) ? 1 : 0;
            $wrapper[$block][$name][$k]['#checked'] = $checked;
          }
        }
      }
    }
  }

}
