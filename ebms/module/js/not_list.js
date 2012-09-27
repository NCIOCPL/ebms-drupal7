/* 
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */

//jQuery(document).ready(function () { alert("Hello!"); });

jQuery(document).ready(function () {
    
    // strip any pager query when the search is performed
    jQuery('input#edit-search-button').click(function () {
        form = jQuery(this).closest('form');
        if(form.length == 0) return false;
        form.attr('action', '/citations/not-list');
        form.submit();
        return false;
    })
    
    // alter the action to submit the pager URLs when they are clicked
    jQuery('.pager a').click(function () {
        form = jQuery(this).closest('form');
        if(form.length == 0) return false;
        form.attr('action', this.getAttribute('href'));
        form.submit();
        return false;
    })
});