# MDB-C Multi-Platform Build Guide

This document describes how to build MDB-C for different target platforms.

## Quick Start

### Using the build script (Recommended)

```bash
# Build for your current platform (auto-detected)
./build.sh

# Build for specific platform
./build.sh linux-x64-release build
./build.sh linux-arm-release build
./build.sh macos-release build
./build.sh raspberry-pi build

# Clean and rebuild
./build.sh linux-x64-release rebuild

# Create distribution package
./build.sh linux-x64-release package

# Build all supported platforms
./build.sh all build
```

### Using CMake presets directly

```bash
# List available presets
cmake --list-presets

# Configure
cmake --preset=linux-x64-release

# Build
cmake --build --preset=linux-x64-release

# Clean
rm -rf build/linux-x64-release
```

## Supported Platforms

### 1. Linux x86_64

**Native compilation on Linux PC:**

```bash
# Debug build
./build.sh linux-x64-debug build

# Release build (optimized)
./build.sh linux-x64-release build
```

**Output:**
- `build/linux-x64-release/libmdb_protocol.a`
- `build/linux-x64-release/libmdb_hal.a`
- `build/linux-x64-release/libmdb_hal_callback.a`

**Requirements:**
- GCC 7.0+ or Clang 6.0+
- CMake 3.10+
- Standard C library with POSIX support

### 2. Linux ARM (Cross-compilation)

**For embedded ARM devices (Raspberry Pi, BeagleBone, etc.):**

```bash
# Install ARM cross-compiler (Ubuntu/Debian)
sudo apt-get install gcc-arm-linux-gnueabihf g++-arm-linux-gnueabihf

# Build
./build.sh linux-arm-release build
```

**Output:**
- `build/linux-arm-release/libmdb_protocol.a`
- `build/linux-arm-release/libmdb_hal.a`
- `build/linux-arm-release/libmdb_hal_callback.a`

**Requirements:**
- ARM cross-compiler toolchain
- Target sysroot (optional but recommended)

**Custom toolchain path:**

Edit `cmake/toolchains/linux-arm.cmake` if your toolchain is in a different location.

### 3. Linux ARM64/AArch64

**Option A: Cross-compilation with Docker (Recommended for macOS):**

The easiest way to cross-compile for ARM64 from macOS is using Docker:

```bash
# Quick start - single command build
make arm64

# Or using the script directly
./docker-build-arm64.sh build

# Clean and rebuild
make arm64-rebuild

# Open interactive shell in build container
make arm64-shell

# Build Docker image only (first time or after Dockerfile changes)
make docker-image
```

**Output:**
- Binaries: `build/docker-arm64/`
- Distribution: `dist/linux-arm64/`
- Includes all libraries and example applications

**Requirements:**
- Docker Desktop for Mac (or Docker Engine on Linux)
- No ARM toolchain installation needed on host!

**Verification:**
The build script automatically verifies ARM64 architecture:
```bash
$ ./docker-build-arm64.sh build
...
✓ Checking: dist/linux-arm64/mdb_test
   ELF 64-bit LSB executable, ARM aarch64, version 1 (SYSV)
```

**Option B: Native cross-compilation (Linux host):**

```bash
# Install ARM64 cross-compiler (Ubuntu/Debian)
sudo apt-get install gcc-aarch64-linux-gnu g++-aarch64-linux-gnu

# Add preset to CMakePresets.json (example already included)
cmake --preset=linux-arm64-release
cmake --build --preset=linux-arm64-release
```

### 4. macOS

**Native compilation on macOS:**

```bash
# Install Xcode Command Line Tools (if not already)
xcode-select --install

# Debug build
./build.sh macos-debug build

# Release build
./build.sh macos-release build
```

**Output:**
- `build/macos-release/libmdb_protocol.a`
- `build/macos-release/libmdb_hal.a`
- `build/macos-release/libmdb_hal_callback.a`

**Requirements:**
- Xcode Command Line Tools
- macOS 10.13+

### 5. Raspberry Pi

#### Option A: Native compilation on Raspberry Pi

```bash
# On the Raspberry Pi device
sudo apt-get install build-essential cmake
./build.sh raspberry-pi build
```

#### Option B: Cross-compilation from Linux PC

```bash
# Install Raspberry Pi toolchain
git clone https://github.com/raspberrypi/tools /opt/rpi-toolchain

# Update toolchain path in cmake/toolchains/raspberry-pi-cross.cmake
# Then build
cmake --preset=raspberry-pi-cross
cmake --build --preset=raspberry-pi-cross
```

### 6. Embedded Minimal (Callback-only)

**For bare-metal or RTOS environments:**

```bash
./build.sh embedded-minimal build
```

This preset:
- Builds only `libmdb_protocol.a` and `libmdb_hal_callback.a`
- Excludes examples and tests
- Uses `-Os` optimization for minimal size
- Provides callback interface for custom I/O

**Output:**
- `build/embedded-minimal/libmdb_protocol.a` (~10-15 KB)
- `build/embedded-minimal/libmdb_hal_callback.a` (~3-5 KB)

## Build Directory Structure

```
MDB-C/
├── build/
│   ├── linux-x64-debug/
│   │   ├── libmdb_protocol.a
│   │   ├── libmdb_hal.a
│   │   ├── libmdb_hal_callback.a
│   │   └── examples/
│   ├── linux-x64-release/
│   ├── linux-arm-release/
│   ├── macos-release/
│   ├── raspberry-pi/
│   └── embedded-minimal/
└── dist/                          # Created by package command
    ├── mdb-c-linux-x64-release.tar.gz
    ├── mdb-c-linux-arm-release.tar.gz
    └── ...
```

## Creating Distribution Packages

```bash
# Package for Linux x64
./build.sh linux-x64-release package

# Package for Linux ARM
./build.sh linux-arm-release package

# Package all platforms
./build.sh all package
```

Package contents:
```
mdb-c-linux-x64-release/
├── lib/
│   ├── libmdb_protocol.a
│   ├── libmdb_hal.a
│   └── libmdb_hal_callback.a
└── include/
    └── mdb/
        ├── Communication_Format.h
        ├── Bus_Timing.h
        ├── PreProcessors.h
        ├── MDB_Linux.h
        └── MDB_Callbacks.h
```

## Using in Your Project

### Option 1: Install to system

```bash
./build.sh linux-x64-release install
# Installs to /usr/local/lib and /usr/local/include/mdb
```

### Option 2: Use distribution package

```bash
# Extract package
tar -xzf dist/mdb-c-linux-x64-release.tar.gz -C /opt/mdb-c

# In your CMakeLists.txt:
set(MDB_ROOT /opt/mdb-c/linux-x64-release)
include_directories(${MDB_ROOT}/include)
link_directories(${MDB_ROOT}/lib)
target_link_libraries(your_app mdb_protocol mdb_hal pthread rt)
```

### Option 3: CMake FetchContent (from source)

```cmake
include(FetchContent)
FetchContent_Declare(
  mdb-c
  GIT_REPOSITORY https://github.com/your-repo/MDB-C.git
  GIT_TAG        main
)
FetchContent_MakeAvailable(mdb-c)

target_link_libraries(your_app mdb_protocol mdb_hal)
```

## Docker-Based Cross-Compilation

### Overview

Docker provides an isolated, reproducible build environment without requiring toolchain installation on your host system. This is especially useful for:

- **macOS users** needing to build for Linux ARM64
- **CI/CD pipelines** requiring consistent build environments
- **Teams** wanting reproducible builds across different developer machines

### Docker Files

The project includes:

- `Dockerfile.arm64` - Build image for ARM64 cross-compilation
- `docker-build-arm64.sh` - Automated build script
- `Makefile` - Convenient shortcuts for common tasks
- `.dockerignore` - Optimizes Docker build context

### Quick Reference

```bash
# Build for ARM64
make arm64
# or
./docker-build-arm64.sh build

# Clean build artifacts
make arm64-clean

# Clean and rebuild
make arm64-rebuild

# Interactive shell (for debugging)
make arm64-shell

# Rebuild Docker image
make docker-image
```

### What Happens During Build

1. **Image Check**: Verifies Docker image exists, builds if needed
2. **Configuration**: Runs CMake with ARM64 toolchain
3. **Compilation**: Builds all libraries and examples
4. **Distribution**: Creates `dist/linux-arm64/` with binaries
5. **Verification**: Checks binaries are ARM64 ELF format

### Directory Structure

```
MDB-C/
├── Dockerfile.arm64           # Docker build definition
├── docker-build-arm64.sh      # Build automation script
├── .dockerignore              # Build context optimization
├── Makefile                   # Convenient shortcuts
├── build/
│   └── docker-arm64/          # Docker build output
└── dist/
    └── linux-arm64/           # Distribution binaries
        ├── mdb_test
        ├── mdb_threaded
        ├── mdb43_basket_demo
        ├── mdb_sniffer_app
        ├── libmdb_protocol.a
        ├── libmdb_hal.a
        └── libmdb_hal_callback.a
```

### Advanced Usage

**Custom build options:**

```bash
# Build with verbose logging
docker run --rm \
  -v "$(pwd):/workspace" \
  -w /workspace \
  mdb-c-arm64-builder \
  bash -c "cmake -B build/custom \
    -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/linux-arm64.cmake \
    -DMDB_VERBOSE_LOGGING=ON && \
    cmake --build build/custom"
```

**Debug build:**

```bash
# Modify CMAKE_BUILD_TYPE in docker-build-arm64.sh
# Change: -DCMAKE_BUILD_TYPE=Release
# To:     -DCMAKE_BUILD_TYPE=Debug
```

**Interactive development:**

```bash
# Open shell in container
make arm64-shell

# Inside container:
root@container:/workspace# cmake -B build/test \
  -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/linux-arm64.cmake
root@container:/workspace# cmake --build build/test
root@container:/workspace# file build/test/examples/mdb_test
```

### Deploying to ARM64 Device

After building, transfer binaries to your ARM64 device:

```bash
# Copy distribution to device
scp -r dist/linux-arm64/* user@arm-device:/opt/mdb-c/

# On device: test the binary
ssh user@arm-device
cd /opt/mdb-c
./mdb_test --version
```

### Troubleshooting Docker Builds

**Docker daemon not running:**
```bash
$ ./docker-build-arm64.sh build
✗ Failed to connect to Docker daemon

Solution: Start Docker Desktop
```

**Permission denied:**
```bash
$ make arm64
bash: ./docker-build-arm64.sh: Permission denied

Solution: chmod +x docker-build-arm64.sh
```

**Out of disk space:**
```bash
# Clean Docker build cache
docker system prune -a

# Remove old build containers
docker container prune
```

**Architecture verification fails:**
```bash
# If binaries show x86_64 instead of aarch64:
# 1. Rebuild Docker image
make docker-image
# 2. Rebuild project
make arm64-rebuild
```

## Cross-Compilation Tips

### Setting up ARM toolchain (Ubuntu/Debian)

```bash
# For 32-bit ARM (ARMv7)
sudo apt-get install gcc-arm-linux-gnueabihf g++-arm-linux-gnueabihf

# For 64-bit ARM (ARMv8/AArch64)
sudo apt-get install gcc-aarch64-linux-gnu g++-aarch64-linux-gnu

# Verify installation
arm-linux-gnueabihf-gcc --version
aarch64-linux-gnu-gcc --version
```

### Custom toolchain

If you have a custom toolchain:

1. Copy an existing toolchain file:
   ```bash
   cp cmake/toolchains/linux-arm.cmake cmake/toolchains/my-custom.cmake
   ```

2. Edit the file and update compiler paths:
   ```cmake
   set(CMAKE_C_COMPILER /path/to/your/gcc)
   set(CMAKE_CXX_COMPILER /path/to/your/g++)
   ```

3. Use it:
   ```bash
   cmake -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/my-custom.cmake ..
   ```

## Troubleshooting

### "arm-linux-gnueabihf-gcc: command not found"

Install the ARM cross-compiler toolchain:
```bash
sudo apt-get install gcc-arm-linux-gnueabihf
```

### "CLOCK_MONOTONIC undeclared"

This is fixed in the code with `_POSIX_C_SOURCE 200112L`. If you still see it, check that your toolchain supports POSIX.1-2001.

### macOS: "library 'System' not found"

Install Xcode Command Line Tools:
```bash
xcode-select --install
```

### Linker errors with pthread/rt

On Linux, link with `-lpthread -lrt`:
```bash
gcc -o app main.c -lmdb_protocol -lmdb_hal -lpthread -lrt
```

On macOS, omit `-lrt`:
```bash
gcc -o app main.c -lmdb_protocol -lmdb_hal -lpthread
```

## Build Configuration Options

You can customize builds by editing `CMakePresets.json` or passing options:

```bash
# Enable verbose logging
cmake --preset=linux-x64-debug -DMDB_VERBOSE_LOGGING=ON

# Change serial device
cmake --preset=linux-x64-release -DMDB_TTY_DEVICE=/dev/ttyACM0

# Disable MDB 4.3 features
cmake --preset=linux-x64-release -DMDB_VERSION_43=OFF

# Build without examples
cmake --preset=linux-x64-release -DBUILD_EXAMPLES=OFF
```

## CI/CD Integration

### GitHub Actions with Docker (Recommended):

```yaml
name: Multi-Platform Build

on: [push, pull_request]

jobs:
  build-arm64:
    name: Build Linux ARM64
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v3

      - name: Set up Docker Buildx
        uses: docker/setup-buildx-action@v2

      - name: Build ARM64 with Docker
        run: |
          chmod +x docker-build-arm64.sh
          ./docker-build-arm64.sh build

      - name: Upload ARM64 artifacts
        uses: actions/upload-artifact@v3
        with:
          name: mdb-c-linux-arm64
          path: dist/linux-arm64/*

  build-native:
    name: Build ${{ matrix.platform }}
    strategy:
      matrix:
        platform: [linux-x64-release, macos-release]
        include:
          - platform: linux-x64-release
            os: ubuntu-latest
          - platform: macos-release
            os: macos-latest
    runs-on: ${{ matrix.os }}
    steps:
      - uses: actions/checkout@v3

      - name: Install dependencies (Linux)
        if: runner.os == 'Linux'
        run: sudo apt-get update && sudo apt-get install -y build-essential cmake

      - name: Install dependencies (macOS)
        if: runner.os == 'macOS'
        run: brew install cmake

      - name: Build
        run: ./build.sh ${{ matrix.platform }} build

      - name: Package
        run: ./build.sh ${{ matrix.platform }} package

      - name: Upload artifacts
        uses: actions/upload-artifact@v3
        with:
          name: mdb-c-${{ matrix.platform }}
          path: dist/*.tar.gz
```

### GitHub Actions without Docker (traditional):

```yaml
name: Multi-Platform Build

on: [push, pull_request]

jobs:
  build:
    strategy:
      matrix:
        platform: [linux-x64-release, linux-arm-release]
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v3
      - name: Install dependencies
        run: |
          sudo apt-get update
          sudo apt-get install -y gcc-arm-linux-gnueabihf
      - name: Build
        run: ./build.sh ${{ matrix.platform }} build
      - name: Package
        run: ./build.sh ${{ matrix.platform }} package
      - name: Upload artifacts
        uses: actions/upload-artifact@v3
        with:
          name: mdb-c-${{ matrix.platform }}
          path: dist/*.tar.gz
```

## Performance Notes

| Platform | Binary Size | Flash Usage | RAM Usage |
|----------|-------------|-------------|-----------|
| Linux x64 Release | ~25 KB | N/A | ~4 KB |
| Linux ARM Release | ~20 KB | N/A | ~4 KB |
| Embedded Minimal | ~15 KB | 15-20 KB | 2-3 KB |

Sizes are for core libraries only (protocol + HAL). Add ~5-10 KB per device implementation.
