<?php
/**
 * @file
 * Main view template.
 *
 * Variables available:
 * - $classes_array: An array of classes determined in
 *   template_preprocess_views_view(). Default classes are:
 *     .view
 *     .view-[css_name]
 *     .view-id-[view_name]
 *     .view-display-id-[display_name]
 *     .view-dom-id-[dom_id]
 * - $classes: A string version of $classes_array for use in the class attribute
 * - $css_name: A css-safe version of the view name.
 * - $css_class: The user-specified classes names, if any
 * - $header: The view header
 * - $footer: The view footer
 * - $rows: The results of the view query, if any
 * - $empty: The empty text to display if the view is empty
 * - $pager: The pager next/prev links to display, if any
 * - $exposed: Exposed widget form/info to display
 * - $feed_icon: Feed icon to display, if any
 * - $more: A link to view more, if any
 *
 * @ingroup views_templates
 */
?>

<?php
// perform initial page setup
// Set the top nav to Forums
module_load_include('inc', 'ebms', 'common');
Ebms\Menu::$active = 'Forums';

// retrieve the forum title
$title = $view->build_info['substitutions']['%1'];
$tid = $view->build_info['substitutions']['!1'];

$forum = taxonomy_term_load($tid);
drupal_set_breadcrumb(array(
    l('Forums', 'forum'),
    $forum->name,
));

$administer = user_access('administer ebms forums');
$archived = FALSE;
$archivedField = field_get_items('taxonomy_term', $forum, 'field_archived');
if ($archivedField)
    $archived = $archivedField[0]['value'];
?>

<?php print _ebms_forums_menu_html($tid, null, $view->current_display); ?>

<div id="forums-right">


    <h2>
        <?php print render($title_prefix); ?>
        <?php
        if ($administer):
            drupal_add_js("(function ($) {
                    $(document).ready(function() {
$('#edit-forum-icon').hover(function () {
$(this).children('a').children('img').attr('src', '/" . drupal_get_path('theme',
                    'ebmstheme') . "/images/EBMS_Edit_Icon_Active.png');
}, function () {
$(this).children('a').children('img').attr('src', '/" . drupal_get_path('theme',
                    'ebmstheme') . "/images/EBMS_Edit_Icon_Inactive.png');
});                   
}); }) (jQuery);",
                'inline');
            ?>
            <span id="edit-forum-icon">
                <a href="<?php print url('forum/' . $tid . '/edit'); ?>">
                    <img src="/<?php
        print
            drupal_get_path('theme', 'ebmstheme');
            ?>/images/EBMS_Edit_Icon_Inactive.png" alt="Edit This Forum"/>
                </a> 
            </span>
            <?php
        endif;
        print $title;


        if ($archived)
            print ' <i>(Archived)</i>';
        ?>
        <?php print render($title_suffix); ?>
    </h2>

    <?php if ($header): ?>
        <div class="view-header">
            <?php print $header; ?>
        </div>
    <?php endif; ?>

    <?php if ($forum): ?>
        <div id="forum-description">
            <?php print $forum->description; ?>
        </div>
        <p id="forum-members">
            <?php print _ebms_forums_get_forum_members($forum); ?>
        </p>
    <?php endif; ?>

    <div id='forum-topic-list' class="<?php print $classes; ?>">

        <?php if ($exposed): ?>
            <div class="view-filters">
                <?php print $exposed; ?>
            </div>
        <?php endif; ?>

        <?php if ($attachment_before): ?>
            <div class="attachment attachment-before">
                <?php print $attachment_before; ?>
            </div>
        <?php endif; ?>

        <?php if ($rows): ?>
            <div class="view-content">
                <?php print $rows; ?>
            </div>
        <?php elseif ($empty): ?>
            <div class="view-empty">
                <?php print $empty; ?>
            </div>
        <?php endif; ?>

        <?php if ($pager): ?>
            <?php print $pager; ?>
        <?php endif; ?>

        <?php if ($attachment_after): ?>
            <div class="attachment attachment-after">
                <?php print $attachment_after; ?>
            </div>
        <?php endif; ?>

        <?php if ($more): ?>
            <?php print $more; ?>
        <?php endif; ?>

        <?php if ($footer): ?>
            <div class="view-footer">
                <?php print $footer; ?>
            </div>
        <?php endif; ?>

        <?php if ($feed_icon): ?>
            <div class="feed-icon">
                <?php print $feed_icon; ?>
            </div>
        <?php endif; ?>
    </div>

</div><?php /* class view */ ?>