#pragma once

#include <cstdint>
#include <vector>

namespace teensy::protocol
{

auto decode_cobs(const std::vector<std::uint8_t> & packet) -> std::vector<std::uint8_t>;

auto encode_cobs(const std::vector<std::uint8_t> & data) -> std::vector<std::uint8_t>;

}  // namespace teensy::protocol
