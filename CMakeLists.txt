cmake_minimum_required(VERSION 3.12)

# initialize pico_sdk from GIT
# (note this can come from environment, CMake cache etc)
# set(PICO_SDK_FETCH_FROM_GIT on)

# pico_sdk_import.cmake is a single file copied from this SDK
# note: this must happen before project()

include(pico_sdk_import.cmake)

project(rpi_usb_sound_card)

# initialize the Pico SDK
pico_sdk_init()

###################################################
# Volume control library
###################################################
add_library(pico_volume_ctrl INTERFACE)

target_sources(pico_volume_ctrl INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}/common/src/volume_ctrl.c
)

target_include_directories(pico_volume_ctrl INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}/common/inc
)

target_link_libraries(pico_volume_ctrl INTERFACE pico_stdlib)
###################################################

###################################################
# PDM microphone library
###################################################

add_library(pico_pdm_microphone INTERFACE)

target_sources(pico_pdm_microphone INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}/common/src/pdm/pdm_microphone.c
    ${CMAKE_CURRENT_LIST_DIR}/common/OpenPDM2PCM/OpenPDMFilter.c
    ${CMAKE_CURRENT_LIST_DIR}/common/src/ring_buf.c
)

target_include_directories(pico_pdm_microphone INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}/common/inc
    ${CMAKE_CURRENT_LIST_DIR}/common/OpenPDM2PCM
)

pico_generate_pio_header(pico_pdm_microphone ${CMAKE_CURRENT_LIST_DIR}/common/src/pdm/pdm_microphone.pio)

target_link_libraries(pico_pdm_microphone INTERFACE pico_stdlib hardware_dma hardware_pio)
###################################################

###################################################
# I2S library
###################################################

add_library(pico_i2S_audio INTERFACE)

target_sources(pico_i2S_audio INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}/common/src/i2s/machine_i2s.c
    ${CMAKE_CURRENT_LIST_DIR}/common/src/ring_buf.c
)

target_include_directories(pico_i2S_audio INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}/common/inc
)

pico_generate_pio_header(pico_i2S_audio ${CMAKE_CURRENT_LIST_DIR}/common/src/i2s/audio_i2s_tx_16b.pio)
pico_generate_pio_header(pico_i2S_audio ${CMAKE_CURRENT_LIST_DIR}/common/src/i2s/audio_i2s_tx_32b.pio)
pico_generate_pio_header(pico_i2S_audio ${CMAKE_CURRENT_LIST_DIR}/common/src/i2s/audio_i2s_rx_32b.pio)

target_link_libraries(pico_i2S_audio INTERFACE pico_stdlib hardware_dma hardware_pio)
###################################################

###################################################
# SSD1306 library
###################################################

add_library(pico_ssd1306 INTERFACE)

target_sources(pico_ssd1306 INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}/common/src/ssd1306/ssd1306.c
)

target_include_directories(pico_ssd1306 INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}/common/inc
)

target_link_libraries(pico_ssd1306 INTERFACE pico_stdlib hardware_i2c)

###################################################

###################################################
# WS2812 library
###################################################
add_library(pico_ws2812 INTERFACE)

target_sources(pico_ws2812 INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}/common/src/ws2812/ws2812.c
)

target_include_directories(pico_ws2812 INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}/common/inc
)

pico_generate_pio_header(pico_ws2812 ${CMAKE_CURRENT_LIST_DIR}/common/src/ws2812/ws2812.pio)

target_link_libraries(pico_ws2812 INTERFACE pico_stdlib hardware_dma hardware_pio)
###################################################

add_subdirectory("hello_microphone_pdm")
add_subdirectory("stereo_microphone_i2s")
add_subdirectory("stereo_speaker")
#add_subdirectory("stereo_microphone_pdm")
add_subdirectory("mono_microphone_pdm")
