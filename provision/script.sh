#! /usr/bin/env bash

# Hiredis headers
sudo apt-get install -y libhiredis-dev python-pip python-dev

# Some configuration files
(
    cd /vagrant/provision
    sudo cp etc/redis.conf /etc/redis.conf
    sudo cp etc/init/redis.conf /etc/init/redis.conf
    sudo cp etc/sysctl.d/99-overcommit.conf /etc/sysctl.d/99-overcommit.conf
)

# Install redis server
wget http://download.redis.io/releases/redis-2.8.19.tar.gz
tar xf redis-2.8.19.tar.gz
(
    cd redis-2.8.19
    make
    sudo make install
    sudo service redis start
    sudo rm -r redis-2.8.19{,.tar.gz}
)

# Install dependencies and the thing itself
(
    cd /vagrant/
    sudo pip install -r requirements.txt
    sudo make install
)
