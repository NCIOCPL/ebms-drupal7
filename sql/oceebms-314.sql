DELETE FROM ebms_publish_queue_flag WHERE article_state_id IN (
    SELECT article_state_id FROM ebms_article_state WHERE article_id = 392404);
DELETE FROM ebms_article_state_comment WHERE article_state_id IN (
    SELECT article_state_id FROM ebms_article_state WHERE article_id = 392404);
DELETE FROM ebms_article_topic WHERE article_id = 392404;
DELETE FROM ebms_article_state WHERE article_id = 392404;
DELETE FROM ebms_article_author_cite WHERE article_id = 392404;
DELETE FROM ebms_article WHERE article_id = 392404;
DROP INDEX ebms_article_source_id_index ON ebms_article;
CREATE UNIQUE INDEX ebms_article_source_id_index
    ON ebms_article(source, source_id);
