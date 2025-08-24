#ifndef MAIN__H
#define MAIN__H

#include "pdm_board_defines.h"
#include "board_defines.h"

#include "microphone_settings.h"

// Comment this define to disable volume control
#define APPLY_VOLUME_FEATURE

typedef int32_t usb_audio_4b_sample;
typedef int16_t usb_audio_2b_sample;

#define USB_MIC_SAMPLE_BUFFER_SIZE  (PDM_MIC_RATE_DEF/1000) // MAX sample rate divided by 1000. Size of 1 ms sample

#endif //MAIN__H