CREATE TABLE ebms_pubmed_results
   (results_id INTEGER    NOT NULL AUTO_INCREMENT PRIMARY KEY,
when_submitted DATETIME   NOT NULL,
  results_file MEDIUMBLOB NOT NULL)
        ENGINE=InnoDB DEFAULT CHARSET=utf8;
CREATE INDEX ebms_pubmed_results_date ON ebms_pubmed_results(when_submitted);
