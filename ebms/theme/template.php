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
    $query = pager_get_query_parameters();
    if (isset($query['pager']) && $query['pager'] == 'off') {
        // unset the pager from the existing query parameters to show the pager
        unset($query['pager']);
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
        if (isset($currResult['node']))
            $sortedNodes += $currResult['node'];

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

    $eventType = field_get_items('node', $node, 'field_event_type');
    $variables['eventType'] = 'In Person';
    if ($eventType[0]['value'] != 'in_person')
        $variables['eventType'] = 'Webex/Phone Conference';

    $board = field_get_items('node', $node, 'field_boards');
    $variables['boardName'] = null;
    if ($board) {
        $values = array();
        foreach ($board as $data) {
            $value = field_view_value('node', $node, 'field_boards', $data);
            $values[] = render($value);
        }

        $variables['boardName'] = implode(', ', $values);
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

    $inhouseStaff = field_get_items('node', $node, 'field_inhouse_staff');
    $boardMembers = field_get_items('node', $node, 'field_board_members');

    $individuals = array();
    if ($inhouseStaff) {
        foreach ($inhouseStaff as $staff) {
            $individuals[] = $staff['value'];
        }
    }

    if ($boardMembers) {
        foreach ($boardMembers as $member) {
            $individuals[] = $member['value'];
        }
    }

    $users = user_load_multiple($individuals);
    $individualNames = array();
    foreach ($users as $individual) {
        $individualNames[] = $individual->name;
    }
    $variables['individuals'] = implode(', ', $individualNames);

    // build links to the various documents
    $documents = field_get_items('node', $node, 'field_documents');
    $docLinks = array();
    if ($documents)
        foreach ($documents as $document) {
            $docLinks[] = l($document['filename'],
                file_create_url($document['uri']));
        }

    $variables['docLinks'] = $docLinks;
}