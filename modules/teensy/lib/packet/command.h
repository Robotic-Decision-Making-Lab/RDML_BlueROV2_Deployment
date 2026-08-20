#ifndef COMMAND_H_
#define COMMAND_H_

// MIRRORED IN modules/autonomy_pi/ros/autonomy_teensy/include/autonomy_teensy/command.hpp
// keep in sync

#include <stdint.h>

namespace packet
{

/// identifier for a command sent from the Pi to the Teensy
enum class CommandId : uint8_t
{
  CALIBRATION_START = 0x01,
  CALIBRATION_SAVE = 0x02,
  CALIBRATION_STOP = 0x03,
};

/// result of executing a command, reported back to the Pi
enum class CommandResponse : uint8_t
{
  OK = 0x00,
  UNKNOWN_COMMAND = 0x01,
  SENSOR_ERROR = 0x02,
  INVALID_STATE = 0x03,
};

/// COMMAND packet payload (3 bytes):
///   [0] command_id  - CommandId
///   [1] seq         - sequence number, echoed back in the COMMAND_RESPONSE
///   [2] arg         - command-specific argument, 0 if unused

/// COMMAND_RESPONSE packet payload (3 bytes):
///   [0] command_id  - CommandId being responded to
///   [1] seq         - sequence number from the originating COMMAND
///   [2] result      - CommandResponse

}  // namespace packet

#endif
