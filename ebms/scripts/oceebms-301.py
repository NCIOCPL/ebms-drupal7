#!/usr/bin/python

#----------------------------------------------------------------------
# Report on journal article acceptance rates. Does a lot of in-memory
# processing, so it helps to run this on a machine with plenty of RAM.
# If this script is invoked with the name of a directory as the optional
# command-line argument, it will read the data for the report from
# the files in that directory instead of from the database (which takes
# longer).
#
# See Jira ticket OCEEBMS-301 for requirements.
#----------------------------------------------------------------------
import argparse
import getpass
import MySQLdb
import os
import xlwt
import sys
import datetime

class States:
    "Dictionary of article state values"
    def __init__(self, path):
        self.values = {}
        for line in open("%s/states" % path):
            state_id, state_text_id = eval(line.strip())
            self.values[state_text_id] = state_id
        self.ABSTRACT_YES = self.values["PassedBMReview"]
        self.ABSTRACT_NO = self.values["RejectBMReview"]
        self.FULL_TEXT_YES = self.values["PassedFullReview"]
        self.FULL_TEXT_NO = self.values["RejectFullReview"]
        self.FINAL_DECISION = self.values["FinalBoardDecision"]
    def wanted(self):
        "These are the states represented by the report"
        return (self.ABSTRACT_YES, self.ABSTRACT_NO,
                self.FULL_TEXT_YES, self.FULL_TEXT_NO,
                self.FINAL_DECISION)

class Counts:
    "Base class for counting occurrences of particular states"
    def __init__(self):
        self.abstract_yes = self.abstract_no = 0
        self.full_text_yes = self.full_text_no = 0
        self.ed_board_yes = self.ed_board_no = 0

class Article(Counts):
    """
    Records the number of times we see a particular state for
    this article in connection with a single board.
    """
    def __init__(self, article_id):
        Counts.__init__(self)
        self.id = article_id
    def map_counts(self, counts):
        """
        No matter how many times we see a particular state for
        this article/board combination, it gets mapped to a
        single count when folding the values into the counts
        for the aricle's journal.
        """
        counts.num_articles += 1
        if self.abstract_yes:
            counts.abstract_yes += 1
        elif self.abstract_no:
            counts.abstract_no += 1
        if self.full_text_yes:
            counts.full_text_yes += 1
        elif self.full_text_no:
            counts.full_text_no += 1
        if self.ed_board_yes:
            counts.ed_board_yes += 1
        elif self.ed_board_no:
            counts.ed_board_no += 1

class Journal(Counts):
    "Statistics for the articles in this journal considered by one board"
    def __init__(self):
        Counts.__init__(self)
        self.num_articles = 0
    def report(self, sheet, row, title):
        "Write the statistics to a single row in a spreadsheet"
        sheet.write(row, 0, title)
        sheet.write(row, 1, self.num_articles)
        sheet.write(row, 2, self.abstract_yes)
        sheet.write(row, 3, self.abstract_no)
        sheet.write(row, 4, self.full_text_yes)
        sheet.write(row, 5, self.full_text_no)
        sheet.write(row, 6, self.ed_board_yes)
        sheet.write(row, 7, self.ed_board_no)

class Board:
    "One of these for each of the six PDQ boards"
    def __init__(self, id, name):
        self.id = id
        self.name = name
        self.not_list = set()
        self.articles = {}
    def add_sheet(self, book, header_style):
        "Create a spreadsheet in one of the two report workbooks"
        if "Complementary" in self.name:
            name = "CAM"
        else:
            name = self.name
        sheet = book.add_sheet(name)
        sheet.col(0).width = 15000
        sheet.write(0, 0, "Journal Title", header_style)
        sheet.write(0, 1, "Total", header_style)
        sheet.write(0, 2, "Abstract Yes", header_style)
        sheet.write(0, 3, "Abstract No", header_style)
        sheet.write(0, 4, "Full-Text Yes", header_style)
        sheet.write(0, 5, "Full-Text No", header_style)
        sheet.write(0, 6, "Ed Board Yes", header_style)
        sheet.write(0, 7, "Ed Board No", header_style)
        return sheet
    def report(self, control):
        """
        Create two spreadsheets, one of the statistical counts on the
        journals on the list of "don't bother with articles in this
        journal when working on this board's queue" (the "not" list),
        and the other sheet for all the other journals.
        """
        empty = Journal()
        journals = {}
        for article_id in self.articles:
            journal_id = control.articles[article_id]
            if journal_id not in journals:
                journals[journal_id] = Journal()
            self.articles[article_id].map_counts(journals[journal_id])
        not_listed = self.add_sheet(control.not_listed, control.header_style)
        other = self.add_sheet(control.other, control.header_style)
        not_listed_row = other_row = 1
        for journal_id in control.journal_ids:
            title = control.journals[journal_id]
            journal = journals.get(journal_id, empty)
            if journal.num_articles:
                if journal_id in self.not_list:
                    row = not_listed_row
                    sheet = not_listed
                else:
                    row = other_row
                    sheet = other
                journal.report(sheet, row, title)
                if journal_id in self.not_list:
                    not_listed_row += 1
                else:
                    other_row += 1
        sys.stderr.write("board %s reported\n" % self.name)
    def __cmp__(self, other):
        "Make the boards sortable by board name"
        return cmp(self.name, other.name)

class Control:
    "Loads up all of the data for the report"
    def __init__(self, directory):
        self.directory = directory
        self.states = States(directory)
        self.boards = {}
        self.journals = {}
        self.articles = {}
        self.decision_values = {}
        self.board_decisions = {}
        for line in open("%s/boards" % directory):
            board_id, name = eval(line.strip())
            self.boards[board_id] = Board(board_id, name)
        sys.stderr.write("loaded %d boards\n" % len(self.boards))
        count = 0
        for line in open("%s/not_list" % directory):
            journal_id, board_id = eval(line.strip())
            self.boards[board_id].not_list.add(journal_id)
            count += 1
        sys.stderr.write("loaded %d not-list directives\n" % count)
        for line in open("%s/journals" % directory):
            journal_id, journal_title = eval(line.strip())
            self.journals[journal_id] = journal_title
        sys.stderr.write("loaded %d journals\n" % len(self.journals))
        for line in open("%s/articles" % directory):
            article_id, journal_id = eval(line.strip())
            self.articles[article_id] = journal_id
        sys.stderr.write("loaded %d articles\n" % len(self.articles))
        count = 0
        for line in open("%s/article_boards" % directory):
            article_id, board_id = eval(line.strip())
            self.boards[board_id].articles[article_id] = Article(article_id)
            count += 1
        sys.stderr.write("loaded %d article/board combos\n" % count)
        for line in open("%s/decision_values" % directory):
            value_id, value_name = eval(line.strip())
            self.decision_values[value_id] = value_name
        sys.stderr.write("loaded %d decision values\n" %
                         len(self.decision_values))
        count = 0
        for line in open("%s/board_decisions" % directory):
            article_state_id, decision_value_id = eval(line.strip())
            decision_value = self.decision_values.get(decision_value_id)
            if article_state_id not in self.board_decisions:
                self.board_decisions[article_state_id] = set()
            self.board_decisions[article_state_id].add(decision_value)
            count += 1
        sys.stderr.write("loaded %d board decisions\n" % count)
        count = 0
        for line in open("%s/article_states" % directory):
            art_state_id, article_id, state_id, board_id = eval(line.strip())
            article = self.boards[board_id].articles[article_id]
            if state_id == self.states.ABSTRACT_NO:
                article.abstract_no += 1
            elif state_id == self.states.ABSTRACT_YES:
                article.abstract_yes += 1
            elif state_id == self.states.FULL_TEXT_NO:
                article.full_text_no += 1
            elif state_id == self.states.FULL_TEXT_YES:
                article.full_text_yes += 1
            elif state_id == self.states.FINAL_DECISION:
                board_decisions = self.board_decisions.get(art_state_id)
                if board_decisions:
                    if "Not cited" in board_decisions:
                        article.ed_board_no += 1
                    else:
                        article.ed_board_yes += 1
            count += 1
            sys.stderr.write("\rloaded %d article states" % count)
        sys.stderr.write("\n")
        self.journal_ids = self.journals.keys()
        self.journal_ids.sort(lambda a,b: cmp(self.journals[a],
                                              self.journals[b]))
        sys.stderr.write("data loaded\n")
    def report(self):
        "Generate two workbooks for the report (see Board.report())"
        self.not_listed = xlwt.Workbook(encoding="UTF-8")
        self.other = xlwt.Workbook(encoding="UTF-8")
        style = "font: bold True; align: wrap True, vert centre, horiz centre"
        self.header_style = xlwt.easyxf(style)
        for board in sorted(self.boards.values()):
            board.report(self)
        fp = open("%s/not_listed.xls" % self.directory, "wb")
        self.not_listed.save(fp)
        fp.close()
        fp = open("%s/not_not_listed.xls" % self.directory, "wb")
        self.other.save(fp)
        fp.close()

def fetch(opts):
    """
    Collect the data from the database and store it to the file system.
    We do it this way so we can tweak the layout of the report by
    reading the stored data into the modified code for the Control
    object, without having to spend time talking to the database all
    over again (which is the lengthier part of the job by quite a bit).
    """
    where = str(datetime.date.today()).replace("-", "")
    try:
        os.mkdir(where)
    except Exception, e:
        print "%s: %s" % (where, e)
    opts = vars(opts)
    opts["passwd"] = getpass.getpass("password for %s: " % opts["user"])
    conn = MySQLdb.connect(**opts)
    cursor = conn.cursor()
    cursor.execute("SET NAMES utf8")
    cursor.execute("USE %s" % opts["db"])
    cursor.execute("SELECT board_id, board_name FROM ebms_board")
    fp = open("%s/boards" % where, "w")
    rows = cursor.fetchall()
    for row in rows:
        fp.write("%s\n" % repr(row))
    fp.close()
    sys.stderr.write("fetched %d boards\n" % len(rows))
    cursor.execute("""\
SELECT source_jrnl_id, board_id
  FROM ebms_not_list
 WHERE start_date <= NOW()""")
    rows = cursor.fetchall()
    fp = open("%s/not_list" % where, "w")
    for row in rows:
        fp.write("%s\n" % repr(row))
    fp.close()
    sys.stderr.write("fetched %d not-list rows\n" % len(rows))
    cursor.execute("SELECT source_jrnl_id, jrnl_title from ebms_journal")
    rows = cursor.fetchall()
    fp = open("%s/journals" % where, "w")
    for row in rows:
        fp.write("%s\n" % repr(row))
    fp.close()
    sys.stderr.write("fetched %d journals\n" % len(rows))
    cursor.execute("SELECT article_id, source_jrnl_id FROM ebms_article")
    fp = open("%s/articles" % where, "w")
    count = 0
    row = cursor.fetchone()
    while row:
        fp.write("%s\n" % repr(row))
        row = cursor.fetchone()
        count += 1
    fp.close()
    sys.stderr.write("fetched %d articles\n" % count)
    cursor.execute("""\
SELECT state_id, state_text_id
  FROM ebms_article_state_type""")
    fp = open("%s/states" % where, "w")
    rows = cursor.fetchall()
    for row in rows:
        fp.write("%s\n" % repr(row))
    fp.close()
    sys.stderr.write("fetched %d states\n" % len(rows))
    states = States(where)
    cursor.execute("""\
SELECT DISTINCT article_id, board_id
           FROM ebms_article_state
          WHERE active_status = 'A'""")
    fp = open("%s/article_boards" % where, "w")
    count = 0
    row = cursor.fetchone()
    while row:
        fp.write("%s\n" % repr(row))
        row = cursor.fetchone()
        count += 1
    fp.close()
    sys.stderr.write("fetched %d article boards\n" % count)
    cursor.execute("""\
SELECT article_state_id, article_id, state_id, board_id
  FROM ebms_article_state
 WHERE active_status = 'A'
   AND state_id IN (%s)""" % ",".join([str(w) for w in states.wanted()]))
    fp = open("%s/article_states" % where, "w")
    row = cursor.fetchone()
    count = 0
    while row:
        fp.write("%s\n" % repr(row))
        row = cursor.fetchone()
        count += 1
    fp.close()
    sys.stderr.write("fetched %d article states\n" % count)
    cursor.execute("""\
SELECT value_id, value_name
  FROM ebms_article_board_decision_value""")
    rows = cursor.fetchall()
    fp = open("%s/decision_values" % where, "w")
    for row in rows:
        fp.write("%s\n" % repr(row))
    fp.close()
    sys.stderr.write("fetched %d decision values\n" % len(rows))
    cursor.execute("""\
SELECT article_state_id, decision_value_id
  FROM ebms_article_board_decision""")
    row = cursor.fetchone()
    count = 0
    fp = open("%s/board_decisions" % where, "w")
    while row:
        fp.write("%s\n" % repr(row))
        row = cursor.fetchone()
        count += 1
    fp.close()
    sys.stderr.write("fetched %d board decisions\n" % count)
    return where

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--host", required=True)
    parser.add_argument("--port", type=int, default=3661)
    parser.add_argument("--db", default="oce_ebms")
    parser.add_argument("--user", default="oce_ebms")
    parser.add_argument("--path")
    opts = parser.parse_args()
    if opts.path:
        path = opts.path
    else:
        path = fetch(opts)
    control = Control(path)
    control.report()
if __name__ == "__main__":
    main()
