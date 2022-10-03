<?php

namespace Drupal\ebms_meeting\Controller;

use Drupal\Core\Controller\ControllerBase;
use Symfony\Component\HttpFoundation\Response;
use Symfony\Component\HttpFoundation\BinaryFileResponse;
use Symfony\Component\HttpFoundation\File\Stream;
use ZipArchive;

/**
 * Pack up the meeting's files and send them to the user.
 */
class Archive extends ControllerBase {

  /**
   * Zip and send the meeting files and the agenda files.
   *
   * @param \Drupal\ebms_meeting\Entity\Meeting $meeting
   *   Entity for the meeting whose files we collect and send.
   *
   * @return Response|array
   *   A custom non-HTML response.
   */
  public function send($meeting): Response|array {

    // Make sure we have something to send.
    $filenames = $meeting->getFiles();
    if (empty($filenames)) {

      // Very unlikely to happen, since we checked for files before creating
      // the button, but you never know.
      $message = 'Meeting has no files tp be downloaded.';
      $this->messenger()->addError($message);
      $this->getLogger('ebms_meeting')->error($message);
      return [
        '#title' => $meeting->name->value,
      ];
    }

    // Create the archive of the meeting's files.
    $id = $meeting->id();
    $name = preg_replace('/\s+/', '_', $meeting->name->value);
    $name = str_replace('/', '_', $name);
    $date = substr($meeting->dates->value, 0, 10);
    $dirname = "{$name}_{$date}_{$id}_Docs";
    $path = $this->createArchive($filenames, $dirname);
    if (empty($path)) {
      $this->messenger()->addError('Failure creating documents archive.');
      return [
        '#title' => $meeting->name->value,
      ];
    }

    // Send it off.
    $stream = new Stream($path);
    $response = new BinaryFileResponse($stream);
    $response->headers->set('Content-type', 'application/zip');
    $response->headers->set('Content-disposition', 'attachment;filename="' . $dirname . '.zip"');
    return $response;
  }

  private function createArchive(array $filenames, string $dirname): string|false {
    $base = \Drupal::service('file_system')->realpath('public://');
    $zip = new \ZipArchive();
    $path = tempnam(sys_get_temp_dir(), 'meeting-docs');
    $rc = $zip->open($path, ZipArchive::CREATE);
    if ($rc !== TRUE) {
      $this->getLogger('ebms_meeting')->error("Error $rc creating $path");
      return FALSE;
    }
    $count = 0;
    foreach ($filenames as $filename) {
      $input = "$base/$filename";
      $rc = $zip->addFile("$base/$filename", "$dirname/$filename");
      if ($rc === TRUE) {
        ++$count;
      }
      else {
        $error_file = $this->createErrorFile();
        if (!empty($error_file)) {
          $zip->addFile($error_file, "$dirname/{$filename}_ERROR.txt");
          unlink($error_file);
        }
        $this->getLogger('ebms_meeting')->error("Error $rc adding $filename to $path");
      }
    }
    $rc = $zip->close();
    if ($rc !== TRUE || $count === 0) {
      unlink($path);
      if ($count === 0) {
        $this->getLogger('ebms_meeting')->error("No files were saved in $path");
      }
      else {
        $this->getLogger('ebms_meeting')->error("Error $rc closing $path");
      }
      return FALSE;
    }
    return $path;
  }

  private function createErrorFile($filename) {
    $path = tempnam(sys_get_temp_dir(), 'meeting-doc-error');
    if (empty($path)) {
      return '';
    }
    chmod($path, 0666);
    $message = <<<EOT
We're sorry, but an error has occurred while attempting to add the file named:

  $filename

to the zip archive file.

Please contact the PDQ Board Manager for help getting a copy of this document.
EOT;
    $rc = file_put_contents($path, $message);
    return $rc === FALSE ? '' : $path;
  }

}
