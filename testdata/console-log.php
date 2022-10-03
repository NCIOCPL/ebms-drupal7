<?php

function _color_log($str, $type = 'info', $bright = FALSE) {
  $values = [
    'e' => [41, 'error'],
    's' => [42, 'success'],
    'w' => [43, 'warning'],
    'i' => [46, 'info'],
  ];
  $key = substr(strtolower($type), 0, 1);
  if (!array_key_exists($key, $values)) {
    $key = 'i';
  }
  list($background, $label) = $values[$key];
  $foreground = $bright ? 97 : 37;
  echo " \033[$background;$foreground;1m[$label]\033[0m $str\n";
}

function log_info($message, $bright = FALSE) {
  _color_log($message, 'info', $bright);
}
function log_error($message, $bright = FALSE) {
  _color_log($message, 'error', $bright);
}
function log_warning($message, $bright = FALSE) {
  _color_log($message, 'warning', $bright);
}
function log_success($message, $bright = FALSE) {
  _color_log($message, 'success', $bright);
}
