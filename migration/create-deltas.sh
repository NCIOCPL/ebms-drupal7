#!/bin/bash

# Run this script as part of the process of bringing a Drupal 9 EBMS
# server in sync with the current running production Drupal 7 EBMS site.
# Then read and follow the instructions in get-new-files.py to copy
# any new managed files to the ../web/sites/default/files directory.
# Finally, after you are satisfied that the deltas are ready, you
# can move to the root directory of the repository (the parent of this
# directory) and run migration/apply-deltas.sh.


# Takes approximately 20 minutes.
./export.py

# Takes approximately 2 minutes.
./refresh-article-xml.py

# Takes under 1 minute.
./get-new-files.py

# Takes approximately 6 minutes.
./find-deltas-from-baseline.py
