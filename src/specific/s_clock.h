#pragma once

#include <stdbool.h>
#include <stdint.h>

bool S_Clock_Init(void);
int32_t S_Clock_GetMS(void);
int32_t S_Clock_SyncTicks(void);
