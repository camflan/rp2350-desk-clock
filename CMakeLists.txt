cmake_minimum_required(VERSION 3.13)

# Set platform and board before SDK import
set(PICO_PLATFORM rp2350-arm-s CACHE STRING "Pico platform")
set(PICO_BOARD pico2 CACHE STRING "Pico board type")

# Pull in Pico SDK (must be before project)
include(pico_sdk_import.cmake)

project(rp2350_amoled_clock C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Initialize the Pico SDK
pico_sdk_init()

# Workaround for C++ standard library headers not being found
# This needs to be set early, before any targets are created
execute_process(
    COMMAND ${CMAKE_CXX_COMPILER} --print-file-name=include
    OUTPUT_VARIABLE GCC_INCLUDE_DIR
    OUTPUT_STRIP_TRAILING_WHITESPACE
)
include_directories(SYSTEM
    "${GCC_INCLUDE_DIR}/../../../../../arm-none-eabi/include/c++/14.3.1"
    "${GCC_INCLUDE_DIR}/../../../../../arm-none-eabi/include/c++/14.3.1/arm-none-eabi"
    "${GCC_INCLUDE_DIR}/../../../../../arm-none-eabi/include/c++/14.3.1/arm-none-eabi/thumb/v8-m.main+fp/softfp"
)


# Add LVGL
add_subdirectory(lib/lvgl)

# Main executable
add_executable(${PROJECT_NAME}
    src/main.c
    src/hal/display_driver.c
    src/hal/lvgl_helpers.c
    src/hal/pio_qspi.c
    src/hal/rtc.c
    src/ui/analog_clock.c
)

# Generate PIO header from .pio file (must be after add_executable)
pico_generate_pio_header(${PROJECT_NAME}
    ${CMAKE_CURRENT_LIST_DIR}/src/hal/pio_qspi.pio
)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
)

target_link_libraries(${PROJECT_NAME}
    pico_stdlib
    hardware_pio
    hardware_i2c
    hardware_timer
    hardware_flash
    hardware_pwm
    lvgl
)

# Disable C++ exceptions and RTTI
target_compile_options(${PROJECT_NAME} PRIVATE
    $<$<COMPILE_LANGUAGE:CXX>:-fno-exceptions>
    $<$<COMPILE_LANGUAGE:CXX>:-fno-rtti>
)


# Enable USB serial output, disable UART
pico_enable_stdio_usb(${PROJECT_NAME} 1)
pico_enable_stdio_uart(${PROJECT_NAME} 0)

# Create map/bin/hex/uf2 files
pico_add_extra_outputs(${PROJECT_NAME})
