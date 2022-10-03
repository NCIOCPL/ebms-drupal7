<?php

namespace Drupal\Tests\ebms_article\Kernel;

use Drupal\Core\File\FileSystemInterface;
use Drupal\KernelTests\KernelTestBase;
use Drupal\ebms_article\Entity\Article;
use Drupal\Tests\user\Traits\UserCreationTrait;

/**
 * Test the article type.
 *
 * @group ebms
 */
class ArticleTest extends KernelTestBase {

  use UserCreationTrait;

  /**
   * {@inheritdoc}
   */
  protected static $modules = [
    'ebms_article',
    'ebms_core',
    'user',
    'file',
    'system',
    'datetime',
  ];

  /**
   * Test saving an EBMS article.
   *
   * @noinspection SpellCheckingInspection
   */
  public function testArticle() {
    // $perms = ['administer site configuration'];
    $this->installEntitySchema('ebms_article');
    $this->installEntitySchema('file');
    $this->installEntitySchema('user');
    $this->installSchema('system', ['sequences']);
    $this->installSchema('file', 'file_usage');
    $entity_type_manager = $this->container->get('entity_type.manager');
    $articles = $entity_type_manager->getStorage('ebms_article')->loadMultiple();
    $this->assertEmpty($articles);
    $module = \Drupal::service('extension.list.module')->getPath('ebms_article');
    $bytes = file_get_contents("$module/tests/data/test.pdf");
    $pdf_size = strlen($bytes);
    $file_repository = \Drupal::service('file.repository');
    $replace = FileSystemInterface::EXISTS_RENAME;
    $file = $file_repository->writeData($bytes, 'temporary://', $replace);
    $file->setPermanent();
    $file->save();
    $fid = $file->id();
    $title = 'Gastric cancer epidemiology in tertiary healthcare in Chiapas.';
    $authors = [
      'Canseco-Ávila LM',
      'Zamudio-Castellanos FY',
      'Sánchez-González RA',
      'Trujillo-Vizuet MG',
      'Domínguez-Arrevillaga S',
      'López-López CA',
    ];
    $author_names = [];
    foreach ($authors as $author) {
      list($surname, $initials) = explode(' ', $author);
      $forename = implode(' ', str_split($initials));
      $author_name = [
        'last_name' => $surname,
        'first_name' => $forename,
        'initials' => $initials,
      ];
      $author_names[] = $author_name;
    }
    $source = 'Pubmed';
    $source_id = '30243530';
    $journal_id = '0404271';
    $journal_title = 'Revista de gastroenterologia de Mexico';
    $journal_abbr = 'Rev Gastroenterol Mex';
    $volume = '84';
    $issue = '3';
    $pagination = '310-316';
    $year = 2019;
    $import_date = '2021-02-03 04:05:06';
    $label = "$journal_abbr $volume($issue): $pagination, $year";
    $abstract = [
      [
        'paragraph_label' => 'INTRODUCTION AND AIM',
        'paragraph_text' => 'Gastric cancer is the most frequent neoplasia ...',
      ],
      [
        'paragraph_label' => 'MATERIAL AND METHODS',
        'paragraph_text' => 'A descriptive, ambispective, longitudinal study was conducted ...',
      ],
      [
        'paragraph_label' => 'RESULTS',
        'paragraph_text' => 'A total of 100 cases of gastric cancer were detected, ...',
      ],
      [
        'paragraph_label' => 'CONCLUSION',
        'paragraph_text' => 'The results of the present epidemiologic analysis showed ...',
      ],
    ];
    $article = Article::create([
      'title' => $title,
      'authors' => $author_names,
      'source' => $source,
      'source_id' => $source_id,
      'source_journal_id' => $journal_id,
      'journal_title' => $journal_title,
      'brief_journal_title' => $journal_abbr,
      'volume' => $volume,
      'issue' => $issue,
      'pagination' => $pagination,
      'year' => $year,
      'pub_date' => ['year' => $year, 'month' => 'May'],
      'import_date' => $import_date,
      'abstract' => $abstract,
      'full_text' => ['file' => $fid],
    ]);
    $article->save();
    $article_id = $article->id();
    $articles = $entity_type_manager->getStorage('ebms_article')->loadMultiple();
    $this->assertNotEmpty($articles);
    $this->assertCount(1, $articles);
    $file_storage = $entity_type_manager->getStorage('file');
    foreach ($articles as $article) {
      $source_journal_id = $article->get('source_journal_id')->value;
      $article_journal_title = $article->get('journal_title')->value;
      $brief_journal_title = $article->get('brief_journal_title')->value;
      $pdf_file = $file_storage->load($article->get('full_text')->file);
      $i = 0;
      foreach ($article->get('authors') as $author) {
        $this->assertEquals($authors[$i++], $author->display_name);
      }
      $this->assertCount(count($authors), $article->get('authors'));
      $this->assertEquals($journal_id, $source_journal_id);
      $this->assertEquals($journal_title, $article_journal_title);
      $this->assertEquals($journal_abbr, $brief_journal_title);
      $this->assertEquals($article_id, $article->id());
      $this->assertEquals($title, $article->get('title')->value);
      $this->assertEquals($source, $article->get('source')->value);
      $this->assertEquals($source_id, $article->get('source_id')->value);
      $this->assertEquals($volume, $article->get('volume')->value);
      $this->assertEquals($issue, $article->get('issue')->value);
      $this->assertEquals($pagination, $article->get('pagination')->value);
      $this->assertEquals($import_date, $article->get('import_date')->value);
      $this->assertEquals($label, $article->getLabel());
      $this->assertEquals('May 2019', $article->pub_date[0]->toString());
      $this->assertEquals($pdf_size, $pdf_file->getSize());
    }
  }

}
