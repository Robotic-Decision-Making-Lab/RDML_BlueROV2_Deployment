#ifndef PACKET_ID_H_
#define PACKET_ID_H_

#include <stdint.h>

namespace packet
{

/// identifier for the packet payload data
enum class PacketId : uint8_t
{
  IMU_DATA = 0x01,
};

}  // namespace packet

#endif
