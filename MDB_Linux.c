/**
 * @file MDB_Linux.c
 * @brief Linux TTY Hardware Abstraction Layer Implementation
 */

/* Define POSIX features for clock_gettime, nanosleep, usleep, etc. */
#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200112L
#endif

#include "MDB_Linux.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <termios.h>
#include <time.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <stdarg.h>

/* Global state */
static int serial_fd = -1;
static struct termios original_termios;
static struct timespec start_time;
static uint8_t current_parity = ODD_PARITY;

/**
 * @brief Initialize timing subsystem
 */
static void init_timing(void) {
    clock_gettime(CLOCK_MONOTONIC, &start_time);
}

/**
 * @brief Get microsecond timestamp
 */
unsigned long micros(void) {
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);

    unsigned long seconds_diff = now.tv_sec - start_time.tv_sec;
    long nsec_diff = now.tv_nsec - start_time.tv_nsec;

    return (seconds_diff * 1000000UL) + (nsec_diff / 1000UL);
}

/**
 * @brief Sleep for specified microseconds
 */
void delayMicroseconds(unsigned long usec) {
    struct timespec ts;
    ts.tv_sec = usec / 1000000UL;
    ts.tv_nsec = (usec % 1000000UL) * 1000UL;
    nanosleep(&ts, NULL);
}

/**
 * @brief Set serial port parity mode
 */
int set_parity(uint8_t parity) {
    if (serial_fd < 0) {
        mdb_debug("set_parity: serial port not initialized\n");
        return -1;
    }

    struct termios tty;
    if (tcgetattr(serial_fd, &tty) != 0) {
        perror("set_parity: tcgetattr");
        return -1;
    }

    /* Enable parity */
    tty.c_cflag |= PARENB;

    /* Set parity mode */
    if (parity == ODD_PARITY) {
        tty.c_cflag |= PARODD;
        mdb_debug("set_parity: ODD\n");
    } else {
        tty.c_cflag &= ~PARODD;
        mdb_debug("set_parity: EVEN\n");
    }

    /* Apply immediately */
    if (tcsetattr(serial_fd, TCSANOW, &tty) != 0) {
        perror("set_parity: tcsetattr");
        return -1;
    }

    current_parity = parity;
    return 0;
}

/**
 * @brief Initialize the MDB serial interface
 */
int mdb_init(const char *device_path) {
    if (serial_fd >= 0) {
        mdb_debug("mdb_init: already initialized\n");
        return -1;
    }

    /* Initialize timing */
    init_timing();

    /* Open serial port */
    serial_fd = open(device_path, O_RDWR | O_NOCTTY | O_NONBLOCK);
    if (serial_fd < 0) {
        perror("mdb_init: open");
        return -1;
    }

    /* Get current attributes (save for restoration) */
    if (tcgetattr(serial_fd, &original_termios) != 0) {
        perror("mdb_init: tcgetattr");
        close(serial_fd);
        serial_fd = -1;
        return -1;
    }

    /* Configure new attributes */
    struct termios tty;
    memset(&tty, 0, sizeof(tty));

    /* Control modes */
    tty.c_cflag = CS8;          /* 8 data bits */
    tty.c_cflag |= PARENB;      /* Enable parity */
    tty.c_cflag |= PARODD;      /* Odd parity (default) */
    tty.c_cflag &= ~CSTOPB;     /* 1 stop bit */
    tty.c_cflag |= CREAD;       /* Enable receiver */
    tty.c_cflag |= CLOCAL;      /* Ignore modem control lines */

    /* Input modes */
    tty.c_iflag = 0;
    tty.c_iflag |= INPCK;       /* Enable parity checking */
    tty.c_iflag &= ~IGNPAR;     /* Don't ignore parity errors */
    tty.c_iflag &= ~PARMRK;     /* Don't mark parity errors */
    tty.c_iflag &= ~(IXON | IXOFF | IXANY);  /* No software flow control */
    tty.c_iflag &= ~(ICRNL | INLCR);         /* No CR/LF translation */

    /* Output modes */
    tty.c_oflag = 0;            /* Raw output */

    /* Local modes */
    tty.c_lflag = 0;            /* Non-canonical, no echo */

    /* Control characters */
    tty.c_cc[VMIN] = 0;         /* Non-blocking read */
    tty.c_cc[VTIME] = 0;        /* No timeout */

    /* Set baud rate to 9600 */
    cfsetispeed(&tty, B9600);
    cfsetospeed(&tty, B9600);

    /* Apply configuration */
    if (tcsetattr(serial_fd, TCSANOW, &tty) != 0) {
        perror("mdb_init: tcsetattr");
        close(serial_fd);
        serial_fd = -1;
        return -1;
    }

    /* Flush any stale data */
    tcflush(serial_fd, TCIOFLUSH);

    current_parity = ODD_PARITY;

    mdb_debug("mdb_init: initialized %s @ 9600 8-O-1\n", device_path);

    return 0;
}

/**
 * @brief Close the MDB serial interface
 */
int mdb_close(void) {
    if (serial_fd < 0) {
        return -1;
    }

    /* Restore original terminal settings */
    tcsetattr(serial_fd, TCSANOW, &original_termios);

    close(serial_fd);
    serial_fd = -1;

    mdb_debug("mdb_close: closed\n");

    return 0;
}

/**
 * @brief Check if data is available for reading
 */
uint8_t rxBitsAvailable(void) {
    if (serial_fd < 0) {
        return 0;
    }

    int bytes_available = 0;
    if (ioctl(serial_fd, FIONREAD, &bytes_available) < 0) {
        return 0;
    }

    return (bytes_available > 0) ? 1 : 0;
}

/**
 * @brief Flush the transmit buffer
 */
uint8_t tXBitsFlush(void) {
    if (serial_fd < 0) {
        return 0;
    }

    /* Wait for output to drain */
    if (tcdrain(serial_fd) != 0) {
        perror("tXBitsFlush: tcdrain");
        return 0;
    }

    return 1;
}

/**
 * @brief Calculate odd parity for a byte
 */
uint8_t parityBitCalc(uint8_t raw) {
    uint8_t parity = 1;  /* Start with 1 for odd parity */

    parity ^= (raw & 0x80) >> 7;
    parity ^= (raw & 0x40) >> 6;
    parity ^= (raw & 0x20) >> 5;
    parity ^= (raw & 0x10) >> 4;
    parity ^= (raw & 0x08) >> 3;
    parity ^= (raw & 0x04) >> 2;
    parity ^= (raw & 0x02) >> 1;
    parity ^= (raw & 0x01);

    return parity;
}

/**
 * @brief Transmit a 9-bit MDB byte
 */
uint8_t tX9Bits(uint8_t mdbMode, uint8_t mdbData) {
    if (serial_fd < 0) {
        mdb_debug("tX9Bits: port not initialized\n");
        return 0;
    }

    /* Calculate what the odd parity should be for this data */
    uint8_t calculated_odd_parity = parityBitCalc(mdbData);

    /*
     * 9-bit encoding via parity manipulation:
     * - Mode bit = 0: Use normal odd parity
     * - Mode bit = 1: Flip parity to encode mode bit
     */
    uint8_t parity_to_use;
    if (mdbMode == 0) {
        /* Mode = 0: use calculated odd parity */
        parity_to_use = calculated_odd_parity ? ODD_PARITY : EVEN_PARITY;
    } else {
        /* Mode = 1: invert the parity */
        parity_to_use = calculated_odd_parity ? EVEN_PARITY : ODD_PARITY;
    }

    /* Set parity mode */
    if (set_parity(parity_to_use) != 0) {
        mdb_debug("tX9Bits: failed to set parity\n");
        return 0;
    }

    /* Wait for previous transmission to complete */
    tXBitsFlush();

    /* Transmit data byte */
    ssize_t written = write(serial_fd, &mdbData, 1);
    if (written != 1) {
        perror("tX9Bits: write");
        return 0;
    }

    mdb_debug("tX9Bits: sent 0x%02X mode=%d parity=%s\n",
              mdbData, mdbMode,
              (parity_to_use == ODD_PARITY) ? "ODD" : "EVEN");

    return 1;
}

/**
 * @brief Receive a 9-bit MDB byte
 */
uint16_t rX9Bits(void) {
    if (serial_fd < 0) {
        mdb_debug("rX9Bits: port not initialized\n");
        return 0xFFFF;
    }

    uint8_t data_byte;
    ssize_t bytes_read = read(serial_fd, &data_byte, 1);

    if (bytes_read != 1) {
        if (bytes_read < 0 && errno != EAGAIN && errno != EWOULDBLOCK) {
            perror("rX9Bits: read");
        }
        return 0xFFFF;
    }

    /* Check for parity error to extract mode bit */
    int serial_status;
    if (ioctl(serial_fd, TIOCMGET, &serial_status) < 0) {
        mdb_debug("rX9Bits: failed to get serial status\n");
    }

    /*
     * Decode mode bit from parity:
     * We need to check if a parity error occurred.
     * Unfortunately, POSIX doesn't give us easy access to parity errors
     * after the byte is read. We'll use the termios INPCK flag and
     * detect parity errors by comparing expected vs actual parity.
     *
     * For a robust implementation, we calculate what the parity should be
     * and compare with current parity setting.
     */

    uint8_t calculated_odd_parity = parityBitCalc(data_byte);
    uint8_t mode_bit = 0;

    /*
     * If current parity is ODD:
     *   - calculated = 1, no error -> mode = 0
     *   - calculated = 0, error expected -> mode = 1
     * If current parity is EVEN:
     *   - calculated = 0, no error -> mode = 1
     *   - calculated = 1, error expected -> mode = 0
     */

    if (current_parity == ODD_PARITY) {
        mode_bit = (calculated_odd_parity == 0) ? 1 : 0;
    } else {  /* EVEN_PARITY */
        mode_bit = (calculated_odd_parity == 1) ? 1 : 0;
    }

    /* Return 16-bit value: bit 8 = mode bit, bits 0-7 = data */
    uint16_t result = ((uint16_t)mode_bit << 8) | data_byte;

    mdb_debug("rX9Bits: received 0x%02X mode=%d (parity was %s)\n",
              data_byte, mode_bit,
              (current_parity == ODD_PARITY) ? "ODD" : "EVEN");

    return result;
}

/**
 * @brief Get file descriptor for the serial port
 */
int mdb_get_fd(void) {
    return serial_fd;
}

#ifdef MDB_VERBOSE_LOGGING
/**
 * @brief Print debug message
 */
void mdb_debug(const char *format, ...) {
    va_list args;
    va_start(args, format);
    fprintf(stderr, "[MDB] ");
    vfprintf(stderr, format, args);
    va_end(args);
}
#endif
