-- /* $Id$ */

/********************************************************
 * Drop all tables in reverse order to any references.
 ********************************************************/
DROP TABLE IF EXISTS ebms_summary_returned_doc;
DROP TABLE IF EXISTS ebms_summary_posted_doc;
DROP TABLE IF EXISTS ebms_summary_supporting_doc;
DROP TABLE IF EXISTS ebms_summary_link;
DROP TABLE IF EXISTS ebms_summary_page;
DROP TABLE IF EXISTS ebms_agenda;
DROP TABLE IF EXISTS ebms_report_request;
DROP TABLE IF EXISTS ebms_reimbursement_receipts;
DROP TABLE IF EXISTS ebms_reimbursement_item;
DROP TABLE IF EXISTS ebms_reimbursement_request;
DROP TABLE IF EXISTS ebms_hotel_request;
DROP TABLE IF EXISTS ebms_message_recipient;
DROP TABLE IF EXISTS ebms_message;
DROP TABLE IF EXISTS ebms_reviewer_doc;
DROP TABLE IF EXISTS ebms_review_rejection_reason;
DROP TABLE IF EXISTS ebms_review_disposition;
DROP TABLE IF EXISTS ebms_review_rejection_value;
DROP TABLE IF EXISTS ebms_review_disposition_value;
DROP TABLE IF EXISTS ebms_article_review;
DROP TABLE IF EXISTS ebms_packet_article;
DROP TABLE IF EXISTS ebms_packet_reviewer;
DROP TABLE IF EXISTS ebms_packet_summary;
DROP TABLE IF EXISTS ebms_packet;
DROP TABLE IF EXISTS ebms_article_board_decision;
DROP TABLE IF EXISTS ebms_article_board_decision_value;
DROP TABLE IF EXISTS ebms_article_state_comment;
DROP TABLE IF EXISTS ebms_article_state;
DROP TABLE IF EXISTS ebms_article_state_type;
DROP TABLE IF EXISTS ebms_import_action;
DROP TABLE IF EXISTS ebms_import_batch;
DROP TABLE IF EXISTS ebms_import_disposition;
DROP TABLE IF EXISTS ebms_not_list;
DROP TABLE IF EXISTS ebms_cycle;
DROP TABLE IF EXISTS ebms_article_author_cite;
DROP TABLE IF EXISTS ebms_article_author;
DROP TABLE IF EXISTS ebms_legacy_article_id;
DROP TABLE IF EXISTS ebms_article;
DROP TABLE IF EXISTS ebms_topic_reviewer;
DROP TABLE IF EXISTS ebms_doc_topic;
DROP VIEW  IF EXISTS ebms_active_topic;
DROP TABLE IF EXISTS ebms_topic;
DROP TABLE IF EXISTS ebms_ad_hoc_group_member;
DROP TABLE IF EXISTS ebms_ad_hoc_group;
DROP TABLE IF EXISTS ebms_doc_board;
DROP TABLE IF EXISTS ebms_doc_tag;
DROP TABLE IF EXISTS ebms_tag;
DROP TABLE IF EXISTS ebms_subgroup_member;
DROP TABLE IF EXISTS ebms_subgroup;
DROP TABLE IF EXISTS ebms_board_member;
DROP TABLE IF EXISTS ebms_board;
DROP TABLE IF EXISTS ebms_doc;
SET sql_mode='NO_AUTO_VALUE_ON_ZERO';

/********************************************************
 * Clear out EBMS rows from Drupal tables.
 ********************************************************/
DELETE FROM users_roles;

DELETE FROM users WHERE uid > 3;

DELETE FROM role WHERE name NOT IN ('anonymous user', 'authenticated user');

/********************************************************
 * Create all tables that are not standard Drupal tables.
 ********************************************************/

/*
 * Uploaded documents (does not include PubMed articles).
 *
 * doc_id         automatically generated primary key
 * file_id        foreign key into Drupal's file managment table
 * when_posted    date/time the user uploaded the documents
 * description    how the poster wants the document represented in lists
 */
CREATE TABLE ebms_doc
     (doc_id INTEGER          NOT NULL AUTO_INCREMENT PRIMARY KEY,
     file_id INTEGER UNSIGNED NOT NULL,
 when_posted DATETIME         NOT NULL,
 description TEXT             NOT NULL,
 FOREIGN KEY (file_id) REFERENCES file_managed (fid))
      ENGINE=InnoDB;

/*
 * Panels of oncology specialists who maintain the PDQ summaries.
 *
 * board_id       automatically generated primary key
 * board_name     unique string for the board's name
 * loe_guidelines optional foreign key into ebms_doc for board's LOE guidelines
 */
  CREATE TABLE ebms_board
     (board_id INTEGER      NOT NULL AUTO_INCREMENT PRIMARY KEY,
    board_name VARCHAR(255) NOT NULL UNIQUE,
loe_guidelines INTEGER          NULL,
   FOREIGN KEY (loe_guidelines) REFERENCES ebms_doc (doc_id))
        ENGINE=InnoDB;

/*
 * Members of the PDQ boards, including their managers (distinguished by role).
 *
 * user_id        foreign key into Drupal's users table
 * board_id       foreign key into our ebms_board table
 */
CREATE TABLE ebms_board_member
    (user_id INTEGER UNSIGNED NOT NULL,
    board_id INTEGER          NOT NULL,
 PRIMARY KEY (user_id, board_id),
 FOREIGN KEY (user_id)  REFERENCES users (uid),
 FOREIGN KEY (board_id) REFERENCES ebms_board (board_id))
      ENGINE=InnoDB;

/*
 * Working subsets of the PDQ boards; not all boards have them.
 *
 * sg_id          automatically generated primary key
 * sg_name        name of the subgroup, unique for each board
 * board_id       foreign key into the ebms_board table
 */
CREATE TABLE ebms_subgroup
      (sg_id INTEGER      NOT NULL AUTO_INCREMENT PRIMARY KEY,
     sg_name VARCHAR(255) NOT NULL,
    board_id INTEGER      NOT NULL,
  UNIQUE KEY sg_name_index (board_id, sg_name),
 FOREIGN KEY (board_id) REFERENCES ebms_board (board_id))
      ENGINE=InnoDB;

/*
 * Joins users to subgroups.
 *
 * user_id        foreign key into Drupal's users table
 * sg_id          foreign key into the ebms_subgroup table
 */
CREATE TABLE ebms_subgroup_member
    (user_id INTEGER UNSIGNED NOT NULL,
       sg_id INTEGER          NOT NULL,
 PRIMARY KEY (user_id, sg_id),
 FOREIGN KEY (user_id) REFERENCES users (uid),
 FOREIGN KEY (sg_id)   REFERENCES ebms_subgroup (sg_id))
      ENGINE=InnoDB;

/*
 * Tags associated with posted documents to indicate intended use.
 *
 * tag_id         automatically generated primary key
 * tag_name       unique name of the tag
 * tag_comment    optional description of what the tag is used for
 */
CREATE TABLE ebms_tag
     (tag_id INTEGER      NOT NULL AUTO_INCREMENT PRIMARY KEY,
    tag_name VARCHAR(64)  NOT NULL UNIQUE,
 tag_comment TEXT             NULL)
      ENGINE=InnoDB;
INSERT INTO ebms_tag (tag_name, tag_comment)
     VALUES ('about',
             CONCAT('Used for documents which should appear in the ',
                    '"General Information" block of the "About PDQ" page'));
INSERT INTO ebms_tag (tag_name, tag_comment)
     VALUES ('agenda', 'Documents linkable from a meeting agenda node');
INSERT INTO ebms_tag (tag_name, tag_comment)
     VALUES ('minutes', 'Documents linkable from a meeting minutes node');
INSERT INTO ebms_tag (tag_name, tag_comment)
     VALUES ('roster', 'Document which should appear on the Roster page');
INSERT INTO ebms_tag (tag_name, tag_comment)
     VALUES ('summary', 'Documents linkable from the Summaries pages');
INSERT INTO ebms_tag (tag_name, tag_comment)
     VALUES ('support',
             CONCAT('Used by board managers in combination with the "summary"',
                    ' tag for general-information documents to appear on the',
                    'Summaries pages'));

/*
 * Assignment of tags to posted documents.
 *
 * doc_id         foreign key into the ebms_doc table
 * tag_id         foreign key into the ebms_tag table
 */
CREATE TABLE ebms_doc_tag
     (doc_id INTEGER NOT NULL,
      tag_id INTEGER NOT NULL,
 PRIMARY KEY (tag_id, doc_id),
 FOREIGN KEY (doc_id) REFERENCES ebms_doc (doc_id),
 FOREIGN KEY (tag_id) REFERENCES ebms_tag (tag_id))
      ENGINE=InnoDB;

/*
 * Association of a posted document with one of the PDQ boards.
 *
 * doc_id         foreign key into the ebms_doc table
 * board_id       foreign key into the ebms_board table
 */
CREATE TABLE ebms_doc_board
     (doc_id INTEGER NOT NULL,
    board_id INTEGER NOT NULL,
 PRIMARY KEY (doc_id, board_id),
 FOREIGN KEY (doc_id)   REFERENCES ebms_doc (doc_id),
 FOREIGN KEY (board_id) REFERENCES ebms_board (board_id))
      ENGINE=InnoDB;

/*
 * Group created on the fly.
 *
 * Used, for example, for sending announcement messages to smaller sets
 * of board members.
 *
 * group_id       automatically generated primary key
 * group_name     unique name of the ad-hoc group
 * created_by     foreign key into Drupal's users table
 */
CREATE TABLE ebms_ad_hoc_group
   (group_id INTEGER          NOT NULL AUTO_INCREMENT PRIMARY KEY,
  group_name VARCHAR(255)     NOT NULL UNIQUE,
  created_by INTEGER UNSIGNED NOT NULL,
 FOREIGN KEY (created_by) REFERENCES users (uid))
      ENGINE=InnoDB;


/*
 * Membership in ad-hoc groups.
 *
 * user_id        foreign key into Drupal's users table
 * group_id       foreign key into the ebms_ad_hoc_group table
 */
CREATE TABLE ebms_ad_hoc_group_member
    (user_id INTEGER UNSIGNED NOT NULL,
    group_id INTEGER          NOT NULL,
 PRIMARY KEY (user_id, group_id),
 FOREIGN KEY (user_id)  REFERENCES users (uid),
 FOREIGN KEY (group_id) REFERENCES ebms_ad_hoc_group (group_id))
      ENGINE=InnoDB;

/*
 * Cancer topic reviewed by one of the PDQ boards.
 *
 * Usually, but not always, an EBMS topic corresponds to a PDQ summary in
 * the CDR.  At this time, any topic which doesn't already have a corresponding
 * PDQ summary associated with it represents a prospective summary to be
 * created at some point in the future.  Each topic is associated with
 * exactly one of the PDQ boards.
 *
 * topic_id       automatically generated primary key
 * topic_name     unique name for the topic
 * board_id       foreign key into the ebms_board table
 * active_status  can only associate articles with active topics.  Old
 *                  topics remain in the database because articles may
 *                  be linked to them.
 */
CREATE TABLE ebms_topic
     (topic_id INTEGER         NOT NULL AUTO_INCREMENT PRIMARY KEY,
    topic_name VARCHAR(255)    NOT NULL UNIQUE,
      board_id INTEGER         NOT NULL,
 active_status ENUM ('A', 'I') NOT NULL DEFAULT 'A',
 FOREIGN KEY (board_id) REFERENCES ebms_board (board_id))
      ENGINE=InnoDB;

   CREATE VIEW ebms_active_topic AS
          SELECT topic_id, topic_name, board_id
            FROM ebms_topic
           WHERE active_status = 'A';

/*
 * Assignment of topics to posted documents.
 *
 * Each posted document can have zero or more topics associated with it.
 * This assignment is used, for example, to restrict the contents of the
 * picklist of posted summary documents.
 *
 * topic_id       foreign key into the ebms_topic table
 * doc_id         foreign key into the ebms_doc table
 */
CREATE TABLE ebms_doc_topic
   (topic_id INTEGER NOT NULL,
      doc_id INTEGER NOT NULL,
 PRIMARY KEY (topic_id, doc_id),
 FOREIGN KEY (topic_id)  REFERENCES ebms_topic (topic_id),
 FOREIGN KEY (doc_id)    REFERENCES ebms_doc (doc_id))
      ENGINE=InnoDB;

/*
 * Reviewers who review specific topic by default.
 *
 * Board managers can override this default responsibility by assigning
 * other members of the board to a literature review packet, but having
 * a shorter set of checkboxes by default makes the normal packet creation
 * task easier.
 *
 * topic_id       foreign key into the ebms_topic table
 * user_id        foreign key into Drupal's users table
 */
CREATE TABLE ebms_topic_reviewer
   (topic_id INTEGER          NOT NULL,
     user_id INTEGER UNSIGNED NOT NULL,
 PRIMARY KEY (topic_id, user_id),
 FOREIGN KEY (topic_id) REFERENCES ebms_topic (topic_id),
 FOREIGN KEY (user_id)  REFERENCES users (uid))
      ENGINE=InnoDB;

/*
 * Articles about cancer and related topics.  Initially, all are from 
 * Pubmed but other sources could be added.
 *
 * These articles to through a series of steps to weed out the ones which
 * do not need to be passed on to the PDQ boards.  The rest are reviewed
 * by the boards to determine what changes to the PDQ summaries are
 * warranted to reflect the findings reported by those articles.  Articles
 * which make it to these later stages of processing will have a full text
 * copy retrieved and stored as a PDF file.
 *
 *  article_id      Automatically generated primary key
 *  source          Name of source, 'Pubmed' predominates.
 *  source_id       ID used by source for this article, e.g., PMID.
 *  source_jrnl_id  If source = 'Pubmed': then NLM unique journal id.
 *                    Else: to be determined.
 *  source_status   Source organization's status assignment to the record
 *                    Enables us to identify, e.g., records that are 
 *                    "In-Process", etc., and need future update.
 *  article_title   Full title of the article, converted to ASCII for searching.
 *  jrnl_title      Full journal title at time of import or update.
 *  brf_jrnl_title  Journal title abbreviation found in article record.
 *                    Normally this is the NLM title abbreviation.
 *  brf_citation    Citation identifying journal, year, vol, issue, pages, or
 *                    however we wish show the article on a single line.
 *  abstract        Abstract, currently always English.
 *  published_date  Indication of when the article was published (free text).
 *                    Can't use SQL datetime since some articles have dates 
 *                    like "Spring 2011".
 *  import_date     Datetime of the first appearance of this article in our
 *                    database.
 *  update_date     Datetime of the most recent replacement of the article
 *                    with updated data.  Null if never replaced.
 *  source_data     Unmodified XML or whatever downloaded from the source.
 *                    We'll assume it's always there and make it not null
 *                    changing that only if there's a real use case.
 *  full_text_id    Foreign key into the Drupal file mgt table for PDF of 
 *                    article.  Full text is actually stored in the file 
 *                    system.  The file_managed table tells us where.
 *  active_status   Provides a way to mark a citation to never be used for
 *                    any active purpose.  This is like the CDR 'D'eleted
 *                    status, not 'I'nactive.  We only use it if the 
 *                    citation should never ever have been imported at all.
 *                    Hopefully we'll never need to use this.
 *                    Values: 'A'ctive, 'D'eleted.
 */
CREATE TABLE ebms_article (
  article_id        INTEGER NOT NULL AUTO_INCREMENT PRIMARY KEY,
  source            VARCHAR(32) NOT NULL,
  source_id         VARCHAR(32) NOT NULL,
  source_jrnl_id    VARCHAR(32) NOT NULL,
  source_status     VARCHAR(32) NULL,
  article_title     VARCHAR(512) CHARACTER SET ASCII NOT NULL,
  jrnl_title        VARCHAR(512) CHARACTER SET ASCII NOT NULL,
  brf_jrnl_title    VARCHAR(127) NULL,
  brf_citation      VARCHAR(255) NOT NULL,
  abstract          TEXT NULL,
  published_date    VARCHAR(64) NOT NULL,
  import_date       DATETIME NOT NULL,
  update_date       DATETIME NULL,
  source_data       LONGTEXT NULL,
  full_text_id      INTEGER UNSIGNED NULL,
  active_status     ENUM('A', 'D') NOT NULL DEFAULT 'A',
  FOREIGN KEY (full_text_id) REFERENCES file_managed (fid)
)
      ENGINE=InnoDB;

      -- Searchable fields
      CREATE INDEX ebms_article_source_id_index
             ON ebms_article(source, source_id);
      CREATE INDEX ebms_article_source_jrnl_id_index
             ON ebms_article(source, source_jrnl_id);
      CREATE INDEX ebms_article_source_status_index
             ON ebms_article(source, source_status);
      CREATE INDEX ebms_article_title_index
             ON ebms_article(article_title);
      CREATE INDEX ebms_article_jrnl_title_index
             ON ebms_article(jrnl_title);
      CREATE INDEX ebms_article_brf_jrnl_title_index
             ON ebms_article(brf_jrnl_title);

/*
 * Make it possible to match up records from the old CiteMS system
 * with those in the new system.  Populated by the conversion software,
 * after which nothing needs to be done with this table.  History has
 * show that it will likely be useful to dig back into the old data
 * to troubleshoot mysteries which pop up, and this table will make
 * that process simpler.
 *
 * legacy_id        Primary key of the record in the CiteMS system
 * article_id       Foreign key into the ebms_article table
 */
CREATE TABLE ebms_legacy_article_id (
    legacy_id       INTEGER NOT NULL PRIMARY KEY,
    article_id      INTEGER NOT NULL,
    FOREIGN KEY (article_id) REFERENCES ebms_article (article_id)
)
    ENGINE=InnoDB;

/*
 * Authors of articles.
 *
 * These are as they appear in the XML records.  Name strings are not
 * unique and searches for an author may retrieve cites by different people
 * with the same names in Pubmed.
 *
 *  author_id       Unique ID for this character string, auto generated.
 *  last_name       Surname, called LastName in NLM XML.
 *  forename        Usually first name + optional middle initial.
 *                    But NLM can put other things in this field, e.g.:
 *                      "R Bryan",  "Deborah Ks",  "J"
 *                      "Maria del Refugio"
 *  initials        Usually first letter of first name + optional first letter
 *                    of middle name.  But again there are outliers:
 *                      "Mdel R" for Maria del Refugio Gonzales-Losa
 *                      "Nde S" for Nicholas de Saint Aubain Somerhausen
 *
 * Searching will be tricky and noisy.
 */
CREATE TABLE ebms_article_author (
    author_id       INT AUTO_INCREMENT PRIMARY KEY,
    last_name       VARCHAR(255) CHARACTER SET ASCII NOT NULL,
    forename        VARCHAR(128) CHARACTER SET ASCII NOT NULL,
    initials        VARCHAR(128) CHARACTER SET ASCII NOT NULL
)
    ENGINE = InnoDB;

    -- Two ways to search, use last + first name, or last + initials
    CREATE UNIQUE INDEX ebms_author_full_index 
           ON ebms_article_author (last_name, forename, initials);
    CREATE INDEX ebms_author_initials_index 
           ON ebms_article_author (last_name, initials);

/*
 * Join the authors with the citations.
 *
 *  article_id      Unique ID in article table.
 *  author_id       Unique ID in author table.
 *  cite_order      Order of this author in article , e.g., first author,
 *                   second author, etc.  Origin 1.
 *
 * Notes:
 *  Use the primary key to find all authors of an article, in the order
 *  they appeared in the article citation.
 *
 *  It's important to cite authors in the correct order of their
 *  appearance in an article.
 */
CREATE TABLE ebms_article_author_cite (
    article_id      INT NOT NULL,
    cite_order      INT NOT NULL,
    author_id       INT NOT NULL,
    PRIMARY KEY (article_id, cite_order, author_id),
    FOREIGN KEY (author_id) REFERENCES ebms_article_author(author_id),
    FOREIGN KEY (article_id) REFERENCES ebms_article(article_id)
)
    ENGINE = InnoDB;

    -- Finds all articles by an author
    CREATE INDEX ebms_author_article_index
              ON ebms_article_author_cite (author_id, article_id);

/*
 * Periods of time used to batch articles associated with a given topic.
 *
 * The concept of batching articles associated with a topic in clumps
 * related to periods of time (typically a month) is no longer as useful
 * as it was in the original Citation Management System.  In the new
 * system board managers will be free to assemble article review packets
 * containing articles from any cycle in the same packet (in fact, this
 * practice has been common for a while with some of the boards).
 * Nevertheless, the assignment of a topic to an article will still
 * carry an association with a cycle, and that cycle will be displayed
 * when the review packet is being created.
 *
 * cycle_id       Automatically generated primary key
 * cycle_name     Unique name for the cycle (e.g., 'November 2011')
 * start_date     Datetime of start.  Always order by start_date to guarantee
 *                 retrieval in date order since cycle_ids could be created
 *                 out of order due to conversion from old CMS or 
 *                 for other reasons.
 *
 * Note:
 *   This table is so small that a date index might not actually optimize it.
 */
CREATE TABLE ebms_cycle
   (cycle_id INTEGER     NOT NULL AUTO_INCREMENT PRIMARY KEY,
  cycle_name VARCHAR(40) NOT NULL UNIQUE,
  start_date DATETIME    NOT NULL UNIQUE)
      ENGINE=InnoDB;

/*
 * Identifies journals that are known to be poor sources of info.  Articles
 * from these journals are not further reviewed.
 *
 * If a journal has been found to be generally useless, an authorized
 * staff member can add it to a "NOT list".
 *
 * Maintenance of NOT lists inside the CiteMS is much simpler than 
 * attempting to modify every PubMed search to exclude every journal that
 * is not desirable.
 *
 * NOT lists will probably only ever be used on Pubmed imports, but we've
 * allowed for other sources.
 *
 * --XXX There's no history here.  If a journal comes off a not list, we
 * --XXX  we remove it from this table.  If it's listed again, we add it
 * --XXX  back again.
 * --XXX Is that reasonable?
 *
 *  source          Name of a source related to source_jrnl_id, normally
 *                   'Pubmed'.
 *  source_jrnl_id  The unique id for this journal assigned by the source.
 *                   Using source + source_jrnl_id is more robust than 
 *                   using the title because titles can change.
 *                   There's no EBMS authority file for this.  The IDs are
 *                   maintained by the source, i.e. NLM.
 *  board_id        ID of the board for which the journal is NOT listed.
 *                  We might use the value NULL to mean "all boards".
 *                  A journal_id may appear multiple times, once for each
 *                  of several boards.
 *  start_date      Date time when the journal was NOT listed.
 *  user_id         ID of the user adding this NOT list entry.
 */
CREATE TABLE ebms_not_list (
    source          VARCHAR(32) NOT NULL,
    source_jrnl_id  VARCHAR(32) NOT NULL,
    board_id        INT NULL,
    start_date      DATETIME NOT NULL,
    user_id         INT UNSIGNED NOT NULL,
    PRIMARY KEY (source, source_jrnl_id, board_id),
    FOREIGN KEY (board_id) REFERENCES ebms_board(board_id),
    FOREIGN KEY (user_id) REFERENCES users(uid)
)
    ENGINE = InnoDB;

    CREATE INDEX ebms_not_journal_index 
        ON ebms_not_list(board_id, source, source_jrnl_id);

/*
 * ebms_import_disposition
 * 
 * Control table for import_action.disposition.  This is a static set
 * of values that describe what happened to an article that was presented
 * to the system in an import batch.  It might have been:
 *
 *   imported
 *   rejected as a duplicate
 *   assigned a new summary topic to an article already in the system
 *   etc.
 * 
 *  disposition_id          Unique ID of the citation.
 *  disposition_name        Human readable display name.
 *  description             Fuller explanation of disposition.
 *  active_status           'A'ctive or 'I'nactive - don't use any more.
 */
CREATE TABLE ebms_import_disposition (
    disposition_id          INTEGER AUTO_INCREMENT PRIMARY KEY,
    disposition_name        VARCHAR(32) NOT NULL UNIQUE,
    description             VARCHAR(2048) NOT NULL,
    active_status           ENUM ('A', 'I') NOT NULL DEFAULT 'A'
)
    ENGINE = InnoDB;

    -- The required disposition values
    INSERT ebms_import_disposition (disposition_name, description) 
      VALUES ('Imported', 
      'First time import into the database');
    INSERT ebms_import_disposition (disposition_name, description) 
      VALUES ('NOT listed',
      'Imported but automatically rejected because the journal was NOT listed');
    INSERT ebms_import_disposition (disposition_name, description) 
      VALUES ('Duplicate, not imported', 
      'Article already in database with same topic.  Not re-imported.');
    INSERT ebms_import_disposition (disposition_name, description) 
      VALUES ('Summary topic added',
      'Article already in database.  New summary topic added.');
    INSERT ebms_import_disposition (disposition_name, description) 
      VALUES ('Replaced',
      'Article record replaced from updated, newly downloaded, source record');
    INSERT ebms_import_disposition (disposition_name, description) 
      VALUES ('Error',
      'An error occurred in locating or parsing the record.  Not imported.');

/*
 * One row for each batch of imported citations.
 *
 * Contains information describing a specific import action.  A full
 * picture of what happens in an import uses this table in combination
 * with other tables that track information about each citation that
 * is linked to a row in this table.
 *
 * Data written to this table should never be modified.
 *
 *  import_batch_id Unique ID of the import batch.
 *  topic_id        The topic under which the batch was imported.
 *  source          Name of source, 'Pubmed' predominates.
 *  import_date     Datetime of the import.
 *  cycle_id        Unique ID of a review cycle for the import batch.
 *  user_id         Unique ID of user running the import.
 *  article_count   Number of unique article IDs.  May be less than the
 *                   number of import_action rows referencing this row
 *                   because one article can appear in multiple categories.
 *  comment         Optional comment, e.g., if the batch was a special
 *                   import, why it was imported.
 */
CREATE TABLE ebms_import_batch (
    import_batch_id INT AUTO_INCREMENT PRIMARY KEY,
    topic_id        INT NULL,
    source          VARCHAR(32) NOT NULL,
    import_date     DATETIME NOT NULL,
    cycle_id        INT NOT NULL,
    user_id         INT UNSIGNED NOT NULL,
    article_count   INT NOT NULL,
    comment         VARCHAR(2048) NULL,
    FOREIGN KEY (topic_id) REFERENCES ebms_topic(topic_id),
    FOREIGN KEY (cycle_id) REFERENCES ebms_cycle(cycle_id),
    FOREIGN KEY (user_id)  REFERENCES users(uid)
)
    ENGINE = InnoDB;

/* 
 * One row for each disposition of a citation in an import batch.
 * See ebms_import_disposition.
 *
 * A single article might have more than one import disposition.  For example,
 * a record imported from NLM might already be in the database but not with
 * the same topic as used in this import batch, and with a different cycle_id.
 * It might also be a later record from NLM.  In such a case there are
 * three import disposition values - Summry topic added, Review cycle added,
 * Replaced.
 * 
 *  source_id       Unique ID of the citation within the source database.
 *                    We don't always have an article_id here because some 
 *                    articles in a batch may not have actually been imported.
 *  article_id      Unique ID of article row, if we have one.
 *  import_batch_id Unique ID of the batch.
 *  disposition_id  What was done with the imported cite, a code or ID
 *                  to be interpreted as one of:
 *                      Imported as new
 *                      Rejected as a duplicate
 *                      Duplicate but new summary topic added, no new review
 *                          cycle.
 *                      Duplicate but new summary topic added, new review
 *                          cycle added.
 */
CREATE TABLE ebms_import_action (
    source_id          VARCHAR(32) NOT NULL,
    article_id         INT NULL,
    import_batch_id    INT NOT NULL,
    disposition_id     INT NOT NULL,
    FOREIGN KEY (article_id) REFERENCES ebms_article(article_id),
    FOREIGN KEY (import_batch_id) REFERENCES ebms_import_batch(import_batch_id),
    FOREIGN KEY (disposition_id)
        REFERENCES ebms_import_disposition(disposition_id)
)
    ENGINE = InnoDB;


/*
 * Control values for recording processing states in the ebms_article_state
 * table.
 *
 *  state_id            Unique ID of the state value.
 *  state_name          Human readable name.
 *  description         Longer, descriptive help text.
 *  completed           'Y' = article in this state requires no further
 *                        processing.
 *  board_required      'Y' = rows with this state must have a non-NULL
 *                        board_id
 *  topic_required      'Y' = rows with this state must have a non-NULL
 *                        topic_id
 *  active_status       'A'ctive or 'I'nactive.
 *  sequence            The sequence order of states in workflows.
 */
CREATE TABLE ebms_article_state_type (
    state_id            INTEGER NOT NULL AUTO_INCREMENT PRIMARY KEY,
    state_name          VARCHAR(64) NOT NULL UNIQUE,
    description         VARCHAR(2048) NOT NULL,
    completed           ENUM('Y', 'N') NOT NULL DEFAULT 'N',
    board_required      ENUM('Y', 'N') NOT NULL DEFAULT 'Y',
    topic_required      ENUM('Y', 'N') NOT NULL DEFAULT 'Y',
    active_status       ENUM('A','I') NOT NULL DEFAULT 'A',
    sequence            INTEGER NULL
)
    ENGINE=InnoDB;

    -- States that an article can be in in the review process
    INSERT ebms_article_state_type 
        (state_name, description, sequence, completed)
        VALUES ('Imported', 'Imported into the database', 1, 'N');
    INSERT ebms_article_state_type 
        (state_name, description, sequence, completed)
        VALUES ('Ready for initial review', 
        'Article is associated with a summary topic, '
        'typically by a new import or an attempted import of a duplicate.  '
        'It is now ready for initial review', 2, 'N');
    INSERT ebms_article_state_type 
        (state_name, description, sequence, completed)
        VALUES ('Rejected by NOT list', 
        'Article appeared in a "NOT listed" journal, rejected without review',
        3, 'Y');
    INSERT ebms_article_state_type 
        (state_name, description, sequence, completed)
        VALUES ('Rejected in initial review', 
        'Rejected in initial review, before publication to board managers', 
        4, 'Y');
    INSERT ebms_article_state_type 
        (state_name, description, sequence, completed)
        VALUES ('Passed initial review', 
        'Article "published" for board manager review', 4, 'N');
    INSERT ebms_article_state_type 
        (state_name, description, sequence, completed)
        VALUES ('Rejected by Board Manager', 
        'Board manager rejected article', 5, 'Y');
    INSERT ebms_article_state_type 
        (state_name, description, sequence, completed)
        VALUES ('Passed Board Manager', 
        'Board manager accepted article for further review', 5, 'N');
    INSERT ebms_article_state_type 
        (state_name, description, sequence, completed)
        VALUES ('Requested full text', 
        'Board manager has requested retrieval of full text', 6, 'N');
    INSERT ebms_article_state_type 
        (state_name, description, sequence, completed)
        VALUES ('Full text retrieval failed', 
        'Staff were unable to obtain a copy of the article', 7, 'Y');
    INSERT ebms_article_state_type 
        (state_name, description, sequence, completed)
        VALUES ('Full text retrieved', 
        'Full text is available, awaiting further review by Board manager', 
        7, 'N');
    INSERT ebms_article_state_type 
        (state_name, description, sequence, completed)
        VALUES ('Rejected after full text review',
        'Full text examined at OCE, article rejected', 8, 'Y');
    INSERT ebms_article_state_type 
        (state_name, description, sequence, completed)
        VALUES ('Passed full text review',
        'Full text examined at OCE, article approved for board member review', 
        8, 'N');
    INSERT ebms_article_state_type 
        (state_name, description, sequence, completed)
        VALUES ('Flagged as FYI',
        'Article is being sent out without being linked to a specific topic',
        9, 'Y');
    INSERT ebms_article_state_type 
        (state_name, description, sequence, completed)
        VALUES ('No further action',
        'Decision after board member review is do not discuss.  '
        'Do not put on agenda.',
        10, 'Y');
    INSERT ebms_article_state_type 
        (state_name, description, sequence, completed)
        VALUES ('Changes not for agenda',
        'Changes may be made to summary but no board meeting required.  '
        'Do not put on agenda.',
        10, 'N');
    INSERT ebms_article_state_type 
        (state_name, description, sequence, completed)
        VALUES ('For future agenda (with changes)',
        'Show this on the picklist of articles that can be added to agenda.  '
        'Summary changes have been proposed.',
        10, 'N');
    INSERT ebms_article_state_type 
        (state_name, description, sequence, completed)
        VALUES ('For future agenda (discussion only)',
        'Show this on the picklist of articles that can be added to agenda.  '
        'No summary changes have been proposed.',
        10, 'N');
    INSERT ebms_article_state_type 
        (state_name, description, sequence, completed)
        VALUES ('On agenda',
        'Article is on the agenda for an upcoming meeting.',
        11, 'N');
    INSERT ebms_article_state_type 
        (state_name, description, sequence, completed)
        VALUES ('Final board decision',
        'Article was discussed at a board meeting and a decision was reached',
        12, 'Y');


/*
 * Processing states that an article is, or has been, in.
 *
 *  article_state_id   Automatically generated primary key
 *  article_id         Unique ID in article table.
 *  topic_id           The summary topic for which this article state is set.
 *  state_id           ID of the state that this row records.
 *  user_id            ID of the user that put the article in this state.
 *  status_dt          Date and time the row/state was created.
 *  active_status      Set to 'I' if the state row was a mistake, or no
 *                      longer applicable because of later events
 */
CREATE TABLE ebms_article_state (
    article_state_id  INTEGER NOT NULL AUTO_INCREMENT PRIMARY KEY,
    article_id        INTEGER NOT NULL,
    state_id          INTEGER NOT NULL,
    user_id           INTEGER UNSIGNED NOT NULL,
    status_dt         DATETIME NOT NULL,
    active_status     ENUM('A','I') NOT NULL DEFAULT 'A',
    board_id          INTEGER NULL,
    topic_id          INTEGER NULL,
    FOREIGN KEY (article_id) REFERENCES ebms_article(article_id),
    FOREIGN KEY (board_id)   REFERENCES ebms_board(board_id),
    FOREIGN KEY (topic_id)   REFERENCES ebms_topic(topic_id),
    FOREIGN KEY (state_id)   REFERENCES ebms_article_state_type(state_id),
    FOREIGN KEY (user_id)    REFERENCES users(uid)
)
    ENGINE InnoDB;

    -- Search for articles by article, state, board, or topic
    CREATE INDEX ebms_article_state_article_index
           ON ebms_article_state(article_id, state_id, active_status);
    CREATE INDEX ebms_article_state_state_index
           ON ebms_article_state(state_id, board_id, topic_id, article_id, 
                           active_status);
    CREATE INDEX ebms_article_state_board_index
           ON ebms_article_state(board_id, state_id, topic_id, article_id, 
                           active_status);
    CREATE INDEX ebms_article_state_topic_index
           ON ebms_article_state(topic_id, state_id, board_id, article_id, 
                           active_status);

/*
 * One or more optional comments can be associated with an 
 * ebms_article_state row.
 *
 * Comments are immutable.  To change one's mind about the contents of
 * a comment, a user can post another comment on the same state row.
 *
 *  comment_id          Unique auto-generated row ID of this comment.
 *  article_state_id    Of the article state row this comments on.
 *  user_id             Of user posting the comment.
 *  comment_dt          Datetime comment was recorded.
 *  comment             Free text.
 */
CREATE TABLE ebms_article_state_comment (
    comment_id          INTEGER NOT NULL AUTO_INCREMENT PRIMARY KEY,
    article_state_id    INTEGER NOT NULL,
    user_id             INTEGER UNSIGNED NOT NULL,
    comment_dt          DATETIME NOT NULL,
    comment            TEXT NOT NULL,

    FOREIGN KEY (article_state_id) REFERENCES
                ebms_article_state(article_state_id),
    FOREIGN KEY (user_id) REFERENCES users(uid)
)
    ENGINE InnoDB;

    -- Usually retrieved in association with state row, in date order
    CREATE INDEX ebms_article_state_comment_state_index ON
           ebms_article_state_comment(article_state_id, comment_dt);


/*
 * Values used to represent the board's final disposition regarding
 * an article for a specific topic.
 *
 * value_id       automatically generated primary key
 * value_name     display string for the value
 */
CREATE TABLE ebms_article_board_decision_value (
    value_id      INTEGER NOT NULL AUTO_INCREMENT PRIMARY KEY,
    value_name    VARCHAR(64) NOT NULL UNIQUE
)
    ENGINE InnoDB;
INSERT INTO ebms_article_board_decision_value (value_name)
     VALUES ('Cited (citation only)');
INSERT INTO ebms_article_board_decision_value (value_name)
     VALUES ('Cited (legacy)');
INSERT INTO ebms_article_board_decision_value (value_name)
     VALUES ('Not cited');
INSERT INTO ebms_article_board_decision_value (value_name)
     VALUES ('Text approved');
INSERT INTO ebms_article_board_decision_value (value_name)
     VALUES ('Text needs to be written');
INSERT INTO ebms_article_board_decision_value (value_name)
     VALUES ('Text needs to be revised');
INSERT INTO ebms_article_board_decision_value (value_name)
     VALUES ('Hold');

/*
 * Record of the ultimate disposition of a journal article with respect
 * to a specific topic.  See the corresponding row in the ebms_article_state
 * table for comments and date.  This table really only exists to record
 * the meeting date.  Otherwise we would have split the 'Final board
 * decision' state into two states ('Rejected at board meeting' and
 * 'Accepted at board meeting').
 *
 *  article_state_id   foreign key into ebms_article_state table
 *  decision_value_id  foreign key into ebms_article_board_decision_value table
 *  meeting_date       foreign key into the ebms_cycle table for the meeting
 *                       at which the article was discussed
 *  discussed          'Y' if the article was discussed
 */
CREATE TABLE ebms_article_board_decision (
    article_state_id  INTEGER NOT NULL,
    decision_value_id INTEGER NOT NULL,
    meeting_date      INTEGER NULL,
    discussed         ENUM('Y', 'N') NULL,
    PRIMARY KEY (article_state_id, decision_value_id),
    FOREIGN KEY (article_state_id)
        REFERENCES ebms_article_state(article_state_id),
    FOREIGN KEY (decision_value_id)
        REFERENCES ebms_article_board_decision_value(value_id),
    FOREIGN KEY (meeting_date)  REFERENCES ebms_cycle(cycle_id)
)
    ENGINE InnoDB;

/*
 * Collection of articles on a given topic assigned for board member review.
 *
 * Board members are regulary assigned sets of published articles to review
 * for determination whether the information contained in those articles
 * should be used to modify one or more of the PDQ summaries.  These sets
 * of articles are referred to as "review packets," and will typically
 * be assigned to several members of the board responsible for the packet's
 * topic (a packet is always assembled for a single topic).  Usually
 * the reviewers assigned will be selected from those board members who
 * are designated as specialists in the packet's topic, but this can
 * be overridden as appropriate.
 *
 * packet_id      automatically generated primary key
 * topic_id       foreign key into the ebms_topic table
 * created_by     foreign key into Drupal's users table
 * packet_title   how the packet should be identified in lists of packets
 * last_seen      when the board manager last saw the feedback for the packet
 */
CREATE TABLE ebms_packet
  (packet_id INTEGER          NOT NULL AUTO_INCREMENT PRIMARY KEY,
    topic_id INTEGER          NOT NULL,
  created_by INTEGER UNSIGNED NOT NULL,
packet_title VARCHAR(255)     NOT NULL,
   last_seen DATETIME             NULL,
 FOREIGN KEY (topic_id)   REFERENCES ebms_topic (topic_id),
 FOREIGN KEY (created_by) REFERENCES users (uid))
      ENGINE=InnoDB;

/*
 * PDQ summary document related to the articles in a review packet.
 *
 * One or more summary documents can be attached to a review packet.
 * These are typically Microsoft Word documents, which the reviewer can
 * download and, if appropriate, upload a modified version of the document
 * (or a portion of the document).  These documents are typically created
 * by running a QC report on the document in the CDR, and converting the
 * HTML page for the report to a Microsoft Word document.
 *
 * packet_id      foreign key into the ebms_packet table
 * doc_id         foreign key into the ebms_doc table
 */
CREATE TABLE ebms_packet_summary
  (packet_id INTEGER      NOT NULL,
      doc_id INTEGER      NOT NULL,
 PRIMARY KEY (packet_id, doc_id),
 FOREIGN KEY (packet_id) REFERENCES ebms_packet (packet_id),
 FOREIGN KEY (doc_id)    REFERENCES ebms_doc (doc_id))
      ENGINE=InnoDB;

/*
 * Board member assigned to review the articles in a review packet.
 *
 * packet_id      foreign key into the ebms_packet table
 * reviewer_id    foreign key into Drupal's users table
 */
CREATE TABLE ebms_packet_reviewer
  (packet_id INTEGER          NOT NULL,
 reviewer_id INTEGER UNSIGNED NOT NULL,
 PRIMARY KEY (packet_id, reviewer_id),
 FOREIGN KEY (packet_id)  REFERENCES ebms_packet (packet_id),
 FOREIGN KEY (reviewer_id) REFERENCES users (uid))
      ENGINE=InnoDB;

/*
 * Article assigned for review in a review packet.
 *
 * The systems does not allow removal of an article from a review packet,
 * in order to avoid discarding of review feedback which has already been
 * posted for the article.  Instead articles can be suppressed so that
 * when reviewers come back to their queues of articles to be reviewed
 * the suppressed articles are not included in their workload.
 *
 * article_id     foreign key into the ebms_article table
 * packet_id      foreign key into the ebms_packet table
 * drop_flag      should the article be omitted from the review work queue?
 */
CREATE TABLE ebms_packet_article
 (article_id INTEGER      NOT NULL,
   packet_id INTEGER      NOT NULL,
   drop_flag INTEGER      NOT NULL DEFAULT 0,
 PRIMARY KEY (packet_id, article_id),
 FOREIGN KEY (article_id) REFERENCES ebms_article (article_id),
 FOREIGN KEY (packet_id)  REFERENCES ebms_packet (packet_id))
      ENGINE=InnoDB;

/*
 * Feedback from a reviewer on an article in a review packet.
 *
 * review_id      automatically generated primary key
 * packet_id      foreign key into the ebms_packet table
 * article_id     foreign key into the ebms_article table
 * reviewer_id    foreign key into Drupal's users table
 * when_posted    date/time the feedback was posted
 * comments       free text elaboration of how the reviewer feels the
 *                article's findings should be incorporated into the
 *                PDQ summaries (or why it shouldn't be)
 * loe_info       reviewer's assessment of the levels of evidence
 *                found in the article; free text, but following the
 *                guidelines used by the board for LOE
 */
CREATE TABLE ebms_article_review
  (review_id INTEGER          NOT NULL AUTO_INCREMENT PRIMARY KEY,
   packet_id INTEGER          NOT NULL,
  article_id INTEGER          NOT NULL,
 reviewer_id INTEGER UNSIGNED NOT NULL,
 when_posted DATETIME         NOT NULL,
    comments TEXT                 NULL,
    loe_info TEXT                 NULL,
  UNIQUE KEY ebms_art_review_index (article_id, reviewer_id, packet_id),
   FOREIGN KEY (packet_id,
                article_id)  REFERENCES ebms_packet_article (packet_id,
                                                             article_id),
 FOREIGN KEY (reviewer_id) REFERENCES users (uid))
      ENGINE=InnoDB;

/*
 * Lookup table for decisions a reviewer can make about an article
 * (s)he is reviewing.  More than one value can be chosen by the
 * reviewer for the article.
 *
 * value_id       automatically generated primary key
 * value_name     string used to represent the disposition's value
 * value_pos      integer specifying position for the user interface
 * instructions   optional additional string for the user interface
 */
CREATE TABLE ebms_review_disposition_value
   (value_id INTEGER      NOT NULL AUTO_INCREMENT PRIMARY KEY,
  value_name VARCHAR(80)  NOT NULL UNIQUE,
   value_pos INTEGER      NOT NULL,
instructions VARCHAR(255)     NULL)
      ENGINE=InnoDB;

INSERT INTO ebms_review_disposition_value (value_name, value_pos)
     VALUES ('Warrants no changes to the summary', 1);
INSERT INTO ebms_review_disposition_value (value_name, value_pos, instructions)
     VALUES ('Deserves citation in the summary', 2,
             'indicate placement in the summary document');
INSERT INTO ebms_review_disposition_value (value_name, value_pos, instructions)
     VALUES ('Merits revision of the text', 3,
             'indicate changes in the summary document');
INSERT INTO ebms_review_disposition_value (value_name, value_pos)
     VALUES ('Merits discussion', 4);

/*
 * Lookup table for reasons a reviewer can give for indicating that
 * an article merits no changes to the summary.
 *
 * value_id       automatically generated primary key
 * value_name     string used to represent the reason
 * value_pos      integer specifying position for the user interface
 * extra_info     optional additional explanation for the user interface
 */
CREATE TABLE ebms_review_rejection_value
   (value_id INTEGER      NOT NULL AUTO_INCREMENT PRIMARY KEY,
  value_name VARCHAR(80)  NOT NULL UNIQUE,
   value_pos INTEGER      NOT NULL,
  extra_info VARCHAR(255)     NULL)
      ENGINE=InnoDB;

INSERT INTO ebms_review_rejection_value (value_name, value_pos)
     VALUES ('Not relevant to PDQ summary topic', 1);
INSERT INTO ebms_review_rejection_value (value_name, value_pos)
     VALUES ('Already cited in PDQ summary', 2);
INSERT INTO ebms_review_rejection_value (value_name, value_pos, extra_info)
     VALUES ('Review/expert opinion/commentary', 3, 'no new primary data');
INSERT INTO ebms_review_rejection_value (value_name, value_pos)
     VALUES ('Provides no new information/novel findings', 4);
INSERT INTO ebms_review_rejection_value (value_name, value_pos)
     VALUES ('Inappropriate study design', 5);
INSERT INTO ebms_review_rejection_value (value_name, value_pos, extra_info)
     VALUES ('Inadequate study population', 6,
       'small number of patients; underpowered study; accrual target not met');
INSERT INTO ebms_review_rejection_value (value_name, value_pos)
     VALUES ('Randomized trial with flawed or insufficiently described randomization process', 7);
INSERT INTO ebms_review_rejection_value (value_name, value_pos, extra_info)
     VALUES ('Unvalidated outcome measure(s) used', 8,
             'e.g., unvalidated surrogate endpoint[s]');
INSERT INTO ebms_review_rejection_value (value_name, value_pos)
     VALUES ('Missing/incomplete outcome data; major protocol deviations', 9);
INSERT INTO ebms_review_rejection_value (value_name, value_pos)
     VALUES ('Inadequate follow-up', 10);
INSERT INTO ebms_review_rejection_value (value_name, value_pos, extra_info)
     VALUES ('Inappropriate statistical analysis', 11,
             'incorrect tests; lack of intent to treat analysis');
INSERT INTO ebms_review_rejection_value (value_name, value_pos)
     VALUES ('Inappropriate interpretation of subgroup analyses', 12);
INSERT INTO ebms_review_rejection_value (value_name, value_pos)
     VALUES ('Preliminary findings; need confirmation', 13);
INSERT INTO ebms_review_rejection_value (value_name, value_pos)
     VALUES ('Findings not clinically important', 14);
INSERT INTO ebms_review_rejection_value (value_name, value_pos, extra_info)
     VALUES ('Other', 15, 'specify reason(s) in the Comments field');

/*
 * Assignment of disposition to an article in a specific packet by
 * a reviewer.
 *
 * review_id      foreign key into ebms_article_review table
 * value_id       foreign key into ebms_review_disposition_value table
 */
CREATE TABLE ebms_review_disposition
  (review_id INTEGER NOT NULL,
    value_id INTEGER NOT NULL,
 PRIMARY KEY (review_id, value_id),
 FOREIGN KEY (review_id) REFERENCES ebms_article_review (review_id),
 FOREIGN KEY (value_id)  REFERENCES ebms_review_disposition_value (value_id))
      ENGINE=InnoDB;

/*
 * Identification of a reason why the reviewer decided an article in
 * a review packet did not merit inclusion in the summary.  More than
 * one reason can be specified for the decision.
 *
 * review_id      foreign key into ebms_article_review table
 * value_id       foreign key into ebms_review_rejection_value table
 */
CREATE TABLE ebms_review_rejection_reason
  (review_id INTEGER NOT NULL,
    value_id INTEGER NOT NULL,
 PRIMARY KEY (review_id, value_id),
 FOREIGN KEY (review_id) REFERENCES ebms_article_review (review_id),
 FOREIGN KEY (value_id)  REFERENCES ebms_review_rejection_value (value_id))
      ENGINE=InnoDB;

/*
 * Document posted by a reviewer to a packet.
 *
 * Reviewers can optionally post documents which he or she believes
 * might be useful accompaniments to the feedback entered for the
 * individual articles (for example, a modified version of a summary
 * document).  If the reviewer decides to drop the document, a flag
 * is set to suppress it, but the document it not purged from the
 * system.
 *
 * doc_id         automatically generated primary key
 * file_id        foreign key into Drupal's file_managed table
 * reviewer_id    foreign key into Drupal's users table
 * packet_id      foreign key into the ebms_packet table
 * drop_flag      has the posting of the document been retracted?
 * when_posted    date/time the document was posted
 * doc_title      how the document should be identified in lists
 * description    optional notes accompanying the posted document
 */
CREATE TABLE ebms_reviewer_doc
     (doc_id INTEGER          NOT NULL AUTO_INCREMENT PRIMARY KEY,
     file_id INTEGER UNSIGNED NOT NULL,
 reviewer_id INTEGER UNSIGNED NOT NULL,
   packet_id INTEGER          NOT NULL,
   drop_flag INTEGER          NOT NULL DEFAULT 0,
 when_posted DATETIME         NOT NULL,
   doc_title VARCHAR(256)     NOT NULL,
 description TEXT                 NULL,
 FOREIGN KEY (file_id)     REFERENCES file_managed (fid),
 FOREIGN KEY (reviewer_id) REFERENCES users (uid),
 FOREIGN KEY (packet_id)   REFERENCES ebms_packet (packet_id))
      ENGINE=InnoDB;

/*
 * An announcement message sent to one or more users of the system.
 *
 * Board managers can send messages to users of the EBMS.  An email
 * message is sent to each of the designated recipients, and the
 * message is available for viewing within the system.  Replies to
 * these announcements (if any) take place outside the EBMS.
 * Board members cannot send these announcement messages, but
 * they have access to similar functionality in the system's
 * forum area.
 * 
 * message_id     automatically generated primary key
 * sender_id      foreign key into Drupal's users table
 * when_posted    date/time the message was sent
 * msg_subject    what the announcement is about
 * msg_body       the announcement's text
 */
CREATE TABLE ebms_message
 (message_id INTEGER          NOT NULL AUTO_INCREMENT PRIMARY KEY,
   sender_id INTEGER UNSIGNED NOT NULL,
 when_posted DATETIME         NOT NULL,
 msg_subject VARCHAR(256)     NOT NULL,
    msg_body TEXT             NOT NULL,
 FOREIGN KEY (sender_id) REFERENCES users (uid))
      ENGINE=InnoDB;

/*
 * Recipients designated to receive specific announcement messages.
 *
 * message_id     foreign key into the ebms_message table
 * recip_id       foreign key into Drupal's users table
 * when_read      date/time the recipient read the message in the EBMS
 *                (not his/her email client)
 */
CREATE TABLE ebms_message_recipient
 (message_id INTEGER          NOT NULL,
    recip_id INTEGER UNSIGNED NOT NULL,
   when_read DATETIME             NULL,
 PRIMARY KEY (message_id, recip_id),
 FOREIGN KEY (message_id) REFERENCES ebms_message (message_id),
 FOREIGN KEY (recip_id)   REFERENCES users (uid))
      ENGINE=InnoDB;

/*
 * Requests for hotel reservations.
 *
 * request_id     automatically generated primary key
 * requestor_id   foreign key into Drupal's users table
 * submitted      date/time the request was posted
 * meeting        meeting the user will be attending
 * checkin_date   date when reservation should start
 * checkout_date  date member will leave the hotel
 * processed      date/time request was processed (not currently used)
 * notes          optional additional information
 */
CREATE TABLE ebms_hotel_request
  (request_id INTEGER          NOT NULL AUTO_INCREMENT PRIMARY KEY,
 requestor_id INTEGER UNSIGNED NOT NULL,
    submitted DATETIME         NOT NULL,
      meeting VARCHAR(128)     NOT NULL,
 checkin_date DATE             NOT NULL,
checkout_date DATE             NOT NULL,
    processed DATETIME             NULL,
        notes TEXT                 NULL,
  FOREIGN KEY (requestor_id) REFERENCES users (uid))
       ENGINE=InnoDB;

/*
 * Request for reimbursement of expenses associated with a board meeting.
 *
 * request_id     automatically generated primary key
 * requestor_id   foreign key into Drupal's users table
 * submitted      date/time the request was posted
 * meeting        meeting for which the expenses were incurred
 * processed      date/time request was processed (not currently used)
 * notes          optional additional information
 */
CREATE TABLE ebms_reimbursement_request
 (request_id INTEGER          NOT NULL AUTO_INCREMENT PRIMARY KEY,
requestor_id INTEGER UNSIGNED NOT NULL,
   submitted DATETIME         NOT NULL,
     meeting INTEGER UNSIGNED NOT NULL,
   processed DATETIME             NULL,
       notes TEXT                 NULL,
 FOREIGN KEY (requestor_id) REFERENCES users (uid),
 FOREIGN KEY (meeting)      REFERENCES node (nid))
      ENGINE=InnoDB;

/*
 * Line item for reimbursement request.
 *
 * item_id        automatically generated primary key
 * request_id     foreign key into the ebms_reimbursement_request table
 * expense_date   text identification of date(s) expenses were incurred
 * amount         text amount of expense
 * description    what the expense was for
 */
CREATE TABLE ebms_reimbursement_item
    (item_id INTEGER      NOT NULL AUTO_INCREMENT PRIMARY KEY,
  request_id INTEGER      NOT NULL,
expense_date DATE         NOT NULL,
      amount VARCHAR(128) NOT NULL,
 description VARCHAR(128) NOT NULL,
 FOREIGN KEY (request_id) REFERENCES ebms_reimbursement_request (request_id))
      ENGINE=InnoDB;

/*
 * Attached file of documentation for expenses.
 *
 * receipt_id     automatically generated primary key
 * request_id     foreign key into the ebms_reimbursement_request table
 * file_id        foreign key into Drupal's file_managed table
 */
CREATE TABLE ebms_reimbursement_receipts
 (receipt_id INTEGER          NOT NULL AUTO_INCREMENT PRIMARY KEY,
  request_id INTEGER          NOT NULL,
     file_id INTEGER UNSIGNED NOT NULL,
 FOREIGN KEY (request_id) REFERENCES ebms_reimbursement_request (request_id),
 FOREIGN KEY (file_id)    REFERENCES file_managed (fid))
      ENGINE=InnoDB;

/*
 * Information about a requested report.
 *
 * request_id     automatically generated primary key
 * report_name    e.g., 'Hotel Reservation Requests'
 * requestor_id   foreign key into Drupal's users table
 * submitted      date/time the request was posted
 * parameters     information needed to generated the report, json-encoded
 */
CREATE TABLE ebms_report_request
 (request_id INTEGER          NOT NULL AUTO_INCREMENT PRIMARY KEY,
 report_name VARCHAR(40)      NOT NULL,
requestor_id INTEGER UNSIGNED NOT NULL,
   submitted DATETIME         NOT NULL,
  parameters TEXT             NOT NULL,
 FOREIGN KEY (requestor_id) REFERENCES users (uid))
      ENGINE=InnoDB;

/*
 * Meeting agenda.
 *
 * event_id       primary key and foreign key into node table for event node
 * agenda_doc     rich text document for agenda
 * when_posted    date/time agenda was first created
 * posted_by      foreign key into Drupal's users table
 * published      XXX
 * last_modified  optional date/time of last agenda changes
 * modified_by    foreign key into Drupal's users table (optionl)
 */
 CREATE TABLE ebms_agenda
    (event_id INTEGER  UNSIGNED NOT NULL PRIMARY KEY,
   agenda_doc LONGTEXT          NOT NULL,
  when_posted DATETIME          NOT NULL,
    posted_by INTEGER  UNSIGNED NOT NULL,
    published INTEGER           NOT NULL,
last_modified DATETIME              NULL,
  modified_by INTEGER  UNSIGNED     NULL,
  FOREIGN KEY (event_id)    REFERENCES node (nid),
  FOREIGN KEY (posted_by )  REFERENCES users (uid),
  FOREIGN KEY (modified_by) REFERENCES users (uid))
       ENGINE=InnoDB;

/*
 * Summary secondary page.
 *
 * page_id        automatically generated primary key
 * board_id       foreign key into ebms_board table
 * page_name      string identifying page content
 */
CREATE TABLE ebms_summary_page
    (page_id INTEGER      NOT NULL AUTO_INCREMENT PRIMARY KEY,
    board_id INTEGER      NOT NULL,
   page_name VARCHAR(255) NOT NULL,
 FOREIGN KEY (board_id) REFERENCES ebms_board (board_id))
      ENGINE=InnoDB;

/*
 * Link to HP summary on Cancer.gov.
 *
 * link_id        automatically generated primary key
 * page_id        foreign key into ebms_summary_page table
 * link_url       URL for the Cancer.gov page
 * link_label     display text for the link
 */
CREATE TABLE ebms_summary_link
    (link_id INTEGER      NOT NULL AUTO_INCREMENT PRIMARY KEY,
     page_id INTEGER      NOT NULL,
    link_url VARCHAR(255) NOT NULL,
  link_label VARCHAR(255) NOT NULL,
 FOREIGN KEY (page_id) REFERENCES ebms_summary_page (page_id))
      ENGINE=InnoDB;

/*
 * Summary supporting document.  Listed in table at the bottom of the
 * primary summary landing page for the board.
 *
 * board_id       foreign key into ebms_board table
 * doc_id         foreign key into ebms_doc table
 * archived       date/time document was suppressed from display
 * notes          optional comments on the posted document
 */
CREATE TABLE ebms_summary_supporting_doc
   (board_id INTEGER  NOT NULL,
      doc_id INTEGER  NOT NULL,
    archived DATETIME     NULL,
       notes TEXT         NULL,
 PRIMARY KEY (board_id, doc_id),
 FOREIGN KEY (board_id) REFERENCES ebms_board (board_id),
 FOREIGN KEY (doc_id)   REFERENCES ebms_doc (doc_id))
      ENGINE=InnoDB;

/*
 * Document posted by board manager for secondary summary page.
 *
 * page_id        foreign key into ebms_summary_page table
 * doc_id         foreign key into ebms_doc table
 * archived       date/time document was suppressed from display
 * notes          optional comments on the posted document
 */
CREATE TABLE ebms_summary_posted_doc
    (page_id INTEGER  NOT NULL,
      doc_id INTEGER  NOT NULL,
    archived DATETIME     NULL,
       notes TEXT         NULL,
 PRIMARY KEY (page_id, doc_id),
 FOREIGN KEY (page_id) REFERENCES ebms_summary_page (page_id),
 FOREIGN KEY (doc_id)  REFERENCES ebms_doc (doc_id))
      ENGINE=InnoDB;

/*
 * Document posted by board member for secondary summary page.
 *
 * page_id        foreign key into ebms_summary_page table
 * doc_id         foreign key into ebms_doc table
 * archived       date/time document was suppressed from display
 * notes          optional comments on the posted document
 */
CREATE TABLE ebms_summary_returned_doc
    (page_id INTEGER  NOT NULL,
      doc_id INTEGER  NOT NULL,
    archived DATETIME     NULL,
       notes TEXT         NULL,
 PRIMARY KEY (page_id, doc_id),
 FOREIGN KEY (page_id) REFERENCES ebms_summary_page (page_id),
 FOREIGN KEY (doc_id)  REFERENCES ebms_doc (doc_id))
      ENGINE=InnoDB;
