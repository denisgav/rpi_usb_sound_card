#ifndef I2S_BOARD_DEFINES__H
#define I2S_BOARD_DEFINES__H

//-------------------------
// I2s defines
//-------------------------
#ifndef I2S_MIC_SD
    #define I2S_MIC_SD 14
#endif //I2S_MIC_SD

#ifndef I2S_MIC_SCK
    #define I2S_MIC_SCK 15
#endif //I2S_MIC_SCK

#ifndef I2S_MIC_WS
    #define I2S_MIC_WS (I2S_MIC_SCK+1) // needs to be I2S_MIC_SCK +1
#endif //I2S_MIC_WS

#ifndef I2S_MIC_BPS
    #define I2S_MIC_BPS 32 // 24 is not valid in this implementation, but INMP441 outputs 24 bits samples
#endif //I2S_MIC_BPS

#ifndef I2S_MIC_RATE_DEF
    #define I2S_MIC_RATE_DEF (48000)
#endif //I2S_MIC_RATE_DEF

//-------------------------

#endif //I2S_BOARD_DEFINES__H