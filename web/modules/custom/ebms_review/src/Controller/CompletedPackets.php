<?php

namespace Drupal\ebms_review\Controller;

use Drupal\Core\Controller\ControllerBase;
use Drupal\Core\Link;

/**
 * Provide a list of packets this board member has completed.
 */
class CompletedPackets extends ControllerBase {

  /**
   * Create the render array for the packets list.
   */
  public function display() {

    // Find the sequence number for the FYI state.
    $storage = $this->entityTypeManager()->getStorage('taxonomy_term');
    $query = $storage->getQuery()->accessCheck(FALSE);
    $query->condition('vid', 'states');
    $query->condition('field_text_id', 'fyi');
    $fyi = $storage->load(reset($query->execute()));
    $fyi_sequence = $fyi->field_sequence->value;

    // We're not interested in packets reviewed more than two years ago.
    $cutoff = new \DateTime();
    $cutoff->sub(new \DateInterval('P2Y'));
    $cutoff = $cutoff->format('Y-m-d');
    ebms_debug_log("completed packets cutoff is $cutoff");

    // Find the packets which might be included on this page. Note that we're
    // not checking whether the packet is still active. It's enough that the
    // user has reviewed all of the articles before the packet was deactivated.
    // This conforms with the original logic for this page.
    // Doesn't seem to be a way to use an aggregate in a TableSort query.
    // See https://drupal.stackexchange.com/questions/311200.
    $uid = $this->currentUser()->id();
    $storage = $this->entityTypeManager()->getStorage('ebms_packet');
    $query = $storage->getQuery()->accessCheck(FALSE);
    $query->condition('articles.entity.reviews.entity.reviewer', $uid);
    $query->condition('articles.entity.reviews.entity.posted', $cutoff, '>=');
    $packets = $storage->loadMultiple($query->execute());
    ebms_debug_log('packet count: ' . count($packets));
    $rows = [];

    // Decide which of the packets are completed.
    foreach ($packets as $packet) {

      // Set initial values for the packet.
      $topic_id = $packet->topic->target_id;
      $count = 0;
      $completed = TRUE;
      $latest = '';
      $packet_id = $packet->id();
      ebms_debug_log("completed packets: checking packet $packet_id");

      // Walk through each of the packet's articles.
      foreach ($packet->articles as $packet_article) {

        // Find out if this user has reviewed the article.
        $reviewed = FALSE;
        foreach ($packet_article->entity->reviews as $review) {
          if ($review->entity->reviewer->target_id == $uid) {
            ebms_debug_log("completed packets: found one of the current user's reviews");
            $reviewed = TRUE;
            $count++;
            if (strcmp($review->entity->posted->value, $latest) > 0) {
              $latest = $review->entity->posted->value;
              ebms_debug_log("completed packets: latest review for this packet is now $latest");
            }
          }
        }

        // If not, see if that prevents us from including the packet.
        if (!$reviewed) {

          // OK if the article has been dropped (this is a change from the
          // original logic, which ignored the drop flag in this situation).
          if (!empty($packet_article->entity->dropped->value)) {
            continue;
          }

          // OK if the article has moved on to a later state.
          $article = $packet_article->entity->article->entity;
          $current_state = $article->getCurrentState($topic_id);
          if ($current_state->value->entity->field_sequence->value > $fyi_sequence) {
            $article_id = $article->id();
            ebms_debug_log("completed packets: skipping article $article_id which has moved on to greener pastures");
            continue;
          }

          // The reviewer isn't expected to review FYI articles.
          if ($current_state->value->entity->field_text_id === 'FYI') {
            ebms_debug_log('completed packets: skipping FYI article');
            continue;
          }

          // If we got here, there's an article in the packet still
          // waiting to be reviewed. Don't include the packet.
          $completed = FALSE;
          break;
        }
      }

      // Add the packet if the board member reviewed the reviewable articles.
      ebms_debug_log("completed packets: count is $count and completed is " . ($completed ? 'TRUE' : 'FALSE'));
      if (!empty($count) && $completed) {
        $rows[] = [
          $packet->title->value,
          substr($latest, 0, 10),
          $count,
          $packet->id(),
        ];
      }
    }

    // Sort the rows by hand.
    usort($rows, function(array &$a, array &$b): int {
      return $b[1] <=> $a[1] ?: $a[0] <=> $b[0];
    });

    // Inject links for the first column.
    foreach ($rows as &$row) {
      $name = $row[0];
      $packet_id = $row[3];
      unset($row[3]);
      $row[0] = Link::createFromRoute($name, 'ebms_review.completed_packet', ['packet_id' => $packet_id]);
    }
    // Assemble the render array for the page.
    return [
      '#title' => 'Completed Packets',
      '#cache' => ['max-age' => 0],
      'packets' => [
        '#theme' => 'table',
        '#rows' => $rows,
        '#header' => ['Packet Name', 'Date Completed', 'Number of Reviews'],
        '#empty' => 'No packets have been completed in the past two years.',
      ],
    ];
  }

}
