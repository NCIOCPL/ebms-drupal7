#!/usr/bin/env python3

"""Refresh EBMS articles from NLM's PubMed.

Find out which PubMed articles have been modified since we last
pulled down the XML from NLM, and refresh those documents.

See https://tracker.nci.nih.gov/browse/OCEEBMS-87.
"""

import argparse
import datetime
import logging
import os
import re
import smtplib
import time
import requests

FAILURE = "Failure updating data_mod column for {} (code {})"

def get_recips():
    """Get email addresses for notification.

    Use drush to get the list of recipients, so we don't have to store
    developers' email addresses in code.
    """

    saved_dir = os.getcwd()
    os.chdir("/local/drupal/sites/ebms.nci.nih.gov")
    cmd = "/usr/local/bin/drush vget --format=string --exact dev_notif_addr"
    stream = os.popen(cmd)
    output = stream.read().strip()
    stream.close()
    os.chdir(saved_dir)
    return [address.strip() for address in output.split(",")]

def get_articles(host):
    """Get a catalog all of the articles in the EBMS.

    Ask the EBMS web application to give us a list of all of the
    Pubmed articles in the ebms_article table.  Each article is
    represented in the web application's response by a line containing
    the articles Pubmed ID, the date of the last time we fetched
    the XML for the article from NLM, and the date previously given
    by NLM for the most recent modification of the article (if any).
    The three values are separated by the tab character.  A dictionary
    is constructed and returned, indexed by the Pubmed IDs, with the
    other two values as the data for each node in the dictionary.

    @param  host       DNS name of EBMS web server (e.g., ebms.nci.nih.gov)

    @return            tuple containing the dictionary for the articles,
                       and the latest date found in the data_mod column
                       or the first day of 2012, if all the values in
                       that column are null; our first retrieval of
                       XML from NLM for the EBMS was in January of 2012,
                       so 2012-01-01 will catch any modifications we
                       need.
    """

    url = "https://%s/get-source-ids/Pubmed" % host
    logging.debug("get_articles(%s)", url)
    tries = 10
    delay = 10
    while True:
        try:
            response = requests.get(url)
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
    if response.status_code == 200:
        for line in response.text.splitlines():
            fields = line.strip().split("\t")
            mod = None
            if len(fields) == 3:
                mod = fields[2]
                if mod and mod > latest_mod:
                    latest_mod = mod
            elif len(fields) != 2:
                print(repr(line), repr(fields))
                continue
            pmids[fields[0]] = (fields[1], mod)
    else:
        raise Exception("Failure fetching article information: HTTP code %s" %
                        response.status_code)
    logging.info("received %d PMIDs; latest_mod=%s", len(pmids), latest_mod)
    return pmids, latest_mod

def get_mod_pmids(date, articles):
    """Find out what has changed on `date`.

    Ask NLM to tell us which Pubmed articles were modified on a specific
    day.  For each Pubmed ID we get back, if the corresponding article
    is in our database, and if the data_mod column for that article
    does not already contain the modification date NLM has for the
    article, add the Pubmed ID to the list to be returned to the caller.
    The return document from NLM can be larger than we could parse in
    memory, so we use a regular expression to tease out the Pubmed IDs.
    We're relying on the fact that NLM returns each ID on a separate line.

    @param  date             date we're asking NLM about
    @param  articles         information about articles in the EBMS DB

    @return                  list of Pubmed IDs we need to record as
                             having been modified on the date specified
    """

    base = "https://eutils.ncbi.nlm.nih.gov/entrez/eutils/esearch.fcgi"
    mod = []
    d = str(date).replace('-', '/')
    parms = "db=pubmed&retmax=50000000&term=%s[MDAT]" % d
    url = "%s?%s" % (base, parms)
    logging.info("opening %s", url)
    response = requests.get(url)
    logging.debug("response received from NLM")
    logging.debug("response from NLM:\n%s", response.text)
    for line in response.text.splitlines():
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

def update_mod_dates(host, articles, latest_mod, stop_date=None):
    """Update the EBMS data_mod column in the ebms_article table.

    Find out from NLM when the XML documents for articles in our
    database have been updated.  Start the last date found in the
    ebms_article table's data_mod column and work forward until we
    get to one week earlier than today.  For each day, ask Pubmed
    for the list of articles whose XML changed on that day, and
    send that list to the EBMS Drupal web application for storage.
    There's a limit on how much data can be sent to that application
    at once, so if we have more than BATCH_SIZE Pubmed IDs, we
    send them in batches of that size.

    @param  host       DNS name of EBMS web server (e.g., ebms.nci.nih.gov)
    @param  articles   information on the articles in the EBMS
    @param  latest_mod latest value found in the data_mod column

    @return            string summarizing processing activity
    """

    BATCH_SIZE = 100
    url = "https://%s/update-source-mod" % host
    logging.debug("top of update_mod_dates(); url: %s", url)
    y, m, d = [int(p) for p in latest_mod.split("-")]
    first = date = datetime.date(y, m, d)
    logging.debug("first date is %s", first)
    one_day = datetime.timedelta(1)
    if stop_date:
        try:
            y, m, d = [int(p) for p in stop_date.split("-")]
        except Exception as e:
            print("stop_date", repr(stop_date), e)
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
                params = dict(date=date, source="Pubmed", ids=subset)
                logging.debug("calling url with params %s", params)
                response = requests.post(url, data=params)
                if response.status_code != 200:
                    msg = FAILURE.format(date, response.status_code)
                    logging.error(msg)
                    logging.error("reason: %s", response.reason)
                    return msg
                logging.debug("code is OK")
            total += len(mod_pmids)
        logging.debug("sleeping ...")
        time.sleep(5)
    return f"Updated data_mod column in {total:d} rows ({first}--{last})"

def refresh_xml(host):
    """Invoke the URL which refreshes the XML for a batch of articles.

    We repeat this until the command fails or reports that there is
    nothing left to update.

    @param  host       DNS name of EBMS web server (e.g., ebms.nci.nih.gov)

    @return            string description of what we did
    """

    url = "https://%s/refresh-xml/Pubmed" % host
    logging.debug("refresh_xml(%s)", url)
    while True:
        response = requests.get(url)
        if response.status_code != 200:
            return f"Failure refreshing XML (code {response.status_code})"
        try:
            remaining = int(response.text.strip())
        except Exception as e:
            print("remaining", repr(remaining), e)
            raise
        if not remaining:
            return "All modified XML has been refreshed."

def report(what, host="EBMS host name not specified"):
    """Send an email report telling what happened during this run.

    @param  what             string for the body of the email message

    @return                  nothing
    """

    sender = "ebms@cancer.gov"
    try:
        recips = get_recips()
    except Exception as e:
        logging.exception("fetching recipients")
        return
    subject = "Update of XML from Pubmed"
    recip_list = ", ".join(recips)
    message = """\
From: %s
To: %s
Subject: %s

%s
%s
""" % (sender, recip_list, subject, host, what)
    try:
        server = smtplib.SMTP("MAILFWD.NIH.GOV")
        server.sendmail(sender, recips, message)
        server.quit()
        logging.info("sent report to %r", recips)
    except Exception as e:
        logging.exception("notifying recipients")

def is_dev(host):
    """Determine whether we are on the development machine."""
    return "-dev" in host.lower()

def main():
    """Top-level driver.  Processing logic:

          1. Fetch information about the articles in the EBMS DB.
          2. Top up the ebms_article's data_mod column.
          3. Refresh the source_data column's XML for modified articles.
          4. Report what we did.
    """

    start = datetime.datetime.now()
    parser = argparse.ArgumentParser()
    parser.add_argument("host_name")
    parser.add_argument("--latest_mod")
    parser.add_argument("--stop")
    parser.add_argument("--level")
    opts = parser.parse_args()
    host = f"{opts.host_name}.nci.nih.gov"
    log_fmt = "%(asctime)s [%(levelname)s] %(message)s"
    log_file = os.path.expanduser("~/logs/update-pubmed-data.log")
    default_level = "DEBUG" if is_dev(host) else "INFO"
    level = opts.level or default_level
    logging.basicConfig(format=log_fmt, level=level, filename=log_file)
    try:
        logging.info("job started for %s", host)
        articles, latest_mod = get_articles(host)
        logging.debug("latest_mod is %s", latest_mod)
        if opts.latest_mod:
            latest_mod = opts.latest_mod
            logging.info("latest_mod is now %s", opts.latest_mod)
        if opts.stop:
            stop = opts.stop
            logging.info("stop date is %s", stop)
        else:
            stop = None
        logging.debug("found %d articles", len(articles))
        mod_date_report = update_mod_dates(host, articles, latest_mod, stop)
        refresh_report = refresh_xml(host)
        logging.info(refresh_report)
        elapsed = f"Elapsed: {datetime.datetime.now() - start}"
        report(f"{mod_date_report}\n{refresh_report}\n{elapsed}", host)
        logging.debug(elapsed)
    except Exception as e:
        logging.error(str(e))
        report(f"Failure: {e}", host)
        raise

if __name__ == "__main__":
    main()
