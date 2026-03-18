cmake_minimum_required(VERSION 3.13)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(PICO_BOARD pico2)
set(PICO_PLATFORM rp2350-arm-s)

include(pico_sdk_import.cmake)

project(desk_clock C CXX ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

add_executable(desk_clock
    src/main.c
)

target_include_directories(desk_clock PRIVATE
    src
    src/hal
    src/ui
    src/config
)

target_link_libraries(desk_clock
    pico_stdlib
)

pico_enable_stdio_usb(desk_clock 1)
pico_enable_stdio_uart(desk_clock 0)

pico_add_extra_outputs(desk_clock)
