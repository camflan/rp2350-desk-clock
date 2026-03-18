cmake_minimum_required(VERSION 3.13)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(PICO_BOARD pico2)
set(PICO_PLATFORM rp2350-arm-s)

include(pico_sdk_import.cmake)

project(desk_clock C CXX ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

set(LV_CONF_PATH ${CMAKE_CURRENT_SOURCE_DIR}/lib/lv_conf.h CACHE STRING "" FORCE)
set(LV_CONF_BUILD_DISABLE_EXAMPLES 1)
set(LV_CONF_BUILD_DISABLE_DEMOS 1)
set(LV_CONF_BUILD_DISABLE_THORVG_INTERNAL 1)
add_subdirectory(lib/lvgl)

add_executable(desk_clock
    src/main.c
    src/hal/pio_qspi.c
    src/hal/display_driver.c
    src/hal/rtc_driver.c
    src/hal/touch_driver.c
    src/ui/clock_analog.c
    src/ui/clock_digital.c
    src/ui/theme.c
    src/config/settings.c
)

pico_generate_pio_header(desk_clock
    ${CMAKE_CURRENT_LIST_DIR}/src/hal/pio_qspi.pio
)

target_include_directories(desk_clock PRIVATE
    src
    src/hal
    src/ui
    src/config
)

target_link_libraries(desk_clock
    pico_stdlib
    hardware_pio
    hardware_gpio
    hardware_i2c
    hardware_flash
    hardware_sync
    lvgl
)

pico_enable_stdio_usb(desk_clock 1)
pico_enable_stdio_uart(desk_clock 0)

pico_add_extra_outputs(desk_clock)
