from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, EmitEvent, RegisterEventHandler
from launch.events import matches_action
from launch.substitutions import LaunchConfiguration, PathJoinSubstitution
from launch_ros.actions import LifecycleNode
from launch_ros.event_handlers import OnStateTransition
from launch_ros.events.lifecycle import ChangeState
from launch_ros.substitutions import FindPackageShare
from lifecycle_msgs.msg import Transition


def generate_launch_description():
    declare_ns = DeclareLaunchArgument("ns", default_value="")
    declare_parameters_file = DeclareLaunchArgument(
        "parameters_file",
        default_value=PathJoinSubstitution(
            [FindPackageShare("teensy_driver"), "config", "dvl.yaml"]
        ),
    )

    teensy_driver_node = LifecycleNode(
        package="autonomy_teensy",
        executable="teensy_driver",
        name="teensy_driver",
        namespace=LaunchConfiguration("ns"),
        output="screen",
        parameters=[LaunchConfiguration("parameters_file")],
    )

    configure_event = EmitEvent(
        event=ChangeState(
            lifecycle_node_matcher=matches_action(teensy_driver_node),
            transition_id=Transition.TRANSITION_CONFIGURE,
        )
    )

    activate_event = RegisterEventHandler(
        OnStateTransition(
            target_lifecycle_node=teensy_driver_node,
            start_state="configuring",
            goal_state="inactive",
            entities=[
                EmitEvent(
                    event=ChangeState(
                        lifecycle_node_matcher=matches_action(teensy_driver_node),
                        transition_id=Transition.TRANSITION_ACTIVATE,
                    )
                )
            ],
        )
    )

    return LaunchDescription(
        [
            declare_parameters_file,
            declare_ns,
            teensy_driver_node,
            configure_event,
            activate_event,
        ]
    )
