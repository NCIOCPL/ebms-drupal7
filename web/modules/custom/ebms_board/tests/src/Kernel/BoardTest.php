<?php

namespace Drupal\Tests\ebms_board\Kernel;

use Drupal\Core\File\FileSystemInterface;
use Drupal\KernelTests\KernelTestBase;
use Drupal\ebms_board\Entity\Board;
use Drupal\Tests\user\Traits\UserCreationTrait;

/**
 * Test the board type.
 *
 * @group ebms
 */
class BoardTest extends KernelTestBase {

  use UserCreationTrait;

  /**
   * {@inheritdoc}
   */
  protected static $modules = ['ebms_board', 'user', 'file', 'system'];

  /**
   * Test saving a PDQ board.
   */
  public function testBoard() {
    // $perms = ['administer site configuration'];
    $this->installEntitySchema('ebms_board');
    $this->installEntitySchema('file');
    $this->installEntitySchema('user');
    $this->installSchema('system', ['sequences']);
    $this->installSchema('file', 'file_usage');
    $entity_type_manager = $this->container->get('entity_type.manager');
    $boards = $entity_type_manager->getStorage('ebms_board')->loadMultiple();
    $this->assertEmpty($boards);
    $board_manager = $this->createUser();
    $guidelines = 'Always do the Right Thing';
    $uri = 'public://test-loe-guidelines.doc';
    $flags = FileSystemInterface::EXISTS_REPLACE;
    $file = $this->container->get('file.repository')->writeData($guidelines, $uri, $flags);
    $board = Board::create([
      'name' => 'Test Board',
      'manager' => $board_manager->id(),
      'loe_guidelines' => $file->id(),
    ]);
    $board->save();
    $boards = $entity_type_manager->getStorage('ebms_board')->loadMultiple();
    $this->assertNotEmpty($boards);
    $this->assertCount(1, $boards);
    foreach ($boards as $board) {
      $this->assertEquals('Test Board', $board->getName());
    }
  }

}
