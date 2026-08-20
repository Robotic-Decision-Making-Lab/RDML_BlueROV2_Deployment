#include <Arduino.h>
#include <SparkFun_BNO08x_Arduino_Library.h>
#include <stdint.h>
#include <string.h>

#include "calibration.h"
#include "command.h"
#include "error_code.h"
#include "packet.h"
#include "pinout.h"
#include "sample.h"
#include "serial_buffer.h"
#include "timing.h"

// IMU
BNO08x imu;
ImuSample imu_sample;

bool have_accel = false;
bool have_quat = false;
uint32_t last_gyro_ms = 0;

// calibration
CalibrationState cal_state = CalibrationState::IDLE;
CalibrationStatus cal_status{};
packet::SerialBuffer serial_buf;
uint32_t last_cal_status_ms = 0;

/// Error loop when the setup function fails. This will loop indefinitely.
void error_loop(ErrorCode error_code);

/// Configure the IMU. This will set the reports, calibration settings, etc.
void configure();

/// Normalize the quaternions before broadcasting to give users a proper invariant.
void normalize_quat(float * q);

/// Decode a received frame and dispatch it to the appropriate handler.
void decode_frame(const packet::SerialBuffer & buf);

/// Validate and execute a decoded COMMAND packet, replying with a COMMAND_RESPONSE.
void handle_command(const packet::Packet & command);

/// Encode a packet and write it to the Pi.
void send_packet(packet::PacketId packet_id, const void * payload, size_t size);

/// Send a COMMAND_RESPONSE for the given command/seq.
void send_command_response(uint8_t command_id, uint8_t seq, packet::CommandResponse result);

/// Send the current calibration status to the Pi.
void send_cal_status();

void setup()
{
  Serial.begin(9600);     // USB serial connection for debugging
  Serial1.begin(921600);  // hardware serial connection to the Pi

  // use SDA pin 18, SCL pin 19, clocked at 1 MHz (BNO08x supports Fast Mode Plus)
  // for the quick connectors, this corresponds to blue: pin 18 and yellow: pin 19
  Wire.begin();
  Wire.setClock(1000000);

  Serial.println("Initializing BNO08x IMU...");

  bool imu_ready = false;

  uint32_t start = millis();
  while (millis() - start < BNO08X_INIT_TIMEOUT_MS) {
    imu_ready = imu.begin(BNO08X_ADDR, Wire, BNO08X_INT_PIN, BNO08X_RST_PIN);
    if (imu_ready) {
      configure();
      Serial.println("BNO08x IMU initialized");
      break;
    }
    delay(100);
  }

  // only proceed to the loop when we have a sensor invariant
  if (!imu_ready) {
    error_loop(ErrorCode::IMU_INIT_FAILED);
  }

  Serial.println("Teensy successfully initialized.");
}

void loop()
{
  // bounded so a flood on RX can never stall the 200 Hz sensor pump
  for (int i = 0; i < SERIAL_RX_BYTES_PER_LOOP && Serial1.available() > 0; i++) {
    serial_buf.push(static_cast<uint8_t>(Serial1.read()));
    if (serial_buf.ready()) {
      decode_frame(serial_buf);
      serial_buf.empty();
    }
  }

  if (imu.wasReset()) {
    configure();
  }

  if (imu.getSensorEvent()) {
    const uint8_t event_id = imu.getSensorEventID();
    switch (event_id) {
      case SH2_ACCELEROMETER: {
        imu_sample.ax = imu.getAccelX();
        imu_sample.ay = imu.getAccelY();
        imu_sample.az = imu.getAccelZ();
        cal_status.accel_accuracy = imu.sensorValue.status & 0x03;
        have_accel = true;
        break;
      }
      case SH2_GYROSCOPE_CALIBRATED: {
        imu_sample.gx = imu.getGyroX();
        imu_sample.gy = imu.getGyroY();
        imu_sample.gz = imu.getGyroZ();
        cal_status.gyro_accuracy = imu.sensorValue.status & 0x03;
        last_gyro_ms = millis();
        break;
      }
      case SH2_MAGNETIC_FIELD_CALIBRATED: {
        cal_status.mag_accuracy = imu.sensorValue.status & 0x03;
        break;
      }
      case SH2_ROTATION_VECTOR: {
        float q[4] = {imu.getQuatI(), imu.getQuatJ(), imu.getQuatK(), imu.getQuatReal()};
        normalize_quat(q);
        imu_sample.qx = q[0];
        imu_sample.qy = q[1];
        imu_sample.qz = q[2];
        imu_sample.qw = q[3];
        cal_status.quat_accuracy = imu.sensorValue.status & 0x03;
        cal_status.quat_rad_accuracy = imu.getQuatRadianAccuracy();
        have_quat = true;
        break;
      }
      case SH2_GAME_ROTATION_VECTOR: {
        cal_status.quat_accuracy = imu.sensorValue.status & 0x03;
        break;
      }
      default:
        break;
    }
  }

  if (cal_state == CalibrationState::CALIBRATING) {
    if (millis() - last_cal_status_ms >= CAL_STATUS_PERIOD_MS) {
      send_cal_status();
      last_cal_status_ms = millis();
    }
    return;
  }

  // we only block on the acceleration and orientation data because the gyroscope only sends velocity measurements on
  // non-zero measurements
  if (!have_accel || !have_quat) {
    return;
  }
  have_accel = false;
  have_quat = false;

  if (millis() - last_gyro_ms > BNO08X_GYRO_STALE_MS) {
    imu_sample.gx = 0.0F;
    imu_sample.gy = 0.0F;
    imu_sample.gz = 0.0F;
  }

  send_packet(packet::PacketId::IMU_DATA, &imu_sample, sizeof(imu_sample));
}

void error_loop(ErrorCode error_code)
{
  while (true) {
    Serial.print("Error occurred while setting up the Teensy: ");
    Serial.print(static_cast<uint8_t>(error_code));
    Serial.println();
    delay(500);
  }
}

void configure()
{
  const bool calibrating = (cal_state == CalibrationState::CALIBRATING);

  imu.setCalibrationConfig(calibrating ? CAL_MASK_CALIBRATING : CAL_MASK_NORMAL);
  sh2_setDcdAutoSave(calibrating);

  // enableReport() takes microseconds as uint32_t; the enableXxx(ms) wrappers silently overflow
  // above ~65 ms because they multiply a uint16_t by 1000 internally - use enableReport() directly
  if (calibrating) {
    imu.enableReport(SH2_ACCELEROMETER, BNO08X_CAL_REPORT_PERIOD_US);
    imu.enableReport(SH2_GYROSCOPE_CALIBRATED, BNO08X_CAL_REPORT_PERIOD_US);
    imu.enableReport(SH2_MAGNETIC_FIELD_CALIBRATED, BNO08X_CAL_REPORT_PERIOD_US);
    imu.enableReport(SH2_GAME_ROTATION_VECTOR, BNO08X_CAL_REPORT_PERIOD_US);
    imu.enableReport(SH2_ROTATION_VECTOR, 0);  // 0 us disables
  } else {
    imu.enableReport(SH2_ACCELEROMETER, BNO08X_REPORT_PERIOD_MS * 1000);
    imu.enableReport(SH2_GYROSCOPE_CALIBRATED, BNO08X_REPORT_PERIOD_MS * 1000);
    imu.enableReport(SH2_ROTATION_VECTOR, BNO08X_REPORT_PERIOD_MS * 1000);
    imu.enableReport(SH2_MAGNETIC_FIELD_CALIBRATED, 0);
    imu.enableReport(SH2_GAME_ROTATION_VECTOR, 0);
  }
  delay(100);  // this is required! no, I don't understand why!
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

void decode_frame(const packet::SerialBuffer & buf)
{
  const std::optional<packet::Packet> decoded = packet::decode(buf.data(), buf.size());
  if (!decoded.has_value()) {
    return;
  }

  if (decoded->packet_id == packet::PacketId::COMMAND) {
    handle_command(*decoded);
  }
}

void handle_command(const packet::Packet & command)
{
  if (command.size != 3) {
    return;
  }

  const uint8_t command_id = command.payload[0];
  const uint8_t seq = command.payload[1];
  // command.payload[2] is `arg`, reserved for future use

  packet::CommandResponse result = packet::CommandResponse::OK;

  switch (static_cast<packet::CommandId>(command_id)) {
    case packet::CommandId::CALIBRATION_START:
      cal_state = CalibrationState::CALIBRATING;
      cal_status = CalibrationStatus{};
      configure();
      break;
    case packet::CommandId::CALIBRATION_SAVE:
      if (cal_state != CalibrationState::CALIBRATING) {
        result = packet::CommandResponse::INVALID_STATE;
      } else {
        result = imu.saveCalibration() ? packet::CommandResponse::OK : packet::CommandResponse::SENSOR_ERROR;
      }
      break;
    case packet::CommandId::CALIBRATION_STOP:
      cal_state = CalibrationState::IDLE;
      configure();
      break;
    default:
      result = packet::CommandResponse::UNKNOWN_COMMAND;
      break;
  }

  cal_status.last_result = static_cast<uint8_t>(result);

  send_command_response(command_id, seq, result);
  send_cal_status();
}

void send_packet(packet::PacketId packet_id, const void * payload, size_t size)
{
  packet::Packet p{packet_id, packet::DeviceId::IMU_01, {}, size};
  memcpy(p.payload, payload, size);

  uint8_t out[packet::MAX_ENCODED_SIZE];
  const ssize_t n = packet::encode(p, out, sizeof(out));
  if (n < 0) {
    Serial.println("Failed to encode packet");
    return;
  }

  Serial1.write(out, static_cast<size_t>(n));
}

void send_command_response(uint8_t command_id, uint8_t seq, packet::CommandResponse result)
{
  const uint8_t payload[3] = {command_id, seq, static_cast<uint8_t>(result)};
  send_packet(packet::PacketId::COMMAND_RESPONSE, payload, sizeof(payload));
}

void send_cal_status()
{
  cal_status.state = static_cast<uint8_t>(cal_state);
  sh2_getCalConfig(&cal_status.cal_config);
  send_packet(packet::PacketId::CAL_STATUS, &cal_status, sizeof(cal_status));
}
