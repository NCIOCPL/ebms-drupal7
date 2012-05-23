<?php

// $Id$

/**
 * @file
 *
 * Implementation of EBMS Home page.
 */
function pdq_ebms_home() {
    global $user;
    if (!user_is_logged_in())
        drupal_goto('login');
    drupal_set_title('PDQ Editorial Board Management System');
    drupal_set_title('');
    drupal_set_breadcrumb(array('Home'));
    return array(
        '#theme' => 'page',
        '#type' => 'page',
        '#show_messages' => true,
        'content' => array(
            'first' => array(
                '#prefix' => '<div id="first">',
                '#suffix' => '</div>',
                'box' => array(
                    '#type' => 'markup',
                    '#markup' => '<p>FIRST</p>',
                ),
                'more' => array(
                    '#type' => 'markup',
                    '#markup' => '<p>MORE</p>',
                ),
            ),
                'themed' => array(
                    '#theme' => 'ebms_aggregate',
                    '#title' => 'Special Title',
                    #'#type' => 'element',
                    'child' => array('fee', 'fie', 'fo', 'fum'),
                ),
            'second' => array(
                '#prefix' => '<div id="second">',
                '#suffix' => '</div>',
                'box' => array(
                    '#type' => 'markup',
                    '#markup' => '<p>SECOND</p>',
                ),
                'more' => array(
                    '#type' => 'markup',
                    '#markup' => '<p>Still More</p>',
                ),
            ),
        ),
        'sidebar_second' => array(
            'third' => array(
                '#prefix' => '<div id="second">',
                '#suffix' => '</div>',
                'box' => array(
                    '#type' => 'markup',
                    '#markup' => '<p>THIRD</p>',
                ),
                'more' => array(
                    '#type' => 'markup',
                    '#markup' => '<p>Getting there</p>',
                ),
            ),
            'fourth' => array(
                '#prefix' => '<div id="fourth">',
                '#suffix' => '</div>',
                'box' => array(
                    '#type' => 'markup',
                    '#markup' => '<p>FOURTH</p>',
                ),
                'more' => array(
                    '#type' => 'markup',
                    '#markup' => '<p>Last Bit</p>',
                ),
            ),
        ),
    );
}
function ebms_theme() {
    return array('ebms_aggregate' => array('render element' => 'element'));
}
function theme_ebms_aggregate($variables) {
    pdq_ebms_debug('theme home page block', $variables);
    $output = array('<h2>' . $variables['element']['#title'] . '</h2>');
    $output[] = '<ul>';
    foreach (element_children($variables['element']['child']) as $k => $v)
        $output[] = "<li>$v</li>";
    $output[] = '</ul>';
    return implode($output);
}
function pdq_ebms_admin_on_front_page() {
    //drupal_set_message(drupal_is_front_page() ? 'front page' : 'back page');
    //drupal_set_message(user_access('administer') ? 'admin' : 'peon');
    return drupal_is_front_page() && user_access('administer');
}

function pdq_ebms_board_member_on_front_page() {
    return drupal_is_front_page() && user_access('review literature');
}

function xebms_block_info() {
    return array(
        'featured' => array(
            'info' => t('Featured'),
            'weight' => 0,
            'status' => 1,
            'visibility' => BLOCK_VISIBILITY_LISTED,
            'region' => 'content',
            'pages' => '<front>',
        ),
        'recent activity' => array(
            'info' => t('Recent Activity'),
            'weight' => 1,
            'status' => 1,
            'visibility' => BLOCK_VISIBILITY_LISTED,
            'region' => 'content',
            'pages' => '<front>',
        ),
        'pdq board rosters (member)' => array(
            'info' => t('PDQ Board Rosters (Board Member)'),
            'weight' => 2,
            'status' => 1,
            'visibility' => BLOCK_VISIBILITY_PHP,
            'pages' =>
            '<?php return pdq_ebms_board_member_on_front_page(); ?>',
            'region' => 'content',
        ),
        'administrative tasks' => array(
            'info' => t('Administrative Tasks'),
            'weight' => 3,
            'status' => 1,
            'visibility' => BLOCK_VISIBILITY_PHP,
            'pages' => '<?php return pdq_ebms_admin_on_front_page(); ?>',
            'region' => 'content',
        ),
        'alerts' => array(
            'info' => t('Alerts'),
            'weight' => 0,
            'status' => 1,
            'visibility' => BLOCK_VISIBILITY_LISTED,
            'pages' => '<front>',
            'region' => 'sidebar_second',
        ),
        'next meeting' => array(
            'info' => t('Next Meeting'),
            'weight' => 1,
            'status' => 1,
            'visibility' => BLOCK_VISIBILITY_LISTED,
            'pages' => '<front>',
            'region' => 'sidebar_second',
        ),
        'home calendar' => array(
            'info' => t('Calendar'),
            'weight' => 2,
            'status' => 1,
            'visibility' => BLOCK_VISIBILITY_LISTED,
            'pages' => '<front>',
            'region' => 'sidebar_second',
        ),
        'pdq board rosters (admin)' => array(
            'info' => t('PDQ Board Rosters (Admin)'),
            'weight' => 3,
            'status' => 1,
            'visibility' => BLOCK_VISIBILITY_PHP,
            'pages' => '<?php return pdq_ebms_admin_on_front_page(); ?>',
            'region' => 'sidebar_second',
        ),
    );
}

class EbmsFeaturesLinks {
    public $title, $links;
    public function __construct($title, $links) {
        $this->title = $title;
        $this->links = $links;
    }
}

function pdq_ebms_features_content() {
    $data = array(
        'admin' => array(
            '1' => new EbmsFeaturesLinks(
                'Citation Management',
                array(
                    l('Search Database', '#'),
                    l('Review Abstracts', '#'),
                    l('Add Citation', '#'),
                )
            ),
            '2' => new EbmsFeaturesLinks(
                'Literature Surveillance',
                array(
                    l('Create A Literature Surveillance Packet', 'review'),
                    l('Edit a Literature Surveillance Packet', 'review'),
                    l('Review Responses to a Literature Surveillance Packet',
                        'review'),
                )
            ),
            '3' => new EbmsFeaturesLinks(
                'Summaries',
                array(
                    l('Post Summary Document', 'summaries'),
                    l('Download and Review Summaries', 'summaries'),
                )
            ),
            '4' => new EbmsFeaturesLinks(
                'Reports',
                array(
                    l('Literature Surveillance Review Report', '#'),
                    l('Travel Reports', '#'),
                )
            ),
        ),
        'member' => array(
            '1' => new EbmsFeaturesLinks(
                'Literature Surveillance',
                array(
                    l('Review the Latest Literature Surveillance', 'review'),
                )
            ),
            '2' => new EbmsFeaturesLinks(
                'Summaries',
                array(
                    l('Download and Review Summaries', 'summaries'),
                    l('Submit Summaries with Changes', 'summaries'),
                )
            ),
            '3' => new EbmsFeaturesLinks(
                'Travel',
                array(
                    l('Hotel Reservations', 'travel'),
                    l('Directions to NCI', 'travel'),
                    l('Reimbursements', 'travel'),
                )
            )
        )
    );
    $feature = Ebms\Util::get_request_value('feature', '1');
    $role = user_access('administer') ? 'admin' : 'member';
    $data = $data[$role][$feature];
    $id = "featured-$role-$feature";
    $markup = array("<div class='ebms-featured-block' id='$id'><div>");
    $markup[] = "<p class='feature-links-title'>{$data->title}</p>";
    foreach ($data->links as $link)
        $markup[] = "<p>$link</p>";
    $markup[] = "</div></div>";
    return implode($markup);
    return implode(
        array(
            '<div class="ebms-featured-block" id="featured-admin-1">',
            '<div>',
            '<p class="feature-links-title">CITATION MANAGEMENT</p>',
            '<p><a href="#">Foo</a></p>',
            '<p><a href="#">Bar</a></p>',
            '<p><a href="#">Baz</a></p>',
            '</div></div>',
        )
    );
}
/*
array(
                    '#prefix' => "<div class=id='$id'>",
                    '#suffix' => '</div>',
                    'links' => array(
                        '#theme' => 'links',
                        
                    ),
                ),
#                '<img src="/ebmsdev/sites/all/themes/custom/ebms/images/featured-manager-1.jpg" />',
                */
function ebms_block_view($delta = '') {
    switch ($delta) {
        case 'featured':
            return array(
                'subject' => null,
                'content' => pdq_ebms_features_content(),
            );
        default:
            return array(
                'subject' => ucwords($delta),
                'content' => '<ul><li>One</li><li>Two</li><li>Three</li></ul>',
            );
    }
}

