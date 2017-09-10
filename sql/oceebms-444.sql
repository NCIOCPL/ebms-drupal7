UPDATE ebms_review_rejection_value
   SET active_status = 'I'
 WHERE value_name IN (
         'Already cited in the PDQ summary',
         'Missing/incomplete outcome data; major protocol deviations');
