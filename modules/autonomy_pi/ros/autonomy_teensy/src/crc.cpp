#include "crc.hpp"

namespace teensy::protocol
{

auto calculate_crc(const std::vector<std::uint8_t> & packet) -> std::uint8_t
{
  std::uint8_t crc = 0x00;
  for (std::uint8_t data : packet) {
    crc ^= data;
    for (std::uint8_t bit = 0; bit < 8; bit++) {
      if ((crc & 0x80) != 0) {
        crc = static_cast<std::uint8_t>((crc << 1) ^ 0x07);
      } else {
        crc = static_cast<std::uint8_t>(crc << 1);
      }
    }
  }

  return crc;
}

}  // namespace teensy::protocol
