#ifndef CRC_H_
#define CRC_H_

#include <stddef.h>
#include <stdint.h>

namespace packet
{

/// Compute the CRC-8 checksum of data using:
///  - poly 0x07
///  - init 0x00
///  - no reflection
///  - no final XOR.
/// check("123456789") == 0xF4.
[[nodiscard]] auto calculate_crc(const uint8_t * data, size_t size) -> uint8_t;

}  // namespace packet

#endif
