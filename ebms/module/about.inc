<?php

// $Id$

/**
 * @file
 *
 * Implementation of EBMS page for general site information.
 */
function pdq_ebms_about() {
    drupal_add_css(Ebms\CSS_DIR . '/about.css');
    Ebms\Menu::$active = 'About';
    drupal_set_breadcrumb(array('About'));
    $page = array(
        'title' => array(
            '#type' => 'markup',
            '#markup' => '<h2>About PDQ&reg;</h2>',
        ),
        'image' => array(
            '#theme' => 'image',
            '#path' => Ebms\IMAGES_DIR . '/EBMS_AboutPage_Image_101212.jpg',
            '#title' => 'About PDQ',
            '#attributes' => array('id' => 'about-img'),
        ),
        'prologue' => array(
            '#type' => 'markup',
            '#markup' => '<p id="lead">Curabitur blandit tempus porttitor.  ' .
            'Vivamus sagittis lacus vel augue laoreet rutrum faucibus ' .
            'dolor auctor.  Donec id elit non mi porta gravida at eget ' .
            'metus. Cum sociis natoque penatibus et magnis dis parturient ' .
            'montes, nascetur ridiculus mus.  Morbi leo risus, porta ac ' .
            'consecietur ac, vestibulum at eros.  Aenean lacinia ' .
            'bibendum null sed consectetur.</p>',
        ),
        'links' => array(),
        'boards' => array(),
    );
    $links = array(
        array(
            l(
                'PDQ on NCI\'s Web Site',
                'http://cancer.gov/cancertopics/pdq/cancerdatabase'
            ),
            'Background information on PDQ',
        ),
    );
    $query = db_select('ebms_doc', 'd');
    $query->join('ebms_doc_tag', 'a', 'a.doc_id = d.doc_id');
    $query->join('ebms_tag', 't', 't.tag_id = a.tag_id');
    $query->join('file_managed', 'f', 'f.fid = d.file_id');
    $query->condition('t.tag_name', 'about');
    $query->fields('d', array('description'));
    $query->fields('f', array('uri'));
    $query->orderBy('d.description');
    $results = $query->execute();
    foreach ($results as $result) {
        // drupal_set_message($result->description);
        list($label, $description) = explode('|', $result->description);
        $link = l($label, file_create_url($result->uri));
        $links[] = array($link, $description);
    }
    foreach ($links as $link_info) {
        list($link, $desc) = $link_info;
        $desc = htmlspecialchars($desc);
        $page['links'][] = array(
            '#type' => 'markup',
            '#markup' => '<h3>' . $link . '</h3><p>' . $desc . '</p>',
        );
    }
    $boards = array(
        array(
            'Adult Treatment',
            'Experts in the fields of medical oncology, radiation ' .
            'oncology, and surgical oncology.',
            array(
                array('Summaries', 'adulttreatment'),
            ),
            'adult-treatment',
            array(***REDACTED***),
            array('Victoria Shields, MS', '***REMOVED***'),
        ),
        array(
            'Cancer Complementary and Alternative Medicine',
            'Experts in the fields of oncology, pharmacology, natural ' .
            'products, drug discovery and development, traditional ' .
            'Chinese medicine, acupuncture, and homeopathy.',
            array(
                array('Summaries', 'cam'),
            ),
            'cancer-cam',
            array(***REDACTED***),
            array('Robin Baldwin, BSN', '***REMOVED***'),
        ),
        array(
            'Cancer Genetics',
            'Experts in the fields of epidemiology, primary care, ethics, ' .
            'law, psychology, social sciences, and medical genetics.',
            array(
                array('Summaries', 'genetics'),
            ),
            'cancer-genetics',
            array(***REDACTED***),
            array('Robin Juthe, MPH', 'h***REMOVED***'),
        ),
        array(
            'Pediatric Treatment',
            'Experts in the fields of pediatric oncology, pediatric ' .
            'surgery, pediatric neuro-oncology, and radiation oncology.',
            array(
                array('Summaries', 'pediatrictreatment'),
            ),
            'pediatric-treatment',
            array(***REDACTED***),
            array('Sharon Quint-Kasner, MS', '***REMOVED***'),
        ),
        array(
            'Screening and Prevention',
            'Experts in the fields of oncology, cancer prevention, ' .
            'hematology, radiology, urology, statistics, epidemiology, ' .
            'and economics.',
            array(
                array('Screening Summaries', 'screening'),
                array('Prevention Summaries', 'prevention'),
            ),
            'screening-prevention',
            array(***REDACTED***),
            array('Valerie Dyer, BSN', '***REMOVED***'),
        ),
        array(
            'Supportive and Palliative Care',
            'Experts in the fields of pain and palliative care, ' .
            'psychology, medical oncology, and pediatric oncology.',
            array(
                array('Summaries', 'supportivecare'),
            ),
            'supportive-care',
            array(
                'Debra Barton, RN, PhD, AOCN, FAAN',
                'barton.debra@mayo.edu',
            ),
            array('Robin Baldwin, BSN', '***REMOVED***'),
        ),
    );
    $page['boards']['title'] = array(
        '#type' => 'markup',
        '#markup' => '<h3 id="boards-title">PDQ&reg; EDITORIAL BOARDS</h3>',
    );
    foreach ($boards as $board) {
        $markup = array('<h4 class="board-name">' . $board[0] .
                  ' Editorial Board</h4>');
        $markup[] = '<p class="board-desc">' . $board[1] . '</p>';
        $markup[] = '<p class="board-person">Editor-in-Chief: ' .
            '<a href="mailto:' . $board[4][1] . '">' .
            strtoupper($board[4][0] . '</a></p>');
        $markup[] = '<p class="board-person">Board Manager: ' .
            '<a href="mailto:' . $board[5][1] . '">' .
            strtoupper($board[5][0] . '</a></p>');
        $markup[] = '<p class="board-links"><a href="http://www.cancer.gov' .
            '/cancertopics/pdq/' . $board[3] . '-board" target="_blank">' .
            'View Board Roster</a>';
        foreach ($board[2] as $summaries) {
            $markup[] = ' | <a href="http://www.cancer.gov/cancertopics/pdq/' .
                $summaries[1] . '" target="_blank">View ' . $summaries[0] .
                '</a>';
        }
        $query = db_select('file_managed', 'f');
        $query->join('ebms_doc', 'd', 'd.file_id = f.fid');
        $query->join('ebms_board', 'b', 'd.doc_id = b.loe_guidelines');
        $query->condition('b.board_name', $board[0]);
        $query->fields('f', array('uri'));
        $uri = $query->execute()->fetchField();
        if ($uri) {
            $url = file_create_url($uri);
            $markup[] = ' | <a href="' . $url .
                '">DOWNLOAD LEVELS OF EVIDENCE</a>';
        }
        $markup[] = '</p>';
        $page['boards'][] = array(
            '#type' => 'markup',
            '#markup' => implode('', $markup),
        );
    }
    return $page;
    return drupal_get_form('pdq_ebms_state_bug_repro');
    drupal_set_title('About EBMS');
    return '<i>Stub for \'about\' page</i>';
}

function pdq_ebms_state_bug_repro($form, &$form_state) {
    return array(
        'flags' => array(
            '#title' => 'Options',
            '#type' => 'checkboxes',
            '#options' => array(
                'show-name-field' => 'Use Name Field',
                'show-date-field' => 'Use Date Field',
            ),
        ),
        'name' => array(
            '#type' => 'textfield',
            '#title' => 'Name',
            '#states' => array(
                'visible' => array(
                    '#edit-flags-show-name-field' => array('checked' => true),
                ),
            ),
        ),
        'date' => array(
            '#type' => 'date',
            '#title' => 'Date',
            '#states' => array(
                'visible' => array(
                    '#edit-flags-show-date-field' => array('checked' => true),
                ),
            ),
        ),
    );
}
