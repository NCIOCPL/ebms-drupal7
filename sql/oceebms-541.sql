UPDATE ebms_internal_article_tag
   SET tag_added = (
       SELECT ebms_article.import_date
         FROM ebms_article
        WHERE ebms_article.article_id = ebms_internal_article_tag.article_id)
 WHERE tag_added IS NULL;
