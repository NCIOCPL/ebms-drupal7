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
