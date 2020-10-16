<?php
$rid = db_select('role', 'r')
    ->fields('r', array('rid'))
    ->condition('name', 'authenticated user')
    ->execute()
    ->fetchField();
$key = array(
    'rid' => $rid,
    'permission' => 'use text format video_html',
);
db_merge('role_permission')
    ->key($key)
    ->fields(array('module' => 'filter'))
    ->execute();
?>
