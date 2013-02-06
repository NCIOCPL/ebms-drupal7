<?php

/**
 * XXX Everything in this file will be jettisoned once we're in production.
 */
class EbmsUploadFile extends stdClass {
    function EbmsUploadFile($path, $mimetype, $uid) {
        $this->uid = $uid;
        $this->uri = $path; //'public://' . $path;
        $this->filename = basename($path);
        $this->filesize = filesize($path);
        $this->timestamp = time();
        $this->status = 1;
        $this->filemime = $mimetype;
    }
}

function pdq_ebms_upload_files() {
    return '<p>Already done</p>';
    global $user;
    $files = file('/tmp/upload-file-list');
    $html = array('<ul>');
    foreach ($files as $line) {
        $line = trim($line);
        $list = explode(' ', $line, 2);
        $mimetype = $list[0];
        $path = $list[1];
        $f = new EbmsUploadFile($path, $mimetype, $user->uid);
        $html[] = '<li>' . $path . ' (' . $mimetype . ')</li>';
        file_copy($f, 'public://' . $f->filename);
    }
    return '<p>Uploaded:</p>' . implode('', $html);
}

function pdq_ebms_stub() {
    return '<h2>Placeholder</h2>' .
        '<p>Stub used while functionality is being developed!</p>';
    return drupal_get_form('pdq_ebms_demo_form_1',
        array(
            'foo' => 1234,
            'bar' => array(
                'buzz' => 'lightyear',
                'prince' => 'hamlet',
            ),
        )
    );
}

/**
 * Form template for Alan.
 */
function pdq_ebms_demo_form_1($form, &$form_state, $extra) {
    return array(
        'name' => array(
            '#title' => 'Name',
            '#type' => 'textfield',
            '#description' => 'Use the format <i>Surname, First Names(s)</i>',
        ),
        'desc' => array(
            '#title' => 'Notes',
            '#type' => 'textarea',
            '#description' => 'Tell me everything you know about him/her.',
        ),
        'gender' => array(
            '#title' => 'Gender',
            '#type' => 'checkboxes',
            '#options' => array('Male', 'Female', 'Other'),
            '#description' => 'Check all that apply',
        ),
        'submit' => array(
            '#type' => 'submit',
            '#value' => 'Initiate Test',
            '#submit' => array('pdq_ebms_demo_form_1_submit'),
        ),
        '#validate' => array('testform_validation'),
        'foo' => array(
            '#type' => 'hidden',
            '#value' => $extra['bar']['prince'],
        ),
    );
}

function pdq_ebms_demo_form_1_submit($form, $form_state) {
    drupal_set_message('wrong name', 'error');
    return '<p>Your test failed.</p>';
}
function testform_validation($form, &$form_state) {
    form_set_error('desc', $form_state['values']['name'] . ' is bad.');
    drupal_set_message(print_r($form_state, true));
}

/**
 * We won't need this in production, but it certainly
 * comes in handy during development.  Hooked in temporarily
 * to a link in the footer.
 */
function pdq_ebms_clear_theme_cache() {
    drupal_theme_rebuild();
    return '<p>Theme cache cleared ' . date('c') . '.</p>';
}

/**
 * We won't need this in production, but it certainly
 * comes in handy during development.  Hooked in temporarily
 * to a link in the footer.
 */
function pdq_ebms_rebuild_menus() {
    menu_rebuild();
    return '<p>Caches cleared ' . date('c') . '.</p>';
}

/**
 * We won't need this in production, but it certainly
 * comes in handy during development.  Hooked in temporarily
 * to a link in the footer.
 */
function pdq_ebms_clear_all_caches() {
    cache_clear_all();
    return '<p>Caches cleared ' . date('c') . '.</p>';
}

function pdq_ebms_tester_theme() { return 'apitest'; }
