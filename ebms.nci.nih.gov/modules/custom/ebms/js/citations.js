/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */

jQuery(document).ready(function(){
    addHooks();
});

function addHooks() {
    jQuery('input[name=submit_source]').val('');

    // Make the 'which journal' checkboxes exclusive (as if they were radios).
    // Because these don't look like radio buttons, if the user "unchecks"
    // a box, treat that as a request for "ALL JOURNALS."
    // TODO: When the EBMS gets rewritten for Drupal 9, we need to eliminate
    // this silliness of conflating radio buttons with checkboxes.
    jQuery(".which-journals input").click(function() {
        var checked = jQuery(this).attr("checked");
        jQuery(".which-journals input").attr("checked", "");
        if (checked)
            jQuery(this).attr("checked", "checked");
        else
            jQuery("#edit-all-journals-check").attr("checked", "checked");
    });

    // strip any pager query when the search is performed
    jQuery('input#edit-search-button, input#edit-sort-button').click(function () {
        var form = jQuery(this).closest('form');
        if(form.length == 0) return false;
        var button_val = jQuery(this).val();

        // remove the query from the action
        var form_action = form.attr('action');
        form.attr('action', form_action.split('?')[0]);
        jQuery('input[name=submit_source]').val(jQuery(this).val());
        form.submit();
        return false;
    });

    // alter the action to submit the pager URLs when they are clicked
    jQuery('.pager a').click(function () {
        var form = jQuery(this).closest('form');
        if(form.length == 0) return false;
        form.attr('action', this.getAttribute('href'));
        jQuery('input[name=submit_source]').val('Pager');
        form.submit();
        return false;
    });

    // point the filter to just the /citations url with no query
    jQuery('input.filter-button-submit').click(function () {
        var form = jQuery(this).closest('form');
        if(form.length == 0) return false;
        form.attr('action', '/citations');
        return true;
    });

    // test to disable each topic check box
    //jQuery('.citation-cell .topic-checks input.form-checkbox').attr("disabled", "disabled");
    //jQuery('.citation-cell .topic-checks .form-type-checkbox label').fadeTo('fast', 0.5);

    //var count = jQuery('.form-checkbox').length;
    //alert("Found " + count + " checkboxes!");

    //var count = jQuery('.summary-topic-check input.form-checkbox').length;
    //alert("Found " + count + " topic checkboxes!");

    // alter pass/reject buttons to disable their siblings on click
    jQuery('.summary-topic-check input.form-checkbox').click(function () {
        // get the parent div
        var parent = jQuery(this).closest('.summary-topic-check');
        //alert("Found parent " + parent);

        // get the sibling checkbox and label
        var siblings = parent.siblings('.summary-topic-check');
        //alert("Found " + siblings.length + ' siblings!');

        var sib_inputs = siblings.find('input.form-checkbox');
        //alert("Found " + sib_inputs.length + ' sibling checkboxes!');
        var sib_labels = siblings.find('label');

        // based on the current state of this checkbox,
        // either disable the other checkbox or enable all checkboxes
        if(jQuery(this).attr('checked')) {
            // this is set, uncheck the other checkbox
            sib_inputs.attr("checked", "");
        }
    });

    jQuery('.full-citation-radio-check input.form-checkbox').click(function () {
        // get the parent div
        var parent = jQuery(this).closest('.form-type-checkbox');

        // get the sibling checkbox and label
        var sibling = parent.siblings('.form-type-checkbox');
        var sib_input = sibling.children('input.form-checkbox');
        var sib_label = sibling.children('label');

        // based on the current state of this checkbox,
        // either disable the other checkbox or enable all checkboxes
        if(jQuery(this).attr('checked')) {
            // this is set, uncheck the other checkbox
            sib_input.attr("checked", "");
        }

        // prevent turning off of a checked button.
        jQuery(this).attr('checked', 'checked');
    });

    // show/hide the response reject fieldset when the response select is
    // set to certain values
    jQuery('.js-response-select').change(function () {
        var select = jQuery(this);

        var val = select.val();

        if(val == 1) {
            jQuery('.js-response-reject').show('normal');
        }
        else{
            jQuery('.js-response-reject').hide('normal');
        }
    });

    // execute a similar function to above to show rejections if the select
    // is already set
    if(jQuery('.js-response-select').val() == 1){
        jQuery('.js-response-reject').show('normal');
    }

    // bind reposition modal to resize
    jQuery(window).resize(repositionModal);
    jQuery(window).scroll(repositionModal);

    // Fix for IE :hover bug (TIR #2485).
    jQuery("div.article-citation input.form-full-citation-link")
        .mouseover(function() { jQuery(this).addClass("hover"); })
        .mouseleave(function() { jQuery(this).removeClass("hover"); });
}

/**
 * Function to reposition the top of a CTools modal dialog.  Included only to
 * duplicate the functionality of a few in-progress patches not yet applied
 * to the CTools release 7.x-1.2.  An example issue thread can be found at
 * http://drupal.org/node/1691816#comment-6381148
 *
 * Can most likely be removed after the next release of CTools.  (7.x-1.3?)
 */
function repositionModal(){
    // test for the existence of modal content
    var modalContent = jQuery('#modalContent');
    if(modalContent.length == 0)
        return;

    // position code lifted from http://www.quirksmode.org/viewport/compatibility.html
    if (self.pageYOffset) { // all except Explorer
    var wt = self.pageYOffset;
    } else if (document.documentElement && document.documentElement.scrollTop) { // Explorer 6 Strict
      var wt = document.documentElement.scrollTop;
    } else if (document.body) { // all other Explorers
      var wt = document.body.scrollTop;
    }

    // Get our dimensions

    // Get the docHeight and (ugly hack) add 50 pixels to make sure we dont have a *visible* border below our div
    var docHeight = jQuery(document).height() + 50;
    var winHeight = jQuery(window).height();
    if( docHeight < winHeight ) docHeight = winHeight;

    // Create our content div, get the dimensions, and hide it
    var mdcTop = wt + ( winHeight / 2 ) - (  modalContent.outerHeight() / 2);
    modalContent.css({top: mdcTop + 'px'});
}

function delArticleLink(url) {
    jQuery("#confirm-link-deletion").dialog({
        resizable: false,
        height: 200,
        width: 400,
        modal: true,
        buttons: {
            "Remove Link": function() {
                location.href = url;
                jQuery(this).dialog("close");
            },
            "Cancel": function() {
                jQuery(this).dialog("close");
            }
        }
    });
}

function delManagerComment(url) {
    jQuery("#confirm-manager-comment-deletion").dialog({
        resizable: false,
        height: 200,
        width: 400,
        modal: true,
        buttons: {
            "Delete Comment": function() {
                location.href = url;
                jQuery(this).dialog("close");
            },
            "Cancel": function() {
                jQuery(this).dialog("close");
            }
        }
    });
    return false;
}

function delInternalComment(url) {
    jQuery("#confirm-internal-comment-deletion").dialog({
        resizable: false,
        height: 200,
        width: 400,
        modal: true,
        buttons: {
            "Delete Comment": function() {
                location.href = url;
                jQuery(this).dialog("close");
            },
            "Cancel": function() {
                jQuery(this).dialog("close");
            }
        }
    });
    return false;
}
