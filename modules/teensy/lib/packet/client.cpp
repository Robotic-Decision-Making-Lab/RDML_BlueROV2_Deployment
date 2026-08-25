#include "client.h"

#include <array>
#include <cstring>

namespace packet
{

Client::Client(Stream & stream)
: stream_(stream)
{
}

auto Client::send_bytes(PacketId packet_id, DeviceId device_id, const uint8_t * data, size_t size) -> bool
{
  Packet packet{packet_id, device_id, {}, size};
  memcpy(packet.payload.data(), data, size);

  std::array<uint8_t, MAX_ENCODED_SIZE> out{};
  const ssize_t n = encode(packet, out.data(), out.size());
  if (n < 0) {
    return false;  // NOLINT
  }

  stream_.write(out.data(), static_cast<size_t>(n));
  return true;
}

auto Client::register_handler(CommandId command_id, CommandHandler handler) -> void
{
  const size_t index = static_cast<size_t>(command_id) - 1;  // command_ids are 1-indexed
  if (index < command_handlers_.size()) {
    command_handlers_[index] = handler;
  }
}

auto Client::poll_command() -> void
{
  while (stream_.available() > 0) {
    if (buffer_.push(static_cast<uint8_t>(stream_.read()))) {
      const std::optional<Packet> packet = decode(buffer_.data(), buffer_.size());
      buffer_.reset();
      if (packet.has_value()) {
        dispatch_command(*packet);
      }
      return;
    }
  }
}

auto Client::dispatch_command(const Packet & packet) -> void
{
  if (packet.packet_id != PacketId::COMMAND || packet.size != 3) {
    return;
  }

  const auto command_id = static_cast<CommandId>(packet.payload[0]);
  const CommandHandler handler = find_handler(command_id);
  const CommandResponse result = handler != nullptr ? handler() : CommandResponse::UNKNOWN_COMMAND;

  const std::array<uint8_t, 3> response{packet.payload[0], packet.payload[1], static_cast<uint8_t>(result)};
  send(PacketId::COMMAND_RESPONSE, packet.device_id, response);
}

auto Client::find_handler(CommandId command_id) const -> CommandHandler
{
  const size_t index = static_cast<size_t>(command_id) - 1;
  return index < command_handlers_.size() ? command_handlers_[index] : nullptr;
}

}  // namespace packet
