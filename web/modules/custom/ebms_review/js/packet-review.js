/**
 * Check the article review disposition checkboxes and dynamically
 * update the form to reflect the current choices. If the user
 * selects the checkbox which means "Don't bother with this article"
 * then we suppress the levels-of-evidence field and display the
 * checkboxes for the reasons the reviewer believes the article
 * should be rejected. We also unselect all of the other disposition
 * checkboxes. If any other disposition checkbox is selected,
 * we uncheck the "does not merit changes" checkbox, hide the
 * rejection reason checkboxes, and show the levels-of-evidence
 * field. The argument merits_changes is a boolean which will
 * be false if the "does not merit changes" checkbox changed
 * value, true if any other disposition checkbox changed value, and
 * will not be passed in at all if we're just supposed to adjust
 * levels-of-evidence and rejection reason fields' visibility
 * based on the current state of the disposition checkboxes.
 *
 * Note that as of jQuery 1.6, attr("checked", boolean) no longer works.
 * Use prop("checked", boolean) instead.
 */
jQuery(document).ready(function () {
  var disposition_check = function(merits_changes) {
    var disposition_boxes = jQuery("#edit-dispositions input");
    var no_change_box = disposition_boxes.first();
    var change_boxes = disposition_boxes.slice(1);
    if (typeof merits_changes === "boolean") {
      if (merits_changes) {
        if (change_boxes.is(":checked")) {
          no_change_box.prop("checked", false);
          jQuery("#reasons-wrapper input").prop("checked", false);
        }
      }
      else if (no_change_box.is(":checked")) {
        change_boxes.prop("checked", false);
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
  disposition_check();
  var disposition_checkboxes = jQuery("#edit-dispositions input");
  disposition_checkboxes.first().change(function() {
    disposition_check(false);
  });
  disposition_checkboxes.slice(1).change(function() {
    disposition_check(true);
  });
});
