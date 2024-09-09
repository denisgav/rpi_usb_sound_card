#ifndef DEFAULT_I2S_BOARD_DEFINES__H
#define DEFAULT_I2S_BOARD_DEFINES__H

#ifndef I2S_SPK_SD
    #define I2S_SPK_SD 14 
#endif //I2S_SPK_SD

#ifndef I2S_SPK_SCK
    #define I2S_SPK_SCK 15
#endif //I2S_SPK_SCK

#ifndef I2S_SPK_WS
    #define I2S_SPK_WS (I2S_SPK_SCK+1) /*16*/// needs to be SPK_SCK +1
#endif //I2S_SPK_WS

#ifndef I2S_SPK_BPS
    #define I2S_SPK_BPS 32
#endif //I2S_SPK_BPS

#ifndef I2S_SPK_RATE_DEF
    #define I2S_SPK_RATE_DEF (48000)
#endif //I2S_SPK_RATE_DEF

//-------------------------

#endif //DEFAULT_I2S_BOARD_DEFINES__H