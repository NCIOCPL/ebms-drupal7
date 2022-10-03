<?php

namespace Drupal\ebms_core\Controller;

use Drupal\Core\Controller\ControllerBase;
use Symfony\Component\HttpFoundation\Response;

/**
 * Show the end of the debugging log.
 */
class LogTail extends ControllerBase {

  /**
   * Show the bottom of the log.
   */
  public function display(): Response {
    try {
      $p = \Drupal::request()->get('p');
      $s = \Drupal::request()->get('s');
      $c = \Drupal::request()->get('c');
      $r = \Drupal::request()->get('r');
      if (!empty($r) & !empty($p) && file_exists($p)) {
        $name = pathinfo($p)['filename'];
        $count = filesize($p);
        $fp = fopen($p, 'rb');
        $bytes = fread($fp, $count);
        $response = new Response($bytes);
        $response->headers->set('Content-type', 'application/octet-stream');
        $response->headers->set('Content-disposition', "attachment;filename=$name");
        return $response;
      }
      if (!empty($p) && preg_match('/[*?]/', $p)) {
        $lines = [];
        foreach (glob($p) as $n) {
          $size = filesize($n);
          $time = date('Y-m-d H:i:s', filemtime($n));
          $lines [] = sprintf("%s %10d %s", $time, $size, $n);
        }
        $response = new Response(implode("\n", $lines) . "\n");
        $response->headers->set('Content-type', 'text/plain');
        return $response;
      }
      if (empty($p)) {
          $p = \Drupal::service('file_system')->realpath('public://') . '/ebms_debug.log';
      }
      if (!file_exists($p)) {
        $response = new Response("$p not found");
        $response->headers->set('Content-type', 'text/plain');
        return $response;
      }
      $size = filesize($p);
      $time = date('Y-m-d H:i:s', filemtime($p));
      if ($c === 'all') {
        $c = $size;
      }
      $count = is_numeric($c) ? intval($c) : 100000;
      $start = is_numeric($s) ? intval($s) : NULL;
      if ($count < 0) {
        $count = 0;
      }
      if (is_null($start)) {
        if (empty($count)) {
          $count = 200000;
        }
        if ($count > $size) {
          $count = $size;
          $start = 0;
        }
        else {
          $start = $size - $count;
        }
      }
      else {
        if ($start < 0) {
          $start = abs($start) > $size ? 0 : $size + $start;
        }
        elseif ($start > $size) {
          $start = $size;
        }
        $available = $size - $start;
        if ($count > $available) {
          $count = $available;
        }
      }
      if (empty($count)) {
        $response = new Response("$p is empty");
        $response->headers->set('Content-type', 'text/plain');
        return $response;
      }
      $fp = fopen($p, 'rb');
      if ($start) {
        fseek($fp, $start);
      }
      $bytes = fread($fp, $count);
      $first = $start + 1;
      $last = $start + $count;
      $response = new Response("$p $size bytes ($time) $first-$last\n$bytes");
      $response->headers->set('Content-type', 'text/plain');
      return $response;
    }
    catch (\Exception $e) {
      $response = new Response("log-tail failure: $e\n");
      $response->headers->set('Content-type', 'text/plain');
      return $response;
    }
  }

}
