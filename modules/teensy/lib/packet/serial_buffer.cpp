#include "serial_buffer.h"

namespace packet
{

auto SerialBuffer::push(uint8_t byte) -> void
{
  // a completed frame is waiting to be consumed - don't overwrite it until empty() is called
  if (state_ == State::READY) {
    return;
  }

  if (byte == 0x00) {
    if (state_ == State::OVERFLOWED || len_ == 0) {
      // an empty or overflowed frame carries no information - drop it and resync
      len_ = 0;
      state_ = State::READING;
      return;
    }
    state_ = State::READY;
    return;
  }

  if (len_ >= sizeof(buffer_)) {
    state_ = State::OVERFLOWED;
    return;
  }

  buffer_[len_++] = byte;
}

auto SerialBuffer::ready() const -> bool { return state_ == State::READY; }

auto SerialBuffer::data() const -> const uint8_t * { return buffer_; }

auto SerialBuffer::size() const -> size_t { return len_; }

auto SerialBuffer::empty() -> void
{
  len_ = 0;
  state_ = State::READING;
}

}  // namespace packet
