#!/bin/bash

# This is a simple script used to reset the estimated state of the
# vehicle during runtime. This is helpful, e.g., when the state
# estimates have started to drift (when the USBL isn't available,
# which is literally always)
ros2 service call ...
