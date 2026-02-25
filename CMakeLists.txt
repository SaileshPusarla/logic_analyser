cmake_minimum_required(VERSION 3.13)

# 1. Point to your submodule
set(PICO_SDK_PATH ${CMAKE_CURRENT_SOURCE_DIR}/pico-sdk)

# 2. FORCE PICO 2 (RP2350) TARGET
set(PICO_BOARD pico2 CACHE STRING "Board type")

# 3. Load the SDK's initialization logic
include(${PICO_SDK_PATH}/pico_sdk_init.cmake)

project(my_pico_project C CXX ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# 4. Initialize the SDK (now it knows it's a Pico 2)
pico_sdk_init()

add_executable(my_pico_project
    src/main.c
)

target_link_libraries(my_pico_project 
    pico_stdlib
)

pico_enable_stdio_usb(my_pico_project 1)
pico_enable_stdio_uart(my_pico_project 0)

pico_add_extra_outputs(my_pico_project)
