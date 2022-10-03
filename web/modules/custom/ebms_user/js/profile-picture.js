// Let the server side know we've got scripting on the client side,
// so the user will be able to use the photo cropping tool.
jQuery(document).ready(function () {
  var js = jQuery("#js-flag");
  if (js) {
    js.val(1);
  }
});
