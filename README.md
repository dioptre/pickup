
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
#cmake .
cmake . -DCMAKE_CXX_FLAGS_RELEASE="-O3 -DNDEBUG"
make
```