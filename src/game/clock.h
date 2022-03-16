#pragma once

#include <stdbool.h>
#include <stdint.h>

bool Clock_Init(void);
int32_t Clock_GetMS(void);
int32_t Clock_Sync(void);
int32_t Clock_SyncTicks(int32_t target);
void Clock_GetDateTime(char *date_time);
