/* 
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */

jQuery(document).ready(function(){
    addHooks(); 
});

function addHooks() {
    jQuery('input[name=submit_source]').val('');
    
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
    });
}