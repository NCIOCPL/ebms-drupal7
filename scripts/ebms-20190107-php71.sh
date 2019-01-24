# Script for upgrading Drupal libraries and custom modules for EBMS
# To be run under the drupal account.

echo Verifying account running script ...
if [ $(whoami) != "drupal" ]
then
    echo This script must be run using the drupal account.
    echo Aborting script.
    exit
fi

echo Backing up ebms module ...
tar cf /tmp/ebms-module.tar /local/drupal/sites/ebms.nci.nih.gov/modules/custom/ebms

echo Copying new files ...
cp -R drupal/* /local/drupal/

echo Adding new modules ...
cd /local/drupal/sites/ebms.nci.nih.gov
drush vset maintenance_mode 1
drush dis ebms ebms_webforms date_ical -y

echo Re-enabling modules ...
drush en ebms ebms_webforms date_ical -y

echo Clearing caches ...
drush cc all
drush cron
drush cc all

echo Putting site back into live mode ...
drush vset maintenance_mode 0

echo Done!
