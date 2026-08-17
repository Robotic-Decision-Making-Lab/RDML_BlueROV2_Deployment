#pragma once

#include <cstdint>
#include <vector>

namespace teensy::protocol
{

/// Compute the CRC-8 checksum of data using:
///  - poly 0x07
///  - init 0x00
///  - no reflection
///  - no final XOR.
/// check("123456789") == 0xF4.
[[nodiscard]] auto calculate_crc(const std::vector<std::uint8_t> & packet) -> std::uint8_t;

}  // namespace teensy::protocol
