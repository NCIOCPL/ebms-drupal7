# Apply security update 7.74 for Drupal core.

echo Verifying account running script
if [ $(whoami) != "drupal" ]
then
    echo This script must be run using the drupal account.
    echo Aborting script.
    exit
fi

echo Setting locations
export SITEDIR=/local/drupal/sites/ebms.nci.nih.gov
echo "Site directory is $SITEDIR"

echo Putting site into maintenance mode
cd $SITEDIR
drush vset maintenance_mode 1

echo Applying security update
drush up -y drupal-7.74

echo Clearing caches twice, once is not always sufficient
drush cc all
drush cc all

echo Putting site back into live mode
drush vset maintenance_mode 0

echo Done
