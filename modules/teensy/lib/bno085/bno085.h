#ifndef BNO085_H_
#define BNO085_H_

#include <Arduino.h>
#include <SparkFun_BNO08x_Arduino_Library.h>

#include <cstdint>
#include <optional>

/// one fused orientation/motion reading from the IMU
struct Sample
{
  float qx, qy, qz, qw;
  float gx, gy, gz;
  float ax, ay, az;
};

/// per-sensor accuracy/status reported alongside a Sample
struct SensorAccuracy
{
  uint8_t accel;
  uint8_t gyro;
  uint8_t mag;
  uint8_t quat;
  float quat_rad;
};

/// driver wrapping the BNO08x IMU
class BNO085
{
public:
  /// initialize the IMU over I2C, retrying until timeout_ms elapses
  [[nodiscard]] auto begin(
    uint8_t address,
    TwoWire & wire,
    uint8_t int_pin,
    uint8_t rst_pin,
    uint32_t report_period_ms = 5,
    uint32_t gyro_stale_ms = 100,
    uint32_t timeout_ms = 5000) -> bool;

  /// (re-)enable the sensor reports the driver depends on
  [[nodiscard]] auto configure() -> bool;

  /// check whether the IMU reported an unexpected reset
  [[nodiscard]] auto was_reset() -> bool;

  /// reconfigure the IMU and restore its prior calibration state after a reset
  [[nodiscard]] auto reset() -> bool;

  /// read the next available Sample, if a complete one is ready
  [[nodiscard]] auto read() -> std::optional<Sample>;

  /// begin calibrating the accelerometer, gyroscope, and magnetometer
  [[nodiscard]] auto start_calibration() -> bool;

  /// stop calibration and return to the default sensor configuration
  [[nodiscard]] auto stop_calibration() -> bool;

  /// persist the current calibration to the IMU's non-volatile memory
  [[nodiscard]] auto save_calibration() -> bool;

  /// get the most recently reported per-sensor accuracy
  [[nodiscard]] auto accuracy() -> SensorAccuracy;

  /// check whether the IMU is currently calibrating
  [[nodiscard]] auto calibrating() const -> bool;

  /// get the bitmask of sensors currently being calibrated
  [[nodiscard]] auto calibration_mask() const -> uint8_t;

private:
  /// enable the accelerometer, gyroscope, and rotation vector reports
  [[nodiscard]] auto set_reports() -> bool;

  /// apply a calibration configuration for the given sensor bitmask
  [[nodiscard]] auto calibrate(uint8_t sensors, uint32_t mag_report_period_ms = 20) -> bool;

  BNO08x imu_;
  Sample sample_{};
  SensorAccuracy accuracy_{};

  uint32_t report_period_ms_{0};
  uint32_t gyro_stale_ms_{0};

  bool have_accel_{false};
  bool have_quat_{false};
  uint32_t last_gyro_ms_{0};

  uint8_t cal_mask_{0};
};

#endif
