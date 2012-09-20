<?php

/*
 * Implements hook_html_head_alter.
 */
function ebmstheme_html_head_alter(&$head_elements) {

    // Force the latest IE rendering engine and Google Chrome Frame.
    $head_elements['chrome_frame'] = array(
        '#type' => 'html_tag',
        '#tag' => 'meta',
        '#attributes' => array(
            'http-equiv' => 'X-UA-Compatible',
            'content' => 'IE=edge,chrome=1'
        ),
    );
}

function ebmstheme_tablesort_indicator($variables) {
    $char = $variables['style'] == 'asc' ? Ebms\UP_ARROW : Ebms\DOWN_ARROW;
    return '<span class="tablesort-indicator">' . $char . '</span>';
}

/**
 * Custom theming for navigation of multipage query results.  The
 * requirements for the pager are underspecified, but from the design
 * mockups it appears that we should have at least links for moving to
 * two adjacent adjacent pages on either side of the current page,
 * with an additional arrow link for moving to the next or previous
 * page, and a link to suppress paging and show the entire table
 * on a single page.
 */
function ebmstheme_pager($variables) {

    // Don't bother doing anything if there's only one page.
    global $pager_page_array, $pager_total;
    $element = $variables['element'];
    $num_pages = $pager_total[$element];
    if ($num_pages < 2)
        return '';

    // Preparation for the loop.
    $parameters = $variables['parameters'];
    $current_page = $pager_page_array[$element] + 1;
    $pager_first = $current_page - 2;
    $pager_last = $current_page + 2;
    if ($pager_first < 1)
        $pager_first = 1;
    if ($pager_last > $num_pages)
        $pager_last = $num_pages;

    // Create the "next" and "previous" links as appropriate.
    $li_previous = theme(
        'pager_previous', array(
            'text' => Ebms\LEFT_ARROW,
            'element' => $element,
            'interval' => 1,
            'parameters' => $parameters,
        )
    );
    $li_next = theme(
        'pager_next', array(
            'text' => Ebms\RIGHT_ARROW,
            'element' => $element,
            'interval' => 1,
            'parameters' => $parameters,
        )
    );

    // Add a link for turning paging off.
    $query = pager_get_query_parameters();
    $query['pager'] = 'off';
    $url = url($_GET['q'], array('query' => $query));
    $items[] = "<a href='$url'>VIEW ALL</a>";
    $items[] = '|';
    $items[] = 'Page';

    // Add the "previous" link if we're not on the first page.
    if ($li_previous) {
        $items[] = array(
            'class' => array('pager-previous'),
            'data' => $li_previous,
        );
    }

    // Add the links for the adjacent pages, and a current page indicator.
    $variables = array('element' => $element, 'parameters' => $parameters);
    for ($i = $pager_first; $i <= $pager_last; $i++) {
        if ($i == $current_page) {
            $class = array('pager-current');
            $data = $i;
        }
        else {
            $class = array('pager-item');
            if ($i < $current_page) {
                $hook = 'pager_previous';
                $variables['interval']  = $current_page - $i;
            }
            else {
                $hook = 'pager_next';
                $variables['interval'] = $i - $current_page;
            }
            $variables['text'] = $i;
            $data = theme($hook, $variables);
        }
        $items[] = array('class' => $class, 'data' => $data);
    }

    // Add the "next" link if we're not on the last page.
    if ($li_next) {
        $items[] = array(
            'class' => array('pager-next'),
            'data' => $li_next,
        );
    }

    // Render the pager items and prefix them with an accessible label.
    $accessible_title = '<h2 class="element-invisible">Pages</h2>';
    $attributes =  array('class' => array('pager'));
    $variables = array('items' => $items, 'attributes' => $attributes);
    $pager = theme('item_list', $variables);
    return $accessible_title . $pager;
}

/**
 * Override the default Drupal separator character between nav breadcrumbs.
 */
function ebmstheme_breadcrumb($variables) {
    //pdq_ebms_debug('BREADCRUMB CUSTOM THEME', $variables);
    $output = array();
    $breadcrumb = $variables['breadcrumb'];
    if (!empty($breadcrumb)) {
        $output[] = '<h2 class="element-invisible">You are here</h2>';
        $output[] = '<div class="breadcrumb">';
        $output[] = implode(' > ', $breadcrumb);
        $output[] = '</div>';
    }
    return implode($output);
}
/*
function ebms_password($variables) {
    $element = $variables['element'];
    $element['#attributes']['type'] = 'password';
    element_set_attributes($element, array('id', 'name', 'size', 'maxlength'));
    _form_set_class($element, array('form-text'));

    return '<input' . drupal_attributes($element['#attributes']) . ' />';
}
*/
/**
 * Custom theming of form elements to get the description attribute
 * tucked in as a subtitle right under the field's title.
 */
function ebmstheme_form_element($variables) {
    $element = &$variables['element'];

    // Add element id for type 'item'.
    if (isset($element['#markup']) && !empty($element['#id']))
        $attributes['id'] = $element['#id'];

    // Add element's type and name as class to aid with JS/CSS selectors.
    $attributes['class'] = array('form-item');
    if (!empty($element['#type'])) {
        $type = strtr($element['#type'], '_', '-');
        $attributes['class'][] = "form-type-$type";
    }
    if (!empty($element['#name'])) {
        $map = array(' ' => '-', '_' => '-', '[' => '-', ']' => '');
        $name = strtr($element['#name'], $map);
        $attributes['class'][] = "form-item-$name";
    }

    // Add a class for disabled elements to facilitate cross-browser styling.
    if (!empty($element['#attributes']['disabled'])) {
        $attributes['class'][] = 'form-disabled';
    }
    $output = array('<div' . drupal_attributes($attributes) . '>' . "\n");

    // If title is not set, we don't display any label or required marker.
    if (!isset($element['#title']))
        $element['#title_display'] = 'none';

    // Set prefix and suffix.
    $prefix = '';
    $suffix = "\n";
    if (isset($element['#field_prefix']))
        $prefix = '<span class="field-prefix">' . $element['#field_prefix'] .
            "</span>\n";
    if (isset($element['#field_suffix']))
        $suffix = ' <span class="field-suffix">' . $element['#field_suffix'] .
            "</span>\n";

    // Prepare the field description.
    $description = '';
    if (!empty($element['#description']))
        $description = '<div class="description">' . $element['#description'] .
            "</div>\n";

    // Assemble the pieces in the order we want.
    switch ($element['#title_display']) {
        case 'before':
        case 'invisible':
            $output[] = ' ' . theme('form_element_label', $variables);
            $output[] = ' ' . $description;
            $output[] = ' ' . $prefix . $element['#children'] . $suffix . "\n";
            break;
        case 'after':
            $output[] = ' ' . $prefix . $element['#children'] . $suffix . "\n";
            $output[] = ' ' . theme('form_element_label', $variables);
            $output[] = ' ' . $description;
            break;
        case 'none':
        case 'attribute':
            $output[] = ' ' . $prefix . $element['#children'] . $suffix . "\n";
            break;
    }

    // Finish off and return the assembled field output.
    $output[] = "</div>\n";
    return implode($output);
}
