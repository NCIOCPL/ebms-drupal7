# Portion of the release 3.3 deployment to be run under the drupal account

if [ $(whoami) != "drupal" ]
then
    echo This script must be run using the drupal account.
    echo Aborting script.
    exit
fi

echo Setting locations
export WORKDIR=/tmp/ebms-3.3
export SITES=/local/drupal/sites
export SITEDIR=$SITES/ebms.nci.nih.gov
export ALLSITES=$SITES/all
echo "Site directory is $SITEDIR"

echo Putting site into maintenance mode
cd $SITEDIR
drush vset maintenance_mode 1

echo Upgrading wysiwyg module
drush up -y wysiwyg-7.x-2.x-dev

echo Adding/enabling new video_filter module
rm -rf $ALLSITES/modules/video_filter
drush dl video_filter --destination=sites/all/modules
cd $SITEDIR
drush en -y wysiwyg_filter
drush en -y video_filter
# drush rap 'authenticated user' 'use text format video_html'
# drush rap 'site manager' 'administer filters'

echo Applying database changes
schema="TABLE_SCHEMA LIKE 'oce_ebms%'"
table=information_schema.TABLES
cond="$schema AND TABLE_NAME = 'ebms_article_topic_comment'"
query="SELECT COUNT(*) FROM $table WHERE $cond"
count=`drush sqlq --extra=--skip-column-names "$query"`
if [ $count = "0" ]
then
    if [ -r $WORKDIR/oceebms-375.sql ]
    then
        echo Database changes for OCEEBMS-375
        drush sqlc < $WORKDIR/oceebms-375.sql
    else
        echo $WORKDIR/oceebms-375.sql missing
        echo Aborting script.
        exit
    fi
else
    echo Database changes for OCEEBMS-375 already applied
fi

echo Deleting the current software
cd $SITEDIR
rm -rf modules/* themes/* ../all/libraries/ckeditor

echo Replacing it with new
cd ../all/libraries
unzip -q $WORKDIR/ckeditor_4.5.9_full.zip
cd $WORKDIR
cp -r modules $SITEDIR/ || { echo cp modules failed; exit; }
cp -r themes $SITEDIR/ || { echo cp themes failed; exit; }

echo Disabling and re-enabling the site modules
cd $SITEDIR
drush -y dis ebms ebms_content ebms_forums ebms_webforms
drush -y en ebms ebms_content ebms_forums ebms_webforms

echo Installing settings for text editing filters
drush -y en ebms_config
drush fr ebms_config

echo Set permission for using video filter
cd $SITEDIR
drush --script-path=$WORKDIR scr oceebms-374.php

echo Clearing caches
cd $SITEDIR
drush cc all
drush cc all

echo Putting site into live mode
drush vset maintenance_mode 0

echo Done
