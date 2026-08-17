#include "cobs.h"

namespace packet
{

auto encode_cobs(const uint8_t * data, size_t size, uint8_t * out, size_t out_size) -> ssize_t
{
  if (out_size < size + (size / 254) + 1) {
    return -1;
  }

  size_t write = 0;
  size_t code_index = write++;
  uint8_t code = 1;

  for (size_t read = 0; read < size; read++) {
    if (data[read] == 0) {
      out[code_index] = code;
      code_index = write++;
      code = 1;
      continue;
    }

    out[write++] = data[read];
    code++;

    // a block holds at most 254 data bytes; start a new one before code would overflow
    if (code == 0xFF) {
      out[code_index] = code;
      code_index = write++;
      code = 1;
    }
  }

  out[code_index] = code;

  return static_cast<ssize_t>(write);
}

}  // namespace packet
