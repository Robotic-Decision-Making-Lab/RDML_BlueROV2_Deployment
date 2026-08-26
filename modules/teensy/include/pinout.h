#ifndef PINOUT_H_
#define PINOUT_H_

#include <stdint.h>

/// I2C address of the BNO08x IMU
static const uint8_t BNO08X_ADDR = 0x4A;
/// interrupt pin connected to the BNO08x IMU
static const uint8_t BNO08X_INT_PIN = 2;
/// reset pin connected to the BNO08x IMU
static const uint8_t BNO08X_RST_PIN = 3;

#endif
