# EBMS 4.0
This version of the PDQ® Editorial Board Management System has been
rewritten to use Drupal 9.x. The project directory was initialized
with the command `composer create-project drupal/recommended-project
ebms4`. This page focuses on setting up a `Docker` container for doing
development work on the EBMS, with non-sensitive dummy data which can
be put under version control. For information on installing the EBMS
on a CBIIT-hosted server, refer to the [migration
documentation](migration/README.md).

## Developer Setup

To create a local development environment for this project, perform the following steps. You will need a recent PHP (8.1 is recommended) and composer 2.x.

1. Clone the repository.
2. Change current directory to the cloned repository.
3. Create a new `unversioned` directory.
4. Run `composer install`.
5. Copy `dburl.example` to `unversioned/dburl`.
6. Create an admin password, copy `adminpw.example` to `unversioned/adminpw`  and put the admin password in the copied file.
7. Create a user password, copy `userpw.example` to `unversioned/userpw`  and put the user password in the copied file.
8. Copy `sitehost.example` to `unversioned/sitehost` and replace the host name if appropriate.
9. Run `docker compose up -d`.
10. Run `docker exec -it ebms-web-1 bash`.
11. Inside the container, run `./install.sh`.
12. Point your favorite browser to http://ebms.localhost:8081.
13. Log in as admin using the password you created in step 5.

On a non-Docker server running Apache or Nginx, instead of step 4,
create a MySQL database using a secure database password, skip steps
8-10, and for step 11 substitute the appropriate URL. Adjust the
`unversioned/dburl` file to use the correct database hostname, port,
and password. In the following commands, replace "localhost" with the
name of the database server if appropriate.

```
CREATE DATABASE ebms;
CREATE USER 'ebms'@'localhost' IDENTIFIED BY '<your-strong-db-password>';
GRANT SELECT, INSERT, UPDATE, DELETE, CREATE, DROP, INDEX, ALTER, CREATE TEMPORARY TABLES, LOCK TABLES ON ebms.* TO 'ebms'@'localhost';
```

## Updated packages.

To update Drupal core (for example, when a new version of Drupal is
released to address serious bugs or security vulnerabilities), run

```
composer update drupal/core "drupal/core-*" --with-all-dependencies
```

Commit the updated `composer.*` files. When other developers pull down
to those files, they should run

```
composer install
```

## Updated Docker configuration

If settings are changed in `docker-compose.yml` or `Dockerfile` you
will need to rebuild the images and containers with

```
docker compose up --build
```
