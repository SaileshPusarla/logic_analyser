cmake_minimum_required(VERSION 3.13)

# 1. Point to your submodule
set(PICO_SDK_PATH ${CMAKE_CURRENT_SOURCE_DIR}/pico-sdk)
set(PICO_BOARD pico2 CACHE STRING "Board type")

# 2. Include SDK
include(${PICO_SDK_PATH}/pico_sdk_init.cmake)

# 3. Project definition
project(logic C CXX ASM)

# 4. Initialize SDK
pico_sdk_init()

# 6. Add executable
add_executable(logic
    src/main.c
    src/lib/capture.c
    src/lib/analyze.c
    src/lib/stream.c
    src/lib/ui.c
)

# Assuming pio_sampling.pio is in your root folder
pico_generate_pio_header(logic ${CMAKE_CURRENT_SOURCE_DIR}/pio_sampling.pio)

# 7. Include directories
target_include_directories(logic PRIVATE
    src/lib
    ${CMAKE_CURRENT_BINARY_DIR}/pio
)

# 8. Link libraries
target_link_libraries(logic pico_stdlib hardware_pio pico_multicore)

# 9. USB/UART
pico_enable_stdio_usb(logic 1)
pico_enable_stdio_uart(logic 0)

# 10. Extra outputs
pico_add_extra_outputs(logic)
