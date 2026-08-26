#include "cobs.hpp"

#include <stdexcept>

namespace teensy::protocol
{

auto decode_cobs(const std::vector<std::uint8_t> & packet) -> std::vector<std::uint8_t>
{
  std::vector<std::uint8_t> decoded_data;
  decoded_data.reserve(packet.size());

  for (std::size_t i = 0; i < packet.size();) {
    const std::uint8_t byte = packet[i++];

    if (byte == 0x00) {
      throw std::invalid_argument("Encountered a 0x00 byte inside the frame");
    }

    for (std::uint8_t n = 1; n < byte; n++) {
      if (i >= packet.size()) {
        throw std::invalid_argument("Code byte claims more data bytes than are present");
      }
      decoded_data.push_back(packet[i++]);
    }

    if (byte != 0xFF && i < packet.size()) {
      decoded_data.push_back(0x00);
    }
  }

  return decoded_data;
}

auto encode_cobs(const std::vector<std::uint8_t> & data) -> std::vector<std::uint8_t>
{
  std::vector<std::uint8_t> out(data.size() + (data.size() / 254) + 1);

  std::size_t write = 0;
  std::size_t code_index = write++;
  std::uint8_t code = 1;

  for (std::uint8_t byte : data) {
    if (byte == 0x00) {
      out[code_index] = code;
      code_index = write++;
      code = 1;
      continue;
    }

    out[write++] = byte;
    code++;

    // a block holds at most 254 data bytes; start a new one before code would overflow
    if (code == 0xFF) {
      out[code_index] = code;
      code_index = write++;
      code = 1;
    }
  }

  out[code_index] = code;
  out.resize(write);

  return out;
}

}  // namespace teensy::protocol
