/**
 * @file MDB_Callbacks.c
 * @brief MDB Callback-Based I/O Implementation
 */

#include "MDB_Callbacks.h"
#include <string.h>
#include <stdlib.h>

/* Global callback interface */
static mdb_callback_interface_t global_callbacks = {0};
static int callbacks_initialized = 0;

/**
 * @brief Initialize MDB library with callback interface
 */
int mdb_callback_init(const mdb_callback_interface_t *callbacks) {
    if (!callbacks) {
        return -1;
    }

    /* Validate required callbacks */
    if (!callbacks->tx_byte || !callbacks->rx_byte ||
        !callbacks->rx_available || !callbacks->get_micros) {
        return -1;
    }

    /* Copy callbacks */
    memcpy(&global_callbacks, callbacks, sizeof(mdb_callback_interface_t));
    callbacks_initialized = 1;

    return 0;
}

/**
 * @brief Close callback-based MDB interface
 */
int mdb_callback_close(void) {
    if (!callbacks_initialized) {
        return -1;
    }

    memset(&global_callbacks, 0, sizeof(mdb_callback_interface_t));
    callbacks_initialized = 0;

    return 0;
}

/**
 * @brief Get the current callback interface
 */
const mdb_callback_interface_t* mdb_callback_get_interface(void) {
    if (!callbacks_initialized) {
        return NULL;
    }
    return &global_callbacks;
}

/* Wrapper functions that can be used by MDB protocol code */

/**
 * @brief Transmit 9-bit byte via callback
 */
uint8_t tX9Bits(uint8_t mdbMode, uint8_t mdbData) {
    if (!callbacks_initialized || !global_callbacks.tx_byte) {
        return 0;
    }
    return global_callbacks.tx_byte(mdbMode, mdbData, global_callbacks.user_ctx);
}

/**
 * @brief Receive 9-bit byte via callback
 */
uint16_t rX9Bits(void) {
    if (!callbacks_initialized || !global_callbacks.rx_byte) {
        return 0xFFFF;
    }

    uint8_t mode, data;
    int result = global_callbacks.rx_byte(&mode, &data, global_callbacks.user_ctx);

    if (result <= 0) {
        return 0xFFFF;  /* No data or error */
    }

    /* Return 16-bit value: bit 8 = mode bit, bits 0-7 = data */
    return ((uint16_t)mode << 8) | data;
}

/**
 * @brief Check if RX data is available via callback
 */
uint8_t rxBitsAvailable(void) {
    if (!callbacks_initialized || !global_callbacks.rx_available) {
        return 0;
    }
    return global_callbacks.rx_available(global_callbacks.user_ctx);
}

/**
 * @brief Flush TX buffer via callback
 */
uint8_t tXBitsFlush(void) {
    if (!callbacks_initialized) {
        return 1;  /* No-op if no callback */
    }

    if (!global_callbacks.tx_flush) {
        return 1;  /* Optional callback */
    }

    return global_callbacks.tx_flush(global_callbacks.user_ctx);
}

/**
 * @brief Get microsecond timestamp via callback
 */
unsigned long micros(void) {
    if (!callbacks_initialized || !global_callbacks.get_micros) {
        return 0;
    }
    return global_callbacks.get_micros(global_callbacks.user_ctx);
}

/**
 * @brief Delay microseconds via callback
 */
void delayMicroseconds(unsigned long usec) {
    if (!callbacks_initialized) {
        return;
    }

    if (!global_callbacks.delay_us) {
        return;  /* Optional callback */
    }

    global_callbacks.delay_us(usec, global_callbacks.user_ctx);
}

/* Thread-safe queue implementation */

/**
 * @brief Initialize a byte queue
 */
void mdb_queue_init(mdb_byte_queue_t *queue) {
    if (!queue) return;

    memset(queue->buffer, 0, sizeof(queue->buffer));
    queue->head = 0;
    queue->tail = 0;
    queue->count = 0;
    queue->mutex = NULL;  /* User must set this if needed */
}

/**
 * @brief Push a 9-bit value to the queue
 */
uint8_t mdb_queue_push(mdb_byte_queue_t *queue, uint8_t mode, uint8_t data) {
    if (!queue) return 0;

    /* Check if queue is full */
    if (queue->count >= 256) {
        return 0;
    }

    /* Combine mode and data into 16-bit value */
    uint16_t value = ((uint16_t)mode << 8) | data;

    /* Add to queue */
    queue->buffer[queue->head] = value;
    queue->head = (queue->head + 1) & 0xFF;  /* Wrap around at 256 */
    queue->count++;

    return 1;
}

/**
 * @brief Pop a 9-bit value from the queue
 */
uint8_t mdb_queue_pop(mdb_byte_queue_t *queue, uint8_t *mode_out, uint8_t *data_out) {
    if (!queue || !mode_out || !data_out) return 0;

    /* Check if queue is empty */
    if (queue->count == 0) {
        return 0;
    }

    /* Get value from queue */
    uint16_t value = queue->buffer[queue->tail];
    queue->tail = (queue->tail + 1) & 0xFF;  /* Wrap around at 256 */
    queue->count--;

    /* Extract mode and data */
    *mode_out = (value >> 8) & 0x01;
    *data_out = value & 0xFF;

    return 1;
}

/**
 * @brief Check if queue has data available
 */
uint8_t mdb_queue_available(mdb_byte_queue_t *queue) {
    if (!queue) return 0;
    return (queue->count > 0) ? 1 : 0;
}

/**
 * @brief Get number of items in queue
 */
uint8_t mdb_queue_count(mdb_byte_queue_t *queue) {
    if (!queue) return 0;
    return queue->count;
}

/**
 * @brief Clear the queue
 */
void mdb_queue_clear(mdb_byte_queue_t *queue) {
    if (!queue) return;

    queue->head = 0;
    queue->tail = 0;
    queue->count = 0;
}
