# Toolchain file for Linux ARM cross-compilation
# Usage: cmake -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/linux-arm.cmake ..

set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR arm)

# Specify the cross compiler
# Adjust these paths based on your toolchain installation
# Common locations:
#   - /usr/bin/arm-linux-gnueabihf-gcc (Debian/Ubuntu)
#   - /opt/arm-toolchain/bin/arm-linux-gnueabihf-gcc (Custom install)
#   - arm-none-linux-gnueabihf-gcc (for Cortex-A)

# For Raspberry Pi / ARM Linux
set(CMAKE_C_COMPILER arm-linux-gnueabihf-gcc)
set(CMAKE_CXX_COMPILER arm-linux-gnueabihf-g++)

# For bare-metal ARM (uncomment if needed)
# set(CMAKE_C_COMPILER arm-none-eabi-gcc)
# set(CMAKE_CXX_COMPILER arm-none-eabi-g++)

# Where to look for the target environment
# set(CMAKE_FIND_ROOT_PATH /usr/arm-linux-gnueabihf)

# Search for programs in the build host directories
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)

# Search for libraries and headers in the target directories
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

# Compiler flags for ARM
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -march=armv7-a -mfpu=neon -mfloat-abi=hard")
