// $Id$

/**
 * Custom Javascript for EBMS.
 * Wrap everything in our own namespace.
 */
ebmsscript = {};

/**
 * State and static variables.
 */
ebmsscript.file_upload_textarea_open = false;
ebmsscript.site_root = "";
ebmsscript.normal_field_css = { "color": "#2d2c28", "font-style": "normal" };
ebmsscript.init_field_css = { "color": "#d3d3d3", "font-style": "italic" };

/**
 * Open the popup form for a forgotten password.
 */
ebmsscript.show_forgot_password_form = function() {
    jQuery("#forgot-password-form-js").dialog("open");
};

/**
 * Clear out the prompt and reset the style for the email field.
 */
ebmsscript.forgot_password_email_field_focus = function() {
    var field = jQuery("#edit-email");
    if (field.val() == "Email Address")
        field.val("");
    field.css(ebmsscript.normal_field_css);
}

/**
 * Put the prompt in the email field if it's empty and apply custom styling.
 */
ebmsscript.forgot_password_email_field_blur = function() {
    var field = jQuery("#edit-email");
    if (!field.val())
        field.val("Email Address");
    if (field.val() == "Email Address")
        field.css(ebmsscript.init_field_css);
    else
        field.css(ebmsscript.normal_field_css);
};

/**
 * Validate the fields and submit the new password form if validation passes.
 */
ebmsscript.submit_new_password_request = function() {
    if (jQuery("#pdq-ebms-password-form input:radio:checked").length != 1) {
        alert("You must indicate which elements have been forgotten.");
        return false;
    }
    var email = jQuery("#pdq-ebms-password-form #edit-email").val();
    if (!email || email == 'Email Address') {
        alert("Email address is required.");
        return false;
    }
    jQuery.ajax({
        url: ebmsscript.site_root + "/account-email-status",
        data: { email: email },
        dataType: "json",
        success: function(result) {
             if (result.status == "valid")
                 jQuery("#pdq-ebms-password-form").submit();
             else
                 alert("Email address not recognized.");
        },
        error: function(a,b,c) { alert("Internal error"); }
    });
    return false;
}

/**
 * Initialization needed for login page's "forgot password" hidden form.
 */
ebmsscript.init_login_form = function() {

    // Optimize away the work of this function if not needed for this page.
    if (jQuery("#forgot-password-form-js").length != 1) {
        return;
    }
    jQuery.ajaxSetup({ cache: false });
    jQuery("#forgot-password-form-js").dialog({
        autoOpen: false,
        width: 420,
        modal: true,
        draggable: true,
        resizable: false,
        title: "CAN'T ACCESS ACCOUNT"
    });
    jQuery("a#forgot-password").attr(
        "href",
        "javascript:ebmsscript.show_forgot_password_form()"
    );
    jQuery("#edit-email").focus(function() {
        ebmsscript.forgot_password_email_field_focus();
    });
    jQuery("#edit-email").blur(function() {
        ebmsscript.forgot_password_email_field_blur();
    });
    ebmsscript.forgot_password_email_field_blur();
    jQuery("#forgot-password-form-js input.form-submit").click(function() {
        return ebmsscript.submit_new_password_request(this);
    });
};

/**
 * Check the article review disposition checkboxes and dynamically
 * update the form to reflect the current choices.  If the user
 * selects the checkbox which means "Don't both with this article"
 * then we suppress the levels-of-evidence field and display the
 * checkboxes for the reasons the reviewer believes the article
 * should be rejected.  We also unselect all of the other disposition
 * checkboxes.  If any other disposition checkbox is selected,
 * we uncheck the "does not merit changes" checkbox, hide the
 * rejection reason checkboxes, and show the levels-of-evidence
 * field.  The argument merits_hanges is a boolean which will
 * be false if the "does not merit changes" checkbox changed
 * value, true if any other disposition checkbox changed value, and
 * will not be passed in at all if we're just supposed to adjust
 * levels-of-evidence and rejection reason fields' visibility
 * based on the current state of the disposition checkboxes.
 */
ebmsscript.disp_check = function(merits_changes) {
    var disp_checkboxes = jQuery("#edit-dispositions input");
    var no_change_box = disp_checkboxes.first()
    var change_boxes = disp_checkboxes.slice(1);
    if (typeof merits_changes === "boolean") {
        if (merits_changes) {
            if (change_boxes.is(":checked")) {
                no_change_box.attr("checked", false);
                jQuery("#reasons-wrapper input").attr("checked", false);
            }
        }
        else if (no_change_box.is(":checked")) {
            change_boxes.attr("checked", false);
        }
    }
    if (change_boxes.is(":checked")) {
        jQuery("#loe-wrapper").show();
    }
    else {
        jQuery("#loe-wrapper").hide();
    }
    if (no_change_box.is(":checked")) {
        jQuery("#reasons-wrapper").show();
    }
    else {
        jQuery("#reasons-wrapper").hide();
    }
};

/**
 * Check the number of characters in the textarea field of the file
 * upload form.  Display the count, and truncate values longer than
 * 315 characters.
 */
ebmsscript.monitor_chars = function() {
    var chars = jQuery("#filenotes").val();
    if (chars.length > 315) {
        chars = chars.slice(0, 315);
        jQuery("#filenotes").val(chars);
    }
    jQuery("#charcount").text(chars.length + "/315");
};

/**
 * Show the rest of the file upload form once the user has selected a
 * file.
 */
ebmsscript.file_chosen = function() {
    if (jQuery("#filepath").length == 1) {
        var p = jQuery("#filepath").val();
        var a = p.split("\\");
        p = a[a.length-1];
        jQuery("#filename").text(p).show();
        jQuery("#choose-file label").css("color", "#2d2c28");
    }
    if (jQuery("#add-notes").length == 1) {
        jQuery("#add-notes label").css("color", "#a90101");
        if (!ebmsscript.file_upload_textarea_open) {
            ebmsscript.file_upload_textarea_open = true;
            jQuery("#charcount").text("0/315").show();
            jQuery("#filenotes").val("").show().focus();
        }
    }
};

ebmsscript.file_unselected = function() {
    jQuery("#charcount").hide();
    jQuery("#filenotes").hide();
}

/**
 * Common cleanup performed when we close the popup file upload form,
 * as well as when we open it up.
 */
ebmsscript.clear_file_upload_form = function() {
    jQuery("#filepath").val("");
    jQuery("#filenotes").val("").hide();
    jQuery("#filename").text("").hide();
    jQuery("#charcount").text("").hide();
};

/**
 * Open the popup file upload form.
 */
ebmsscript.show_reviewer_document_post_form = function() {
    ebmsscript.show_doc_post_form();
};

/**
 * Extracted common code used by document posting form from multiple
 * places.
 */
ebmsscript.show_doc_post_form = function() {
    ebmsscript.clear_file_upload_form();
    jQuery("#file-upload-form-js").dialog("open");
    jQuery("#filepath").focus();
    jQuery("#choose-file label").css("color", "#2d2c28");
    //jQuery("#upload-file").css("color", "#2d2c28");
    jQuery("#add-notes label").css("color", "#2d2c28");
    ebmsscript.file_upload_textarea_open = false;
};

/**
 * Verify that a file has been selected and if so submit the form.
 */
ebmsscript.submit_file = function() {
    if (jQuery("#filepath").val()) {
        jQuery("#file-upload-form-js form").submit();
        return true;
    }
    else {
        alert("No file selected");
        return false;
    }
};

/**
 * Initialization needed for the form used to review a journal article.
 */
ebmsscript.init_literature_review_form = function() {

    // Optimize away the work of this function if not needed for this page.
    if (jQuery("form#member-review").length != 1) {
        return;
    }
    ebmsscript.disp_check();
    var disp_checkboxes = jQuery("#edit-dispositions input");
    disp_checkboxes.first().change(function() {
        ebmsscript.disp_check(false);
    });
    disp_checkboxes.slice(1).change(function() {
        ebmsscript.disp_check(true);
    });
}

/**
 * Initialization needed for reviewer document file upload form.
 */
ebmsscript.init_reviewer_file_upload_form = function() {

    var dialog_id = "file-upload-form-js";

    // Optimize away the work of this function if not needed for this page.
    if (jQuery("#" + dialog_id).length != 1) {
        return;
    }
    if (jQuery("form#reviewer-upload-form").length != 1) {
        return;
    }
    jQuery("#" + dialog_id).dialog({
        autoOpen: false,
        width: 800,
        modal: true,
        draggable: true,
        resizable: false,
        title: "POST DOCUMENT"
    });
    jQuery("a#reviewer-post-button").attr(
        "href",
        "javascript:ebmsscript.show_reviewer_document_post_form()"
    );
    ebmsscript.clear_file_upload_form();
    ebmsscript.register_file_upload_form_event_handlers();
    /* REPLACED BY COMMON CODE IN FUNCTION INVOKED ABOVE ...^
    jQuery("#file-upload-form-js").bind("dialogclose", function(event) {
         ebmsscript.file_upload_textarea_open = false;
         ebmsscript.clear_file_upload_form();
    });
    jQuery("#choose-file #filepath").change(function() {
        ebmsscript.file_chosen();
    });
    jQuery("#choose-file label").click(function() {
        jQuery("#choose-file label").css("color", "#a90101");
        jQuery("#upload-file").css("color", "#2d2c28");
        jQuery("#add-notes label").css("color", "#2d2c28");
        if (!jQuery.browser.msie)
            jQuery("#choose-file #filepath").click();
    });
    jQuery("#upload-file").focus(function() {
        jQuery("#choose-file label").css("color", "#2d2c28");
        jQuery("#upload-file").css("color", "#a90101");
        jQuery("#add-notes label").css("color", "#2d2c28");
    });
    jQuery("#filenotes").focus(function() {
        jQuery("#choose-file label").css("color", "#2d2c28");
        jQuery("#upload-file").css("color", "#2d2c28");
        jQuery("#add-notes label").css("color", "#a90101");
    });
    jQuery("#upload-file").click(function() {
        return ebmsscript.submit_file(dialog_id);
    });
    jQuery("#file-upload-form-js h2").hide();
    ebmsscript.setup_filenote_monitoring();
    */
};

ebmsscript.setup_filenote_monitoring = function() {
    jQuery("#filenotes").change(function() { ebmsscript.monitor_chars(); });
    jQuery("#filenotes").blur(function() {
        ebmsscript.monitor_chars();
        jQuery("#charcount").css("color", "#2d2c28");
        jQuery("#filenotes").css("border-color", "#2d2c28");
    });
    jQuery("#filenotes").focus(function() { 
        jQuery("#charcount").css("color", "#a90101");
        jQuery("#filenotes").css("border-color", "#a90101");
    });
    jQuery("#filenotes").keyup(function() { ebmsscript.monitor_chars(); });
};

/**
 * Hook fancy handling of submenus into our main menu bar.
 */
ebmsscript.init_superfish = function() {
    var selector = "#ebms-menu";
    var ss_opts = { minWidth: 12, maxWidth: 55, extraWidth: 1 };
    var sf_opts = {
        autoArrows: false,
        dropShadows: false,
        animation: { opacity: "show", height: "show" },
        speed: "fast",
        delay: 800
    };
    jQuery(selector).supersubs(ss_opts).superfish(sf_opts);

    // Make sure the submenus are at least as wide as the parent.
    jQuery("ul.ebms-submenu").each(function() {
        var submenu = jQuery(this);
        var parent = submenu.parent()
        var min_width = parent.width() + 1;
        if (min_width > submenu.width())
            submenu.css('width', min_width + "px");
    });
};

/**
 * Confirm marking a packet as inactive.  The URL which will route
 * control to the server function which handles the action is
 * passed in as the only argument.  We store this URL somewhere
 * where the dialog window we open can find it.
 */
ebmsscript.delete_packet = function(packet_delete_url) {
    ebmsscript.packet_delete_url = packet_delete_url;
    jQuery("#confirm-packet-delete").dialog("open");
};

/**
 * Convert the #confirm-packet-delete div into a hidden block waiting
 * in the wings to appear as a confirmation dialog box when the user
 * asks to delete a review packet.  Set it up so that if the user
 * clicks the "Delete" button in the confirmation dialog box we
 * transfer control to the server function which handles the packet
 * deletion (which is actually accomplished by marking the packet
 * as inactive, so we don't lose any reviewer feedback which has
 * aslready been returned).
 */
ebmsscript.init_manager_packets_page = function() {
    if (jQuery("#confirm-packet-delete").length != 1) {
        return;
    }
    jQuery("#confirm-packet-delete").dialog({
        resizable: false,
        autoOpen: false,
        modal: true,
        buttons: {
            "Delete": function() {
                jQuery(this).dialog("close");
                window.location.href = ebmsscript.packet_delete_url;
            },
            "Cancel": function() {
                jQuery(this).dialog("close");
            }
        }
    });
};

/**
 * Give the user some assistance in knowing whether the two copies of the
 * new password she's typing (blind) match or not.
 */
ebmsscript.password_check = function() {
    var pw1 = jQuery("#edit-new").val();
    var pw2 = jQuery("#edit-new2").val();
    if (pw2.length < 1)
        jQuery("#match-check").text("\xa0");
    else if (pw1 == pw2) {
        jQuery("#match-check").text("Passwords Match");
        jQuery("#match-check").css('color', 'inherit');
    }
    else {
        jQuery("#match-check").text("Passwords Don't Match");
        jQuery("#match-check").css('color', '#a90101');
    }
};

/**
 * Initialize client-side scripting support for the EBMS profile pages.
 */
ebmsscript.init_profile_page = function() {

    // This is a separate form from the general profile form.
    if (jQuery("#cropbox").length == 1)
        ebmsscript.show_cropbox();

    // Nothing else to do if we're not on the general profile form page.
    if (jQuery("#profile-form").length != 1)
        return;

    // Set up support for the form to set a new password.
    if (jQuery("#edit-new2").length == 1) {
        var edit_new = jQuery("#edit-new");
        var edit_new2 = jQuery("#edit-new2");
        edit_new.change(function() { ebmsscript.password_check(); });
        edit_new.blur(function() { ebmsscript.password_check(); });
        edit_new.keyup(function() { ebmsscript.password_check(); });
        edit_new2.change(function() { ebmsscript.password_check(); });
        edit_new2.blur(function() { ebmsscript.password_check(); });
        edit_new2.keyup(function() { ebmsscript.password_check(); });
    }

    // Initialize the form for uploaded a new profile picture.
    if (jQuery("td.edit-picture-cell").length > 0) {

        // Let the server side know we've got scripting on the client side.
        jQuery("#js").val(1);
        jQuery("#picture-fields").addClass("with-js");

        // Register the handler to show the rest of the form.
        jQuery("#choose-file #filepath").change(function() {
            ebmsscript.profile_picture_chosen();
        });

        // Different browsers handle event bubbling differently.
        if (!jQuery.browser.msie) {
            jQuery("#choose-file label").click(function() {
                jQuery("#choose-file #filepath").click();
                return false;
            });
        }
    }
}

/**
 * Constructs a hovering dialog window for the form used to
 * crop a new profile picture down to the right shape.
 */
ebmsscript.show_cropbox = function() {

    // Capture the original dimensions of the uploaded image.
    var width = jQuery("input[name='width']").val();
    var height = jQuery("input[name='height']").val();
    var ratio = width / height;

    // Constrain the cropping box size to reasonable dimensions.
    var box_width = width;
    var box_height = height;
    if (box_width > 500) {
        box_width = 500;
        box_height = box_width / ratio;
    }
    if (box_height > 500) {
        box_height = 500;
        box_width = box_height * ratio;
    }

    // Match the dimensions of the dialog window to the cropping box size.
    var dialog_width = box_width * 1 + 35;
    var dialog_height = box_height * 1 + 150;

    // Calculate a reasonable default for the cropping selection.
    var init_dim = (height > width) ? width * .5 : height * .5;
    if (init_dim < 135)
        init_dim = 135;
    var init_x1 = (width - init_dim) / 2;
    var init_y1 = (height - init_dim) / 2;
    var init_x2 = init_x1 * 1 + init_dim;
    var init_y2 = init_y1 * 1 + init_dim;

    // Construct the dialog window object, which opens immediately.
    jQuery("#cropbox").dialog({
        autoOpen: true,
        width: dialog_width,
        height: dialog_height,
        modal: true,
        draggable: true,
        resizable: true,
        title: "CROP PICTURE"
    });

    // Construct the cropping widget.
    jQuery("#cropbox img").Jcrop({
        aspectRatio: 1,
        setSelect: [init_x1, init_y1, init_x2, init_y2],
        minSize: [135, 135],
        boxWidth: box_width,
        boxHeight: box_height,
        onSelect: ebmsscript.capture_crop_coordinates
    });

    // Tweak the horizontal positioning of the submit button.
    var margin_left = dialog_width / 2 - 50;
    jQuery("#cropbox #edit-submit").css("margin-left", margin_left + "px");
};

/**
 * Callback registered with the cropping widget above.  Stores the
 * cropping coordinates in the hidden fields provided on the form
 * for this purpose.
 */
ebmsscript.capture_crop_coordinates = function(c) {
    jQuery("input[name='x']").val(c.x);
    jQuery("input[name='y']").val(c.y);
    jQuery("input[name='w']").val(c.w);
    jQuery("input[name='h']").val(c.h);
}

/**
 * Once the user has specified a profile picture, enable the rest of the
 * form and disable the "Choose Photo" button.
 */
ebmsscript.profile_picture_chosen = function() {
    var p = jQuery("#filepath").val();
    var a = p.split("\\");
    var n = a[a.length-1];
    jQuery("#filename").text(n).show();
    jQuery("#submit-box").show();
    jQuery("#filepath").attr("readonly", true);
    jQuery("#choose-file").addClass("disabled");
    jQuery("#choose-file label").css("color", "#bbb");
    jQuery("#choose-file label").css("border-color", "#bbb");
    jQuery("#choose-file label").unbind("click");
    jQuery("#filepath").click(function() { return false; });
    return false;
};

ebmsscript.show_new_summaries_page_form = function() {
    jQuery("#new-summary-page-form").dialog("open");
    jQuery("#edit-title").blur();
}
ebmsscript.show_new_cg_summary_form = function() {
    jQuery("#new-cg-summary-form").dialog("open");
    jQuery("#edit-title").blur();
    jQuery("#edit-url").blur();
}

ebmsscript.init_summaries_page = function() {
    if (jQuery("#new-summary-page-form").length == 1) {
        jQuery("#new-summary-page-form").dialog({
            autoOpen: false,
            width: 530,
            modal: true,
            draggable: true,
            resizable: false,
            title: "ADD NEW SUBPAGE"
        });
        jQuery("#edit-title").blur(function() {
            ebmsscript.summaries_subpage_title_field_blur();
        });
        jQuery("#edit-title").focus(function() {
            ebmsscript.summaries_subpage_title_field_focus();
        });
        //ebmsscript.summaries_subpage_title_field_blur();
        jQuery("#new-summary-page-form .form-submit").click(function() {
            return ebmsscript.new_summaries_subpage_submit();
        });
    }
    if (jQuery("#post-summary-sd-form").length == 1) {
        jQuery("#post-summary-sd-form").dialog({
            autoOpen: false,
            width: 800,
            modal: true,
            draggable: true,
            resizable: false,
            title: "POST DOCUMENT"
        });
        jQuery("#post-summary-sd-form .form-submit").click(function() {
            return ebmsscript.post_summary_sd_submit();
        });
        jQuery("#post-summary-sd-form #edit-doc").change(function() {
            ebmsscript.doc_picklist_changed();
        });
        ebmsscript.setup_filenote_monitoring();
    }
    if (jQuery("#new-cg-summary-form").length == 1) {
        jQuery("#new-cg-summary-form").dialog({
            autoOpen: false,
            width: 390,
            modal: true,
            draggable: true,
            resizable: false,
            title: "ADD NEW CANCER.GOV LINK"
        });
        jQuery("#new-cg-summary-form .form-submit").click(function() {
            return ebmsscript.new_cg_summary_submit();
        });
        jQuery("#edit-url").blur(function() {
            ebmsscript.field_blur("edit-url", "Link Url");
        });
        jQuery("#edit-url").focus(function() {
            ebmsscript.field_focus("edit-url", "Link Url");
        });
        jQuery("#edit-title").blur(function() {
            ebmsscript.field_blur("edit-title", "Link Title");
        });
        jQuery("#edit-title").focus(function() {
            ebmsscript.field_focus("edit-title", "Link Title");
        });
    }
    if (jQuery("#post-summary-nd-form").length == 1) {
        jQuery("#post-summary-nd-form").dialog({
            autoOpen: false,
            width: 800,
            modal: true,
            draggable: true,
            resizable: false,
            title: "POST DOCUMENT"
        });
        jQuery("#post-summary-nd-form .form-submit").click(function() {
            return ebmsscript.post_summary_nd_submit();
        });
        jQuery("#post-summary-nd-form #edit-doc").change(function() {
            ebmsscript.doc_picklist_changed();
        });
        ebmsscript.setup_filenote_monitoring();
    }

    /* Make sure we don't do this twice for the Literature page. */
    if (jQuery("body.page-summaries").length != 1)
        return;
    var dialog_id = "file-upload-form-js";
    if (jQuery("#" + dialog_id).length == 1) {
        jQuery("#" + dialog_id).dialog({
            autoOpen: false,
            width: 800,
            modal: true,
            draggable: true,
            resizable: false,
            title: "POST DOCUMENT"
        });
        jQuery("#member-docs-block a.button").attr(
            "href",
            "javascript:ebmsscript.show_member_summary_doc_post_form()"
        );
        ebmsscript.clear_file_upload_form();
        ebmsscript.register_file_upload_form_event_handlers();
    }        
};

ebmsscript.register_file_upload_form_event_handlers = function() {
    jQuery("#file-upload-form-js").bind("dialogclose", function(event) {
         ebmsscript.file_upload_textarea_open = false;
         ebmsscript.clear_file_upload_form();
    });
    jQuery("#choose-file #filepath").change(function() {
        ebmsscript.file_chosen();
    });
    jQuery("#choose-file label").click(function() {
        jQuery("#choose-file label").css("color", "#a90101");
        //jQuery("#upload-file").css("color", "#2d2c28");
        jQuery("#add-notes label").css("color", "#2d2c28");
        if (!jQuery.browser.msie) {
            jQuery("#choose-file #filepath").click();
            return false;
        }
    });
    jQuery("#upload-file").focus(function() {
        jQuery("#choose-file label").css("color", "#2d2c28");
        //jQuery("#upload-file").css("color", "#a90101");
        jQuery("#add-notes label").css("color", "#2d2c28");
    });
    jQuery("#filenotes").focus(function() {
        jQuery("#choose-file label").css("color", "#2d2c28");
        //jQuery("#upload-file").css("color", "#2d2c28");
        jQuery("#add-notes label").css("color", "#a90101");
    });
    jQuery("#upload-file").click(function() {
        return ebmsscript.submit_file();
    });
    jQuery("#file-upload-form-js h2").hide();
    ebmsscript.setup_filenote_monitoring();
};

/**
 * Open the popup file upload form for the literature page
 */
ebmsscript.show_member_summary_doc_post_form = function() {
    ebmsscript.show_doc_post_form("file-upload-form-js");
};


ebmsscript.doc_picklist_changed = function() {
    if (ebmsscript.picklist_value_selected("edit-doc"))
        ebmsscript.file_chosen();
    else
        ebmsscript.file_unselected();
};

ebmsscript.picklist_value_selected = function(id) {
    var selector = "#" + id + " option:selected";
    var answer = false;
    jQuery(selector).each(function() {
        if (jQuery(this).val() != 0)
            answer = true;
    });
    return answer;
};

ebmsscript.new_summaries_subpage_submit = function() {
    var title = jQuery.trim(jQuery("#edit-title").val());
    if (!title || title == 'Subpage Title') {
        alert("Title is required for new subpage.");
        return false;
    }
    if (jQuery("#edit-topics option:selected").length < 1) {
        alert("You must associate at least one topic with the new subpage.");
        return false;
    }
    jQuery("#pdq-ebms-new-summary-page-form").submit();
    return true;
};
ebmsscript.new_cg_summary_submit = function() {
    var url = jQuery.trim(jQuery("#edit-url").val());
    if (!url || url == "Link Url") {
        alert("URL field is required.");
        return false;
    }
    var title = jQuery.trim(jQuery("#edit-title").val());
    if (!title || title == "Link Title") {
        alert("Title field is required.");
        return false;
    }
    jQuery("#pdq-ebms-new-cg-summary-form").submit();
    return true;
}    
ebmsscript.post_summary_nd_submit = function() {
    if (ebmsscript.picklist_value_selected("edit-doc")) {
        jQuery("#pdq-ebms-post-summary-sd-form").submit();
        return true;
    }
    alert("No document has been selected.");
    return false;
};

ebmsscript.post_summary_sd_submit = function() {
    if (ebmsscript.picklist_value_selected("edit-doc")) {
        jQuery("#pdq-ebms-post-summary-sd-form").submit();
        return true;
    }
    alert("No document has been selected.");
    return false;
};

ebmsscript.field_blur = function(field_id, prompt) {
    var field = jQuery("#" + field_id);
    if (!field.val())
        field.val(prompt);
    if (field.val() == prompt)
        field.css(ebmsscript.init_field_css);
    else
        field.css(ebmsscript.normal_field_css);
};
ebmsscript.field_focus = function(field_id, prompt) {
    var field = jQuery("#" + field_id);
    if (field.val() == prompt)
        field.val("");
    field.css(ebmsscript.normal_field_css);
}

/**
 * Put the prompt in the Summaries Subpage Title field if it's empty and
 * apply custom styling.
 */
ebmsscript.summaries_subpage_title_field_blur = function() {
    var field = jQuery("#edit-title");
    if (!field.val())
        field.val("Subpage Title");
    if (field.val() == "Subpage Title")
        field.css(ebmsscript.init_field_css);
    else
        field.css(ebmsscript.normal_field_css);
};

/**
 * Clear out the prompt and reset the style for the subpage title field.
 */
ebmsscript.summaries_subpage_title_field_focus = function() {
    var field = jQuery("#edit-title");
    if (field.val() == "Subpage Title")
        field.val("");
    field.css(ebmsscript.normal_field_css);
}

ebmsscript.show_post_summaries_sd_form = function() {
    jQuery("#post-summary-sd-form").dialog("open");
};

ebmsscript.show_post_nci_doc_form = function() {
    jQuery("#post-summary-nd-form").dialog("open");
};

/**
 * Confirm marking a document as inactive.  The URL which will route
 * control to the server function which handles the action is
 * passed in as the only argument.  We store this URL somewhere
 * where the dialog window we open can find it.
 */
ebmsscript.delete_doc = function(doc_delete_url, doc_delete_name) {
    ebmsscript.doc_delete_url = doc_delete_url;
    jQuery("#confirmation-filename").text(doc_delete_name);
    jQuery("#confirm-doc-delete").dialog("open");
};

ebmsscript.init_docs_page = function() {
    if (jQuery("body.page-docs").length != 1) {
        return;
    }
    if (jQuery("#confirm-doc-delete").length == 1) {
        jQuery("#confirm-doc-delete").dialog({
            resizable: false,
            autoOpen: false,
            modal: true,
            buttons: {
                "Yes, Delete": function() {
                    jQuery(this).dialog("close");
                    window.location.href = ebmsscript.doc_delete_url;
                },
                "Cancel": function() {
                    jQuery(this).dialog("close");
                }
            }
        });
    }
    if (jQuery("#choose-file").length == 1) {
        jQuery("#choose-file #filepath").change(function() {
            ebmsscript.file_chosen();
        });
        jQuery("#choose-file span.label-508").click(function() {
            if (!jQuery.browser.msie) {
                jQuery("#choose-file #filepath").click();
                return false;
            }
        });
    }
};

ebmsscript.delete_group = function(group_delete_url, group_delete_name) {
    ebmsscript.group_delete_url = group_delete_url;
    jQuery("#confirmation-groupname").text(group_delete_name);
    jQuery("#confirm-group-delete").dialog("open");
};

ebmsscript.delete_subpage = function(url) {
    if (confirm("Archive summary subpage?"))
        window.location.href = url;
    else
        return false;
}

ebmsscript.delete_summary_link = function(url) {
    if (confirm("Delete Cancer.gov summary link?"))
        window.location.href = url;
    else
        return false;
}

ebmsscript.init_groups_page = function() {
    if (jQuery("body.page-groups").length != 1)
        return;
    if (jQuery("#confirm-group-delete").length == 1) {
        jQuery("#confirm-group-delete").dialog({
            resizable: false,
            autoOpen: false,
            modal: true,
            buttons: {
                "Yes, Delete": function() {
                    jQuery(this).dialog("close");
                    window.location.href = ebmsscript.group_delete_url;
                },
                "Cancel": function() {
                    jQuery(this).dialog("close");
                }
            }
        });
    }
/*
    if (jQuery("#edit-members").length == 1) {
        jQuery("#edit-members").focus(function() {
            jQuery("#edit-members").css("height", "6em");
            jQuery("#member-instructions").show();
        });
        jQuery("#edit-members").blur(function() {
            jQuery("#edit-members").css("height", "1.5em");
            jQuery("#member-instructions").hide();
        });
    }
*/
};

ebmsscript.publish_checkbox = function(box, queue_id, article_state_id) {
    var action = box.checked ? 'set' : 'clear';
    jQuery.ajax({
        url: ebmsscript.site_root + "/publish-checkbox-ajax",
        data: {
            article_state_id: article_state_id,
            queue_id: queue_id,
            action: action
        },
        dataType: "json",
        success: function(result) {
             if (result.error)
                 alert(result.error);
        },
        error: function(a,b,c) { alert("Internal error"); }
    });
    return false;
}

ebmsscript.publish_check_all = function(queue_id) {
    jQuery("td.col-5 input").each(function() {
        jQuery(this).attr("checked", true);
    });
    jQuery.ajax({
        url: ebmsscript.site_root + "/publish-checkbox-ajax",
        data: {
            queue_id: queue_id,
            action: 'check-all'
        },
        dataType: "json",
        success: function(result) {
             if (result.error)
                 alert(result.error);
        },
        error: function(a,b,c) { alert("Internal error"); }
    });
    return false;
}
ebmsscript.publish_clear_all = function(queue_id) {
    jQuery("td.col-5 input").each(function() {
        jQuery(this).attr("checked", false);
    });
    jQuery.ajax({
        url: ebmsscript.site_root + "/publish-checkbox-ajax",
        data: {
            queue_id: queue_id,
            action: 'clear-all'
        },
        dataType: "json",
        success: function(result) {
             if (result.error)
                 alert(result.error);
        },
        error: function(a,b,c) { alert("Internal error"); }
    });
    return false;
}

ebmsscript.clear_publish_box = function(queue_id, article_state_id) {
    jQuery.ajax({
        url: ebmsscript.site_root + "/clear-publish-box",
        data: { article_state_id: article_state_id,
                queue_id: queue_id
        },
        dataType: "json",
        error: function(a,b,c) { alert("Internal error"); }
    });
    return false;
}

/**
 * Common initialization we always want done.
 */
ebmsscript.init = function() {
    var loc = window.location.href;
    if (/ebmsdev/.test(loc))
        ebmsscript.site_root = "/ebmsdev";
    if (/ebmsqa/.test(loc))
        ebmsscript.site_root = "/ebmsqa";
    jQuery(".working").hide().ajaxStart(function() {
        jQuery(this).show();
    }).ajaxStop(function() {
        jQuery(this).hide();
    });
}
/**
 * Initialization housekeeping which can only be performed after we're
 * sure the document has been loaded.
 */
jQuery(function() {
    ebmsscript.init();
    ebmsscript.init_login_form();
    ebmsscript.init_reviewer_file_upload_form();
    ebmsscript.init_literature_review_form();
    ebmsscript.init_superfish();
    ebmsscript.init_manager_packets_page();
    ebmsscript.init_profile_page();
    ebmsscript.init_summaries_page();
    ebmsscript.init_docs_page();
    ebmsscript.init_groups_page();
});
