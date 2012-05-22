<?php

/**
 * $Id$
 *
 * Common classes, functions, etc., for the EBMS system.
 *
 */

namespace Ebms;

define('Ebms\APP_DIR', '/ebmsdev');
define('Ebms\THEME_DIR', APP_DIR . '/' .
    drupal_get_path('theme', 'ebms'));
define('Ebms\IMAGES_DIR', THEME_DIR . '/images');
define('Ebms\MODULE_DIR', drupal_get_path('module', 'ebms'));
define('Ebms\JS_DIR', MODULE_DIR . '/js');
define('Ebms\FROM', 'PDQ Editorial Board Management System <ebms@cancer.gov>');

class Util {
    private static $u2aMapping = array(
        160 => ' ',
        163 => '[pounds]',
        165 => '[yen]',
        169 => '(C)',
        174 => '(R)',
        176 => '[degree]',
        177 => '+/-',
        178 => '2',
        179 => '3',
        180 => '\'',
        181 => 'm',
        183 => '.',
        185 => '1',
        189 => '1/2',
        191 => '?',
        192 => 'A',
        193 => 'A',
        194 => 'A',
        197 => 'A',
        199 => 'C',
        201 => 'E',
        205 => 'I',
        207 => 'I',
        211 => 'O',
        214 => 'O',
        215 => 'x',
        216 => 'O',
        220 => 'U',
        223 => 'ss',
        224 => 'a',
        225 => 'a',
        226 => 'a',
        227 => 'a',
        228 => 'a',
        229 => 'a',
        230 => 'ae',
        231 => 'c',
        232 => 'e',
        233 => 'e',
        234 => 'e',
        235 => 'e',
        236 => 'i',
        237 => 'i',
        238 => 'i',
        239 => 'i',
        240 => 'eth',
        241 => 'n',
        242 => 'o',
        243 => 'o',
        244 => 'o',
        245 => 'o',
        246 => 'o',
        247 => '/',
        248 => 'o',
        249 => 'u',
        250 => 'u',
        251 => 'u',
        252 => 'u',
        253 => 'y',
        255 => 'y',
        257 => 'a',
        259 => 'a',
        261 => 'a',
        263 => 'c',
        265 => 'c',
        269 => 'c',
        273 => 'd',
        275 => 'e',
        277 => 'e',
        279 => 'e',
        281 => 'e',
        283 => 'e',
        285 => 'g',
        287 => 'g',
        291 => 'g',
        297 => 'i',
        299 => 'i',
        301 => 'i',
        304 => 'I',
        305 => 'i',
        309 => 'j',
        311 => 'k',
        312 => 'K',
        314 => 'l',
        321 => 'L',
        322 => 'l',
        324 => 'n',
        326 => 'n',
        328 => 'n',
        333 => 'o',
        335 => 'o',
        339 => 'oe',
        341 => 'r',
        345 => 'r',
        346 => 'S',
        347 => 's',
        349 => 's',
        350 => 'S',
        351 => 's',
        352 => 'S',
        353 => 's',
        355 => 't',
        361 => 'u',
        363 => 'u',
        365 => 'u',
        367 => 'u',
        378 => 'z',
        380 => 'z',
        381 => 'Z',
        382 => 'z',
        402 => 'f',
        487 => 'g',
        603 => 'e',
        730 => 'o',
        776 => '',
        913 => 'A',
        916 => '[DELTA]',
        922 => 'K',
        924 => 'M',
        931 => '[SIGMA]',
        932 => 'T',
        934 => '[PHI]',
        935 => 'X',
        936 => '[PSI]',
        937 => '[OMEGA]',
        945 => '[alpha]',
        946 => '[beta]',
        947 => '[gamma]',
        948 => '[delta]',
        949 => '[epsilon]',
        950 => '[zeta]',
        951 => '[eta]',
        952 => '[theta]',
        953 => 'i',
        954 => 'k',
        955 => '[lamda]',
        956 => '[mu]',
        959 => 'o',
        960 => '[pi]',
        961 => '[rho]',
        963 => '[sigma]',
        964 => '[tau]',
        966 => '[phi]',
        967 => 'X',
        968 => '[psi]',
        969 => '[omega]',
        981 => '[phi]',
        1008 => '[kappa]',
        1013 => '[epsilon]',
        1050 => 'K',
        8048 => 'a',
        8194 => ' ',
        8195 => ' ',
        8197 => ' ',
        8200 => ' ',
        8201 => ' ',
        8202 => ' ',
        8208 => '-',
        8209 => '-',
        8210 => '-',
        8211 => '-',
        8212 => '--',
        8216 => '\'',
        8217 => '\'',
        8220 => '"',
        8221 => '"',
        8224 => '[dagger]',
        8226 => '.',
        8230 => '...',
        8232 => ' ',
        8243 => '\'\'',
        8244 => '\'\'\'',
        8260 => '/',
        8304 => '0',
        8308 => '4',
        8309 => '5',
        8310 => '6',
        8311 => '7',
        8312 => '8',
        8313 => '9',
        8314 => '+',
        8315 => '-',
        8320 => '0',
        8321 => '1',
        8322 => '2',
        8323 => '3',
        8324 => '4',
        8325 => '5',
        8326 => '6',
        8327 => '7',
        8328 => '8',
        8329 => '9',
        8331 => '-',
        8364 => '[euro]',
        8467 => 'l',
        8482 => 'TM',
        8544 => 'I',
        8594 => '->',
        8710 => '[increment]',
        8722 => '-',
        8723 => '-/+',
        8727 => '*',
        8730 => '[square root]',
        8734 => '[infinity]',
        8745 => '[intersection] ',
        8746 => '[union]',
        8758 => ':',
        8764 => '~',
        8776 => '=',
        8804 => '<=',
        8805 => '>=',
        8806 => '<=',
        8807 => '>=',
        8853 => '+',
        9552 => '=',
        9651 => '[triangle]',
        10877 => '<=',
        10878 => '>=',
        12288 => ' ',
        12296 => '>',
        12297 => '<',
        20185 => '?',
        24310 => '?',
        26041 => '?',
        27668 => '?',
        30410 => '?',
        31890 => '?',
        32958 => '?',
        34917 => '?',
        39063 => '?',
        40548 => '?',
        40836 => '?',
        58102 => '?',
        58106 => '?',
        61472 => '?',
        61538 => '?',
        64257 => 'fi',
        64258 => 'fl',
        65279 => '',
        65308 => '<',
    );

    /**
     * Create a version of a string which replaces non-ascii characters
     * with ascii equivalents more likely to be entered by users
     * searching for titles or authors of articles.
     */
    public static function utf8ToAscii($utf8) {
        $ucs2 = iconv("utf-8", "UCS-2", $utf8);
        $ascii = array();
        $i = 0;
        $len = strlen($ucs2);
        while ($i < $len) {
            $lo = ord($ucs2[$i++]);
            $hi = ord($ucs2[$i++]);
            $code = $lo + $hi * 256;
            if (array_key_exists($code, self::$u2aMapping))
                $ascii[] = self::$u2aMapping[$code];
            elseif ($code < 128)
                $ascii[] = chr($code);
            else
                $ascii[] = '?';
        }
        return implode('', $ascii);
    }

    /**
     * Pulls a parameter from a request without letting in parameters
     * from cookies (which we'd get from $_REQUEST).
     */
    public static function get_request_value($name, $default='') {
        if (array_key_exists($name, $_POST))
            return $_POST[$name];
        if (array_key_exists($name, $_GET))
            return $_GET[$name];
        return $default;
    }

    /**
     * Replace with Alan's version.
     */
    public static function get_full_citation($article_id) {
        $xml = db_select('ebms_article', 'a')
            ->fields('a', array('source_data'))
            ->condition('a.article_id', $article_id)
            ->execute()
            ->fetchField();
        $doc = new \SimpleXMLElement($xml);
        $title = $ta = $vol = $issue = $pgn = $year = $pmid = '';
        $article = $doc->MedlineCitation->Article;
        $title = $article->ArticleTitle;
        $pgn = $article->Pagination->MedlinePgn;
        $year = $article->Journal->JournalIssue->PubDate->Year;
        $issue = $article->Journal->JournalIssue->Issue;
        $vol = $article->Journal->JournalIssue->Volume;
        $authors = array();
        foreach ($doc->MedlineCitation->Article->AuthorList->Author as $author)
        {
            if (sizeof($authors) == 3) {
                $authors[] = 'et al.';
                break;
            }
            $name = $author->LastName;
            if ($author->Initials)
                $name .= ' ' . $author->Initials;
            $authors[] = $name;
        }
        $ta = $doc->MedlineCitation->MedlineJournalInfo->MedlineTA;
        $pmid = $doc->MedlineCitation->PMID;
        return implode(', ', $authors) . ': ' . $title . ' ' . $ta . ' ' .
            $vol . '(' . $issue . '): ' . $pgn . ', ' . $year . '. PMID: ' .
            $pmid;
    }

    /**
     * Prepare an address for a system email message.
     */
    public static function format_email_address($email, $name) {
        $name = addslashes(mime_header_encode($name));
        return "\"$name\" <$email>";
    }
}
