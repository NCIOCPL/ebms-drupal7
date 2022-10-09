#!/bin/bash

start_time=${SECONDS}
date
SUDO=$(which sudo)
REPO_BASE=$(pwd)
while getopts r: flag
do
    case "${flag}" in
        r) REPO_BASE=${OPTARG};;
    esac
done
export REPO_BASE
DRUSH=${REPO_BASE}/vendor/bin/drush
DATA=${REPO_BASE}/testdata
SITE=${REPO_BASE}/web/sites/default
UNVERSIONED=${REPO_BASE}/unversioned
DBURL=$(cat ${UNVERSIONED}/dburl)
ADMINPW=$(cat ${UNVERSIONED}/adminpw)
SITEHOST=$(cat ${UNVERSIONED}/sitehost)
echo options: > ${REPO_BASE}/drush/drush.yml
case $SITEHOST in
    *localhost*)
        echo "  uri: http://$SITEHOST" >> ${REPO_BASE}/drush/drush.yml
        ;;
    *)
        echo "  uri: https://$SITEHOST" >> ${REPO_BASE}/drush/drush.yml
        ;;
esac
$SUDO chmod a+w ${SITE}
[ -d ${SITE}/files ] && $SUDO chmod -R a+w ${SITE}/files && rm -rf ${SITE}/files/*
[ -f ${SITE}/settings.php ] && $SUDO chmod +w ${SITE}/settings.php
cp -f ${SITE}/default.settings.php ${SITE}/settings.php
$SUDO chmod +w ${SITE}/settings.php
echo "\$settings['trusted_host_patterns'] = ['^$SITEHOST\$'];" >> ${SITE}/settings.php
$DRUSH si -y --site-name EBMS --account-pass=${ADMINPW} --db-url=${DBURL} \
       --site-mail=ebms@cancer.gov
$SUDO chmod -w ${SITE}/settings.php
$DRUSH pmu contact
$DRUSH then uswds_base
$DRUSH then ebms
$DRUSH en datetime_range
$DRUSH en devel
$DRUSH en linkit
$DRUSH en role_delegation
$DRUSH en editor_advanced_link
$DRUSH en ebms_core
$DRUSH en ebms_board
$DRUSH en ebms_journal
$DRUSH en ebms_group
$DRUSH en ebms_message
$DRUSH en ebms_topic
$DRUSH en ebms_meeting
$DRUSH en ebms_state
$DRUSH en ebms_article
$DRUSH en ebms_import
$DRUSH en ebms_user
$DRUSH en ebms_doc
$DRUSH en ebms_review
$DRUSH en ebms_summary
$DRUSH en ebms_travel
$DRUSH en ebms_home
$DRUSH en ebms_menu
$DRUSH en ebms_report
$DRUSH en ebms_breadcrumb
$DRUSH en ebms_help
$DRUSH cset -y -q system.theme default ebms
$DRUSH cset -y -q system.site page.front /home
$DRUSH cset -y -q system.date country.default US
$DRUSH cset -y -q system.date timezone.default America/New_York
$DRUSH cset -y -q system.date timezone.user.configurable false
$DRUSH scr --script-path=$DATA vocabularies
$DRUSH scr --script-path=$DATA users
$DRUSH scr --script-path=$DATA files
$DRUSH scr --script-path=$DATA boards
$DRUSH scr --script-path=$DATA journals
$DRUSH scr --script-path=$DATA groups
$DRUSH scr --script-path=$DATA topics
$DRUSH scr --script-path=$DATA meetings
$DRUSH scr --script-path=$DATA docs
$DRUSH scr --script-path=$DATA articles
$DRUSH scr --script-path=$DATA imports
$DRUSH scr --script-path=$DATA reviews
$DRUSH scr --script-path=$DATA summaries
$DRUSH scr --script-path=$DATA travel
$DRUSH scr --script-path=$DATA messages
$DRUSH scr --script-path=$DATA assets
$DRUSH scr --script-path=$DATA pubtypes
$DRUSH scr --script-path=$DATA help
$DRUSH scr --script-path=$DATA about
$SUDO chmod -R 777 ${SITE}/files
date
elapsed=$(( SECONDS - start_time ))
eval "echo Elapsed time: $(date -ud "@$elapsed" +'$((%s/3600/24)) days %H hr %M min %S sec')"
