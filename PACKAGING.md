# MDB-C Packaging Guide

## Creating Distribution Packages

### Quick Commands

```bash
# Build and package for your platform
./build.sh macos-release package

# Package for Linux x64
./build.sh linux-x64-release package

# Package for Linux ARM (cross-compile)
./build.sh linux-arm-release package

# Package all platforms
./build.sh all package
```

## Package Structure

Each package contains:

```
mdb-c-[platform].tar.gz
└── [platform]/
    ├── lib/                          # Static libraries
    │   ├── libmdb_protocol.a         # Core MDB protocol (~3-4 KB)
    │   ├── libmdb_hal.a              # Direct hardware access (~5-6 KB)
    │   ├── libmdb_hal_callback.a     # Callback interface (~3-4 KB)
    │   └── pkgconfig/
    │       └── mdb-c.pc              # pkg-config metadata
    ├── include/                      # All header files
    │   └── mdb/
    │       ├── mdb.h                 # Main convenience header
    │       ├── protocol/             # Protocol layer
    │       │   ├── Communication_Format.h
    │       │   ├── Bus_Timing.h
    │       │   ├── PreProcessors.h
    │       │   ├── FTL.h
    │       │   └── EVA-DTS.h
    │       ├── hal/                  # Hardware abstraction
    │       │   ├── MDB_Linux.h
    │       │   └── MDB_Callbacks.h
    │       └── devices/              # Device implementations
    │           ├── VMC.h
    │           ├── Cashless.h
    │           ├── Coin_Changer.h
    │           ├── Bill_Validator.h
    │           ├── Coin_Hopper.h
    │           ├── Sniffer.h
    │           ├── Age_Verification_Device.h
    │           ├── Communications_Gateway.h
    │           └── Universal_Satellite_Device.h
    ├── examples/                     # Example source code
    │   ├── sniffer.c
    │   ├── threaded_example.c
    │   ├── mdb43_basket_demo.c
    │   └── test_protocol.c
    ├── docs/                         # Documentation
    │   └── INTEGRATION.md
    ├── README.md                     # Quick start guide
    └── VERSION                       # Version and build info
```

## Using the Package

### Method 1: Extract and use directly

```bash
# Extract package
tar -xzf dist/mdb-c-linux-x64-release.tar.gz
cd linux-x64-release

# Your project structure
your-project/
├── libs/
│   └── mdb-c/                    # Copy the extracted package here
│       ├── lib/
│       └── include/
└── src/
    └── main.c
```

**CMakeLists.txt:**
```cmake
cmake_minimum_required(VERSION 3.10)
project(YourApp C)

set(MDB_ROOT ${CMAKE_SOURCE_DIR}/libs/mdb-c)

include_directories(${MDB_ROOT}/include)
link_directories(${MDB_ROOT}/lib)

add_executable(your_app src/main.c)

target_link_libraries(your_app
    mdb_protocol
    mdb_hal
    pthread
    rt  # Linux only, omit on macOS
)
```

### Method 2: Install system-wide

```bash
# Extract
tar -xzf dist/mdb-c-linux-x64-release.tar.gz
cd linux-x64-release

# Install (requires sudo)
sudo cp -r include/mdb /usr/local/include/
sudo cp lib/*.a /usr/local/lib/
sudo cp lib/pkgconfig/mdb-c.pc /usr/local/lib/pkgconfig/

# Update library cache (Linux only)
sudo ldconfig
```

**Then in your project:**
```cmake
find_package(PkgConfig REQUIRED)
pkg_check_modules(MDB REQUIRED mdb-c)

add_executable(your_app src/main.c)
target_include_directories(your_app PRIVATE ${MDB_INCLUDE_DIRS})
target_link_libraries(your_app ${MDB_LIBRARIES})
```

Or with pkg-config directly:
```bash
gcc main.c $(pkg-config --cflags --libs mdb-c) -o your_app
```

### Method 3: Copy only what you need

**Minimal integration (protocol only):**
```bash
project/
├── include/
│   └── mdb/
│       └── protocol/
│           ├── Communication_Format.h
│           ├── Bus_Timing.h
│           └── PreProcessors.h
└── lib/
    └── libmdb_protocol.a
```

**With direct hardware access:**
```bash
project/
├── include/
│   └── mdb/
│       ├── protocol/
│       └── hal/
│           └── MDB_Linux.h
└── lib/
    ├── libmdb_protocol.a
    └── libmdb_hal.a
```

**Thread-safe callback version:**
```bash
project/
├── include/
│   └── mdb/
│       ├── protocol/
│       └── hal/
│           └── MDB_Callbacks.h
└── lib/
    ├── libmdb_protocol.a
    └── libmdb_hal_callback.a
```

## Simple Usage Example

**main.c:**
```c
#define _POSIX_C_SOURCE 200112L

#include <stdio.h>
#include <signal.h>
#include <mdb/hal/MDB_Linux.h>
#include <mdb/protocol/Communication_Format.h>
#include <mdb/protocol/PreProcessors.h>

volatile int running = 1;

void signal_handler(int sig) {
    running = 0;
}

int main(void) {
    signal(SIGINT, signal_handler);

    /* Initialize MDB serial port */
    if (mdb_init("/dev/ttyUSB0") != 0) {
        fprintf(stderr, "Failed to open /dev/ttyUSB0\n");
        return 1;
    }

    printf("MDB initialized on /dev/ttyUSB0\n");

    /* Main loop */
    while (running) {
        /* Example: receive data for Cashless device #1 */
        uint8_t result = rX(CASHLESS1_ADDRESS);

        if (result == RECEIVED) {
            printf("Received data for Cashless #1\n");
            /* Process data in block[] global array */
        }

        delayMicroseconds(1000);  /* 1ms delay */
    }

    mdb_close();
    printf("MDB closed\n");

    return 0;
}
```

**Compile:**
```bash
# Using extracted package
gcc -o app main.c \
    -I./mdb-c/include \
    -L./mdb-c/lib \
    -lmdb_protocol -lmdb_hal \
    -lpthread -lrt

# Or if installed system-wide
gcc -o app main.c \
    -lmdb_protocol -lmdb_hal \
    -lpthread -lrt

# Or with pkg-config
gcc -o app main.c $(pkg-config --cflags --libs mdb-c)
```

## Using the Convenience Header

**Option 1: Include everything**
```c
#include <mdb/mdb.h>
#include <mdb/hal/MDB_Linux.h>  /* Uncomment in mdb.h */
```

**Option 2: Include only what you need**
```c
#include <mdb/protocol/PreProcessors.h>
#include <mdb/protocol/Communication_Format.h>
#include <mdb/hal/MDB_Linux.h>
```

## Header Include Paths

With the package structure, use these include paths:

| What you need | Include statement |
|---------------|-------------------|
| Protocol constants | `#include <mdb/protocol/PreProcessors.h>` |
| Communication functions | `#include <mdb/protocol/Communication_Format.h>` |
| Timing functions | `#include <mdb/protocol/Bus_Timing.h>` |
| Direct hardware HAL | `#include <mdb/hal/MDB_Linux.h>` |
| Callback HAL | `#include <mdb/hal/MDB_Callbacks.h>` |
| VMC device | `#include <mdb/devices/VMC.h>` |
| Cashless device | `#include <mdb/devices/Cashless.h>` |
| Everything (convenience) | `#include <mdb/mdb.h>` |

## Platform-Specific Notes

### Linux
- Link with: `-lmdb_protocol -lmdb_hal -lpthread -lrt`
- Default device: `/dev/ttyUSB0`
- May need dialout group: `sudo usermod -a -G dialout $USER`

### macOS
- Link with: `-lmdb_protocol -lmdb_hal -lpthread` (no `-lrt`)
- Default device: `/dev/cu.usbserial` or `/dev/tty.usbserial`
- No special permissions needed

### Embedded/RTOS
- Use `libmdb_hal_callback.a` instead of `libmdb_hal.a`
- Implement callback functions for your platform
- Link with: `-lmdb_protocol -lmdb_hal_callback`
- See `docs/INTEGRATION.md` in package

## Package Sizes

| Platform | Total Size | Protocol | HAL Direct | HAL Callback |
|----------|-----------|----------|------------|--------------|
| Linux x64 | ~12 KB | 3.4 KB | 5.5 KB | 3.9 KB |
| Linux ARM | ~10 KB | 3.0 KB | 4.8 KB | 3.5 KB |
| macOS | ~12 KB | 3.4 KB | 5.5 KB | 3.9 KB |
| Embedded | ~8 KB | 3.0 KB | - | 3.5 KB |

## Distribution

You can distribute packages via:

1. **Direct download** - Upload `.tar.gz` to your server/GitHub releases
2. **Package managers** - Create `.deb`, `.rpm`, Homebrew formula
3. **Git submodule** - Users can clone and build
4. **CMake FetchContent** - Download and build automatically

### Example GitHub Release

```bash
# Tag and create release
git tag -a v4.3.0 -m "MDB-C Version 4.3.0"
git push origin v4.3.0

# Build packages for all platforms
./build.sh all package

# Upload to GitHub Releases
# dist/mdb-c-linux-x64-release.tar.gz
# dist/mdb-c-linux-arm-release.tar.gz
# dist/mdb-c-macos-release.tar.gz
```

## Updating an Existing Integration

When a new version is released:

1. **Extract new package** to temporary location
2. **Compare headers** for API changes
3. **Replace libraries** in your `lib/` directory
4. **Update headers** in your `include/mdb/` directory
5. **Rebuild** your project
6. **Test** with your application

```bash
# Backup old version
cp -r libs/mdb-c libs/mdb-c.backup

# Extract new version
tar -xzf mdb-c-linux-x64-release-v4.3.1.tar.gz -C libs/

# If everything works, remove backup
rm -rf libs/mdb-c.backup
```
