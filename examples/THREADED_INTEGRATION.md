# MDB Threaded Integration Guide

This guide explains how to integrate the MDB-C library into multi-threaded applications using the callback interface.

## Overview

The callback-based interface allows you to:
- Run MDB protocol logic in one thread
- Handle serial I/O in a separate thread
- Integrate with RTOS (FreeRTOS, Zephyr, ThreadX, etc.)
- Use custom transport layers (USB, TCP/IP, etc.)
- Test without hardware using mock implementations

## Architecture

```
┌─────────────────┐         ┌──────────────────┐
│   MDB Thread    │         │  Serial Thread   │
│  (Protocol)     │         │   (Hardware)     │
├─────────────────┤         ├──────────────────┤
│                 │         │                  │
│  vmc_process()  │         │  Hardware Read   │
│  cashless_...() │         │        │         │
│       │         │         │        ↓         │
│       ↓         │         │   RX Queue ←──   │
│  MDB Callbacks  │         │        │         │
│       │         │         │        ↓         │
│       ↓         │         │  Queue → Lib     │
│   TX Queue   ←──┼────────→│                  │
│       │         │         │  Lib → Queue     │
│       ↓         │         │        │         │
│  Lib ← Queue    │         │        ↓         │
│                 │         │   TX Queue ──→   │
│                 │         │        │         │
│                 │         │        ↓         │
│                 │         │  Hardware Write  │
└─────────────────┘         └──────────────────┘
```

## Building

### Static Library with Callbacks

```bash
mkdir build && cd build
cmake ..
make
```

This creates:
- `libmdb_protocol.a` - Core MDB protocol (thread-safe)
- `libmdb_hal_callback.a` - Callback-based HAL
- `mdb_threaded` - Example application

### Using in Your Project

#### Option 1: Link against libraries

```cmake
target_link_libraries(your_app
    PRIVATE
    mdb_protocol
    mdb_hal_callback
    pthread  # Or your RTOS threading library
)
```

#### Option 2: Include as subdirectory

```cmake
add_subdirectory(path/to/MDB-C)
target_link_libraries(your_app PRIVATE mdb_core_callback pthread)
```

## Usage Example

### 1. Define Callback Functions

```c
#include "MDB_Callbacks.h"

/* TX Queue (thread-safe) */
mdb_byte_queue_t tx_queue;
mdb_byte_queue_t rx_queue;
pthread_mutex_t tx_mutex;
pthread_mutex_t rx_mutex;

/* TX Callback: Queue data for serial thread */
uint8_t my_tx_callback(uint8_t mode, uint8_t data, void *ctx) {
    pthread_mutex_lock(&tx_mutex);
    uint8_t result = mdb_queue_push(&tx_queue, mode, data);
    pthread_mutex_unlock(&tx_mutex);
    return result;
}

/* RX Callback: Get data from serial thread */
int my_rx_callback(uint8_t *mode, uint8_t *data, void *ctx) {
    pthread_mutex_lock(&rx_mutex);
    int result = mdb_queue_pop(&rx_queue, mode, data);
    pthread_mutex_unlock(&rx_mutex);
    return result ? 1 : 0;
}

/* RX Available Callback: Check if data ready */
uint8_t my_rx_available(void *ctx) {
    pthread_mutex_lock(&rx_mutex);
    uint8_t available = mdb_queue_available(&rx_queue);
    pthread_mutex_unlock(&rx_mutex);
    return available;
}

/* Timing Callbacks */
unsigned long my_micros(void *ctx) {
    /* Return microsecond timestamp */
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000000UL + ts.tv_nsec / 1000UL;
}

void my_delay_us(unsigned long usec, void *ctx) {
    struct timespec ts = {
        .tv_sec = usec / 1000000UL,
        .tv_nsec = (usec % 1000000UL) * 1000UL
    };
    nanosleep(&ts, NULL);
}
```

### 2. Initialize Callback Interface

```c
int main() {
    /* Initialize queues */
    mdb_queue_init(&tx_queue);
    mdb_queue_init(&rx_queue);

    /* Setup callbacks */
    mdb_callback_interface_t callbacks = {
        .tx_byte = my_tx_callback,
        .rx_byte = my_rx_callback,
        .rx_available = my_rx_available,
        .tx_flush = NULL,  /* Optional */
        .get_micros = my_micros,
        .delay_us = my_delay_us,
        .user_ctx = NULL
    };

    /* Initialize MDB library */
    if (mdb_callback_init(&callbacks) != 0) {
        fprintf(stderr, "Failed to initialize MDB\n");
        return 1;
    }

    /* Now you can use MDB functions normally */
    /* They will call your callbacks instead of hardware */
}
```

### 3. Serial Thread Implementation

```c
void* serial_thread(void *arg) {
    /* Initialize hardware (UART, USB, etc.) */
    int serial_fd = open("/dev/ttyUSB0", O_RDWR);
    /* Configure serial port... */

    while (running) {
        /* TX: Check queue and send to hardware */
        pthread_mutex_lock(&tx_mutex);
        if (mdb_queue_available(&tx_queue)) {
            uint8_t mode, data;
            mdb_queue_pop(&tx_queue, &mode, &data);
            pthread_mutex_unlock(&tx_mutex);

            /* Write to hardware with mode bit encoding */
            write_9bit_to_serial(serial_fd, mode, data);
        } else {
            pthread_mutex_unlock(&tx_mutex);
        }

        /* RX: Read from hardware and push to queue */
        if (data_available(serial_fd)) {
            uint8_t mode, data;
            read_9bit_from_serial(serial_fd, &mode, &data);

            pthread_mutex_lock(&rx_mutex);
            mdb_queue_push(&rx_queue, mode, data);
            pthread_mutex_unlock(&rx_mutex);
        }
    }

    return NULL;
}
```

### 4. Main MDB Thread

```c
int main() {
    /* Start serial thread */
    pthread_t serial_tid;
    pthread_create(&serial_tid, NULL, serial_thread, NULL);

    /* MDB processing loop */
    while (running) {
        /* Process MDB devices */
        vmc_process();          /* VMC coordinator */
        cashless_process(&cashless1);  /* Cashless device */

        /* Or just monitor the bus */
        if (callback_rx_available(NULL)) {
            uint8_t mode, data;
            callback_rx(&mode, &data, NULL);
            /* Handle received data */
        }

        usleep(1000);  /* 1ms */
    }

    pthread_join(serial_tid, NULL);
    mdb_callback_close();
}
```

## RTOS Integration

### FreeRTOS Example

```c
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

QueueHandle_t tx_queue_handle;
QueueHandle_t rx_queue_handle;

/* TX Callback for FreeRTOS */
uint8_t freertos_tx(uint8_t mode, uint8_t data, void *ctx) {
    uint16_t value = ((uint16_t)mode << 8) | data;
    return (xQueueSend(tx_queue_handle, &value, 0) == pdTRUE) ? 1 : 0;
}

/* RX Callback for FreeRTOS */
int freertos_rx(uint8_t *mode, uint8_t *data, void *ctx) {
    uint16_t value;
    if (xQueueReceive(rx_queue_handle, &value, 0) == pdTRUE) {
        *mode = (value >> 8) & 0x01;
        *data = value & 0xFF;
        return 1;
    }
    return 0;
}

void vMDBTask(void *pvParameters) {
    while (1) {
        vmc_process();
        vTaskDelay(pdMS_TO_TICKS(1));
    }
}

void vSerialTask(void *pvParameters) {
    /* Handle UART I/O */
}
```

## Thread Safety

### What's Thread-Safe

✅ **Safe to call from different threads:**
- All callback functions (with proper queue locking)
- `mdb_queue_*` functions (with external mutex)
- MDB protocol functions (when using callbacks)

### What Requires Synchronization

⚠️ **Requires user synchronization:**
- Queue access (use mutexes)
- Device state structures (if shared)
- Global MDB buffers (if accessed directly)

### Best Practices

1. **Separate concerns:**
   - MDB thread: Protocol logic only
   - Serial thread: Hardware I/O only

2. **Use lock-free queues if possible:**
   - For RTOS, use native queue primitives
   - For Linux, consider `eventfd` or `pipe`

3. **Minimize lock time:**
   - Lock only during queue operations
   - Process data outside locks

4. **Handle overflow:**
   - Check queue full conditions
   - Implement backpressure if needed

## Performance Considerations

### Queue Sizes

The default queue size is 256 bytes. Adjust if needed:

```c
typedef struct {
    uint16_t buffer[512];  /* Larger buffer */
    /* ... */
} mdb_byte_queue_t;
```

### Polling vs Interrupts

**Serial Thread Options:**

1. **Polling** (simple, higher CPU usage):
   ```c
   while (running) {
       check_and_process();
       usleep(50);  /* 50us poll interval */
   }
   ```

2. **Interrupt-driven** (efficient, complex):
   ```c
   /* UART interrupt pushes to queue */
   void uart_rx_interrupt() {
       uint8_t data = UART_RX_REG;
       mdb_queue_push(&rx_queue, get_mode(), data);
   }
   ```

3. **select/poll** (Linux):
   ```c
   while (running) {
       fd_set rfds;
       FD_SET(serial_fd, &rfds);
       select(serial_fd + 1, &rfds, NULL, NULL, &timeout);
       /* Process I/O */
   }
   ```

## Distributing as Precompiled Library

### Build for Distribution

```bash
cmake -DCMAKE_BUILD_TYPE=Release \
      -DBUILD_EXAMPLES=OFF \
      -DBUILD_TESTS=OFF \
      ..
make
```

### Files to Distribute

```
lib/
  libmdb_protocol.a         # Core protocol
  libmdb_hal_callback.a     # Callback HAL
include/mdb/
  MDB_Callbacks.h           # Public API
  Communication_Format.h    # Protocol interface
  PreProcessors.h           # Constants
  Bus_Timing.h              # Timing
  devices/                  # Device headers
    VMC.h
    Cashless.h
    Bill_Validator.h
    Coin_Changer.h
```

### Integration Instructions

```c
// 1. Link libraries
// gcc your_app.c -lmdb_protocol -lmdb_hal_callback -lpthread

// 2. Include headers
#include <mdb/MDB_Callbacks.h>
#include <mdb/devices/VMC.h>

// 3. Implement callbacks and use library
```

## Testing Without Hardware

Create mock callbacks for testing:

```c
uint8_t mock_tx(uint8_t mode, uint8_t data, void *ctx) {
    printf("TX: mode=%d data=0x%02X\n", mode, data);
    return 1;
}

int mock_rx(uint8_t *mode, uint8_t *data, void *ctx) {
    /* Return test data */
    *mode = test_mode;
    *data = test_data;
    return 1;
}
```

## Troubleshooting

### Queue Overflow
**Symptom:** Lost data, dropped commands
**Solution:** Increase queue size or speed up serial thread

### Timing Issues
**Symptom:** Protocol timeouts, DISORDERED errors
**Solution:** Reduce serial thread latency, check micros() implementation

### Deadlocks
**Symptom:** Application hangs
**Solution:** Always unlock mutexes, use consistent lock ordering

## See Also

- `threaded_example.c` - Complete working example
- `MDB_Callbacks.h` - Full API documentation
- `CLAUDE.md` - MDB protocol details
