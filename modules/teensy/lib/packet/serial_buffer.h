#ifndef SERIAL_BUFFER_H_
#define SERIAL_BUFFER_H_

#include <stddef.h>
#include <stdint.h>

#include "packet.h"

namespace packet
{

class SerialBuffer
{
public:
  auto push(uint8_t byte) -> void;

  [[nodiscard]] auto ready() const -> bool;

  [[nodiscard]] auto data() const -> const uint8_t *;

  [[nodiscard]] auto size() const -> size_t;

  auto empty() -> void;

private:
  enum class State : uint8_t
  {
    READING,
    OVERFLOWED,
    READY,
  };

  uint8_t buffer_[MAX_ENCODED_SIZE];
  size_t len_{0};
  State state_{State::READING};
};

}  // namespace packet

#endif
