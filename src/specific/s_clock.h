#pragma once

#include <stdint.h>
#include <stdbool.h>

bool S_Clock_Init();
int32_t S_Clock_GetMS();
int32_t S_Clock_Sync();
int32_t S_Clock_SyncTicks(int32_t target);
struct tm *S_Clock_GetLocalTime();
int32_t S_Clock_GetYear();
int32_t S_Clock_GetMonth();
int32_t S_Clock_GetDay();
int32_t S_Clock_GetHours();
int32_t S_Clock_GetMinutes();
int32_t S_Clock_GetSeconds();