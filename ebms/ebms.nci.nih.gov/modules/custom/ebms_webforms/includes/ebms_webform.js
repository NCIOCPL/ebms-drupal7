jQuery(document).ready(function ($) {
    var visible = $("#edit-submitted-hotel-2").attr('checked');
    if(!visible){
        $('#webform-component-amount').addClass('hidden');
    }
    $("#edit-submitted-hotel-2").click(function(){
        $('#webform-component-amount').removeClass('hidden');
    });
    $("#edit-submitted-hotel-1").click(function(){
        $('#webform-component-amount').addClass('hidden');
    });
});