#ifndef TIMING_H_
#define TIMING_H_

#include <stdint.h>

static const uint32_t BNO08X_INIT_TIMEOUT_MS = 5000;
static const uint16_t BNO08X_REPORT_PERIOD_MS = 5;  // 200 Hz
static const uint32_t BNO08X_GYRO_STALE_MS = 100;

static const uint32_t BNO08X_CAL_REPORT_PERIOD_US = 20000;  // 50 Hz, while calibrating
static const uint32_t CAL_STATUS_PERIOD_MS = 100;           // 10 Hz, while calibrating
static const int SERIAL_RX_BYTES_PER_LOOP = 32;

#endif
