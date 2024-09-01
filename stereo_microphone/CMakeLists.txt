cmake_minimum_required(VERSION 3.12)

# rest of your project
add_executable(stereo_microphone
    src/main.c
    src/usb_descriptors.c
    src/usb_microphone.c
)

target_include_directories(stereo_microphone PRIVATE ./inc ${CMAKE_CURRENT_LIST_DIR}/common/inc)

target_link_libraries(stereo_microphone PRIVATE tinyusb_device tinyusb_board pico_stdlib pico_i2S_audio pico_volume_ctrl pico_ssd1306)

# create map/bin/hex/uf2 file in addition to ELF.
pico_add_extra_outputs(stereo_microphone)
