#include "serial_buffer.h"

namespace packet
{

auto SerialBuffer::push(uint8_t byte) -> bool
{
  if (byte == 0x00) {
    const bool have_frame = len_ > 0 && !overflowed_;
    if (!have_frame) {
      reset();
    }
    return have_frame;
  }

  if (len_ >= buffer_.size()) {
    overflowed_ = true;
    return false;
  }

  buffer_[len_++] = byte;
  return false;
}

auto SerialBuffer::data() const -> const uint8_t * { return buffer_.data(); }

auto SerialBuffer::size() const -> size_t { return len_; }

auto SerialBuffer::reset() -> void
{
  len_ = 0;
  overflowed_ = false;
}

}  // namespace packet
