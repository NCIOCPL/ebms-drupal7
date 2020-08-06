#!/usr/bin/env python3

"""Report on related articles.

From the ticket:

  In order to get a better understanding of the breadth and volume of
  related citations to inform requirements for OCEEBMS-568, would it
  be possible to generate an ad-hoc spreadsheet showing related
  citations for all those articles imported into the EBMS since Jan 1,
  2020? It would be helpful if it's possible for the spreadsheet to
  include the following information:

    * PMID of the citation in the EBMS
    * the journal abbreviation for the citation in the EBMS
    * the date of publication of the citation
    * the date the citation was imported into the EBMS
    * the PMID of the related citation
    * the type of related citation (if this is something PubMed provides?
      It would be something like editorial/comment, supplement, errata)
    * the journal abbreviation for the related citation
    * the date of publication of the related citation
"""

import argparse
import datetime
import getpass
import os
import sys
import pymysql
import openpyxl
from lxml import etree

PATH = "MedlineCitation/Article/PublicationTypeList/PublicationType"
COLS = (
    "PMID",
    "Journal",
    "Published",
    "Imported",
    "Related",
    "Type(s)",
    "Related Journal",
    "Related Publication",
)
parser = argparse.ArgumentParser()
parser.add_argument("--host", required=True)
parser.add_argument("--port", type=int, default=3661)
parser.add_argument("--db", default="oce_ebms")
parser.add_argument("--user", default="oce_ebms")
opts = vars(parser.parse_args())
opts["passwd"] = getpass.getpass("password for %s: " % opts["user"])
conn = pymysql.connect(**opts)
cursor = conn.cursor()
cursor.execute("SET NAMES utf8")
cursor.execute("""\
SELECT f.source_id,
       f.brf_jrnl_title,
       f.published_date,
       f.import_date,
       t.source_id,
       t.source_data,
       t.brf_jrnl_title,
       t.published_date,
       r.inactivated,
       rt.type_name
  FROM ebms_article f
  JOIN ebms_related_article r
    ON r.from_id = f.article_id
  JOIN ebms_article t
    ON t.article_id = r.to_id
  JOIN ebms_article_relation_type rt
    ON rt.type_id = r.type_id
 WHERE f.import_date >= '2020-01-01'""")
row = cursor.fetchone()
rows = []
while row:
    fid, fjnl, fpub, fimp, tid, xml, tjnl, tpub, inac, rtype = row
    root = etree.fromstring(xml.encode("utf-8"))
    types = []
    for node in root.findall(PATH):
        if node.text and node.text.strip():
            types.append(node.text.strip())
    types = "\n".join(types)
    fimp = str(fimp)[:10]
    rows.append((fid, fjnl, fpub, fimp, tid, types, tjnl, tpub, inac, rtype))
    row = cursor.fetchone()
book = openpyxl.Workbook()
sheet = book.active
sheet.title = "Related"
for c, name in enumerate(COLS):
    sheet.cell(row=1, column=c+1, value=name)
r = 2
for row in rows:
    for c, value in enumerate(row):
        sheet.cell(row=r, column=c+1, value=value)
    r += 1
book.save("oceebms-570.xlsx")
