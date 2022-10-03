#!/bin/bash

# Must be run from the git repository directory.
if [ ! -f "dburl" ]; then
    echo Must be run from the git repository directory
    exit 1
fi

REPO_BASE=$(pwd)
while getopts r: flag
do
    case "${flag}" in
        r) REPO_BASE=${OPTARG};;
    esac
done
export REPO_BASE
export EBMS_MIGRATION_LOAD=1
DRUSH=${REPO_BASE}/vendor/bin/drush
MIGRATION=${REPO_BASE}/migration
date
$DRUSH scr --script-path=$MIGRATION apply-deltas
