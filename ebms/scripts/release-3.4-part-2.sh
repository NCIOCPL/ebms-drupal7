# Portion of the release 3.4 deployment to be run under the drupal account

if [ $(whoami) != "drupal" ]
then
    echo This script must be run using the drupal account.
    echo Aborting script.
    exit
fi

echo Setting locations
export WORKDIR=/tmp/ebms-3.4
export SITES=/local/drupal/sites
export SITEDIR=$SITES/ebms.nci.nih.gov
export ALLSITES=$SITES/all
echo "Site directory is $SITEDIR"

echo Putting site into maintenance mode
cd $SITEDIR
drush vset maintenance_mode 1

echo Applying database changes
cd $SITEDIR
schema="TABLE_SCHEMA LIKE 'oce_ebms%'"
table=information_schema.TABLES
cond="$schema AND TABLE_NAME = 'ebms_pubmed_results'"
query="SELECT COUNT(*) FROM $table WHERE $cond"
count=`drush sqlq --extra=--skip-column-names "$query"`
if [ $count = "0" ]
then
    if [ -r $WORKDIR/oceebms-313.sql ]
    then
        echo Database changes for OCEEBMS-313
        drush sqlc < $WORKDIR/oceebms-313.sql
    else
        echo $WORKDIR/oceebms-313.sql missing
        echo Aborting script.
        exit
    fi
else
    echo Database changes for OCEEBMS-313 already applied
fi
query="SELECT COUNT(*) FROM ebms_article WHERE article_id = 392404"
count=`drush sqlq --extra=--skip-column-names "$query"`
if [ $count = "0" ]
then
    if [ -r $WORKDIR/oceebms-314.sql ]
    then
        echo Database changes for OCEEBMS-314
        drush sqlc < $WORKDIR/oceebms-314.sql
    else
        echo $WORKDIR/oceebms-314.sql missing
        echo Aborting script.
        exit
    fi
else
    echo Database changes for OCEEBMS-314 already applied
fi
if [ -r $WORKDIR/oceebms-339.sql ]
then
    echo Database changes for OCEEBMS-339
    drush sqlc < $WORKDIR/oceebms-339.sql
else
    echo $WORKDIR/oceebms-339.sql missing
    echo Aborting script.
    exit
fi

echo Deleting the current software
cd $SITEDIR
rm -rf modules/* themes/*

echo Replacing it with new
cd $WORKDIR
cp -r modules $SITEDIR/ || { echo cp modules failed; exit; }
cp -r themes $SITEDIR/ || { echo cp themes failed; exit; }

echo Disabling and re-enabling the site modules
cd $SITEDIR
drush -y dis ebms ebms_content ebms_forums ebms_webforms
drush -y en ebms ebms_content ebms_forums ebms_webforms

echo Refreshing settings for text editing filters
drush -y en ebms_config
drush fr ebms_config

echo Installing common sites file
cd $SITEDIR
cp -f $WORKDIR/sites.php ..

echo Clearing caches
cd $SITEDIR
drush cc all
drush cc all

echo Putting site back into live mode
drush vset maintenance_mode 0

echo Done
