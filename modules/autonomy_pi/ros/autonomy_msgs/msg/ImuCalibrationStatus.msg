# This message represents the calibration status of the BNO08x IMU on the Teensy.

std_msgs/Header header

uint8 STATE_IDLE=0
uint8 STATE_CALIBRATING=1

uint8 ACCURACY_UNRELIABLE=0
uint8 ACCURACY_LOW=1
uint8 ACCURACY_MEDIUM=2
uint8 ACCURACY_HIGH=3

# Whether the sensor is idle or currently running the calibration motion sequence.
uint8 state

# The accelerometer's dynamic calibration accuracy.
uint8 accelerometer_accuracy

# The gyroscope's dynamic calibration accuracy.
uint8 gyroscope_accuracy

# The magnetometer's dynamic calibration accuracy.
uint8 magnetometer_accuracy

# The fused orientation estimate's accuracy.
uint8 orientation_accuracy

# The fused orientation estimate's accuracy, in radians.
float32 orientation_accuracy_rad

# Whether the sensor is actively refining the accelerometer's dynamic calibration.
bool accelerometer_calibration_enabled

# Whether the sensor is actively refining the gyroscope's dynamic calibration.
bool gyroscope_calibration_enabled

# Whether the sensor is actively refining the magnetometer's dynamic calibration.
bool magnetometer_calibration_enabled
