#ifndef MAIN__H
#define MAIN__H

#include "i2s_board_defines.h"
#include "i2c_board_defines.h"
#include "board_defines.h"

#include "speaker_settings.h"

#define SAMPLE_BUFFER_SIZE  (96000/1000) // MAX sample rate divided by 1000. Size of 1 ms sample

// Uncomment it to enable WS2812 indication
//#define WS2812_EN

//-------------------------

#endif //MAIN__H