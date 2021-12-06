#ifndef T1M_SPECIFIC_S_CLOCK_H
#define T1M_SPECIFIC_S_CLOCK_H

#include <stdint.h>
#include <stdbool.h>

bool S_Clock_Init();
int32_t S_Clock_GetMS();
int32_t S_Clock_Sync();
int32_t S_Clock_SyncTicks(int32_t target);

#endif
