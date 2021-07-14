# Script for deploying EBMS Release 3.9 ("Denali") to a tier

echo Verifying account running script
if [ $(whoami) != "drupal" ]
then
    echo This script must be run using the drupal account.
    echo Aborting script.
    exit
fi

echo Setting locations
export NCIOCPL=https://api.github.com/repos/NCIOCPL
export URL=$NCIOCPL/ebms/tarball/denali
export WORKDIR=/tmp/ebms-3.9
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
tar -czf $WORKDIR/ebms-pre-release-3.9-backup.tgz modules themes || {
    echo "tar of themes and modules failed"
    exit 1
}
cp $SITEDIR/../sites.php $WORKDIR/sites.php-pre-release-3.9-backup

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

echo Applying database changes
table=ebms_article_tag_type
cond="text_id = 'preliminary'"
query="SELECT COUNT(*) FROM $table WHERE $cond"
count=`drush sqlq --extra=--skip-column-names "$query"`
if [ $count = "0" ]
then
    if [ -r $WORKDIR/ebms/sql/oceebms-592.sql ]
    then
        echo Database changes for OCEEBMS-592
        drush sqlc < $WORKDIR/ebms/sql/oceebms-592.sql
    else
        echo $WORKDIR/ebms/sql/oceebms-592.sql missing
        echo Aborting script.
        exit
    fi
else
    echo Database changes for OCEEBMS-592 already applied
fi

echo Clearing caches twice, once is not always sufficient
drush cc all
drush cc all

echo Putting site back into live mode
drush vset maintenance_mode 0

echo Done
