ALTER TABLE ebms_board
        ADD auto_imports INTEGER NOT NULL DEFAULT 0;
UPDATE ebms_board
   SET auto_imports = 1
 WHERE board_name IN (
    'Adult Treatment',
    'Cancer Genetics',
    'Screening and Prevention'
);
