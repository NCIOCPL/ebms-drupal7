DELETE FROM ebms_doc_tag
 WHERE tag_id = (SELECT tag_id
                   FROM ebms_tag
                  WHERE tag_name = 'about');
DELETE FROM ebms_tag
 WHERE tag_name = 'about';
