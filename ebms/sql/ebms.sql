-- /* $Id$ */

-- XXX FOR TEST ONLY
SET FOREIGN_KEY_CHECKS=0;
/*******
use test;
drop table if exists users;
create table users (uid int unsigned primary key) engine InnoDB;
drop table if exists files;
create table files (fid int unsigned primary key) engine InnoDB;
drop table if exists node;
create table node  (nid int unsigned primary key) engine InnoDB;
*******/

/*
 * Uploaded documents (does not include PubMed articles).
 *
 * doc_id         automatically generated primary key
 * file_id        foreign key into Drupal's files table
 * when_posted    date/time the user uploaded the documents
 * description    how the poster wants the document represented in lists
 */
DROP TABLE IF EXISTS ebms_doc;
CREATE TABLE ebms_doc
     (doc_id INTEGER          NOT NULL AUTO_INCREMENT PRIMARY KEY,
     file_id INTEGER UNSIGNED NOT NULL,
 when_posted DATETIME         NOT NULL,
 description TEXT             NOT NULL,
 FOREIGN KEY (file_id) REFERENCES files (fid))
      ENGINE=InnoDB;

/*
 * Panels of oncology specialists who maintain the PDQ summaries.
 *
 * board_id       automatically generated primary key
 * board_name     unique string for the board's name
 * loe_guidelines optional foreign key into ebms_doc for board's LOE guidelines
 */
DROP TABLE IF EXISTS ebms_board;
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
DROP TABLE IF EXISTS ebms_board_member;
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
DROP TABLE IF EXISTS ebms_subgroup;
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
DROP TABLE IF EXISTS ebms_subgroup_member;
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
DROP TABLE IF EXISTS ebms_tag;
CREATE TABLE ebms_tag
     (tag_id INTEGER      NOT NULL AUTO_INCREMENT PRIMARY KEY,
    tag_name VARCHAR(64)  NOT NULL UNIQUE,
 tag_comment TEXT             NULL)
      ENGINE=InnoDB;

/*
 * Assignment of tags to posted documents.
 *
 * doc_id         foreign key into the ebms_doc table
 * tag_id         foreign key into the ebms_tag table
 */
DROP TABLE IF EXISTS ebms_doc_tag;
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
DROP TABLE IF EXISTS ebms_doc_board;
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
DROP TABLE IF EXISTS ebms_ad_hoc_group;
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
DROP TABLE IF EXISTS ebms_ad_hoc_group_member;
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
DROP TABLE IF EXISTS ebms_topic;
CREATE TABLE ebms_topic
     (topic_id INTEGER         NOT NULL AUTO_INCREMENT PRIMARY KEY,
    topic_name VARCHAR(255)    NOT NULL UNIQUE,
      board_id INTEGER         NOT NULL,
 -- XXX I think topics have (rarely) been deleted in the past
 active_status ENUM ('A', 'I') NOT NULL DEFAULT 'A',
 FOREIGN KEY (board_id) REFERENCES ebms_board (board_id))
      ENGINE=InnoDB;

   -- XXX Should we name the other 'ebms_all_topic' and this 'ebms_topic'?
   DROP VIEW IF EXISTS ebms_active_topic;
   CREATE VIEW ebms_active_topic AS
          SELECT topic_id, topic_name, board_id
            FROM ebms_topic
           WHERE active_status = 'A';

/*
 * Assignment of topics to posted documents.
 *
 * Each posted document can have zero or more topics associated with it.
 * This assignment is used, for example, to restrict the contents of the
 * picklist of posted summary documents 
 *
 * topic_id       foreign key into the ebms_topic table
 * doc_id         foreign key into the ebms_doc table
 */
DROP TABLE IF EXISTS ebms_doc_topic;
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
DROP TABLE IF EXISTS ebms_topic_reviewer;
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
 *  source_id       ID used by source for this article, e.g., PMID
 *  source_jrnl_id  If source = 'Pubmed': then NLM unique journal id
 *                    Else: to be determined
 *  source_status   Source organization's status assignment to the record
 *                    Enables us to identify, e.g., records that are 
 *                    "In-Process", etc., and need future update.
 *  article_title   Full title of the article
 *  jrnl_title      Full journal title at time of import or update
 *  brf_jrnl_title  Journal title abbreviation found in article record.
 *                    Normally this is the NLM title abbreviation.
 *  brf_citation    Citation identifying journal, year, vol, issue, pages, or
 *                    however we wish show the article on a single line.
 *  abstract        Abstract, currently always English.
 *  published_date  Indication of when the article was published (free text)
 *                    Can't use SQL datetime since some articles have dates like
 *                    "Spring 2011".
 *  import_date     Datetime of first import.
 *  update_date     Datetime of last replacement from NLM or other source.
 *  source_data     Unmodified XML or whatever downloaded from the source.
 *                    We'll assume it's always there and make it not null
 *                    changing that only if there's a real use case.
 *  full_text_id    Foreign key into the Drupal files table for PDF of article.
 *  active_status   Provides a way to mark a citation to never be used for
 *                    any active purpose.  This is like the CDR 'D'eleted
 *                    status, not 'I'nactive.  We only use it if the 
 *                    citation should never ever have been imported at all.
 *                    Hopefully we'll never need to use this.
 *                    Values: 'A'ctive, 'D'eleted.
 */
DROP TABLE IF EXISTS ebms_article;
CREATE TABLE ebms_article (
  article_id        INTEGER NOT NULL AUTO_INCREMENT PRIMARY KEY,
  source            VARCHAR(32) NOT NULL,
  source_id         VARCHAR(32) NOT NULL,
  source_jrnl_id    VARCHAR(32) NOT NULL,
  source_status     VARCHAR(32) NULL,
  article_title     VARCHAR(512) NOT NULL,
  jrnl_title        VARCHAR(512) NOT NULL,
  brf_jrnl_title    VARCHAR(127) NULL,
  brf_citation      VARCHAR(255) NOT NULL,
  abstract          TEXT NULL,
  published_date    VARCHAR(64) NOT NULL,
  import_date       DATETIME NOT NULL,
  update_date       DATETIME NULL,
  source_data       TEXT NULL,
  full_text_id      INTEGER UNSIGNED NULL,
  active_status     ENUM('A', 'D') NOT NULL DEFAULT 'A',
  FOREIGN KEY (full_text_id) REFERENCES files (fid)
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
DROP TABLE IF EXISTS ebms_article_author;
CREATE TABLE ebms_article_author (
    author_id       INT AUTO_INCREMENT PRIMARY KEY,
    last_name       VARCHAR(128) NOT NULL,
    forename        VARCHAR(128),
    initials        CHAR(5)
)
    ENGINE = InnoDB;

    -- Two ways to search, use last + first name, or last + initials
    CREATE UNIQUE INDEX ebms_author_full_index 
           ON ebms_article_author (last_name, forename);
    CREATE UNIQUE INDEX ebms_author_initials_index 
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
DROP TABLE IF EXISTS ebms_article_author_cite;
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
DROP TABLE IF EXISTS ebms_cycle;
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
DROP TABLE IF EXISTS ebms_not_list;
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
 *  disposition_id  Unique ID of the citation.
 *  name            Human readable display name.
 *  description     Fuller human readable explanation of disposition.
 *  active_status   'A'ctive or 'I'nactive - don't use any more.
 */
DROP TABLE IF EXISTS ebms_import_disposition;
CREATE TABLE ebms_import_disposition (
    disposition_id  INT AUTO_INCREMENT PRIMARY KEY,
    name            VARCHAR(32) NOT NULL UNIQUE,
    description     VARCHAR(2048) NOT NULL,
    active_status   ENUM ('A', 'I') NOT NULL DEFAULT 'A'
)
    ENGINE = InnoDB;

    -- XXX Maybe we should add these right here, with descriptions?
    INSERT INTO ebms_import_disposition (name) values ('Imported');
    INSERT INTO ebms_import_disposition (name) values ('Rejected NOT listed');
    INSERT INTO ebms_import_disposition (name) values ('Rejected duplicate');
    INSERT INTO ebms_import_disposition (name) values ('Summary topic added');
    INSERT INTO ebms_import_disposition (name) values ('Review cycle added');
    INSERT INTO ebms_import_disposition (name) values ('Replaced');
    INSERT INTO ebms_import_disposition (name) values ('Error');

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
 *  summary_id      Unique ID of the summary for which this was an import.
 *                  Might be NULL in special cases?
 */
DROP TABLE IF EXISTS ebms_import_batch;
CREATE TABLE ebms_import_batch (
    import_batch_id INT AUTO_INCREMENT PRIMARY KEY,
    topic_id        INT NULL,
    source          VARCHAR(32) NOT NULL,
    import_date     DATETIME NOT NULL,
    cycle_id        INT NOT NULL,
    user_id         INT UNSIGNED NOT NULL,
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
DROP TABLE IF EXISTS ebms_import_action;
CREATE TABLE ebms_import_action (
    source_id          VARCHAR(32) NOT NULL,
    article_id         INT NULL,
    import_batch_id    INT NOT NULL,
    disposition_id     INT NOT NULL,
    FOREIGN KEY (article_id) 
        REFERENCES ebms_article(article_id),
    FOREIGN KEY (import_batch_id)
        REFERENCES ebms_import_batch(import_batch_id),
    FOREIGN KEY (disposition_id)
        REFERENCES ebms_import_disposition(disposition_id)
)
    ENGINE = InnoDB;


/*
 * Association of articles to topics.
 *
 * Each article imported into the system will have at least one topic
 * assigned to it.  Articles can have more than one topic assigned.
 * When a review packet is assembled, the packet is assigned one of
 * the topics, and only articles which have been associated with
 * that topic can be added to the review packet.  In the normal case
 * only those articles which have not already been added to another
 * packet for the topic assigned to the packet being assembled,
 * but special types of review packets (e.g., comprehensive review
 * packets) may bypass this restriction.
 *
 * This table just tells us if an article is currently associated with a
 * topic.  If there is a row in the table, the association is current.  If
 * the association is broken (because someone decided that the association
 * was an error), the row is removed.
 *
 * For information about who assigned a topic or when, or whether a topic
 * that is not now associated with a topic was ever so associated, etc.,
 * see the ebms_event table.
 *
 * article_id     foreign key into the ebms_article table
 * topic_id       foreign key into the ebms_topic table
 * event_id       foreign key into the ebms_article_event table.  This
 *                  event is the one that created the association.
 */
DROP TABLE IF EXISTS ebms_article_topic;
CREATE TABLE ebms_article_topic
 (article_id              INTEGER NOT NULL,
    topic_id              INTEGER NOT NULL,
    article_event_id      INTEGER NOT NULL,
 PRIMARY KEY (topic_id, article_id),
 FOREIGN KEY (article_id) REFERENCES ebms_article (article_id),
 FOREIGN KEY (topic_id)   REFERENCES ebms_topic (topic_id),
 FOREIGN KEY (article_event_id)   
                          REFERENCES ebms_article_event (article_event_id))
      ENGINE=InnoDB;

    CREATE INDEX topic_article ON ebms_article_topic(article_id, topic_id);

/*
 * Control values used in recording events.
 * 
 * This should be a relatively static table, changed only when there is a
 * significant change in software.
 * 
 *  event_type_id       Unique ID.
 *  name                Human readable name for brief display.
 *  description         Human readable description for help display and
 *                        documentation.
 *  active_status       'A'ctive or 'I'nactive.  We need this in case
 *                        we ever wish to stop using a particular event_type
 *                        but don't want to invalidate past events of that
 *                        type.
 */
DROP TABLE IF EXISTS ebms_event_type;
CREATE TABLE ebms_event_type (
    event_type_id       INT AUTO_INCREMENT PRIMARY KEY,
    name                VARCHAR(32) NOT NULL,
    description         VARCHAR(2048) NOT NULL,
    active_status       ENUM('A', 'I') DEFAULT 'A'
)
    ENGINE=InnoDB;

/*
 * event_type specific control values used in recording events.  Some
 * event_types have different values.  For example there is an event_type
 * for tagging an article to attach comments and/or to support special
 * kinds of searching.  The ebms_event_val rows for this event_type are
 * the specific tags that are legal to attach to a tag event.
 * 
 * This should be a relatively static table, changed only when there 
 * is a change in operating procedures.  For example, a tag value may
 * added.
 * 
 *  event_type_id       Unique ID.
 *  event_val_id        Unique ID of the value within the type.
 *  name                Human readable name for brief display.
 *  description         Human readable description for help display and
 *                        documentation.
 *  active_status       'A'ctive or 'I'nactive.  We need this in case
 *                        we ever wish to stop using a particular event value
 *                        but don't want to invalidate past events that use
 *                        that value.
 */
DROP TABLE IF EXISTS ebms_event_val;
CREATE TABLE ebms_event_val (
    event_type_id       INT,
    event_val_id        INT AUTO_INCREMENT PRIMARY KEY,
    name                VARCHAR(32) NOT NULL,
    description         VARCHAR(2048) NOT NULL,
    active_status       ENUM('A', 'I') DEFAULT 'A',
    -- XXX Maybe better to put active_status first?
    UNIQUE KEY event_val_index (event_type_id, event_val_id, active_status),
    FOREIGN KEY (event_type_id) REFERENCES ebms_event_type(event_type_id)
)
    ENGINE=InnoDB;

/*
 * A history of the events relating to an article.
 * 
 * One row in the table represents some action that was taken regarding
 * a particular article.  It can be used to reconstruct a history of
 * the actions that occurred.
 * 
 *  article_event_id    Unique ID of this article_event row.
 *  article_id          Article undergoing the event.
 *  topic_id            Summary topic, if applicable, may be null.
 *  event_type_id       What kind of event was this?
 *                        Import/update
 *                        Review status change
 *                        Tag/comment added
 *                        etc.
 *  event_val_id        Is it a particular kind of import, review, tag, etc.?
 *  prev_event_id       Is this threaded to a previous event, e.g., a comment
 *                        on a previous comment?
 *  user_id             Who did this.
 *  dt                  Datetime of event.
 *  comment             Free text from user or program generating event.
 *  active_status       Is this event info still active for this article?
 *                        'A'ctive   - event applies.
 *                        'I'nactive - event was superseded, rendered inactive.
 *                        'D'eleted  - this was a mistake, it should never 
 *                                     have happened.  Ignore it.
 */
DROP TABLE IF EXISTS ebms_article_event;
CREATE TABLE ebms_article_event (
     article_event_id  INT AUTO_INCREMENT PRIMARY KEY,
     article_id        INT NOT NULL,
     topic_id          INT NULL,
     event_type_id     INT NOT NULL,
     event_val_id      INT NULL,
     prev_event_id     INT NULL,
     user_id           INT UNSIGNED NOT NULL,
     dt                DATETIME NOT NULL,
     comment           TEXT NULL,
     active_status     ENUM('A', 'I', 'D'),
     FOREIGN KEY (event_type_id)
          REFERENCES ebms_event_type(event_type_id),
     FOREIGN KEY (article_id)
          REFERENCES ebms_article(article_id),
     FOREIGN KEY (topic_id)
          REFERENCES ebms_topic(topic_id),
     FOREIGN KEY (event_type_id, event_val_id)
          REFERENCES ebms_event_val(event_type_id, event_val_id),
     FOREIGN KEY (user_id)
          REFERENCES users(uid)
)
     ENGINE=InnoDB; 

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
DROP TABLE IF EXISTS ebms_packet;
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
DROP TABLE IF EXISTS ebms_packet_summary;
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
DROP TABLE IF EXISTS ebms_packet_reviewer;
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
DROP TABLE IF EXISTS ebms_packet_article;
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
 * review_flags   bit flags indicating whether:
 *                  - the article warrants no changes to any summaries
 *                  - the article should be cite in at least one summary
 *                  - the article merits revision of text in a summary
 *                  - the article should be discussed collectively
 * comments       free text elaboration of how the reviewer feels the
 *                article's findings should be incorporated into the
 *                PDQ summaries (or why it shouldn't be)
 * loe_info       reviewer's assessment of the levels of evidence
 *                found in the article; free text, but following the
 *                guidelines used by the board for LOE
 */
DROP TABLE IF EXISTS ebms_article_review;
CREATE TABLE ebms_article_review
  (review_id INTEGER          NOT NULL AUTO_INCREMENT PRIMARY KEY,
   packet_id INTEGER          NOT NULL,
  article_id INTEGER          NOT NULL,
 reviewer_id INTEGER UNSIGNED NOT NULL,
 when_posted DATETIME         NOT NULL,
review_flags INTEGER          NOT NULL,
    comments TEXT                 NULL,
    loe_info TEXT                 NULL,
  UNIQUE KEY ebms_art_review_index (article_id, reviewer_id),
 FOREIGN KEY (packet_id,
              article_id)  REFERENCES ebms_packet_article (packet_id,
                                                           article_id),
 FOREIGN KEY (reviewer_id) REFERENCES users (uid))
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
 * file_id        foreign key into Drupal's files table
 * reviewer_id    foreign key into Drupal's users table
 * packet_id      foreign key into the ebms_packet table
 * drop_flag      has the posting of the document been retracted?
 * when_posted    date/time the document was posted
 * doc_title      how the document should be identified in lists
 * description    optional notes accompanying the posted document
 */
DROP TABLE IF EXISTS ebms_reviewer_doc;
CREATE TABLE ebms_reviewer_doc
     (doc_id INTEGER          NOT NULL AUTO_INCREMENT PRIMARY KEY,
     file_id INTEGER UNSIGNED NOT NULL,
 reviewer_id INTEGER UNSIGNED NOT NULL,
   packet_id INTEGER          NOT NULL,
   drop_flag INTEGER          NOT NULL DEFAULT 0,
 when_posted DATETIME         NOT NULL,
   doc_title VARCHAR(256)     NOT NULL,
 description TEXT                 NULL,
 FOREIGN KEY (file_id)     REFERENCES files (fid),
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
DROP TABLE IF EXISTS ebms_message;
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
DROP TABLE IF EXISTS ebms_message_recipient;
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
 * checkin_date   identification of when reservation should start
 * nights         free text explanation of how many nights are needed
 * processed      date/time request was processed (not currently used)
 * notes          optional additional information
 */
DROP TABLE IF EXISTS ebms_hotel_request;
CREATE TABLE ebms_hotel_request
 (request_id INTEGER          NOT NULL AUTO_INCREMENT PRIMARY KEY,
requestor_id INTEGER UNSIGNED NOT NULL,
   submitted DATETIME         NOT NULL,
     meeting INTEGER UNSIGNED NOT NULL,
checkin_date DATE             NOT NULL,
      nights VARCHAR(128)     NOT NULL,
   processed DATETIME             NULL,
       notes TEXT                 NULL,
 FOREIGN KEY (requestor_id) REFERENCES users (uid),
 FOREIGN KEY (meeting)      REFERENCES node (nid))
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
DROP TABLE IF EXISTS ebms_reimbursement_request;
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
DROP TABLE IF EXISTS ebms_reimbursement_item;
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
 * file_id        foreign key into Drupal's files table
 */
DROP TABLE IF EXISTS ebms_reimbursement_receipts;
CREATE TABLE ebms_reimbursement_receipts
 (receipt_id INTEGER          NOT NULL AUTO_INCREMENT PRIMARY KEY,
  request_id INTEGER          NOT NULL,
     file_id INTEGER UNSIGNED NOT NULL,
 FOREIGN KEY (request_id) REFERENCES ebms_reimbursement_request (request_id),
 FOREIGN KEY (file_id)    REFERENCES files (fid))
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
DROP TABLE IF EXISTS ebms_report_request;
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
DROP TABLE IF EXISTS ebms_agenda;
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
DROP TABLE IF EXISTS ebms_summary_page;
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
 * page_id        foreign key into embs_summary_page table
 * link_url       URL for the Cancer.gov page
 * link_label     display text for the link
 */
DROP TABLE IF EXISTS ebms_summary_link;
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
DROP TABLE IF EXISTS ebms_summary_supporting_doc;
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
DROP TABLE IF EXISTS ebms_summary_posted_doc;
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
DROP TABLE IF EXISTS ebms_summary_returned_doc;
CREATE TABLE ebms_summary_returned_doc
    (page_id INTEGER  NOT NULL,
      doc_id INTEGER  NOT NULL,
    archived DATETIME     NULL,
       notes TEXT         NULL,
 PRIMARY KEY (page_id, doc_id),
 FOREIGN KEY (page_id) REFERENCES ebms_summary_page (page_id),
 FOREIGN KEY (doc_id)  REFERENCES ebms_doc (doc_id))
      ENGINE=InnoDB;

-- XXX Reset from test conditions
SET FOREIGN_KEY_CHECKS=1;
