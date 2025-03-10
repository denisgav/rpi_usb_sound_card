#ifndef MAIN__H
#define MAIN__H

#include "i2s_board_defines.h"
#include "i2c_board_defines.h"
#include "board_defines.h"

#include "microphone_settings.h"

// Comment this define to disable volume control
#define APPLY_VOLUME_FEATURE
#define FORMAT_24B_TO_24B_SHIFT_VAL 4u
#define FORMAT_24B_TO_16B_SHIFT_VAL 12u

typedef int32_t usb_audio_4b_sample;
typedef int16_t usb_audio_2b_sample;

#define USB_MIC_SAMPLE_BUFFER_SIZE  (I2S_MIC_RATE_DEF/1000) // MAX sample rate divided by 1000. Size of 1 ms sample

// Uncomment it to enable WS2812 indication
//#define WS2812_EN

#endif //MAIN__H