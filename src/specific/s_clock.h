#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <windows.h>

bool S_Clock_Init();
int32_t S_Clock_GetMS();
int32_t S_Clock_Sync();
int32_t S_Clock_SyncTicks(int32_t target);
