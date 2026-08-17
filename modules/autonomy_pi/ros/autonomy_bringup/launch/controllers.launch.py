# Copyright 2025, Evan Palmer
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
# THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.

import itertools

from launch import LaunchDescription
from launch.actions import RegisterEventHandler
from launch.event_handlers import OnProcessExit
from launch.substitutions import PathJoinSubstitution
from launch_ros.actions import Node
from launch_ros.substitutions import FindPackageShare


def generate_launch_description() -> LaunchDescription:
    controller_manager = Node(
        package="controller_manager",
        executable="ros2_control_node",
        output="both",
        parameters=[
            PathJoinSubstitution(
                [
                    FindPackageShare("autonomy_description"),
                    "config",
                    "controllers.yaml",
                ]
            ),
        ],
    )

    def make_controller_args(
        name: str, active: bool = False, remappings: list[str] | None = None
    ):
        cm = ["--controller-manager", ["", "controller_manager"]]
        controller_timeout = ["--controller-manager-timeout", "120"]
        switch_timeout = ["--switch-timeout", "100"]
        inactive = ["--inactive"] if not active else []
        remap = (
            itertools.chain(
                *[["--controller-ros-args", f"-r {remap}"] for remap in remappings]
            )
            if remappings
            else []
        )
        commands = [name, *cm, *controller_timeout, *switch_timeout, *inactive, *remap]
        return commands

    vehicle_state = "/vehicle/odometry/filtered"
    impedance_controller_spawner = Node(
        package="controller_manager",
        executable="spawner",
        arguments=make_controller_args(
            "impedance_controller",
            remappings=[f"impedance_controller/system_state:={vehicle_state}"],
        ),
    )

    tam_controller_spawner = Node(
        package="controller_manager",
        executable="spawner",
        arguments=make_controller_args("thruster_allocation_matrix_controller"),
    )

    thruster_controller_spawners = [
        Node(
            package="controller_manager",
            executable="spawner",
            arguments=make_controller_args(f"thruster{i + 1}_controller"),
        )
        for i in range(8)
    ]

    delay_thruster_controller_spawners = []
    for i, thruster_spawner in enumerate(thruster_controller_spawners):
        if not len(delay_thruster_controller_spawners):
            delay_thruster_controller_spawners.append(
                thruster_spawner,
            )
        else:
            delay_thruster_controller_spawners.append(
                RegisterEventHandler(
                    event_handler=OnProcessExit(
                        target_action=thruster_controller_spawners[i - 1],
                        on_exit=[thruster_spawner],
                    )
                )
            )

    delay_tam_controller_spawner_after_thruster_controller_spawners = (
        RegisterEventHandler(
            event_handler=OnProcessExit(
                target_action=thruster_controller_spawners[-1],
                on_exit=[tam_controller_spawner],
            )
        )
    )

    delay_impedance_controller_spawner_after_tam_controller_spawner = (
        RegisterEventHandler(
            event_handler=OnProcessExit(
                target_action=tam_controller_spawner,
                on_exit=[impedance_controller_spawner],
            )
        )
    )

    controller_coordinator_spawner = Node(
        package="controller_coordinator",
        executable="controller_coordinator",
        parameters=[
            PathJoinSubstitution(
                [
                    FindPackageShare("autonomy_description"),
                    "config",
                    "coordinator.yaml",
                ]
            )
        ],
    )

    return LaunchDescription(
        [
            controller_manager,
            controller_coordinator_spawner,
            *delay_thruster_controller_spawners,
            delay_tam_controller_spawner_after_thruster_controller_spawners,
            delay_impedance_controller_spawner_after_tam_controller_spawner,
        ]
    )
