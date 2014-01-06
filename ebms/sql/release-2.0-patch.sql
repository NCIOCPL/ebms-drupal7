USE oce_ebms;

UPDATE ebms_review_rejection_value
   SET value_pos = 16
 WHERE value_id = 15;
INSERT INTO ebms_review_rejection_value (value_name, value_pos)
     VALUES ('I am unable to suggest changes due to a conflict of interest', 15);
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
