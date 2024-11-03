cmake_minimum_required(VERSION 3.12)

set(PICO_SDK_PATH "${CMAKE_CURRENT_LIST_DIR}/../pico-sdk")
# initialize pico-sdk from submodule
# note: this must happen before project()
include(${PICO_SDK_PATH}/external/pico_sdk_import.cmake)
# initialize the Raspberry Pi Pico SDK
pico_sdk_init()

# Define the project name
project(pico_blinky)

include_directories(
    src/
)

# Define the executable
add_executable(pico_blinky src/main.c)

# Link standard Pico libraries
target_link_libraries(pico_blinky pico_stdlib pico_multicore hardware_adc hardware_dma tinyusb_device tinyusb_board tinyusb_host)

# Enable USB output if required
pico_enable_stdio_usb(pico_blinky 1)
pico_enable_stdio_uart(pico_blinky 0)

# Add extra outputs like the .uf2 file
pico_add_extra_outputs(pico_blinky)
