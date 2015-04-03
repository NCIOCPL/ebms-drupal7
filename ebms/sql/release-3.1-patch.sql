ALTER TABLE ebms_board
    ADD board_manager INTEGER UNSIGNED NOT NULL
    AFTER board_name;
ALTER TABLE ebms_board
    ADD FOREIGN KEY (board_manager)
    REFERENCES users (uid);
/* Adult Treatment managed by Victoria Shields */
UPDATE ebms_board SET board_manager = 2 WHERE board_id = 1;
/* Cancer Genetics managed by Robin Juthe */
UPDATE ebms_board SET board_manager = 3 WHERE board_id = 2;
/* Pediatric Treatment managed by Sharon Quint-Kasner */
UPDATE ebms_board SET board_manager = 4 WHERE board_id = 3;
/* Screening and Prevention managed by Valerie Dyer */
UPDATE ebms_board SET board_manager = 5 WHERE board_id = 4;
/* Supportive and Palliative Care managed by Robin Baldwin */
UPDATE ebms_board SET board_manager = 6 WHERE board_id = 5;
/* Cancer Complementary and Alternative Medicine managed by Robin Baldwin */
UPDATE ebms_board SET board_manager = 6 WHERE board_id = 6;
CREATE TEMPORARY TABLE null_target
SELECT p.packet_id AS pid
  FROM ebms_packet p
  JOIN ebms_topic t
    ON t.topic_id = p.topic_id
  JOIN ebms_board b
    ON b.board_id = t.board_id
 WHERE p.last_seen IS NOT NULL
   AND p.created_by != b.board_manager;
UPDATE ebms_packet SET last_seen = NULL WHERE packet_id IN
(
    SELECT pid FROM null_target
);
/*
 * The column will still be UNIQUE, but adding the 'UNIQUE' keyword
 * to the column definition here would result in a second unique
 * index being created. I call that a bug in MYSQL.
 */
ALTER TABLE ebms_article_state_type
     MODIFY state_text_id VARCHAR(32) NOT NULL;
UPDATE ebms_article_state_type
   SET description = 'Article has been reviewed but should not yet '
                     'be eligible for inclusion on meeting agendas.'
 WHERE state_text_id = 'OnHold';
INSERT ebms_article_state_type 
    (state_text_id, state_name, description, sequence, completed)
    VALUES ('FullReviewHold', 'Held After Full Text Review',
    'Article should not yet be eligible for inclusion in review packets.',
    60, 'N');
CREATE TABLE ebms_core_journal (
    source          VARCHAR(32) NOT NULL,
    source_jrnl_id  VARCHAR(32) NOT NULL,
    PRIMARY KEY (source, source_jrnl_id)
)
    ENGINE = InnoDB DEFAULT CHARSET=utf8;
INSERT INTO ebms_core_journal (source, source_jrnl_id) VALUES ('Pubmed', '0255562');
INSERT INTO ebms_core_journal (source, source_jrnl_id) VALUES ('Pubmed', '7501160');
INSERT INTO ebms_core_journal (source, source_jrnl_id) VALUES ('Pubmed', '8309333');
INSERT INTO ebms_core_journal (source, source_jrnl_id) VALUES ('Pubmed', '7503089');
INSERT INTO ebms_core_journal (source, source_jrnl_id) VALUES ('Pubmed', '9216904');
INSERT INTO ebms_core_journal (source, source_jrnl_id) VALUES ('Pubmed', '2985213R');
INSERT INTO ebms_core_journal (source, source_jrnl_id) VALUES ('Pubmed', '100957246');
INSERT INTO ebms_core_journal (source, source_jrnl_id) VALUES ('Pubmed', '0372351');
INSERT INTO ebms_core_journal (source, source_jrnl_id) VALUES ('Pubmed', '7603616');
INSERT INTO ebms_core_journal (source, source_jrnl_id) VALUES ('Pubmed', '7603509');
INSERT INTO ebms_core_journal (source, source_jrnl_id) VALUES ('Pubmed', '101186624');
INSERT INTO ebms_core_journal (source, source_jrnl_id) VALUES ('Pubmed', '0374236');
INSERT INTO ebms_core_journal (source, source_jrnl_id) VALUES ('Pubmed', '8900488');
