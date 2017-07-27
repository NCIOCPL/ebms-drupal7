export PATH=$PATH:/usr/local/php/bin
cd /local/drupal/sites/ebms.nci.nih.gov
/usr/local/bin/drush scr refresh-journal-list --script-path=$HOME/cron
