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

kprint_r($content['comment_form']);
global $user;
module_load_include('inc', 'ebms', 'profile');
$authorPicture = EbmsProfile::get_picture($user);
if ($authorPicture) {
    $authorPicture['picture']['#width'] = 45;
    $authorPicture['picture']['#height'] = 45;
}

// Updates to the comment form
$commentForm = &$content['comment_form'];
$commentForm['author']['#title_display'] = 'invisible';
$commentForm['author']['_author']['#markup'] = '';
$commentForm['author']['_author']['#title_display'] = 'invisible';
//$commentForm['comment_body']['und']['#title_display'] = 'invisible';
kprint_r(get_defined_vars());
?>
<div id="forum-topic-comments"><?php print render($content['comments']); ?></div>
<br/>
<div id="forum-topic-add-comment">
    <div id="forum-topic-add-author-pic"><?php print render($authorPicture); ?></div>
    <div id="forum-topic-add-author-and-form">
    <div id="forum-topic-add-author"><?php print $user->name; ?></div>
    <div id="forum-topic-comment-form"><?php print render($content['comment_form']); ?></div>
    </div>
</div>