#!/usr/bin/python

#----------------------------------------------------------------------
# $Id$
#

# We would like to determine the primary reasons Board members provide
# for excluding articles from PDQ. With this information, we may pare
# down the list of exclusion reasons currently offered in the
# EBMS. For this report (which I'm guessing could be generated ad-hoc
# - please let me know if that isn't the case), we would like to see
# the following in a single Excel spreadsheet.
#
# Column 1: Exclusion Reason (please include all reasons, even if the total=0)
#
# Column 2: Total (referring to the total number of times a Board member
#            provided that reason across all Boards)
# Columns 3-8: Total for each Board (referring to the total number of times
#              a Board member reviewing an article for that Board provided
#              that reason)
#
#----------------------------------------------------------------------
import MySQLdb
import os
import xlwt
import sys
import datetime

"""
ebms_review_rejection_reason review_id, value_id
ebms_article_review review_id, packet_id
ebms_packet packet_id, topic_id
ebms_topic topic_id, board_id
ebms_board board_id, board_name
"""
class Reason:
    def __init__(self, name):
        self.name = name
        self.counts = {}
        self.total = 0
    def add_count(self, count, board):
        self.counts[board] = self.counts.get(board, 0) + count
        self.total += count
    def report(self, sheet, row, board_ids):
        "Write the statistics to a single row in a spreadsheet"
        sheet.write(row, 0, self.name)
        sheet.write(row, 1, self.total)
        col = 2
        for board_id in board_ids:
            count = self.counts.get(board_id)
            if count:
                sheet.write(row, col, count)
            col += 1
    def __cmp__(self, other):
        return cmp(self.name, other.name)

class Control:
    "Loads up all of the data for the report"
    def __init__(self):
        host = "cbdb-p2001.nci.nih.gov"
        port = 3661
        db = "oce_ebms"
        pw = "***REMOVED***"
        user = "read_ebms"
        conn = MySQLdb.connect(user=user, passwd=pw, db=db, host=host,
                               port=port)
        cursor = conn.cursor()
        cursor.execute("SET NAMES utf8")
        cursor.execute("USE %s" % db)
        cursor.execute("""\
SELECT value_id, value_name
  FROM ebms_review_rejection_value""")
        self.reasons = {}
        for value_id, value_name in cursor.fetchall():
            self.reasons[value_id] = Reason(value_name)
        cursor.execute("SELECT board_id, board_name FROM ebms_board")
        self.boards = {}
        for board_id, board_name in cursor.fetchall():
            self.boards[board_id] = board_name
        self.board_ids = self.boards.keys()
        self.board_ids.sort(lambda a,b: cmp(self.boards[a], self.boards[b]))
        cursor.execute("""\
  SELECT COUNT(*), r.value_id, t.board_id
    FROM ebms_review_rejection_reason r
    JOIN ebms_article_review v
      ON v.review_id = r.review_id
    JOIN ebms_packet p
      ON p.packet_id = v.packet_id
    JOIN ebms_topic t
      ON t.topic_id = p.topic_id
GROUP BY r.value_id, t.board_id""")
        for count, reason, board in cursor.fetchall():
            self.reasons[reason].add_count(count, board)
    def report(self):
        "Create and populated Excel spreadsheet for the report."
        book = xlwt.Workbook(encoding="UTF-8")
        sheet = book.add_sheet("Reasons")
        style = "font: bold True; align: wrap True, vert centre, horiz centre"
        header_style = xlwt.easyxf(style)
        sheet.col(0).width = 5000
        sheet.write(0, 0, "Reason", header_style)
        sheet.write(0, 1, "Total", header_style)
        col = 2
        for board_id in self.board_ids:
            sheet.write(0, col, self.boards[board_id], header_style)
            col += 1
        row = 1
        for reason in sorted(self.reasons.values()):
            reason.report(sheet, row, self.board_ids)
            row += 1
        fp = open("oceebms-316.xls", "wb")
        book.save(fp)
        fp.close()

def main():
    control = Control()
    control.report()
if __name__ == "__main__":
    main()
