#!/bin/bash

# $Id
#
# Script for deploying EBMS OpenID login patch
# Note: Patch is based on trunk as of early February, 2016.
# Modeled upon Bob's release-3.2 shell script

echo Applying patch to permit OpenID logins
echo
echo Checking login userid
if [ $(whoami) != "drupal" ]
then
    echo This script must be run using the drupal account.
    echo Aborting script.
    exit
fi

echo Finding base of drupal sites
if [ -d "/web/appdev/sites" ]
then
    export DRUPAL_DIR=/web/appdev
else
    export DRUPAL_DIR=/local/drupal
fi
echo Drupal base directory is $DRUPAL_DIR

echo Setting SVN locations
export SVN_OPENID=https://ncisvn.nci.nih.gov/svn/oce_cdr/branches/openid
export SVN_SSO14=https://ncisvn.nci.nih.gov/svn/oce_dev/Products/Drupal/shared/modules/Custom/nci_SSO/branches/1.4

echo Setting site working directory
export WORKDIR=/tmp/ebms-openid
echo Working directory is $WORKDIR

echo Setting site directory for EBMS
export SITEDIR=$DRUPAL_DIR/sites/ebms.nci.nih.gov
echo Site directory for EBMS is $SITEDIR

echo Setting output directories where code goes
export SITE_MODDIR=$SITEDIR/modules/custom/ebms
echo Modules directory is $SITE_MODDIR
export SITE_CSSDIR=$SITEDIR/themes/ebmstheme/css
echo CSS directory is $SITE_CSSDIR
export SITE_SSODIR=$DRUPAL_DIR/sites/all/modules/Custom/nci_SSO 
echo SSO directory is $SITE_SSODIR

echo Putting site into maintenance mode
cd $SITEDIR
drush vset maintenance_mode 1

if [ -d $WORKDIR ]
then
    echo deleting old instance of working directory $WORKDIR
    rm -rf $WORKDIR
fi

echo Creating a working directory at $WORKDIR
mkdir $WORKDIR || {
    echo creating $WORKDIR failed
    exit
}
chmod 755 $WORKDIR

echo Backing up existing files
cd $SITEDIR
export TAR_FILE=$WORKDIR/ebms-openid-backup.tar
tar cf $TAR_FILE \
          modules/custom/ebms/ebms.module \
          themes/ebmstheme/css/ebms.css || {
    echo tar ebms files failed
    exit
}
cd $SITE_SSODIR
tar rf $TAR_FILE * || {
    echo tar SSO files failed
    exit
}
echo Created backup in tar file $TAR_FILE

#### DEBUG TESTING
# echo TESTING WITH TEMPORARY OUTPUT DIRECTORIES !!!
# export SITE_MODDIR=/tmp/ebms-openid
# echo Changed SITE_MODDIR to $SITE_MODDIR
# export SITE_CSSDIR=/tmp/ebms-openid
# echo Changed SITE_CSSDIR to $SITE_CSSDIR
# export SITE_SSODIR=/tmp/ebms-openid
# echo Changed SITE_SSODIR to $SITE_SSODIR
# exit
#### DEBUG TESTING

echo Exporting new software to install from svn
cd $SITE_SSODIR
svn export -q --force $SVN_SSO14 . || {
    echo export of new SSO software failed
    exit
}
chmod +x *

cd $SITE_MODDIR
echo Exporting modified ebms.module
svn export -q --force $SVN_OPENID/ebms.nci.nih.gov/modules/custom/ebms/ebms.module ./ebms.module || {
    echo export of new OpenID software failed
    exit
}
chmod +x ./ebms.module

echo Exporting modified ebms.css
cd $SITE_CSSDIR
svn export -q --force $SVN_OPENID/ebms.nci.nih.gov/themes/ebmstheme/css/ebms.css ./ebms.css || {
    echo export of new OpenID software failed
    exit
}
chmod +x ./ebms.css

echo Exporting SQL script to update board member authnames
cd $WORKDIR
svn export -q --force $SVN_OPENID/scripts/GoogleAuthAddrs.sql ./GoogleAuthAddrs.sql || {
    echo export of authmap update script failed
    exit
}
chmod +x *

echo Backing up the authmap database table
cd $SITEDIR
drush sqlq "DROP TABLE IF EXISTS authmap_backup"
drush sqlq "CREATE TABLE authmap_backup AS (SELECT * FROM authmap ORDER BY aid)"

echo Updating authmap with OpenID Google account email addresses
cd $SITEDIR
drush sqlq --file=$WORKDIR/GoogleAuthAddrs.sql
echo Authmap updated

echo Disabling NCI eDir module.  eDir users will have to switch to OpenID
cd $SITEDIR
drush -y dis nci_edir

echo Clearing caches
cd $SITEDIR
drush cc all

echo Bringing site out of maintenance mode
cd $SITEDIR
drush vset maintenance_mode 0

echo Done!  Please check for errors.
