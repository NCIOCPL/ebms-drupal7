USE ebms;
ALTER TABLE ebms_import_batch
MODIFY input_type ENUM('R','F','S','D');
