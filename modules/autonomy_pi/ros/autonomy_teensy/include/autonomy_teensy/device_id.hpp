#pragma once

#include <cstdint>

namespace teensy::protocol
{

enum class DeviceId : std::uint8_t
{
  IMU_01 = 0x01,
};

}  // namespace teensy::protocol
