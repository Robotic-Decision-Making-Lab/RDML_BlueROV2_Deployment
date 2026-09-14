# Changelog for module autonomy_pi

## 0.2.1

- Fixes parameter passing for `controllers.launch.py`.
- Re-expresses the thruster allocation matrix in the FLU frame and corrects the
  `reverse_spin_direction` flags so that thruster behavior matches the
  previously validated configuration.

## 0.2.0

- Implements the `autonomy_msgs` package, starting with `ImuCalibrationStatus`.
- Implements a calibration interface for the autonomy bottle IMU.
- Adds the `calibrate-imu` alias to simplify the calibration procedure.

## 0.1.0

- Initial project release.
- Migrates from ROS 2 Jazzy to ROS 2 Lyrical.
- Implements the `autonomy_teensy` package to interface with the Teensy 4.0.
- Implements the `cbs`, `wks`, and `reset-ekf` aliases.
- Implements the `estimation.launch.yaml` launch file to consolidate launch
  targets used for state estimation.
- Replaces the custom TAM with the standard TAM and incorporates the
  `reverse_spin_direction` to address reversed thrusters.
- Places all configuration files into `autonomy_description/config` to avoid
  scattered configuration files.
- Implements second netplan configuration file for ethernet connection instead
  of editing the base configuration file.
- Implements the `colcon-defaults.yaml` file to set standard build settings
  for colcon.
