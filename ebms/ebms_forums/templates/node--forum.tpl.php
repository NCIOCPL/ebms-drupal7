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

// Determine if a user has access to edit this topic
$administer = user_access('edit forum topic');
    
    // Determine if this topic is archived
    $archived = FALSE;
    $archivedField = field_get_items('node', $node, 'field_archived');
    if ($archivedField)
        $archived = $archivedField[0]['value'];
    
?>
<?php print _ebms_forums_menu_html($forum_tid, $nid); ?>

<div id="forums-right">
    
    <h2>
        <?php
            
            if ($administer):
                drupal_add_js("(function ($) {
                    $(document).ready(function() {
$('#edit-forum-icon').hover(function () {
$(this).children('a').children('img').attr('src', '".drupal_get_path('theme', 'ebmstheme')."/images/EBMS_Edit_Icon_Active.png');
}, function () {
$(this).children('a').children('img').attr('src', '".drupal_get_path('theme', 'ebmstheme')."/images/EBMS_Edit_Icon_Inactive.png');
});                   
}); }) (jQuery);", 'inline');
                ?>
            <span id="edit-forum-icon">
            <a href="<?php print url('/node/'.$nid.'/edit'); ?>">
            <img src="<?php print 
                drupal_get_path('theme', 'ebmstheme');?>/images/EBMS_Edit_Icon_Inactive.png" alt="Edit This Forum Topic"/>
            </a> 
            </span>
        <?php endif;
        print $title;
        
        
        if ($archived) print ' <i>(Archived)</i>';
        ?>
    </h2>
    
    <?php /*if ($archived): ?>
    <div class="forum-archived">
        <img src="<?php print drupal_get_path('theme', 'ebmstheme'); ?>/images/checkbox-checked.png" alt="This forum topic is archived."/>
        Archived
    </div>
    <?php endif; */?>
    
    <div id="forum-topic-first-post">
        <?php print render($elements['body']); ?>
    </div>
    
    <div class="forum-topic-started-info">
        Started by: <?php print $name; ?> | <?php print format_date($created, 'forum_started_by', 'Y'); ?>
    </div>
    
    <br/><br/><br/><br/>
    
    <div id="forum-comments">
        <?php print render($content['comments']); ?>
    </div>
    
</div>