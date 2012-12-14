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
global $user;
kprint_r($user);
kprint_r(get_defined_vars());
?>
<?php print _ebms_forums_menu(); ?>
<div id="forums-right">
<h2><?php print $title;?></h2>
<div id="forum-topic-first-post"><?php print render($elements['body']);?></div>
<div class="forum-topic-started-info">Started by: <?php print $name;?> | <?php print format_date($created, 'forum_started_by', 'Y'); ?></div>
<br/><br/><br/><br/>
<div id="forum-comments">
    <?php print render($content['comments']);?>
</div>
</div>