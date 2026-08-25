#ifndef BNO085_H_
#define BNO085_H_

#include <Arduino.h>
#include <SparkFun_BNO08x_Arduino_Library.h>

#include <cstdint>
#include <optional>

struct Sample
{
  float qx, qy, qz, qw;
  float gx, gy, gz;
  float ax, ay, az;
};

struct SensorAccuracy
{
  uint8_t accel;
  uint8_t gyro;
  uint8_t mag;
  uint8_t quat;
  float quat_rad;
};

class BNO085
{
public:
  [[nodiscard]] auto begin(
    uint8_t address,
    TwoWire & wire,
    uint8_t int_pin,
    uint8_t rst_pin,
    uint32_t report_period_ms = 5,
    uint32_t gyro_stale_ms = 100,
    uint32_t timeout_ms = 5000) -> bool;

  [[nodiscard]] auto configure() -> bool;

  [[nodiscard]] auto was_reset() -> bool;

  [[nodiscard]] auto reset() -> bool;

  [[nodiscard]] auto read() -> std::optional<Sample>;

  [[nodiscard]] auto start_calibration() -> bool;

  [[nodiscard]] auto stop_calibration() -> bool;

  [[nodiscard]] auto save_calibration() -> bool;

  [[nodiscard]] auto accuracy() -> SensorAccuracy;

  [[nodiscard]] auto calibrating() const -> bool;

  [[nodiscard]] auto calibration_mask() const -> uint8_t;

private:
  [[nodiscard]] auto set_reports() -> bool;

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
