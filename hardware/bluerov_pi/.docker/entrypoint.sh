#!/bin/bash

# Source ROS 2
source /opt/ros/${ROS_DISTRO}/setup.bash

# Source the base workspace, if built
if [ -f /home/ubuntu/ws_ros/install/setup.bash ]
then
  source /home/ubuntu/ws_ros/install/setup.bash
fi

# Execute the command passed into this entrypoint
exec "$@"
