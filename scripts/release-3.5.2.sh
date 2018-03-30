# Script for deploying EBMS Release 3.5.2 to a tier

echo Verifying account running script
if [ $(whoami) != "drupal" ]
then
    echo This script must be run using the drupal account.
    echo Aborting script.
    exit
fi

echo Setting locations
export NCIOCPL=https://api.github.com/repos/NCIOCPL
export URL=$NCIOCPL/ebms/tarball/work
export WORKDIR=/tmp/ebms-3.5.2
export SITEDIR=/local/drupal/sites/ebms.nci.nih.gov
echo "Site directory is $SITEDIR"

echo Creating a working directory at $WORKDIR
mkdir $WORKDIR || {
    echo creating $WORKDIR failed
    exit 1
}

echo Backing up existing files
cd $SITEDIR
tar cjf $WORKDIR/ebms-pre-release-3.5.2-backup.tar.bz2 modules themes || {
    echo "tar of themes and modules failed"
    exit 1
}
cp $SITEDIR/../sites.php $WORKDIR/sites.php-pre-release-3.5.2-backup

echo Fetching the new release from GitHub
cd $WORKDIR
curl -L -s -k $URL | tar -xz || {
    echo fetch $URL failed
    exit 1
}
mv NCIOCPL-ebms* ebms || {
    echo rename failed
    exit 1
}

echo Putting site into maintenance mode
cd $SITEDIR
drush vset maintenance_mode 1

echo Upgrading Drupal core
drush up -y drupal-7.57

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
drush -y en ebms_config
drush fr ebms_config

echo Clearing caches twice, once is not always sufficient
drush cc all
drush cc all

echo Putting site back into live mode
drush vset maintenance_mode 0

echo Done

