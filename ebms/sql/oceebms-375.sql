CREATE TABLE ebms_article_topic_comment
 (comment_id INTEGER          NOT NULL AUTO_INCREMENT PRIMARY KEY,
  article_id INTEGER          NOT NULL,
    topic_id INTEGER          NOT NULL,
  created_by INTEGER UNSIGNED NOT NULL,
     created DATETIME         NOT NULL,
     comment TEXT             NOT NULL,
 modified_by INTEGER UNSIGNED     NULL,
    modified DATETIME             NULL,
  UNIQUE KEY (article_id, topic_id),
 FOREIGN KEY (article_id)       REFERENCES ebms_article (article_id),
 FOREIGN KEY (topic_id)         REFERENCES ebms_topic (topic_id),
 FOREIGN KEY (created_by)       REFERENCES users (uid),
 FOREIGN KEY (modified_by)      REFERENCES users (uid))
      ENGINE=InnoDB DEFAULT CHARSET=utf8;
