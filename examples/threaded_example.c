/**
 * @file threaded_example.c
 * @brief Example: MDB Library with Threaded Serial I/O
 *
 * This example demonstrates how to integrate the MDB library with a
 * separate serial I/O thread using the callback interface.
 *
 * Architecture:
 * - Main thread: Runs MDB protocol logic
 * - Serial thread: Handles hardware I/O, feeds data to/from queues
 * - Queues: Thread-safe communication between threads
 */

/* Define POSIX and BSD features for clock_gettime, nanosleep, usleep, etc. */
#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200112L
#endif
#ifndef _DEFAULT_SOURCE
#define _DEFAULT_SOURCE
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>
#include "MDB_Callbacks.h"
#include "MDB_Linux.h"  /* For direct serial access in serial thread */
#include "Communication_Format.h"
#include "PreProcessors.h"

/* Global state */
static volatile int running = 1;
static mdb_byte_queue_t tx_queue;  /* MDB -> Serial */
static mdb_byte_queue_t rx_queue;  /* Serial -> MDB */
static pthread_mutex_t tx_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t rx_mutex = PTHREAD_MUTEX_INITIALIZER;
static struct timespec start_time;

/**
 * @brief Signal handler for graceful shutdown
 */
void signal_handler(int signum) {
    (void)signum;
    printf("\nShutting down...\n");
    running = 0;
}

/**
 * @brief Get microsecond timestamp (callback)
 */
unsigned long callback_micros(void *ctx) {
    (void)ctx;
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);

    unsigned long seconds_diff = now.tv_sec - start_time.tv_sec;
    long nsec_diff = now.tv_nsec - start_time.tv_nsec;

    return (seconds_diff * 1000000UL) + (nsec_diff / 1000UL);
}

/**
 * @brief Delay microseconds (callback)
 */
void callback_delay_us(unsigned long usec, void *ctx) {
    (void)ctx;
    struct timespec ts;
    ts.tv_sec = usec / 1000000UL;
    ts.tv_nsec = (usec % 1000000UL) * 1000UL;
    nanosleep(&ts, NULL);
}

/**
 * @brief Transmit byte (callback)
 *
 * Called by MDB library when it wants to send data.
 * Queues the data for the serial thread to transmit.
 */
uint8_t callback_tx(uint8_t mode, uint8_t data, void *ctx) {
    (void)ctx;

    pthread_mutex_lock(&tx_mutex);
    uint8_t result = mdb_queue_push(&tx_queue, mode, data);
    pthread_mutex_unlock(&tx_mutex);

    return result;
}

/**
 * @brief Receive byte (callback)
 *
 * Called by MDB library when it wants to receive data.
 * Gets data from the queue filled by the serial thread.
 */
int callback_rx(uint8_t *mode_out, uint8_t *data_out, void *ctx) {
    (void)ctx;

    pthread_mutex_lock(&rx_mutex);
    int result = mdb_queue_pop(&rx_queue, mode_out, data_out);
    pthread_mutex_unlock(&rx_mutex);

    return result ? 1 : 0;
}

/**
 * @brief Check if RX data available (callback)
 */
uint8_t callback_rx_available(void *ctx) {
    (void)ctx;

    pthread_mutex_lock(&rx_mutex);
    uint8_t available = mdb_queue_available(&rx_queue);
    pthread_mutex_unlock(&rx_mutex);

    return available;
}

/**
 * @brief Flush TX buffer (callback)
 */
uint8_t callback_tx_flush(void *ctx) {
    (void)ctx;
    /* Wait until TX queue is empty */
    while (1) {
        pthread_mutex_lock(&tx_mutex);
        uint8_t count = mdb_queue_count(&tx_queue);
        pthread_mutex_unlock(&tx_mutex);

        if (count == 0) break;
        usleep(100);  /* 100us */
    }
    return 1;
}

/**
 * @brief Serial I/O thread
 *
 * This thread handles the actual hardware communication.
 * It reads from the serial port and pushes to RX queue,
 * and pops from TX queue and writes to serial port.
 */
void* serial_thread(void *arg) {
    const char *device = (const char*)arg;

    printf("Serial thread: initializing %s\n", device);

    /* Initialize direct serial access */
    if (mdb_init(device) != 0) {
        fprintf(stderr, "Serial thread: failed to initialize serial port\n");
        return NULL;
    }

    printf("Serial thread: running\n");

    while (running) {
        /* TX: Check if there's data to send */
        pthread_mutex_lock(&tx_mutex);
        if (mdb_queue_available(&tx_queue)) {
            uint8_t mode, data;
            if (mdb_queue_pop(&tx_queue, &mode, &data)) {
                pthread_mutex_unlock(&tx_mutex);

                /* Transmit to hardware */
                tX9Bits(mode, data);
                tXBitsFlush();
            } else {
                pthread_mutex_unlock(&tx_mutex);
            }
        } else {
            pthread_mutex_unlock(&tx_mutex);
        }

        /* RX: Check if there's data from hardware */
        if (rxBitsAvailable()) {
            uint16_t rx_data = rX9Bits();

            if (rx_data != 0xFFFF) {
                uint8_t mode = (rx_data >> 8) & 0x01;
                uint8_t data = rx_data & 0xFF;

                /* Push to RX queue */
                pthread_mutex_lock(&rx_mutex);
                mdb_queue_push(&rx_queue, mode, data);
                pthread_mutex_unlock(&rx_mutex);
            }
        }

        /* Small delay to avoid busy-waiting */
        usleep(50);  /* 50 microseconds */
    }

    printf("Serial thread: closing\n");
    mdb_close();

    return NULL;
}

/**
 * @brief Main function
 */
int main(int argc, char *argv[]) {
    const char *device_path = (argc > 1) ? argv[1] : MDB_TTY_DEVICE;
    pthread_t serial_tid;

    printf("MDB Threaded Example\n");
    printf("====================\n");
    printf("Device: %s\n\n", device_path);

    /* Setup signal handlers */
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    /* Initialize timing */
    clock_gettime(CLOCK_MONOTONIC, &start_time);

    /* Initialize queues */
    mdb_queue_init(&tx_queue);
    mdb_queue_init(&rx_queue);
    tx_queue.mutex = &tx_mutex;
    rx_queue.mutex = &rx_mutex;

    /* Setup callback interface */
    mdb_callback_interface_t callbacks = {
        .tx_byte = callback_tx,
        .rx_byte = callback_rx,
        .rx_available = callback_rx_available,
        .tx_flush = callback_tx_flush,
        .get_micros = callback_micros,
        .delay_us = callback_delay_us,
        .user_ctx = NULL
    };

    if (mdb_callback_init(&callbacks) != 0) {
        fprintf(stderr, "Failed to initialize MDB callbacks\n");
        return 1;
    }

    printf("MDB callback interface initialized\n");

    /* Start serial thread */
    if (pthread_create(&serial_tid, NULL, serial_thread, (void*)device_path) != 0) {
        fprintf(stderr, "Failed to create serial thread\n");
        return 1;
    }

    printf("Serial thread started\n");
    printf("Press Ctrl+C to stop...\n\n");

    /* Main MDB processing loop */
    while (running) {
        /* Check for incoming data */
        if (callback_rx_available(NULL)) {
            /* Example: Just monitor the bus */
            uint8_t mode, data;
            if (callback_rx(&mode, &data, NULL) > 0) {
                printf("[%010lu us] ", callback_micros(NULL));
                if (mode) {
                    printf(">>> [ADDR/CMD] 0x%02X\n", data);
                } else {
                    if (data == ACK) printf("<<< [ACK]\n");
                    else if (data == NAK) printf("<<< [NAK]\n");
                    else if (data == RET) printf("<<< [RET]\n");
                    else printf("    [DATA] 0x%02X\n", data);
                }
            }
        }

        /* Here you would call your MDB device processing functions:
         * - vmc_process()
         * - cashless_process()
         * - billValidator_process()
         * etc.
         */

        usleep(1000);  /* 1ms */
    }

    /* Cleanup */
    printf("\nWaiting for serial thread to finish...\n");
    pthread_join(serial_tid, NULL);

    mdb_callback_close();

    printf("Done.\n");
    return 0;
}
