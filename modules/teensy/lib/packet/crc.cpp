#include "crc.h"

namespace packet
{

auto calculate_crc(const uint8_t * data, size_t size) -> uint8_t
{
  uint8_t crc = 0x00;

  for (size_t i = 0; i < size; i++) {
    crc ^= data[i];
    for (uint8_t bit = 0; bit < 8; bit++) {
      if ((crc & 0x80) != 0) {
        crc = static_cast<uint8_t>((crc << 1) ^ 0x07);
      } else {
        crc = static_cast<uint8_t>(crc << 1);
      }
    }
  }

  return crc;
}

}  // namespace packet
