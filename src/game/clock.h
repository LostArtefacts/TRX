#pragma once

#include <stdbool.h>
#include <stdint.h>

bool Clock_Init();
int32_t Clock_GetMS();
int32_t Clock_Sync();
int32_t Clock_SyncTicks(int32_t target);
