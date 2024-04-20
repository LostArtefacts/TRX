#pragma once

#include <stdbool.h>
#include <stdint.h>

void Clock_Init(void);

int32_t Clock_SyncTicks(void);

void Clock_SetTickProgress(double progress);
double Clock_GetTickProgress(void);

int32_t Clock_GetTurboSpeed(void);
void Clock_CycleTurboSpeed(bool forward);
void Clock_SetTurboSpeed(int32_t value);
double Clock_GetSpeedMultiplier(void);

int32_t Clock_GetMS(void);
int32_t Clock_GetLogicalFrame(void);
int32_t Clock_GetDrawFrame(void);
bool Clock_IsAtLogicalFrame(int32_t how_often);
void Clock_GetDateTime(char *date_time);

int32_t Clock_GetFrameAdvance(void);
double Clock_GetFrameAdvanceAdjusted(void);
