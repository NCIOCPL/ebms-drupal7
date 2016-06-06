# Portion of the release 3.2 deployment to be run under the drupal account

if [ $(whoami) != "drupal" ]
then
    echo This script must be run using the drupal account.
    echo Aborting script.
    exit
fi

echo Setting locations
export WORKDIR=/tmp/ebms-3.2
export ESEARCH=http://eutils.ncbi.nlm.nih.gov/entrez/eutils/esearch.fcgi
export OCEEBMS270_RECIPS=***REMOVED***,***REMOVED***
if [ -d "/web/appdev/sites" ]
then
    export SITEDIR=/web/appdev/sites/ebms.nci.nih.gov
else
    export SITEDIR=/local/drupal/sites/ebms.nci.nih.gov
fi
echo "Site directory is $SITEDIR"

echo Putting site into maintenance mode
cd $SITEDIR
drush vset maintenance_mode 1

echo Applying database changes
schema="TABLE_SCHEMA = 'oce_ebms'"
table=information_schema.TABLES
cond="$schema AND TABLE_NAME = 'ebms_topic_group'"
query="SELECT COUNT(*) FROM $table WHERE $cond"
count=`drush sqlq --extra=--skip-column-names "$query"`
if [ $count = "0" ]
then
    if [ -r $WORKDIR/oceebms-161.sql ]
    then
        echo Database changes for OCEEBMS-161
        drush sqlc < $WORKDIR/oceebms-161.sql
    else
        echo $WORKDIR/oceebms-161.sql missing
        echo Aborting script.
        exit
    fi
else
    echo Database changes for OCEEBMS-161 already applied
fi
table=ebms_review_rejection_value
value="Inappropriate study design"
query="SELECT COUNT(*) FROM $table WHERE value_name = '$value'"
count=`drush sqlq --extra=--skip-column-names "$query"`
if [ $count = "1" ]
then
    if [ -r $WORKDIR/oceebms-323.sql ]
    then
        echo Database changes for OCEEBMS-323
        drush sqlc < $WORKDIR/oceebms-323.sql
    else
        echo $WORKDIR/oceebms-323.sql missing
        echo Aborting script.
        exit
    fi
else
    echo Database changes for OCEEBMS-323 already applied
fi
table=ebms_article_tag_type
value="i_core_journals"
query="SELECT COUNT(*) FROM $table WHERE text_id = '$value'"
count=`drush sqlq --extra=--skip-column-names "$query"`
if [ $count = "0" ]
then
    if [ -r $WORKDIR/oceebms-281.sql ]
    then
        echo Database changes for OCEEBMS-281
        drush sqlc < $WORKDIR/oceebms-281.sql
    else
        echo $WORKDIR/oceebms-281.sql missing
        echo Aborting script.
        exit
    fi
else
    echo Database changes for OCEEBMS-281 already applied
fi
if [ -r $WORKDIR/oceebms-304.sql ]
then
    echo Database changes for OCEEBMS-304
    drush sqlc < $WORKDIR/oceebms-304.sql
else
    echo $WORKDIR/oceebms-304.sql missing
    echo Aborting script.
    exit
fi
if [ -r $WORKDIR/oceebms-347.sql ]
then
    echo Database changes for OCEEBMS-347
    drush sqlc < $WORKDIR/oceebms-347.sql
else
    echo $WORKDIR/oceebms-347.sql missing
    echo Aborting script.
    exit
fi
table=information_schema.TABLES
cond="$schema AND TABLE_NAME = 'ebms_related_article'"
query="SELECT COUNT(*) FROM $table WHERE $cond"
count=`drush sqlq --extra=--skip-column-names "$query"`
if [ $count = "0" ]
then
    if [ -r $WORKDIR/oceebms-349.sql ]
    then
        echo Database changes for OCEEBMS-349
        drush sqlc < $WORKDIR/oceebms-349.sql
    else
        echo $WORKDIR/oceebms-349.sql missing
        echo Aborting script.
        exit
    fi
else
    echo Database changes for OCEEBMS-349 already applied
fi
table=information_schema.COLUMNS
cond="$schema AND TABLE_NAME = 'ebms_packet' AND COLUMN_NAME = 'starred'"
query="SELECT COUNT(*) FROM $table WHERE $cond"
count=`drush sqlq --extra=--skip-column-names "$query"`
if [ $count = "0" ]
then
    if [ -r $WORKDIR/oceebms-350.sql ]
    then
        echo Database changes for OCEEBMS-350
        drush sqlc < $WORKDIR/oceebms-350.sql
    else
        echo $WORKDIR/oceebms-350.sql missing
        echo Aborting script.
        exit
    fi
else
    echo Database changes for OCEEBMS-350 already applied
fi

echo Installing variables for OCEEBMS-270
drush vset --yes pubmed_esearch_url $ESEARCH
drush vset --yes pubmed_missing_article_report_recips $OCEEBMS270_RECIPS

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

echo Clearing caches
cd $SITEDIR
drush cc all

echo Putting site into live mode
drush vset maintenance_mode 0

echo Installing cron scripts for OCEEBMS-270
cp $WORKDIR/find-pubmed-drops.* $HOME/cron/
chmod +x $HOME/cron/find-pubmed-drops.sh
echo If this is the production server, add this line to the drupal crontab:
echo "15 2 * * 0 $HOME/cron/find-pubmed-drops.sh"

echo Done
