/**
 * Hook in the cropperjs object.
 */
jQuery(document).ready(function () {
  const image = jQuery("#picture-crop-form img");
  if (image) {

    // Create the object for the user to crop the profile picture.
    const cropper = new window.Cropper(image[0], {
      aspectRatio: 1,
      viewMode: 1,
      checkOrientation: false,
      rotatable: false,
      scalable: false,
      movable: false,
      zoomable: false,
      zoomOnTouch: false,
      zoomOnWheel: false,
      autoCropArea: 1,
      crop: function(event) {
        var data = cropper.getData(false);
        console.log("cropper: " + data.width + " pixels square at x=" + data.x + " y=" + data.y);
        jQuery("input[name='x']").val(data.x);
        jQuery("input[name='y']").val(data.y);
        jQuery("input[name='w']").val(data.width);
        jQuery("input[name='h']").val(data.height);
        if (event.detail.width < 135) {
          console.log('the width is ' + data.width);
          jQuery("#edit-submit").prop("disabled", true);
          jQuery("#min-width-message").text('Cropped area is only ' + Math.floor(data.width) + ' pixels square. Minimum required is 135 pixels square.');
        }
        else {
          jQuery("#edit-submit").prop("disabled", false);
          jQuery("#min-width-message").text("");
        }
      }
    });
  }
});
