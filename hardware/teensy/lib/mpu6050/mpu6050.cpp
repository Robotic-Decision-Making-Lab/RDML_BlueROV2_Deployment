#include "mpu6050.h"

#include <Wire.h>

void mpu6050_init()
{
  Wire.beginTransmission(MPU6050_ADDRESS);
  Wire.write(0x6B);  // PWR_MGMT_1 register
  Wire.write(0x00);  // set to zero (wakes up the MPU-6050)
  Wire.endTransmission(true);

  // Set the accel sensitivity to +/- 8g
  Wire.beginTransmission(MPU6050_ADDRESS);
  Wire.write(0x1C);
  Wire.write(0x10);
  Wire.endTransmission(true);

  // Set the DLPF to 94 Hz
  // TODO(evan-palmer): do we need this?
  Wire.beginTransmission(MPU6050_ADDRESS);
  Wire.write(0x1A);
  Wire.write(0x02);
  Wire.endTransmission(true);

  // Enable the data-ready interrupt
  Wire.beginTransmission(MPU6050_ADDRESS);
  Wire.write(0x38);  // INT_PIN_CFG register
  Wire.write(0x01);  // set to 0x01 (enables the interrupt pin)
  Wire.endTransmission(true);
}

void read_mpu6050(double * ax, double * ay, double * az, double * gx, double * gy, double * gz)
{
  Wire.beginTransmission(MPU6050_ADDRESS);
  Wire.write(0x3B);  // starting with register 0x3B (ACCEL_XOUT_H)
  Wire.endTransmission(false);
  Wire.requestFrom(MPU6050_ADDRESS, 14, true);  // request a total of 14 registers

  int16_t ax_raw, ay_raw, az_raw, temp_raw, gx_raw, gy_raw, gz_raw;  // NOLINT

  ax_raw = Wire.read() << 8 | Wire.read();
  ay_raw = Wire.read() << 8 | Wire.read();
  az_raw = Wire.read() << 8 | Wire.read();
  temp_raw = Wire.read() << 8 | Wire.read();
  gx_raw = Wire.read() << 8 | Wire.read();
  gy_raw = Wire.read() << 8 | Wire.read();
  gz_raw = Wire.read() << 8 | Wire.read();

  // Convert to doubles and scale
  *ax = (double)(ax_raw) / ACC_SCALE_FACTOR;
  *ay = (double)(ay_raw) / ACC_SCALE_FACTOR;
  *az = (double)(az_raw) / ACC_SCALE_FACTOR;
  *gx = (double)(gx_raw) / GYRO_SCALE_FACTOR;
  *gy = (double)(gy_raw) / GYRO_SCALE_FACTOR;
  *gz = (double)(gz_raw) / GYRO_SCALE_FACTOR;
}
