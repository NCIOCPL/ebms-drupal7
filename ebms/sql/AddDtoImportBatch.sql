USE oce_ebms;

ALTER TABLE ebms_import_batch
MODIFY input_type ENUM('R','F','S','D');

ALTER TABLE ebms_import_batch
ADD COLUMN messages TEXT NULL;

ALTER TABLE ebms_import_batch
ADD COLUMN status VARCHAR(32) NOT NULL DEFAULT 'Success';
