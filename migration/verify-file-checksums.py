#!/usr/bin/env python3

"""Compare file checksums between the Drupal 7 and Drupal 9 servers.

After the final migration to the new production site, in order to confirm
that the files were transferred intact, you can perform the following steps:

  # On nciws-p2154-v using the drupal login account:
  NAME=`/bin/date +"/local/home/drupal/ebms-files-%Y%m%d.sums.gz"`
  cd /local/drupal/sites/ebms.nci.nih.gov
  /bin/find files -type f -exec /bin/sha1sum '{}' \; | /bin/gzip > $NAME

  # On the new production EBMS web server, to which the file created
  # on nciws-p2154-v has been copied:
  cd /local/drupal/ebms/migration
  /bin/gzip < FILE_COPIED_FROM_NCIWS-P2154-V | ./verify-file-checksums.py
"""

from argparse import ArgumentParser
from datetime import datetime
from json import loads
from sys import stdin

PREFIX = "public://"

parser = ArgumentParser()
parser.add_argument("--cutoff", "-c", help="skip files after this date/time")
opts = parser.parse_args()
old_sums = {}
for line in stdin:
    checksum, path = line.strip().split(None, 1)
    old_sums[path] = checksum
new_sums = {}
with open("files.sums", encoding="utf-8") as fp:
    for line in fp:
        checksum, path = line.strip().split(None, 1)
        new_sums[path] = checksum
tested = 0
with open("exported/files.json", encoding="utf-8") as fp:
    for line in fp:
        values = loads(line)
        if opts.cutoff:
            created = datetime.fromtimestamp(values["created"])
            if str(created) > opts.cutoff:
                continue
        uri = values["uri"]
        if uri.startswith(PREFIX):
            tested += 1
            path = "files/" + uri[len(PREFIX):]
            old_sum = old_sums.get(path)
            new_sum = new_sums.get(path)
            if not old_sum:
                print(f"not on old server: {path}")
            elif not new_sum:
                print(f"not on new server: {path}")
            elif old_sum != new_sum:
                print(f"checksum mismatch: {path}")
print(f"tested {tested} files")
