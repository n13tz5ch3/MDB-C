/**
 * @file MDB_Linux.h
 * @brief Linux TTY Hardware Abstraction Layer for MDB Protocol
 *
 * This module provides serial communication functions for MDB protocol
 * using POSIX termios API. Supports 9-bit communication via parity
 * bit manipulation.
 */

#ifndef MDB_LINUX_H
#define MDB_LINUX_H

#include <stdint.h>
#include <stdbool.h>

/* Parity mode constants for 9-bit emulation */
#define ODD_PARITY  0x01
#define EVEN_PARITY 0x00
#define PARITY_ERROR 0x04

/**
 * @brief Initialize the MDB serial interface
 *
 * Opens and configures the TTY device for MDB communication:
 * - 9600 baud
 * - 8 data bits
 * - Odd parity (default)
 * - 1 stop bit
 * - Non-blocking I/O
 *
 * @param device_path Path to TTY device (e.g., "/dev/ttyUSB0")
 * @return 0 on success, -1 on error
 */
int mdb_init(const char *device_path);

/**
 * @brief Close the MDB serial interface
 *
 * @return 0 on success, -1 on error
 */
int mdb_close(void);

/**
 * @brief Check if data is available for reading
 *
 * @return 1 if data available, 0 if no data, -1 on error
 */
uint8_t rxBitsAvailable(void);

/**
 * @brief Flush the transmit buffer
 *
 * Waits until all pending output has been transmitted.
 *
 * @return 0 on success, -1 on error
 */
uint8_t tXBitsFlush(void);

/**
 * @brief Transmit a 9-bit MDB byte
 *
 * Transmits 8 data bits with mode bit encoded in parity.
 * For mode bit = 0: normal odd parity
 * For mode bit = 1: inverted parity (even parity)
 *
 * @param mdbMode Mode bit (0 or 1)
 * @param mdbData Data byte (8 bits)
 * @return Number of bytes written (1), or -1 on error
 */
uint8_t tX9Bits(uint8_t mdbMode, uint8_t mdbData);

/**
 * @brief Receive a 9-bit MDB byte
 *
 * Receives 8 data bits and extracts mode bit from parity error.
 * Returns a 16-bit value with mode bit in bit 8.
 *
 * Format: 0x0000_00MX_DDDD_DDDD
 *         M = mode bit (bit 8)
 *         D = data bits (bits 0-7)
 *
 * @return 16-bit value with mode bit and data, or 0xFFFF on error
 */
uint16_t rX9Bits(void);

/**
 * @brief Calculate odd parity for a byte
 *
 * @param raw Input byte
 * @return 1 if odd parity (odd number of 1s), 0 if even parity
 */
uint8_t parityBitCalc(uint8_t raw);

/**
 * @brief Get microsecond timestamp
 *
 * Returns monotonic time in microseconds for timing operations.
 *
 * @return Current time in microseconds
 */
unsigned long micros(void);

/**
 * @brief Sleep for specified microseconds
 *
 * @param usec Microseconds to sleep
 */
void delayMicroseconds(unsigned long usec);

/**
 * @brief Set serial port parity mode
 *
 * Dynamically changes parity mode for next transmission.
 * Used internally by tX9Bits to encode mode bit.
 *
 * @param parity ODD_PARITY or EVEN_PARITY
 * @return 0 on success, -1 on error
 */
int set_parity(uint8_t parity);

/**
 * @brief Get file descriptor for the serial port
 *
 * Useful for advanced operations like select() or poll().
 *
 * @return File descriptor, or -1 if not initialized
 */
int mdb_get_fd(void);

/* Debug functions */
#ifdef MDB_VERBOSE_LOGGING
/**
 * @brief Print debug message
 *
 * @param format Printf-style format string
 * @param ... Variable arguments
 */
void mdb_debug(const char *format, ...);
#else
#define mdb_debug(...)
#endif

#endif /* MDB_LINUX_H */
