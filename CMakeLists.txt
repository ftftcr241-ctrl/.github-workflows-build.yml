cmake_minimum_required(VERSION 3.13)

set(PICO_BOARD pico2)
include(pico_sdk_import.cmake)

project(pico2_xover C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

add_executable(xover
    main.c
    i2s_out.c
    usb_descriptors.c
    usb_audio.c
)

pico_generate_pio_header(xover ${CMAKE_CURRENT_LIST_DIR}/i2s3.pio)
target_include_directories(xover PRIVATE ${CMAKE_CURRENT_LIST_DIR})   # tusb_config.h nằm ở đây

target_link_libraries(xover
    pico_stdlib
    pico_multicore
    pico_unique_id
    hardware_pio
    hardware_dma
    hardware_vreg
    tinyusb_device
)

pico_add_extra_outputs(xover)
