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

/// encodes/sends packets and dispatches incoming commands over a Stream
class Client
{
public:
  /// callback invoked when a registered command is received
  using CommandHandler = CommandResponse (*)();

  explicit Client(Stream & stream);

  /// encode and send payload as a packet with the given ids
  template <typename T>
  auto send(PacketId packet_id, DeviceId device_id, const T & payload) -> bool
  {
    static_assert(std::is_trivially_copyable_v<T>);
    static_assert(sizeof(T) <= MAX_PAYLOAD_SIZE);
    return send_bytes(packet_id, device_id, reinterpret_cast<const uint8_t *>(&payload), sizeof(T));
  }

  /// associate a handler with a command id, replacing any prior handler for it
  auto register_handler(CommandId command_id, CommandHandler handler) -> void;

  /// read available bytes, and dispatch a command if a complete packet arrives
  auto poll_command() -> void;

private:
  /// encode and write a packet's raw payload bytes to the stream
  [[nodiscard]] auto send_bytes(PacketId packet_id, DeviceId device_id, const uint8_t * data, size_t size) -> bool;

  /// run the handler registered for a decoded COMMAND packet and send its response
  auto dispatch_command(const Packet & packet) -> void;

  /// look up the handler registered for a command id, if any
  [[nodiscard]] auto find_handler(CommandId command_id) const -> CommandHandler;

  static constexpr size_t MAX_COMMAND_HANDLERS = 3;  // set to the number of CommandId values

  Stream & stream_;
  SerialBuffer buffer_;
  std::array<CommandHandler, MAX_COMMAND_HANDLERS> command_handlers_{};
};

}  // namespace packet

#endif
