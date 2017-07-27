#!/usr/bin/python

#----------------------------------------------------------------------
#
# Add the not_reviewed article tag to articles which have been waiting
# for review so long that we've given up.
#
# JIRA::OCEEBMS-426
#
#----------------------------------------------------------------------
import datetime
import logging
import os
import smtplib
import subprocess
import sys
import urllib2

def fix_path():
    """
    Make sure CBIIT's custom PHP can be found.
    """

    path = os.environ.get("PATH", "")
    logging.debug("PATH is %r", path)
    if "php" not in path:
        path = "%s:/usr/local/php/bin" % path
        logging.debug("setting path to %r", path)
        os.environ["PATH"] = path

def get_drupal_var(name):
    """
    Fetch a drupal variable by name.
    """

    command = "/usr/local/bin/drush vget --exact --format=string %s" % name
    opts = {
        "stdout": subprocess.PIPE,
        "cwd": "/local/drupal/sites/ebms.nci.nih.gov",
        "shell": True
    }
    logging.debug("running %r", command)
    logging.debug("options %r", opts)
    try:
        value = subprocess.Popen(command, **opts).stdout.read()
        logging.debug("value is %r", value)
        return value
    except Exception as e:
        logging.exception("fetching drupal var %r", name)
        return ""

def get_dev_notification_recipients():
    """
    Fetch the list of developer notification email addresses.
    """

    recips = []
    for recip in get_drupal_var("dev_notif_addr").strip().split(","):
        recip = recip.strip()
        if recip:
            recips.append(recip)
    return recips

def report(what, host="EBMS host name not specified"):
    """
    Send an email report telling what happened during this run of the
    program.

    @param  what             string for the body of the email message

    @return                  nothing
    """

    recips = get_dev_notification_recipients()
    if not recips:
        logging.error("no developer notification email addresses set")
        return
    sender = "ebms@cancer.gov"
    subject = "Marking unreviewed articles"
    recip_list = ",\n  ".join(recips)
    message = """\
From: %s
To: %s
Subject: %s

%s
%s
""" % (sender, recip_list, subject, host, what)
    logging.debug("sending email report")
    try:
        server = smtplib.SMTP("MAILFWD.NIH.GOV")
        server.sendmail(sender, recips, message)
        logging.debug("back from call to sendmail()")
        server.quit()
    except Exception as e:
        logging.exception("reporting to %r", recips)

def is_dev(host):
    """
    Determine whether we are on the development machine.
    """

    normalized_host = host.lower()
    for alias in ("***REMOVED***", "ebms-dev"):
        if alias in normalized_host:
            return True
    return False

def main():
    """
    Invoke the URL for marking unreviewed articles and log results.
    """

    start = datetime.datetime.now()
    if len(sys.argv) < 2:
        cmd = sys.argv[0]
        message = "command line argument for EBMS host name is required"
        print "usage %s EBMS-HOST-NAME" % cmd
        report(message)
        exit(1)
    host = "%s.nci.nih.gov" % sys.argv[1]
    url = "https://%s/admin/mark-unreviewed-articles" % host
    log_fmt = "%(asctime)s [%(levelname)s] %(message)s"
    log_file = os.path.expanduser("~/logs/unreviewed-articles.log")
    #log_file = "/tmp/unreviewed-articles.log"
    log_level = is_dev(host) and logging.DEBUG or logging.INFO
    logging.basicConfig(format=log_fmt, level=log_level, filename=log_file)
    logging.info("job started for %s", host)
    fix_path()
    try:
        logging.debug("opening %s", url)
        f = urllib2.urlopen(url)
        logging.debug("response received from the EBMS host")
        line = f.readline()
        last_line = "marking unreviewed articles"
        while line:
            logging.info(line.strip())
            last_line = line.strip()
            line = f.readline()
        elapsed = datetime.datetime.now() - start
        # this ancient version of python doesn't have total_seconds()
        elapsed = (elapsed.microseconds +
                   (elapsed.seconds + elapsed.days * 24.0 * 3600) * 10**6
                   ) / 10**6
        logging.info("elapsed time: %f seconds", elapsed)
        report("%s in %f seconds" % (last_line, elapsed), host)
    except Exception as e:
        logging.exception("failure")
        report("Failure: %s" % e, host)
        raise
if __name__ == "__main__":
    main()
