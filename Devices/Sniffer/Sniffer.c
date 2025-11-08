/**
 * @file Sniffer.c
 * @brief MDB Bus Sniffer Implementation
 *
 * Passive monitoring device for MDB protocol analysis.
 * The sniffer listens to all bus traffic without participating
 * in the protocol.
 */

#include "Sniffer.h"
#include "PreProcessors.h"
#include <stdio.h>

/**
 * @brief Sniff MDB bus for data
 *
 * Passively monitors the bus and receives data when available.
 * Unlike normal devices, the sniffer doesn't respond to commands.
 *
 * @return Status code from rX() or NO_RESPONSE
 */
uint8_t sniff(void) {
    uint8_t result;

    /* Check if data is available */
    if (rxBitsAvailable()) {
        /*
         * Receive data using SNIFFER_ADDRESS (special address).
         * The sniffer sees all traffic regardless of address,
         * because rX() returns IGNORED for non-matching addresses
         * but still stores the data in block[] for analysis.
         */
        result = rX(SNIFFER_ADDRESS);

        /* Return result for parsing */
        return result;
    }

    return NO_RESPONSE;
}

/**
 * @brief Parse received MDB block
 *
 * Analyzes the received data and logs/processes it.
 * This is where you would add custom analysis logic.
 *
 * @param address Device address (ignored, sniffer sees all)
 * @return OKKK if parsed successfully
 */
uint8_t parse(uint8_t address) {
    (void)address;  /* Unused - sniffer monitors all addresses */

    /*
     * At this point, the data is in the global block[] array.
     * block[0] contains the command/address byte
     * block[1...n] contain the data bytes
     *
     * Example parsing logic could go here:
     * - Decode command type
     * - Log transaction
     * - Analyze timing
     * - Track device state
     */

    /* For now, just indicate successful parse */
    return OKKK;
}
