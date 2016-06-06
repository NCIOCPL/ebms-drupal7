# Script for deploying EBMS Release 3.2 to a tier

echo Setting locations
export SVN_EBMS32=https://ncisvn.nci.nih.gov/svn/oce_cdr/branches/ebms-3.2
export WORKDIR=/tmp/ebms-3.2
if [ -d "/web/appdev/sites" ]
then
    export SITEDIR=/web/appdev/sites/ebms.nci.nih.gov
else
    export SITEDIR=/local/drupal/sites/ebms.nci.nih.gov
fi
echo "Site directory is $SITEDIR"

echo Creating a working directory at $WORKDIR
mkdir $WORKDIR || {
    echo creating $WORKDIR failed
    exit
}

echo Backing up existing files
cd $SITEDIR
tar cjf $WORKDIR/ebms-pre-release-3.2-backup.tar modules themes || {
    echo tar failed
    exit
}

echo Fetching the new release from svn
cd $WORKDIR
svn export -q $SVN_EBMS32/ebms.nci.nih.gov/modules || {
    echo modules export failed
    exit
}
svn export -q $SVN_EBMS32/ebms.nci.nih.gov/themes || {
    echo themes export failed
    exit
}
svn export -q $SVN_EBMS32/sql/oceebms-161.sql || {
    echo export of oceebms-161.sql failed
    exit
}
svn export -q $SVN_EBMS32/sql/oceebms-281.sql || {
    echo export of oceebms-281.sql failed
    exit
}
svn export -q $SVN_EBMS32/sql/oceebms-304.sql || {
    echo export of oceebms-304.sql failed
    exit
}
svn export -q $SVN_EBMS32/sql/oceebms-323.sql || {
    echo export of oceebms-323.sql failed
    exit
}
svn export -q $SVN_EBMS32/sql/oceebms-347.sql || {
    echo export of oceebms-347.sql failed
    exit
}
svn export -q $SVN_EBMS32/sql/oceebms-349.sql || {
    echo export of oceebms-349.sql failed
    exit
}
svn export -q $SVN_EBMS32/sql/oceebms-350.sql || {
    echo export of oceebms-350.sql failed
    exit
}
svn export -q $SVN_EBMS32/scripts/release-3.2-part-2.sh || {
    echo export of release-3.2-part-2.sh failed
    exit
}
svn export -q $SVN_EBMS32/scheduled/find-pubmed-drops.sh || {
    echo export of find-pubmed-drops.sh failed
    exit
}
svn export -q $SVN_EBMS32/scheduled/find-pubmed-drops.php || {
    echo export of find-pubmed-drops.php failed
    exit
}
chmod +x $WORKDIR/release-3.2-part-2.sh
echo Switch to the drupal account and run $WORKDIR/release-3.2-part-2.sh
