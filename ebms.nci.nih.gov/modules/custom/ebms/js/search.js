/*
 * Dynamic customization of the fieldset legend for topics, so the user
 * can see how many topics she has selected when the fieldset is
 * collapsed.
 */
ebmsscript.count_search_topics = function() {
    var n = jQuery("#topics-boxes input:checked").length;
    if (n > 0)
        jQuery("#topselcnt").text(' \u00A0 (' + n + ' Topics Selected)');
    else
        jQuery("#topselcnt").text('');
};
jQuery(function() {
    // Make the 'only-' checkboxes exclusive (as if they were radios).
    jQuery("#edit-administrator-search .only-box input").click(function() {
        if (jQuery(this).attr("checked")) {
            var boxes = jQuery("#edit-administrator-search .only-box input");
            boxes.attr("checked", "");
            jQuery("#edit-unpublished").attr("checked", "");
            jQuery("#edit-not-listed").attr("checked", "");
            jQuery("#edit-rejected").attr("checked", "");
            jQuery(this).attr("checked", "checked");
        }
    });
    jQuery("#edit-administrator-search .early-box input").click(function() {
        if (jQuery(this).attr("checked")) {
            var boxes = jQuery("#edit-administrator-search .only-box input");
            boxes.attr("checked", "");
        }
    });
});
