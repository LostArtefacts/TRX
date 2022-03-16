#pragma once

#include <stdint.h>
#include <stdbool.h>

bool S_Clock_Init(void);
int32_t S_Clock_GetMS(void);
int32_t S_Clock_Sync(void);
int32_t S_Clock_SyncTicks(int32_t target);
