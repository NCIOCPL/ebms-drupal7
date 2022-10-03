FROM ubuntu:jammy

RUN DEBIAN_FRONTEND=noninteractive apt update

RUN DEBIAN_FRONTEND=noninteractive \
    apt install -yq \
    apache2 \
    build-essential \
    php \
    libapache2-mod-php \
    php-bz2 \
    php-cli \
    php-common \
    php-curl \
    php-fpm \
    php-gd \
    php-json \
    php-mbstring \
    php-memcached \
    php-mysql \
    php-oauth \
    php-opcache \
    php-readline \
    php-sqlite3 \
    php-soap \
    php-xdebug \
    php-xml \
    php-zip \
    mariadb-client \
    ssmtp \
    curl \
    git \
    imagemagick \
    vim \
    python3 \
    emacs-nox \
    elpa-php-mode \
    locales \
    elpa-python-environment \
    wget \
    p7zip \
    zip

ADD php.ini /etc/php/8.1/apache2/php.ini
ADD ssmtp.conf /etc/ssmtp/ssmtp.conf
RUN chmod 666 /etc/ssmtp/ssmtp.conf
RUN a2enmod rewrite

RUN locale-gen en_US.UTF-8
ENV LANG en_US.UTF-8
ENV LANGUAGE en_US:en
ENV LC_ALL en_US.UTF-8
ENV TZ America/New_York

RUN curl -sS https://getcomposer.org/installer | \
    php -- --install-dir=/usr/local/bin --filename=composer

CMD ["apachectl", "-D", "FOREGROUND"]

WORKDIR /var/www
