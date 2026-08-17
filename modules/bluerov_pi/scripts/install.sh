#!/bin/bash

# Install apt packages
sudo apt-get update \
  && sudo apt-get install -y \
    python3-pip \
    python3-venv \
    curl \
    build-essential \
    nmap \
    iputils-ping \
    git \
  && sudo apt-get autoremove -y

# Clone the bluerov-deployment repo
cd ~ \
  && git clone git@github.com:evan-palmer/bluerov-deployment.git

# Configure the default gateway for the Ethernet interface
sudo cp /home/pi/bluerov-development/bluerov-pi/network/eth0.network /etc/systemd/network/ \
  && sudo systemctl restart systemd-networkd

# Setup the ROS systemd service
sudo cp /home/pi/bluerov-development/bluerov-pi/systemd/ros.service /etc/systemd/system/ \
  && sudo systemctl enable ros.service \
  && sudo systemctl start ros.service
