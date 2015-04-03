<?php

/*
 * Implements hook_html_head_alter.
 */

function ebmstheme_html_head_alter(&$head_elements) {

    // Force the latest IE rendering engine and Google Chrome Frame.
    // Trying out doing this in ebms.module with an HTTP header instead.
    if (false)
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
    $query = pager_get_query_parameters();
    if (isset($query['pager']) && $query['pager'] == 'off') {
        // unset the pager from the existing query parameters to show the pager
        unset($query['pager']);
        unset($query['items_per_page']);
        // Add a link for turning paging on.
        $url = url($_GET['q'], array('query' => $query));
        $items[] = "<a href='$url'>VIEW PAGES</a>";

        // Render the pager items and prefix them with an accessible label.
        $accessible_title = '<h2 class="element-invisible">Pages</h2>';
        $attributes = array('class' => array('pager'));
        $variables = array('items' => $items, 'attributes' => $attributes);
        $pager = theme('item_list', $variables);
        return $accessible_title . $pager;
    }

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
        'pager_previous',
        array(
            'text' => Ebms\LEFT_ARROW,
            'element' => $element,
            'interval' => 1,
            'parameters' => $parameters,
        )
    );
    $li_next = theme(
        'pager_next',
        array(
            'text' => Ebms\RIGHT_ARROW,
            'element' => $element,
            'interval' => 1,
            'parameters' => $parameters,
        )
    );
    $li_first = theme(
        'pager_first',
        array(
            'text' => EBMS\DOUBLE_LEFT_ARROW,
            'element' => $element,
            'parameters' => $parameters,
        )
    );
    $li_last = theme(
        'pager_last',
        array(
            'text' => EBMS\DOUBLE_RIGHT_ARROW,
            'element' => $element,
            'parameters' => $parameters,
        )
    );

    // Add a link for turning paging off.
    $query = pager_get_query_parameters();
    $query['pager'] = 'off';
    $query['items_per_page'] = 'All';
    $url = url($_GET['q'], array('query' => $query));
    $items[] = "<a href='$url'>VIEW ALL</a>";
    $items[] = '|';
    $items[] = 'Page';

    // Add link to jump to the front for search results (TIR 2304/OCEEBMS-68).
    $is_search = strpos($_GET['q'], 'citations/search') !== false;
    if ($is_search && $li_first) {
        $items[] = array(
            'class' => array('pager-first'),
            'data' => $li_first,
        );
    }

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
        } else {
            $class = array('pager-item');
            if ($i < $current_page) {
                $hook = 'pager_previous';
                $variables['interval'] = $current_page - $i;
            } else {
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

    // Add link to jump to the end for search results (TIR 2304/OCEEBMS-68).
    if ($is_search && $li_last) {
        $items[] = array(
            'class' => array('pager-last'),
            'data' => $li_last,
        );
    }

    // Render the pager items and prefix them with an accessible label.
    $accessible_title = '<h2 class="element-invisible">Pages</h2>';
    $attributes = array('class' => array('pager'));
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
        if(is_array($breadcrumb))
        $output[] = implode(' > ', $breadcrumb);
        else
            $output[] = $breadcrumb;
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
    // pdq_ebms_debug('THEME FORM ELEMENT', $variables);
    $element = &$variables['element'];

    // Since these get overridden in there own themed outputs, don't address it here
    if (($element['#type'] == 'checkboxes') || ($element['#type'] == 'radios')) {
        return $element['#children'];
    }

    $attributes = array();
    $variables['#attributes'] = &$attributes;
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

    $output =  array('<div' . drupal_attributes($attributes) . '>' . "\n");

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
            $attributes['class'][] = 'hidden-508';
            $output[] = ' ' . theme('form_element_label', $variables);
            $output[] = ' ' . $prefix . $element['#children'] . $suffix . "\n";
            break;
    }

    // Finish off and return the assembled field output.
    $output[] = "</div>\n";
    return implode($output);
}

function ebmstheme_form_element_label($variables) {
    $element = &$variables['element'];
    // This is also used in the installer, pre-database setup.
    $t = get_t();

    // If title and required marker are both empty, output no label.
    if ((!isset($element['#title']) || $element['#title'] === '') && empty($element['#required'])) {
        return '';
    }

    // If the element is required, a required marker is appended to the label.
    $required = !empty($element['#required']) ? theme('form_required_marker', array('element' => $element)) : '';

    $title = filter_xss_admin($element['#title']);

    $attributes = array();
    if (array_key_exists('#attributes', $variables))
            $attributes = $variables['#attributes'];

    // Style the label as class option to display inline with the element.
    if ($element['#title_display'] == 'after') {
        $attributes['class'][] = 'option';
    }
    // Show label only to screen readers to avoid disruption in visual flows.
    elseif ($element['#title_display'] == 'invisible') {
        $attributes['class'][] = 'element-invisible';
    }
    $attributes['class'][] = 'label-508';

    if (!empty($element['#id'])) {
        $attributes['for'] = $element['#id'];
    }

    // If it's checkboxes or radios, that label is the legend
    // of a fieldset, so output nothing.
    if (($element['#type'] == 'checkboxes') || ($element['#type'] == 'radios')) {
        return '';
    }

    // string describing the label format
    $format = '!title !required';
    if (
            $element['#type'] == 'date_popup' ||
            $element['#type'] == 'date' ||
            $element['#type'] == 'managed_file'
    ) {


        return ' <div' . drupal_attributes($attributes) . '>' .
                $t($format, array('!title' => $title, '!required' => $required)) .
                "</div>\n";
    } elseif ($element['#type'] == 'file') {
        $attributes['class'][] = 'label-508';
        return '<span ' . drupal_attributes($attributes) . '>'
                . $t($format, array('!title' => $title, '!required' => $required))
                . "</span>\n";
    }
    else {
        // The leading whitespace helps visually separate fields from inline labels.

        // 2013-03-08 (TIR 2260): hoist link outside of checkbox label,
        // because IE9 propogates the click event for the link up to the
        // label, with the result that in addition to launching a separate
        // window or tab for display of the full text by the link, the
        // state of the checkbox is altered.  This has to be some of the
        // ugliest code I've ever written, but I don't have any really
        // good options.  Unfortunately, Drupal's checkboxes form field
        // doesn't provide a way to attach much of anything to the individual
        // options in the checkboxes set.  I'm happy to listen to suggestions
        // for alternate approaches.  The module code appends the link
        // to the label.  This code finds it with a regular expression,
        // and pulls it outside of the label.  The ugliest part of this,
        // of course, is the fragile dependency on the exact string we're
        // moving.  It works, though, which is more than I can say about
        // all the other techniques I tried.  I could have used separate
        // checkbox fields instead of a single checkboxes field, pretending
        // the individual article options had nothing to do with each
        // other, but that seemed even more unattractive than this.
        $suffix = '';
        if ($element['#type'] == 'checkbox') {
            $pattern = '@ &nbsp; <a .*>DOWNLOAD FULL TEXT</a>@';
            $rc = preg_match($pattern, $title, $matches);
            if ($rc) {
                $suffix = $matches[0];
                $title = str_replace($suffix, '', $title);
                $suffix = $suffix;
            }
        }
        return '<label' . drupal_attributes($attributes) . '>' .
        $t($format, array('!title' => $title, '!required' => $required)) .
        "</label> $suffix\n";
    }
}

/* Make sure we can pick out the time portion of pair of widgets for
 * date and time.
 */
function ebmstheme_textfield($variables) {
    $parents = $variables['element']['#array_parents'];
    if (in_array('field_datespan', $parents, true)) {
        if (in_array('time', $parents, true)) {
            $variables['element']['#attributes']['class'][] = 'time-subfield';
        }
    }
    return theme_textfield($variables);
}

function ebmstheme_file($variables) {
    // Added by Lauren for 508

    $element = $variables['element'];

    $element['#attributes']['type'] = 'file';
    element_set_attributes($element, array('id', 'name', 'size'));
    _form_set_class($element, array('form-file'));

    // Only needs 508 compliance help when the label is "invisible"
    if ($element['#title_display'] == 'invisible')
        return '<label class="hidden-508" for="' . $element['#id'] . '">' . $element['#title'] . '</label><input' . drupal_attributes($element['#attributes']) . ' />';
    else
        return '<input' . drupal_attributes($element['#attributes']) . ' />';
}

function ebmstheme_radios($variables) {
    // Added by Lauren for 508 compliance
    $required = !empty($variables['element']['#required']) ? theme('form_required_marker', array('element' => $variables['element'])) : '';
    $element = $variables['element'];
    $description = '';
    if (!empty($element['#description']))
        $description = '<div class="description">' . $element['#description'] .
            "</div>\n";
    $attributes = array();
    if (isset($element['#id'])) {
        $attributes['id'] = $element['#id'];
    }
    $attributes['class'][] = 'form-item';
    $attributes['class'][] = 'form-type-radios';
    $attributes['class'][] = 'radios-508';
    if (!empty($element['#attributes']['class'])) {
        $attributes['class'] .= ' ' . implode(' ', $element['#attributes']['class']);
    }
    if (isset($element['#attributes']['title'])) {
        $attributes['title'] = $element['#attributes']['title'];
    }
    $description = array_key_exists('#description', $element) ? $element['#description'] : NULL;

    return '<fieldset ' . drupal_attributes($attributes) . '>
            <legend class="radios-508" for="' . $attributes['id'] . '">'
                . (array_key_exists('#title', $element) ? $element['#title'] : '')
                . ' ' . $required .
            '</legend>'
        . ($description ? '<div class="description">' . $description . '</div>' : '')
            . (!empty($element['#children']) ?
                    ('<div id="'.$attributes['id'].'" class="form-radios">'. $element['#children'].'</div>')
                    : '')
        . '</fieldset>';
}

function ebmstheme_checkboxes($variables) {
    //Added by Lauren for 508 compliance
    $required = !empty($variables['element']['#required']) ? theme('form_required_marker', array('element' => $variables['element'])) : '';
    //$variables['element']['#field_prefix'] = '<fieldset class="checkboxes-508"><legend class="checkboxes-508">' . $required . ' ' . $variables['element']['#title'] . '</legend>';
    //$variables['element']['#field_suffix'] = '</fieldset>';

    $element = $variables['element'];
    $description = '';
    if (!empty($element['#description']))
        $description = '<div class="description">' . $element['#description'] .
            "</div>\n";
    $attributes = array();
    if (isset($element['#id'])) {
        $attributes['id'] = $element['#id'];
    }
    $attributes['class'][] = 'form-item';
    $attributes['class'][] = 'form-type-checkboxes';
    $attributes['class'][] = 'checkboxes-508';
    if (!empty($element['#attributes']['class'])) {
        $attributes['class'] = array_merge($attributes['class'], $element['#attributes']['class']);
    }
    if (isset($element['#attributes']['title'])) {
        $attributes['title'] = $element['#attributes']['title'];
    }
    $description = array_key_exists('#description', $element) ?
        $element['#description'] : NULL;
    return '<fieldset ' . drupal_attributes($attributes) . '>
            <legend class="checkboxes-508" for="' . $attributes['id'] . '">'
                . (array_key_exists('#title', $element) ? $element['#title'] : '')
                . ' ' . $required . '</legend>'
        . ($description ? '<div class="description">' . $description . '</div>' : '')
            . (!empty($element['#children']) ?
                    ('<div id="'.$attributes['id'].'" class="form-checkboxes">'. $element['#children'].'</div>')
                    : '')
        . '</fieldset>';
}

// function ebmstheme_date($variables) {
//     $variables['element']['month']['#weight'] = -0.001;
//     $variables['element']['day']['#weight'] = 0;
//     $variables['element']['year']['#weight'] = 0.001;
//     $variables['element']['#sorted'] = false;
//     return theme_date($variables);
// }

/* function ebmstheme_checkbox($element) { */
/*     pdq_ebms_debug('THEME CHECKBOX', $element); */
/*     _form_set_class($element, array('form-checkbox')); */
/*     $checkbox = '<input '; */
/*     $checkbox .= 'type="checkbox" '; */
/*     $checkbox .= 'name="' . $element['#name'] . '" '; */
/*     $checkbox .= 'id="' . $element['#id'] . '" '; */
/*     $checkbox .= 'value="' . $element['#return_value'] . '" '; */
/*     $checkbox .= $element['#value'] ? ' checked="checked" ' : ' '; */
/*     $checkbox .= drupal_attributes($element['#attributes']) . ' />'; */

/*   if (!is_null($element['#title'])) { */
/*       if ($element['#title_display'] == 'before') */
/*           $checkbox = '<label class="option" for="' . $element['#id'] . */
/*               '">' . $element['#title'] . '</label> ' . $checkbox; */
/*       else */
/*           $checkbox .= ' <label class="option" for="' . $element['#id'] . */
/*               '">' . $element['#title'] . '</label>'; */
/*   } */

/*   unset($element['#title']); */
/*   return theme('form_element', $element, $checkbox); */
/* } */

function ebmstheme_preprocess_node(&$variables) {
    $node = $variables['node'];

    // save if the current viewer is the editor of the event
    $editor = user_access("edit any $node->type content");
    if ($editor) {
        $themepath = drupal_get_path('theme', 'ebmstheme');
        $variables['editIconPath'] =
            url("$themepath/images/EBMS_Edit_Icon_Inactive.png");

        $variables['editNodePath'] =
            url("node/$node->nid/edit");
    }
    $variables['editor'] = $editor;

    $variables['status'] = $node->status;

    $variables['in_preview'] = (isset($node->in_preview) && $node->in_preview);

    if ($node->type == 'ebms_event') {
        preprocess_ebms_event($variables);
    }
}

function preprocess_ebms_event(&$variables) {
    module_load_include('inc', 'ebms', 'EbmsArticle');

    //retrieve the node from the variables
    $node = $variables['node'];

    // save if the current viewer is the editor of the event
    $editor = $variables['editor'];

    // retrieve the needed values for the template
    $eventDate = field_get_items('node', $node, 'field_datespan');
    $variables['eventDate'] = 'unknown';
    $variables['eventTime'] = 'unknown';
    if ($eventDate) {
        $startDate = $eventDate[0]['value'];
        $endDate = $eventDate[0]['value2'];

        // build the date of the event
        $date = date('F j, Y', $startDate);

        // build the time of the event
        // render out separate pieces of the dates
        $startTime = date('g:i', $startDate);
        $endTime = date('g:i', $endDate);

        $startMeridiem = date('A', $startDate);
        $endMeridiem = date('A', $endDate);

        $endTime .= " $endMeridiem";
        if ($startMeridiem != $endMeridiem) {
            $startTime .= " $startMeridiem";
        }

        $time = "$startTime - $endTime E.T.";

        // store date and time
        $variables['startDate'] = $startDate;
        $variables['eventDate'] = $date;
        $variables['eventTime'] = $time;

        // need to determine next and previous events
        $sortedNodes = array();

        // get the nearest previous event
        $prevQuery = new EntityFieldQuery();
        $prevQuery
            ->entityCondition('entity_type', 'node')
            ->entityCondition('bundle', 'ebms_event')
            ->propertyCondition('status', '1')
            ->fieldCondition('field_datespan', 'value', $startDate, '<')
            ->fieldOrderBy('field_datespan', 'value', 'DESC')
            ->entityOrderBy('entity_id')
            ->range(0, 1)
            ->addTag('event_filter');

        $prevResult = $prevQuery->execute();
        if (isset($prevResult['node']))
            $sortedNodes += $prevResult['node'];

        // determine if there are events at the same time,
        // and attempt to get previous and next from those
        $currQuery = new EntityFieldQuery();
        $currQuery
            ->entityCondition('entity_type', 'node')
            ->entityCondition('bundle', 'ebms_event')
            ->propertyCondition('status', '1')
            ->fieldCondition('field_datespan', 'value', $startDate)
            ->entityOrderBy('entity_id')
            ->addTag('event_filter');

        $currResult = $currQuery->execute();
        $nodeAdded = false;
        if (isset($currResult['node'])) {
            // make sure node ends up in list, even if not returned by query

            foreach ($currResult['node'] as $nid => $obj) {
                if ($nid > $node->nid && !$nodeAdded) {
                    $nodeAdded = true;
                    $sortedNodes[$node->nid] = $node;
                }

                $sortedNodes[$nid] = $obj;
            }
        }

        if (!$nodeAdded) {
            $sortedNodes[$node->nid] = $node;
        }

        // get the first following event
        $nextQuery = new EntityFieldQuery();
        $nextQuery
            ->entityCondition('entity_type', 'node')
            ->entityCondition('bundle', 'ebms_event')
            ->propertyCondition('status', '1')
            ->fieldCondition('field_datespan', 'value', $startDate, '>')
            ->fieldOrderBy('field_datespan', 'value')
            ->entityOrderBy('entity_id')
            ->range(0, 1)
            ->addTag('event_filter');

        $nextResult = $nextQuery->execute();
        if (isset($nextResult['node']))
            $sortedNodes += $nextResult['node'];

        $prevNode = null;
        $lastNode = null;
        $nextNode = null;
        foreach ($sortedNodes as $nid => $obj) {
            // if found the current node, keep the previous
            if ($nid == $node->nid)
                $prevNode = $lastNode;

            // if this node follows the current node, keep as next
            if ($lastNode == $node->nid)
                $nextNode = $nid;

            $lastNode = $nid;
        }

        $variables['prevNode'] = $prevNode;
        $variables['nextNode'] = $nextNode;
    }

    $eventStatus = field_get_items('node', $node, 'field_event_status');
    $variables['cancelled'] = ($eventStatus[0]['value'] == 'cancelled');

    $eventType = field_get_items('node', $node, 'field_event_type');
    $variables['eventType'] = 'In Person';
    if ($eventType[0]['value'] != 'in_person')
        $variables['eventType'] = 'Webex/Phone Conference';

    // Collect information about participants. See JIRA::OCEEBMS-8.
    $participants = array();
    $board = field_get_items('node', $node, 'field_boards');
    $variables['boardName'] = null;
    if ($board) {
        $values = array();
        foreach ($board as $data) {
            $value = field_view_value('node', $node, 'field_boards', $data);
            $board = render($value) . ' Board';
            $values[] = $board;
            $query = db_select('users', 'u')->fields('u', array('name'));
            $query->join('ebms_board_member', 'm', 'm.user_id = u.uid');
            $query->condition('u.status', 1);
            $query->condition('m.board_id', $data['value']);
            $query->orderBy('u.name');
            $members = $query->execute()->fetchCol();
            $participants[] = _group_span_with_members($board, $members);
        }
        $variables['boardName'] = implode(', ', $values);
    }

    // Same thing for subgroups.
    $subgroups = field_get_items('node', $node, 'field_subgroups');
    if ($subgroups) {
        foreach ($subgroups as $data) {
            $name = htmlspecialchars(db_select('ebms_subgroup', 's')
                ->fields('s', array('sg_name'))
                ->condition('s.sg_id', $data['value'])
                ->execute()
                ->fetchfield());
            $query = db_select('users', 'u')->fields('u', array('name'));
            $query->join('ebms_subgroup_member', 'm', 'm.user_id = u.uid');
            $query->condition('u.status', 1);
            $query->condition('m.sg_id', $data['value']);
            $query->orderBy('u.name');
            $members = $query->execute()->fetchCol();
            $participants[] = _group_span_with_members($name, $members);
        }
    }

    // Same thing for subgroups.
    $ad_hoc_groups = field_get_items('node', $node, 'field_ad_hoc_groups');
    if ($ad_hoc_groups) {
        foreach ($ad_hoc_groups as $data) {
            $name = htmlspecialchars(db_select('ebms_ad_hoc_group', 'g')
                ->fields('g', array('group_name'))
                ->condition('g.group_id', $data['value'])
                ->execute()
                ->fetchfield());
            $query = db_select('users', 'u')->fields('u', array('name'));
            $query->join('ebms_ad_hoc_group_member', 'm', 'm.user_id = u.uid');
            $query->condition('u.status', 1);
            $query->condition('m.group_id', $data['value']);
            $query->orderBy('u.name');
            $members = $query->execute()->fetchCol();
            $participants[] = _group_span_with_members($name, $members);
        }
    }

    $eventNotes = field_get_items('node', $node, 'field_notes');
    $variables['eventNotes'] = null;
    if ($eventNotes)
        $variables['eventNotes'] = $eventNotes[0]['value'];

    $agenda = field_get_items('node', $node, 'field_agenda');
    $variables['agenda'] = null;
    if ($agenda)
        $variables['agenda'] = $agenda[0]['value'];

    // retrieve the agenda status
    // if not set, either hide the agenda, or display it as draft to users
    // capable of editing events
    $field_agenda_status = field_get_items('node', $node, 'field_agenda_status');
    $agenda_status = 0;
    if($field_agenda_status)
        $agenda_status = $field_agenda_status[0]['value'];
    if (!$editor && !$agenda_status)
        $variables['agenda'] = null;

    $variables['agenda_status'] = $agenda_status ? '' : ' - Draft';

    $submitter = user_load($node->uid);
    $variables['submitter'] = $submitter->name;
    $submitted = $node->created;
    $variables['submitted'] = date('m/d/y', $submitted);

    $individuals = field_get_items('node', $node, 'field_individuals');
    $individualValues = array();

    if ($individuals) {
        foreach ($individuals as $individual) {
            $individualValues[] = $individual['value'];
        }
    }

    $users = user_load_multiple($individualValues);
    $individualNames = array();
    foreach ($users as $individual) {
        $individualNames[] = $individual->name;
        $participants[] = htmlspecialchars($individual->name);
    }
    $variables['individuals'] = implode(', ', $individualNames);
    $variables['participants'] = implode(', ', $participants);

    // build links to the various documents
    $documents = field_get_items('node', $node, 'field_documents');
    $docLinks = array();
    $target = array('attributes' => array('target' => '_blank'));
    if ($documents)
        foreach ($documents as $document) {
            $url = file_create_url($document['uri']);
            $docLinks[] = l($document['filename'], $url, $target);
        }

    $variables['docLinks'] = $docLinks;
}

function _group_span_with_members($group, $members) {
    $members = htmlspecialchars(implode(', ', $members));
    $members = str_replace('"', '&#34', $members);
    return "<span title=\"$members\">$group</span>";
}

/**
 * Overwrite the box that shows a user they have submitted a webform request before.
 *
 * @param type $variables
 * @return string HTML output
 */
function ebmstheme_webform_view_messages($variables) {
    return '';
}