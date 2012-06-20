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
ebmsscript.site_root = "/ebmsdev";
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
    $email = jQuery("#pdq-ebms-password-form #edit-email").val();
    if (!$email || $email == 'Email Address') {
        alert("Email address is required.");
        return false;
    }
    jQuery.ajax({
        url: ebmsscript.site_root + "/account-email-status",
        data: { email: $email },
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
        width: 400,
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
    var p = jQuery("#filepath").val();
    var a = p.split("\\");
    p = a[a.length-1];
    jQuery("#filename").text(p).show();
    jQuery("#choose-file label").css("color", "#2d2c28");
    if (jQuery("#add-notes").length == 1) {
        jQuery("#add-notes label").css("color", "#a90101");
        if (!ebmsscript.file_upload_textarea_open) {
            ebmsscript.file_upload_textarea_open = true;
            jQuery("#charcount").text("0/315").show();
            jQuery("#filenotes").val("").show().focus();
        }
    }
};

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
    ebmsscript.clear_file_upload_form();
    jQuery("#file-upload-form-js").dialog("open");
    jQuery("#filepath").focus();
    jQuery("#choose-file label").css("color", "#2d2c28");
    jQuery("#upload-file").css("color", "#2d2c28");
    jQuery("#add-notes label").css("color", "#2d2c28");
    ebmsscript.file_upload_textarea_open = false;
};

/**
 * Verify that a file has been selected and if so submit the form.
 */
ebmsscript.submit_file = function() {
    if (jQuery("#filepath").val()) {
        jQuery("#reviewer-upload-form").submit();
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

    // Optimize away the work of this function if not needed for this page.
    if (jQuery("#file-upload-form-js").length != 1) {
        return;
    }
    if (jQuery("form#reviewer-upload-form").length != 1) {
        return;
    }
    jQuery("#file-upload-form-js").dialog({
        autoOpen: false,
        width: 800,
        modal: true,
        draggable: true,
        resizable: false,
        title: "POST DOCUMENT"
    });
    jQuery("#reviewer-post-button a").attr(
        "href",
        "javascript:ebmsscript.show_reviewer_document_post_form()"
    );
    ebmsscript.clear_file_upload_form();
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
    jQuery("#filenotes").change(function() { ebmsscript.monitor_chars(); });
    jQuery("#filenotes").blur(function() { ebmsscript.monitor_chars(); });
    jQuery("#filenotes").keyup(function() { ebmsscript.monitor_chars(); });
    jQuery("#upload-file").click(function() {
        return ebmsscript.submit_file();
    });
    jQuery("#file-upload-form-js h2").hide();
};

/**
 * Hook fancy handling of submenus into our main menu bar.
 */
ebmsscript.init_superfish = function () {
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

/**
 * Initialization housekeeping which can only be performed after we're
 * sure the document has been loaded.
 */
jQuery(function() {
    ebmsscript.init_login_form();
    ebmsscript.init_reviewer_file_upload_form();
    ebmsscript.init_literature_review_form();
    ebmsscript.init_superfish();
    ebmsscript.init_manager_packets_page();
    ebmsscript.init_profile_page();
});