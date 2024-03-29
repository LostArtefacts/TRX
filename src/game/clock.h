#pragma once

#include <stdbool.h>
#include <stdint.h>

void Clock_Init(void);

int32_t Clock_SyncTicks(void);

void Clock_CycleTurboSpeed(bool forward);
void Clock_SetTurboSpeed(const int32_t idx);
int32_t Clock_GetTurboSpeed(void);
double Clock_GetSpeedMultiplier(void);

int32_t Clock_GetMS(void);
void Clock_GetDateTime(char *date_time);

int32_t Clock_GetFrameAdvance(void);
double Clock_GetFrameAdvanceAdjusted(void);
