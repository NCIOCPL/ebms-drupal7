// Prepare the callbacks for the review queue form.
jQuery(document).ready(function () {

  // Drupal's handling of nested fields is broken, so we handle them.
  jQuery('div.topic-buttons input[type=radio]').click(function () {
    let key = this.getAttribute('name');
    let value = jQuery('input[name="' + key + '"]:checked').val();
    let field = jQuery('input[name=decisions]');
    let decisions = JSON.parse(field.val());
    if (value === '0') {
      delete decisions[key];
    }
    else {
      decisions[key] = value;
    }
    field.val(JSON.stringify(decisions)).change();
  });
});
