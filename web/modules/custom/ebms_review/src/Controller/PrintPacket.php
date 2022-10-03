<?php

namespace Drupal\ebms_review\Controller;

use Drupal\Core\Controller\ControllerBase;
use Drupal\ebms_article\Entity\Article;
use Drupal\ebms_core\Entity\SavedRequest;
use Drupal\ebms_review\Entity\Packet;
use Drupal\file\Entity\File;
use Drupal\user\Entity\User;
use Symfony\Component\HttpFoundation\Response;

/**
 * AJAX callback for starred packets settings.
 */
class PrintPacket extends ControllerBase {

  /**
   * Values used for creating PDF files.
   */
  const LEFT_MARGIN = 60;
  const TOP_MARGIN = 70;
  const RIGHT_MARGIN = 540;
  const DEFAULT_FONT_HEIGHT = 13;
  const TITLE_FONT_HEIGHT = 18;
  const BIG_FONT_HEIGHT = 16;
  const NARROW_HEIGHT = 17;
  const WIDE_HEIGHT = 24;
  const FONT_FAMILY = 'DejaVu';
  const PRINT_JOBS = '/tmp/ebms/PrintJobs';

  /**
   * Return a zip file containing the documents to be printed for a packet.
   *
   * If the zip file has already been created by a previous invocation,
   * just return it. Otherwise, first create it, and then return it. The
   * file is created under /tmp, so the operating system may clear it out
   * as part of a periodic cleanup, in which case we just recreate the file.
   *
   * @param int $request_id
   *   ID for the `SavedRequest` entity representing the print job.
   *
   * @return Response
   *   Object representing the zip file.
   */
  public function retrieve(int $request_id) {

    // Establish where things will be located.
    $job_id = $request_id;
    $job_name = sprintf("PrintJob%05d", $job_id);
    $job_path = self::PRINT_JOBS . '/' . $job_name;
    $zip_name = "$job_name.zip";
    $zip_path = "$job_path.zip";

    // See if we already have the file we need.
    if (!is_file($zip_path)) {

      // Fetch the user's choices for this request.
      $parameters = SavedRequest::loadParameters($job_id);
      $member_id = $parameters['member'];
      $packet_id = $parameters['packet'];
      $options = $parameters['options'];
      $packet = Packet::load($packet_id);
      $member = User::load($member_id);
      $topic_id = $packet->topic->target_id;

      // Make sure the directory exists.
      if (!is_dir($job_path)) {
        mkdir($job_path, 0777, TRUE);
      }

      // Initialize the script which will perform the printing.
      $script = [
        '@echo off',
        'if "%EBMS_PACKET_PRINTER%." == "." goto usage',
        'echo Printing to %EBMS_PACKET_PRINTER%',
      ];

      // Load the values we'll need for the review form options.
      $dispositions = $this->loadDispositions();
      $reasons = $this->loadReasons();

      // Create the zip file.
      $flags = \FilesystemIterator::SKIP_DOTS | \FilesystemIterator::UNIX_PATHS;
      $zip_file = new \PharData($zip_path, $flags, null, \Phar::ZIP);

      // Walk through the articles, identifying the files needing printing.
      $filenames = [];
      $review_articles = [];
      foreach ($packet->articles as $packet_article) {
        $article = $packet_article->entity->article->entity;

        // Add the full-text PDF for the article if appropriate.
        if (in_array('articles', $options)) {
          if (!empty($article->full_text->file)) {
            $file = File::load($article->full_text->file);
            $filename = $file->filename->value;
            $filename_counter = 1;
            $path_parts = pathinfo($filename);
            $extension = $path_parts['extension'];
            while (in_array(strtolower($filename), $filenames)) {
              $filename = $path_parts['filename'] . '--' . $filename_counter++ . '.' . $extension;
            }
            $filenames[] = strtolower($filename);
            $bytes = file_get_contents($file->getFileUri());
            $local_name = $filename;
            $zip_file->addFromString($local_name, $bytes);
            ebms_debug_log("added $local_name to zip file");
            $command = $this->makePrintCommand($filename);
            if (empty($command)) {
              $script[] = "echo don't know how to print $filename";
            }
            else {
              $script[] = 'echo printing ' . $filename;
              $script[] = $command;
            }
          }
        }

        // Remember the review sheets we will create later.
        if (in_array('review-sheets', $options)) {
          $current_state = $article->getCurrentState($topic_id);
          if ($current_state->value->entity->field_text_id->value !== 'fyi') {
            $review_articles[] = $article;
          }
        }
      }

      // Create the review sheets, now that we know how many there will be.
      $counter = 1;
      $count = count($review_articles);
      foreach ($review_articles as $article) {
        $pdf = $this->createReviewForm($packet, $article, $member, $dispositions, $reasons, $counter, $count);
        $pdf_filename = 'packet-review-response-sheet-' . $counter++ . '.pdf';
        $pdf_path = "$job_path/$pdf_filename";
        $local_name = $pdf_filename;
        $pdf->Output($pdf_path, 'F');
        $zip_file->addFile($pdf_path, $local_name);
        ebms_debug_log("added $local_name to zip file");
        $command = $this->makePrintCommand($pdf_filename);
        if (empty($command)) {
          $script[] = "echo don't know how to print $pdf_filename";
        }
        else {
          $script[] = 'echo printing ' . $pdf_filename;
          $script[] = $command;
        }
      }

      // Add the Word documents for the summaries, if any.
      if (in_array('summaries', $options)) {
        foreach ($packet->summaries as $summary) {
          $file = $summary->entity->file->entity;
          $filename = $file->filename->value;
          $filename_counter = 1;
          $path_parts = pathinfo($filename);
          $extension = $path_parts['extension'];
          while (in_array(strtolower($filename), $filenames)) {
            $filename = $path_parts['filename'] . '--' . $filename_counter++ . '.' . $extension;
          }
          $filenames[] = strtolower($filename);
          $bytes = file_get_contents($file->getFileUri());
          $local_name = $filename;
          $zip_file->addFromString($local_name, $bytes);
          ebms_debug_log("added $local_name to zip file");
          $command = $this->makePrintCommand($filename);
          if (empty($command)) {
            $script[] = "echo don't know how to print $filename";
          }
          else {
            $script[] = 'echo printing ' . $filename;
            $script[] = $command;
          }
        }
      }

      // Finish off the printing script and add it to the bundle.
      $local_name = 'print-packets.cmd';
      $script[] = 'goto done';
      $script[] = ':usage';
      $script[] = 'echo You must set the environment variable EBMS_PACKET_PRINTER to the name of';
      $script[] = 'echo the printer which should be used for printing the packet.';
      $script[] = 'echo ------------------------------------------------------------------------';
      $script[] = ':done';
      $script[] = 'pause';
      $script = implode("\r\n", $script) . "\r\n";
      $zip_file->addFromString($local_name, $script);
      ebms_debug_log("added $local_name to zip file");
    }

    // Load the zip file from the disk and send it back to the browser.
    $bytes = file_get_contents($zip_path);
    $response = new Response($bytes);
    $response->headers->set('Content-type', 'application/zip');
    $response->headers->set('Content-disposition', "attachment;filename=$zip_name");
    return $response;
  }

  /**
   * Assemble the appropriate string for printing a document.
   *
   * @param string $filename
   *   String with extension used to identify the file type.
   *
   * @return string
   *   Command to invoke the right program, based on the file's type.
   *   Empty string if the file type is not supported.
   */
  private function makePrintCommand($filename) {
    $path_parts = pathinfo($filename);
    $extension = strtolower($path_parts['extension']);
    if ($extension === 'pdf') {
      return 'pdfprint.exe -duplex 2 -printer "%EBMS_PACKET_PRINTER%" "' . $filename . '"';
    }
    elseif (in_array($extension, ['doc', 'docx', 'rtf', 'odt'])) {
      return 'swriter.exe -pt "%EBMS_PACKET_PRINTER%" "' . $filename . '"';
    }
    return '';
  }

  /**
   * Load the values for decisions the reviewer can make for the article.
   *
   * @return array
   *   Nested arrays of strings describing each choice, each with a base
   *   description, as well as an optional string with extra information.
   */
  private function loadDispositions(): array {
    $storage = \Drupal::entityTypeManager()->getStorage('taxonomy_term');
    $query = $storage->getQuery()->accessCheck(FALSE)
                     ->sort('weight')
                     ->condition('status', 1)
                     ->condition('vid', 'dispositions');
    $dispositions = [];
    $first = TRUE;
    foreach ($storage->loadMultiple($query->execute()) as $term) {
      $name = $term->name->value;
      $desc = $term->description->value ?: '';
      if ($first) {
        $desc = 'indicate reason(s) for exclusion on the back';
        $first = FALSE;
      }
      $dispositions[] = [$name, rtrim($desc, '.')];
    }
    return $dispositions;
  }

  /**
   * Load the values for the reasons an article might be rejected.
   *
   * @return array
   *   Nested arrays of strings describing each choice, each with a base
   *   description, as well as an optional string with extra information.
   */
  private function loadReasons(): array {
    $storage = \Drupal::entityTypeManager()->getStorage('taxonomy_term');
    $query = $storage->getQuery()->accessCheck(FALSE)
                     ->sort('weight')
                     ->condition('status', 1)
                     ->condition('vid', 'rejection_reasons');
    $reasons = [];
    foreach ($storage->loadMultiple($query->execute()) as $term) {
      $name = $term->name->value;
      $desc = $term->description->value ?: '';
      $desc = str_replace('in the Comments field', 'below', $desc);
      $reasons[] = [$name, rtrim($desc, '.')];
    }
    return $reasons;
  }

  /**
   * Create a PDF form for reviewing an article.
   *
   * @param Packet $packet
   *   Entity for the packet being printed.
   * @param Article $article
   *   Entity for the article being reviewed.
   * @param User $member
   *   Board member who will review the article.
   * @param array $dispositions
   *   Decisions the board member can make about the article. Each disposition
   *   is an array of two strings, one for the basic identification of the
   *   decision, and an optional second with extra information, which is shown
   *   in italics.
   * @param array $reason
   *   Reasons why the article was rejected, if that was the decision. Same
   *   structure as the dispositions.
   * @param int $counter
   *   Which one of the set of review forms is this one?
   * @param int $count
   *   The number of review forms in the packet.
   *
   * @return \tFPDF
   *   Object representing a new PDF file.
   */
  private function createReviewForm(Packet $packet, Article $article, User $member, array $dispositions, array $reasons, int $counter, int $count): \tFPDF {

    // Collect the information we use to describe the article being reviewed.
    $authors = $article->getAuthors(3);
    if (empty($authors)) {
      $authors = '[No authors listed]';
    }
    else {
      $authors = implode(', ', $authors);
    }
    $pmid = $article->source_id->value;
    $ebms_id = $article->id();

    // Initialize the object and add the first page.
    $pdf = new \tFPDF('P', 'pt', 'Letter');
    $pdf->AddFont(self::FONT_FAMILY, '', 'DejaVuSansCondensed.ttf', TRUE);
    $pdf->AddFont(self::FONT_FAMILY, 'B', 'DejaVuSansCondensed-Bold.ttf', TRUE);
    $pdf->AddFont(self::FONT_FAMILY, 'I', 'DejaVuSansCondensed-Oblique.ttf', TRUE);
    $pdf->setFont(self::FONT_FAMILY, '');
    $pdf->setLeftMargin(self::LEFT_MARGIN);
    $pdf->setTopMargin(self::TOP_MARGIN);
    $pdf->setFontSize(self::TITLE_FONT_HEIGHT);
    $pdf->AddPage();

    // Identify the packet, reviewer, and article being reviewed.
    $pdf->Write(self::TITLE_FONT_HEIGHT, $packet->title->value);
    $pdf->setFontSize(self::BIG_FONT_HEIGHT);
    $pdf->Ln(self::NARROW_HEIGHT);
    $y = $pdf->GetY();
    $pdf->Line(self::LEFT_MARGIN, $y, self::RIGHT_MARGIN, $y);
    $pdf->ln(self::NARROW_HEIGHT);
    $pdf->setFont(self::FONT_FAMILY, '');
    $pdf->setFontSize(self::DEFAULT_FONT_HEIGHT);
    $pdf->Write(self::DEFAULT_FONT_HEIGHT, 'Reviewer: ' . $member->name->value);
    $pdf->Ln(self::DEFAULT_FONT_HEIGHT);
    $pdf->ln(self::NARROW_HEIGHT);
    $pdf->setFontSize(self::DEFAULT_FONT_HEIGHT);
    $pdf->Write(self::DEFAULT_FONT_HEIGHT, "Article $counter of $count");
    $pdf->setFont(self::FONT_FAMILY, 'B');
    $pdf->Ln(self::WIDE_HEIGHT);
    $pdf->Write(self::DEFAULT_FONT_HEIGHT, $authors);
    $pdf->Ln(self::WIDE_HEIGHT);
    $pdf->setFont(self::FONT_FAMILY, 'I');
    $pdf->Write(self::TITLE_FONT_HEIGHT, $article->title->value);
    $pdf->Ln(self::WIDE_HEIGHT);
    $pdf->setFont(self::FONT_FAMILY, '');
    $pdf->Write(self::DEFAULT_FONT_HEIGHT, $article->getLabel());
    $pdf->Ln(self::WIDE_HEIGHT);
    $pdf->setFont(self::FONT_FAMILY, '');
    $pdf->Write(self::DEFAULT_FONT_HEIGHT, "PMID: $pmid   EBMS ID: $ebms_id");
    $pdf->Ln(self::NARROW_HEIGHT * 2);

    // Add the choices for the reviewer's assessment of the article.
    $pdf->setFont(self::FONT_FAMILY, 'B', self::DEFAULT_FONT_HEIGHT);
    $pdf->Write(self::WIDE_HEIGHT, 'Disposition');
    $pdf->Ln(self::WIDE_HEIGHT);
    $pdf->SetFont(self::FONT_FAMILY, '', self::DEFAULT_FONT_HEIGHT);
    $pdf->Write(self::WIDE_HEIGHT, 'Indicate how the article might affect the summary.');
    $pdf->Ln(self::NARROW_HEIGHT * 2);
    $y = $pdf->GetY() + 4;
    $pdf->SetLeftMargin(self::LEFT_MARGIN + 18);
    foreach ($dispositions as $disposition) {
      $pdf->Rect(self::LEFT_MARGIN + 5, $y, 10, 10);
      $pdf->SetFont(self::FONT_FAMILY, '', self::DEFAULT_FONT_HEIGHT);
      $pdf->Write(self::TITLE_FONT_HEIGHT, $disposition[0]);
      if (!empty($disposition[1])) {
        $pdf->SetFont(self::FONT_FAMILY, 'I', self::DEFAULT_FONT_HEIGHT);
        $pdf->Write(self::TITLE_FONT_HEIGHT, ' (' . $disposition[1] . ')');
      }
      $pdf->Ln(self::WIDE_HEIGHT);
      $y = $pdf->GetY() + 4;
    }

    // Add some blank lines for comments.
    $pdf->SetLeftMargin(self::LEFT_MARGIN);
    $pdf->Ln(self::BIG_FONT_HEIGHT);
    $pdf->SetFont(self::FONT_FAMILY, 'B', self::DEFAULT_FONT_HEIGHT);
    $pdf->Write(self::WIDE_HEIGHT, 'Comments');
    $pdf->Ln(self::WIDE_HEIGHT + self::NARROW_HEIGHT);
    $y = $pdf->GetY();
    for ($i = 0; $i < 3; ++$i) {
      $pdf->Line(self::LEFT_MARGIN, $y, self::RIGHT_MARGIN, $y);
      $y += self::WIDE_HEIGHT;
    }

    // Add a block for specifying the article's level of evidence.
    $pdf->SetY($y);
    $pdf->Write(self::WIDE_HEIGHT, 'Levels of Evidence Information');
    $pdf->Ln(self::WIDE_HEIGHT);
    $pdf->SetFont(self::FONT_FAMILY, '', self::DEFAULT_FONT_HEIGHT);
    $pdf->Write(self::WIDE_HEIGHT, 'Enter the appropriate level of evidence for this article.');
    $y = $pdf->GetY() + 50;
    $pdf->Line(self::LEFT_MARGIN, $y, self::RIGHT_MARGIN, $y);

    // Add a second page for justifying rejected articles.
    $pdf->AddPage();
    $pdf->SetFont(self::FONT_FAMILY, 'B', self::DEFAULT_FONT_HEIGHT);
    $pdf->Write(self::WIDE_HEIGHT, 'Reason(s) for Exclusion From PDQ® Summary');
    $pdf->Ln(30);
    $pdf->SetFont(self::FONT_FAMILY, '', self::DEFAULT_FONT_HEIGHT);
    $pdf->Write(17, 'If you indicated that the article warrants no changes to the summary, indicate which of the reasons below led to your decision to exclude the article. You may choose more than one reason.');
    $pdf->Ln(30);
    $y = $pdf->GetY() + 4;
    $pdf->SetLeftMargin(self::LEFT_MARGIN + 18);
    foreach ($reasons as $reason) {
      $pdf->Rect(self::LEFT_MARGIN + 5, $y, 10, 10);
      $pdf->SetFont(self::FONT_FAMILY, '', self::DEFAULT_FONT_HEIGHT);
      $pdf->Write(self::TITLE_FONT_HEIGHT, $reason[0]);
      if (!empty($reason[1])) {
        $pdf->SetFont(self::FONT_FAMILY, 'I', self::DEFAULT_FONT_HEIGHT);
        $pdf->Write(self::TITLE_FONT_HEIGHT, ' (' . $reason[1] . ')');
      }
      $pdf->Ln(self::WIDE_HEIGHT);
      $y = $pdf->GetY() + 4;
    }
    $pdf->SetLeftMargin(self::LEFT_MARGIN);
    $y = $pdf->GetY() + 30;
    for ($i = 0; $i < 3; ++$i) {
      $pdf->Line(self::LEFT_MARGIN, $y, self::RIGHT_MARGIN, $y);
      $y += self::WIDE_HEIGHT;
    }

    // Pass the populated PDF object back to the caller.
    return $pdf;
  }

}
