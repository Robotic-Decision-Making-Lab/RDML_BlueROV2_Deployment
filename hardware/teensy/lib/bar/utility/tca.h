#ifndef _TCA_H_
#define _TCA_H_

#include <stdint.h>

const uint8_t TCA_ADDRESS = 0x70;
const uint8_t NUM_TCA_CHANNELS = 8;

/// Disable all TCA9548A channels.
void disable_all_tca_channels();

/// Select a TCA9548A channel.
void select_tca_channel(uint8_t channel);

#endif
