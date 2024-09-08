#ifndef PDM_BOARD_DEFINES__H
#define PDM_BOARD_DEFINES__H

//-------------------------
// PDM defines
//-------------------------
#ifndef PDM_MIC_0_DATA
    #define PDM_MIC_0_DATA 18
#endif //PDM_MIC_0_DATA

#ifndef PDM_MIC_0_CLK
    #define PDM_MIC_0_CLK (PDM_MIC_0_DATA + 1)
#endif //PDM_MIC_0_CLK

#ifndef PDM_MIC_1_DATA
    #define PDM_MIC_1_DATA 20
#endif //PDM_MIC_1_DATA

#ifndef PDM_MIC_1_CLK
    #define PDM_MIC_1_CLK (PDM_MIC_1_DATA + 1)
#endif //PDM_MIC_1_CLK

//-------------------------

#endif //PDM_BOARD_DEFINES__H