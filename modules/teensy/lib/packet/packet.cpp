#include "packet.h"

#include "cobs.h"
#include "crc.h"

namespace packet
{

namespace
{

constexpr size_t HEADER_SIZE = 2;
constexpr size_t CRC_SIZE = 1;
constexpr size_t MAX_FRAME_SIZE = HEADER_SIZE + MAX_PAYLOAD_SIZE + CRC_SIZE;

}  // namespace

auto encode(const Packet & packet, uint8_t * out, size_t out_size) -> ssize_t
{
  if (packet.size > MAX_PAYLOAD_SIZE || out_size == 0) {
    return -1;
  }

  uint8_t frame[MAX_FRAME_SIZE];
  frame[0] = static_cast<uint8_t>(packet.packet_id);
  frame[1] = static_cast<uint8_t>(packet.device_id);
  for (size_t i = 0; i < packet.size; i++) {
    frame[HEADER_SIZE + i] = packet.payload[i];
  }

  const size_t frame_size = HEADER_SIZE + packet.size;
  frame[frame_size] = calculate_crc(frame, frame_size);

  const ssize_t encoded_size = encode_cobs(frame, frame_size + CRC_SIZE, out, out_size - 1);
  if (encoded_size < 0) {
    return -1;
  }

  out[encoded_size] = 0x00;

  return encoded_size + 1;
}

}  // namespace packet
