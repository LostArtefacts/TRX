#pragma once

#include <stddef.h>
#include <stdint.h>

void Clock_Init(void);

// Disables any kind of waiting in Clock_WaitTick
void Clock_DisableWait(void);

void Clock_SyncTick(void);
int32_t Clock_WaitTick(void);

size_t Clock_GetDateTime(char *buffer, size_t size);

int32_t Clock_GetFrameAdvance(void);
extern int32_t Clock_GetCurrentFPS(void);

void Clock_SetSimSpeed(double new_speed);
double Clock_GetRealTime(void);
double Clock_GetSimTime(void);
