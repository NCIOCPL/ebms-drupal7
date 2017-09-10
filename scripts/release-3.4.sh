# Script for deploying EBMS Release 3.4 to a tier

echo Setting locations
export SVN_BASE=https://ncisvn.nci.nih.gov/svn
export SVN_SITES=$SVN_BASE/oce_dev/Products/Drupal/sites/sites.php
export SVN_EBMS34=$SVN_BASE/oce_cdr/branches/ebms-3.4
export WORKDIR=/tmp/ebms-3.4
export SITEDIR=/local/drupal/sites/ebms.nci.nih.gov

echo "Site directory is $SITEDIR"

echo Creating a working directory at $WORKDIR
mkdir $WORKDIR || {
    echo creating $WORKDIR failed
    exit
}

echo Backing up existing files
cd $SITEDIR
tar cjf $WORKDIR/ebms-pre-release-3.4-backup.tar.bz2 modules themes || {
    echo tar failed
    exit
}

echo Fetching the new release from svn
cd $WORKDIR
svn export -q $SVN_SITES || {
    echo export of release-3.4-part-2.sh failed
    exit
}
svn export -q $SVN_EBMS34/ebms.nci.nih.gov/modules || {
    echo modules export failed
    exit
}
svn export -q $SVN_EBMS34/ebms.nci.nih.gov/themes || {
    echo themes export failed
    exit
}
svn export -q $SVN_EBMS34/sql/oceebms-313.sql || {
    echo export of oceebms-313.sql failed
    exit
}
svn export -q $SVN_EBMS34/sql/oceebms-314.sql || {
    echo export of oceebms-314.sql failed
    exit
}
svn export -q $SVN_EBMS34/sql/oceebms-339.sql || {
    echo export of oceebms-339.sql failed
    exit
}
svn export -q $SVN_EBMS34/scripts/release-3.4-part-2.sh || {
    echo export of release-3.4-part-2.sh failed
    exit
}
chmod +x $WORKDIR/release-3.4-part-2.sh

echo Switch to the drupal account and run $WORKDIR/release-3.4-part-2.sh
