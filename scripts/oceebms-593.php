<?php

// Create the ebms_config table if it doesn't already exist.
if (db_table_exists('ebms_config')) {
    echo "ebms_config table already exists\n";
}
else {
    $table = array(
        'description' => 'Configuration values which are loaded as needed.',
        'fields' => array(
            'config_name' => array(
                'description' => 'Identifies which value is stored.',
                'type' => 'varchar',
                'length' => 256,
                'not null' => TRUE,
            ),
            'config_value' => array(
                'description' => 'String value holding the configuration.',
                'type' => 'text',
                'not null' => TRUE,
            ),
        ),
        'primary key' => array('config_name'),
    );
    echo "creating ebms_config table\n";
    db_create_table('ebms_config', $table);
}

// Add the article-type-ancestors values.
$name = 'article-type-ancestors';
$value = <<<EOT
{
  "abbreviations": [
    "publication formats"
  ],
  "abstracts": [
    "publication components",
    "publication formats"
  ],
  "academic dissertation": [
    "publication formats"
  ],
  "account book": [
    "publication formats"
  ],
  "adaptive clinical trial": [
    "clinical study",
    "clinical trial",
    "study characteristics"
  ],
  "address": [
    "publication formats"
  ],
  "advertisement": [
    "ephemera",
    "publication components",
    "publication formats"
  ],
  "almanac": [
    "publication formats"
  ],
  "anecdotes": [
    "publication formats"
  ],
  "animation": [
    "publication components",
    "publication formats"
  ],
  "annual report": [
    "publication formats"
  ],
  "aphorisms and proverbs": [
    "publication formats"
  ],
  "architectural drawing": [
    "publication components",
    "publication formats"
  ],
  "atlas": [
    "publication formats"
  ],
  "autobiography": [
    "biography",
    "historical article",
    "personal narrative",
    "publication components",
    "publication formats"
  ],
  "bibliography": [
    "publication components",
    "publication formats"
  ],
  "biobibliography": [
    "bibliography",
    "publication components",
    "publication formats"
  ],
  "biography": [
    "historical article",
    "publication components",
    "publication formats"
  ],
  "blog": [
    "biography",
    "historical article",
    "publication components",
    "publication formats"
  ],
  "book illustrations": [
    "publication components"
  ],
  "book review": [
    "publication formats"
  ],
  "bookplate": [
    "publication components"
  ],
  "broadside": [
    "publication formats"
  ],
  "calendar": [
    "publication formats"
  ],
  "caricature": [
    "book illustrations",
    "pictorial work",
    "publication components",
    "publication formats"
  ],
  "cartoon": [
    "pictorial work",
    "publication formats",
    "wit and humor"
  ],
  "case reports": [
    "study characteristics"
  ],
  "catalog": [
    "publication formats"
  ],
  "catalog, bookseller": [
    "catalog",
    "catalog, commercial",
    "publication formats"
  ],
  "catalog, commercial": [
    "catalog",
    "publication formats"
  ],
  "catalog, drug": [
    "catalog",
    "catalog, commercial",
    "publication formats"
  ],
  "catalog, publisher": [
    "catalog",
    "catalog, commercial",
    "publication formats"
  ],
  "catalog, union": [
    "catalog",
    "publication formats"
  ],
  "chart": [
    "publication components",
    "publication formats"
  ],
  "chronology": [
    "publication formats"
  ],
  "classical article": [
    "historical article",
    "publication formats"
  ],
  "clinical conference": [
    "study characteristics"
  ],
  "clinical study": [
    "study characteristics"
  ],
  "clinical trial": [
    "clinical study",
    "study characteristics"
  ],
  "clinical trial protocol": [
    "clinical study",
    "study characteristics"
  ],
  "clinical trial, phase i": [
    "clinical study",
    "clinical trial",
    "study characteristics"
  ],
  "clinical trial, phase ii": [
    "clinical study",
    "clinical trial",
    "study characteristics"
  ],
  "clinical trial, phase iii": [
    "clinical study",
    "clinical trial",
    "study characteristics"
  ],
  "clinical trial, phase iv": [
    "clinical study",
    "clinical trial",
    "study characteristics"
  ],
  "clinical trial, veterinary": [
    "clinical study",
    "study characteristics"
  ],
  "collected correspondence": [
    "collected work",
    "publication formats"
  ],
  "collected work": [
    "publication formats"
  ],
  "collection": [
    "publication formats"
  ],
  "comment": [
    "publication components",
    "publication formats"
  ],
  "comparative study": [
    "study characteristics"
  ],
  "congress": [
    "publication formats"
  ],
  "consensus development conference": [
    "congress",
    "journal article",
    "publication formats",
    "review",
    "study characteristics"
  ],
  "consensus development conference, nih": [
    "congress",
    "consensus development conference",
    "journal article",
    "publication formats",
    "review",
    "study characteristics"
  ],
  "controlled clinical trial": [
    "clinical study",
    "clinical trial",
    "study characteristics"
  ],
  "cookbook": [
    "publication formats"
  ],
  "corrected and republished article": [
    "publication formats"
  ],
  "database": [
    "publication formats"
  ],
  "dataset": [
    "publication formats"
  ],
  "diary": [
    "publication formats"
  ],
  "dictionary": [
    "publication formats"
  ],
  "dictionary, chemical": [
    "dictionary",
    "publication formats"
  ],
  "dictionary, classical": [
    "dictionary",
    "publication formats"
  ],
  "dictionary, dental": [
    "dictionary",
    "publication formats"
  ],
  "dictionary, medical": [
    "dictionary",
    "publication formats"
  ],
  "dictionary, pharmaceutic": [
    "dictionary",
    "publication formats"
  ],
  "dictionary, polyglot": [
    "dictionary",
    "publication formats"
  ],
  "directory": [
    "publication formats"
  ],
  "dispensatory": [
    "publication formats"
  ],
  "documentaries and factual films": [
    "publication formats"
  ],
  "drawing": [
    "book illustrations",
    "pictorial work",
    "publication components",
    "publication formats"
  ],
  "duplicate publication": [
    "publication formats"
  ],
  "editorial": [
    "publication components",
    "publication formats"
  ],
  "electronic supplementary materials": [
    "publication components"
  ],
  "encyclopedia": [
    "publication formats"
  ],
  "english abstract": [
    "publication components"
  ],
  "ephemera": [
    "publication formats"
  ],
  "equivalence trial": [
    "clinical study",
    "clinical trial",
    "controlled clinical trial",
    "randomized controlled trial",
    "study characteristics"
  ],
  "essay": [
    "publication formats"
  ],
  "eulogy": [
    "publication formats"
  ],
  "evaluation study": [
    "study characteristics"
  ],
  "examination questions": [
    "publication formats"
  ],
  "exhibition": [
    "publication formats"
  ],
  "expression of concern": [
    "publication components"
  ],
  "festschrift": [
    "collected work",
    "historical article",
    "overall",
    "publication formats"
  ],
  "fictional work": [
    "publication formats"
  ],
  "form": [
    "publication formats"
  ],
  "formulary": [
    "publication formats"
  ],
  "formulary, dental": [
    "formulary",
    "publication formats"
  ],
  "formulary, homeopathic": [
    "formulary",
    "publication formats"
  ],
  "formulary, hospital": [
    "formulary",
    "publication formats"
  ],
  "funeral sermon": [
    "eulogy",
    "publication formats"
  ],
  "government publication": [
    "publication formats"
  ],
  "graphic novel": [
    "pictorial work",
    "publication formats"
  ],
  "guidebook": [
    "publication formats"
  ],
  "guideline": [
    "publication formats"
  ],
  "handbook": [
    "publication formats"
  ],
  "herbal": [
    "pharmacopoeia",
    "publication formats"
  ],
  "historical article": [
    "publication formats"
  ],
  "incunabula": [
    "publication formats"
  ],
  "index": [
    "publication formats"
  ],
  "instructional film and video": [
    "electronic supplementary materials",
    "publication components",
    "publication formats",
    "video-audio media"
  ],
  "interactive tutorial": [
    "electronic supplementary materials",
    "publication components",
    "video-audio media"
  ],
  "interview": [
    "biography",
    "historical article",
    "publication components",
    "publication formats"
  ],
  "introductory journal article": [
    "journal article",
    "publication formats"
  ],
  "journal article": [
    "publication formats"
  ],
  "juvenile literature": [
    "popular work",
    "publication formats"
  ],
  "laboratory manual": [
    "publication formats"
  ],
  "lecture": [
    "publication formats"
  ],
  "lecture note": [
    "publication formats"
  ],
  "legal case": [
    "publication formats"
  ],
  "legislation": [
    "publication formats"
  ],
  "letter": [
    "ephemera",
    "publication components",
    "publication formats"
  ],
  "manuscript": [
    "publication formats"
  ],
  "manuscript, medical": [
    "manuscript",
    "publication formats"
  ],
  "map": [
    "book illustrations",
    "pictorial work",
    "publication components",
    "publication formats"
  ],
  "meeting abstract": [
    "publication formats"
  ],
  "meta-analysis": [
    "study characteristics"
  ],
  "monograph": [
    "publication formats"
  ],
  "movable books": [
    "publication formats"
  ],
  "multicenter study": [
    "study characteristics"
  ],
  "news": [
    "publication components",
    "publication formats"
  ],
  "newspaper article": [
    "publication formats"
  ],
  "nurses instruction": [
    "publication formats"
  ],
  "observational study": [
    "clinical study",
    "study characteristics"
  ],
  "observational study, veterinary": [
    "clinical study",
    "study characteristics"
  ],
  "outline": [
    "publication formats"
  ],
  "overall": [
    "congress",
    "publication formats"
  ],
  "patent": [
    "publication formats"
  ],
  "patient education handout": [
    "popular work",
    "publication components",
    "publication formats"
  ],
  "periodical": [
    "publication formats"
  ],
  "periodical index": [
    "publication formats"
  ],
  "personal narrative": [
    "biography",
    "historical article",
    "publication components",
    "publication formats"
  ],
  "pharmacopoeia": [
    "publication formats"
  ],
  "pharmacopoeia, homeopathic": [
    "pharmacopoeia",
    "publication formats"
  ],
  "photograph": [
    "publication formats"
  ],
  "phrases": [
    "publication formats"
  ],
  "pictorial work": [
    "publication formats"
  ],
  "poetry": [
    "publication formats"
  ],
  "popular work": [
    "publication formats"
  ],
  "portrait": [
    "book illustrations",
    "pictorial work",
    "publication components",
    "publication formats"
  ],
  "postcard": [
    "publication formats"
  ],
  "poster": [
    "ephemera",
    "publication formats"
  ],
  "practice guideline": [
    "guideline",
    "publication formats"
  ],
  "pragmatic clinical trial": [
    "clinical study",
    "clinical trial",
    "controlled clinical trial",
    "randomized controlled trial",
    "study characteristics"
  ],
  "preprint": [
    "manuscript",
    "publication formats"
  ],
  "price list": [
    "catalog",
    "publication formats"
  ],
  "problems and exercises": [
    "publication formats"
  ],
  "program": [
    "ephemera",
    "publication formats"
  ],
  "programmed instruction": [
    "publication formats"
  ],
  "prospectus": [
    "ephemera",
    "publication formats"
  ],
  "public service announcement": [
    "publication formats"
  ],
  "publication components": [],
  "publication formats": [],
  "published erratum": [
    "publication components",
    "publication formats"
  ],
  "randomized controlled trial": [
    "clinical study",
    "clinical trial",
    "controlled clinical trial",
    "study characteristics"
  ],
  "randomized controlled trial, veterinary": [
    "clinical study",
    "clinical trial, veterinary",
    "study characteristics"
  ],
  "research support, american recovery and reinvestment act": [
    "research support, u.s. government",
    "support of research"
  ],
  "research support, n.i.h., extramural": [
    "research support, u.s. gov't, p.h.s.",
    "research support, u.s. government",
    "support of research"
  ],
  "research support, n.i.h., intramural": [
    "research support, u.s. gov't, p.h.s.",
    "research support, u.s. government",
    "support of research"
  ],
  "research support, non-u.s. gov't": [
    "support of research"
  ],
  "research support, u.s. gov't, non-p.h.s.": [
    "research support, u.s. government",
    "support of research"
  ],
  "research support, u.s. gov't, p.h.s.": [
    "research support, u.s. government",
    "support of research"
  ],
  "research support, u.s. government": [
    "support of research"
  ],
  "resource guide": [
    "publication formats"
  ],
  "retracted publication": [
    "publication formats"
  ],
  "retraction of publication": [
    "publication components",
    "publication formats"
  ],
  "review": [
    "journal article",
    "publication formats"
  ],
  "scientific integrity review": [
    "study characteristics"
  ],
  "sermon": [
    "address",
    "publication formats"
  ],
  "statistics": [
    "publication formats"
  ],
  "study characteristics": [],
  "study guide": [
    "publication formats"
  ],
  "support of research": [],
  "systematic review": [
    "study characteristics"
  ],
  "tables": [
    "publication formats"
  ],
  "technical report": [
    "publication formats"
  ],
  "terminology": [
    "publication formats"
  ],
  "textbook": [
    "monograph",
    "publication formats"
  ],
  "twin study": [
    "study characteristics"
  ],
  "unedited footage": [
    "publication formats"
  ],
  "union list": [
    "publication formats"
  ],
  "unpublished work": [
    "publication formats"
  ],
  "validation study": [
    "study characteristics"
  ],
  "video-audio media": [
    "electronic supplementary materials",
    "publication components",
    "publication formats"
  ],
  "web archive": [
    "collected work",
    "publication formats"
  ],
  "webcast": [
    "electronic supplementary materials",
    "publication components",
    "publication formats",
    "video-audio media"
  ],
  "wit and humor": [
    "publication formats"
  ]
}
EOT;
echo "adding publication type hierarchy information to config table table\n";
$fields = array('config_name' => $name, 'config_value' => $value);
db_delete('ebms_config')->condition('config_name', $name)->execute();
db_insert('ebms_config')->fields($fields)->execute();

// Test the values.
$article_type = 'Clinical Trial, Phase III';
$expected = array(
    'clinical study',
    'clinical trial',
    'study characteristics',
);
$query = db_select('ebms_config', 'c')
    ->condition('c.config_name', $name)
    ->fields('c', array('config_value'));
$ancestors = json_decode($query->execute()->fetchField(), TRUE);
if ($ancestors[strtolower($article_type)] === $expected) {
    echo "testing the loaded configuration value - OK\n";
}
else {
    echo "testing the loaded configuration value - FAILED\n";
}
?>