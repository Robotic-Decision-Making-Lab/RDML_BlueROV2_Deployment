#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <vector>

#include "device_id.hpp"
#include "packet_id.hpp"

namespace teensy::protocol
{

// MIRRORED IN modules/teensy/lib/packet/packet.h (packet::MAX_PAYLOAD_SIZE)
// keep in sync
inline constexpr std::size_t MAX_PAYLOAD_SIZE = 64;

struct Packet
{
  PacketId packet_id;
  DeviceId device_id;
  std::vector<std::uint8_t> payload;

  template <typename T>
  auto pop_front() -> T
  {
    static_assert(std::is_trivially_copyable_v<T>, "pop_front<T> requires a trivially copyable type");

    if (payload.size() < sizeof(T)) {
      throw std::invalid_argument("Not enough bytes remaining in the payload to extract the requested type.");
    }

    T value{};
    std::memcpy(&value, payload.data(), sizeof(T));
    payload.erase(payload.begin(), payload.begin() + sizeof(T));

    return value;
  }
};

auto decode_packet(const std::vector<std::uint8_t> & data) -> Packet;

auto decode_packets(const std::vector<std::uint8_t> & data)
  -> std::pair<std::vector<Packet>, std::vector<std::uint8_t>::const_iterator>;

auto encode_packet(PacketId packet_id, DeviceId device_id, const std::vector<std::uint8_t> & payload)
  -> std::vector<std::uint8_t>;

}  // namespace teensy::protocol
