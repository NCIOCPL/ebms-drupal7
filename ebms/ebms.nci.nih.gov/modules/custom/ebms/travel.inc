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

function xebms_preprocess(&$vars, $hook) {
    // pdq_ebms_debug('preprocess', array($vars, $hook));
    switch ($hook) {
        case 'ebms_hotel_form':
            $vars['form']['choice']['#description'] =
                t('Where ya gonna snooze?');
            $vars['hotel_form_content'] = array();
            $hotel_form_hidden = array();
            foreach (element_children($vars['form']) as $key) {
                $type = $vars['form'][$key]['#type'];
                if ($type == 'hidden' || $type == 'token') {
                    $hotel_form_hidden[] =
                        drupal_render($vars['form'][$key]);
                }
                else {
                    $result = array();
                    if (array_key_exists('#title', $vars['form'][$key])) {
                        $title = $vars['form'][$key]['#title'];
                        $result[] = "<h3>$title</h3>";
                        $vars['form'][$key]['#title'] = '';
                    }
                    if (array_key_exists('#description', $vars['form'][$key])) {
                        $desc = $vars['form'][$key]['#description'];
                        $result[] = "<h4>$desc</h4>";
                        $vars['form'][$key]['#description'] = '';
                    }
                    $result[] = drupal_render($vars['form'][$key]);
                    $vars['hotel_form_content'][$key] = implode($result);
                }
            }
            $vars['hotel_form_content']['hidden'] =
                implode($hotel_form_hidden);
            $vars['hotel_form'] =
                implode($vars['hotel_form_content']);
            #$vars['myform'] = print_r($vars['form'], true);
            #    $vars['hotel_form_content'] = array(1, 2, 3, 4);
            pdq_ebms_debug('preprocess', $vars);
    }
}
*/
