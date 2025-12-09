/**
 * @file MDB_Callbacks.h
 * @brief MDB Callback-Based I/O Interface
 *
 * This interface allows the MDB library to work with external serial threads
 * or custom I/O implementations. The library calls user-provided callbacks
 * instead of accessing hardware directly.
 *
 * Use cases:
 * - Integration with RTOS (FreeRTOS, Zephyr, etc.)
 * - Multi-threaded applications
 * - Custom transport layers (USB, TCP/IP, etc.)
 * - Hardware abstraction for testing
 */

#ifndef MDB_CALLBACKS_H
#define MDB_CALLBACKS_H

#include <stdint.h>

/**
 * @brief Callback function type for transmitting a 9-bit MDB byte
 *
 * This function is called by the MDB library when it needs to transmit
 * data on the bus. The implementation should queue the data for transmission
 * by the serial thread.
 *
 * @param mode Mode bit (0 or 1)
 * @param data Data byte (8 bits)
 * @param user_ctx User context pointer passed during initialization
 * @return Number of bytes queued (1 on success, 0 on error)
 */
typedef uint8_t (*mdb_tx_callback_t)(uint8_t mode, uint8_t data, void *user_ctx);

/**
 * @brief Callback function type for receiving a 9-bit MDB byte
 *
 * This function is called by the MDB library when it needs to receive
 * data from the bus. The implementation should check if data is available
 * from the serial thread and return it.
 *
 * @param mode_out Pointer to store the mode bit (0 or 1)
 * @param data_out Pointer to store the data byte (8 bits)
 * @param user_ctx User context pointer passed during initialization
 * @return 1 if data was available, 0 if no data, -1 on error
 */
typedef int (*mdb_rx_callback_t)(uint8_t *mode_out, uint8_t *data_out, void *user_ctx);

/**
 * @brief Callback function type for checking if RX data is available
 *
 * This function is called by the MDB library to poll for incoming data
 * without actually consuming it.
 *
 * @param user_ctx User context pointer passed during initialization
 * @return 1 if data is available, 0 if no data
 */
typedef uint8_t (*mdb_rx_available_callback_t)(void *user_ctx);

/**
 * @brief Callback function type for flushing TX buffer
 *
 * This function is called by the MDB library after transmitting data
 * to ensure all bytes have been sent before continuing.
 *
 * @param user_ctx User context pointer passed during initialization
 * @return 1 on success, 0 on error
 */
typedef uint8_t (*mdb_tx_flush_callback_t)(void *user_ctx);

/**
 * @brief Callback function type for microsecond timing
 *
 * This function should return a monotonic microsecond timestamp.
 *
 * @param user_ctx User context pointer passed during initialization
 * @return Current time in microseconds
 */
typedef unsigned long (*mdb_micros_callback_t)(void *user_ctx);

/**
 * @brief Callback function type for microsecond delay
 *
 * This function should delay execution for the specified microseconds.
 *
 * @param usec Microseconds to delay
 * @param user_ctx User context pointer passed during initialization
 */
typedef void (*mdb_delay_us_callback_t)(unsigned long usec, void *user_ctx);

/**
 * @brief MDB callback interface structure
 *
 * Contains all callback functions needed by the MDB library.
 * Initialize this structure and pass it to mdb_callback_init().
 */
typedef struct {
    mdb_tx_callback_t tx_byte;              /* Transmit 9-bit byte */
    mdb_rx_callback_t rx_byte;              /* Receive 9-bit byte */
    mdb_rx_available_callback_t rx_available; /* Check if RX data available */
    mdb_tx_flush_callback_t tx_flush;       /* Flush TX buffer */
    mdb_micros_callback_t get_micros;       /* Get microsecond timestamp */
    mdb_delay_us_callback_t delay_us;       /* Delay microseconds */
    void *user_ctx;                         /* User context pointer */
} mdb_callback_interface_t;

/**
 * @brief Initialize MDB library with callback interface
 *
 * Call this instead of mdb_init() when using callback-based I/O.
 *
 * @param callbacks Pointer to callback interface structure
 * @return 0 on success, -1 on error
 */
int mdb_callback_init(const mdb_callback_interface_t *callbacks);

/**
 * @brief Close callback-based MDB interface
 *
 * @return 0 on success, -1 on error
 */
int mdb_callback_close(void);

/**
 * @brief Get the current callback interface
 *
 * @return Pointer to current callback interface, or NULL if not initialized
 */
const mdb_callback_interface_t* mdb_callback_get_interface(void);

/* Thread-safe queue structures for serial thread integration */

/**
 * @brief Thread-safe byte queue for TX/RX buffering
 *
 * This structure can be used in the user's serial thread to buffer
 * data between the MDB library and the hardware.
 */
typedef struct {
    uint16_t buffer[256];   /* 9-bit data buffer (8 data + 1 mode) */
    uint8_t head;           /* Write position */
    uint8_t tail;           /* Read position */
    volatile uint16_t count; /* Number of items in queue (0-256) */
    void *mutex;            /* Mutex for thread safety (user-managed) */
} mdb_byte_queue_t;

/**
 * @brief Initialize a byte queue
 *
 * @param queue Pointer to queue structure
 */
void mdb_queue_init(mdb_byte_queue_t *queue);

/**
 * @brief Push a 9-bit value to the queue (thread-safe if mutex used)
 *
 * @param queue Pointer to queue structure
 * @param mode Mode bit (0 or 1)
 * @param data Data byte (8 bits)
 * @return 1 on success, 0 if queue full
 */
uint8_t mdb_queue_push(mdb_byte_queue_t *queue, uint8_t mode, uint8_t data);

/**
 * @brief Pop a 9-bit value from the queue (thread-safe if mutex used)
 *
 * @param queue Pointer to queue structure
 * @param mode_out Pointer to store mode bit
 * @param data_out Pointer to store data byte
 * @return 1 on success, 0 if queue empty
 */
uint8_t mdb_queue_pop(mdb_byte_queue_t *queue, uint8_t *mode_out, uint8_t *data_out);

/**
 * @brief Check if queue has data available
 *
 * @param queue Pointer to queue structure
 * @return 1 if data available, 0 if empty
 */
uint8_t mdb_queue_available(mdb_byte_queue_t *queue);

/**
 * @brief Get number of items in queue
 *
 * @param queue Pointer to queue structure
 * @return Number of items
 */
uint8_t mdb_queue_count(mdb_byte_queue_t *queue);

/**
 * @brief Clear the queue
 *
 * @param queue Pointer to queue structure
 */
void mdb_queue_clear(mdb_byte_queue_t *queue);

#endif /* MDB_CALLBACKS_H */
