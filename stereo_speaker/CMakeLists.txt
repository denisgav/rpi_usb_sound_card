cmake_minimum_required(VERSION 3.12)

# rest of your project
add_executable(stereo_speaker
    src/main.c
    src/usb_descriptors.c
    src/usb_speaker.c
)

target_include_directories(stereo_speaker PRIVATE ${CMAKE_CURRENT_LIST_DIR}/inc)

target_link_libraries(stereo_speaker PRIVATE tinyusb_device tinyusb_board pico_stdlib pico_i2S_audio pico_volume_ctrl)

# create map/bin/hex/uf2 file in addition to ELF.
pico_add_extra_outputs(stereo_speaker)
