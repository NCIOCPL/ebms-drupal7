<?php
/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */
?>

<?php
$elements = node_view($node);

$preview = array(
    '#type' => 'fieldset',
    '#title' => 'Preview',
    'elements' => $elements,
    '#attributes' => array(
        'class' => array(
            'form-item',
        ),
    ),
);

print drupal_render($preview);
?>