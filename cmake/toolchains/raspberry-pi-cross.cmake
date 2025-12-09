# Toolchain file for Raspberry Pi cross-compilation
# For Raspberry Pi 3/4/5 (64-bit)
# Download toolchain from: https://github.com/raspberrypi/tools

set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR aarch64)

# Set this to your Raspberry Pi toolchain path
set(RPI_TOOLCHAIN_PATH "/opt/rpi-toolchain" CACHE PATH "Path to Raspberry Pi toolchain")

set(CMAKE_C_COMPILER ${RPI_TOOLCHAIN_PATH}/bin/aarch64-linux-gnu-gcc)
set(CMAKE_CXX_COMPILER ${RPI_TOOLCHAIN_PATH}/bin/aarch64-linux-gnu-g++)

set(CMAKE_SYSROOT ${RPI_TOOLCHAIN_PATH}/sysroot)

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

# Raspberry Pi specific flags
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -march=armv8-a+crc -mtune=cortex-a72")
