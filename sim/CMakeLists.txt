cmake_minimum_required(VERSION 3.13)
project(desk_clock_sim C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(PkgConfig REQUIRED)
pkg_check_modules(SDL2 REQUIRED sdl2)

# Point LVGL at the sim-specific lv_conf.h
set(LV_CONF_PATH ${CMAKE_CURRENT_SOURCE_DIR}/lv_conf_sim.h CACHE STRING "" FORCE)
set(LV_CONF_BUILD_DISABLE_EXAMPLES 1)
set(LV_CONF_BUILD_DISABLE_DEMOS 1)
set(LV_CONF_BUILD_DISABLE_THORVG_INTERNAL 1)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../lib/lvgl lvgl_build)

add_executable(desk_clock_sim
    sim_main.c
    ../src/ui/clock_analog.c
    ../src/hal/rtc_driver.c
)

target_include_directories(desk_clock_sim PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}          # pico_stubs.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/hal
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/ui
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/config
    ${SDL2_INCLUDE_DIRS}
)

# Provide pico/stdlib.h and pico/time.h stubs
target_compile_definitions(desk_clock_sim PRIVATE
    PICO_SIM=1
)

target_link_libraries(desk_clock_sim
    lvgl
    ${SDL2_LIBRARIES}
    m
)
