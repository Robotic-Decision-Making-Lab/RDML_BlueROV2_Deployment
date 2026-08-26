#pragma once

// MIRRORED IN modules/teensy/lib/packet/packet_id.h
// keep in sync

#include <cstdint>

namespace teensy::protocol
{

enum class PacketId : std::uint8_t
{
  IMU_DATA = 0x01,
  CAL_STATUS = 0x02,
  COMMAND = 0x10,
  COMMAND_RESPONSE = 0x11,
};

}  // namespace teensy::protocol
