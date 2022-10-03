#!/usr/bin/env python3

"""Determine what needs to happen to bring the EBMS 4 database up to date.

This script in run when one of the following sequence has occurred:

  1. The export.py script is run, extracting records from EBMS PROD
  2. The results of that script are imported into the Drupal 9 EBMS
  3. The "exported" directory is renamed "baseline"
  4. Time passes and EBMS PROD data evolves
  5. The export.py script is run again

The job of this script is to create and populate a fres "deltas" directory,
which will contain new and changed records derived by comparing the contents
of the "baseline" directory with the just-populated "exported" directory.

Two other things need to happen at this point:

  1. The set of article XML files needs to be updated (refresh-article-xml.py)
  2. The set of managed files needs to be updated (get-new-files.py)

Each of those scripts has detailed explanations at the top describing what
they do and how they are to be executed.

Finally, the apply-updates.sh script can be run, bringing the Drupal 9
EBMS up to date with the running Drupal 7 production EBMS.

Of course, the only time when the new Drupal 9 EBMS is guaranteed to be
completely in sync with the Drupal 7 production EBMS is the final cutover
when user access to the live EBMS has been turned off, and the scheduled
jobs have been disabled. In every other case, while we're running these
steps, the users and scheduled jobs will be busy performing ongoing work,
changing the data in the Drupal 7 instance.
"""

from datetime import datetime
from hashlib import sha1
from json import loads
from pathlib import Path

ID_KEYS = dict(
    board_summaries="board",
    files="fid",
    journals="source_id",
    print_job_statuses_vocabulary="name",
    print_job_types_vocabulary="name",
    users="uid",
)

start = datetime.now()
path = Path("deltas")
if path.exists():
    stamp = datetime.now().strftime("%Y%m%d%H%M%S")
    path.rename(f"deltas-{stamp}")
Path("deltas").mkdir()
Path("deltas/mod").mkdir()
Path("deltas/new").mkdir()
for path in sorted(Path("baseline").glob("*.json")):
    hashes = {}
    name = path.name.replace(".json", "")
    print(f"comparing {name}")
    key = ID_KEYS.get(name, "id")
    with path.open(encoding="utf-8") as fp:
        for line in fp:
            record_id = loads(line)[key]
            hashes[record_id] = sha1(line.encode("utf-8")).hexdigest()
    with open(f"exported/{name}.json", encoding="utf-8") as fp:
        for line in fp:
            record_id = loads(line)[key]
            if record_id not in hashes:
                new_path = f"deltas/new/{name}.json"
                with open(new_path, "a", encoding="utf-8") as new_fp:
                    new_fp.write(line)
            else:
                h = sha1(line.encode("utf-8")).hexdigest()
                if h != hashes[record_id]:
                    mod_path = f"deltas/mod/{name}.json"
                    with open(mod_path, "a", encoding="utf-8") as mod_fp:
                        mod_fp.write(line)
elapsed = datetime.now() - start
print(f"elapsed: {elapsed}")
