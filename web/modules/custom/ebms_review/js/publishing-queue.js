// Prepare the callbacks for the topic "publication" form.
jQuery(document).ready(function () {

  // Drupal's handling of nested fields is broken, so we handle them.
  jQuery('.unpublished-topics input:checkbox').click(function() {
    let checkbox_id = this.getAttribute('id');
    console.log('id is ' + checkbox_id);
    let parsed = /unpublished-topic-(\d+)-(\d+)/.exec(checkbox_id);
    if (!parsed) {
      console.log('We have a wonky field id: ' + checkbox_id);
    }
    else {
      let article_id = parsed[1];
      let article_topic_id = parsed[2];
      let key = article_id + '-' + article_topic_id;
      let checked = jQuery('input[id="' + checkbox_id + '"]:checked').val();
      let field = jQuery('input[name=queued]');
      let queued = JSON.parse(field.val());
      let index = queued.indexOf(key);
      if (checked === '1') {
        console.log('checked is true');
        if (index == -1) {
          queued.push(key);
          console.log('added ' + key + ' to queued list');
        }
        else {
          console.log(key + ' is already included in the queued list');
        }
      }
      else {
        console.log('checkbox is cleared');
        if (index == -1) {
          queued.push(key);
          console.log(key + ' was not in the queued list to start with');
        }
        else {
          queued.splice(index, 1);
          console.log('removed ' + key + ' from the queued list');
        }
      }
      field.val(JSON.stringify(queued)).change();
    }
  });
});
