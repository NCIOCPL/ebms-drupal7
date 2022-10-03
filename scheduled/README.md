# EBMS cron jobs
Install under the account which runs the web server (`sudo crontab -u ACCOUNT-NAME -e`). For example:

```
0 2 * * 0 cd /var/www/ebms && ./vendor/bin/drush scr --script-path=/var/www/ebms/scheduled find-pubmed-drops
```
