# Guitar pickup with pico

## Turn anything into an instrument

I needed this for an instrument for an art project. I used a guitar pickup to interact with moving metal on magnets.

![RPi Pico Amp Schematics](https://github.com/user-attachments/assets/443ece3e-96ee-4f5c-9202-811b5c025777)
https://github.com/michelemaroni/pico_guitar_to_midi/commit/cbf83c39cf9979ff5f4594b2199a0cb3d392c99f

## Instructions

The following works in macOS 2024, tested on m2.


```sh
git submodule init
git submodule update
cd pico-sdk
git submodule init
git submodule update
brew install cmake
#brew remove arm-none-eabi-gcc
brew install --cask gcc-arm-embedded
brew install arm-none-eabi-gcc
sed -i '' '1s/^/#include <stdbool.h>\n/' pico-sdk/src/rp2_common/pico_stdio_usb/stdio_usb.c
cmake .
cd ..
cmake . -DCMAKE_CXX_FLAGS_RELEASE="-O3"
make
#debug
#screen /dev/tty.usbmodemXXXX 115200
# ioreg -p IOUSB -l
```

## Credits

Thanks to https://github.com/ripxorip/pico_guitar_to_midi for getting this started
