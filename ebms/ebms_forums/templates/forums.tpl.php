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
//$stored_feed_links = &drupal_static(__FUNCTION__, array());
//kprint_r($stored_feed_links);
if (!$tid):
    $forums = _ebms_forums_for_global_user();
    if (count($forums) > 0):
        $forum = array_pop($forums);
        drupal_goto('forum/'.$forum->tid);
    else: ?>
<?php print _ebms_forums_menu_html(); ?>

<div id="no-forums">
    <h2><?php print drupal_get_title(); ?></h2>
    You do not belong to any forums.</div>
    <?php endif;
else:
    $forum = taxonomy_term_load($tid);
    $administer = user_access('administer ebms forums');
    $archived = FALSE;
$archivedField = field_get_items('taxonomy_term', $forum, 'field_archived');
    if ($archivedField)
        $archived = $archivedField[0]['value'];
?>

<?php print _ebms_forums_menu_html($tid); ?>

<?php if ($forums_defined): ?>

    <div id="forums-right">

        <h2>
            <?php
            
            if ($administer):
                drupal_add_js("(function ($) {
                    $(document).ready(function() {
$('#edit-forum-icon').hover(function () {
$(this).children('a').children('img').attr('src', '/".drupal_get_path('theme', 'ebmstheme')."/images/EBMS_Edit_Icon_Active.png');
}, function () {
$(this).children('a').children('img').attr('src', '/".drupal_get_path('theme', 'ebmstheme')."/images/EBMS_Edit_Icon_Inactive.png');
});                   
}); }) (jQuery);", 'inline');
                ?>
            <span id="edit-forum-icon">
            <a href="<?php print url('/forum/'.$tid.'/edit'); ?>">
            <img src="/<?php print 
                drupal_get_path('theme', 'ebmstheme');?>/images/EBMS_Edit_Icon_Inactive.png" alt="Edit This Forum"/>
            </a> 
            </span>
        <?php endif;
        print drupal_get_title();
        
        
        if ($archived) print ' <i>(Archived)</i>';
        ?>
        </h2>
        
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
