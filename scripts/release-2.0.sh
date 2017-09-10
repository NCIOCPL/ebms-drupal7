SVN_TRUNK=https://ncisvn.nci.nih.gov/svn/oce_cdr/trunk
SVN_WORK=`/bin/date +"/tmp/ebms-release-2.0-%Y%m%d%H%M%S"`

# Put the EBMS web site into maintenanace mode.
cd /web/appdev/sites/ebms.nci.nih.gov
drush vset maintenance_mode 1

# Disable the EBMS custom modules.
drush -y dis ebms ebms_content ebms_forums ebms_webforms

# Refresh EBMS code from version control repository.
rm -rf modules themes
cd /tmp
mkdir $SVN_WORK
cd $SVN_WORK
svn export $SVN_TRUNK/ebms/ebms.nci.nih.gov/modules@12294
svn export $SVN_TRUNK/ebms/ebms.nci.nih.gov/themes@12294
cp -r modules /web/appdev/sites/ebms.nci.nih.gov/
cp -r themes /web/appdev/sites/ebms.nci.nih.gov/
cd /web/appdev
/home/drupal/fixPermissions.sh

# Re-enable the EBMS custom modules.
cd /web/appdev/sites/ebms.nci.nih.gov
drush -y en ebms ebms_content ebms_forums ebms_webforms

# Force code changes for the calendar views to override the DB.
drush vr event_calendar

# Clear the Drupal caches.
drush cc all

# Take the site back out of maintenance mode.
drush vset maintenance_mode 0
