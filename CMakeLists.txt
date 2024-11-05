
cmake_minimum_required(VERSION 3.13)

set(PICO_SDK_PATH "${CMAKE_CURRENT_LIST_DIR}/pico-sdk")
# initialize pico-sdk from submodule
# note: this must happen before project()
include(${PICO_SDK_PATH}/external/pico_sdk_import.cmake)
# initialize the Raspberry Pi Pico SDK
pico_sdk_init()

project(pickup)



include_directories(
    src/
    ../../inc
    ../../contrib/libfixmath
)

FILE(GLOB LIBpickup_SOURCE ../../src/*.c)
# TUD_DEBUG=2
# TINYUSB_DEBUG=1
# TUP_ASSERT=1

add_definitions(
    -Dpickup_NO_64_BIT
    -DFIXMATH_NO_OVERFLOW
#    -DNDEBUG
    )
# rest of your project
add_executable(pickup
    # libpickup source
    ${LIBpickup_SOURCE}
    src/usb_descriptors.c
    src/main.c
)
# Add pico_stdlib library which aggregates commonly used features
target_link_libraries(pickup pico_stdlib pico_multicore hardware_adc hardware_dma tinyusb_device tinyusb_board tinyusb_host)

pico_enable_stdio_usb(pickup 1)
pico_enable_stdio_uart(pickup 0)

# create map/bin/hex/uf2 file in addition to ELF.
pico_add_extra_outputs(pickup)

# Generate from build with: cmake -LA .. -DCMAKE_CXX_FLAGS_RELEASE="-O3 -DNDEBUG"
