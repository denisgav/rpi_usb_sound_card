#ifndef DEFAULT_I2S_BOARD_DEFINES__H
#define DEFAULT_I2S_BOARD_DEFINES__H

// PCM5102A configuration inputs:
// FLT - Filter select : Normal latency (Low) / Low latency (High)
// DEMP - De-emphasis control for 44.1kHz sampling rate: Off (Low) / On (High)
// XSMT - Soft mute control(1): Soft mute (Low) / soft un-mute (High)
// FMT - Audio format selection : I2S (Low) / Left justified (High)

#ifndef I2S_SPK_SD
    #define I2S_SPK_SD 6 
#endif //I2S_SPK_SD

#ifndef I2S_SPK_SCK
    #define I2S_SPK_SCK 7
#endif //I2S_SPK_SCK

#ifndef I2S_SPK_WS
    #define I2S_SPK_WS (I2S_SPK_SCK+1) // needs to be SPK_SCK +1
#endif //I2S_SPK_WS

#ifndef I2S_SPK_BPS
    #define I2S_SPK_BPS 32
#endif //I2S_SPK_BPS

#ifndef I2S_SPK_RATE_DEF
    #define I2S_SPK_RATE_DEF (48000)
#endif //I2S_SPK_RATE_DEF

//-------------------------

#endif //DEFAULT_I2S_BOARD_DEFINES__H