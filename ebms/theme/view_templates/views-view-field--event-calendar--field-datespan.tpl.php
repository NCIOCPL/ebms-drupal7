<?php

/**
 * @file
 * This template is used to print a single field in a view.
 *
 * It is not actually used in default Views, as this is registered as a theme
 * function which has better performance. For single overrides, the template is
 * perfectly okay.
 *
 * Variables available:
 * - $view: The view object
 * - $field: The field handler object that can process the input
 * - $row: The raw SQL result that can be used
 * - $output: The processed output that will normally be used.
 *
 * When fetching output from the $row, this construct should be used:
 * $data = $row->{$field->field_alias}
 *
 * The above will guarantee that you'll always get the correct data,
 * regardless of any changes in the aliasing that might happen if
 * the view is modified.
 */
?>
<?php

$showEnd = $view->current_display != 'month';

$value = $row->field_field_datespan[0]['raw']['value'];
$timeStamp = ($value % 3600) ? date('g:iA', $value) : date('gA', $value);

if ($showEnd) {
    $value2 = $row->field_field_datespan[0]['raw']['value2'];
    $timeStamp2 = ($value2 % 3600) ? date('g:iA', $value2) : date('gA', $value2);

    print "<span class='date-display-single'>
        <span class='date-display-start'>$timeStamp</span>
            to <span class='date-display-end'>$timeStamp2</span>
                E.T.</span>";
}
else{
    print "<span class='date-display-single'>
        <span class='date-display-start'>$timeStamp</span> E.T.</span>";
}
?>