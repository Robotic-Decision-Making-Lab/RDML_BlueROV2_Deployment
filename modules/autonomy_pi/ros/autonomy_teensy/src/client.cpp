#include "autonomy_teensy/client.hpp"

#include <poll.h>

#include <array>
#include <boost/asio/buffer.hpp>
#include <boost/asio/serial_port_base.hpp>
#include <boost/asio/write.hpp>
#include <boost/system/error_code.hpp>
#include <ranges>
#include <stdexcept>
#include <utility>

namespace teensy::protocol
{

namespace
{

constexpr int POLL_TIMEOUT_MS = 100;

}  // namespace

Client::Client(const std::string & port, int baudrate)
: port_(io_context_, port)
{
  port_.set_option(boost::asio::serial_port_base::baud_rate(baudrate));
  port_.set_option(boost::asio::serial_port_base::character_size(8));
  port_.set_option(boost::asio::serial_port_base::parity(boost::asio::serial_port_base::parity::none));
  port_.set_option(boost::asio::serial_port_base::stop_bits(boost::asio::serial_port_base::stop_bits::one));
  port_.set_option(boost::asio::serial_port_base::flow_control(boost::asio::serial_port_base::flow_control::none));

  read_thread_ = std::thread(&Client::poll_connection, this);
}

Client::~Client()
{
  running_ = false;

  if (read_thread_.joinable()) {
    read_thread_.join();
  }
}

auto Client::register_callback(PacketId packet_id, std::function<void(const Packet &)> callback) -> void
{
  const std::lock_guard<std::mutex> lock(callbacks_lock_);
  callbacks_[packet_id].push_back(std::move(callback));
}

auto Client::send(const std::vector<std::uint8_t> & frame) -> bool
{
  if (!connected_) {
    return false;
  }

  const std::lock_guard<std::mutex> lock(write_lock_);

  boost::system::error_code ec;
  boost::asio::write(port_, boost::asio::buffer(frame), ec);

  if (ec) {
    connected_ = false;
    return false;
  }

  return true;
}

auto Client::connected() const -> bool { return connected_; }

auto Client::read_from_socket() -> std::vector<std::uint8_t>
{
  pollfd pfd{.fd = port_.native_handle(), .events = POLLIN, .revents = 0};
  const int rc = poll(&pfd, 1, POLL_TIMEOUT_MS);

  if (rc < 0) {
    throw std::runtime_error("Failed to poll the serial port.");
  }

  // timeout with no data available
  if (rc == 0 || (pfd.revents & POLLIN) == 0) {
    return {};
  }

  std::array<std::uint8_t, 256> data{};
  boost::system::error_code ec;
  const std::size_t n = port_.read_some(boost::asio::buffer(data), ec);

  if (ec) {
    throw std::runtime_error("Failed to read from the serial port: " + ec.message());
  }

  return {data.begin(), data.begin() + n};
}

auto Client::poll_connection() -> void
{
  while (running_) {
    std::vector<std::uint8_t> data;
    try {
      data = read_from_socket();
    }
    catch (const std::runtime_error &) {
      connected_ = false;
      break;  // we can't recover from I/O errors, so just break
    }

    if (data.empty()) {
      continue;
    }

    std::ranges::copy(data, std::back_inserter(accumulated_));

    auto [packets, stop] = decode_packets(accumulated_);
    accumulated_.erase(accumulated_.begin(), stop);

    for (const Packet & packet : packets) {
      dispatch(packet);
    }
  }
}

auto Client::dispatch(const Packet & packet) -> void
{
  std::vector<std::function<void(const Packet &)>> to_call;
  {
    const std::lock_guard<std::mutex> lock(callbacks_lock_);
    auto it = callbacks_.find(packet.packet_id);
    if (it == callbacks_.end()) {
      return;
    }
    to_call = it->second;
  }

  for (const auto & callback : to_call) {
    callback(packet);
  }
}

}  // namespace teensy::protocol
