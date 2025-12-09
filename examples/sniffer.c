/**
 * @file sniffer.c
 * @brief MDB Bus Sniffer - Monitor and decode MDB protocol traffic
 *
 * This tool passively monitors the MDB bus and decodes all communication
 * between the VMC and peripherals.
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
#include <signal.h>
#include <unistd.h>
#include <getopt.h>
#include "MDB_Linux.h"
#include "Communication_Format.h"
#include "PreProcessors.h"

/* Global flag for graceful shutdown */
static volatile int running = 1;

/**
 * @brief Signal handler for graceful shutdown
 */
void signal_handler(int signum) {
    (void)signum;
    printf("\nShutting down...\n");
    running = 0;
}

/**
 * @brief Print usage information
 */
void print_usage(const char *program_name) {
    printf("Usage: %s [options]\n", program_name);
    printf("\nOptions:\n");
    printf("  -d, --device PATH    Serial device path (default: /dev/ttyUSB0)\n");
    printf("  -v, --verbose        Enable verbose logging\n");
    printf("  -h, --help           Show this help message\n");
    printf("\nExample:\n");
    printf("  %s --device /dev/ttyUSB0 --verbose\n", program_name);
}

/**
 * @brief Decode and print device address
 */
const char* decode_address(uint8_t addr) {
    switch(addr & 0xF8) {  /* Mask to get device address */
        case VMC_ADDRESS: return "VMC";
        case CHANGER_ADDRESS: return "Changer";
        case CASHLESS1_ADDRESS: return "Cashless1";
        case COMMUNICATIONS_GATEWAY_ADDRESS: return "CommGW";
        case BILL_VALIDATOR_ADDRESS: return "BillVal";
        case CASHLESS2_ADDRESS: return "Cashless2";
        case AGE_VERIFICATION_DEVICE_ADDRESS: return "AgeVerify";
        default: return "Unknown";
    }
}

/**
 * @brief Print MDB block in hex format
 */
void print_block(union nineBit *blk, uint8_t len) {
    printf("  Data: ");
    for (uint8_t i = 0; i < len; i++) {
        printf("%02X ", blk[i].part.data);
        if (blk[i].part.mode) {
            printf("[M] ");
        }
    }
    printf("\n");
}

/**
 * @brief Main function
 */
int main(int argc, char *argv[]) {
    const char *device_path = MDB_TTY_DEVICE;
    int verbose = 0;

    /* Parse command line arguments */
    struct option long_options[] = {
        {"device", required_argument, 0, 'd'},
        {"verbose", no_argument, 0, 'v'},
        {"help", no_argument, 0, 'h'},
        {0, 0, 0, 0}
    };

    int opt;
    while ((opt = getopt_long(argc, argv, "d:vh", long_options, NULL)) != -1) {
        switch (opt) {
            case 'd':
                device_path = optarg;
                break;
            case 'v':
                verbose = 1;
                break;
            case 'h':
                print_usage(argv[0]);
                return 0;
            default:
                print_usage(argv[0]);
                return 1;
        }
    }

    /* Setup signal handlers */
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    printf("MDB Bus Sniffer\n");
    printf("===============\n");
    printf("Device: %s\n", device_path);
    printf("Verbose: %s\n", verbose ? "Yes" : "No");
    printf("\nPress Ctrl+C to stop...\n\n");

    /* Initialize MDB interface */
    if (mdb_init(device_path) != 0) {
        fprintf(stderr, "ERROR: Failed to initialize MDB interface\n");
        fprintf(stderr, "Make sure:\n");
        fprintf(stderr, "  1. Device %s exists\n", device_path);
        fprintf(stderr, "  2. You have permission to access it (add user to 'dialout' group)\n");
        fprintf(stderr, "  3. No other program is using the serial port\n");
        return 1;
    }

    printf("MDB interface initialized successfully\n");
    printf("Monitoring bus...\n\n");

    /* Monitor loop */
    while (running) {
        /* Check if data is available */
        if (rxBitsAvailable()) {
            /* Receive 9-bit data */
            uint16_t data = rX9Bits();

            if (data != 0xFFFF) {  /* Valid data received */
                uint8_t byte = data & 0xFF;
                uint8_t mode = (data >> 8) & 0x01;

                /* Print timestamp */
                unsigned long timestamp = micros();
                printf("[%010lu us] ", timestamp);

                /* Check if this is an address/command byte */
                if (mode) {
                    printf(">>> [ADDR] %s (0x%02X)\n", decode_address(byte), byte);
                } else {
                    /* Data byte or response code */
                    switch (byte) {
                        case ACK:
                            printf("<<< [ACK]\n");
                            break;
                        case NAK:
                            printf("<<< [NAK]\n");
                            break;
                        case RET:
                            printf("<<< [RET]\n");
                            break;
                        default:
                            printf("    [DATA] 0x%02X (%d)\n", byte, byte);
                            break;
                    }
                }

                fflush(stdout);
            }
        }

        /* Small delay to avoid busy-waiting */
        usleep(100);  /* 100 microseconds */
    }

    /* Cleanup */
    printf("\nClosing MDB interface...\n");
    mdb_close();

    printf("Done.\n");
    return 0;
}
