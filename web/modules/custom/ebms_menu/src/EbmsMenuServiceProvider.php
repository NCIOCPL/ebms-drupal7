<?php

namespace Drupal\ebms_menu;

use \Drupal\Core\DependencyInjection\ContainerBuilder;
use \Drupal\Core\DependencyInjection\ServiceProviderBase;
use Symfony\Component\DependencyInjection\Reference;

/**
 * Overrides the class for the menu link tree.
 */
class EbmsMenuServiceProvider extends ServiceProviderBase {

  /**
   * {@inheritdoc}
   */
  public function alter(ContainerBuilder $container) {
    $definition = $container->getDefinition('menu.active_trail');
    $definition->setClass('Drupal\ebms_menu\ActiveTrail');
    $definition->addArgument(new Reference('request_stack'));
  }
}
