export PATH=$PATH:/usr/local/php/bin
cd /local/drupal/sites/ebms.nci.nih.gov
/usr/local/bin/drush scr find-pubmed-drops --script-path=$HOME/cron
