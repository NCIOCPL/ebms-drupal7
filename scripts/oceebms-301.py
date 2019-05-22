#!/usr/bin/env python3

"""
Report on journal article acceptance rates.

Does a lot of in-memory processing, so it helps to run this on a
machine with plenty of RAM.  If this script is invoked with the
`--path` option, it will read the data for the report from the files
in that directory captured on a previous run without that option.

See Jira ticket OCEEBMS-301 for requirements.
"""

import argparse
import datetime
import getpass
import os
import sys
import pymysql
import xlwt


class States:
    "Dictionary of article state values"

    def __init__(self, path):
        self.values = dict()
        for line in open(f"{path}/states"):
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
        for the article's journal.
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
        self.articles = dict()

    def add_sheet(self, book, header_style):
        "Create a spreadsheet in one of the two report workbooks"

        if "Complementary" in self.name:
            name = "IACT"
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
        journals = dict()
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
        sys.stderr.write(f"board {self.name} reported\n")


class Control:
    "Loads up all of the data for the report"

    def __init__(self, directory):
        self.directory = directory
        self.states = States(directory)
        self.boards = dict()
        self.journals = dict()
        self.articles = dict()
        self.decision_values = dict()
        self.board_decisions = dict()
        for line in open(f"{directory}/boards"):
            board_id, name = eval(line.strip())
            self.boards[board_id] = Board(board_id, name)
        sys.stderr.write(f"loaded {len(self.boards):d} boards\n")
        count = 0
        for line in open(f"{directory}/not_list"):
            journal_id, board_id = eval(line.strip())
            self.boards[board_id].not_list.add(journal_id)
            count += 1
        sys.stderr.write(f"loaded {count:d} not-list directives\n")
        for line in open(f"{directory}/journals"):
            journal_id, journal_title = eval(line.strip())
            self.journals[journal_id] = journal_title
        sys.stderr.write(f"loaded {len(self.journals):d} journals\n")
        for line in open(f"{directory}/articles"):
            article_id, journal_id = eval(line.strip())
            self.articles[article_id] = journal_id
        sys.stderr.write(f"floaded {len(self.articles):d} articles\n")
        count = 0
        for line in open(f"{directory}/article_boards"):
            article_id, board_id = eval(line.strip())
            self.boards[board_id].articles[article_id] = Article(article_id)
            count += 1
        sys.stderr.write(f"loaded {count:d} article/board combos\n")
        for line in open(f"{directory}/decision_values"):
            value_id, value_name = eval(line.strip())
            self.decision_values[value_id] = value_name
        msg = f"loaded {len(self.decision_values):d} decision values\n"
        sys.stderr.write(msg)
        count = 0
        for line in open(f"{directory}/board_decisions"):
            article_state_id, decision_value_id = eval(line.strip())
            decision_value = self.decision_values.get(decision_value_id)
            if article_state_id not in self.board_decisions:
                self.board_decisions[article_state_id] = set()
            self.board_decisions[article_state_id].add(decision_value)
            count += 1
        sys.stderr.write(f"loaded {count:d} board decisions\n")
        count = 0
        for line in open(f"{directory}/article_states"):
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
            sys.stderr.write(f"\rloaded {count:d} article states")
        sys.stderr.write("\n")
        self.journal_ids = list(self.journals.keys())
        self.journal_ids.sort(key=lambda k: self.journals[k])
        sys.stderr.write("data loaded\n")

    def report(self):
        "Generate two workbooks for the report (see Board.report())"

        self.not_listed = xlwt.Workbook(encoding="UTF-8")
        self.other = xlwt.Workbook(encoding="UTF-8")
        style = "font: bold True; align: wrap True, vert centre, horiz centre"
        self.header_style = xlwt.easyxf(style)
        for board in sorted(self.boards.values(), key=lambda b: b.name):
            board.report(self)
        with open(f"{self.directory}/not_listed.xls", "wb") as fp:
            self.not_listed.save(fp)
        with open(f"{self.directory}/not_not_listed.xls", "wb") as fp:
            self.other.save(fp)


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
    except Exception as e:
        print(f"{where}: {e}")
    opts["passwd"] = getpass.getpass(f"password for {opts['user']}: ")
    conn = pymysql.connect(**opts)
    cursor = conn.cursor()
    cursor.execute("SET NAMES utf8")
    cursor.execute(f"USE {opts['db']}")
    cursor.execute("SELECT board_id, board_name FROM ebms_board")
    with open(f"{where}/boards", "w") as fp:
        rows = cursor.fetchall()
        for row in rows:
            fp.write(f"{row!r}\n")
    sys.stderr.write(f"fetched {len(rows):d} boards\n")
    cursor.execute("""\
SELECT source_jrnl_id, board_id
  FROM ebms_not_list
 WHERE start_date <= NOW()""")
    rows = cursor.fetchall()
    with open(f"{where}/not_list", "w") as fp:
        for row in rows:
            fp.write(f"{row!r}\n")
    sys.stderr.write(f"fetched {len(rows):d} not-list rows\n")
    cursor.execute("SELECT source_jrnl_id, jrnl_title from ebms_journal")
    rows = cursor.fetchall()
    with open(f"{where}/journals", "w") as fp:
        for row in rows:
            fp.write(f"{row!r}\n")
    sys.stderr.write(f"fetched {len(rows):d} journals\n")
    cursor.execute("SELECT article_id, source_jrnl_id FROM ebms_article")
    with open(f"{where}/articles", "w") as fp:
        count = 0
        row = cursor.fetchone()
        while row:
            fp.write(f"{row!r}\n")
            row = cursor.fetchone()
            count += 1
    sys.stderr.write(f"fetched {count:d} articles\n")
    cursor.execute("""\
SELECT state_id, state_text_id
  FROM ebms_article_state_type""")
    with open(f"{where}/states", "w") as fp:
        rows = cursor.fetchall()
        for row in rows:
            fp.write(f"{row!r}\n")
    sys.stderr.write(f"fetched {len(rows):d} states\n")
    states = States(where)
    cursor.execute("""\
SELECT DISTINCT article_id, board_id
           FROM ebms_article_state
          WHERE active_status = 'A'""")
    with open(f"{where}/article_boards", "w") as fp:
        count = 0
        row = cursor.fetchone()
        while row:
            fp.write(f"{row!r}\n")
            row = cursor.fetchone()
            count += 1
    sys.stderr.write(f"fetched {count:d} article boards\n")
    wanted = ",".join([str(w) for w in states.wanted()])
    cursor.execute(f"""\
SELECT article_state_id, article_id, state_id, board_id
  FROM ebms_article_state
 WHERE active_status = 'A'
   AND state_id IN ({wanted})""")
    with open(f"{where}/article_states", "w") as fp:
        row = cursor.fetchone()
        count = 0
        while row:
            fp.write(f"{row!r}\n")
            row = cursor.fetchone()
            count += 1
    sys.stderr.write(f"fetched {count:d} article states\n")
    cursor.execute("""\
SELECT value_id, value_name
  FROM ebms_article_board_decision_value""")
    rows = cursor.fetchall()
    with open(f"{where}/decision_values", "w") as fp:
        for row in rows:
            fp.write(f"{row!r}\n")
    sys.stderr.write(f"fetched {len(rows):d} decision values\n")
    cursor.execute("""\
SELECT article_state_id, decision_value_id
  FROM ebms_article_board_decision""")
    row = cursor.fetchone()
    count = 0
    with open(f"{where}/board_decisions", "w") as fp:
        while row:
            fp.write(f"{row!r}\n")
            row = cursor.fetchone()
            count += 1
    sys.stderr.write(f"fetched {count:d} board decisions\n")
    return where

def main():
    parser = argparse.ArgumentParser()
    group = parser.add_mutually_exclusive_group(required=True)
    group.add_argument("--host")
    group.add_argument("--path")
    parser.add_argument("--port", type=int, default=3661)
    parser.add_argument("--db", default="oce_ebms")
    parser.add_argument("--user", default="oce_ebms")
    opts = parser.parse_args()
    if opts.path:
        path = opts.path
    else:
        opts = vars(opts)
        del opts["path"]
        path = fetch(opts)
    control = Control(path)
    control.report()

if __name__ == "__main__":
    main()
