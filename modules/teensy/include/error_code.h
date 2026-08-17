#ifndef ERROR_CODE_H_
#define ERROR_CODE_H_

#include <stdint.h>

/// error codes reported by error_loop() when setup() fails
enum class ErrorCode : uint8_t
{
  IMU_INIT_FAILED = 0x01,
};

#endif
