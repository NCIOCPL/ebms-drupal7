# EBMS Migration

These instructions explain the plan for creating the Drupal 9 replacement
for EBMS, which is currently running on Drupal 7. This implementation has
been completely rewritten, and the data structures are all different than
they were in the earlier version of the software, which made extensive
use of custom data tables because the Drupal entity APIs were not yet
available for production use when the EBMS was first implemented.

## Overview

The steps, at a broad level, include:

1. Linux server is provisioned
2. The software for the new EBMS is installed on the server from the repository
3. The files which are not under version control are copied to the new server
4. The script to extract the data from the existing EBMS is run (~ 1/2 hour)
5. The script to install Drupal 9, enable the modules, and load the data is run (between 9 and 15 hours)
6. The Drupal 7 EBMS site is put into maintenance mode
7. The files, XML, and extracted database values are refreshed and applied (approximately an hour)
8. The DNS name ebms.nci.nih.gov is pointed to the new server

## Server Provisioning

CBIIT creates a virtual server for each of the four tiers, using the
requirements attached to ServiceNow ticket NCI-RITM0368065 (the ticket
for the DEV tier's server). While we are still running the Drupal 7
system in parallel with development and testing of the new Drupal 9
rewrite, the new servers will be given temporary DNS names which will
be flipped over to the canonical names. The exception is the STAGE
tier, which for Drupal 7 uses a name which does not match the standard
naming convention for the tiers (ebms-test.nci.nih.gov).

* ebms4-dev.nci.nih.gov (will become ebms-dev.nci.nih.gov)
* ebms4-qa.nci.nih.gov (will become ebms-qa.nci.nih.gov)
* ebms-stage.nci.nih.gov (will keep this name)
* ebms4.nci.nih.gov (will become ebms.nci.nih.gov)

## Install Software From GitHub

Execute the following commands on the new server. From this point on all commands should be run as the `drupal` account (`sudo su - drupal`).

```
cd /local/drupal/ebms
curl -L https://api.github.com/repos/NCIOCPL/ebms/tarball/ebms4 | tar -xzf -
mv NCIOCPL-ebms-*/* .
rm -rf NCIOCPL-ebms-*
```

## Fetch the unversioned files

Execute these commands on the new server.

```
cd /local/drupal/ebms
rsync -a nciws-d2387-v:/local/drupal/ebms/unversioned ./
```

Edit the file `unversioned/dburl` so that it contains the correct
database credentials, host name, and port number. Similarly, edit the
file `unversioned/sitehost` so that it contains the correct name for
the web host (_e.g._, `ebms4.nci.nih.gov`).

## Copy EBMS Data

Execute these commands on the new server. They will extract the
necessary values from the Drupal 7 EBMS database tables into
JSON-serialized lines, one line for each database table row.

```
cd /local/drupal/ebms/migration
./export.py
```

## Create the Web Site

Creation of the new production site should probably happen a few days
before the planned cutover to the new site. To create the site,
execute these commands on the new server. The `migrate.sh` command can
take 14-15 hours to complete on the CBIIT-hosted servers, so it is
necessary to start the job early in the morning so that it completes
before midnight, when CBIIT performs database and/or network
maintenance which would cause the job to fail. Even though this step
is run in the background, it is necessary to wait until it has
finished before proceeding with the next commands. You can monitor
progress while `migrate.sh` is running by executing `cat nohup.out`
from time to time.

```
cd /local/drupal/ebms
nohup migration/migrate.sh &
cd unversioned
rm -rf baseline
mv exported baseline
cd ..
TODAY=`/bin/date +"%Y%m%d"`
drush sql:dump | gzip > ~/ebms-$TODAY.sql.gz
```

At this point the new site has been left in maintenance mode. When
testing these steps on lower tiers it is generally safe to take the
site out of maintenance mode. To do this using the web interface,
perform the following steps (replacing "-stage" with "-qa" or "-dev"
if appropriate for the current tier):

* log onto https://ebms-stage.nci.nih.gov as `admin`
* navigate to /admin/config/development/maintenance
* uncheck the "Put site into maintenance mode" box
* click the "Save configuration" button

As an alternate method, this can be done from the command line using `drush`.

* log onto the web server using ssh
* `sudo` to the drupal account
* change to the /local/drupal/ebms directory
* run the command `drush state:set system.maintenance_mode 0`

## Turn Off User Access

Everything from this point on is done at the time agreed with the users
for switching over to the new production site. We need to block all activity
on the old production server which might change any data. To do this we
put the Drupal 7 production site into maintenance mode. The process for
doing that is very similar to the instructions given above for taking a
new Drupal 9 site *out* of maintennce mode. Perform the following steps:

* log onto https://ebms.nci.nih.gov as an administrator
* navigate to /admin/config/development/maintenance
* check the "Put site into maintenance mode" box
* click the "Save configuration" button

As an alternate method, this can be done from the command line using `drush`.

* log onto nciws-p2154-v using ssh
* `sudo` to the drupal account
* change to the /local/drupal/sites/ebms.nci.nih.gov directory
* run the command `drush vset maintenance_mode 1`

## Top Up the New Server

Next we need to fetch all of the changes which have been made to the
production data since we captured our initial snapshot (see *Copy EBMS
Data* above). Run the following steps on the new server (after
confirming that the `exported` subdirectory of the `unversioned`
directory from the previous export run has been moved to `baseline`,
which should already have happened in the *Create the Web Site* steps
above):

```
cd /local/drupal/ebms/migration
./export.py
./refresh-article-xml.py
rm -rf ../unversioned/files
mkdir ../unversioned/files
./get-new-files.py
rsync -a ../unversioned/files ../web/sites/default/
./find-deltas-from-baseline.py
cd ..
migration/apply-deltas.sh
```

## Bring the New Server Online

After the development team has done some spot-checking to make sure
everything has landed safely, CBIIT can do their magic to have
ebms.nci.nih.gov point to the new server, and we can let the users
know it's ready. Use the instructions given above in the *Create the
Web Site* section to take the site out of maintenance mode.

When the server name is changed, be sure to edit the line near the
bottom of `web/sites/default/settings.php` to reflect the new host
name. For example:

```
$settings['trusted_host_patterns'] = ['^ebms.nci.nih.gov$'];
```
