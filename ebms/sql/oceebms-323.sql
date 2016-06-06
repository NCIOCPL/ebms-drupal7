ALTER TABLE ebms_review_rejection_value
        ADD active_status ENUM ('A', 'I') NOT NULL DEFAULT 'A'
      AFTER value_name;
UPDATE ebms_review_rejection_value
   SET active_status = 'I'
 WHERE value_name IN (
         'Inappropriate interpretation of subgroup analyses',
         'Inappropriate statistical analysis',
         'Randomized trial with flawed or insufficiently described randomization process',
         'Unvalidated outcome measure(s) used');
UPDATE ebms_review_rejection_value
   SET value_name = 'Inappropriate study design or analyses'
 WHERE value_name = 'Inappropriate study design';
