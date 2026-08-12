#include <Arduino.h>
#include <SparkFun_BNO08x_Arduino_Library.h>
#include <stdint.h>

#include "error_code.h"
#include "packet.h"
#include "pinout.h"
#include "sample.h"
#include "timing.h"

// IMU
BNO08x imu;
ImuSample imu_sample;

bool have_accel = false;
bool have_quat = false;
uint32_t last_gyro_ms = 0;

/// Error loop when the setup function fails. This will loop indefinitely.
void error_loop(ErrorCode error_code);

/// Configure the reports monitored by the IMU.
void set_reports();

/// Normalize the quaternions before broadcasting to give users a proper invariant.
void normalize_quat(float * q);

void setup()
{
  Serial.begin(9600);     // USB serial connection for debugging
  Serial1.begin(921600);  // hardware serial connection to the Pi

  // use SDA pin 18, SCL pin 19 with a clock rate of 400 kHz
  // for the quick connectors, this corresponds to blue: pin 18 and yellow: pin 19
  Wire.begin();
  Wire.setClock(400000);

  Serial.println("Initializing BNO08x IMU...");

  bool imu_ready = false;

  uint32_t start = millis();
  while (millis() - start < BNO08X_INIT_TIMEOUT_MS) {
    imu_ready = imu.begin(BNO08X_ADDR, Wire, BNO08X_INT_PIN, BNO08X_RST_PIN);
    if (imu_ready) {
      set_reports();
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
  if (imu.wasReset()) {
    set_reports();
  }

  if (imu.getSensorEvent()) {
    const uint8_t event_id = imu.getSensorEventID();
    if (event_id == SH2_ROTATION_VECTOR || event_id == SH2_ACCELEROMETER || event_id == SH2_GYROSCOPE_CALIBRATED) {
      switch (event_id) {
        case SH2_ACCELEROMETER: {
          imu_sample.ax = imu.getAccelX();
          imu_sample.ay = imu.getAccelY();
          imu_sample.az = imu.getAccelZ();
          have_accel = true;
          break;
        }
        case SH2_GYROSCOPE_CALIBRATED: {
          imu_sample.gx = imu.getGyroX();
          imu_sample.gy = imu.getGyroY();
          imu_sample.gz = imu.getGyroZ();
          last_gyro_ms = millis();
          break;
        }
        case SH2_ROTATION_VECTOR: {
          float q[4] = {imu.getQuatI(), imu.getQuatJ(), imu.getQuatK(), imu.getQuatReal()};
          normalize_quat(q);
          imu_sample.qx = q[0];
          imu_sample.qy = q[1];
          imu_sample.qz = q[2];
          imu_sample.qw = q[3];
          have_quat = true;
          break;
        }
        default:
          // this shouldn't happen, but add the condition anyway
          break;
      }
    }
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

  const uint8_t * data = reinterpret_cast<const uint8_t *>(&imu_sample);
  const packet::Packet p{packet::DeviceId::IMU_01, packet::PacketId::IMU_DATA, data, sizeof(imu_sample)};

  uint8_t out[packet::MAX_ENCODED_SIZE];
  const ssize_t n = packet::encode(p, out, sizeof(out));
  if (n < 0) {
    Serial.println("Failed to encode the IMU packet");
    return;
  }

  Serial1.write(out, static_cast<size_t>(n));
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

void set_reports()
{
  imu.enableAccelerometer(BNO08X_REPORT_PERIOD_MS);
  imu.enableGyro(BNO08X_REPORT_PERIOD_MS);
  imu.enableRotationVector(BNO08X_REPORT_PERIOD_MS);
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
