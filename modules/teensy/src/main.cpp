#include <Arduino.h>

#include <cstring>

#include "bno085.h"
#include "calibration.h"
#include "client.h"
#include "command.h"
#include "error_code.h"
#include "pinout.h"

namespace
{

BNO085 imu;
packet::Client client(Serial1);

uint32_t last_cal_status_ms = 0;

auto start_calibration_command() -> packet::CommandResponse;

auto save_calibration_command() -> packet::CommandResponse;

auto stop_calibration_command() -> packet::CommandResponse;

auto send_calibration_status() -> void;

[[noreturn]] auto error_loop(ErrorCode error_code) -> void;

}  // namespace

void setup()
{
  Serial.begin(9600);
  Serial1.begin(921600);

  Wire.begin();
  Wire.setClock(1000000);

  Serial.println("Initializing BNO08x IMU...");
  if (!imu.begin(BNO08X_ADDR, Wire, BNO08X_INT_PIN, BNO08X_RST_PIN)) {
    error_loop(ErrorCode::IMU_INIT_FAILED);
  }

  if (!imu.stop_calibration()) {
    Serial.println("Failed to apply the default calibration configuration");
  }

  client.register_handler(packet::CommandId::CALIBRATION_START, start_calibration_command);
  client.register_handler(packet::CommandId::CALIBRATION_SAVE, save_calibration_command);
  client.register_handler(packet::CommandId::CALIBRATION_STOP, stop_calibration_command);

  Serial.println("Teensy successfully initialized.");
}

void loop()
{
  client.poll_command();

  if (imu.was_reset() && !imu.reset()) {
    Serial.println("Failed to recover the IMU after a reset");
  }

  const std::optional<Sample> sample = imu.read();
  if (sample.has_value()) {
    if (!client.send(packet::PacketId::IMU_DATA, packet::DeviceId::IMU_01, *sample)) {
      Serial.println("Failed to send IMU_DATA");
    }
  }

  if (imu.calibrating() && millis() - last_cal_status_ms >= CAL_STATUS_PERIOD_MS) {
    send_calibration_status();
  }
}

namespace
{

auto start_calibration_command() -> packet::CommandResponse
{
  if (imu.calibrating()) {
    return packet::CommandResponse::OK;
  }
  return imu.start_calibration() ? packet::CommandResponse::OK : packet::CommandResponse::SENSOR_ERROR;
}

auto save_calibration_command() -> packet::CommandResponse
{
  if (!imu.calibrating()) {
    return packet::CommandResponse::INVALID_STATE;
  }
  return imu.save_calibration() ? packet::CommandResponse::OK : packet::CommandResponse::SENSOR_ERROR;
}

auto stop_calibration_command() -> packet::CommandResponse
{
  if (!imu.calibrating()) {
    return packet::CommandResponse::OK;
  }
  return imu.stop_calibration() ? packet::CommandResponse::OK : packet::CommandResponse::SENSOR_ERROR;
}

auto send_calibration_status() -> void
{
  const SensorAccuracy accuracy = imu.accuracy();

  std::array<uint8_t, 9> status{};
  status[0] = accuracy.accel;
  status[1] = accuracy.gyro;
  status[2] = accuracy.mag;
  status[3] = accuracy.quat;
  status[4] = imu.calibration_mask();
  memcpy(&status[5], &accuracy.quat_rad, sizeof(float));

  if (!client.send(packet::PacketId::CAL_STATUS, packet::DeviceId::IMU_01, status)) {
    Serial.println("Failed to send CAL_STATUS");
  }

  last_cal_status_ms = millis();
}

auto error_loop(ErrorCode error_code) -> void
{
  while (true) {
    Serial.print("Error occurred while setting up the Teensy: ");
    Serial.print(static_cast<uint8_t>(error_code));
    Serial.println();
    delay(500);
  }
}

}  // namespace
