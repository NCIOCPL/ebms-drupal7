export PATH=$PATH:/usr/local/php/bin
if [ -d "/web/appdev/sites" ]
then
    cd /web/appdev/sites/ebms.nci.nih.gov
else
    cd /local/drupal/sites/ebms.nci.nih.gov
fi
/usr/local/bin/drush scr find-pubmed-drops --script-path=$HOME/cron
