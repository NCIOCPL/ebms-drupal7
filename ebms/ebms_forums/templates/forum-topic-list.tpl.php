<?php
/**
 * @file
 * Displays a list of forum topics.
 *
 * Available variables:
 * - $header: The table header. This is pre-generated with click-sorting
 *   information. If you need to change this, see
 *   template_preprocess_forum_topic_list().
 * - $pager: The pager to display beneath the table.
 * - $topics: An array of topics to be displayed. Each $topic in $topics
 *   contains:
 *   - $topic->icon: The icon to display.
 *   - $topic->moved: A flag to indicate whether the topic has been moved to
 *     another forum.
 *   - $topic->title: The title of the topic. Safe to output.
 *   - $topic->message: If the topic has been moved, this contains an
 *     explanation and a link.
 *   - $topic->zebra: 'even' or 'odd' string used for row class.
 *   - $topic->comment_count: The number of replies on this topic.
 *   - $topic->new_replies: A flag to indicate whether there are unread
 *     comments.
 *   - $topic->new_url: If there are unread replies, this is a link to them.
 *   - $topic->new_text: Text containing the translated, properly pluralized
 *     count.
 *   - $topic->created: A string representing when the topic was posted. Safe
 *     to output.
 *   - $topic->last_reply: An outputtable string representing when the topic was
 *     last replied to.
 *   - $topic->timestamp: The raw timestamp this topic was posted.
 * - $topic_id: Numeric ID for the current forum topic.
 *
 * @see template_preprocess_forum_topic_list()
 * @see theme_forum_topic_list()
 *
 * @ingroup themeable
 */
?>
<div id="forum-topic-list">
    <?php
    if (count($topics) > 0):
        foreach ($topics as $topic):
            ?>
        <?php $topicLoaded = node_load($topic->nid); ?>
            <div class="forum-topic-on-forum" id="forum-topic-<?php print $topic_id; ?>">
                <div class="forum-title"><?php print $topic->title; ?></div>
                <?php 
                // Determine if a Topic is Archived
                $archived = FALSE;
                $archivedField = field_get_items('node', $topicLoaded, 'field_archived');
                if ($archivedField)
                    $archived = $archivedField[0]['value'];
                
                // If a Topic is Archived, add the special field
                if ($archived): ?>
                <div class="forum-archived">
                    <img src="<?php print drupal_get_path('theme', 'ebmstheme'); ?>/images/checkbox-checked.png" alt="This forum topic is archived."/>
                    Archived
                </div>
                <?php endif; ?>
                <div class="forum-description">
                    <?php
                    $teaser = node_view($topicLoaded, 'teaser');
                    if (array_key_exists('body', $teaser) && array_key_exists(0, $teaser['body']) && array_key_exists('#markup', $teaser['body'][0])) {
                        print $teaser['body'][0]['#markup'];
                    } else {
                        print "<p>No Summary Available</p>";
                    }
                    ?>
                </div>
                <div class="forum-topic-started-info">Started by: <?php print $topicLoaded->name; ?> | <?php print format_date($topicLoaded->created, 'forum_started_by', 'Y'); ?></div>
                <div class="forum-topic-recent-activity">
        <?php if ($topic->comment_count > 0): ?>

                        <div class="forum-topic-recent-activity-words">Recent Activity:</div>
                        <div class="forum-topic-recent-activity-activity">
                            <?php
                            // Broken down structure here compensates for strict warnings.
                            $lastComment = comment_get_recent(1);
                            $lastComment = array_pop($lastComment);
                            $lastComment = comment_load($lastComment->cid);
                            $comment_view = comment_view($lastComment, $topicLoaded);
                            print render($comment_view);
                            ?>
                        </div>
        <?php endif; ?>
                </div>
            </div>
            <?php
        endforeach;

        print $pager;

    else:
        ?>

        <div class="forum-topics-none-found">No Topics Were Found on This Forum</div>

    <?php
    endif;
    ?>
        
</div>