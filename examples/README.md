# MDB-C Examples

This directory contains example applications demonstrating different ways to use the MDB-C library.

## Examples

### 1. mdb_sniffer - Bus Monitor
**File:** `sniffer.c`

A real-time MDB bus monitoring tool that passively observes all traffic.

**Features:**
- Real-time traffic display
- Device address decoding
- Mode bit detection
- Timestamp logging
- ACK/NAK/RET detection

**Usage:**
```bash
./mdb_sniffer --device /dev/ttyUSB0 --verbose
```

**Use case:** Debugging MDB communications, reverse engineering protocols

---

### 2. mdb_test - Protocol Tests
**File:** `test_protocol.c`

Basic unit tests for MDB protocol functions without requiring hardware.

**Tests:**
- Parity calculation
- Block clear function
- 9-bit data union
- Address decoding

**Usage:**
```bash
./mdb_test
```

**Use case:** Verifying protocol implementation, regression testing

---

### 3. mdb_threaded - Multi-threaded Integration ⭐ **NEW**
**File:** `threaded_example.c`

**Complete example showing how to integrate MDB library with separate I/O thread using callbacks.**

**Architecture:**
```
Main Thread (MDB Protocol) ←→ Queues ←→ Serial Thread (Hardware I/O)
```

**Features:**
- Callback-based I/O abstraction
- Thread-safe TX/RX queues
- pthread integration
- Separates protocol logic from hardware

**Usage:**
```bash
./mdb_threaded /dev/ttyUSB0
```

**Use cases:**
- **RTOS Integration** (FreeRTOS, Zephyr, ThreadX)
- **Multi-threaded applications**
- **Custom transport layers** (USB, TCP/IP, virtual serial)
- **Testing without hardware** (mock implementations)
- **Precompiled library distribution**

**See:** `THREADED_INTEGRATION.md` for detailed integration guide

---

## Quick Start

### Build all examples:
```bash
cd MDB-C
mkdir build && cd build
cmake ..
make
```

Executables will be in `build/examples/`:
- `mdb_sniffer`
- `mdb_test`
- `mdb_threaded`

### Run examples:

**Bus monitoring:**
```bash
./build/examples/mdb_sniffer --device /dev/ttyUSB0
```

**Protocol tests:**
```bash
./build/examples/mdb_test
```

**Threaded example:**
```bash
./build/examples/mdb_threaded /dev/ttyUSB0
```

## Library Variants

The project provides two library variants:

### 1. Direct Hardware Access
**Libraries:** `libmdb_protocol.a` + `libmdb_hal.a`

For simple, single-threaded applications with direct serial port access.

**Link:**
```cmake
target_link_libraries(your_app PRIVATE mdb_core)
```

**Examples:** `mdb_sniffer`, `mdb_test`

---

### 2. Callback-Based (Thread-Safe) ⭐
**Libraries:** `libmdb_protocol.a` + `libmdb_hal_callback.a`

For multi-threaded applications, RTOS integration, or custom I/O.

**Link:**
```cmake
target_link_libraries(your_app PRIVATE mdb_core_callback pthread)
```

**Example:** `mdb_threaded`

**Advantages:**
- ✅ Thread-safe
- ✅ Hardware abstraction
- ✅ RTOS compatible
- ✅ Testable without hardware
- ✅ Distributable as precompiled library

---

## Integration Guides

- **Threaded Applications:** See `THREADED_INTEGRATION.md`
- **MDB Protocol Details:** See `../CLAUDE.md`
- **Build System:** See `../CMakeLists.txt`

## Serial Port Permissions

On Linux, add your user to the `dialout` group:
```bash
sudo usermod -a -G dialout $USER
# Log out and back in
```

Or use udev rules:
```bash
# /etc/udev/rules.d/99-mdb-serial.rules
SUBSYSTEM=="tty", ATTRS{idVendor}=="0403", MODE="0666"
```

## Recommended Hardware

- **USB-to-Serial:** FTDI FT232R, CP2102, CH340
- **Requirements:** Configurable parity support for 9-bit emulation
- **Baud rate:** 9600 baud, 8-O-1 (8 data bits, odd parity, 1 stop bit)

## Next Steps

1. **Start with `mdb_test`** to verify the library works
2. **Use `mdb_sniffer`** to monitor your MDB bus
3. **Study `mdb_threaded`** for production integration
4. **Read `THREADED_INTEGRATION.md`** for advanced usage
5. **Implement your device** using the device headers in `Devices/`

## Support

For issues or questions:
- Check `CLAUDE.md` for protocol details
- Review `THREADED_INTEGRATION.md` for integration help
- See device headers in `Devices/` for API reference
