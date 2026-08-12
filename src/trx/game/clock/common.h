#pragma once

#include <stddef.h>
#include <stdint.h>

// Disables any kind of waiting in Clock_WaitTick
void Clock_DisableWait(void);

// Restores the waiting Clock_DisableWait turned off.
void Clock_EnableWait(void);

// Counts time in frames: every Clock_WaitTick moves the clock on by 1/fps. Zero
// goes back to real time, carrying on from where the frame count reached.
void Clock_EnableFixedFPS(int32_t fps);

void Clock_SyncTick(void);
int32_t Clock_WaitTick(void);

size_t Clock_GetDateTime(char *buffer, size_t size);

int32_t Clock_GetFrameAdvance(void);
int32_t Clock_GetCurrentFPS(void);

void Clock_SetSimSpeed(double new_speed);
double Clock_GetRealTime(void);
double Clock_GetSimTime(void);
