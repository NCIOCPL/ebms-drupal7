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
3. A tar file with the old EBMS /files directory is copied to the new server
4. The article XML is copied to the new server
5. The script to extract the data from the existing EBMS is run (~ 1/2 hour)
6. The script to install Drupal 9, enable the modules, and load the data is run (about 9 hours)
7. The Drupal 7 EBMS site is put into maintenance mode
8. The files, XML, and extracted database values are refreshed and applied

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

Execute the following commands on the new server.

```
cd /local/drupal/ebms
curl -L https://api.github.com/repos/NCIOCPL/ebms/tarball/ebms4 | tar -xzf -
mv NCIOCPL-ebms-*/* .
rm -rf NCIOCPL-ebms-*
```

## Fetch Archive of EBMS User Files

Execute these commands (or commands which produce the same result more
effectively, given the permissions/accounts to which you have access)
on the new server.

```
cd /local/drupal/ebms/unversioned
scp nciws-d2387-v:/local/drupal/ebms/unversioned/files.tar .
```

## Copy Article XML

Execute these commands (or their equivalent).

```
cd /local/drupal/ebms/unversioned
rsync -a nciws-d2387-v:/local/drupal/ebms/unversioned/Articles ./
```

## Copy EBMS Data

Execute these commands on the new server.

```
cd /local/drupal/ebms/unversioned
scp nciws-d2387-v:/local/drupal/ebms/unversioned/ebms3db.json .
cd ../migration
./export.py
```

adminpw
dburl
ebms3db.json
sitehost
userpw
migration/about.html
migration/articles
migration/articles.manifest
migration/articles.sums ?
migration/authmap.json ?
migration/baseline ?
migration/deltas ?
migration/developers
migration/exported ?
migration/files ?
migration/files.manifest ?
migration/files.sums ?
migration/files.tar
migration/fix-reimbursement-values.php
migration/help
migration/hotel-form.html
migration/inline-images
migration/ncihelp
migration/packet_article_ids ?
migration/reimbursement-form.html
migration/travel-directions.html
migration/travel-directions.html
migration/travel-manager
migration/travel-policies.html
