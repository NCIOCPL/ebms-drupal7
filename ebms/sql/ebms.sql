-- /* $Id$ */

/********************************************************
 * Drop all tables in reverse order to any references.
 ********************************************************/
DROP TABLE IF EXISTS ebms_article_topic;
DROP TABLE IF EXISTS ebms_temp_report;
DROP TABLE IF EXISTS ebms_publish_queue_flag;
DROP TABLE IF EXISTS ebms_publish_queue;
DROP TABLE IF EXISTS ebms_search;
DROP TABLE IF EXISTS ebms_summary_returned_doc;
DROP TABLE IF EXISTS ebms_summary_posted_doc;
DROP TABLE IF EXISTS ebms_summary_supporting_doc;
DROP TABLE IF EXISTS ebms_summary_link;
DROP TABLE IF EXISTS ebms_summary_page_topic;
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
DROP TABLE IF EXISTS ebms_member_wants_print;
DROP TABLE IF EXISTS ebms_packet_printed;
DROP TABLE IF EXISTS ebms_print_job;
DROP TABLE IF EXISTS ebms_print_status_type;
DROP TABLE IF EXISTS ebms_print_job_type;
DROP TABLE IF EXISTS ebms_packet_article;
DROP TABLE IF EXISTS ebms_packet_reviewer;
DROP TABLE IF EXISTS ebms_packet_summary;
DROP TABLE IF EXISTS ebms_packet;
DROP TABLE IF EXISTS ebms_agenda_meeting;
DROP TABLE IF EXISTS ebms_article_board_decision;
DROP TABLE IF EXISTS ebms_article_board_decision_value;
DROP TABLE IF EXISTS ebms_article_board_decision_member;
DROP TABLE IF EXISTS ebms_article_state_comment;
DROP TABLE IF EXISTS ebms_article_state;
DROP TABLE IF EXISTS ebms_article_state_type;
DROP TABLE IF EXISTS ebms_article_tag_comment;
DROP TABLE IF EXISTS ebms_article_tag;
DROP TABLE IF EXISTS ebms_article_tag_type;
DROP TABLE IF EXISTS ebms_import_action;
DROP TABLE IF EXISTS ebms_import_batch;
DROP TABLE IF EXISTS ebms_import_disposition;
DROP TABLE IF EXISTS ebms_journal;
DROP TABLE IF EXISTS ebms_not_list;
DROP TABLE IF EXISTS ebms_cycle;
DROP TABLE IF EXISTS ebms_article_author_cite;
DROP TABLE IF EXISTS ebms_article_author;
DROP TABLE IF EXISTS ebms_legacy_article_id;
DROP TABLE IF EXISTS ebms_ft_unavailable;
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
DROP TABLE IF EXISTS ebms_user;
SET sql_mode='NO_AUTO_VALUE_ON_ZERO';

/********************************************************
 * Clear out EBMS rows from Drupal tables.
 ********************************************************/
DELETE FROM users_roles WHERE uid > 1;

DELETE FROM users WHERE uid > 1;

DELETE FROM role WHERE name NOT IN ('anonymous user',
                                    'authenticated user',
                                    'administrator');

/********************************************************
 * Create all tables that are not standard Drupal tables.
 ********************************************************/

/*
 * Additional user information not stored in Drupal users table.
 *
 * user_id           foreign key into Drupal's users table
 * password_changed  date/time the user last changed his/her password
 */
     CREATE TABLE ebms_user
         (user_id INTEGER UNSIGNED NOT NULL PRIMARY KEY,
 password_changed DATETIME         NOT NULL,
      FOREIGN KEY (user_id) REFERENCES users (uid))
      ENGINE=InnoDB DEFAULT CHARSET=utf8;

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
   drop_flag INTEGER          NOT NULL DEFAULT 0,
 FOREIGN KEY (file_id) REFERENCES file_managed (fid))
      ENGINE=InnoDB DEFAULT CHARSET=utf8;

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
        ENGINE=InnoDB DEFAULT CHARSET=utf8;

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
      ENGINE=InnoDB DEFAULT CHARSET=utf8;

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
      ENGINE=InnoDB DEFAULT CHARSET=utf8;

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
      ENGINE=InnoDB DEFAULT CHARSET=utf8;

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
      ENGINE=InnoDB DEFAULT CHARSET=utf8;
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
      ENGINE=InnoDB DEFAULT CHARSET=utf8;

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
      ENGINE=InnoDB DEFAULT CHARSET=utf8;

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
      ENGINE=InnoDB DEFAULT CHARSET=utf8;


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
      ENGINE=InnoDB DEFAULT CHARSET=utf8;

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
 * nci_reviewer   NCI staff member primarily responsible for the topic
 * active_status  can only associate articles with active topics; old
 *                  topics remain in the database because articles may
 *                  be linked to them.
 */
CREATE TABLE ebms_topic
     (topic_id INTEGER          NOT NULL AUTO_INCREMENT PRIMARY KEY,
    topic_name VARCHAR(255)     NOT NULL UNIQUE,
      board_id INTEGER          NOT NULL,
  nci_reviewer INTEGER UNSIGNED NOT NULL,
 active_status ENUM ('A', 'I')  NOT NULL DEFAULT 'A',
 FOREIGN KEY (board_id) REFERENCES ebms_board (board_id),
 FOREIGN KEY (nci_reviewer) REFERENCES users (uid))
      ENGINE=InnoDB DEFAULT CHARSET=utf8;

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
      ENGINE=InnoDB DEFAULT CHARSET=utf8;

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
      ENGINE=InnoDB DEFAULT CHARSET=utf8;

/*
 * Articles about cancer and related topics.  Initially, all are from 
 * Pubmed but other sources could be added.
 *
 * These articles go through a series of steps to weed out the ones which
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
 *  imported_by     Userid of the person who performed the import.  This is
 *                    the original import only, not the update, if there was
 *                    one.
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
  imported_by       INTEGER UNSIGNED NOT NULL,
  source_data       LONGTEXT NULL,
  full_text_id      INTEGER UNSIGNED NULL,
  active_status     ENUM('A', 'D') NOT NULL DEFAULT 'A',
  FOREIGN KEY (imported_by) REFERENCES users(uid),
  FOREIGN KEY (full_text_id) REFERENCES file_managed (fid)
)
      ENGINE=InnoDB DEFAULT CHARSET=utf8;

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
      CREATE INDEX ebms_article_import_date_index
             ON ebms_article(import_date);
      CREATE INDEX ebms_article_published_date_index
             ON ebms_article(published_date);

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
    ENGINE=InnoDB DEFAULT CHARSET=utf8;

/*
 * Records articles for which we are unable to obtain the full
 * text.
 *
 *  article_id    Primary key, as well as foreign key into ebms_article
 *  flagged       Date/time the full text was marked as unavailable
 *  flagged_by    Foreign key into the Drupal users table
 *  comment       Optional notes explaining failure to obtain the full text
 */
CREATE TABLE ebms_ft_unavailable
 (article_id INTEGER      NOT NULL PRIMARY KEY,
     flagged DATETIME     NOT NULL,
  flagged_by INT UNSIGNED NOT NULL,
     comment TEXT             NULL,
 FOREIGN KEY (article_id) REFERENCES ebms_article (article_id))
      ENGINE=InnoDB DEFAULT CHARSET=utf8;

/*
 * Authors of articles.
 *
 * These are as they appear in the XML records.  Name strings are not
 * unique in the NLM data (or in real life) and searches for an author 
 * may retrieve cites by different people with the same names in Pubmed.
 *
 * One real person may also have more than one entry in this table,
 * depending on how the person was cited in his publications and by Pubmed.
 * Some examples: "Doe, John A"; "Doe J"; "Doe JA"
 *
 * An author should always have either a collective_name (for a corporate
 * author) or a last_name (for a personal author).  However, the data
 * does contain occasional surprises and there may sometimes be an author
 * with a forename but no last_name.  Rather than throw exceptions and/or
 * require NCI staff to correct Pubmed data, we'll just accept whatever
 * we get.
 *
 * Searching will be tricky and noisy.  All text will be converted to plain
 * ASCII and search expressions should be converted the same way.  This
 * is to assist users with regular American keyboards who are not necessarily
 * familiar with multiple language techniques.
 *
 *  author_id       Unique ID for this character string, auto generated.
 *  last_name       Surname, called LastName in NLM XML.
 *  forename        Usually first name + optional middle initial.
 *                    But NLM can put other things in this field, e.g.:
 *                      "R Bryan",  "Deborah Ks",  "J"
 *                      "Maria del Refugio"
 *  initials        Usually first letter of first name + optional first letter
 *                    of middle name.  But again there are outliers:
 *                      "Mdel R" for Maria del Refugio Gonzales-Losa;
 *                      "Nde S" for Nicholas de Saint Aubain Somerhausen
 *  collective_name Corporate name alternative to personal names.
 */
CREATE TABLE ebms_article_author (
    author_id       INT AUTO_INCREMENT PRIMARY KEY,
    last_name       VARCHAR(255) CHARACTER SET ASCII NULL,
    forename        VARCHAR(128) CHARACTER SET ASCII NULL,
    initials        VARCHAR(128) CHARACTER SET ASCII NULL,
    collective_name VARCHAR(767) CHARACTER SET ASCII NULL
)
    ENGINE = InnoDB DEFAULT CHARSET=utf8;

    -- Two ways to search, use last + first name, or last + initials
    CREATE UNIQUE INDEX ebms_author_full_index 
           ON ebms_article_author (last_name, forename, initials);
    CREATE INDEX ebms_author_initials_index 
           ON ebms_article_author (last_name, initials);

    -- Collective names are separate searchable
    CREATE UNIQUE INDEX ebms_author_collective_index 
           ON ebms_article_author (collective_name);


/*
 * Join the authors with the citations.
 *
 * Notes:
 *  Use the primary key to find all authors of an article, in the order
 *  they appeared in the article citation.
 *
 *  It's important to cite authors in the correct order of their
 *  appearance in an article.
 *
 *  article_id      Unique ID in article table.
 *  cite_order      Order of this author in article , e.g., first author,
 *                   second author, etc.  Origin 1.
 *  author_id       Unique ID in author table.
 */
CREATE TABLE ebms_article_author_cite (
    article_id      INT NOT NULL,
    cite_order      INT NOT NULL,
    author_id       INT NOT NULL,
    PRIMARY KEY (article_id, cite_order, author_id),
    FOREIGN KEY (author_id) REFERENCES ebms_article_author(author_id),
    FOREIGN KEY (article_id) REFERENCES ebms_article(article_id)
)
    ENGINE = InnoDB DEFAULT CHARSET=utf8;

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
 * Note:
 *   This table is so small that a date index might not actually optimize it.
 *
 * cycle_id       Automatically generated primary key
 * cycle_name     Unique name for the cycle (e.g., 'November 2011')
 * start_date     Datetime of start.  Always order by start_date to guarantee
 *                 retrieval in date order since cycle_ids could be created
 *                 out of order due to conversion from old CMS or 
 *                 for other reasons.
 */
CREATE TABLE ebms_cycle
   (cycle_id INTEGER     NOT NULL AUTO_INCREMENT PRIMARY KEY,
  cycle_name VARCHAR(40) NOT NULL UNIQUE,
  start_date DATETIME    NOT NULL UNIQUE)
      ENGINE=InnoDB DEFAULT CHARSET=utf8;

/*
 * A list of all known journals.
 *
 *  source          Name of source, 'Pubmed' predominates.
 *  source_jrnl_id  If source = 'Pubmed': then NLM unique journal id.
 *                    Else: to be determined.
 *  jrnl_title      Full journal title at time of import or update.
 *  brf_jrnl_title  Journal title abbreviation found in article record.
 */
CREATE TABLE ebms_journal (
  source            VARCHAR(32) NOT NULL,
  source_jrnl_id    VARCHAR(32) NOT NULL,
  jrnl_title        VARCHAR(512) CHARACTER SET ASCII NOT NULL,
  brf_jrnl_title    VARCHAR(127) NULL,
PRIMARY KEY (source, source_jrnl_id, jrnl_title, brf_jrnl_title)
)
      ENGINE=InnoDB DEFAULT CHARSET=utf8;
      
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
 *       remove it from this table.  If it's listed again, we add it back
 *       again.
 *       Is that reasonable?
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
    ENGINE = InnoDB DEFAULT CHARSET=utf8;

    CREATE INDEX ebms_not_journal_index 
        ON ebms_not_list(board_id, source, source_jrnl_id);

/*
 * ebms_import_disposition
 * 
 * Control table for import_action.disposition.  This is a static set
 * of values that describe what happened to an article that was presented
 * to the system in an import batch.  It might have been: (1) imported,
 * (2) rejected as a duplicate; (3) assigned a new summary topic to an
 * article already in the system; etc.
 * 
 *  disposition_id          Unique ID of the citation.
 *  text_id                 Human readable invariant name for code refs.
 *  disposition_name        Human readable display name.
 *  description             Fuller explanation of disposition.
 *  active_status           'A'ctive or 'I'nactive - don't use any more.
 */
CREATE TABLE ebms_import_disposition (
    disposition_id          INTEGER AUTO_INCREMENT PRIMARY KEY,
    text_id                 VARCHAR(16) NOT NULL UNIQUE,
    disposition_name        VARCHAR(32) NOT NULL UNIQUE,
    description             VARCHAR(2048) NOT NULL,
    active_status           ENUM ('A', 'I') NOT NULL DEFAULT 'A'
)
    ENGINE = InnoDB DEFAULT CHARSET=utf8;

    -- The required disposition values
    INSERT ebms_import_disposition (text_id, disposition_name, description) 
      VALUES ('imported', 'Imported', 
      'First time import into the database');
    INSERT ebms_import_disposition (text_id, disposition_name, description) 
      VALUES ('reviewReady', 'Ready for review',
      'Imported as ready for review');
    INSERT ebms_import_disposition (text_id, disposition_name, description) 
      VALUES ('notListed', 'NOT listed',
      'Imported but automatically rejected because the journal was NOT listed');
    INSERT ebms_import_disposition (text_id, disposition_name, description) 
      VALUES ('duplicate', 'Duplicate, not imported', 
      'Article already in database with same topic.  Not re-imported.');
    INSERT ebms_import_disposition (text_id, disposition_name, description) 
      VALUES ('topicAdded', 'Summary topic added',
      'Article already in database.  New summary topic added.');
    INSERT ebms_import_disposition (text_id, disposition_name, description) 
      VALUES ('replaced', 'Replaced',
      'Article record replaced from updated, newly downloaded, source record');
    INSERT ebms_import_disposition (text_id, disposition_name, description) 
      VALUES ('error', 'Error',
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
 *  not_list        'Y' = NOT list was used, 'N' = no NOT list.
 *  input_type      One of 'R'egular, 'F'ast track, 'S'pecial search.
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
    not_list        ENUM ('Y', 'N') NOT NUlL DEFAULT 'Y',
    input_type      ENUM ('R', 'F', 'S') NOT NULL DEFAULT 'R',
    article_count   INT NOT NULL,
    comment         VARCHAR(2048) NULL,
    FOREIGN KEY (topic_id) REFERENCES ebms_topic(topic_id),
    FOREIGN KEY (cycle_id) REFERENCES ebms_cycle(cycle_id),
    FOREIGN KEY (user_id)  REFERENCES users(uid)
)
    ENGINE = InnoDB DEFAULT CHARSET=utf8;

/* 
 * One row for each disposition of a citation in an import batch.
 * See ebms_import_disposition.
 *
 * A single article might have more than one import disposition.  For example,
 * a record imported from NLM might already be in the database but not with
 * the same topic as used in this import batch, and with a different cycle_id.
 * It might also be a later record from NLM.  In such a case there are
 * two import disposition values - "Summary topic added" and "Replaced".
 *
 *  action_id       Automatically generated primary key.
 *  source_id       Unique ID of the citation within the source database.
 *                    We don't always have an article_id here because some 
 *                    articles in a batch may not have actually been imported.
 *  article_id      Unique ID of article row, if we have one.
 *  import_batch_id Unique ID of the batch.
 *  disposition_id  What was done with the imported cite, one of the
 *                    ebms_import_disposition.disposition_id values.
 *  message         Probably only used for error messages on failed imports.
 */
CREATE TABLE ebms_import_action (
    action_id          INTEGER NOT NULL AUTO_INCREMENT PRIMARY KEY,
    source_id          VARCHAR(32) NOT NULL,
    article_id         INT NULL,
    import_batch_id    INT NOT NULL,
    disposition_id     INT NOT NULL,
    message            VARCHAR(400) NULL,
    FOREIGN KEY (article_id) REFERENCES ebms_article(article_id),
    FOREIGN KEY (import_batch_id) REFERENCES ebms_import_batch(import_batch_id),
    FOREIGN KEY (disposition_id)
        REFERENCES ebms_import_disposition(disposition_id)
)
    ENGINE = InnoDB DEFAULT CHARSET=utf8;


/*
 * Control values for recording processing states in the ebms_article_state
 * table.
 *
 *  state_id            Unique ID of the state value.
 *  state_name          Human readable name.
 *  description         Longer, descriptive help text.
 *  completed           'Y' = article in this state requires no further
 *                        processing.
 *  active_status       'A'ctive or 'I'nactive.
 *  sequence            The sequence order of states in workflows.
 */
CREATE TABLE ebms_article_state_type (
    state_id            INTEGER NOT NULL AUTO_INCREMENT PRIMARY KEY,
    state_text_id       VARCHAR(32) NULL UNIQUE,
    state_name          VARCHAR(64) NOT NULL UNIQUE,
    description         VARCHAR(2048) NOT NULL,
    completed           ENUM('Y', 'N') NOT NULL DEFAULT 'N',
    active_status       ENUM('A', 'I') NOT NULL DEFAULT 'A',
    sequence            INTEGER NULL
)
    ENGINE=InnoDB DEFAULT CHARSET=utf8;

    -- States that an article can be in in the review process
    INSERT ebms_article_state_type 
        (state_text_id, state_name, description, sequence, completed)
        VALUES ('ReadyInitReview', 'Ready for initial review', 
        'Article is associated with a summary topic, '
        'typically by a new import or an attempted import of a duplicate.  '
        'It is now ready for initial review', 10, 'N');
    INSERT ebms_article_state_type 
        (state_text_id, state_name, description, sequence, completed)
        VALUES ('RejectJournalTitle', 'Rejected by NOT list', 
        'Article appeared in a "NOT listed" journal, rejected without review',
        20, 'Y');
    INSERT ebms_article_state_type 
        (state_text_id, state_name, description, sequence, completed)
        VALUES ('RejectInitReview', 'Rejected in initial review', 
        'Rejected in initial review, before publication to board managers', 
        30, 'Y');
    INSERT ebms_article_state_type 
        (state_text_id, state_name, description, sequence, completed)
        VALUES ('PassedInitReview', 'Passed initial review', 
        'Article ready for "publishing" for board manager review', 30, 'N');
    INSERT ebms_article_state_type 
        (state_text_id, state_name, description, sequence, completed)
        VALUES ('Published', 'Published', 
        'Article "published" for board manager review', 40, 'N');
    INSERT ebms_article_state_type 
        (state_text_id, state_name, description, sequence, completed)
        VALUES ('RejectBMReview', 'Rejected by Board Manager', 
        'Board manager rejected article', 50, 'Y');
    INSERT ebms_article_state_type 
        (state_text_id, state_name, description, sequence, completed)
        VALUES ('PassedBMReview', 'Passed Board Manager', 
        'Board manager accepted article for further review', 50, 'N');
    INSERT ebms_article_state_type 
        (state_text_id, state_name, description, sequence, completed)
        VALUES ('RejectFullReview', 'Rejected after full text review',
        'Full text examined at OCE, article rejected', 60, 'Y');
    INSERT ebms_article_state_type 
        (state_text_id, state_name, description, sequence, completed)
        VALUES ('PassedFullReview', 'Passed full text review',
        'Full text examined at OCE, article approved for board member review', 
        60, 'N');
    INSERT ebms_article_state_type 
        (state_text_id, state_name, description, sequence, completed)
        VALUES ('FYI', 'Flagged as FYI',
        'Article is being sent out without being linked to a specific topic',
        60, 'Y');
    INSERT ebms_article_state_type 
        (state_text_id, state_name, description, sequence, completed)
        VALUES ('FullEnd', 'No further action',
        'Decision after board member review is do not discuss.  '
        'Do not put on agenda.',
        70, 'Y');
    INSERT ebms_article_state_type 
        (state_text_id, state_name, description, sequence, completed)
        VALUES ('NotForAgenda', 'Minor changes not for Board review',
        'Changes may be made to summary but no board meeting required.  '
        'Do not put on agenda.',
        70, 'N');
    INSERT ebms_article_state_type 
        (state_text_id, state_name, description, sequence, completed)
        VALUES ('AgendaNoPaprChg', 
		'Summary changes for Board review (no paper for discussion)',
        'Show this on the picklist of articles that can be added to agenda.  '
        'Summary changes have been proposed, but no paper exists.',
        70, 'N');
    INSERT ebms_article_state_type 
        (state_text_id, state_name, description, sequence, completed)
        VALUES ('AgendaFutureChg', 'Paper and summary changes for discussion',
        'Show this on the picklist of articles that can be added to agenda.  '
        'Summary changes have been proposed.',
        70, 'N');
    INSERT ebms_article_state_type 
        (state_text_id, state_name, description, sequence, completed)
        VALUES ('AgendaBoardDiscuss', 'Paper for Board discussion',
        'Show this on the picklist of articles that can be added to agenda.  '
        'No changes proposed, discuss with Board.',
        70, 'N');
    INSERT ebms_article_state_type 
        (state_text_id, state_name, description, sequence, completed)
        VALUES ('AgendaWrkGrpDiscuss', 'Paper for Working Group discussion',
        'Show this on the picklist of articles that can be added to agenda.  '
        'No changes proposed, discuss with Working Group.',
        70, 'N');
    INSERT ebms_article_state_type 
        (state_text_id, state_name, description, sequence, completed)
        VALUES ('OnAgenda', 'On agenda',
        'Article is on the agenda for an upcoming meeting.',
        80, 'N');
    INSERT ebms_article_state_type 
        (state_text_id, state_name, description, sequence, completed)
        VALUES ('FinalBoardDecision', 'Final board decision',
        'Article was discussed at a board meeting and a decision was reached',
        90, 'Y');


/*
 * Processing states that an article is, or has been, in.
 *
 *  article_state_id   Automatically generated primary key
 *  article_id         Unique ID in article table.
 *  state_id           ID of the state that this row records.
 *  board_id           Board for which this state is set.
 *  topic_id           Summary topic for which this article state is set.
 *  user_id            ID of the user that put the article in this state.
 *  status_dt          Date and time the row/state was created.
 *  active_status      Set to 'I' if the state row was a mistake, or no
 *                      longer applicable because of later events.
 *  current            Set to 'Y' if this is the most recent row in the table
 *                     for a given article/topic combination.  Only applied
 *                     to state rows created after conversion from the legacy
 *                     system; default is 'N'.
 */
CREATE TABLE ebms_article_state (
    article_state_id  INTEGER NOT NULL AUTO_INCREMENT PRIMARY KEY,
    article_id        INTEGER NOT NULL,
    state_id          INTEGER NOT NULL,
    board_id          INTEGER NOT NULL,
    topic_id          INTEGER NOT NULL,
    user_id           INTEGER UNSIGNED NOT NULL,
    status_dt         DATETIME NOT NULL,
    active_status     ENUM('A','I') NOT NULL DEFAULT 'A',
    current           ENUM('Y','N') NOT NULL DEFAULT 'N',
    FOREIGN KEY (article_id) REFERENCES ebms_article(article_id),
    FOREIGN KEY (board_id)   REFERENCES ebms_board(board_id),
    FOREIGN KEY (topic_id)   REFERENCES ebms_topic(topic_id),
    FOREIGN KEY (state_id)   REFERENCES ebms_article_state_type(state_id),
    FOREIGN KEY (user_id)    REFERENCES users(uid)
)
    ENGINE=InnoDB DEFAULT CHARSET=utf8;

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
    CREATE INDEX ebms_current_article_state_article_index
           ON ebms_article_state(current, article_id, state_id);
    CREATE INDEX ebms_article_state_date_index
           ON ebms_article_state(status_dt, article_id, state_id,
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
    comment             TEXT NOT NULL,

    FOREIGN KEY (article_state_id) REFERENCES
                ebms_article_state(article_state_id),
    FOREIGN KEY (user_id) REFERENCES users(uid)
)
    ENGINE=InnoDB DEFAULT CHARSET=utf8;

    -- Usually retrieved in association with state row, in date order
    CREATE INDEX ebms_article_state_comment_state_index ON
           ebms_article_state_comment(article_state_id, comment_dt);


/*
 * Control values for recording descriptive tags in the ebms_article_tag
 * table.
 *
 *  tag_id              Unique ID of the tag value.
 *  text_id             Human readable invariant name for code refs.
 *  tag_name            Human readable name.
 *  description         Longer, descriptive help text.
 *  topic_required      'Y' = rows with this state must have a non-NULL
 *                        topic_id.
 *  active_status       'A'ctive or 'I'nactive.
 */
CREATE TABLE ebms_article_tag_type (
    tag_id              INTEGER NOT NULL AUTO_INCREMENT PRIMARY KEY,
    text_id             VARCHAR(16) NOT NULL UNIQUE,
    tag_name            VARCHAR(64) NOT NULL UNIQUE,
    description         VARCHAR(2048) NOT NULL,
    topic_required      ENUM('Y', 'N') NOT NULL DEFAULT 'N',
    active_status       ENUM('A', 'I') NOT NULL DEFAULT 'A'
)
    ENGINE=InnoDB DEFAULT CHARSET=utf8;

    /* These are partly for illustration.  They may not last */
    INSERT ebms_article_tag_type 
        (text_id, tag_name, description, topic_required)
        VALUES ('q_search', 'Questionable search',
        'This article was imported as a result of a search, but the article '
        'appears to be out of scope and the search '
        'criteria may have been too broad.', 'N');
    INSERT ebms_article_tag_type 
        (text_id, tag_name, description, topic_required)
        VALUES ('q_init_review', 'Borderline initial review',
        'This was examined in initial review but no judgment made.  It was a '
        'borderline case.  Look at it again later.', 'N');
    INSERT ebms_article_tag_type 
        (text_id, tag_name, description, topic_required)
        VALUES ('q_bm_review', 'Borderline board manager review',
        'This was examined by a board manager but no judgment made.  It was a '
        'borderline case.  Look at it again later.  A specific topic must '
        'be identified.', 'Y');
    INSERT ebms_article_tag_type 
        (text_id, tag_name, description, topic_required)
        VALUES ('i_fasttrack', 'Import fast track',
        'The article was imported as part of a fast track import process.  '
        'A specific topic must be identified.', 'Y');
    INSERT ebms_article_tag_type 
        (text_id, tag_name, description, topic_required)
        VALUES ('i_specialsearch', 'Import special search',
        'The article was imported as part of a special search import '
        'process.  A specific topic must be identified.', 'Y');
    INSERT ebms_article_tag_type 
        (text_id, tag_name, description, topic_required)
        VALUES ('conversion_note', 'Legacy conversion note',
        'Information about the article recorded during conversion '
        'from the legacy CiteMS', 'N');

/*
 * Descriptive tags that have been attached to an article.
 *
 *  article_tag_id     Automatically generated primary key
 *  article_id         Unique ID in article table.
 *  tag_id             ID of the tag that this row records.
 *  topic_id           Optional (depending on tag type) topic for which this
 *                      tag was assigned.
 *  user_id            ID of the user who attached the tag to this article.
 *  tag_dt             Date and time the row/tag was created.
 *  active_status      Set to 'I' if the tag assignment was a mistake, or no
 *                      longer applicable because of later events.
 */
CREATE TABLE ebms_article_tag (
    article_tag_id    INTEGER NOT NULL AUTO_INCREMENT PRIMARY KEY,
    article_id        INTEGER NOT NULL,
    tag_id            INTEGER NOT NULL,
    topic_id          INTEGER NULL,
    user_id           INTEGER UNSIGNED NOT NULL,
    tag_dt            DATETIME NOT NULL,
    active_status     ENUM('A','I') NOT NULL DEFAULT 'A',
    FOREIGN KEY (article_id) REFERENCES ebms_article(article_id),
    FOREIGN KEY (tag_id)     REFERENCES ebms_article_tag_type(tag_id),
    FOREIGN KEY (topic_id)   REFERENCES ebms_topic(topic_id),
    FOREIGN KEY (user_id)    REFERENCES users(uid)
)
    ENGINE=InnoDB DEFAULT CHARSET=utf8;

    -- Search for articles by article, tag, or topic
    CREATE INDEX ebms_article_tag_article_index
           ON ebms_article_tag(article_id, tag_id, active_status);
    CREATE INDEX ebms_article_tag_tag_index
           ON ebms_article_tag(tag_id, topic_id, article_id, active_status);
    CREATE INDEX ebms_article_tag_topic_index
           ON ebms_article_tag(topic_id, tag_id, article_id, active_status);

/*
 * One or more optional comments can be associated with an 
 * ebms_article_tag row.  The structure and concepts are identical to
 * article_state_comments.
 *
 * Comments are immutable.  To change one's mind about the contents of
 * a comment, a user can post another comment on the same tag row.  To
 * eliminate all comments, mark the ebms_article_tag row inactive.
 *
 *  comment_id          Unique auto-generated row ID of this comment.
 *  article_tag_id      Of the article tag row this comments on.
 *  user_id             Of user posting the comment.
 *  comment_dt          Datetime comment was recorded.
 *  comment             Free text.
 */
CREATE TABLE ebms_article_tag_comment (
    comment_id          INTEGER NOT NULL AUTO_INCREMENT PRIMARY KEY,
    article_tag_id      INTEGER NOT NULL,
    user_id             INTEGER UNSIGNED NOT NULL,
    comment_dt          DATETIME NOT NULL,
    comment             TEXT NOT NULL,

    FOREIGN KEY (article_tag_id) REFERENCES ebms_article_tag(article_tag_id),
    FOREIGN KEY (user_id)        REFERENCES users(uid)
)
    ENGINE=InnoDB DEFAULT CHARSET=utf8;

    -- Usually retrieved in association with article_tag row, in date order
    CREATE INDEX ebms_article_tag_comment_tag_index ON
           ebms_article_tag_comment(article_tag_id, comment_dt);


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
    ENGINE=InnoDB DEFAULT CHARSET=utf8;
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
    ENGINE=InnoDB DEFAULT CHARSET=utf8;

/*
 * Record of the board members affiliated with a particular board decision.
 * The date of the decision entry can be found by looking up article_state_id
 * in ebms_article_state.
 *
 *  article_state_id   foreign key into ebms_article_state table
 *  uid                foreign key into users table
 */
CREATE TABLE ebms_article_board_decision_member
( article_state_id INTEGER      NOT NULL,
           uid INT UNSIGNED NOT NULL,
PRIMARY KEY (article_state_id, uid),           
FOREIGN KEY (article_state_id)
    REFERENCES ebms_article_state(article_state_id),
FOREIGN KEY (uid) 
    REFERENCES users(uid)
)
    ENGINE=InnoDB DEFAULT CHARSET=utf8;
    
/*
 * Collection of articles on a given topic assigned for board member review.
 *
 * Board members are regularly assigned sets of published articles to review
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
 * created_at     date/time the packet came into existence
 * packet_title   how the packet should be identified in lists of packets
 * last_seen      when the board manager last saw the feedback for the packet
 * active_status  'A'ctive or 'I'nactive.
 */
 CREATE TABLE ebms_packet
   (packet_id INTEGER          NOT NULL AUTO_INCREMENT PRIMARY KEY,
     topic_id INTEGER          NOT NULL,
   created_by INTEGER UNSIGNED NOT NULL,
   created_at DATETIME         NOT NULL,
 packet_title VARCHAR(255)     NOT NULL,
    last_seen DATETIME             NULL,
active_status ENUM ('A', 'I')  NOT NULL DEFAULT 'A',
  FOREIGN KEY (topic_id)   REFERENCES ebms_topic (topic_id),
  FOREIGN KEY (created_by) REFERENCES users (uid))
       ENGINE=InnoDB DEFAULT CHARSET=utf8;

/*
 * Types of supported packet printing jobs.
 *
 *  print_job_type_id   What kind of job this is.
 *  description         Purely for documentation purposes.
 */
CREATE TABLE ebms_print_job_type (
    print_job_type_id   VARCHAR(16) PRIMARY KEY,
    description         VARCHAR(2048) NOT NULL
)
    ENGINE=InnoDB DEFAULT CHARSET=utf8;

    -- Predefined types
    INSERT ebms_print_job_type (print_job_type_id, description)
      VALUES ('board', 
       'Job run for all board members that receive printouts on one board');
    INSERT ebms_print_job_type (print_job_type_id, description)
      VALUES ('package', 
       'Printing all packets (one package) for one user on one board');
    INSERT ebms_print_job_type (print_job_type_id, description)
      VALUES ('packet', 'Printing a single packet for a user');

    -- For future use
    INSERT ebms_print_job_type (print_job_type_id, description)
      VALUES ('meeting', 'Printing all documents for a board meeting');

/*
 * Types of packet printing job status values / outcomes.
 *
 *  print_job_status_id     What kind of job this is.
 *  description             Purely for documentation purposes.
 */
CREATE TABLE ebms_print_status_type (
    print_job_status_id     VARCHAR(16) PRIMARY KEY,
    description             VARCHAR(2048) NOT NULL
)
    ENGINE=InnoDB DEFAULT CHARSET=utf8;

    -- Recognized statuss
    INSERT ebms_print_status_type (print_job_status_id, description)
      VALUES('queued', 'Job parameters entered, job is ready to run');
    INSERT ebms_print_status_type (print_job_status_id, description)
      VALUES('in-process', 'Job started, not yet reached normal finish');
    INSERT ebms_print_status_type (print_job_status_id, description)
      VALUES('failure', 'Job failed.  May be more info in comment');
    INSERT ebms_print_status_type (print_job_status_id, description)
      VALUES('success', 'Job completed successfully');

/*
 * Records print jobs.
 *
 *  print_job_id        Unique ID of this job.
 *  old_job_id          Unique ID of a previous job if this job is a reprint
 *                       of that one.  Else null.
 *  user_id             Drupal user ID of person who requested the printing.
 *  print_job_type_id   Why was this job printed.
 *  packet_start_dt     Only include packets created on or after this datetime.
 *  packet_end_dt       Only include packets created before this datetime.
 *  print_dt            Date time of actual printing.
 *  board_id            ID of board for which job was run.
 *  board_member_id     ID of board member for 'package' job type, optional
 *                       for 'packet' type and null for 'board' type.
 *  packet_id           For job_type 'packet', just printing this one.
 *  status              One of the legal status values.
 *  mode                'live' produces files and updates the database.
 *                      'test' produces files but no update.
 *                      'report' just produces the report listing files.
 *  comment             Optional free text comment about job.
 */
CREATE TABLE ebms_print_job (
    print_job_id        INTEGER NOT NULL AUTO_INCREMENT PRIMARY KEY,
    old_job_id          INTEGER NULL,
    user_id             INTEGER UNSIGNED NOT NULL,
    print_job_type_id   VARCHAR(16) NOT NULL,
    packet_start_dt     DATETIME NULL,
    packet_end_dt       DATETIME NULL,
    print_dt            DATETIME NULL,
    board_id            INTEGER NULL,
    board_member_id     INTEGER UNSIGNED NULL,
    packet_id           INTEGER NULL,
    mode                ENUM('live', 'test', 'report') NOT NULL,
    status              VARCHAR(16) NOT NULL,
    comment             VARCHAR(2048) NULL,
    FOREIGN KEY (old_job_id) REFERENCES ebms_print_job (print_job_id),
    FOREIGN KEY (user_id)  REFERENCES users (uid),
    FOREIGN KEY (print_job_type_id) 
        REFERENCES ebms_print_job_type (print_job_type_id),
    FOREIGN KEY (board_id) REFERENCES ebms_board (board_id),
    FOREIGN KEY (board_member_id) REFERENCES ebms_board_member(user_id),
    FOREIGN KEY (packet_id) REFERENCES ebms_packet(packet_id),
    FOREIGN KEY (status) 
        REFERENCES ebms_print_status_type(print_job_status_id)
)
    ENGINE=InnoDB DEFAULT CHARSET=utf8;

/*
 * Records what has been printed for a board member.  This is used to
 * avoid printing something twice, to answer a board member's questions
 * about whether and when something was printed for him, and possibly to
 * provide management information reports.
 *
 *  board_member_id     The ID of the board member for whom printing occurred.
 *  packet_id           The ID of the packet that was printed.
 *                       If a single document is printed apart from a packet,
 *                       that is not recorded.  It's only packets that we
 *                       record.
 *  print_job_id        The ID of the job for which the printout occurred.
 */
CREATE TABLE ebms_packet_printed (
    board_member_id     INTEGER UNSIGNED NOT NULL,
    packet_id           INTEGER NOT NULL,
    print_job_id        INTEGER NOT NULL,
    PRIMARY KEY (board_member_id, packet_id, print_job_id),
    FOREIGN KEY (board_member_id) REFERENCES ebms_board_member(user_id),
    FOREIGN KEY (packet_id) REFERENCES ebms_packet(packet_id),
    FOREIGN KEY (print_job_id) REFERENCES ebms_print_job(print_job_id)
)
    ENGINE=InnoDB DEFAULT CHARSET=utf8;

/*
 * Record of which board members want printed packets.
 *
 * Add a row here when a board member wants printouts.  Set the end_dt
 * if and only if he no longer wants printouts after that date.  This also
 * provides a historical record of who wanted printouts in the past.
 *
 *  board_member_id     Who wanted printouts.
 *  start_dt            When they should start.
 *  end_dt              When no longer needed.  Null if still needed.
 */
CREATE TABLE ebms_member_wants_print (
    board_member_id     INTEGER UNSIGNED NOT NULL,
    start_dt            DATE NOT NULL,
    end_dt              DATE null,
    PRIMARY KEY (board_member_id, start_dt),
    FOREIGN KEY (board_member_id) REFERENCES ebms_board_member(user_id)
)
    ENGINE=InnoDB DEFAULT CHARSET=utf8;

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
      ENGINE=InnoDB DEFAULT CHARSET=utf8;

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
      ENGINE=InnoDB DEFAULT CHARSET=utf8;

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
 * archived       date/time article was suppressed from review display
 */
CREATE TABLE ebms_packet_article
 (article_id INTEGER      NOT NULL,
   packet_id INTEGER      NOT NULL,
   drop_flag INTEGER      NOT NULL DEFAULT 0,
    archived DATETIME         NULL,
 PRIMARY KEY (packet_id, article_id),
 FOREIGN KEY (article_id) REFERENCES ebms_article (article_id),
 FOREIGN KEY (packet_id)  REFERENCES ebms_packet (packet_id))
      ENGINE=InnoDB DEFAULT CHARSET=utf8;

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
      ENGINE=InnoDB DEFAULT CHARSET=utf8;

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
      ENGINE=InnoDB DEFAULT CHARSET=utf8;

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
      ENGINE=InnoDB DEFAULT CHARSET=utf8;

INSERT INTO ebms_review_rejection_value (value_name, value_pos)
     VALUES ('Already cited in the PDQ summary', 1);
INSERT INTO ebms_review_rejection_value (value_name, value_pos)
     VALUES ('Not relevant to the PDQ summary topic', 2);
INSERT INTO ebms_review_rejection_value (value_name, value_pos)
     VALUES ('Findings not clinically important', 3);
INSERT INTO ebms_review_rejection_value (value_name, value_pos)
     VALUES ('Preliminary findings; need confirmation', 4);
INSERT INTO ebms_review_rejection_value (value_name, value_pos)
     VALUES ('Provides no new information/novel findings', 5);
INSERT INTO ebms_review_rejection_value (value_name, value_pos, extra_info)
     VALUES ('Review/expert opinion/commentary', 6, 'no new primary data');
INSERT INTO ebms_review_rejection_value (value_name, value_pos, extra_info)
     VALUES ('Inadequate study population', 7,
       'small number of patients; underpowered study; accrual target not met');
INSERT INTO ebms_review_rejection_value (value_name, value_pos)
     VALUES ('Inadequate follow-up', 8);
INSERT INTO ebms_review_rejection_value (value_name, value_pos)
     VALUES ('Inappropriate interpretation of subgroup analyses', 9);
INSERT INTO ebms_review_rejection_value (value_name, value_pos, extra_info)
     VALUES ('Inappropriate statistical analysis', 10,
             'incorrect tests; lack of intent-to-treat analysis');
INSERT INTO ebms_review_rejection_value (value_name, value_pos)
     VALUES ('Inappropriate study design', 11);
INSERT INTO ebms_review_rejection_value (value_name, value_pos)
     VALUES ('Missing/incomplete outcome data; major protocol deviations', 12);
INSERT INTO ebms_review_rejection_value (value_name, value_pos)
     VALUES ('Randomized trial with flawed or insufficiently described randomization process', 13);
INSERT INTO ebms_review_rejection_value (value_name, value_pos, extra_info)
     VALUES ('Unvalidated outcome measure(s) used', 14,
             'e.g., unvalidated surrogate endpoint[s]');
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
      ENGINE=InnoDB DEFAULT CHARSET=utf8;

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
      ENGINE=InnoDB DEFAULT CHARSET=utf8;

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
      ENGINE=InnoDB DEFAULT CHARSET=utf8;

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
      ENGINE=InnoDB DEFAULT CHARSET=utf8;

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
      ENGINE=InnoDB DEFAULT CHARSET=utf8;

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
       ENGINE=InnoDB DEFAULT CHARSET=utf8;

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
      ENGINE=InnoDB DEFAULT CHARSET=utf8;

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
      ENGINE=InnoDB DEFAULT CHARSET=utf8;

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
      ENGINE=InnoDB DEFAULT CHARSET=utf8;

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
      ENGINE=InnoDB DEFAULT CHARSET=utf8;

/*
 * Meeting agenda.
 *
 * event_id       primary key and foreign key into node table for event node
 * agenda_doc     rich text document for agenda
 * when_posted    date/time agenda was first created
 * posted_by      foreign key into Drupal's users table
 * published      when the board manager made the agenda visible
 * last_modified  optional date/time of last agenda changes
 * modified_by    foreign key into Drupal's users table (optional)
 */
 CREATE TABLE ebms_agenda
    (event_id INTEGER  UNSIGNED NOT NULL PRIMARY KEY,
   agenda_doc LONGTEXT          NOT NULL,
  when_posted DATETIME          NOT NULL,
    posted_by INTEGER  UNSIGNED NOT NULL,
    published DATETIME              NULL,
last_modified DATETIME              NULL,
  modified_by INTEGER  UNSIGNED     NULL,
  FOREIGN KEY (event_id)    REFERENCES node (nid),
  FOREIGN KEY (posted_by)   REFERENCES users (uid),
  FOREIGN KEY (modified_by) REFERENCES users (uid))
       ENGINE=InnoDB DEFAULT CHARSET=utf8;
	   
/*
 * Rows in the state table with associated events/meetings.
 *
 * nid               foreign key into the nodes table
 * article_state_id  foreign key into the ebms_article_state table
 */
    CREATE TABLE ebms_agenda_meeting
            (nid INTEGER UNSIGNED NOT NULL,
article_state_id INTEGER NOT NULL,
     PRIMARY KEY (nid, article_state_id),
     FOREIGN KEY (nid) REFERENCES node (nid),
     FOREIGN KEY (article_state_id)
                 REFERENCES ebms_article_state (article_state_id))
      ENGINE=InnoDB DEFAULT CHARSET=utf8;

/*
 * Summary secondary page.
 *
 * page_id        automatically generated primary key
 * board_id       foreign key into ebms_board table
 * page_name      string identifying page content
 * archived       if not NULL, date/time manager suppressed
 *                display of this page
 */
CREATE TABLE ebms_summary_page
    (page_id INTEGER      NOT NULL AUTO_INCREMENT PRIMARY KEY,
    board_id INTEGER      NOT NULL,
   page_name VARCHAR(255) NOT NULL,
    archived DATETIME         NULL,
 FOREIGN KEY (board_id) REFERENCES ebms_board (board_id))
      ENGINE=InnoDB DEFAULT CHARSET=utf8;

/*
 * Topic for summary secondary page.
 *
 * page_id        foreign key into ebms_summary_page table
 * topic_id       foreign key into ebms_topic table
 */
CREATE TABLE ebms_summary_page_topic
    (page_id INTEGER      NOT NULL,
    topic_id INTEGER      NOT NULL,
 PRIMARY KEY (page_id, topic_id),
 FOREIGN KEY (page_id)  REFERENCES ebms_summary_page (page_id),
 FOREIGN KEY (topic_id) REFERENCES ebms_topic (topic_id))
      ENGINE=InnoDB DEFAULT CHARSET=utf8;

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
      ENGINE=InnoDB DEFAULT CHARSET=utf8;

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
      ENGINE=InnoDB DEFAULT CHARSET=utf8;

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
      ENGINE=InnoDB DEFAULT CHARSET=utf8;

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
      ENGINE=InnoDB DEFAULT CHARSET=utf8;

/*
 * Database search.
 *
 * search_id      automatically generated primary key
 * when_searched  date/time search was submitted
 * searched_by    foreign key into Drupal's users table
 * search_spec    JSON encoded search criteria
 */
 CREATE TABLE ebms_search
   (search_id INTEGER           NOT NULL AUTO_INCREMENT PRIMARY KEY,
when_searched DATETIME          NOT NULL,
  searched_by INTEGER  UNSIGNED NOT NULL,
  search_spec LONGTEXT          NOT NULL,
  FOREIGN KEY (searched_by)   REFERENCES users (uid))
       ENGINE=InnoDB DEFAULT CHARSET=utf8;

/*
 * Queue for publishable articles.
 *
 * queue_id       automatically generated primary key
 * when_created   date/time the queue was first requested
 * requested_by   foreign key into Drupal's users table
 * queue_filtler  JSON encoded filter criteria for queue's query
 */
 CREATE TABLE ebms_publish_queue
    (queue_id INTEGER           NOT NULL AUTO_INCREMENT PRIMARY KEY,
 when_created DATETIME          NOT NULL,
 requested_by INTEGER  UNSIGNED NOT NULL,
 queue_filter LONGTEXT          NOT NULL,
  FOREIGN KEY (requested_by)   REFERENCES users (uid))
       ENGINE=InnoDB DEFAULT CHARSET=utf8;

/*
 * Rows in the state table which the user has flagged for promoting to
 * the Published state.
 *
 * queue_id          foreign key into the ebms_publish_queue table
 * article_state_id  foreign key into the ebms_article_state table
 */
    CREATE TABLE ebms_publish_queue_flag
       (queue_id INTEGER NOT NULL,
article_state_id INTEGER NOT NULL,
     PRIMARY KEY (queue_id, article_state_id),
     FOREIGN KEY (queue_id) REFERENCES ebms_publish_queue (queue_id),
     FOREIGN KEY (article_state_id)
                 REFERENCES ebms_article_state (article_state_id))
      ENGINE=InnoDB DEFAULT CHARSET=utf8;

/*
 * Stores report request parameters, serialized using jsencode().
 *
 * report_id       automatically generated primary key
 * report_data     request parameters
 */
CREATE TABLE ebms_temp_report
  (report_id INTEGER NOT NULL AUTO_INCREMENT PRIMARY KEY,
 report_data text    NOT NULL)
      ENGINE=InnoDB DEFAULT CHARSET=utf8;

/*
 * Record of the topics associated with an article, along with the review
 * cycle assigned to the article-topic combination.  The link into the
 * article state table provides all of the remaining information about
 * who was responsible for the association, when it happened, what the
 * status of the article was at the time for that topic, etc.  Populated
 * by the conversion software for the legacy data, and by the API for
 * setting article state going forward.
 *
 * article_id         foreign key into the ebms_article table
 * topic_id           foreign key into the ebms_topic table
 * cycle_id           foreign key into the ebms_cycle table
 * article_state_id   foreign key into the ebms_article_state table
 */
    CREATE TABLE ebms_article_topic
     (article_id INTEGER NOT NULL,
        topic_id INTEGER NOT NULL,
        cycle_id INTEGER NOT NULL,
article_state_id INTEGER NOT NULL,
     PRIMARY KEY (article_id, topic_id),
     FOREIGN KEY (article_id)       REFERENCES ebms_article (article_id),
     FOREIGN KEY (topic_id)         REFERENCES ebms_topic (topic_id),
     FOREIGN KEY (cycle_id)         REFERENCES ebms_cycle (cycle_id),
     FOREIGN KEY (article_state_id)
     REFERENCES ebms_article_state (article_state_id))
          ENGINE=InnoDB DEFAULT CHARSET=utf8;
