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
    #define I2S_SPK_WS (I2S_SPK_SCK+1) /*8*/// needs to be SPK_SCK +1
#endif //I2S_SPK_WS

#ifndef I2S_SPK_PCM5102A_FLT
    #define I2S_SPK_PCM5102A_FLT (I2S_SPK_WS+1)/*9*/
#endif //I2S_SPK_PCM5102A_FLT

#ifndef I2S_SPK_PCM5102A_DEMP
    #define I2S_SPK_PCM5102A_DEMP (I2S_SPK_PCM5102A_FLT+1)/*10*/
#endif //I2S_SPK_PCM5102A_DEMP

#ifndef I2S_SPK_PCM5102A_XSMT
    #define I2S_SPK_PCM5102A_XSMT (I2S_SPK_PCM5102A_DEMP+1)/*11*/
#endif //I2S_SPK_PCM5102A_XSMT

#ifndef I2S_SPK_PCM5102A_FMT
    #define I2S_SPK_PCM5102A_FMT (I2S_SPK_PCM5102A_XSMT+1)/*12*/
#endif //I2S_SPK_PCM5102A_FMT


#ifndef I2S_SPK_BPS
    #define I2S_SPK_BPS 32
#endif //I2S_SPK_BPS

#ifndef I2S_SPK_RATE_DEF
    #define I2S_SPK_RATE_DEF (48000)
#endif //I2S_SPK_RATE_DEF

//-------------------------

#endif //DEFAULT_I2S_BOARD_DEFINES__H