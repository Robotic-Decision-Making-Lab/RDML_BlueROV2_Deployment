#ifndef PACKET_ID_H_
#define PACKET_ID_H_

// MIRRORED IN modules/autonomy_pi/ros/autonomy_teensy/include/autonomy_teensy/packet_id.hpp
// keep in sync

#include <stdint.h>

namespace packet
{

/// identifier for the packet payload data
enum class PacketId : uint8_t
{
  IMU_DATA = 0x01,          // Teensy -> Pi
  CAL_STATUS = 0x02,        // Teensy -> Pi
  COMMAND = 0x10,           // Pi -> Teensy
  COMMAND_RESPONSE = 0x11,  // Teensy -> Pi
};

}  // namespace packet

#endif
