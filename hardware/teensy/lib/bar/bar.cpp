#include "bar.h"

#include <Wire.h>

namespace
{

void bar_init(const MS5837 * sensor, uint8_t channel, uint8_t type)
{
  select_tca_channel(channel);
  sensor->setModel(type);
  sensor->init();
  sensor->setFluidDensity(997);  // kg/m^3 (freshwater, 1029 for seawater)
}

void read_bar(const MS5837 * sensor, float * pressure, float * temperature, float * depth)
{
  sensor->read();
  *pressure = sensor->pressure();
  *temperature = sensor->temperature();
  *depth = sensor->depth();
}

};  // namespace

PressureSensorArray::PressureSensorArray(uint8_t channels[], uint8_t models[], uint8_t addr, TwoWire * wire)  // NOLINT
: _channels(channels),
  _num_sensors(sizeof(channels) / sizeof(channels[0])),
  _addr(addr),
  _wire(wire)
{
  _sensors = new MS5837[_num_sensors];

  for (size_t i = 0; i < _num_sensors; i++) {
    bar_init(_channels[i], &_sensors[i], models[i]);
  }
}

PressureSensorArray::~PressureSensorArray() { delete[] _sensors; }  // NOLINT

void PressureSensorArray::read_sensors(float pressure[], float temperature[], float depth[])  // NOLINT
{
  for (size_t i = 0; i < _num_sensors; i++) {
    select_tca_channel(_channels[i]);
    read_bar(&_sensors[i], &pressure[i], &temperature[i], &depth[i]);
  }
}
