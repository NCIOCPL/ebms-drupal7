# Script for deploying EBMS Release 3.6 ("Acadia") to a tier

echo Verifying account running script
if [ $(whoami) != "drupal" ]
then
    echo This script must be run using the drupal account.
    echo Aborting script.
    exit
fi

echo Setting locations
export NCIOCPL=https://api.github.com/repos/NCIOCPL
export URL=$NCIOCPL/ebms/tarball/acadia
export URL34=$NCIOCPL/ebms/tarball/release-3.4
export EBMS_FORUMS='*/ebms/ebms.nci.nih.gov/modules/custom/ebms_forums'
export WORKDIR=/tmp/ebms-3.6
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
tar cjf $WORKDIR/ebms-pre-release-3.6-backup.tar.bz2 modules themes || {
    echo "tar of themes and modules failed"
    exit 1
}
cp $SITEDIR/../sites.php $WORKDIR/sites.php-pre-release-3.6-backup

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

# CBIIT has an older version of tar installed on the servers,
# so the --wildcard option has to be specified explicitly.
echo Preparing to remove the ebms_forums module for OCEEBMS-504
cd $WORKDIR/ebms/ebms.nci.nih.gov/modules/custom
$CURL $URL34 | tar -xzf - --strip 5 --wildcards $EBMS_FORUMS || {
    echo fetch $URL34 failed
    exit 1
}

echo Putting site into maintenance mode
cd $SITEDIR
drush vset maintenance_mode 1

echo Preparing for the upgrade to PHP 7.2
chmod +w $SITEDIR/settings.php
chmod +w $SITEDIR
sed -i 's/^ini_set.*session.save_handler/#&/' $SITEDIR/settings.php
chmod -w $SITEDIR/settings.php
chmod -w $SITEDIR

echo Applying database changes
cd $SITEDIR
table=ebms_article_tag_type
cond="text_id = 'high_priority'"
query="SELECT COUNT(*) FROM $table WHERE $cond"
count=`drush sqlq --extra=--skip-column-names "$query"`
if [ $count = "0" ]
then
    if [ -r $WORKDIR/ebms/sql/oceebms-469.sql ]
    then
        echo Database changes for OCEEBMS-469
        drush sqlc < $WORKDIR/ebms/sql/oceebms-469.sql
    else
        echo $WORKDIR/ebms/sql/oceebms-469.sql missing
        echo Aborting script.
        exit
    fi
else
    echo Database changes for OCEEBMS-469 already applied
fi
if [ -r $WORKDIR/ebms/sql/oceebms-509.sql ]
then
    echo Database changes for OCEEBMS-509
    drush sqlc < $WORKDIR/ebms/sql/oceebms-509.sql
else
    echo $WORKDIR/ebms/sql/oceebms-509.sql missing
    echo Aborting script.
    exit
fi

echo Deleting the current software
cd $SITEDIR
rm -rf modules/* themes/*

echo Replacing it with new
cd $WORKDIR/ebms/ebms.nci.nih.gov
cp -r modules $SITEDIR/ || { echo cp modules failed; exit; }
cp -r themes $SITEDIR/ || { echo cp themes failed; exit; }

echo Removing the ebms_forums module for OCEEBMS-504
cd $SITEDIR
drush pmu -y ebms_forums
rm -rf modules/custom/ebms_forums

echo Disabling the broken ldap_servers module
drush dis -y ldap_servers

echo Removing unused Organic Groups module
drush field-delete -y og_membership_request
drush field-delete -y group_audience
drush cron
drush dis -y og
drush pmu -y ldap_authorization_og
drush pmu -y og

echo Removing other ldap cruft
drush pmu -y ldap_help ldap_feeds ldap_views nci_edir
drush pmu -y ldap_test ldap_query ldap_authorization_drupal_role
drush pmu -y ldap_authorization ldap_authentication
drush pmu -y ldap_user ldap_servers

echo Applying Drupal security updates - ignore 4 byte UTF-8 for mysql warnings
drush up -y drupal-7.67 chain_menu_access-7.x-2.1 ctools-7.x-1.15
drush up -y role_delegation-7.x-1.2 views-7.x-3.23
drush up -y webform-7.2-4.20 wysiwyg-7.x-2.6
drush cc all

echo Disabling and re-enabling the site modules
drush -y dis ebms ebms_content ebms_webforms
drush -y en ebms ebms_content ebms_webforms

echo Refreshing settings for text editing filters
drush en -y ebms_config
drush fr -y ebms_config

echo Clearing caches twice, once is not always sufficient
drush cc all
drush cc all

echo Putting site back into live mode
drush vset maintenance_mode 0

echo Done
