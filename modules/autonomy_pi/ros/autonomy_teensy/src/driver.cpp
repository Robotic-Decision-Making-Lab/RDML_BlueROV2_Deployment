#include "autonomy_teensy/driver.hpp"

#include <chrono>
#include <rclcpp/create_publisher.hpp>
#include <vector>

#include "autonomy_teensy/client.hpp"
#include "autonomy_teensy/command.hpp"
#include "autonomy_teensy/packet.hpp"

namespace teensy
{

TeensyDriver::TeensyDriver(const rclcpp::NodeOptions & options)
: rclcpp_lifecycle::LifecycleNode("teensy_driver", options)
{
}

TeensyDriver::~TeensyDriver() = default;

auto TeensyDriver::on_configure(const rclcpp_lifecycle::State & /*previous_state*/) -> CallbackReturn
{
  RCLCPP_INFO(get_logger(), "Configuring the TeensyDriver");

  try {
    param_listener_ = std::make_shared<teensy_driver::ParamListener>(get_node_parameters_interface());
    params_ = param_listener_->get_params();
  }
  catch (const std::exception & e) {
    RCLCPP_ERROR(get_logger(), "Failed to get TeensyDriver parameters: %s", e.what());  // NOLINT
    return CallbackReturn::ERROR;
  }

  // the topic name is defined using the same identifier used by the Teensy
  //
  // in the future we might want to set the topic dynamically using the packets subscribed to
  imu_pub_ = create_publisher<sensor_msgs::msg::Imu>("imu01", rclcpp::SystemDefaultsQoS());

  try {
    client_ = std::make_unique<protocol::Client>(params_.port, static_cast<int>(params_.baudrate));
  }
  catch (const std::exception & e) {
    RCLCPP_ERROR(get_logger(), "Failed to open the Teensy serial connection: %s", e.what());  // NOLINT
    imu_pub_.reset();
    return CallbackReturn::ERROR;
  }

  client_->register_callback(protocol::PacketId::IMU_DATA, [this](protocol::Packet packet) {
    if (packet.payload.size() != 40) {
      RCLCPP_WARN_THROTTLE(
        get_logger(), *get_clock(), 5000, "Received an IMU_DATA packet with an unexpected payload size");
      return;
    }

    sensor_msgs::msg::Imu msg;
    msg.header.frame_id = params_.frame_id;
    msg.header.stamp = get_clock()->now();

    msg.orientation.x = packet.pop_front<float>();
    msg.orientation.y = packet.pop_front<float>();
    msg.orientation.z = packet.pop_front<float>();
    msg.orientation.w = packet.pop_front<float>();

    msg.angular_velocity.x = packet.pop_front<float>();
    msg.angular_velocity.y = packet.pop_front<float>();
    msg.angular_velocity.z = packet.pop_front<float>();

    msg.linear_acceleration.x = packet.pop_front<float>();
    msg.linear_acceleration.y = packet.pop_front<float>();
    msg.linear_acceleration.z = packet.pop_front<float>();

    for (std::size_t i = 0; i < 3; i++) {
      msg.orientation_covariance[i * 3 + i] = params_.orientation_covariance[i];
      msg.angular_velocity_covariance[i * 3 + i] = params_.angular_velocity_covariance[i];
      msg.linear_acceleration_covariance[i * 3 + i] = params_.linear_acceleration_covariance[i];
    }

    imu_pub_->publish(msg);
  });

  client_->register_callback(protocol::PacketId::CAL_STATUS, [this](protocol::Packet packet) {
    if (packet.payload.size() != 12) {
      RCLCPP_WARN_THROTTLE(
        get_logger(), *get_clock(), 5000, "Received a CAL_STATUS packet with an unexpected payload size");
      return;
    }

    autonomy_msgs::msg::ImuCalibrationStatus msg;
    msg.header.frame_id = params_.frame_id;
    msg.header.stamp = get_clock()->now();

    msg.state = packet.pop_front<std::uint8_t>();
    msg.accelerometer_accuracy = packet.pop_front<std::uint8_t>();
    msg.gyroscope_accuracy = packet.pop_front<std::uint8_t>();
    msg.magnetometer_accuracy = packet.pop_front<std::uint8_t>();
    msg.orientation_accuracy = packet.pop_front<std::uint8_t>();

    const std::uint8_t cal_config = packet.pop_front<std::uint8_t>();
    msg.accelerometer_calibration_enabled = (cal_config & 0x01) != 0;
    msg.gyroscope_calibration_enabled = (cal_config & 0x02) != 0;
    msg.magnetometer_calibration_enabled = (cal_config & 0x04) != 0;

    packet.pop_front<std::uint8_t>();  // last_result
    packet.pop_front<std::uint8_t>();  // reserved padding byte

    msg.orientation_accuracy_rad = packet.pop_front<float>();

    calib_pub_->publish(msg);
  });

  client_->register_callback(protocol::PacketId::COMMAND_RESPONSE, [this](protocol::Packet packet) {
    if (packet.payload.size() != 3) {
      RCLCPP_WARN_THROTTLE(
        get_logger(), *get_clock(), 5000, "Received a COMMAND_RESPONSE packet with an unexpected payload size");
      return;
    }

    // a response whose seq doesn't match the attempt currently being waited on is stale - it
    // belongs to an earlier, already-abandoned retry, and is discarded
    const std::lock_guard<std::mutex> lock(command_lock_);
    if (pending_promise_.has_value() && packet.payload[1] == pending_seq_) {
      pending_promise_->set_value(std::array<std::uint8_t, 3>{packet.payload[0], packet.payload[1], packet.payload[2]});
      pending_promise_.reset();
    }
  });

  calib_pub_ = rclcpp::create_publisher<autonomy_msgs::msg::ImuCalibrationStatus>(
    *this, "~/calibration_status", rclcpp::SystemDefaultsQoS());

  start_cal_srv_ = create_service<std_srvs::srv::Trigger>(
    "~/start_calibration",
    [this](const std_srvs::srv::Trigger::Request::SharedPtr, std_srvs::srv::Trigger::Response::SharedPtr response) {
      response->success = send_command(protocol::CommandId::CALIBRATION_START, 0, response->message);
    });

  save_cal_srv_ = create_service<std_srvs::srv::Trigger>(
    "~/save_calibration",
    [this](const std_srvs::srv::Trigger::Request::SharedPtr, std_srvs::srv::Trigger::Response::SharedPtr response) {
      response->success = send_command(protocol::CommandId::CALIBRATION_SAVE, 0, response->message);
    });

  stop_cal_srv_ = create_service<std_srvs::srv::Trigger>(
    "~/stop_calibration",
    [this](const std_srvs::srv::Trigger::Request::SharedPtr, std_srvs::srv::Trigger::Response::SharedPtr response) {
      response->success = send_command(protocol::CommandId::CALIBRATION_STOP, 0, response->message);
    });

  RCLCPP_INFO(get_logger(), "TeensyDriver configured successfully");
  return CallbackReturn::SUCCESS;
}

auto TeensyDriver::on_activate(const rclcpp_lifecycle::State & /*previous_state*/) -> CallbackReturn
{
  RCLCPP_INFO(get_logger(), "Activating the TeensyDriver");
  imu_pub_->on_activate();
  return CallbackReturn::SUCCESS;
}

auto TeensyDriver::on_deactivate(const rclcpp_lifecycle::State & /*previous_state*/) -> CallbackReturn
{
  RCLCPP_INFO(get_logger(), "Deactivating the TeensyDriver");
  imu_pub_->on_deactivate();
  return CallbackReturn::SUCCESS;
}

auto TeensyDriver::on_cleanup(const rclcpp_lifecycle::State & /*previous_state*/) -> CallbackReturn
{
  RCLCPP_INFO(get_logger(), "Cleaning up the TeensyDriver");

  client_.reset();
  imu_pub_.reset();
  calib_pub_.reset();
  start_cal_srv_.reset();
  save_cal_srv_.reset();
  stop_cal_srv_.reset();
  param_listener_.reset();

  {
    const std::lock_guard<std::mutex> lock(command_lock_);
    pending_promise_.reset();
    pending_seq_ = 0;
    command_seq_ = 0;
  }

  return CallbackReturn::SUCCESS;
}

auto TeensyDriver::send_command(protocol::CommandId command_id, std::uint8_t arg, std::string & message) -> bool
{
  for (int attempt = 0; attempt < params_.command_retries; attempt++) {
    std::future<std::array<std::uint8_t, 3>> future;
    std::uint8_t seq;

    {
      const std::lock_guard<std::mutex> lock(command_lock_);
      seq = ++command_seq_;
      pending_seq_ = seq;
      pending_promise_.emplace();
      future = pending_promise_->get_future();
    }

    std::vector<std::uint8_t> frame;
    try {
      frame = protocol::encode_packet(
        protocol::PacketId::COMMAND, protocol::DeviceId::IMU_01, {static_cast<std::uint8_t>(command_id), seq, arg});
    }
    catch (const std::invalid_argument & e) {
      message = std::string("Failed to encode the command: ") + e.what();
      return false;
    }

    if (!client_->send(frame)) {
      message = "The serial link to the Teensy is down";
      return false;
    }

    if (future.wait_for(std::chrono::duration<double>(params_.command_timeout)) != std::future_status::ready) {
      continue;
    }

    switch (static_cast<protocol::CommandResponse>(future.get()[2])) {
      case protocol::CommandResponse::OK:
        message = "Command executed successfully";
        return true;
      case protocol::CommandResponse::UNKNOWN_COMMAND:
        message = "The Teensy did not recognize the command";
        return false;
      case protocol::CommandResponse::SENSOR_ERROR:
        message = "The Teensy reported a sensor error while executing the command";
        return false;
      case protocol::CommandResponse::INVALID_STATE:
        message = "The command is not valid in the Teensy's current calibration state";
        return false;
      default:
        message = "The Teensy returned an unrecognized result code";
        return false;
    }
  }

  message = "Timed out waiting for a response from the Teensy";
  return false;
}

}  // namespace teensy
