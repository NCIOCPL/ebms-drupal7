<?php

// $Id$

/**
 * @file
 *
 * Implementation of EBMS page for general site information.  This file
 * contains only a single public function, which is registered for the
 * site's About page.  It assembles and returns the render array for
 * the page.
 */
function pdq_ebms_about() {

    // We have our own style rules.
    drupal_add_css(Ebms\CSS_DIR . '/about.css');

    // Highlight the main menu tab for the page.
    Ebms\Menu::$active = 'About';

    // We construct our own breadcrumbs by hand, because the requirements
    // for them differ from what the Drupal menu system would give us.
    drupal_set_breadcrumb(array('About'));

    // Assemble the render array.
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
            '#markup' => '<p id="lead">The PDQ cancer information summaries ' .
            'are produced, maintained, and updated regularly by six ' .
            'editorial boards comprised of oncology specialists in fields ' .
            'including medical, surgical, and radiation oncology; ' .
            'epidemiology; psychology; genetics; and complementary and ' .
            'alternative medicine. Each Board reviews published research ' .
            'findings on a monthly basis and meets several times a year ' .
            'to review and discuss updates to their summaries. The Boards ' .
            'are not formal advisory or policy-making boards for the NCI.</p>',
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
        $pieces = explode('|', $result->description, 2);
        if (count($pieces) == 2)
            list($label, $description) = $pieces;
        else {
            $label = $pieces[0];
            $description = '';
        }
        $link = l($label, file_create_url($result->uri));
        $links[] = array($link, $description);
    }
    foreach ($links as $link_info) {
        list($link, $desc) = $link_info;
        $desc = htmlspecialchars($desc);
        $page['links'][] = array(
            '#type' => 'markup',
            '#markup' => '<h3>' . $link . '</h3><p class="link-desc">' .
            $desc . '</p>',
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
            array('VICTORIA SHIELDS, MS', '***REMOVED***'),
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
            array('ROBIN BALDWIN, BSN', '***REMOVED***'),
        ),
        array(
            'Cancer Genetics',
            'Experts in the fields of epidemiology, primary care, ethics, ' .
            'law, psychology, social sciences, and medical genetics.',
            array(
                array('Summaries', 'genetics'),
            ),
            'cancer-genetics',
            array('MARY B. DALY, MD, PhD', ''),
            array(***REDACTED***),
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
            array('SHARON QUINT-KASNER, MS', '***REMOVED***'),
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
            array('VALERIE DYER, BSN', '***REMOVED***'),
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
                'DEBRA BARTON, RN, PhD, AOCN, FAAN',
                'barton.debra@mayo.edu',
            ),
            array('ROBIN BALDWIN, BSN', '***REMOVED***'),
        ),
    );
    $page['boards']['title'] = array(
        '#type' => 'markup',
        '#markup' => '<h3 id="boards-title"><a name="pdq-boards"></a>' .
        'PDQ&reg; EDITORIAL BOARDS</h3>',
    );
    foreach ($boards as $board) {
        $markup = array('<h4 class="board-name">' . $board[0] .
                  ' Editorial Board</h4>');
        $markup[] = '<p class="board-desc">' . $board[1] . '</p>';
        $markup[] = '<p class="board-person">Editor-in-Chief: ' .
            //'<a href="mailto:' . $board[4][1] . '">' .
            $board[4][0] .
            //'</a>' .
            '</p>';
        $markup[] = '<p class="board-person">Board Manager: ' .
            '<a href="mailto:' . $board[5][1] . '">' .
            $board[5][0] . '</a></p>';
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
}
