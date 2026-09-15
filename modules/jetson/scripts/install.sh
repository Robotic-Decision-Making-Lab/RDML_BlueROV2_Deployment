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

# install docker and the NVIDIA Container Toolkit
sudo apt-get update \
  && sudo apt-get install -y \
    nvidia-container \
    curl \
    jq \
  && curl https://get.docker.com | sh \
  && sudo nvidia-ctk runtime configure --runtime=docker \
  && sudo systemctl daemon-reload \
  && sudo systemctl restart docker

# set the NVIDIA runtime as the default Docker runtime:
sudo jq '. + {"default-runtime": "nvidia"}' /etc/docker/daemon.json | \
    sudo tee /etc/docker/daemon.json.tmp
sudo mv /etc/docker/daemon.json.tmp /etc/docker/daemon.json
sudo systemctl restart docker

# add your user to the `docker` group
sudo usermod -aG docker $USER
newgrp docker

# clone the repo
export REPO_ROOT=/home/$USER/RDML_BlueROV2_Deployment
export JETSON=$REPO_ROOT/modules/jetson

cd ~ \
  && { [ -d $REPO_ROOT ] || git clone git@github.com:Robotic-Decision-Making-Lab/RDML_BlueROV2_Deployment.git $REPO_ROOT; } \

# configure a static IP for the Ethernet interface.
sudo cp $JETSON/network/99-enP8p1s0-static.yaml /etc/netplan/ \
  && sudo chmod 600 /etc/netplan/99-enP8p1s0-static.yaml \
  && sudo netplan apply
