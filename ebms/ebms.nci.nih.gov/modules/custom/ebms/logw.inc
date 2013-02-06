<?php
define('FNAME', '/tmp/logw');
function logw($msg) {
    $fp = fopen(FNAME, "a") or die("can't open");
    fwrite($fp, $msg);
    fwrite($fp, "\n");
    fclose($fp);
}
