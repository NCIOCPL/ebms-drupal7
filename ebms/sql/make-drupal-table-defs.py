#!/usr/bin/python

#----------------------------------------------------------------------
#
# $Id$
#
# Transforms SQL DDL into structures usable by Drupal's schema API.
#
#----------------------------------------------------------------------
import re

#----------------------------------------------------------------------
# Extracts the name of the referenced table, the names of the referencing
# columns, and the names of the referenced columns.  The arrays for
# the referenced and referencing columns will be the same length.
#----------------------------------------------------------------------
class ForeignKey:
    count = 1
    pattern = (r"FOREIGN KEY\s+\(([^)]+)\)\s+"
               r"REFERENCES\s+(\S+)\s*\(([^)]+)\)")
    def __init__(self, line):
        pieces = re.match(ForeignKey.pattern, line)
        try:
            self.cols = [col.strip() for col in pieces.group(1).split("|")]
            self.target = pieces.group(2)
            self.tcols = [col.strip() for col in pieces.group(1).split("|")]
        except Exception, e:
            print "%s %s" % (repr(line), e)
    def makeDrupalDef(self):
        dd = ["            'ebms_fk_%04d' => array(" % ForeignKey.count]
        ForeignKey.count += 1
        dd.append("                'table' => '%s'," % self.target)
        dd.append("                'columns' => array(")
        for i, c in enumerate(self.cols):
            dd.append("                    '%s' => '%s'," %
                      (c, self.tcols[i]))
        dd.append("                ),")
        dd.append("            ),")
        return "\n".join(dd)

#----------------------------------------------------------------------
# Extracts the names of the columns which make up the primary key
# for the table.
#----------------------------------------------------------------------
class PrimaryKey:
    def __init__(self, line):
        pieces = re.match(r"PRIMARY KEY\s+\(([^)]+)\)", line)
        self.cols = [col.strip() for col in pieces.group(1).split("|")]

#----------------------------------------------------------------------
# Extracts the names of the columns that go into a unique index,
# as well as the name of that index.
#----------------------------------------------------------------------
class UniqueKey:
    def __init__(self, line):
        pieces = re.match(r"UNIQUE KEY\s+(\S+)\s\(([^)]+)\)", line)
        self.name = pieces.group(1)
        self.cols = [col.strip() for col in pieces.group(2).split("|")]

#----------------------------------------------------------------------
# Extracts the definition for a single table column.
#----------------------------------------------------------------------
class Column:
    def __init__(self, line):
        words = line.split()
        self.name = words[0]
        self.type = self.unsigned = self.length = self.nullable = None
        self.unique = self.size = self.default = self.enum = None
        self.serial = self.primary = self.ascii = self.mysqlType = False
        if 'NOT' in words:
            self.nullable = False
            words.remove('NOT')
            words.remove('NULL')
        elif 'NULL' in words:
            self.nullable = True
            words.remove('NULL')
        if 'UNIQUE' in words:
            self.unique = True
            words.remove('UNIQUE')
        if 'PRIMARY' in words:
            self.primary = True
            words.remove('PRIMARY')
            words.remove('KEY')
        if 'AUTO_INCREMENT' in words:
            self.serial = True
            words.remove('AUTO_INCREMENT')
        if 'ASCII' in words and 'CHARACTER' in words and 'SET' in words:
            self.ascii = True
            for word in ('CHARACTER', 'SET', 'ASCII'):
                words.remove(word)
        if 'UNSIGNED' in words:
            self.unsigned = True
            words.remove('UNSIGNED')
        if 'DEFAULT' in words:
            self.default = words.pop()
            words.remove('DEFAULT')
            if "'" in self.default:
                self.default = self.default.replace("'", '')
            else:
                self.default = int(self.default)
        if words[1].startswith('ENUM'):
                match = re.search(r"ENUM\s*(\([^)]+\))", line)
                self.mysqlType = True
                values = eval(match.group(1).replace("|", ","))
                self.type = "ENUM %s" % repr(values)
                for i, word in enumerate(words):
                    if word.endswith(")"):
                        if i < len(words) - 1:
                            raise Exception("EXTRA ENUM ATTRIBUTES: %s" % line)
                #print "%s %s" % (self.name, self.type)
        elif len(words) == 2:
            if words[1] in ('INT', 'INTEGER'):
                self.type = 'int'
            elif words[1] == 'DATETIME':
                self.type = 'datetime'
                self.mysqlType = True
            elif words[1] == 'DATE':
                self.type = 'date'
                self.mysqlType = True
            elif words[1] == 'TEXT':
                self.type = 'text'
            elif words[1] == 'LONGTEXT':
                self.type = 'text'
                self.size = 'big'
            elif words[1].startswith('VARCHAR'):
                match = re.match(r"VARCHAR\((\d+)\)", words[1])
                self.type = 'varchar'
                self.length = int(match.group(1))
            else:
                raise Exception(line)
        else:
            raise Exception(line)

    #------------------------------------------------------------------
    # Assembles the Drupal schema API definition for this column.
    #------------------------------------------------------------------
    def makeDrupalDef(self, colComments):
        dd = ["            '%s' => array(" % self.name]
        if self.mysqlType:
            if self.type in ('datetime', 'date'):
                dd.append("                'type' => 'varchar',")
            elif self.type.startswith("ENUM"):
                dd.append("                'type' => 'char',")
            dd.append("                'mysql_type' => '%s'," % self.type)
        else:
            dd.append("                'type' => '%s'," % self.type)
        if self.nullable is not None:
            dd.append("                'not null' => %s," %
                      (self.nullable and 'false' or 'true'))
        if self.default:
            if type(self.default) == int:
                default = self.default
            else:
                default = "'%s'" % self.default
            dd.append("                'default' => %s," % default)
        desc = colComments[self.name].value.replace("'", "\\'")
        dd.append("                'description' => '%s'," % desc)
        dd.append("            ),")
        return "\n".join(dd)

#----------------------------------------------------------------------
# Collects the string documenting a table column, extracted from the
# comment at the head of the SQL table definition.
#----------------------------------------------------------------------
class ColumnComment:
    def __init__(self, name, start):
        self.name = name
        self.value = start

#----------------------------------------------------------------------
# Extracts the table-level documentation as separate paragraphs, as
# well as each of the description string for the table's columns.
# The exceptions raised here were used during development, tweaking
# the parser until all had been eliminated.
#----------------------------------------------------------------------
class Comment:
    def __init__(self, c):
        c = c.replace("{", "*").replace("}", "*")
        lines = [line.strip() for line in c.splitlines()]
        if lines.pop() != '*':
            raise Exception("ODD COMMENT END: %s" % repr(c))
        if lines.pop(0) != '*':
            raise Exception("ODD COMMENT START: %s" % repr(c))
        lastLineBreak = -1
        for i, line in enumerate(lines):
            if line == "*":
                lastLineBreak = i
        if lastLineBreak < 1 or lastLineBreak == len(lines) - 1:
            raise Exception("ODD POSITION OF LAST LINE BREAK: %s" % repr(c))
        para = []
        self.paras = []
        descLines = lines[:lastLineBreak]
        for line in descLines:
            if line == "*":
                if para:
                    self.paras.append(" ".join(para))
                    para = []
            else:
                para.append(line[1:].strip())
        if para:
            self.paras.append(" ".join(para))
        colCommentLines = lines[lastLineBreak+1:]
        self.colComments = {}
        currentComment = None
        self.colCommentIndex = {}
        for line in colCommentLines:
            if not line.startswith("*"):
                raise Exception("FUNKY LINE START: %s" % repr(c))
            line = line[1:]
            if line[:8].isspace():
                if not currentComment:
                    raise Exception("ORPHAN CONTINUATION: %s" % repr(c))
                currentComment.value += (" %s" % line.strip())
            else:
                words = line.split()
                currentComment = ColumnComment(words[0], " ".join(words[1:]))
                self.colComments[currentComment.name] = currentComment

#----------------------------------------------------------------------
# Extract the column definitions, primary key definitions, foreign
# key definitions, and unique key definitions from the original SQL.
#----------------------------------------------------------------------
class Table:
    def __init__(self, pieces):
        try:
            self.comment = Comment(pieces[0])
        except Exception, e:
            print e
            self.comment = None
        self.name = pieces[1]
        self.body = pieces[2]
        self.lines = [line.strip() for line in self.body.split(",")]
        self.columns = []
        self.foreignKeys = []
        self.uniqueKeys = []
        self.primaryKey = None
        for line in self.lines:
            if "FOREIGN KEY" in line:
                self.foreignKeys.append(ForeignKey(line))
            elif line.startswith("PRIMARY"):
                self.primaryKey = PrimaryKey(line)
            elif line.startswith("UNIQUE"):
                self.uniqueKeys.append(UniqueKey(line))
            else:
                try:
                    col = Column(line)
                    self.columns.append(col)
                    if self.comment:
                        if col.name not in self.comment.colComments:
                            print "%s NOT DOCUMENTED" % col.name
                except Exception, e:
                    print "%s: %s" % (self.name, e)

    #------------------------------------------------------------------
    # Assembles the Drupal schema API definition for this table.
    #------------------------------------------------------------------
    def makeDrupalDef(self):
        pk = None
        desc = "\n".join(self.comment.paras).replace("'", "\\'")
        if self.primaryKey:
            pk = tuple(self.primaryKey.cols)
        else:
            for col in self.columns:
                if col.primary:
                    pk = (col.name,)
        dd = ["    $schema['%s'] = array(" % self.name]
        dd.append("        'description' => '%s'," % desc)
        dd.append("        'fields' => array(")
        for col in self.columns:
            dd.append(col.makeDrupalDef(self.comment.colComments))
        dd.append("        ),")
        if self.foreignKeys:
            dd.append("        'foreign keys' => array(")
            for fk in self.foreignKeys:
                dd.append(fk.makeDrupalDef())
            dd.append("        ),")
        if pk:
            dd.append("        'primary key' => array%s," % repr(pk))
        dd.append("    );")
        return "\n".join(dd)

#----------------------------------------------------------------------
# Replace commas with an unused character so we can break column
# definitions on commas.
#----------------------------------------------------------------------
def escapeEnum(match):
    return "ENUM (%s)" % match.group(1).replace(",", "|")

#----------------------------------------------------------------------
# Replace commas with an unused character so we can break column
# definitions on commas.
#----------------------------------------------------------------------
def escapeKey(match):
    return "%s (%s)" % (match.group(1), match.group(2).replace(",", "|"))

#----------------------------------------------------------------------
# Replace commas with an unused character so we can break column
# definitions on commas.
#----------------------------------------------------------------------
def escapeForeignKey(match):
    return "%s (%s) %s (%s)" % (match.group(1),
                                match.group(2).replace(",", "|"),
                                match.group(3),
                                match.group(4).replace(",", "|"))

#----------------------------------------------------------------------
# Entry point.
#----------------------------------------------------------------------
def main():

    # Load the original SQL script.
    script = open("ebms.sql").read()

    # Munges to make regular expression handling work.
    script = script.replace("*/", "}").replace("/*", "{")
    script = re.sub(r"ENUM\s*\(([^)]+)\)", escapeEnum, script)
    script = re.sub(r"(UNIQUE\s+KEY\s+\S+)\s*\(([^)]+)\)", escapeKey, script)
    script = re.sub(r"(PRIMARY\s+KEY)\s*\(([^)]+)\)", escapeKey, script)
    script = re.sub(r"(FOREIGN\s+KEY)\s*\(([^)]+)\)\s*"
                    r"(REFERENCES\s+\S+)\s*\(([^)]+)\)", escapeForeignKey,
                    script)

    # Create a regular expression pattern to recognize each table definition.
    pattern = (r"({[^}]+})\s+CREATE TABLE\s+(\S+)\s+\((.*?)\)\s+"
               r"ENGINE.*?InnoDB;")
    pattern = re.compile(pattern, re.DOTALL)

    # Create the top of the Drupal .install file
    print """\
<?php

/**
 * Implements hook_schema().
 */
function ebms_schema() {"""

    # Walk through the tables, creating drupal schema structures for each.
    for pieces in re.findall(pattern, script):
        table = Table(pieces)
        print table.makeDrupalDef()

    # Finish off the file.
    print """\
    return $schema;
}"""

if __name__ == "__main__":
    main()
