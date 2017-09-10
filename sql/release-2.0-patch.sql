UPDATE ebms_review_rejection_value
   SET value_pos = 16
 WHERE value_id = 15;
INSERT INTO ebms_review_rejection_value (value_name, value_pos, extra_info)
     VALUES ('Conflict of interest', 15,
             'e.g., article author, competing financial, commercial, or professional interests');
ALTER TABLE ebms_article
        ADD data_mod DATE NULL
      AFTER source_data;
ALTER TABLE ebms_article
        ADD data_checked DATE NULL
      AFTER data_mod;
CREATE INDEX ebms_article_data_mod
          ON ebms_article(data_mod);
CREATE INDEX ebms_article_data_checked
          ON ebms_article(data_checked);
ALTER TABLE ebms_import_batch
     MODIFY input_type ENUM('R','F','S','D');
ALTER TABLE ebms_import_batch
        ADD messages TEXT NULL;
ALTER TABLE ebms_import_batch
        ADD status VARCHAR(32) NOT NULL DEFAULT 'Success';
INSERT INTO ebms_tag (tag_name, tag_comment)
     VALUES ('help', 'Documents to be listed on user help landing page');
INSERT ebms_article_state_type 
        (state_text_id, state_name, description, sequence, completed)
        VALUES ('OnHold', 'On Hold',
        'New board manager action added for request in ticket OCEEBMS-82.',
        70, 'N');
CREATE TABLE ebms_ad_hoc_group_board
   (group_id INTEGER NOT NULL,
    board_id INTEGER NOT NULL,
 PRIMARY KEY (group_id, board_id),
 FOREIGN KEY (group_id) REFERENCES ebms_ad_hoc_group (group_id),
 FOREIGN KEY (board_id) REFERENCES ebms_board (board_id))
      ENGINE=InnoDB DEFAULT CHARSET=utf8;
