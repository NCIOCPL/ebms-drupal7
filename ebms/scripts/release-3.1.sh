# Script for deploying EBMS Release 3.1 to a tier

echo Setting locations
export SVN_EBMS31=https://ncisvn.nci.nih.gov/svn/oce_cdr/branches/ebms-3.1
export WORKDIR=/tmp/ebms-3.1
export SITEDIR=/web/appdev/sites/ebms.nci.nih.gov
export AUTOLOGOUT_MODULE=/tmp/autologout-7.x-4.3.tar.gz

echo Install and configure autologout module
cd /tmp
wget http://ftp.drupal.org/files/projects/autologout-7.x-4.3.tar.gz
if [ ! -f $AUTOLOGOUT_MODULE ]
then
    echo "File $AUTOLOGOUT_MODULE does not exist. Exiting..."
    exit 1
fi
cd /web/appdev/sites/all/modules
tar xzf $AUTOLOGOUT_MODULE || { echo tar unpack failed; exit; }
cd $SITEDIR
drush en --yes autologout
drush vset --yes autologout_timeout 3500
drush vset --yes autologout_padding 270
echo -n "Your session is about to expire. Do you want to remain logged in?" \
 | drush vset autologout_message -

echo Configure WYSIWYG editor link picklist size
cd $SITEDIR
drush vset --yes ckeditor_link_limit 100

echo Create temp work directory
mkdir $WORKDIR || { echo creating $WORKDIR failed; exit; }

echo Backup existing files
cd $SITEDIR
tar cvjf $WORKDIR/ebms-release-3.0-backup.tar modules themes || { echo tar failed; exit; }

echo Users off
cd $SITEDIR
drush vset maintenance_mode 1

echo Get the new release from svn
cd $WORKDIR
svn export $SVN_EBMS31/ebms.nci.nih.gov/modules || { echo modules export failed; exit; }
svn export $SVN_EBMS31/ebms.nci.nih.gov/themes || { echo themes export failed; exit; }

echo Delete the current software
cd $SITEDIR
rm -rf modules themes

echo Replace it with new
cd $WORKDIR
cp -r modules $SITEDIR || { echo cp modules failed; exit; }
cp -r themes $SITEDIR || { echo cp themes failed; exit; }

echo Set correct working permissions
cd /web/appdev
/home/drupal/fixPermissions.sh

echo Reset all drupal cached data
cd $SITEDIR
echo Replace view info in the database with info exported from dev to svn
drush vr recent_activity event_calendar message ebms_forum
echo Clear caches
drush cc all
echo Disable and re-enable our modules
drush -y dis ebms ebms_content ebms_forums ebms_webforms
drush -y en ebms ebms_content ebms_forums ebms_webforms

echo Ready for users
drush vset maintenance_mode 0
