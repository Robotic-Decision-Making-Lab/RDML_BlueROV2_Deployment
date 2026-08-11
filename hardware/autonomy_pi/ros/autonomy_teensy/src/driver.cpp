#include "autonomy_teensy/driver.hpp"

#include "autonomy_teensy/client.hpp"
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
    sensor_msgs::msg::Imu msg;
    msg.header.frame_id = params_.frame_id;
    msg.header.stamp = get_clock()->now();

    try {
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
    }
    catch (const std::invalid_argument &) {
      RCLCPP_WARN_THROTTLE(
        get_logger(), *get_clock(), 5000, "Received an IMU_DATA packet with an unexpected payload size");
      return;
    }

    for (std::size_t i = 0; i < 3; i++) {
      msg.orientation_covariance[i * 3 + i] = params_.orientation_covariance[i];
      msg.angular_velocity_covariance[i * 3 + i] = params_.angular_velocity_covariance[i];
      msg.linear_acceleration_covariance[i * 3 + i] = params_.linear_acceleration_covariance[i];
    }

    imu_pub_->publish(msg);
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
  param_listener_.reset();

  return CallbackReturn::SUCCESS;
}

}  // namespace teensy
