#ifndef DEVICE_ID_H_
#define DEVICE_ID_H_

#include <stdint.h>

namespace packet
{

/// device identifier used to determine which sensor sent the data
enum class DeviceId : uint8_t
{
  IMU_01 = 0x01,
};

}  // namespace packet

#endif
