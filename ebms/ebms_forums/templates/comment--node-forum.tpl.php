<?php
/**
 * @file
 * Displays a forum.
 *
 * May contain forum containers as well as forum topics.
 *
 * Available variables:
 * - $forums: The forums to display (as processed by forum-list.tpl.php).
 * - $topics: The topics to display (as processed by forum-topic-list.tpl.php).
 * - $forums_defined: A flag to indicate that the forums are configured.
 *
 * @see template_preprocess_forums()
 *
 * @ingroup themeable
 */

$commentAuthor = user_load($comment->uid);

module_load_include('inc', 'ebms', 'profile');
$authorPicture = EbmsProfile::get_picture($commentAuthor);
if ($authorPicture) {
    $authorPicture['picture']['#width'] = 45;
    $authorPicture['picture']['#height'] = 45;
}
?>
<div class="forum-topic-comment-wrapper">
    <div class="forum-topic-commenter-pic">
        <?php if ($authorPicture) print render($authorPicture); ?>
    </div>
    <div class="forum-topic-comment-and-info">
        <div class="forum-topic-comment-info">
            <div class="forum-topic-commenter">

<?php print $commentAuthor->name; ?>
                <?php /*   <div class="forum-topic-commenter-pic">
                  <?php
                  print render ($authorPicture);
                  ?>
                  </div> */ ?>
            </div>

            <div class="forum-topic-comment-date"><?php print format_date($comment->created, 'forum_comment'); ?></div>
        </div>
        <div class="forum-topic-comment">
<?php print render($content['comment_body']); ?>
        </div>
    </div>
    <div style="clear:both;"></div>
</div>