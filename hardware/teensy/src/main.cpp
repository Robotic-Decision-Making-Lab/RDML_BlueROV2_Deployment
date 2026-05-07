#include <Arduino.h>
#include <SparkFun_BNO08x_Arduino_Library.h>
#include <buzzer.h>
#include <micro_ros_platformio.h>
#include <rcl/error_handling.h>
#include <rcl/rcl.h>
#include <rclc/executor.h>
#include <rclc/rclc.h>
#include <sensor_msgs/msg/imu.h>

// constants
const uint8_t BNO08X_ADDR = 0x4A;
const uint8_t BNO08X_INT_PIN = 2;
const uint8_t BNO08X_RST_PIN = 3;
const uint8_t BNO08X_CONNECTION_ATTEMPTS = 3;
const uint16_t BNO08X_PERIOD_MS = 100;  // this ends up running at ~90 Hz - no I don't understand why

const uint8_t TEENSY_BUZZER_PIN = 4;
const uint32_t TEENSY_STARTUP_DELAY_MS = 60000;  // 60s

const int SYNC_TIMEOUT_MS = 1000;
const int SYNC_TIMER_PERIOD_MS = 1000 > SYNC_TIMEOUT_MS ? 1000 : SYNC_TIMEOUT_MS;

// ROS entities
rclc_executor_t executor;
rclc_support_t support;
rcl_allocator_t allocator;
rcl_node_t node;

// publishers
rcl_publisher_t pub;

// messages
sensor_msgs__msg__Imu msg;

// timers
rcl_timer_t sync_timer;

// IMU
BNO08x imu;

// time synchronization
int64_t synced_time_ms = 0;
int64_t synced_time_ns = 0;
int64_t last_time_sync_ms = 0;
int64_t last_time_sync_ns = 0;

#define RCCHECK(fn)                 \
  {                                 \
    rcl_ret_t temp_rc = fn;         \
    if ((temp_rc != RCL_RET_OK)) {  \
      error_loop((int32_t)temp_rc); \
    }                               \
  }

#define RCSOFTCHECK(fn)                                          \
  {                                                              \
    rcl_ret_t temp_rc = fn;                                      \
    if ((temp_rc != RCL_RET_OK)) {                               \
      Serial.print("A non-fatal error occurred in micro-ROS: "); \
      Serial.print(temp_rc);                                     \
      Serial.println();                                          \
    }                                                            \
  }

/// Error loop for micro-ROS - this will loop indefinitely.
void error_loop(int32_t error_code);

/// Enable the IMU reports.
void set_reports();

/// Set the timestamp of the message.
void set_message_timestamp(std_msgs__msg__Header * header);

/// Populate and send the IMU message.
void update_imu_msg(sensor_msgs__msg__Imu * msg, uint8_t event_id);

/// Time synchronization callback.
void sync_timer_cb(rcl_timer_t * timer, int64_t last_call_time);

/// Normalize the given quaternion.
void normalize_quat(float * q);

/// Setup function for the Teensy.
void setup()
{
  // USB serial connection for debugging
  Serial.begin(9600);

  Serial.print("Delaying ");
  Serial.print(TEENSY_STARTUP_DELAY_MS);
  Serial.println("ms for setup...");

  // we need to wait for the pi, otherwise the Teensy will go straight to the error loop
  delay(TEENSY_STARTUP_DELAY_MS);

  Serial.println("Beginning setup");

  pinMode(TEENSY_BUZZER_PIN, OUTPUT);
  play_startup_melody(TEENSY_BUZZER_PIN);

  // hardware serial connection to the Pi
  Serial1.begin(921600);
  set_microros_serial_transports(Serial1);

  // use SDA pin 18, SCL pin 19 with clock rate of 400 kHz
  Wire.begin();
  Wire.setClock(400000);

  Serial.println("Initializing BNO08x IMU");

  // configure the IMU
  for (uint8_t i = 0; i < BNO08X_CONNECTION_ATTEMPTS; i++) {
    if (imu.begin(BNO08X_ADDR, Wire, BNO08X_INT_PIN, BNO08X_RST_PIN)) {
      break;
    }
    delay(100);
  }
  set_reports();

  Serial.println("BNO08x IMU initialized");
  Serial.println("Initializing micro-ROS");

  // configure the ROS interface
  allocator = rcl_get_default_allocator();
  executor = rclc_executor_get_zero_initialized_executor();

  RCCHECK(rclc_support_init(&support, 0, NULL, &allocator));
  RCCHECK(rclc_executor_init(&executor, &support.context, 1, &allocator));
  RCCHECK(rclc_node_init_default(&node, "micro_ros_teensy_node", "", &support));
  RCCHECK(rclc_publisher_init_default(&pub, &node, ROSIDL_GET_MSG_TYPE_SUPPORT(sensor_msgs, msg, Imu), "/bno08x/imu"));
  RCCHECK(rclc_timer_init_default2(&sync_timer, &support, RCL_MS_TO_NS(SYNC_TIMER_PERIOD_MS), sync_timer_cb, true));
  RCCHECK(rclc_executor_add_timer(&executor, &sync_timer));

  Serial.println("micro-ROS initialized");

  // perform an initial time synchronization
  rmw_uros_sync_session(SYNC_TIMEOUT_MS);
  if (rmw_uros_epoch_synchronized()) {
    synced_time_ms = rmw_uros_epoch_millis();
    synced_time_ns = rmw_uros_epoch_nanos();
  }

  // pre-set static values in the IMU message
  msg.header.frame_id.capacity = 100;
  msg.header.frame_id.data = (char *)malloc(10 * sizeof(char));
  msg.header.frame_id.size = 0;

  strcpy(msg.header.frame_id.data, "bno08x");
  msg.header.frame_id.size = strlen(msg.header.frame_id.data);

  // the following covariance values are estimated using 120s worth of recorded data at a rate of ~60 Hz
  const float orientation_cov = 0.0012;
  const float ang_vel_cov = 0.00005;
  const float lin_acc_cov = 0.0023;

  memset(msg.orientation_covariance, 0, sizeof(msg.orientation_covariance));
  msg.orientation_covariance[0] = orientation_cov;
  msg.orientation_covariance[4] = orientation_cov;
  msg.orientation_covariance[8] = orientation_cov;

  memset(msg.angular_velocity_covariance, 0, sizeof(msg.angular_velocity_covariance));
  msg.angular_velocity_covariance[0] = ang_vel_cov;
  msg.angular_velocity_covariance[4] = ang_vel_cov;
  msg.angular_velocity_covariance[8] = ang_vel_cov;

  memset(msg.linear_acceleration_covariance, 0, sizeof(msg.linear_acceleration_covariance));
  msg.linear_acceleration_covariance[0] = lin_acc_cov;
  msg.linear_acceleration_covariance[4] = lin_acc_cov;
  msg.linear_acceleration_covariance[8] = lin_acc_cov;

  // delay to allow the IMU to initialize
  delay(100);
}

/// Main loop for the Teensy.
void loop()
{
  rclc_executor_spin_some(&executor, RCL_MS_TO_NS(1));

  if (imu.wasReset()) {
    set_reports();
  }

  if (imu.getSensorEvent()) {
    const uint8_t event_id = imu.getSensorEventID();
    if (event_id == SH2_ROTATION_VECTOR || event_id == SH2_ACCELEROMETER || event_id == SH2_GYROSCOPE_CALIBRATED) {
      update_imu_msg(&msg, event_id);
      RCSOFTCHECK(rcl_publish(&pub, &msg, NULL));
    }
  }
}

void set_reports()
{
  imu.enableAccelerometer(BNO08X_PERIOD_MS);
  imu.enableGyro(BNO08X_PERIOD_MS);
  imu.enableRotationVector(BNO08X_PERIOD_MS);
}

void error_loop(int32_t error_code)
{
  while (true) {
    play_error_melody(TEENSY_BUZZER_PIN);
    Serial.print("Error occurred in micro-ROS: ");
    Serial.print(error_code);
    Serial.println();
    delay(500);
  }
}

void set_message_timestamp(std_msgs__msg__Header * header)
{
  header->stamp.sec = (synced_time_ms + millis() - last_time_sync_ms) / 1000;
  header->stamp.nanosec = synced_time_ns + (micros() * 1000 - last_time_sync_ns) % 1000000000;
}

void update_imu_msg(sensor_msgs__msg__Imu * msg, uint8_t event_id)
{
  set_message_timestamp(&msg->header);

  // update values based on the event ID
  // the read clears the buffer, so reading all data on each event will result in null data
  switch (event_id) {
    case SH2_ACCELEROMETER: {
      msg->linear_acceleration.x = imu.getAccelX();  // m/s^2
      msg->linear_acceleration.y = imu.getAccelY();  // m/s^2
      msg->linear_acceleration.z = imu.getAccelZ();  // m/s^2
      break;
    }

    case SH2_GYROSCOPE_CALIBRATED: {
      msg->angular_velocity.x = imu.getGyroX();  // rad/s
      msg->angular_velocity.y = imu.getGyroY();  // rad/s
      msg->angular_velocity.z = imu.getGyroZ();  // rad/s
      break;
    }

    case SH2_ROTATION_VECTOR: {
      float q[4] = {imu.getQuatI(), imu.getQuatJ(), imu.getQuatK(), imu.getQuatReal()};
      normalize_quat(q);
      msg->orientation.x = q[0];
      msg->orientation.y = q[1];
      msg->orientation.z = q[2];
      msg->orientation.w = q[3];
      break;
    }

    default:
      break;
  }
}

void sync_timer_cb(rcl_timer_t * timer, int64_t /*last_call_time*/)
{
  if (timer == nullptr) {
    return;
  }
  rmw_uros_sync_session(SYNC_TIMEOUT_MS);
  if (rmw_uros_epoch_synchronized()) {
    synced_time_ms = rmw_uros_epoch_millis();
    synced_time_ns = rmw_uros_epoch_nanos();
    last_time_sync_ms = millis();
    last_time_sync_ns = RCL_MS_TO_NS(last_time_sync_ms);
  }
}

void normalize_quat(float * q)
{
  float norm = sqrt((q[0] * q[0]) + (q[1] * q[1]) + (q[2] * q[2]) + (q[3] * q[3]));
  if (norm > 0.0F) {
    for (int i = 0; i < 4; i++) {
      q[i] /= norm;
    }
  }
}
