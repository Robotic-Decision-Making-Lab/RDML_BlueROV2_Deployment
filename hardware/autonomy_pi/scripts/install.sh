#!/bin/bash

export ROS_DISTRO=lyrical

# Install apt packages
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

# Install ROS 2
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

# Configure the environment
echo "source /opt/ros/$ROS_DISTRO/setup.bash" >> ~/.bashrc \
  && source ~/.bashrc

# Create a workspace
cd ~ \
  && export USER_WORKSPACE=/home/$USER/ws_ros \
  && mkdir -p $USER_WORKSPACE/src \
  && cd $USER_WORKSPACE

# Install the project dependencies
git clone git@github.com:Robotic-Decision-Making-Lab/RDML_BlueROV2_Deployment.git src/RDML_BlueROV2_Deployment \
  && export AUTONOMY_PI=$USER_WORKSPACE/src/RDML_BlueROV2_Deployment/hardware/autonomy_pi \
  && vcs import src < $AUTONOMY_PI/ros/pi.repos \
  && sudo rosdep init \
  && rosdep update \
  && rosdep install -y --from-paths src --ignore-src

# Build the workspace
cd $USER_WORKSPACE \
  && MAKEFLAGS="-j1 -l1" colcon build \
  && echo "if [ -f /home/neptune/ws_ros/install/setup.bash ]; then source /home/neptune/ws_ros/install/setup.bash; fi" >> ~/.bashrc \
  && source ~/.bashrc

# Create the micro-ROS workspace
# cd ~ \
#   && export MICROROS_WORKSPACE=/home/$USER/ws_microros \
#   && mkdir -p $MICROROS_WORKSPACE/src \
#   && cd $MICROROS_WORKSPACE

# # Install micro-ROS
# git clone -b $ROS_DISTRO https://github.com/micro-ROS/micro_ros_setup.git src/micro_ros_setup \
#     && sudo apt update \
#     && rosdep update \
#     && rosdep install --from-paths src --ignore-src -r -y

# source /opt/ros/$ROS_DISTRO/setup.sh \
#     && colcon build \
#     && source /home/$USER/ws_microros/install/setup.sh \
#     && ros2 run micro_ros_setup create_agent_ws.sh \
#     && ros2 run micro_ros_setup build_agent.sh \
#     && echo "if [ -f /home/$USER/ws_microros/install/setup.sh ]; then source /home/$USER/ws_microros/install/setup.sh; fi" >> /home/$USER/.bashrc \
#     && source ~/.bashrc

# Setup the power script
sudo apt-get update \
  && sudo apt-get install -y python3-lgpio \
  && sudo chmod +x $AUTONOMY_PI/scripts/power.py \
  && echo "alias power='$AUTONOMY_PI/scripts/power.py'" >> ~/.bashrc \
  && source ~/.bashrc

# Setup the pld monitor
sudo apt-get update \
  && sudo apt-get install -y gpiod \
  && sudo cp $AUTONOMY_PI/scripts/pld.py /usr/local/bin \
  && sudo chmod +x /usr/local/bin/pld.py \
  && sudo ln -s /usr/local/bin/pld.py /usr/local/bin/pld_monitor

# Setup the reset_ekf alias
sudo chmod +x $AUTONOMY_PI/scripts/reset_ekf.sh \
  && echo "alias reset-ekf='$AUTONOMY_PI/scripts/reset_ekf.sh'" >> ~/.bashrc \
  && source ~/.bashrc

# Configure the uart pins
echo "dtparam=uart0=on" | sudo tee -a /boot/firmware/config.txt > /dev/null

# Configure systemd to run the micro-ROS service on boot
sudo cp $AUTONOMY_PI/systemd/microros.service /etc/systemd/system \
  && sudo systemctl enable microros.service

# Configure user access to the I2C devices
sudo usermod -aG dialout $USER

# Configure systemd to run the ROS stack on boot
# sudo cp $AUTONOMY_PI/scripts/launch.sh /usr/local/bin \
#   && cp $AUTONOMY_PI/services/ros.service /etc/systemd/system \
#   && sudo systemctl enable ros.service
