#include <Arduino.h>
#include <SparkFun_BNO08x_Arduino_Library.h>
#include <micro_ros_platformio.h>
#include <rcl/error_handling.h>
#include <rcl/rcl.h>
#include <rclc/rclc.h>
#include <rmw_microros/rmw_microros.h>
#include <sensor_msgs/msg/imu.h>

const uint8_t BNO08X_ADDR = 0x4A;

const uint8_t BNO08X_INT_PIN = 2;
const uint8_t BNO08X_RST_PIN = 3;
const uint32_t IMU_RETRY_INTERVAL_MS = 1000;
const uint8_t BNO08X_MAX_EVENTS_PER_LOOP = 6;
const uint16_t BNO08X_PERIOD_MS = 100;

// agent connection
enum AgentState
{
  WAITING_AGENT,
  AGENT_AVAILABLE,
  AGENT_CONNECTED,
  AGENT_DISCONNECTED,
};
AgentState agent_state = WAITING_AGENT;

const int AGENT_PING_TIMEOUT_MS = 100;
const uint32_t AGENT_PING_INTERVAL_MS = 500;
uint32_t last_ping_ms = 0;

// time synchronization
const int SYNC_TIMEOUT_MS = 20;
const int SYNC_STARTUP_TIMEOUT_MS = 200;
const uint8_t SYNC_STARTUP_ATTEMPTS = 3;
const uint32_t SYNC_INTERVAL_MS = 10000;
uint32_t last_sync_ms = 0;

// max backward stamp jump allowed before we treat it as a real clock correction
const int64_t STAMP_RESET_THRESHOLD_NS = 1000000000LL;  // 1s
int64_t last_stamp_ns = 0;

// ROS entities
rclc_support_t support;
rcl_allocator_t allocator;
rcl_node_t node;

// publishers
rcl_publisher_t pub;

// messages
sensor_msgs__msg__Imu msg;

// IMU
BNO08x imu;
bool imu_ok = false;
uint32_t last_imu_retry_ms = 0;
bool have_accel = false;
bool have_gyro = false;
bool have_quat = false;
uint32_t sample_us = 0;

#define RCCHECK(fn)                                          \
  {                                                          \
    rcl_ret_t temp_rc = fn;                                  \
    if ((temp_rc != RCL_RET_OK)) {                           \
      Serial.print("A fatal error occurred in micro-ROS: "); \
      Serial.println((int32_t)temp_rc);                      \
      return false;                                          \
    }                                                        \
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

/// Enable the IMU reports.
void set_reports();

/// Poll the IMU for new sensor events, reconnecting it if needed.
void poll_imu();

/// Return the sample instant of the current IMU event in the local micros() timebase.
uint32_t imu_sample_micros();

/// Set the header stamp from a sample instant in the local micros() timebase;
/// returns false if unsynced.
bool set_message_timestamp(std_msgs__msg__Header * header, uint32_t sampled_us);

/// Publish the IMU message once accel, gyro, and orientation have all been refreshed by poll_imu() since the last
/// publish.
void publish_imu_msg();

/// Request a time synchronization with the agent.
void sync_time(int timeout_ms);

/// Create the micro-ROS node, publisher, and supporting entities.
bool create_entities();

/// Finalize the micro-ROS entities created by create_entities().
void destroy_entities();

/// Normalize the given quaternion.
void normalize_quat(float * q);

/// Setup function for the Teensy.
void setup()
{
  // USB serial connection for debugging
  Serial.begin(9600);

  // hardware serial connection to the Pi
  Serial1.begin(921600);
  set_microros_serial_transports(Serial1);

  // use SDA pin 18, SCL pin 19 with clock rate of 400 kHz
  Wire.begin();
  Wire.setClock(400000);

  Serial.println("Initializing BNO08x IMU");

  imu_ok = imu.begin(BNO08X_ADDR, Wire, BNO08X_INT_PIN, BNO08X_RST_PIN);
  if (imu_ok) {
    set_reports();
    Serial.println("BNO08x IMU initialized");
  } else {
    Serial.println("Failed to initialize the BNO08x IMU - will retry");
  }

  // pre-set static values in the IMU message
  static char frame_id[] = "bno08x";
  msg.header.frame_id.data = frame_id;
  msg.header.frame_id.size = strlen(frame_id);
  msg.header.frame_id.capacity = sizeof(frame_id);  // includes the NUL

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

  agent_state = WAITING_AGENT;
}

/// Main loop for the Teensy.
void loop()
{
  poll_imu();

  switch (agent_state) {
    case WAITING_AGENT: {
      if (millis() - last_ping_ms < AGENT_PING_INTERVAL_MS) {
        break;
      }
      last_ping_ms = millis();
      if (rmw_uros_ping_agent(AGENT_PING_TIMEOUT_MS, 1) == RMW_RET_OK) {
        agent_state = AGENT_AVAILABLE;
      }
      break;
    }

    case AGENT_AVAILABLE: {
      if (!create_entities()) {
        destroy_entities();
        agent_state = WAITING_AGENT;
        break;
      }

      // the epoch offset lives in the session, so it must be redone on every (re)connect
      for (uint8_t i = 0; i < SYNC_STARTUP_ATTEMPTS; i++) {
        sync_time(SYNC_STARTUP_TIMEOUT_MS);
        delay(20);
      }

      Serial.println("Connected to the micro-ROS agent");
      agent_state = AGENT_CONNECTED;
      break;
    }

    case AGENT_CONNECTED: {
      if (millis() - last_ping_ms >= AGENT_PING_INTERVAL_MS) {
        last_ping_ms = millis();
        if (rmw_uros_ping_agent(AGENT_PING_TIMEOUT_MS, 1) != RMW_RET_OK) {
          Serial.println("Lost connection to the micro-ROS agent");
          agent_state = AGENT_DISCONNECTED;
          break;
        }
      }

      if (millis() - last_sync_ms >= SYNC_INTERVAL_MS) {
        sync_time(SYNC_TIMEOUT_MS);
      }

      publish_imu_msg();
      break;
    }

    case AGENT_DISCONNECTED: {
      destroy_entities();
      agent_state = WAITING_AGENT;
      break;
    }
  }
}

void set_reports()
{
  imu.enableAccelerometer(BNO08X_PERIOD_MS);
  imu.enableGyro(BNO08X_PERIOD_MS);
  imu.enableRotationVector(BNO08X_PERIOD_MS);
}

void poll_imu()
{
  if (!imu_ok) {
    if (millis() - last_imu_retry_ms < IMU_RETRY_INTERVAL_MS) {
      return;
    }
    last_imu_retry_ms = millis();
    imu_ok = imu.begin(BNO08X_ADDR, Wire, BNO08X_INT_PIN, BNO08X_RST_PIN);
    if (!imu_ok) {
      Serial.println("Failed to initialize the BNO08x IMU - retrying");
      return;
    }
    set_reports();
    return;
  }

  if (imu.wasReset()) {
    set_reports();
  }

  for (uint8_t i = 0; i < BNO08X_MAX_EVENTS_PER_LOOP; i++) {
    if (!imu.getSensorEvent()) {
      break;
    }

    // each event refreshes exactly one field; the others keep their last value
    switch (imu.getSensorEventID()) {
      case SH2_ACCELEROMETER: {
        msg.linear_acceleration.x = imu.getAccelX();  // m/s^2
        msg.linear_acceleration.y = imu.getAccelY();  // m/s^2
        msg.linear_acceleration.z = imu.getAccelZ();  // m/s^2
        sample_us = imu_sample_micros();
        have_accel = true;
        break;
      }

      case SH2_GYROSCOPE_CALIBRATED: {
        msg.angular_velocity.x = imu.getGyroX();  // rad/s
        msg.angular_velocity.y = imu.getGyroY();  // rad/s
        msg.angular_velocity.z = imu.getGyroZ();  // rad/s
        sample_us = imu_sample_micros();
        have_gyro = true;
        break;
      }

      case SH2_ROTATION_VECTOR: {
        float q[4] = {imu.getQuatI(), imu.getQuatJ(), imu.getQuatK(), imu.getQuatReal()};
        normalize_quat(q);
        msg.orientation.x = q[0];
        msg.orientation.y = q[1];
        msg.orientation.z = q[2];
        msg.orientation.w = q[3];
        sample_us = imu_sample_micros();
        have_quat = true;
        break;
      }

      default:
        break;
    }
  }
}

uint32_t imu_sample_micros() { return (uint32_t)imu.getTimeStamp(); }

bool set_message_timestamp(std_msgs__msg__Header * header, uint32_t sampled_us)
{
  if (!rmw_uros_epoch_synchronized()) {
    return false;
  }

  const uint32_t age_us = micros() - sampled_us;

  int64_t epoch_ns = rmw_uros_epoch_nanos() - ((int64_t)age_us * 1000LL);
  if (epoch_ns + STAMP_RESET_THRESHOLD_NS < last_stamp_ns) {
    last_stamp_ns = 0;
  }
  if (epoch_ns <= last_stamp_ns) {
    epoch_ns = last_stamp_ns + 1000;
  }
  last_stamp_ns = epoch_ns;

  header->stamp.sec = (int32_t)(epoch_ns / 1000000000LL);
  header->stamp.nanosec = (uint32_t)(epoch_ns % 1000000000LL);

  return true;
}

void publish_imu_msg()
{
  if (!have_accel || !have_gyro || !have_quat) {
    return;
  }

  // clear before the sync check so a drop doesn't accumulate a stale batch
  have_accel = false;
  have_gyro = false;
  have_quat = false;

  if (!set_message_timestamp(&msg.header, sample_us)) {
    return;
  }

  RCSOFTCHECK(rcl_publish(&pub, &msg, NULL));
}

void sync_time(int timeout_ms)
{
  // set before the call so a failed sync cannot cause a retry storm
  last_sync_ms = millis();
  if (rmw_uros_sync_session(timeout_ms) != RMW_RET_OK) {
    Serial.println("Failed to synchronize the session time");
  }
}

bool create_entities()
{
  allocator = rcl_get_default_allocator();

  RCCHECK(rclc_support_init(&support, 0, NULL, &allocator));
  RCCHECK(rclc_node_init_default(&node, "micro_ros_teensy_node", "", &support));
  RCCHECK(rclc_publisher_init_default(&pub, &node, ROSIDL_GET_MSG_TYPE_SUPPORT(sensor_msgs, msg, Imu), "/bno08x/imu"));

  return true;
}

void destroy_entities()
{
  rmw_context_t * rmw_context = rcl_context_get_rmw_context(&support.context);
  (void)rmw_uros_set_context_entity_destroy_session_timeout(rmw_context, 0);

  RCSOFTCHECK(rcl_publisher_fini(&pub, &node));
  RCSOFTCHECK(rcl_node_fini(&node));
  RCSOFTCHECK(rclc_support_fini(&support));

  have_accel = false;
  have_gyro = false;
  have_quat = false;
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
