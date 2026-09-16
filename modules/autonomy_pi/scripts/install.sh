#!/bin/bash

# prior to running this script, the Pi NVMe drive should have been flashed.
#
# if it has not yet been flashed, refer to the following resource:
# https://wolfpaulus.com/rp5-ubuntu-cli/

export ROS_DISTRO=lyrical

# install apt packages
sudo apt-get update \
  && sudo apt-get install -y \
    curl \
    build-essential \
    nmap \
    iputils-ping \
    git \
    net-tools \
    python-is-python3 \
    minicom \
  && sudo apt-get autoremove -y

# install ROS 2
#
# See the ROS installation instructions for further information:
# https://docs.ros.org/en/lyrical/Installation/Ubuntu-Install-Debs.html
sudo apt update \
  && sudo apt install locales \
  && sudo locale-gen en_US en_US.UTF-8 \
  && sudo update-locale LC_ALL=en_US.UTF-8 LANG=en_US.UTF-8 \
  && export LANG=en_US.UTF-8 \
  && sudo apt install software-properties-common \
  && sudo add-apt-repository universe \
  && sudo apt update && sudo apt install curl -y \
  && sudo curl -sSL https://raw.githubusercontent.com/ros/rosdistro/master/ros.key -o /usr/share/keyrings/ros-archive-keyring.gpg \
  && echo "deb [arch=$(dpkg --print-architecture) signed-by=/usr/share/keyrings/ros-archive-keyring.gpg] http://packages.ros.org/ros2/ubuntu $(. /etc/os-release && echo $UBUNTU_CODENAME) main" | sudo tee /etc/apt/sources.list.d/ros2.list > /dev/null \
  && sudo apt update \
  && sudo apt install -y \
    ros-dev-tools \
    ros-$ROS_DISTRO-ros-base \
  && sudo apt autoremove -y

# configure the ROS 2 environment
echo "source /opt/ros/$ROS_DISTRO/setup.bash" >> ~/.bashrc \
  && source ~/.bashrc

# clone the repo outside the colcon workspace, then symlink only this module's
# ros/ directory into the workspace
#
# we use this to avoid cluttering the project workspace with packages from the other modules
export REPO_ROOT=/home/$USER/RDML_BlueROV2_Deployment
export AUTONOMY_PI=$REPO_ROOT/modules/autonomy_pi
export USER_WORKSPACE=/home/$USER/ws_ros

cd ~ \
  && { [ -d $REPO_ROOT ] || git clone git@github.com:Robotic-Decision-Making-Lab/RDML_BlueROV2_Deployment.git $REPO_ROOT; } \
  && mkdir -p $USER_WORKSPACE/src \
  && ln -sfn $AUTONOMY_PI/ros $USER_WORKSPACE/src/autonomy_pi

# configure a static IP for the Ethernet interface.
#
# named with a 99- prefix so that it is applied after (and overrides) Ubuntu Server's default
# /etc/netplan/50-cloud-init.yaml.
sudo cp $AUTONOMY_PI/network/99-eth0-static.yaml /etc/netplan/ \
  && sudo chmod 600 /etc/netplan/99-eth0-static.yaml \
  && sudo netplan apply

# install the project dependencies
vcs import $USER_WORKSPACE/src < $AUTONOMY_PI/deps.repos \
  && sudo rosdep init \
  && rosdep update \
  && rosdep install -y --from-paths $USER_WORKSPACE/src --ignore-src

# build the workspace
#
# the COLCON_DEFAULTS_FILE scopes the build to autonomy_bringup's dependency
# closure (see colcon-defaults.yaml), so unrelated packages pulled in by
# deps.repos don't get compiled here.
export COLCON_DEFAULTS_FILE=$AUTONOMY_PI/colcon-defaults.yaml
cd $USER_WORKSPACE \
  && MAKEFLAGS="-j1 -l1" colcon build \
  && echo "export COLCON_DEFAULTS_FILE=$COLCON_DEFAULTS_FILE" >> ~/.bashrc \
  && echo "if [ -f $USER_WORKSPACE/install/setup.bash ]; then source $USER_WORKSPACE/install/setup.bash; fi" >> ~/.bashrc \
  && source ~/.bashrc

# setup the power script
#
# this gets used to turn on and off system peripherals like the DVL, Alpha 5, etc.
sudo apt-get update \
  && sudo apt-get install -y python3-lgpio \
  && sudo chmod +x $AUTONOMY_PI/scripts/power.py \
  && echo "alias power='$AUTONOMY_PI/scripts/power.py'" >> ~/.bashrc \
  && source ~/.bashrc

# setup the pld monitor
sudo apt-get update \
  && sudo apt-get install -y gpiod \
  && sudo cp $AUTONOMY_PI/scripts/pld.py /usr/local/bin \
  && sudo chmod +x /usr/local/bin/pld.py \
  && sudo ln -s /usr/local/bin/pld.py /usr/local/bin/pld_monitor

# setup the utility aliases
#
# this might be better placed in an `aliases.sh` file, but they will only really
# be configured on install, so this is fine
sudo chmod +x $AUTONOMY_PI/scripts/reset_ekf.sh \
  && echo "alias reset-ekf='$AUTONOMY_PI/scripts/reset_ekf.sh'" >> ~/.bashrc \
  && sudo chmod +x $AUTONOMY_PI/scripts/calibrate_imu.py \
  && echo "alias calibrate-imu='$AUTONOMY_PI/scripts/calibrate_imu.py'" >> ~/.bashrc \
  && echo "alias wks='cd $USER_WORKSPACE'" >> ~/.bashrc \
  && echo "alias cbs='colcon build && source install/setup.bash'" >> ~/.bashrc \
  && source ~/.bashrc

# configure the uart pins so that we can stream serial data from the teensy
#
# this will get placed in the [all] section
echo "dtparam=uart0=on" | sudo tee -a /boot/firmware/config.txt > /dev/null

# configure user access to the I2C devices
#
# note that Ubuntu Server 26.04 enables serial-getty@ttyAMA0.service by default, which
# can result in /dev/ttyAMA0 being owned by root:tty and no group access despite running
# the following command (you will probably notice this because minicom or the driver isn't
# able to access ttyAMA0 without sudo permissions). if this happens, check
#   systemctl status serial-getty@ttyAMA0.service
# and disable it:
#   sudo systemctl disable serial-getty@ttyAMA0.service
#   sudo systemctl mask serial-getty@ttyAMA0.service
sudo usermod -aG dialout $USER

# disable sysrq
#
# for some reason this gets assigned to ttyAMA0 on the Pi 5? the simple solution is to
# disable it. you could also probably move to a different serial port on the Pi, but I
# don't really feel like doing that :D
echo 'kernel.sysrq=0' | sudo tee /etc/sysctl.d/99-disable-sysrq.conf

# install docker, which is used to run the autonomy stack on boot
#
# the stack is split across three containers (core, estimation, controllers) so that any
# one of them can be left out; see docker/docker-compose.yml.
curl -fsSL https://get.docker.com | sh \
  && sudo systemctl enable docker \
  && sudo systemctl start docker

# add your user to the `docker` group
#
# log out and back in (or run `newgrp docker`) for this to take effect
sudo usermod -aG docker $USER

# record the host GID that the core container needs supplementary access to
#
# /dev/i2c-1 (BME680) is owned by the host's i2c group, and that GID is not stable
# across distributions or images, so it is resolved here rather than hard-coded.
echo "I2C_GID=$(getent group i2c | cut -d: -f3)" > $AUTONOMY_PI/docker/.env

# the containers mount ~/.ros so that ROS logs survive a container being recreated.
# create it up front: docker would otherwise create the bind mount source as root and
# the container user (uid 1000) would not be able to write to it.
mkdir -p $HOME/.ros

# build the container image
sudo docker compose -f $AUTONOMY_PI/docker/docker-compose.yml build core

# configure systemd to run the ROS stack on boot
for unit in ros-core ros-estimation ros-controllers; do
  sed "s|__REPO_ROOT__|$REPO_ROOT|g" $AUTONOMY_PI/services/$unit.service \
    | sudo tee /etc/systemd/system/$unit.service > /dev/null
done

sudo systemctl daemon-reload \
  && sudo systemctl enable ros-core.service ros-estimation.service ros-controllers.service
