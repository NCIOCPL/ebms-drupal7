<?php

/**
 * Support routines for customizing the calendar views.
 *
 * Don't bother looking here for the code that creates the page for a
 * calendar display. That's tucked away in the EBMS theme (see, for
 * example, view_templates/views-view--event-calendar--page.tpl.php.
 * (Isn't Drupal tricky?)
 */

/**
 * Parameters for controlling visibility of calendar events.
 */
class EbmsCalendarFiltering {
    public static $filtering = null;
    public
        $boards,
        $meetings;
    public function __construct() {
        $parms = drupal_get_query_parameters();
        $this->boards = $this->boards_default = 'mine';
        $this->meetings = $this->meetings_default = 'all';
        if (!empty($parms['boards']))
            $this->boards = $parms['boards'];
        if (!empty($parms['meetings']))
            $this->meetings = $parms['meetings'];
        $this->path = current_path();
    }

    // Create checkbox-like link for choosing a filtering option.
    public function option($option, $value, $label='') {
        $boards = $this->boards;
        $meetings = $this->meetings;
        $img = Ebms\CB_CHECKED;
        if ($option == 'boards' && $value != $boards) {
            $boards = $value;
            $img = Ebms\CB_UNCHECKED;
        }
        if ($option == 'meetings' && $value != $meetings) {
            $meetings = $value;
            $img = Ebms\CB_UNCHECKED;
        }
        $opts = $this->opts($boards, $meetings);
        $url = url($this->path, $opts);
        $label = $label ? $label : "$value $option";
        $label = strtoupper($label);
        $html[] = '<li class="cal-event-filter-box">';
        $html[] = "<a href='$url'><img src='$img' alt=''> ";
        $html[] = "<span>$label</span></a></li>";
        return implode('', $html);
    }

    // Create a box for the calendar left-nav.
    public function box($title, $items) {
        $html[] = '<div class="nav-block">';
        $html[] = "<h3>$title</h3>";
        $html[] = '<div class="actions">';
        $html[] = '<ul>';
        $html = array_merge($html, $items);
        $html[] = '</ul>';
        $html[] = '</div>';
        $html[] = '<div class="footer"></div>';
        $html[] = '</div>';
        return implode('', $html);
    }

    // Assemble query parameters for calendar URL.
    public function opts($boards='', $meetings='') {
        $board = $boards ? $boards : $this->boards;
        $meetings = $meetings ? $meetings : $this->meetings;
        $opts = array();
        if ($boards != $this->boards_default)
            $opts['query']['boards'] = $boards;
        if ($meetings != $this->meetings_default)
            $opts['query']['meetings'] = $meetings;
        return $opts;
    }

    // Create a link to another calendar event.
    public function event_nav_link($label, $nid) {
        if (!isset($nid))
            return '';
        return l($label, "node/$nid", $this->opts());
    }
}

// We only need one of these for the entire process.
EbmsCalendarFiltering::$filtering = new EbmsCalendarFiltering;

/**
 * Assemble the left column for calendar and event pages.
 */
function ebms_calendar_left_nav($span = null, $args = array()) {

    $filtering = EbmsCalendarFiltering::$filtering;
    $opts = $filtering->opts();
    $output = array();

    $date = isset($args[0]) ? $args[0] : null;
    if ($span) {

        // This might not be the best place to do this, but it's the
        // most straightforward place.  OCEEBMS-41.
        drupal_set_breadcrumb(
            array(
                l('Calendar', 'calendar', $opts),
                $span,
            )
        );
    }

    // create the links to month, week, and day views
    $output[] = '<div id="calendar-views">';
    $output[] = '<div class="item-list">';
    if ($span == 'month')
        $output[] = '<ul><li class="active first">';
    else
        $output[] = '<ul><li class="inactive first">';
    $output[] = l('Month', 'calendar', $opts);
    if ($span == 'week')
        $output[] = '</li><li class="active">';
    else
        $output[] = '</li><li class="inactive">';
    $output[] = l('Week', 'calendar/week', $opts);
    if ($span == 'day')
        $output[] = '</li><li class="active last">';
    else
        $output[] = '</li><li class="inactive last">';
    $output[] = l('Day', 'calendar/day', $opts);
    $output[] = '</li></ul>';
    $output[] = '</div>';
    $output[] = '</div>';

    // build any actions available to the user
    $actions = array();

    if (user_access('create ebms_event content')) {
        // create event on shown day if possible
        if ($span == 'day')
            $opts['query']['date'] = $date;
        $link = l('Create Event', 'node/add/ebms-event', $opts);
        $actions[] = "<li>$link</li>";
        $output[] = $filtering->box('Calendar Actions', $actions);
    }

    if (user_access('view all events')) {
        global $user;
        $s = count($user->boards) > 1 ? 's' : '';
        $options = array(
            $filtering->option('boards', 'mine', "My Board$s"),
            $filtering->option('boards', 'all'),
        );
        $output[] = $filtering->box('Board Filtering', $options);
        $options = array(
            $filtering->option('meetings', 'board'),
            $filtering->option('meetings', 'all'),
        );
        $output[] = $filtering->box('Event Categories', $options);
    }

    // retrieve upcoming events view
    $view = views_get_view('event_calendar');
    if ($view) {
        $display = $view->preview('block_upcoming');
        $output[] = $display;
    }

    return implode('', $output);
}

/**
 * Calendar helper function for altering a Views query.
 *
 * @param type $view  The view object about to be processed.
 * @param type $query An object describing the query.
 */
function _calendar_query_alter(&$view, &$query) {
    global $user;
    $filtering = EbmsCalendarFiltering::$filtering;
    if ($filtering->meetings == 'board') {
        $join = new views_join;
        $table = 'field_data_field_event_category';
        $join->construct($table, 'node', 'nid', 'entity_id');
        $alias = $query->add_relationship('ec', $join, $table);
        $field = "$alias.field_event_category_value";
        $query->add_where('category', $field, 'board');
    }
    if (user_access('administer ebms') || user_access('view all events')) {
        if ($filtering->boards == 'all')
            return;
        if ($view->current_display == 'ical_event_feed')
            return;
    }

    // Don't show board members article publication messages (TIR 2515).
    if ($view->base_table == 'message') {
        if (in_array('board member', $user->roles)) {
            $query->add_where(0, 'message.type',
                'activity_new_articles', '<>');
        }
        else
            $query->add_where(0, 'message.type', 'activity_new_packet', '<>');
    }

    //Initialize an OR conditional for selects
    $or_conditions = db_or();
    $nullConditions = db_and();

    //Boards:

    // Add table to query
    $boardsjoin = new views_join();
    $boardsjoin->construct("field_data_field_boards", $view->base_table,
        $view->base_field, 'entity_id',
        array(
        array('field' => 'entity_type', 'value' => $view->base_table),
        array('field' => 'deleted', 'value' => 0, 'numeric' => TRUE),
    ));
    $query->table_queue[$view->base_table . '__field_data_field_boards'] = array(
        'table' => 'field_data_field_boards',
        'num' => 1,
        'alias' => 'field_data_field_boards',
        'join' => $boardsjoin,
        'relationship' => 'node'
    );

    //Add where condition
    if($user->boards)
        $or_conditions->condition('field_data_field_boards.field_boards_value',
            $user->boards, 'IN');
    $nullConditions->isNull('field_data_field_boards.field_boards_value');

    //Subgroups:

    //Add table to query
    $subgroupsjoin = new views_join();
    $subgroupsjoin->construct("field_data_field_subgroups",
        $view->base_table, $view->base_field, 'entity_id',
        array(
        array('field' => 'entity_type', 'value' => $view->base_table),
        array('field' => 'deleted', 'value' => 0, 'numeric' => TRUE),
    ));
    $query->table_queue[$view->base_table . '__field_data_field_subgroups'] = array(
        'table' => 'field_data_field_subgroups',
        'num' => 1,
        'alias' => 'field_data_field_subgroups',
        'join' => $subgroupsjoin,
        'relationship' => 'node'
    );

    //Add where condition
    if($user->subgroups)
        $or_conditions->condition('field_data_field_subgroups.field_subgroups_value',
            $user->subgroups, 'IN');
    $nullConditions->isNull('field_data_field_subgroups.field_subgroups_value');

    //Ad hoc groups:

    //Add table to query
    $adhocgroupsjoin = new views_join();
    $adhocgroupsjoin->construct("field_data_field_ad_hoc_groups",
        $view->base_table, $view->base_field, 'entity_id',
        array(
        array('field' => 'entity_type', 'value' => $view->base_table),
        array('field' => 'deleted', 'value' => 0, 'numeric' => TRUE),
    ));
    $query->table_queue[$view->base_table . '__field_data_field_ad_hoc_groups'] = array(
        'table' => 'field_data_field_ad_hoc_groups',
        'num' => 1,
        'alias' => 'field_data_field_ad_hoc_groups',
        'join' => $adhocgroupsjoin,
        'relationship' => 'node'
    );

    //Add where condition
    if($user->ebms_ad_hoc_groups)
        $or_conditions->condition('field_data_field_ad_hoc_groups.field_ad_hoc_groups_value',
            $user->ebms_ad_hoc_groups, 'IN');
    $nullConditions->isNull('field_data_field_ad_hoc_groups.field_ad_hoc_groups_value');

    //Individual Users:
    //Add table to query
    $userjoin = new views_join();
    $userjoin->construct("field_data_field_individuals", $view->base_table,
        $view->base_field, 'entity_id',
        array(
        array('field' => 'entity_type', 'value' => $view->base_table),
        array('field' => 'deleted', 'value' => 0, 'numeric' => TRUE),
    ));
    $query->table_queue[$view->base_table . '__field_data_field_individuals'] = array(
        'table' => 'field_data_field_individuals',
        'num' => 1,
        'alias' => 'field_data_field_individuals',
        'join' => $userjoin,
        'relationship' => 'node'
    );

    //Add where condition
    $or_conditions->condition('field_data_field_individuals.field_individuals_value',
        array($user->uid), 'IN'); //Note the use of array here.  IN must have an array
    $nullConditions->isNull('field_data_field_individuals.field_individuals_value');

    // or together both conditions (at least one filter match or all filters empty)
    $or_conditions->condition($nullConditions);

    $query->add_where('membership', $or_conditions);
}

/**
 * Callback for editing an event node from the calendar.
 *
 * @param string $nid Node id of the event
 */
function ebms_calendar_edit_event($nid) {
    $node = node_load($nid);

    if (!$node || $node->type != 'ebms_event') {
        return MENU_NOT_FOUND;
    }

    $form = array();
    $leftNav = ebms_calendar_left_nav();
    $form['left_nav']['#markup'] = "<div id='left-nav'>$leftNav</div>";
    $form['event'] = drupal_get_form('ebms_calendar_event_form', $node);
    $form['event']['#prefix'] = "<div id='calendar-enclosure'>";
    $form['event']['#suffix'] = "</div>";

    return $form;
}

/**
 * Local implementation of hook_form_alter.  Adds a default date to the event's
 * date fields.
 *
 * @param array $form       The existing form to alter.
 * @param array $form_state The form's state.
 * @return array $form      The updated form.
 */
function calendar_node_form_alter(&$form, &$form_state) {
    drupal_set_breadcrumb(
        array(
            l('Calendar', 'calendar'),
            'Create Event',
        )
    );

    // Date module ignores #datepicker_options when the type is date_combo
    // (see https://www.drupal.org/node/2324567). So we have to sneak in
    // a patch to the options on the run.
    // Be careful to set the option only once per field (OCEEBMS-336).
    $js = '
jQuery(function() {
    var start = jQuery("#edit-field-datespan-und-0-value-datepicker-popup-0");
    var end = jQuery("#edit-field-datespan-und-0-value2-datepicker-popup-0");
    start.focus(function() {
        if (jQuery(this).datepicker("option", "changeMonth"))
            jQuery(this).datepicker("option", "changeMonth", false)
                        .datepicker("option", "changeYear", false);
    });
    end.focus(function() {
        if (jQuery(this).datepicker("option", "changeMonth"))
            jQuery(this).datepicker("option", "changeMonth", false)
                        .datepicker("option", "changeYear", false);
    });
});';
    drupal_add_js($js, 'inline');
    $params = drupal_get_query_parameters();
    if (isset($params['date'])) {
        $date = $params['date'];

        $dateTime = "$date 12:00:00";

        $date = new DateTime($dateTime);
        $timestamp = $date->getTimestamp();

        $form['field_datespan']['und'][0]['#default_value']['value'] = $timestamp;
        $form['field_datespan']['und'][0]['#default_value']['value2'] = $timestamp;
    }

    $form['#validate'][] = 'ebms_calendar_event_validate';
    $form['actions']['submit']['#submit'][] = 'ebms_redirect_handler';

    $form['revision_information']['#access'] = false;
    $form['author']['#weight'] = 95;
    $form['options']['#weight'] = 90;
    $form['options']['promote']['#access'] = false;
    $form['options']['sticky']['#access'] = false;

    return $form;
}

function calendar_event_board_select_js($form, &$form_state) {
    $form_state['rebuild'] = true;

    return $form;
}

/**
 * Package all of the files for a meeting into a zip archive and send it
 * to the user.
 *
 * ASSUMPTIONS:
 *   Software has already checked to be sure there are files that should
 *   be sent to the user.  It has only put up a link to this function if
 *   there are.
 *
 *  @param int $meetingNode     Identifier for the meeting record.
 *
 *  @return                     No return if successful.  Calls common.inc
 *                              download_file() which sends the zip file and
 *                              exits.
 *                              Returns message in the event of error.
 */
function pdq_ebms_download_meeting_docs($meetingNode) {

    // Get the agenda string and doc links from the meeting records.
    $node = node_load($meetingNode);

    // Extract info from the node
    $dateData   = field_get_items('node', $node, 'field_datespan');
    $docData    = field_get_items('node', $node, 'field_documents');
    $agendaData = field_get_items('node', $node, 'field_agenda');

    // Date info
    if ($dateData) {
        // Timezone established elsewhere
        $timestamp = $dateData[0]['value'];
        $eventDate = date('Y-m-j', $timestamp);
    }
    else
        $eventDate = '';

    // Extract document file names from meeting docs uris.
    $meetingFiles = array();
    if ($docData) {
        foreach ($docData as $ddata) {
            // Get the filename after the uri scheme:
            $uri = $ddata['uri'];
            if (substr($uri, 0, 9) == 'public://') {
                $meetingFiles[] = substr($uri, 9);
            }
        }
    }

    // Extract document file names from the agenda
    $agendaFiles = array();
    if ($agendaData) {
        $agenda = $agendaData[0]['value'];
        $agendaFiles = extract_file_hrefs($agenda);
    }

    // Combine and deduplicate
    // XXX This does not deduplicate pairs of identical files, only pairs
    // XXX of identical filenames.
    $allFiles = array_unique(array_merge($agendaFiles, $meetingFiles));

    // Sanity check
    if (count($allFiles) < 1) {
        watchdog('pdq_ebms_download_meeting_docs',
                 "No files to download for meeting with node id=$meetingNode");
        return "Error, could not locate any files to download!";
    }

    // Create useful human readable names for zip output, no pesky spaces
    $title = preg_replace('/\s+/', '_', $node->title);
    $readablePath = $title . '_' . $eventDate . '_Docs';
    $readableName = $readablePath . '.zip';

    // Create a temporary zip archive
    $zipName = tempnam(sys_get_temp_dir(), 'zipDocs');
    $rc = make_zip_archive($allFiles, $zipName, $readablePath);
    if (!$rc)
        return "Error, zip operation failed!";

    // Send the file and delete the zip
    Ebms\Util::download_file($zipName, $readableName, 'application/zip', true);

    // Should have exited.  If we got here report the error
    return "Error, could not download zip file!";
}

/**
 * Get names of files identifying EBMS managed files from a references in
 * an HTML string, e.g. an agenda.
 *
 * Filenames found are not checked to see if the file actually exists.
 * Caller must check if needed.
 *
 *  @param string $content  HTML string of the agenda.
 *
 *  @return                 Array of filenames in the files directory.
 *                          Empty array if none found.
 */
function extract_file_hrefs($content) {

    // Return results here
    $filenames = array();

    // Make sure we've got something to parse.
    if (!$content)
        return $filenames;

    // Parse the doc
    $doc = new DOMDocument();
    $doc->loadHTML($content);

    // Get all of the links
    $nodes = $doc->getElementsByTagName('a');
    foreach ($nodes as $node) {
        $href = $node->getAttribute('href');

        // Search for hrefs pointing in our managed files directory
        if (preg_match('@nci\.nih\.gov/files/(.*)@', $href, $matches))
            $filenames[] = urldecode($matches[1]);
    }
    return $filenames;
}

/**
 * Put all of the files named in a passed array into a zip archive.
 *
 * ASSUMPTION:
 *   All files are presumed to be in the public files directory.
 *
 *  @param array $fileNames     Array of public file uris.
 *  @param string $zipName      Output archive name.  Will overwrite if it
 *                              exists.
 *  @param string $outputPath   If non-null, output the zip files as being
 *                              in this path.
 *
 *  @return                     True on success.
 *                              False if operations failed.  See watchdog.
 */
function make_zip_archive($fileNames, $zipName, $outputPath=null) {

    // Don't do anything if there's nothing to do
    if (count($fileNames) < 1)
        return false;

    // Create the zip file, overwriting anything there
    $zip = new ZipArchive();
    $rc = $zip->open($zipName, ZipArchive::CREATE);
    if ($rc !== true) {
        watchdog('make_zip_archive', "Error $rc creating $zipArchive", null,
                 WATCHDOG_ERROR);
        return false;
    }

    // All files must be in the public files path
    $filesPath = Ebms\Util::get_public_files_path();

    // Add all the files
    $zipCount = 0;
    foreach ($fileNames as $fname) {

        // Setup names for finding files and placing them in the output zip
        $inf  = $filesPath . '/' . $fname;
        if ($outputPath)
            $outf = $outputPath . '/' . $fname;
        else
            $outf = $fname;

        // Archive it
        $rc = $zip->addFile($inf, $outf);
        if ($rc === true)
            ++$zipCount;
        else {
            // Add an error document in its place
            $errFile = create_zip_error_file($inf);
            if ($errFile) {
                // Add it to the archive with a useful name
                $newName = $outf . "_ERROR.txt";
                $zip->addFile($errFile, $newName);
                unlink($errFile);
            }
            watchdog('make_zip_archive', "Error $rc adding $fname to $zipName",
                      null, WATCHDOG_ERROR);
        }
    }

    // Cleanup
    $rc = $zip->close();
    if ($rc !== true || $zipCount == 0) {
        unlink($zipName);
        if ($zipCount == 0)
            watchdog('make_zip_archive', "No files were saved in $zipName",
                      null, WATCHDOG_ERROR);
        else
            watchdog('make_zip_archive', "Error $rc closing $zipName",
                      null, WATCHDOG_ERROR);
        return false;
    }

    return true;
}

/**
 * Create a temporary document in the file system to describe an error
 * that occurred during a zip operation.  The caller should incorporate
 * this error document in place of the failed doc, then erase it from the
 * file system.
 *
 *  @param string $docName      Name of the doc that failed.
 *
 *  @return                     Name of the temp file, or false if failed.
 */
function create_zip_error_file($docName) {

    // Create a temporary file, easy to get rid of
    $fname = tempnam(sys_get_temp_dir(), 'zipErr');
    if ($fname === false)
        return false;
    drupal_chmod($fname, 0666);

    // Error message to show to the user
    $msg = <<<EOT
We're sorry, but an error has occurred while attempting to add the file named:

  $docName

to the zip archive file.

Please contact the PDQ Board Manager for help getting a copy of this document.
EOT;

    // Assume failure until success happens
    $ok = false;
    $fp = fopen($fname, "w");
    if ($fp !== false) {
        $bytes = fwrite($fp, $msg);
        if ($bytes)
            if (fclose($fp))
                $ok = true;
    }

    // Cleanup any failure
    if (!$ok) {
       unlink($fname);
       return false;
    }
    return $fname;
}
