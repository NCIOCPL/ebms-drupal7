CREATE TABLE ebms_internal_tag
       (tag_id INTEGER          NOT NULL AUTO_INCREMENT PRIMARY KEY,
      tag_name VARCHAR(255)     NOT NULL,
 active_status ENUM ('A', 'I')  NOT NULL DEFAULT 'A',
  UNIQUE KEY internal_tag_name_ix (tag_name))
      ENGINE=InnoDB DEFAULT CHARSET=utf8;
CREATE TABLE ebms_internal_article_tag
     (tag_pk INTEGER NOT NULL AUTO_INCREMENT PRIMARY KEY,
  article_id INTEGER NOT NULL,
      tag_id INTEGER NOT NULL,
 FOREIGN KEY (tag_id)     REFERENCES ebms_internal_tag (tag_id),
 FOREIGN KEY (article_id) REFERENCES ebms_article (article_id))
      ENGINE=InnoDB DEFAULT CHARSET=utf8;
CREATE TABLE ebms_internal_article_comment
 (comment_id INTEGER NOT NULL AUTO_INCREMENT PRIMARY KEY,
  article_id INTEGER NOT NULL,
     user_id INTEGER UNSIGNED NOT NULL,
comment_date DATETIME NOT NULL
comment_text TEXT     NOT NULL,
 FOREIGN KEY (article_id) REFERENCES ebms_article (article_id),
 FOREIGN KEY (user_id)    REFERENCES users(uid))
      ENGINE=InnoDB DEFAULT CHARSET=utf8;
ALTER TABLE ebms_import_batch
     MODIFY input_type ENUM('R','F','S','D','I') NOT NULL DEFAULT 'R';
