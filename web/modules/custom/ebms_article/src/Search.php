<?php /** @noinspection ALL */

namespace Drupal\ebms_article;

use Drupal\Core\Entity\EntityTypeManagerInterface;
use Drupal\Core\Database\Connection;
use Drupal\ebms_core\Entity\SavedRequest;

/**
 * The EBMS search service.
 *
 * Exposes two public methods, one for loading the search parameters from the
 * database, and the other for building the entity query to find the articles
 * matching the user's search parameters.
 */
class Search {

  /**
   * The entity type manager needed by our two methods.
   *
   * @var EntityTypeManagerInterface
   */
  protected $typeManager;

  /**
   * Connection to the SQL database.
   *
   * @var Connection
   */
  protected $db;

  /**
   * Constructs a new \Drupal\ebms_article\Search object.
   *
   * @param EntityTypeManagerInterface $entity_type_manager
   *   The entity type manager.
   * @param Connection
   *   Connection to the SQL database.
   */
  public function __construct(EntityTypeManagerInterface $entity_type_manager, Connection $connection) {
    $this->typeManager = $entity_type_manager;
    $this->db = $connection;
  }

  /**
   * Assemble an entity query using the user's search options.
   *
   * @param array $parms
   *   Keyed values for the user's search options.
   * @return object
   *   The entity query for finding the articles matching the user's search.
   */
  public function buildQuery(array $parms) {
    $query_builder = new SearchQuery($this->typeManager, $this->db, $parms);
    return $query_builder->build();
  }

  /**
   * Retrieve the user's search criteria from the database.
   *
   * @param integer $search_id
   *   Primary key for the search request's entity.
   * @return array
   *   Keyed values for the user's search options.
   */
  public function loadParameters(int $search_id): array {
    return SavedRequest::loadParameters($search_id);
  }

}
