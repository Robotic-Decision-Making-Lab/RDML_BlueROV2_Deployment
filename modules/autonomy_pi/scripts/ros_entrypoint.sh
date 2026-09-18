#!/bin/bash

# run a command inside the ROS 2 environment (the system install and the workspace overlay)
#
# systemd services don't source ~/.bashrc, so this is used by the services in services/
# to set up the environment before handing off to, e.g., `ros2 launch`
source /opt/ros/${ROS_DISTRO:-lyrical}/setup.bash \
  && source ${USER_WORKSPACE:-$HOME/ws_ros}/install/setup.bash \
  && exec "$@"
