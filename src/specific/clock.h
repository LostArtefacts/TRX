#ifndef T1M_SPECIFIC_CLOCK_H
#define T1M_SPECIFIC_CLOCK_H

#include <stdint.h>

int32_t ClockSyncTicks(int32_t target);
void ClockUpdateTicks();
int8_t ClockInit();
int32_t ClockSync();
int32_t ClockGetMS();

#endif
