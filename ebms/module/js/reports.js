jQuery(function() {
    jQuery("#edit-import-end").hide();
    jQuery("#edit-use-import-date-range").click(function() {
        if (jQuery(this).is(":checked"))
            jQuery("#edit-import-end").show();
        else
            jQuery("#edit-import-end").hide();
    });
});
