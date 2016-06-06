<?php

// $Id$

/**
 * @file
 *
 * Implementation of EBMS Travel page.
 */
require_once('common.inc');
function pdq_ebms_travel($which='') {
    Ebms\Menu::$active = 'Travel';
    //debug($_GET, 'debug test');
    return '<i>Stub for travel page.</i>';
    /*
    switch ($which) {
        case 'hotel':
            return drupal_get_form('ebms_hotel_form');
        case 'transportation':
            return drupal_get_form('ebms_travel_form');
        default:
            return '<i>Stub for travel page.</i>';
    }
    */
}

/*
function ebms_theme() {
    return array(
        'ebms_travel_form' => array(
            'render element' => 'form',
        ),
        'ebms_hotel_form' => array(
            'render element' => 'form',
            'template' => 'ebms_hotel_form',
        ),
    );
}

function ebms_hotel_form($form, &$form_state) {
    $options = array(
        'hilton' => t('Hilton'),
        'holiday-inn' => t('Holiday Inn'),
        'marriott' => t('Marriott'),
        'other' => t('Other'),
    );
    $form['choice'] = array(
        '#type' => 'select',
        '#options' => $options,
        '#title' => t('Hotel'),
        '#description' => t('Choose where you want to sleep.'),
    );
    $form['submit'] = array(
        '#type' => 'submit',
        '#value' => t('Go'),
    );
    return $form;
}

function ebms_travel_form($form, &$form_state) {
    return array(
        'transportation' => array(
            '#type' => 'textfield',
            '#title' => t('Transportation Mode'),
            '#required' => true,
            '#description' => t('Tell us how you want to get here.'),
        ),
        'submit' => array(
            '#type' => 'submit',
            '#value' => t('Do it!'),
        ),
    );
}

function theme_ebms_hotel_form_submit($form, &$form_state) {
    drupal_set_message(print_r($form, true));
    drupal_set_message(print_r($form_state, true));
}

function theme_ebms_travel_form_submit($form, &$form_state) {
    drupal_set_message(print_r($form, true));
    drupal_set_message(print_r($form_state, true));
}

function theme_ebms_travel_form($variables) {
    $form = $variables['form'];
    return '<div id="traveling">' . drupal_render_children($form) . '</div>';
}

function template_preprocess_ebms_hotel_form(&$variables) {
#    pdq_ebms_debug('preprocess', $variables);
}

*/
