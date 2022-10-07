"""Support for connecting to the Drupal 7 EBMS database.

Expects the file ebms3db.json to be in the directory../unversioned.
We do not store that file in the version control repository, as it
contains sensitive information.

Example usage:

    from ebms_db import DBMS
    cursor = DBMS().connect().cursor()
"""

from json import load
from pymysql import connect, cursors

class DBMS:
    def connect(self):
        with open("../unversioned/ebms3db.json", encoding="utf-8") as fp:
            opts = load(fp)
        opts["cursorclass"] = cursors.DictCursor
        return connect(**opts)
