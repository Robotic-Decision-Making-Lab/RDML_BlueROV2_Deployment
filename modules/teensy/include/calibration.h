#ifndef CALIBRATION_H_
#define CALIBRATION_H_

#include <SparkFun_BNO08x_Arduino_Library.h>
#include <stdint.h>

enum class CalibrationState : uint8_t
{
  IDLE = 0,
  CALIBRATING = 1,
};

struct CalibrationStatus
{
  uint8_t state;
  uint8_t accel_accuracy;
  uint8_t gyro_accuracy;
  uint8_t mag_accuracy;
  uint8_t quat_accuracy;
  uint8_t cal_config;
  uint8_t last_result;
  uint8_t reserved;
  float quat_rad_accuracy;
};

static const uint8_t CAL_MASK_CALIBRATING = SH2_CAL_ACCEL | SH2_CAL_GYRO | SH2_CAL_MAG;

static const uint8_t CAL_MASK_NORMAL = SH2_CAL_GYRO;

#endif
