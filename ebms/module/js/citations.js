/* 
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */

//jQuery(document).ready(function () { alert("Hello!"); });

jQuery(document).ready(function () {
    
    jQuery('input[name=submit_source]').val('');
    
    // strip any pager query when the search is performed
    jQuery('input#edit-search-button, input#edit-sort-button').click(function () {
        form = jQuery(this).closest('form');
        if(form.length == 0) return false;
        button_val = jQuery(this).val();
        
        // remove the query from the action
        form_action = form.attr('action');
        form.attr('action', form_action.split('?')[0]);
        jQuery('input[name=submit_source]').val(jQuery(this).val());
        form.submit();
        return false;
    })
    
    // alter the action to submit the pager URLs when they are clicked
    jQuery('.pager a').click(function () {
        form = jQuery(this).closest('form');
        if(form.length == 0) return false;
        form.attr('action', this.getAttribute('href'));
        jQuery('input[name=submit_source]').val('Pager');
        form.submit();
        return false;
    })
    
    // test to disable each topic check box
    //jQuery('.citation-cell .topic-checks input.form-checkbox').attr("disabled", "disabled");
    //jQuery('.citation-cell .topic-checks .form-type-checkbox label').fadeTo('fast', 0.5);
    
    // alter pass/reject buttons to disable their siblings on click
    jQuery('.citation-cell .topic-checks input.form-checkbox').click(function () {
        // get the parent div
        parent = jQuery(this).closest('.form-type-checkbox');
        
        // get the sibling checkbox and label
        sibling = parent.siblings('.form-type-checkbox')
        sib_input = sibling.children('input.form-checkbox');
        sib_label = sibling.children('label');
        
        // based on the current state of this checkbox,
        // either disable the other checkbox or enable all checkboxes
        if(jQuery(this).attr('checked')) {
            // this is set, uncheck the other checkbox
            sib_input.attr("checked", "");
        }
    })
});