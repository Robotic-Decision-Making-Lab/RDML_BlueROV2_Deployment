# Changelog for module autonomy_pi

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
