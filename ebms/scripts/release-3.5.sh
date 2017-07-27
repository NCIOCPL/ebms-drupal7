# Script for deploying EBMS Release 3.5 to a tier

echo Verifying required command-line argument
if [ "$#" -ne 1 ]; then
    echo usage: release-ebms-3.5.sh HOST-NAME
    echo " e.g.: release-ebms-3.5.sh ebms-test"
    exit 1
fi

echo Setting locations
export SVN_BASE=https://ncisvn.nci.nih.gov/svn
export SVN_EBMS35=$SVN_BASE/oce_cdr/branches/ebms-3.5
export SVN_DRUPAL=$SVN_BASE/oce_dev/Products/Drupal
export SVN_SSO=$SVN_DRUPAL/shared/modules/Custom/nci_SSO
export SVN_SSO15=$SVN_SSO/branches/1.5
export SVN_SITES=$SVN_DRUPAL/sites/sites.php
export WORKDIR=/tmp/ebms-3.5
export SITEDIR=/local/drupal/sites/ebms.nci.nih.gov
export SSO=all/modules/Custom/nci_SSO

echo "Site directory is $SITEDIR"

echo Creating a working directory at $WORKDIR
mkdir $WORKDIR || {
    echo creating $WORKDIR failed
    exit
}

echo Backing up existing files
cd $SITEDIR
tar cjf $WORKDIR/ebms-pre-release-3.5-backup.tar.bz2 modules themes || {
    echo "tar of themes and modules failed"
    exit
}
cd ..
tar cjf $WORKDIR/ebms-pre-release-3.5-nci_SSO-backup.tar.bz2 $SSO || {
    echo tar of nci_SSO failed
    exit
}
cp $SITEDIR/../sites.php $WORKDIR/sites.php-pre-release-3.5-backup

echo Fetching the new release from svn
cd $WORKDIR
svn export -q $SVN_EBMS35/ebms.nci.nih.gov/modules || {
    echo modules export failed
    exit
}
svn export -q $SVN_EBMS35/ebms.nci.nih.gov/themes || {
    echo themes export failed
    exit
}
svn export -q $SVN_EBMS35/sql/oceebms-427.sql || {
    echo export of oceebms-427.sql failed
    exit
}
svn export -q $SVN_EBMS35/sql/oceebms-444.sql || {
    echo export of oceebms-444.sql failed
    exit
}
svn export -q $SVN_EBMS35/scheduled || {
    echo export of cron scripts failed
    exit
}
svn export -q $SVN_EBMS35/scripts/release-3.5-part-2.sh || {
    echo export of release-3.5-part-2.sh failed
    exit
}
svn export -q $SVN_SITES || {
    echo export of sites.php failed
    exit
}
svn export -q $SVN_SSO15 nci_SSO || {
    echo export of nci_SSO failed
    exit
}
chmod +x $WORKDIR/release-3.5-part-2.sh

echo Customizing crontab for $1
sed -i -e "s/@@HOST@@/$1/" $WORKDIR/scheduled/drupal.crontab

echo Switch to the drupal account and run $WORKDIR/release-3.5-part-2.sh
