# Script for upgrading third-party Drupal modules used in the EBMS
# To be run under the drupal account.

echo Verifying account running script ...
if [ $(whoami) != "drupal" ]
then
    echo This script must be run using the drupal account.
    echo Aborting script.
    exit
fi

echo Backing up sites/all/modules/webform ...
cd /local/drupal
tar cf /tmp/webform-all.tar sites/all/modules/webform

echo Putting site into maintenance mode ...
cd sites/ebms.nci.nih.gov/
drush vset maintenance_mode 1

echo Moving third-party modules to sites/all/modules ...
drush dis -y ckeditor_link webform
rm -rf ../all/modules/webform
mv modules/ckeditor_link/ modules/webform ../all/modules/

echo Re-enabling modules ...
drush en -y ckeditor_link ebms ebms_config ebms_webforms webform webform_share
drush up -y

echo Clearing caches ...
drush cc all
drush cc all

echo Putting site back into live mode ...
drush vset maintenance_mode 0

echo Done!
