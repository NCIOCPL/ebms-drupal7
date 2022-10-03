<?php

function _color_log($str, $type = 'info', $seconds = FALSE, $bright = FALSE) {
  $values = [
    'e' => [41, 'error'],
    's' => [42, 'success'],
    'w' => [43, 'warning'],
    'i' => [46, 'info'],
  ];
  if ($seconds !== FALSE) {
    $s = $seconds;
    $h = floor($s / 3600);
    $s -= $h * 3600;
    $m = floor($s / 60);
    $s -= $m * 60;
    if (empty($h)) {
      $elapsed = $m . ':' . sprintf('%02d', $s);
    }
    else {
      $elapsed = $h . ':' . sprintf('%02d', $m) . ':' . sprintf('%02d', $s);
    }
    $str = "$str ($elapsed)";
  }
  $key = substr(strtolower($type), 0, 1);
  if (!array_key_exists($key, $values)) {
    $key = 'i';
  }
  list($background, $label) = $values[$key];
  $foreground = $bright ? 97 : 37;
  echo " \033[$background;$foreground;1m[$label]\033[0m $str\n";
}

function log_info($message, $seconds = FALSE, $bright = FALSE) {
  _color_log($message, 'info', $seconds, $bright);
}
function log_error($message, $seconds = FALSE, $bright = FALSE) {
  _color_log($message, 'error', $seconds, $bright);
}
function log_warning($message, $seconds = FALSE, $bright = FALSE) {
  _color_log($message, 'warning', $seconds, $bright);
}
function log_success($message, $seconds = FALSE, $bright = FALSE) {
  _color_log($message, 'success', $seconds, $bright);
}
