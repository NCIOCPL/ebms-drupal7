# Script for deploying EBMS Patch 3.9.1 (PHP 8.0.21 upgrade support).

echo Verifying account running script
if [ $(whoami) != "drupal" ]
then
    echo This script must be run using the drupal account.
    echo Aborting script.
    exit
fi

echo Setting locations
export NCIOCPL=https://api.github.com/repos/NCIOCPL
export URL=$NCIOCPL/ebms/tarball/oceebms-626
export WORKDIR=/tmp/ebms-3.9.1
export SITEDIR=/local/drupal/sites/ebms.nci.nih.gov
export SITES_ALL=/local/drupal/sites/all
export CURL="curl -L -s -k"
echo "Site directory is $SITEDIR"

echo Creating a working directory at $WORKDIR
mkdir $WORKDIR || {
    echo creating $WORKDIR failed
    exit 1
}

echo Backing up existing files
cd $SITEDIR
tar -czf $WORKDIR/ebms-pre-release-3.9.1-backup.tgz modules themes || {
    echo "tar of themes and modules failed"
    exit 1
}
cd $SITES_ALL
tar -czf $WORKDIR/fpdf-181.tgz libraries/fpdf
tar -czf $WORKDIR/PHPExcel-broken.tgz libraries/PHPExcel
tar -czf $WORKDIR/ckeditor_link.tgz modules/ckeditor_link
tar -czf $WORKDIR/webform.tgz modules/webform

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

echo Updating third-party modules
drush up -y date-7.x-2.12 role_delegation-7.x-1.3 wysiwyg-7.x-2.9 \
  ctools-7.x-1.20 entity-7.x-1.10 features-7.x-2.14 token-7.x-1.9 \
  video_filter-7.x-3.5 views-7.x-3.25

echo Updating Drupal core
drush up -y drupal-7.91

echo Patching webform module
cd $SITES_ALL
cp $WORKDIR/ebms/scripts/number.inc modules/webform/components/

echo Patching ckeditor_link module
cp $WORKDIR/ebms/scripts/ckeditor_link.module modules/ckeditor_link/

echo Updating fpdf library
cd $SITES_ALL/libraries
rm -rf fpdf
tar -xzf $WORKDIR/ebms/scripts/fpdf184.tgz
mv fpdf184 fpdf

echo Updating PHPExcel library
cd $SITES_ALL/libraries
rm -rf PHPExcel
tar -xzf $WORKDIR/ebms/scripts/PHPExcel-hacked-20220804.tgz

echo Patching EBMS code
cd $WORKDIR/ebms/ebms.nci.nih.gov/modules/custom/ebms
cp *.inc $SITEDIR/modules/custom/ebms/
cp js/*.js $SITEDIR/modules/custom/ebms/js/
cd $WORKDIR/ebms/ebms.nci.nih.gov/modules/custom/ebms_webforms/templates
cp *.php $SITEDIR/modules/custom/ebms_webforms/templates/

echo Adding logo image
cd $WORKDIR/ebms/ebms.nci.nih.gov/themes/ebmstheme
cp logo.png $SITEDIR/themes/ebmstheme/

echo Clearing caches twice, once is not always sufficient
cd $SITEDIR
drush cc all
drush cc all

echo Putting site back into live mode
drush vset maintenance_mode 0

echo Done
