#pragma once

#include <stdbool.h>
#include <stdint.h>

bool Clock_Init();
int32_t Clock_GetMS();
int32_t Clock_Sync();
int32_t Clock_SyncTicks(int32_t target);
int32_t Clock_GetYear();
int32_t Clock_GetMonth();
int32_t Clock_GetDay();
int32_t Clock_GetHours();
int32_t Clock_GetMinutes();
int32_t Clock_GetSeconds();
char *Clock_GetDateTime(char *date_time);