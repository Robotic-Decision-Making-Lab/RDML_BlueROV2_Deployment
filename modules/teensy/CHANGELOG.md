# Changelog for module teensy

## 0.2.0

- Implements an interface for calibrating the BNO08x IMU.
- Adds support for receiving commands and sending command responses.
- Disables magnetometer and accelerometer dynamic calibration during normal
  operation to avoid degraded compass calibration during deployment.

## 0.1.0

- Initial project release.
- Implements a custom serial driver to replace micro-ros dependency.
