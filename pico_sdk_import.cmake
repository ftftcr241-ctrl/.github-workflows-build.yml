# Bản rút gọn của pico_sdk_import.cmake: dùng PICO_SDK_PATH (biến môi trường hoặc -D).
# Phải được include TRƯỚC project().
if (DEFINED ENV{PICO_SDK_PATH} AND (NOT PICO_SDK_PATH))
    set(PICO_SDK_PATH $ENV{PICO_SDK_PATH})
    message("Using PICO_SDK_PATH from environment ('${PICO_SDK_PATH}')")
endif ()

if (NOT PICO_SDK_PATH)
    message(FATAL_ERROR
        "Không tìm thấy Pico SDK. Đặt biến môi trường PICO_SDK_PATH hoặc chạy cmake với -DPICO_SDK_PATH=<đường dẫn>.")
endif ()

get_filename_component(PICO_SDK_PATH "${PICO_SDK_PATH}" REALPATH BASE_DIR "${CMAKE_BINARY_DIR}")
if (NOT EXISTS ${PICO_SDK_PATH})
    message(FATAL_ERROR "Thư mục Pico SDK '${PICO_SDK_PATH}' không tồn tại")
endif ()

set(PICO_SDK_INIT_CMAKE_FILE ${PICO_SDK_PATH}/pico_sdk_init.cmake)
if (NOT EXISTS ${PICO_SDK_INIT_CMAKE_FILE})
    message(FATAL_ERROR "Thư mục '${PICO_SDK_PATH}' không chứa Pico SDK hợp lệ (thiếu pico_sdk_init.cmake)")
endif ()

set(PICO_SDK_PATH ${PICO_SDK_PATH} CACHE PATH "Path to the Raspberry Pi Pico SDK" FORCE)
include(${PICO_SDK_INIT_CMAKE_FILE})
