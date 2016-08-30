# Script for deploying EBMS Release 3.2 to a tier

echo Setting locations
export SVN_EBMS33=https://ncisvn.nci.nih.gov/svn/oce_cdr/branches/ebms-3.3
export WORKDIR=/tmp/ebms-3.3
export SITEDIR=/local/drupal/sites/ebms.nci.nih.gov
export CKEDITOR=http://download.cksource.com/CKEditor/CKEditor
export CKFULL=$CKEDITOR/CKEditor%204.5.9/ckeditor_4.5.9_full.zip

echo "Site directory is $SITEDIR"

echo Creating a working directory at $WORKDIR
mkdir $WORKDIR || {
    echo creating $WORKDIR failed
    exit
}

echo Backing up existing files
cd $SITEDIR
tar cjf $WORKDIR/ebms-pre-release-3.3-backup.tar.bz2 modules themes || {
    echo tar failed
    exit
}
cd ../all
tar cjf $WORKDIR/old-ckeditor.tar.bz2 libraries/ckeditor || {
    echo tar of old ckeditor failed
    exit
}

echo Fetching the new release from svn
cd $WORKDIR
svn export -q $SVN_EBMS33/ebms.nci.nih.gov/modules || {
    echo modules export failed
    exit
}
svn export -q $SVN_EBMS33/ebms.nci.nih.gov/themes || {
    echo themes export failed
    exit
}
svn export -q $SVN_EBMS33/sql/oceebms-375.sql || {
    echo export of oceebms-375.sql failed
    exit
}
svn export -q $SVN_EBMS33/scripts/oceebms-374.php || {
    echo export of oceebms-374.php failed
    exit
}
svn export -q $SVN_EBMS33/scripts/release-3.3-part-2.sh || {
    echo export of release-3.3-part-2.sh failed
    exit
}
chmod +x $WORKDIR/release-3.3-part-2.sh
wget -q $CKFULL

echo Switch to the drupal account and run $WORKDIR/release-3.3-part-2.sh
