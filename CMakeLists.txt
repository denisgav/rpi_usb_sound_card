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
# I2S library
###################################################

add_library(pico_i2S_audio INTERFACE)

target_sources(pico_i2S_audio INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}/common/src/i2s/machine_i2s.c
    ${CMAKE_CURRENT_LIST_DIR}/common/src/i2s/ring_buf.c
)

target_include_directories(pico_i2S_audio INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}/common/inc
)

pico_generate_pio_header(pico_i2S_audio ${CMAKE_CURRENT_LIST_DIR}/common/src/i2s/audio_i2s_tx_16b.pio)
pico_generate_pio_header(pico_i2S_audio ${CMAKE_CURRENT_LIST_DIR}/common/src/i2s/audio_i2s_tx_32b.pio)
pico_generate_pio_header(pico_i2S_audio ${CMAKE_CURRENT_LIST_DIR}/common/src/i2s/audio_i2s_rx_32b.pio)

target_link_libraries(pico_i2S_audio INTERFACE pico_stdlib hardware_dma hardware_pio)
###################################################

add_subdirectory("stereo_microphone")
add_subdirectory("stereo_speaker")
