<?php

namespace Drupal\ebms_topic\Form;

use Drupal\Core\Form\FormBase;
use Drupal\Core\Form\FormStateInterface;
use Drupal\ebms_board\Entity\Board;
use Drupal\ebms_topic\Entity\Topic;
use Drupal\user\Entity\User;

/**
 * Manage default reviewers for topic.
 *
 * @ingroup ebms
 */
class TopicReviewersForm extends FormBase {

  /**
   * {@inheritdoc}
   */
  public function getFormId(): string {
    return 'topic_reviewers_form';
  }

  /**
   * {@inheritdoc}
   */
  public function buildForm(array $form, FormStateInterface $form_state, int $topic_id = 0): array {
    $topic = Topic::load($topic_id);
    $board_members = [];
    foreach (Board::boardMembers($topic->board->target_id) as $uid => $user) {
      $board_members[$uid] = $user->name->value;
    }
    $reviewers = array_intersect_key($board_members, $topic->reviewers());
    return [
      '#title' => $topic->name->value,
      'topic' => [
        '#type' => 'hidden',
        '#value' => $topic_id,
      ],
      'reviewers' => [
        '#type' => 'checkboxes',
        '#title' => 'Reviewers',
        '#description' => 'Select the board members with default responsibility for reviewing articles to which this topic has been assigned.',
        '#options' => $board_members,
        '#default_value' => array_keys($reviewers),
      ],
      'submit' => [
        '#type' => 'submit',
        '#value' => 'Save',
      ],
    ];
  }

  /**
   * {@inheritdoc}
   */
  public function submitForm(array &$form, FormStateInterface $form_state) {
    $topic_id = $form_state->getValue('topic');
    $topic = Topic::load($topic_id);
    $topic_name = $topic->name->value;
    $old_reviewers = $topic->reviewers();
    $new_reviewers = array_values(array_diff($form_state->getValue('reviewers'), [0]));
    foreach ($old_reviewers as $uid => $name) {
      if (!in_array($uid, $new_reviewers)) {
        $user = User::load($uid);
        foreach ($user->topics as $position => $user_topic) {
          if ($user_topic->target_id == $topic_id) {
            $user_name = $user->name->value;
            $user->topics->removeItem($position);
            $user->save();
            $this->getLogger('ebms_topic')->info("Removed $user_name as default reviewer for topic $topic_name.");
            break;
          }
        }
      }
    }
    foreach ($new_reviewers as $uid) {
      if (!array_key_exists($uid, $old_reviewers)) {
        $user = User::load($uid);
        $user_name = $user->name->value;
        $user->topics[] = $topic_id;
        $user->save();
        $this->getLogger('ebms_topic')->info("Added $user_name as default reviewer for topic $topic_name.");
      }
    }
    $this->messenger()->addMessage('Saved changes to reviewer list.');
    $report_id = $this->getRequest()->query->get('report');
    $form_state->setRedirect('ebms_report.topic_reviewers', ['report_id' => $report_id]);
  }

}
