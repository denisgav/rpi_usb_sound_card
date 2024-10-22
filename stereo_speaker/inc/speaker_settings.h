#ifndef SPEAKER_SETTINGS__H
#define SPEAKER_SETTINGS__H

#include <pico/stdlib.h>

#include "tusb_config.h"

//--------------------------------------------------------------------+
// MACRO CONSTANT TYPEDEF PROTOTYPES
//--------------------------------------------------------------------+
typedef struct _speaker_settings_t {
    // Audio controls
  // Current states
  int8_t mute[CFG_TUD_AUDIO_FUNC_1_N_CHANNELS_RX + 1];       // +1 for master channel 0
  int16_t volume[CFG_TUD_AUDIO_FUNC_1_N_CHANNELS_RX + 1];    // +1 for master channel 0
  uint16_t volume_db[CFG_TUD_AUDIO_FUNC_1_N_CHANNELS_RX + 1];    // +1 for master channel 0
  uint32_t volume_mul_db[CFG_TUD_AUDIO_FUNC_1_N_CHANNELS_RX];

  uint32_t sample_rate;
  uint8_t resolution;
  uint32_t blink_interval_ms;

  uint16_t samples_in_i2s_frame_min;
  uint16_t samples_in_i2s_frame_max;

  bool status_updated;  
} speaker_settings_t;


#endif //SPEAKER_SETTINGS__H