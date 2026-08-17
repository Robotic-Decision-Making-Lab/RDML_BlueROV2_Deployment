#ifndef COBS_H_
#define COBS_H_

#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>

namespace packet
{

/// Encode the data using COBS encoding.
///
/// This returns the number of bytes written or -1 on error (e.g., the arguments are invalid).
[[nodiscard]] auto encode_cobs(const uint8_t * data, size_t size, uint8_t * out, size_t out_size) -> ssize_t;

}  // namespace packet

#endif
