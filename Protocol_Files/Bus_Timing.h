#ifndef BUS_TIMING_HEADER
#define BUS_TIMING_HEADER

#include <stdint.h>
#include "PreProcessors.h"
#include "MDB_Linux.h"

extern unsigned long time_elapsed;
extern uint8_t chronoLogic(uint8_t command);    /*Stopwatch that updates all timing flags*/

#endif  /*BUS_TIMING_HEADER*/
