<?php

// $Id$

/**
 * @file
 *
 * Implementation of EBMS pages for managing ad-hoc groups.
 */

/**
 * Callback registered for /groups pages.
 */
function pdq_ebms_groups($request=null, $group_id=0) {
    $groups = new EbmsGroups($request, $group_id);
    return $groups->run();
}

/**
 * Wrapper for logic to handle ad-hoc group management pages.  Keeps our
 * functions out of the global namespace with less ugly syntax than the
 * PHP namespace mechanism provides, and lets us pass around the current
 * state information more easily.
 */
class EbmsGroups {
    public
        $user,
        $group_id,
        $parms,
        $expanded;
    public static $url = 'groups';

    /**
     * Passing settings in the URL instead of in $_SESSION in order
     * to avoid dependency on Javascript being enabled on the user's
     * browser.  In addition to the parameter values passed in with
     * the request, we create a separate member of the object to
     * record which groups are currently expanded to show membership.
     */
    public function __construct($request, $group_id) {
        global $user;
        $this->user   = user_load($user->uid);
        $this->request = $request;
        $this->group_id = $group_id;
        $this->parms  = drupal_get_query_parameters();
        $this->expanded = array();
        if (isset($this->parms['expanded']))
            $this->expanded = explode(',', $this->parms['expanded']);
    }

    /**
     * Chooses the implementation corresponding to the submitted
     * request.
     */
    public function run() {
        switch ($this->request) {
            case 'expand':
                return $this->expand_group();
            case 'collapse':
                return $this->collapse_group();
            case 'delete':
                return $this->del_group();
            default:
                return $this->show_groups();
        }
    }

    /**
     * Adjust the object's attributes to reflect the addition of
     * one of the groups to the list of those which have been
     * expanded, then display the group page.
     */
    function expand_group() {
        $this->expanded[] = $this->group_id;
        $this->parms['expanded'] = implode(',', $this->expanded);
        return $this->show_groups();
    }

    /**
     * Adjust the object's attributes to reflect the collapse
     * of one of the expanded groups, then display the group
     * page.
     */
    function collapse_group() {
        foreach ($this->expanded as $i => $v) {
            if ($v == $this->group_id)
                unset($this->expanded[$i]);
        }
        if (count($this->expanded) > 0)
            $this->parms['expanded'] = implode(',', $this->expanded);
        else
            unset($this->parms['expanded']);
        return $this->show_groups();
    }

    /**
     * Drop the table rows associated with the group being deleted,
     * then re-route the request to display the group page.
     */
    function del_group() {
        db_delete('ebms_ad_hoc_group_member')
            ->condition('group_id', $this->group_id)
            ->execute();
        db_delete('ebms_ad_hoc_group')
            ->condition('group_id', $this->group_id)
            ->execute();
        drupal_goto(EbmsGroups::$url, array('query' => $this->parms));
    }

    /**
     * Create the markup for displaying a group which has not
     * been expanded.
     */
    function collapsed_group($id, $name) {
        $options = array(
            'html' => true,
            'query' => $this->parms,
            'attributes' => array('class' => array('collapsed')),
        );
        $label = "<span>$name</span> &nbsp; +";
        $url = EbmsGroups::$url . '/expand/' . $id;
        return l($label, $url, $options);
    }

    /**
     * Display a group which has been expanded, showing the members
     * of the group.
     */
    function expanded_group($id, $name) {
        $options = array(
            'html' => true,
            'query' => $this->parms,
            'attributes' => array('class' => array('expanded')),
        );
        $label = "<span>$name</span> &nbsp; &ndash;";
        $url = EbmsGroups::$url . '/collapse/' . $id;
        $link = l($label, $url, $options);
        $query = db_select('users', 'u');
        $query->join('ebms_ad_hoc_group_member', 'm', 'm.user_id = u.uid');
        $query->fields('u', array('name'));
        $query->condition('m.group_id', $id);
        $query->orderBy('u.name');
        $results = $query->execute();
        $names = array();
        foreach ($results as $row)
            $names[] = htmlspecialchars($row->name);
        $list = theme('item_list', array('items' => $names));
        return $link . $list;
    }

    /**
     * Wraps the form for creating or editing an ad-hoc group in
     * a single-cell table row.
     */
    function group_form($group_id=null) {
        $form = drupal_get_form('pdq_ebms_group_form', $this, $group_id);
        return array(
            'data' => array(
                array(
                    'data' => render($form),
                    'colspan' => 2,
                ),
            ),
            'no_striping' => true,
        );
    }

    /**
     * Displays the ad-hoc group in a table which allows sorting by
     * name in forward or reverse order.
     */
    function show_groups() {

        // This page depends on client-side scripting.
        Ebms\Util::add_js();

        // Override Drupal's default breadcrumbs to match requirements.
        drupal_set_breadcrumb(
            array(
                l('Home', 'home'),
                'Groups',
            )
        );

        // Set up the table headers.
        $options = array(
            'query' => $this->parms,
            'attributes' => array('class' => array('button')),
        );
        $headers = array(
            array(
                'data' => 'Groups',
                'field' => 'g.group_name',
            ),
            array(
                'data' => l('Add New', EbmsGroups::$url . '/add', $options),
                'class' => array('col-2'),
            ),
        );

        // Build the query to find all the ad-hoc groups.
        $query = db_select('ebms_ad_hoc_group', 'g');
        $query = $query->extend('TableSort');
        $query->fields('g', array('group_id', 'group_name'));
        $query->orderByHeader($headers);
        $groups = $query->execute();

        // Collect some values we'll use in the processing loop.
        $divider = '<span class="action-divider">|</span>';
        $opts = array('query' => $this->parms);
        $del_func = 'javascript:ebmsscript.delete_group';
        $main_url = EbmsGroups::$url;

        // If requested, show the form for adding a new group.
        $rows = array();
        if ($this->request == 'add')
            $rows[] = $this->group_form();

        // Fold in the rows for the groups.
        foreach ($groups as $g) {
            $edit_link = l('EDIT', "$main_url/edit/{$g->group_id}", $opts);
            $del_url = url("$main_url/delete/{$g->group_id}", $opts);
            $group_name = htmlspecialchars($g->group_name);
            $del_name = str_replace("'", "\\'", $group_name);
            $del_url = "$del_func('$del_url','$del_name');";
            $del_link = '<a href="' . $del_url . '">DELETE</a>';

            // Type of display depends on whether the group is being edited,
            // and on whether the user has expanded it.
            if ($this->request == 'edit' && $this->group_id == $g->group_id)
                $rows[] = $this->group_form($g->group_id);
            elseif (in_array($g->group_id, $this->expanded))
                $group = $this->expanded_group($g->group_id, $group_name);
            else
                $group = $this->collapsed_group($g->group_id, $group_name);
            $data = array(
                $group,
                array(
                    'data' => "$edit_link $divider $del_link",
                    'class' => array('col-2'),
                ),
            );
            $rows[] = array(
                'data' => $data,
                'no_striping' => true,
            );
        }

        // Assemble and return the table structure, including a hidden
        // popup form to confirm group deletion.
        return array(
            'table' => array(
                '#attributes' => array('id' => 'groups-table'),
                '#theme' => 'table',
                '#rows' => $rows,
                '#header' => $headers,
            ),
            'delete-confirm' => array(
                '#type' => 'container',
                '#attributes' => array(
                    'id' => 'confirm-group-delete',
                    'title' => 'Are you sure you want to delete?',
                ),
                'confirm-message' => array(
                    '#type' => 'markup',
                    '#markup' =>
                    '<p id="confirmation-groupname"></p>',
                ),
            ),
        );
    }
}

/**
 * Callback which builds the form for editing an existing ad-hoc group
 * or creating a new one.
 */
function pdq_ebms_group_form($form, &$form_state, $control, $group_id=0) {

    // Build the list of existing group membership (empty for new group).
    $default_name = '';
    $default_members = array();
    if ($group_id) {
        $default_name = db_select('ebms_ad_hoc_group', 'g')
            ->fields('g', array('group_name'))
            ->condition('g.group_id', $group_id)
            ->execute()
            ->fetchField();
        $results = db_select('ebms_ad_hoc_group_member', 'm')
            ->fields('m', array('user_id'))
            ->condition('m.group_id', $group_id)
            ->execute();
        foreach ($results as $member) {
            $default_members[] = $member->user_id;
        }
    }

    // Build the picklist for eligible members (anyone but the superuser).
    $options = array('query' => $control->parms);
    $results = db_select('users', 'u')
        ->fields('u', array('uid', 'name'))
        ->condition('u.uid', 1, '>')
        ->orderBy('u.name')
        ->execute();
    // $users = array(0 => 'Members');
    $users = array();
    foreach ($results as $u)
        $users[$u->uid] = $u->name;

    // Assemble and return the render array for the form.
    $instr = 'Highlighted names are part of this group; hold down the Ctrl ' .
        'key to select multiple names.';
    if (isset($form_state['values']))
        $query_parms = $form_state['values']['query-parms'];
    else
        $query_parms = drupal_json_encode($control->parms);
    return array(
        '#id' => 'group-form',
        'group' => array(
            '#type' => 'hidden',
            '#value' => $group_id,
        ),
        'query-parms' => array(
            '#type' => 'hidden',
            '#value' => $query_parms ? $query_parms : '',
        ),
        'name' => array(
            '#type' => 'textfield',
            '#title' => 'Name',
            '#title_display' => 'none',
            '#default_value' => $default_name,
        ),
        'members' => array(
            '#type' => 'select',
            '#options' => $users,
            '#multiple' => true,
            '#size' => 6,
            '#default_value' => $default_members,
            '#title' => 'members',
            '#title_display' => 'none',
        ),
        'instructions' => array(
            '#type' => 'markup',
            '#markup' => '<p id="member-instructions">' . $instr . '</p>',
        ),
        'buttons' => array(
            '#type' => 'container',
            '#attributes' => array(
                'id' => 'group-form-buttons',
            ),
            'save' => array(
                '#type' => 'submit',
                '#value' => $group_id ? 'Save Changes' : 'Save',
                '#suffix' => ' | '
            ),
            'cancel' => array(
                '#type' => 'markup',
                '#markup' => l('CANCEL', EbmsGroups::$url, $options),
            ),
        ),
    );
}

/**
 * Validation callback for ad-hoc group form.  Make sure we have a group
 * name and at least one member.
 */
function pdq_ebms_group_form_validate($form, &$form_state) {
    $group_id = trim($form_state['values']['group']);
    $name = trim($form_state['values']['name']);
    $members = $form_state['values']['members'];
    if (!$name)
        form_set_error('name', 'You must provide a name for the group.');
    $existing = db_select('ebms_ad_hoc_group', 'g')
        ->fields('g', array('group_id'))
        ->condition('g.group_name', $name)
        ->condition('g.group_id', $group_id, '!=')
        ->execute()
        ->fetchField();
    if ($existing)
        form_set_error('name', 'Name must be unique.');
    $selected_members = array();
    foreach ($members as $id => $val) {
        if ($val)
            $selected_members[] = $id;
    }
    if (count($selected_members) < 1)
        form_set_error('topics',
            'At least one group member must be selected.');
    $form_state['selected members'] = $selected_members;
}

/**
 * Store the new or edited group.
 */
function pdq_ebms_group_form_submit($form, &$form_state) {
    global $user;
    $group_id = trim($form_state['values']['group']);
    $name = trim($form_state['values']['name']);
    $members = $form_state['selected members'];
    if ($group_id) {
        db_update('ebms_ad_hoc_group')
            ->fields(array('group_name' => $name))
            ->condition('group_id', $group_id)
            ->execute();

        // These will be re-populated.
        db_delete('ebms_ad_hoc_group_member')
            ->condition('group_id', $group_id)
            ->execute();
    }
    else {
        $group_id = db_insert('ebms_ad_hoc_group')
            ->fields(array('group_name' => $name, 'created_by' => $user->uid))
            ->execute();
    }

    // Add a row for each group member.
    foreach ($members as $user_id) {
        db_insert('ebms_ad_hoc_group_member')
            ->fields(
                array(
                    'group_id' => $group_id,
                    'user_id' => $user_id,
                )
            )
            ->execute();
    }

    // Re-route back to the group display page, preserving settings.
    $parms = drupal_json_decode($form_state['values']['query-parms']);
    drupal_goto(EbmsGroups::$url, array('query' => $parms));
}
