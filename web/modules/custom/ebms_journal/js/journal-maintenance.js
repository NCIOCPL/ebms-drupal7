// Prepare the callbacks for the journal maintenance form.
jQuery(document).ready(function () {

  // Drupal's handling of nested fields is broken, so we handle them.
  jQuery('.exclusion-checkbox').click(function() {
    let name = this.getAttribute('name');
    console.log('name is ' + name);
    let parsed = /journal-(\d+)-(.+)/.exec(name);
    if (!parsed) {
      console.log('We have a wonky field name: ' + name);
    }
    else {
      let id = parsed[1];
      let original = parsed[2];
      let checked = jQuery('input[name="' + name + '"]:checked').val();
      let field = jQuery('input[name=changes]');
      let changes = JSON.parse(field.val());
      if (checked === '1') {
        console.log('checked is true');
        if (original === 'excluded') {
          delete changes[id];
          console.log('deleting id from array');
        }
        else {
          changes[id] = 'excluded';
          console.log('adding id to array');
        }
      }
      else {
        console.log('checkbox is cleared');
        if (original === 'included') {
          delete changes[id];
          console.log('deleting id from array');
        }
        else {
          changes[id] = 'included';
          console.log('adding id to array');
        }
      }
      field.val(JSON.stringify(changes)).change();
    }
  });
});
