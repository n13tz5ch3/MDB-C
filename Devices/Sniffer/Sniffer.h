#ifndef SNIFFER_HEADER
#define SNIFFER_HEADER

#include <stdint.h>
#include "Communication_Format.h"
#include "MDB_Linux.h"

/**
 * @brief Check MDB bus for data and receive full block
 *
 * This function monitors the MDB bus for incoming data and receives
 * a complete block when data is available.
 *
 * @return Status code from rX() or NO_RESPONSE if no data
 */
extern uint8_t sniff(void);

/**
 * @brief Parse received MDB data for analysis
 *
 * Decodes and analyzes received MDB block data.
 *
 * @param address Device address to parse data for
 * @return Parse status code
 */
extern uint8_t parse(uint8_t address);

#endif /*SNIFFER_HEADER*/

