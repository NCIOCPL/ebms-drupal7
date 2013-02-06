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
module_load_include('inc', 'ebms', 'profile');
$authorPicture = EbmsProfile::get_picture($user);
if ($authorPicture) {
    $authorPicture['picture']['#width'] = 45;
    $authorPicture['picture']['#height'] = 45;
}

?>
<div id="forum-topic-comments"><?php print render($content['comments']); ?></div>
<br/>
    <?php
    // Determine if this topic is archived
    $archived = FALSE;
    $archivedField = field_get_items('node', $node, 'field_archived');
    if ($archivedField)
        $archived = $archivedField[0]['value'];
    
    // If it is archived, then comments cannot be added.
    // So hide the comment wrapper if it's archived.
    if (!$archived):
    ?>
<div id="forum-topic-add-comment">
    <div id="forum-topic-add-author-pic"><?php print render($authorPicture); ?></div>
    <div id="forum-topic-add-author-and-form">
    <div id="forum-topic-add-author"><?php print $user->name; ?></div>
    <div id="forum-topic-comment-form"><?php print render($content['comment_form']); ?></div>
    </div>
</div>
<?php
endif;
?>