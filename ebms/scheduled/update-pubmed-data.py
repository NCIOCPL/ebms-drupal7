#!/usr/bin/python

#----------------------------------------------------------------------
#
# Find out which Pubmed articles have been modified since we last
# pulled down the XML from NLM, and refresh those documents.
#
# JIRA::OCEEBMS-87
#
#----------------------------------------------------------------------
import urllib2, sys, glob, datetime, time, re, smtplib
import logging
import os

#----------------------------------------------------------------------
# Use drush to get the list of recipients, so we don't have to store
# developers' email addresses in code.
#----------------------------------------------------------------------
def get_recips():
    saved_dir = os.getcwd()
    os.chdir("/local/drupal/sites/ebms.nci.nih.gov")
    stream = os.popen("drush vget dev_notif_addr --exact")
    output = stream.read().strip()
    if output and output[0] in "'\"":
        output = output[1:-1]
    stream.close()
    os.chdir(saved_dir)
    return [address.strip() for address in output.split(",")]

#----------------------------------------------------------------------
# Ask the EBMS web application to give us a list of all of the
# Pubmed articles in the ebms_article table.  Each article is
# represented in the web application's response by a line containing
# the articles Pubmed ID, the date of the last time we fetched
# the XML for the article from NLM, and the date previously given
# by NLM for the most recent modification of the article (if any).
# The three values are separated by the tab character.  A dictionary
# is constructed and returned, indexed by the Pubmed IDs, with the
# other two values as the data for each node in the dictionary.
#
#  @param  host       DNS name of EBMS web server (e.g., ebms.nci.nih.gov)
#
#  @return            tuple containing the dictionary for the articles,
#                     and the latest date found in the data_mod column
#                     or the first day of 2012, if all the values in
#                     that column are null; our first retrieval of
#                     XML from NLM for the EBMS was in January of 2012,
#                     so 2012-01-01 will catch any modifications we
#                     need.
#----------------------------------------------------------------------
def get_articles(host):
    url = "https://%s/get-source-ids/Pubmed" % host
    logging.debug("url: %s", url)
    tries = 10
    delay = 10
    while True:
        try:
            f = urllib2.urlopen(url)
            break
        except Exception as e:
            logging.error("%s: %s", url, e)
            tries -= 1
            if tries < 1:
                logging.error("giving up after too many successful attempts")
                raise
            logging.info("%d tries left; waiting %d seconds", tries, delay)
            time.sleep(delay)
            delay += 10
    logging.debug("response received from %s", host)
    pmids = {}
    latest_mod = "2011-01-01"
    if f.code == 200:
        for line in f.readlines():
            fields = line.strip().split("\t")
            mod = None
            if len(fields) == 3:
                mod = fields[2]
                if mod and mod > latest_mod:
                    latest_mod = mod
            elif len(fields) != 2:
                print repr(line), repr(fields)
                continue
            pmids[fields[0]] = (fields[1], mod)
    else:
        raise Exception("Failure fetching article information: HTTP code %s" %
                        f.code)
    logging.info("received %d PMIDs; latest_mod=%s", len(pmids), latest_mod)
    return pmids, latest_mod

#----------------------------------------------------------------------
# Ask NLM to tell us which Pubmed articles were modified on a specific
# day.  For each Pubmed ID we get back, if the corresponding article
# is in our database, and if the data_mod column for that article
# does not already contain the modification date NLM has for the
# article, add the Pubmed ID to the list to be returned to the caller.
# The return document from NLM can be larger than we could parse in
# memory, so we use a regular expression to tease out the Pubmed IDs.
# We're relying on the fact that NLM returns each ID on a separate line.
#
#  @param  date             date we're asking NLM about
#  @param  articles         information about articles in the EBMS DB
#
#  @return                  list of Pubmed IDs we need to record as
#                           having been modified on the date specified
#----------------------------------------------------------------------
def get_mod_pmids(date, articles):
    base = "https://eutils.ncbi.nlm.nih.gov/entrez/eutils/esearch.fcgi"
    mod = []
    d = str(date).replace('-', '/')
    parms = "db=pubmed&retmax=50000000&term=%s[MDAT]" % d
    url = "%s?%s" % (base, parms)
    logging.debug("opening %s", url)
    f = urllib2.urlopen(url)
    logging.debug("response received from NLM")
    while True:
        line = f.readline()
        if not line:
            break
        if "<ERROR>" in line:
            match = re.search("<ERROR>(.*)</ERROR>", line)
            if match:
                raise Exception("Failure fetching MDAT info: %s" %
                                match.group(1))
            else:
                raise Exception("Failure fetching MDAT information")
        match = re.search("<Id>(\\d+)</Id>", line)
        if match:
            pmid = match.group(1)
            if pmid in articles:
                latest, data_mod = articles[pmid]
                if data_mod != date:
                    mod.append(pmid)
    logging.info("%d modified articles found", len(mod))
    return mod

#----------------------------------------------------------------------
# Find out from NLM when the XML documents for articles in our
# database have been updated.  Start the last date found in the
# ebms_article table's data_mod column and work forward until we
# get to one week earlier than today.  For each day, ask Pubmed
# for the list of articles whose XML changed on that day, and
# send that list to the EBMS Drupal web application for storage.
# There's a limit on how much data can be sent to that application
# at once, so if we have more than BATCH_SIZE Pubmed IDs, we
# send them in batches of that size.
#
#  @param  host       DNS name of EBMS web server (e.g., ebms.nci.nih.gov)
#  @param  articles   information on the articles in the EBMS
#  @param  latest_mod latest value found in the data_mod column
#
#  @return            string summarizing processing activity
#----------------------------------------------------------------------
def update_mod_dates(host, articles, latest_mod, stop_date=None):
    BATCH_SIZE = 10000
    url = "https://%s/update-source-mod" % host
    logging.debug("top of update_mod_dates(); url: %s", url)
    y, m, d = [int(p) for p in latest_mod.split("-")]
    first = date = datetime.date(y, m, d)
    logging.debug("first date is %s", first)
    one_day = datetime.timedelta(1)
    if stop_date:
        try:
            y, m, d = [int(p) for p in stop_date.split("-")]
        except Exception, e:
            print "stop_date", repr(stop_date), e
            raise
        stop_date = datetime.date(y, m, d)
    else:
        one_week = datetime.timedelta(7)
        today = datetime.date.today()
        stop_date = today - one_week
    logging.debug("stop_date is %s", stop_date)
    if first >= stop_date:
        return "The data_mod column is up to date (as of %s)" % first
    total = 0
    while date < stop_date:
        last = date
        mod_pmids = get_mod_pmids(date, articles)
        date += one_day
        if mod_pmids:
            offset = 0
            while offset < len(mod_pmids):
                subset = "\t".join(mod_pmids[offset:offset+BATCH_SIZE])
                offset += BATCH_SIZE
                parms = "date=%s&source=Pubmed&ids=%s" % (date, subset)
                logging.debug("calling url with parms %s", parms)
                f = urllib2.urlopen(url, parms)
                if f.code != 200:
                    msg = ("Failure updating data_mod column for %s (code %s)"
                           % (date, f.code))
                    logging.error(msg)
                    return msg
                logging.debug("code is OK")
            total += len(mod_pmids)
        time.sleep(5)
    return "Updated data_mod column in %d rows (%s--%s)" % (total, first, last)

#----------------------------------------------------------------------
# Invoke the URL which refreshes the XML for a batch of articles.
# We repeat this until the command fails or reports that there is
# nothing left to update.
#----------------------------------------------------------------------
def refresh_xml(host):
    url = "https://%s/refresh-xml/Pubmed" % host
    while True:
        f = urllib2.urlopen(url)
        if f.code != 200:
            return "Failure refreshing XML (code %s)" % f.code
        response = f.read()
        try:
            remaining = int(response.strip())
        except Exception, e:
            print "remaining", repr(remaining), e
            raise
        if not remaining:
            return "All modified XML has been refreshed."

#----------------------------------------------------------------------
# Send an email report telling what happened during this run of the
# program.
#
#  @param  what             string for the body of the email message
#
#  @return                  nothing
#----------------------------------------------------------------------
def report(what, host="EBMS host name not specified"):
    sender = "ebms@cancer.gov"
    recips = get_recips()
    subject = "Update of XML from Pubmed"
    recip_list = ", ".join(recips)
    message = """\
From: %s
To: %s
Subject: %s

%s
%s
""" % (sender, recip_list, subject, host, what)
    server = smtplib.SMTP("MAILFWD.NIH.GOV")
    server.sendmail(sender, recips, message)
    server.quit()
    logging.info("sent report to %r", recips)

#----------------------------------------------------------------------
# Determine whether we are on the development machine.
#----------------------------------------------------------------------
def is_dev(host):
    normalized_host = host.lower()
    for alias in ("***REMOVED***", "ebms-dev"):
        if alias in normalized_host:
            return True
    return False

#----------------------------------------------------------------------
#
# Top-level driver.  Processing logic:
#
#   1. Fetch information about the articles in the EBMS DB.
#   2. Top up the ebms_article's data_mod column.
#   3. Refresh the source_data column's XML for modified articles.
#   4. Report what we did.
#
#----------------------------------------------------------------------
def main():
    start = time.time()
    if len(sys.argv) < 2:
        cmd = sys.argv[0]
        message = "command line argument for EBMS host name is required"
        print "usage %s EBMS-HOST NAME [LAST-MOD-DATE [STOP-DATE]]" % cmd
        report(message)
        exit(1)
    host = "%s.nci.nih.gov" % sys.argv[1]
    log_fmt = "%(asctime)s [%(levelname)s] %(message)s"
    log_file = os.path.expanduser("~/logs/update-pubmed-data.log")
    log_level = is_dev(host) and logging.DEBUG or logging.INFO
    logging.basicConfig(format=log_fmt, level=log_level, filename=log_file)
    try:
        logging.info("job started for %s", host)
        articles, latest_mod = get_articles(host)
        stop = None
        if len(sys.argv) > 2:
            latest_mod = sys.argv[2]
            logging.info("latest_mod given on command line as %s", latest_mod)
        if len(sys.argv) > 3:
            stop = sys.argv[3]
            logging.info("stop date given on command line as %s", stop)
        mod_date_report = update_mod_dates(host, articles, latest_mod, stop)
        refresh_report = refresh_xml(host)
        logging.info(refresh_report)
        elapsed = "Elapsed: %.3f seconds" % (time.time() - start)
        report("%s\n%s\n%s" % (mod_date_report, refresh_report, elapsed), host)
        logging.debug(elapsed)
    except Exception, e:
        logging.error("%s" % e)
        report("Failure: %s" % e, host)
        raise
if __name__ == "__main__":
    main()
