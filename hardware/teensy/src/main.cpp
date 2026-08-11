#include <Arduino.h>
#include <SparkFun_BNO08x_Arduino_Library.h>

#include "packet.h"

const uint8_t BNO08X_ADDR = 0x4A;
const uint8_t BNO08X_INT_PIN = 2;
const uint8_t BNO08X_RST_PIN = 3;
const uint32_t IMU_RETRY_INTERVAL_MS = 1000;
const uint8_t BNO08X_MAX_EVENTS_PER_LOOP = 6;
const uint16_t BNO08X_PERIOD_MS = 5;  // ~200 Hz

BNO08x imu;
bool imu_ok = false;
uint32_t last_imu_retry_ms = 0;

bool have_accel = false;
bool have_gyro = false;
bool have_quat = false;

struct ImuSample
{
  float qx, qy, qz, qw;
  float gx, gy, gz;
  float ax, ay, az;
};
ImuSample sample{};

void set_reports();

void poll_imu();

void send_imu_packet();

void normalize_quat(float * q);

void setup()
{
  Serial.begin(9600);     // USB serial connection for debugging
  Serial1.begin(921600);  // hardware serial connection to the Pi

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
}

void loop()
{
  poll_imu();
  send_imu_packet();
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
        sample.ax = imu.getAccelX();
        sample.ay = imu.getAccelY();
        sample.az = imu.getAccelZ();
        have_accel = true;
        break;
      }

      case SH2_GYROSCOPE_CALIBRATED: {
        sample.gx = imu.getGyroX();
        sample.gy = imu.getGyroY();
        sample.gz = imu.getGyroZ();
        have_gyro = true;
        break;
      }

      case SH2_ROTATION_VECTOR: {
        float q[4] = {imu.getQuatI(), imu.getQuatJ(), imu.getQuatK(), imu.getQuatReal()};
        normalize_quat(q);
        sample.qx = q[0];
        sample.qy = q[1];
        sample.qz = q[2];
        sample.qw = q[3];
        have_quat = true;
        break;
      }

      default:
        break;
    }
  }
}

void send_imu_packet()
{
  if (!have_accel || !have_gyro || !have_quat) {
    return;
  }

  have_accel = false;
  have_gyro = false;
  have_quat = false;

  const uint8_t * data = reinterpret_cast<const uint8_t *>(&sample);
  const packet::Packet p{packet::DeviceId::IMU_01, packet::PacketId::IMU_DATA, data, sizeof(sample)};

  uint8_t out[packet::MAX_ENCODED_SIZE];
  const ssize_t n = packet::encode(p, out, sizeof(out));
  if (n < 0) {
    Serial.println("Failed to encode the IMU packet");
    return;
  }

  Serial1.write(out, static_cast<size_t>(n));
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
