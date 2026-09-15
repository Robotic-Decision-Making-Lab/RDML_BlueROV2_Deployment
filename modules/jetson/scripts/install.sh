#!/bin/bash

# install apt packages
sudo apt-get update \
  && sudo apt-get install -y \
    curl \
    build-essential \
    nmap \
    iputils-ping \
    git \
    net-tools \
    minicom \
  && sudo apt-get autoremove -y

# configure a static IP for the Ethernet interface.
#
# named with a 99- prefix so that it is applied after (and overrides) Ubuntu Server's default
# /etc/netplan/50-cloud-init.yaml.
sudo cp $AUTONOMY_PI/network/99-eth0-static.yaml /etc/netplan/ \
  && sudo chmod 600 /etc/netplan/99-eth0-static.yaml \
  && sudo netplan apply
