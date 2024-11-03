
cmake_minimum_required(VERSION 3.13)

set(PICO_SDK_PATH "${CMAKE_CURRENT_LIST_DIR}/pico-sdk")
# initialize pico-sdk from submodule
# note: this must happen before project()
include(${PICO_SDK_PATH}/external/pico_sdk_import.cmake)
# initialize the Raspberry Pi Pico SDK
pico_sdk_init()

project(migic)



include_directories(
    src/
    ../../inc
    ../../contrib/libfixmath
)

FILE(GLOB LIBMIGIC_SOURCE ../../src/*.c)
add_definitions(
    -DMIGIC_NO_64_BIT
    -DFIXMATH_NO_OVERFLOW
    )
# rest of your project
add_executable(migic
    # libmigic source
    ${LIBMIGIC_SOURCE}
    src/usb_descriptors.c
    src/main.c
)
# Add pico_stdlib library which aggregates commonly used features
target_link_libraries(migic pico_stdlib pico_multicore hardware_adc hardware_dma tinyusb_device tinyusb_board tinyusb_host)

pico_enable_stdio_usb(migic 1)
pico_enable_stdio_uart(migic 0)

# create map/bin/hex/uf2 file in addition to ELF.
pico_add_extra_outputs(migic)

# Generate from build with: cmake -LA .. -DCMAKE_CXX_FLAGS_RELEASE="-O3 -DNDEBUG"
