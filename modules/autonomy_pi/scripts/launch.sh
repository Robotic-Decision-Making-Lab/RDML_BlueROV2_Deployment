#!/bin/bash

# launch the autonomy stack
# this is meant to be run the ros.service
source /opt/ros/${ROS_DISTRO:-lyrical}/setup.bash \
  && source ${USER_WORKSPACE:-$HOME/ws_ros}/install/setup.bash \
  && ros2 launch autonomy_bringup estimation.launch.yaml
