#ifndef TIMING_H_
#define TIMING_H_

#include <stdint.h>

static const uint32_t BNO08X_INIT_TIMEOUT_MS = 5000;
static const uint16_t BNO08X_REPORT_PERIOD_MS = 5;  // 200 Hz
static const uint32_t BNO08X_GYRO_STALE_MS = 100;

#endif
