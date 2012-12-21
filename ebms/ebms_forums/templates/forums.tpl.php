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
$forum = taxonomy_term_load($tid);
if (!$tid):
    $forums = _ebms_forums_for_global_user();
    if (count($forums) > 0):
        $forum = array_pop($forums);
        drupal_goto('forum/'.$forum->tid);
    else: ?>
<h2><?php print drupal_get_title(); ?></h2>
<div id="no-forums">You do not belong to any forums.</div>
    <?php endif;
else:
?>

<?php print _ebms_forums_menu_html($tid); ?>

<?php if ($forums_defined): ?>

    <div id="forums-right">

        <h2><?php print drupal_get_title(); ?></h2>
        
        <?php if ($forum): ?>
        <div id="forum-description">
            <?php print $forum->description; ?>
        </div>


        <?php print $topics; ?>
        <?php endif; ?>
        
    </div>

<?php
endif;
endif;?>
