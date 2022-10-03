<?php

namespace Drupal\Tests\ebms_topic\Kernel;

use Drupal\KernelTests\KernelTestBase;
use Drupal\ebms_board\Entity\Board;
use Drupal\ebms_topic\Entity\Topic;
use Drupal\taxonomy\Entity\Term;
use Drupal\Tests\user\Traits\UserCreationTrait;

/**
 * Test the topic type.
 *
 * @group ebms
 */
class TopicTest extends KernelTestBase {

  use UserCreationTrait;

  /**
   * {@inheritdoc}
   */
  protected static $modules = [
    'ebms_board',
    'ebms_topic',
    'file',
    'system',
    'taxonomy',
    'text',
    'user',
  ];

  /**
   * Test saving an EBMS topic.
   */
  public function testTopic() {

    $this->installEntitySchema('ebms_board');
    $this->installEntitySchema('taxonomy_term');
    $this->installEntitySchema('ebms_topic');
    $this->installEntitySchema('user');
    $this->installSchema('system', ['sequences']);
    $entity_type_manager = $this->container->get('entity_type.manager');
    $topics = $entity_type_manager->getStorage('ebms_topic')->loadMultiple();
    $this->assertEmpty($topics);
    $name = 'Toenail Cancer';
    $board = Board::create(['name' => 'Test Board']);
    $board->save();
    $group_id = 135;
    $group_name = 'Lower Extremities';
    $topic_group = Term::create([
      'tid' => $group_id,
      'vid' => 'topic_groups',
      'name' => $group_name,
    ]);
    $topic_group->save();
    $nci_reviewer = $this->createUser();
    $topic = Topic::create([
      'name' => $name,
      'board' => $board->id(),
      'nci_reviewer' => $nci_reviewer,
      'topic_group' => $topic_group->id(),
      'active' => TRUE,
    ]);
    $topic->save();
    $topics = $entity_type_manager->getStorage('ebms_topic')->loadMultiple();
    $this->assertNotEmpty($topics);
    $this->assertCount(1, $topics);
    foreach ($topics as $topic) {
      $this->assertEquals($topic->getName(), $name);
      $this->assertEquals(TRUE, $topic->get('active')->value);
      $this->assertEquals($board->id(), $topic->get('board')->target_id);
      $this->assertEquals($nci_reviewer->id(), $topic->get('nci_reviewer')->target_id);
      $this->assertEquals($group_id, $topic->get('topic_group')->target_id);
    }
  }

}
