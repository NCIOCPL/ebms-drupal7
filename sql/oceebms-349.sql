 CREATE TABLE ebms_article_relation_type
     (type_id INTEGER         NOT NULL AUTO_INCREMENT PRIMARY KEY,
    type_name VARCHAR(255)    NOT NULL UNIQUE,
active_status ENUM ('A', 'I') NOT NULL DEFAULT 'A',
    type_desc TEXT                NULL)
       ENGINE=InnoDB DEFAULT CHARSET=utf8;
INSERT INTO ebms_article_relation_type (type_name, type_desc)
     VALUES ('Duplicate',
     'This article has a duplicate record (e.g., the PMID has changed).');
INSERT INTO ebms_article_relation_type (type_name, type_desc)
     VALUES ('Article/Editorial',
         'This article (or editorial) has a related editorial (or article).');
INSERT INTO ebms_article_relation_type (type_name, type_desc)
     VALUES ('Other',
          'This article is related in some other way to another article.');
CREATE TABLE ebms_related_article
(relationship_id INTEGER          NOT NULL AUTO_INCREMENT PRIMARY KEY,
         from_id INTEGER          NOT NULL,
           to_id INTEGER          NOT NULL,
         type_id INTEGER          NOT NULL,
      created_by INTEGER UNSIGNED NOT NULL,
         created DATETIME         NOT NULL,
         comment TEXT                 NULL,
  inactivated_by INTEGER UNSIGNED     NULL,
     inactivated DATETIME             NULL,
FOREIGN KEY (from_id)        REFERENCES ebms_article (article_id),
FOREIGN KEY (to_id)          REFERENCES ebms_article (article_id),
FOREIGN KEY (type_id)        REFERENCES ebms_article_relation_type (type_id),
FOREIGN KEY (created_by)     REFERENCES users (uid),
FOREIGN KEY (inactivated_by) REFERENCES users (uid))
        ENGINE=InnoDB DEFAULT CHARSET=utf8;
