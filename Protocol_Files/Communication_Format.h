#ifndef COMMUNICATION_FORMAT_HEADER
#define COMMUNICATION_FORMAT_HEADER

#include <stdint.h>
#include "PreProcessors.h"
#include "Bus_Timing.h"
#include "MDB_Linux.h"

extern uint8_t chronoLogic(uint8_t command);

extern unsigned int counter;
extern uint8_t checksum, pointer;
extern uint8_t rX(uint8_t address);                        /*Receives a byte, judges if it needs to receive the rest of the data, and ends at appropriate time. Returns status codes.*/
extern void clearBlock(void);                              /*Clear all communications related variables.*/
extern uint8_t tX(unsigned int pointer, uint8_t address);  /*Send the data in the block, calculate and insert check byte into location of pointer, unless pointer is zero. Always sets mode bit on last byte.*/

/* 9-bit data structure */
extern union nineBit {                 /*Union allows writing mode and data bits simultaneously as a short*/
        struct {                       /*Structure allows accessing 8 data bits and 1 mode bit separately*/
                uint8_t data : 8;      /*8 data bits*/
                uint8_t mode : 1;      /*1 mode bit*/
        } part;
        uint16_t whole;
} block[35];                           /*Array of 9 bit data, MDB has a maximum block size of 36 "bytes"*/

#endif
