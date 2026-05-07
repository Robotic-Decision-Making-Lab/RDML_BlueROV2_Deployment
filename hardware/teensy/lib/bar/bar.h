#ifndef _BAR_H_
#define _BAR_H_

#include <MS5837.h>
#include <stdint.h>

#include "tca.h"

class PressureSensorArray
{
public:
  /// Create a new pressure sensor array given the TCA channels that the sensors are connected to and the sensor models.
  PressureSensorArray(uint8_t channels[], uint8_t models[], uint8_t addr = TCA_ADDRESS, TwoWire * wire = &Wire);

  /// Destructor.
  ~PressureSensorArray();

  /// Read the pressure, temperature, and depth from all sensors.
  void read_sensors(float pressure[], float temperature[], float depth[]);

private:
  MS5837 _sensors[];
  uint8_t _channels[];
  size_t _num_sensors;
  uint8_t _addr;
  TwoWire * _wire;
};

#endif
