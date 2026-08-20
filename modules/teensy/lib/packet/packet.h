#ifndef PACKET_H_
#define PACKET_H_

#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>

#include <optional>

#include "device_id.h"
#include "packet_id.h"

namespace packet
{

/// Largest payload a single packet can carry.
constexpr size_t MAX_PAYLOAD_SIZE = 64;

/// Buffer size a caller must provide to encode(), worst case: 2 header bytes + payload + 1 CRC
/// byte, plus one COBS code byte per started 254-byte block, plus the delimiter.
constexpr size_t MAX_ENCODED_SIZE = (MAX_PAYLOAD_SIZE + 3) + ((MAX_PAYLOAD_SIZE + 3) / 254) + 2;

/// Data structure representing a block of data to be streamed. `payload` is embedded storage, not
/// a view into memory owned elsewhere, so a Packet is self-contained and can be freely passed
/// around, encoded, or decoded into without the caller managing a separate buffer.
struct Packet
{
  PacketId packet_id;
  DeviceId device_id;
  uint8_t payload[MAX_PAYLOAD_SIZE];
  size_t size;
};

/// Encode a packet into a byte-stream to be sent over the serial line.
///
/// This returns the number of bytes written or -1 on error (e.g., the payload is too large, or
/// out is too small to hold the encoded result).
[[nodiscard]] auto encode(const Packet & packet, uint8_t * out, size_t out_size) -> ssize_t;

/// Decode a COBS-encoded, CRC-checked frame into a packet.
///
/// `data` should not include the trailing 0x00 frame delimiter. Returns std::nullopt if the frame
/// is malformed (e.g., COBS decode failure, too short to contain a header and CRC, or a CRC mismatch).
[[nodiscard]] auto decode(const uint8_t * data, size_t size) -> std::optional<Packet>;

}  // namespace packet

#endif
