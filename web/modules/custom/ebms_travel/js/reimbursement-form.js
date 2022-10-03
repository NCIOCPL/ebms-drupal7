/**
 * Control display of the amount and mileage transportation expense fields.
 *
 * Drupal's #states mechanism is broken. We need to show the mileage field
 * when the "privately-owned vehicle" expense type has been chosen, and the
 * amount field if any other expense type has been chosen.
 *
 * See https://www.drupal.org/project/drupal/issues/1091852.
 */
function ebms_transportation_expense_type_changed() {
  let count = jQuery('input[name="transportation-expense-count"]').val();
  let pov = jQuery('input[name="pov"]').val();
  for (let i = 1; i <= count; i++) {
    let type = jQuery('select[name="transportation-type-' + i + '"]').find(":selected").val();
    let amount = jQuery('input[name="transportation-amount-' + i + '"]');
    let mileage = jQuery('input[name="transportation-mileage-' + i + '"]');
    amount.parent().hide();
    mileage.parent().hide();
    if (type) {
      if (type == pov) {
        mileage.parent().attr("style", "display: inline-block");
      }
      else {
        amount.parent().attr("style", "display: inline-block");
      }
    }
  }
}

jQuery(document).ajaxComplete(function(event, xhr, settings) {
  if (settings.url.indexOf("travel/reimbursement-request") != -1) {
    ebms_transportation_expense_type_changed();
  }
});

jQuery(document).ready(function() {
  ebms_transportation_expense_type_changed();
});
