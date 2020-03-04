# Script for deploying EBMS Release 3.7 ("Biscayne") to a tier

echo Verifying account running script
if [ $(whoami) != "drupal" ]
then
    echo This script must be run using the drupal account.
    echo Aborting script.
    exit
fi

echo Setting locations
export NCIOCPL=https://api.github.com/repos/NCIOCPL
export URL=$NCIOCPL/ebms/tarball/biscayne
export WORKDIR=/tmp/ebms-3.7
export SITEDIR=/local/drupal/sites/ebms.nci.nih.gov
export CURL="curl -L -s -k"
echo "Site directory is $SITEDIR"

echo Creating a working directory at $WORKDIR
mkdir $WORKDIR || {
    echo creating $WORKDIR failed
    exit 1
}

echo Backing up existing files
cd $SITEDIR
tar cjf $WORKDIR/ebms-pre-release-3.7-backup.tar.bz2 modules themes || {
    echo "tar of themes and modules failed"
    exit 1
}
cp $SITEDIR/../sites.php $WORKDIR/sites.php-pre-release-3.7-backup

echo Fetching the new release from GitHub
cd $WORKDIR
$CURL $URL | tar -xzf - || {
    echo fetch $URL failed
    exit 1
}
mv NCIOCPL-ebms* ebms || {
    echo rename failed
    exit 1
}

echo Updating cron job
cp $WORKDIR/ebms/scheduled/update-pubmed-data.py $HOME/cron/

echo Putting site into maintenance mode
cd $SITEDIR
drush vset maintenance_mode 1

echo Deleting the current software
cd $SITEDIR
rm -rf modules/* themes/*

echo Replacing it with new
cd $WORKDIR/ebms/ebms.nci.nih.gov
cp -r modules $SITEDIR/ || { echo cp modules failed; exit; }
cp -r themes $SITEDIR/ || { echo cp themes failed; exit; }

echo Disabling and re-enabling the site modules
cd $SITEDIR
drush -y dis ebms ebms_content ebms_webforms
drush -y en ebms ebms_content ebms_webforms

echo Refreshing settings for text editing filters
drush en -y ebms_config
drush fr -y ebms_config

echo Adding travel admin role/permissions
cd $SITEDIR
query="SELECT COUNT(*) FROM role WHERE name = 'travel admin'"
count=`drush sqlq --extra=--skip-column-names "$query"`
if [ $count = "0" ]
then
    drush rcrt 'travel admin'
    echo Role added for travel admin
else
    echo Role for travel admin already added
fi
drush rap 'travel admin' 'access all webform results'
drush rap 'travel admin' 'manage travel'
drush rap 'travel admin' 'view all events'

echo Applying security updates
drush up -y webform-7.x-4.22
drush up -y drupal-7.69

echo Clearing caches twice, once is not always sufficient
drush cc all
drush cc all

echo Putting site back into live mode
drush vset maintenance_mode 0

echo Done
