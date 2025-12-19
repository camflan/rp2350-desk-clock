cmake_minimum_required(VERSION 3.13)

# Pull in Pico SDK (must be before project)
include(pico_sdk_import.cmake)

project(rp2350_amoled_clock C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Initialize the Pico SDK
pico_sdk_init()

# Add LVGL
add_subdirectory(lib/lvgl)

# Main executable
add_executable(${PROJECT_NAME}
    src/main.c
    src/hal/display_driver.c
)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
)

target_link_libraries(${PROJECT_NAME}
    pico_stdlib
    hardware_spi
    hardware_i2c
    hardware_timer
    hardware_flash
    hardware_pwm
    lvgl
)

# Enable USB serial output, disable UART
pico_enable_stdio_usb(${PROJECT_NAME} 1)
pico_enable_stdio_uart(${PROJECT_NAME} 0)

# Create map/bin/hex/uf2 files
pico_add_extra_outputs(${PROJECT_NAME})
