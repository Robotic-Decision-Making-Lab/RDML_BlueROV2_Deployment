#ifndef CLIENT_H_
#define CLIENT_H_

#include <Arduino.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <type_traits>

#include "command.h"
#include "packet.h"
#include "serial_buffer.h"

namespace packet
{

class Client
{
public:
  using CommandHandler = CommandResponse (*)();

  explicit Client(Stream & stream);

  template <typename T>
  auto send(PacketId packet_id, DeviceId device_id, const T & payload) -> bool
  {
    static_assert(std::is_trivially_copyable_v<T>);
    static_assert(sizeof(T) <= MAX_PAYLOAD_SIZE);
    return send_bytes(packet_id, device_id, reinterpret_cast<const uint8_t *>(&payload), sizeof(T));
  }

  auto register_handler(CommandId command_id, CommandHandler handler) -> void;

  auto poll_command() -> void;

private:
  [[nodiscard]] auto send_bytes(PacketId packet_id, DeviceId device_id, const uint8_t * data, size_t size) -> bool;

  auto dispatch_command(const Packet & packet) -> void;

  [[nodiscard]] auto find_handler(CommandId command_id) const -> CommandHandler;

  static constexpr size_t MAX_COMMAND_HANDLERS = 3;  // set to the number of CommandId values

  Stream & stream_;
  SerialBuffer buffer_;
  std::array<CommandHandler, MAX_COMMAND_HANDLERS> command_handlers_{};
};

}  // namespace packet

#endif
