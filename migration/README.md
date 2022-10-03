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