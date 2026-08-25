#include "bno085.h"

#include <array>
#include <cmath>
#include <cstdint>

namespace
{

static const uint8_t CAL_MASK_CALIBRATING = SH2_CAL_ACCEL | SH2_CAL_GYRO | SH2_CAL_MAG;
static const uint8_t CAL_MASK_DEFAULT = SH2_CAL_GYRO;

auto normalize_quat(std::array<float, 4> & q) -> void
{
  const float norm = sqrtf((q[0] * q[0]) + (q[1] * q[1]) + (q[2] * q[2]) + (q[3] * q[3]));
  if (norm > 0.0F) {
    for (float & c : q) {
      c /= norm;
    }
  }
}

}  // namespace

auto BNO085::begin(
  uint8_t address,
  TwoWire & wire,
  uint8_t int_pin,
  uint8_t rst_pin,
  uint32_t report_period_ms,
  uint32_t gyro_stale_ms,
  uint32_t timeout_ms) -> bool
{
  report_period_ms_ = report_period_ms;
  gyro_stale_ms_ = gyro_stale_ms;

  const uint32_t start = millis();
  while (millis() - start < timeout_ms) {
    if (imu_.begin(address, wire, int_pin, rst_pin)) {
      return configure();
    }
    delay(100);
  }
  return false;
}

auto BNO085::configure() -> bool
{
  have_accel_ = false;
  have_quat_ = false;
  last_gyro_ms_ = 0;

  const bool ok = set_reports();
  delay(100);

  return ok;
}

auto BNO085::set_reports() -> bool
{
  const bool accel_ok = imu_.enableReport(SH2_ACCELEROMETER, report_period_ms_ * 1000);
  const bool gyro_ok = imu_.enableReport(SH2_GYROSCOPE_CALIBRATED, report_period_ms_ * 1000);
  const bool quat_ok = imu_.enableReport(SH2_ROTATION_VECTOR, report_period_ms_ * 1000);

  // ignore accel_ok - it doesn't seem to reflect the actual result
  return gyro_ok && quat_ok;
}

auto BNO085::was_reset() -> bool { return imu_.wasReset(); }

auto BNO085::reset() -> bool
{
  if (!configure()) {
    return false;
  }
  return calibrating() ? start_calibration() : stop_calibration();
}

auto BNO085::read() -> std::optional<Sample>
{
  if (imu_.getSensorEvent()) {
    switch (imu_.getSensorEventID()) {
      case SH2_ACCELEROMETER: {
        sample_.ax = imu_.getAccelX();
        sample_.ay = imu_.getAccelY();
        sample_.az = imu_.getAccelZ();
        accuracy_.accel = imu_.sensorValue.status & 0x03U;
        have_accel_ = true;
        break;
      }
      case SH2_GYROSCOPE_CALIBRATED: {
        sample_.gx = imu_.getGyroX();
        sample_.gy = imu_.getGyroY();
        sample_.gz = imu_.getGyroZ();
        accuracy_.gyro = imu_.sensorValue.status & 0x03U;
        last_gyro_ms_ = millis();
        break;
      }
      case SH2_MAGNETIC_FIELD_CALIBRATED: {
        accuracy_.mag = imu_.sensorValue.status & 0x03U;
        break;
      }
      case SH2_ROTATION_VECTOR: {
        std::array<float, 4> q{imu_.getQuatI(), imu_.getQuatJ(), imu_.getQuatK(), imu_.getQuatReal()};
        normalize_quat(q);
        sample_.qx = q[0];
        sample_.qy = q[1];
        sample_.qz = q[2];
        sample_.qw = q[3];
        accuracy_.quat = imu_.sensorValue.status & 0x03U;
        accuracy_.quat_rad = imu_.getQuatRadianAccuracy();
        have_quat_ = true;
        break;
      }
      default:
        break;
    }
  }

  if (!have_accel_ || !have_quat_) {
    return std::nullopt;
  }
  have_accel_ = false;
  have_quat_ = false;

  if (millis() - last_gyro_ms_ > gyro_stale_ms_) {
    sample_.gx = 0.0F;
    sample_.gy = 0.0F;
    sample_.gz = 0.0F;
  }

  return sample_;
}

auto BNO085::start_calibration() -> bool { return calibrate(CAL_MASK_CALIBRATING); }

auto BNO085::stop_calibration() -> bool { return calibrate(CAL_MASK_DEFAULT, 0); }

auto BNO085::calibrate(uint8_t sensors, uint32_t mag_report_period_ms) -> bool
{
  if (!imu_.enableReport(SH2_MAGNETIC_FIELD_CALIBRATED, mag_report_period_ms * 1000)) {
    return false;
  }
  if (!imu_.setCalibrationConfig(sensors)) {
    return false;
  }

  cal_mask_ = sensors;
  accuracy_.mag = 0;
  return true;
}

auto BNO085::save_calibration() -> bool { return imu_.saveCalibration(); }

auto BNO085::accuracy() -> SensorAccuracy { return accuracy_; }

auto BNO085::calibrating() const -> bool { return (cal_mask_ & SH2_CAL_MAG) != 0; }

auto BNO085::calibration_mask() const -> uint8_t { return cal_mask_; }
