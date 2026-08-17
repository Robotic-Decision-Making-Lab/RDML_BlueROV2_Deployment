#pragma once

#include <atomic>
#include <boost/asio/io_context.hpp>
#include <boost/asio/serial_port.hpp>
#include <cstdint>
#include <functional>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

#include "packet.hpp"

namespace teensy::protocol
{

class Client
{
public:
  Client(const std::string & port, int baudrate);

  ~Client();

  auto register_callback(PacketId packet_id, std::function<void(const Packet &)> callback) -> void;

private:
  auto poll_connection() -> void;

  auto read_from_socket() -> std::vector<std::uint8_t>;

  auto dispatch(const Packet & packet) -> void;

  boost::asio::io_context io_context_;
  boost::asio::serial_port port_;

  std::thread read_thread_;
  std::atomic<bool> running_{true};

  std::vector<std::uint8_t> accumulated_;

  std::unordered_map<PacketId, std::vector<std::function<void(const Packet &)>>> callbacks_;
  std::mutex callbacks_lock_;
};

}  // namespace teensy::protocol
