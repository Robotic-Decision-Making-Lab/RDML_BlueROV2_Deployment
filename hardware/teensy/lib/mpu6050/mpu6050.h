#ifndef _MPU6050_H_
#define _MPU6050_H_

#include <stddef.h>
#include <stdint.h>

// MPU6050 I2C address
#define MPU6050_ADDRESS 0x68

const double ACC_SCALE_FACTOR = 4096.0;
const double GYRO_SCALE_FACTOR = 131.0;

/// Initialize the MPU6050 sensor.
/// Make sure that you initialize the I2C bus before calling this function.
void mpu6050_init();

/// Read the accelerometer and gyroscope data from the MPU6050 sensor.
void read_mpu6050(double * ax, double * ay, double * az, double * gx, double * gy, double * gz);

#endif
