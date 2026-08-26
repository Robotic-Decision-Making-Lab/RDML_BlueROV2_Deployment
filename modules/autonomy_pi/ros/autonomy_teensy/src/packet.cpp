#include "autonomy_teensy/packet.hpp"

#include <algorithm>
#include <cstddef>
#include <stdexcept>

#include "cobs.hpp"
#include "crc.hpp"

namespace teensy::protocol
{

namespace
{

constexpr std::uint8_t PACKET_DELIMITER = 0x00;
constexpr std::size_t HEADER_SIZE = 2;
constexpr std::size_t CRC_SIZE = 1;

}  // namespace

auto decode_packet(const std::vector<std::uint8_t> & data) -> Packet
{
  std::vector<std::uint8_t> decoded_data = decode_cobs(data);
  if (decoded_data.size() < HEADER_SIZE + CRC_SIZE) {
    throw std::invalid_argument("The decoded data is too short to parse.");
  }

  const std::size_t payload_size = decoded_data.size() - CRC_SIZE;
  const std::vector<std::uint8_t> body(decoded_data.begin(), decoded_data.begin() + payload_size);
  const std::uint8_t expected_crc = calculate_crc(body);
  const std::uint8_t actual_crc = decoded_data[payload_size];

  if (actual_crc != expected_crc) {
    throw std::invalid_argument("The expected CRC value does not match the true CRC value.");
  }

  auto packet_id = static_cast<PacketId>(decoded_data[0]);
  auto device_id = static_cast<DeviceId>(decoded_data[1]);
  std::vector<std::uint8_t> payload(decoded_data.begin() + HEADER_SIZE, decoded_data.begin() + payload_size);

  return {packet_id, device_id, payload};
}

auto decode_packets(const std::vector<std::uint8_t> & data)
  -> std::pair<std::vector<Packet>, std::vector<std::uint8_t>::const_iterator>
{
  std::vector<Packet> packets;

  auto start = data.begin();
  auto delimiter = std::find(start, data.end(), PACKET_DELIMITER);

  while (delimiter != data.end()) {
    const std::vector<std::uint8_t> packet_data(start, delimiter);

    start = delimiter + 1;
    delimiter = std::find(start, data.end(), PACKET_DELIMITER);

    if (packet_data.empty()) {
      continue;
    }

    try {
      packets.push_back(decode_packet(packet_data));
    }
    catch (const std::invalid_argument &) {  // NOLINT
      // a malformed frame shouldn't stop the rest of the batch from being decoded - the stream
      // resynchronizes on the next delimiter
    }
  }

  return {packets, start};
}

auto encode_packet(PacketId packet_id, DeviceId device_id, const std::vector<std::uint8_t> & payload)
  -> std::vector<std::uint8_t>
{
  if (payload.size() > MAX_PAYLOAD_SIZE) {
    throw std::invalid_argument("The payload exceeds the maximum payload size.");
  }

  std::vector<std::uint8_t> body;
  body.reserve(HEADER_SIZE + payload.size());
  body.push_back(static_cast<std::uint8_t>(packet_id));
  body.push_back(static_cast<std::uint8_t>(device_id));
  body.insert(body.end(), payload.begin(), payload.end());
  body.push_back(calculate_crc(body));

  std::vector<std::uint8_t> frame = encode_cobs(body);
  frame.push_back(PACKET_DELIMITER);

  return frame;
}

}  // namespace teensy::protocol
