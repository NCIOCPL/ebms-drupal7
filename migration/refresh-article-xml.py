#!/usr/bin/env python3

"""
Get new and changed PubMed article XML from the production Drupal 7 EBMS.

1. load the information from the manifest written by the previous run
   into a dictionary indexed by the articles' integer IDs, each record
   containing a tuple of the following values:
     * hex representation of the SHA1 checksum for the XML document's bytes
     * string representation of the article's unique EBMS ID
     * string representation of the size of the XML document (in UTF-8 bytes)
     * date/time when the article was last added/refreshed (YYYYMMDDhhmmss)
2. Move the manifest file we just loaded to articles.manifest.TIMESTAMP,
   and open new files for writing a fresh manifest, as well as a checksum
   file which can be used by "sha1sum -c articles.sums"
3. Fetch the EBMS article IDs and SHA1 checksum for the PubMed XML from the
   database
4. For each row in the set fetched in step 3:
     - if we already had the article and the checksum has not changed:
         * copy the line from the original manifest file to the new manifest
         * add a line to the new checksum file
     - otherwise:
         * note whether this is a new or a modified XML file
         * fetch the XML from the database
         * write a line to the new manifest file for the article's XML
         * add a line to the new checksum file
5. Close the files opened in step 3
6. Report how many XML files were added and how many were modified

There is a --report-only option which can be passed to the script invocation,
which causes the program to report what it would have done, without actually
doing it.
"""

from argparse import ArgumentParser
from datetime import datetime
from pathlib import Path
from ebms_db import DBMS

SELECT_XML = "SELECT source_data FROM ebms_article WHERE article_id = %s"

start = datetime.now()
parser = ArgumentParser()
parser.add_argument("--report-only", "-r", action="store_true")
opts = parser.parse_args()
with open("articles.manifest", encoding="utf-8") as fp:
    articles = {}
    for line in fp:
        sha1, article_id, filesize, timestamp = line.strip().split()
        articles[int(article_id)] = sha1, article_id, filesize, timestamp
if not opts.report_only:
    path = Path("articles.manifest")
    stamp = int(path.stat().st_mtime)
    name = f"articles.manifest.{stamp}"
    path.rename(name)
    manifest_fp = open("articles.manifest", "w", encoding="utf-8")
    sums_fp = open("articles.sums", "w", encoding="utf-8")
cursor = DBMS().connect().cursor()
cursor.execute("SELECT article_id, SHA1(source_data) AS sha1_hash "
               "FROM ebms_article ORDER BY article_id")
updated = added = 0
for row in cursor.fetchall():
    article_id = row["article_id"]
    sha1 = row["sha1_hash"]
    if article_id in articles:
        if articles[article_id][0] == sha1:
            if not opts.report_only:
                manifest_fp.write(" ".join(articles[article_id]) + "\n")
                sha1 = articles[article_id][0]
                article_id = articles[article_id][1]
                sums_fp.write(f"{sha1} articles/{article_id}.xml\n")
            continue
        else:
            stamp = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
            print(f"[{stamp}] updating article {article_id}")
            updated += 1
    else:
        stamp = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
        print(f"[{stamp}] adding article {article_id}")
        added += 1
    if not opts.report_only:
        cursor.execute(SELECT_XML, (article_id,))
        article_xml = cursor.fetchone()["source_data"].encode("utf-8")
        filesize = len(article_xml)
        now = datetime.now().strftime("%Y%m%d%H%M%S")
        #with open(f"articles.updated/{article_id}.xml", "wb") as fp:
        #    fp.write(article_xml)
        with open(f"articles/{article_id}.xml", "wb") as fp:
            fp.write(article_xml)
        manifest_fp.write(f"{sha1} {article_id} {filesize} {now}\n")
        sums_fp.write(f"{sha1} articles/{article_id}.xml\n")
if not opts.report_only:
    manifest_fp.close()
    sums_fp.close()
elapsed = datetime.now() - start
print(f"updated {updated} articles")
print(f"added {added} articles")
print(f"elapsed: {elapsed}")
