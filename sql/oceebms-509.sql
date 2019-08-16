CREATE TABLE ebms_internal_tag
       (tag_id INTEGER          NOT NULL AUTO_INCREMENT PRIMARY KEY,
      tag_name VARCHAR(255)     NOT NULL,
 active_status ENUM ('A', 'I')  NOT NULL DEFAULT 'A',
  UNIQUE KEY internal_tag_name_ix (tag_name))
      ENGINE=InnoDB DEFAULT CHARSET=utf8;
