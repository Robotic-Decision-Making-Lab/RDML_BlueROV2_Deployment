#!/bin/bash

source /opt/ros/${ROS_DISTRO}/setup.bash

if [ -f /home/ubuntu/ws_ros/install/setup.bash ]
then
  source /home/ubuntu/ws_ros/install/setup.bash
fi

exec "$@"
