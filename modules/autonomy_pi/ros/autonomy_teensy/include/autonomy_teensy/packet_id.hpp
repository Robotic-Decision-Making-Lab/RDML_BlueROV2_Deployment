#pragma once

#include <cstdint>

namespace teensy::protocol
{

enum class PacketId : std::uint8_t
{
  IMU_DATA = 0x01,
};

}  // namespace teensy::protocol
