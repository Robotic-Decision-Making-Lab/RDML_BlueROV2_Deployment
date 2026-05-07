#include "tca.h"

#include <Wire.h>
#include <assert.h>

void disable_all_tca_channels()
{
  Wire.beginTransmission(TCA_ADDRESS);
  Wire.write(0x00);
  Wire.endTransmission();
}

void select_tca_channel(uint8_t channel)
{
  assert(channel < NUM_TCA_CHANNELS);
  disable_all_tca_channels();
  Wire.beginTransmission(TCA_ADDRESS);
  Wire.write(1 << channel);
  Wire.endTransmission();
}
