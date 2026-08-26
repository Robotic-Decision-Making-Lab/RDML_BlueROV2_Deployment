#ifndef SERIAL_BUFFER_H_
#define SERIAL_BUFFER_H_

#include <stddef.h>
#include <stdint.h>

#include <array>

#include "packet.h"

namespace packet
{

/// accumulates incoming bytes into a single COBS-encoded frame
class SerialBuffer
{
public:
  /// append a byte, returning true once a complete, non-overflowed frame is buffered
  [[nodiscard]] auto push(uint8_t byte) -> bool;

  /// get a pointer to the buffered frame data, excluding the delimiter
  [[nodiscard]] auto data() const -> const uint8_t *;

  /// get the number of bytes currently buffered, excluding the delimiter
  [[nodiscard]] auto size() const -> size_t;

  /// clear the buffer to start accumulating a new frame
  auto reset() -> void;

private:
  std::array<uint8_t, MAX_ENCODED_SIZE> buffer_{};
  size_t len_{0};
  bool overflowed_{false};
};

}  // namespace packet

#endif
