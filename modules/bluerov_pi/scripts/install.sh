#!/bin/bash

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

cd ~ \
  && git clone git@github.com:Robotic-Decision-Making-Lab/RDML_BlueROV2_Deployment.git

# configure the default gateway for the Ethernet interface
sudo cp ~/RDML_BlueROV2_Deployment/modules/bluerov_pi/network/eth0.network /etc/systemd/network/ \
  && sudo systemctl restart systemd-networkd

# setup the ROS systemd service
#
# this runs by default so that MAVROS doesn't need to be launched manually
sudo cp ~/RDML_BlueROV2_Deployment/modules/bluerov_pi/services/ros.service /etc/systemd/system/ \
  && sudo systemctl enable ros.service \
  && sudo systemctl start ros.service
