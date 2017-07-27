# Portion of the release 3.5 deployment to be run under the drupal account

if [ $(whoami) != "drupal" ]
then
    echo This script must be run using the drupal account.
    echo Aborting script.
    exit
fi

echo Setting locations
export WORKDIR=/tmp/ebms-3.5
export SITES=/local/drupal/sites
export SITEDIR=$SITES/ebms.nci.nih.gov
echo "Site directory is $SITEDIR"

echo Creating log directory if necessary
mkdir -p $HOME/logs

echo Installing cron jobs
rm $HOME/cron/*
cp $WORKDIR/scheduled/* $HOME/cron/
crontab $HOME/cron/drupal.crontab

echo Putting site into maintenance mode
cd $SITEDIR
drush vset maintenance_mode 1

echo Upgrading Drupal core
drush up -y drupal-7.56

echo Disabling modules we will no longer be using
cd $SITEDIR
drush -y dis ebms_forums forum

echo Applying database changes
cd $SITEDIR
table=ebms_article_tag_type
cond="text_id = 'not_reviewed'"
query="SELECT COUNT(*) FROM $table WHERE $cond"
count=`drush sqlq --extra=--skip-column-names "$query"`
if [ $count = "0" ]
then
    if [ -r $WORKDIR/oceebms-427.sql ]
    then
        echo Database changes for OCEEBMS-427
        drush sqlc < $WORKDIR/oceebms-427.sql
    else
        echo $WORKDIR/oceebms-427.sql missing
        echo Aborting script.
        exit
    fi
else
    echo Database changes for OCEEBMS-427 already applied
fi
if [ -r $WORKDIR/oceebms-444.sql ]
then
    echo Database changes for OCEEBMS-444
    drush sqlc < $WORKDIR/oceebms-444.sql
else
    echo $WORKDIR/oceebms-444.sql missing
    echo Aborting script.
    exit
fi

echo Deleting the current software
cd $SITEDIR
rm -rf modules/* themes/*
rm -rf $SITES/all/modules/Custom/nci_SSO/*

echo Replacing it with new
cd $WORKDIR
cp -r modules $SITEDIR/ || { echo cp modules failed; exit; }
cp -r themes $SITEDIR/ || { echo cp themes failed; exit; }
cp nci_SSO/* $SITES/all/modules/Custom/nci_SSO/ || {
    echo cp nci_SSO failed;
    exit;
}

echo Disabling and re-enabling the site modules
cd $SITEDIR
drush -y dis ebms ebms_content ebms_webforms
drush -y en ebms ebms_content ebms_webforms

echo Refreshing settings for text editing filters
drush -y en ebms_config
drush fr ebms_config

echo Installing common sites file
cd $SITEDIR
cp -f $WORKDIR/sites.php ..

echo Clearing caches twice, once is not always sufficient
cd $SITEDIR
drush cc all
drush cc all

echo Putting site back into live mode
drush vset maintenance_mode 0

echo Done
