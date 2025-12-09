#!/bin/bash
# MDB-C Multi-Platform Build Script
# Usage: ./build.sh [platform] [action]
#   platform: linux-x64, linux-arm, macos, raspberry-pi, embedded, all
#   action: configure, build, clean, rebuild, install

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Function to print colored messages
print_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

print_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Print usage
usage() {
    cat << EOF
MDB-C Multi-Platform Build Script

Usage: $0 [platform] [action]

Platforms:
  linux-x64-debug     Linux x86_64 Debug build
  linux-x64-release   Linux x86_64 Release build (default)
  linux-arm-debug     Linux ARM Debug (cross-compile)
  linux-arm-release   Linux ARM Release (cross-compile)
  macos-debug         macOS Debug build
  macos-release       macOS Release build
  raspberry-pi        Raspberry Pi native build
  embedded-minimal    Embedded minimal build (callback only)
  all                 Build all supported platforms

Actions:
  configure           Configure only (run CMake)
  build               Build only (default)
  clean               Clean build directory
  rebuild             Clean + Configure + Build
  install             Install to system (requires sudo)
  package             Create distribution package

Examples:
  $0                            # Build linux-x64-release
  $0 linux-x64-debug build      # Build Linux debug
  $0 macos-release rebuild      # Rebuild macOS release
  $0 all build                  # Build all platforms

EOF
    exit 1
}

# Detect host platform
detect_platform() {
    if [[ "$OSTYPE" == "linux-gnu"* ]]; then
        echo "linux-x64-release"
    elif [[ "$OSTYPE" == "darwin"* ]]; then
        echo "macos-release"
    else
        echo "linux-x64-release"
    fi
}

# Configure platform
configure_platform() {
    local preset=$1
    local build_dir="build/$preset"

    print_info "Configuring $preset..."

    # Check CMake version for preset support
    local cmake_version=$(cmake --version | head -n1 | grep -oE '[0-9]+\.[0-9]+' | head -n1)
    local cmake_major=$(echo $cmake_version | cut -d. -f1)
    local cmake_minor=$(echo $cmake_version | cut -d. -f2)

    # CMake 3.19+ supports presets
    if [ "$cmake_major" -ge 3 ] && [ "$cmake_minor" -ge 19 ]; then
        cmake --preset="$preset"
    else
        # Fallback for older CMake versions
        print_warning "CMake 3.19+ required for presets. Using manual configuration."
        mkdir -p "$build_dir"
        cd "$build_dir"

        # Set options based on preset name
        local cmake_opts=""

        case "$preset" in
            *-debug)
                cmake_opts="-DCMAKE_BUILD_TYPE=Debug -DMDB_VERBOSE_LOGGING=ON"
                ;;
            *-release)
                cmake_opts="-DCMAKE_BUILD_TYPE=Release -DMDB_VERBOSE_LOGGING=OFF"
                ;;
            embedded-minimal)
                cmake_opts="-DCMAKE_BUILD_TYPE=MinSizeRel -DBUILD_EXAMPLES=OFF -DBUILD_TESTS=OFF"
                ;;
        esac

        # Platform specific options
        case "$preset" in
            linux-arm*)
                cmake_opts="$cmake_opts -DCMAKE_TOOLCHAIN_FILE=../../cmake/toolchains/linux-arm.cmake"
                ;;
            macos*)
                cmake_opts="$cmake_opts -DCMAKE_C_COMPILER=clang -DMDB_TTY_DEVICE=/dev/cu.usbserial"
                ;;
            linux-x64*)
                cmake_opts="$cmake_opts -DCMAKE_C_COMPILER=gcc -DMDB_TTY_DEVICE=/dev/ttyUSB0"
                ;;
        esac

        cmake $cmake_opts -DMDB_VERSION_43=ON ../..
        cd "$SCRIPT_DIR"
    fi

    print_success "Configuration complete: $build_dir"
}

# Build platform
build_platform() {
    local preset=$1
    local build_dir="build/$preset"

    if [ ! -d "$build_dir" ]; then
        print_error "Build directory not found: $build_dir"
        print_info "Run configure first: $0 $preset configure"
        exit 1
    fi

    print_info "Building $preset..."

    # Check CMake version
    local cmake_version=$(cmake --version | head -n1 | grep -oE '[0-9]+\.[0-9]+' | head -n1)
    local cmake_major=$(echo $cmake_version | cut -d. -f1)
    local cmake_minor=$(echo $cmake_version | cut -d. -f2)

    if [ "$cmake_major" -ge 3 ] && [ "$cmake_minor" -ge 19 ]; then
        cmake --build --preset="$preset"
    else
        # Fallback for older CMake
        cmake --build "$build_dir" -- -j8
    fi

    print_success "Build complete: $build_dir"
}

# Clean platform
clean_platform() {
    local preset=$1
    local build_dir="build/$preset"

    if [ -d "$build_dir" ]; then
        print_info "Cleaning $preset..."
        rm -rf "$build_dir"
        print_success "Clean complete: $preset"
    else
        print_warning "Build directory not found: $build_dir"
    fi
}

# Rebuild platform
rebuild_platform() {
    local preset=$1
    clean_platform "$preset"
    configure_platform "$preset"
    build_platform "$preset"
}

# Install platform
install_platform() {
    local preset=$1
    local build_dir="build/$preset"

    if [ ! -d "$build_dir" ]; then
        print_error "Build directory not found: $build_dir"
        print_info "Run: $0 $preset build"
        exit 1
    fi

    print_info "Installing $preset..."
    cd "$build_dir"
    sudo make install
    cd "$SCRIPT_DIR"
    print_success "Installation complete"
}

# Create package
package_platform() {
    local preset=$1
    local build_dir="build/$preset"

    if [ ! -d "$build_dir" ]; then
        print_error "Build directory not found: $build_dir"
        exit 1
    fi

    print_info "Creating package for $preset..."

    local package_dir="dist/$preset"
    mkdir -p "$package_dir/lib"
    mkdir -p "$package_dir/include/mdb/protocol"
    mkdir -p "$package_dir/include/mdb/hal"
    mkdir -p "$package_dir/include/mdb/devices"
    mkdir -p "$package_dir/examples"
    mkdir -p "$package_dir/docs"

    # Copy libraries
    print_info "  Copying libraries..."
    cp "$build_dir"/*.a "$package_dir/lib/" 2>/dev/null || true
    cp "$build_dir"/Devices/*.a "$package_dir/lib/" 2>/dev/null || true

    # Copy protocol headers
    print_info "  Copying protocol headers..."
    cp Protocol_Files/Communication_Format.h "$package_dir/include/mdb/protocol/"
    cp Protocol_Files/Bus_Timing.h "$package_dir/include/mdb/protocol/"
    cp Protocol_Files/PreProcessors.h "$package_dir/include/mdb/protocol/"
    cp Protocol_Files/EVA-DTS.h "$package_dir/include/mdb/protocol/" 2>/dev/null || true
    cp Protocol_Files/FTL.h "$package_dir/include/mdb/protocol/" 2>/dev/null || true

    # Copy HAL headers
    print_info "  Copying HAL headers..."
    cp MDB_Linux.h "$package_dir/include/mdb/hal/"
    cp MDB_Callbacks.h "$package_dir/include/mdb/hal/"

    # Copy device headers
    print_info "  Copying device headers..."
    find Devices -name "*.h" -exec cp {} "$package_dir/include/mdb/devices/" \; 2>/dev/null || true

    # Copy example code
    print_info "  Copying examples..."
    cp examples/*.c "$package_dir/examples/" 2>/dev/null || true

    # Create main include file
    cat > "$package_dir/include/mdb/mdb.h" << 'EOF'
/**
 * @file mdb.h
 * @brief MDB-C Main Include File
 *
 * Include this file to get all MDB-C functionality.
 */

#ifndef MDB_H
#define MDB_H

/* Protocol headers */
#include "protocol/PreProcessors.h"
#include "protocol/Bus_Timing.h"
#include "protocol/Communication_Format.h"

/* HAL headers - choose one based on your needs */
/* Direct hardware access (single-threaded): */
/* #include "hal/MDB_Linux.h" */

/* Callback-based (thread-safe, RTOS-compatible): */
/* #include "hal/MDB_Callbacks.h" */

/* Device headers - include as needed */
/* #include "devices/VMC.h" */
/* #include "devices/Cashless.h" */
/* #include "devices/Coin_Changer.h" */
/* #include "devices/Bill_Validator.h" */

#endif /* MDB_H */
EOF

    # Create README
    cat > "$package_dir/README.md" << 'EOF'
# MDB-C Library Package

Version: 4.3.0
Build: PRESET_NAME
Generated: BUILD_DATE

## Package Contents

```
.
├── lib/                          # Static libraries
│   ├── libmdb_protocol.a         # Core MDB protocol implementation
│   ├── libmdb_hal.a              # Hardware abstraction (direct I/O)
│   └── libmdb_hal_callback.a     # Hardware abstraction (callback-based)
├── include/                      # Header files
│   └── mdb/
│       ├── mdb.h                 # Main include file
│       ├── protocol/             # Protocol layer headers
│       │   ├── Communication_Format.h
│       │   ├── Bus_Timing.h
│       │   └── PreProcessors.h
│       ├── hal/                  # Hardware abstraction layer
│       │   ├── MDB_Linux.h
│       │   └── MDB_Callbacks.h
│       └── devices/              # Device implementations
│           ├── VMC.h
│           ├── Cashless.h
│           ├── Coin_Changer.h
│           └── Bill_Validator.h
├── examples/                     # Example code
└── docs/                         # Documentation
```

## Quick Start

### 1. Installation

Copy to your project:
```bash
cp -r include/mdb /your/project/include/
cp lib/*.a /your/project/lib/
```

Or install system-wide:
```bash
sudo cp -r include/mdb /usr/local/include/
sudo cp lib/*.a /usr/local/lib/
```

### 2. CMake Integration

```cmake
# In your CMakeLists.txt
include_directories(/path/to/mdb-c/include)
link_directories(/path/to/mdb-c/lib)

add_executable(your_app main.c)

# Direct hardware access
target_link_libraries(your_app
    mdb_protocol
    mdb_hal
    pthread
    rt  # Linux only
)

# Or callback-based (thread-safe)
target_link_libraries(your_app
    mdb_protocol
    mdb_hal_callback
    pthread
)
```

### 3. Makefile Integration

```makefile
CC = gcc
CFLAGS = -Wall -O2 -I/path/to/mdb-c/include
LDFLAGS = -L/path/to/mdb-c/lib -lmdb_protocol -lmdb_hal -lpthread -lrt

your_app: main.o
	$(CC) -o $@ $^ $(LDFLAGS)

main.o: main.c
	$(CC) $(CFLAGS) -c $<
```

### 4. Simple Example

```c
#define _POSIX_C_SOURCE 200112L

#include <stdio.h>
#include <mdb/hal/MDB_Linux.h>
#include <mdb/protocol/Communication_Format.h>
#include <mdb/protocol/PreProcessors.h>

int main(void) {
    /* Initialize MDB on /dev/ttyUSB0 */
    if (mdb_init("/dev/ttyUSB0") != 0) {
        fprintf(stderr, "Failed to initialize MDB\n");
        return 1;
    }

    printf("MDB initialized successfully\n");

    /* Your MDB code here */

    mdb_close();
    return 0;
}
```

Compile:
```bash
gcc -o app main.c -I./include -L./lib -lmdb_protocol -lmdb_hal -lpthread -lrt
```

## Library Sizes

- libmdb_protocol.a: ~3-4 KB (core protocol)
- libmdb_hal.a: ~5-6 KB (direct hardware access)
- libmdb_hal_callback.a: ~3-4 KB (callback interface)

Total: ~12-15 KB for complete MDB support

## Platform Support

- Linux (x86_64, ARM, ARM64)
- macOS
- Embedded systems (via callback interface)
- RTOS (FreeRTOS, Zephyr, etc.)

## Documentation

Full documentation available at: https://github.com/your-repo/MDB-C

## License

See LICENSE file in source repository.
EOF

    # Replace placeholders in README
    sed -i.bak "s/PRESET_NAME/$preset/g" "$package_dir/README.md"
    sed -i.bak "s/BUILD_DATE/$(date -u +"%Y-%m-%d %H:%M:%S UTC")/g" "$package_dir/README.md"
    rm -f "$package_dir/README.md.bak"

    # Create integration example
    cat > "$package_dir/docs/INTEGRATION.md" << 'EOF'
# MDB-C Integration Guide

## Option 1: Copy files to your project

```bash
project/
├── src/
│   └── main.c
├── lib/
│   ├── libmdb_protocol.a
│   ├── libmdb_hal.a
│   └── libmdb_hal_callback.a
└── include/
    └── mdb/
        ├── protocol/
        ├── hal/
        └── devices/
```

## Option 2: Reference from install location

```cmake
set(MDB_ROOT /opt/mdb-c)
include_directories(${MDB_ROOT}/include)
link_directories(${MDB_ROOT}/lib)
```

## Option 3: CMake FetchContent

```cmake
include(FetchContent)
FetchContent_Declare(
  mdb-c
  URL file:///path/to/mdb-c-package.tar.gz
)
FetchContent_MakeAvailable(mdb-c)
```

## Choosing HAL Implementation

### Direct Hardware Access (libmdb_hal.a)
- Simple, low overhead
- Direct serial port access
- Single-threaded
- Use when: Running on Linux, single process

### Callback-based (libmdb_hal_callback.a)
- Thread-safe
- User-provided I/O functions
- RTOS compatible
- Use when: Multi-threaded, custom transport, embedded

## Minimal Integration

Only need core protocol? Use just `libmdb_protocol.a` and implement your own HAL functions:
- `tX9Bits()` - transmit 9-bit byte
- `rX9Bits()` - receive 9-bit byte
- `micros()` - microsecond timer
- `delayMicroseconds()` - microsecond delay
- `rxBitsAvailable()` - check if data available
- `tXBitsFlush()` - wait for transmission complete
EOF

    # Create pkg-config file
    mkdir -p "$package_dir/lib/pkgconfig"
    cat > "$package_dir/lib/pkgconfig/mdb-c.pc" << EOF
prefix=/usr/local
exec_prefix=\${prefix}
libdir=\${exec_prefix}/lib
includedir=\${prefix}/include

Name: MDB-C
Description: MDB/ICP Protocol Library
Version: 4.3.0
Cflags: -I\${includedir}
Libs: -L\${libdir} -lmdb_protocol -lmdb_hal
Libs.private: -lpthread -lrt
EOF

    # Create version file
    cat > "$package_dir/VERSION" << EOF
MDB-C Version 4.3.0
Build: $preset
Date: $(date -u +"%Y-%m-%d %H:%M:%S UTC")
Commit: $(git rev-parse --short HEAD 2>/dev/null || echo "unknown")
EOF

    # Create tarball
    cd dist
    tar -czf "mdb-c-$preset.tar.gz" "$preset"
    cd "$SCRIPT_DIR"

    print_success "Package created: dist/mdb-c-$preset.tar.gz"
    print_info "Package contents:"
    print_info "  - Libraries in lib/"
    print_info "  - Headers in include/mdb/"
    print_info "  - Examples in examples/"
    print_info "  - Documentation in docs/"
}

# Process action for a single platform
process_platform() {
    local preset=$1
    local action=$2

    case "$action" in
        configure)
            configure_platform "$preset"
            ;;
        build)
            if [ ! -d "build/$preset" ]; then
                configure_platform "$preset"
            fi
            build_platform "$preset"
            ;;
        clean)
            clean_platform "$preset"
            ;;
        rebuild)
            rebuild_platform "$preset"
            ;;
        install)
            install_platform "$preset"
            ;;
        package)
            package_platform "$preset"
            ;;
        *)
            print_error "Unknown action: $action"
            usage
            ;;
    esac
}

# Main script
main() {
    local platform=${1:-$(detect_platform)}
    local action=${2:-build}

    # Check for help
    if [[ "$1" == "-h" || "$1" == "--help" ]]; then
        usage
    fi

    print_info "MDB-C Multi-Platform Build System"
    print_info "Platform: $platform | Action: $action"
    echo

    # Process based on platform
    if [[ "$platform" == "all" ]]; then
        # Build all supported platforms
        local platforms=("linux-x64-release" "linux-arm-release" "macos-release")

        for preset in "${platforms[@]}"; do
            echo
            print_info "=========================================="
            process_platform "$preset" "$action"
        done

        echo
        print_success "All platforms processed!"

    else
        # Build single platform
        process_platform "$platform" "$action"
    fi

    echo
    print_success "Done!"
}

# Run main
main "$@"
