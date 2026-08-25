#ifndef SERIAL_BUFFER_H_
#define SERIAL_BUFFER_H_

#include <stddef.h>
#include <stdint.h>

#include <array>

#include "packet.h"

namespace packet
{

class SerialBuffer
{
public:
  [[nodiscard]] auto push(uint8_t byte) -> bool;

  [[nodiscard]] auto data() const -> const uint8_t *;

  [[nodiscard]] auto size() const -> size_t;

  auto reset() -> void;

private:
  std::array<uint8_t, MAX_ENCODED_SIZE> buffer_{};
  size_t len_{0};
  bool overflowed_{false};
};

}  // namespace packet

#endif
