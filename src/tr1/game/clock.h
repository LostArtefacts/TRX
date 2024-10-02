#pragma once

#include <stdbool.h>
#include <stdint.h>

#define CLOCK_TURBO_SPEED_MIN -2
#define CLOCK_TURBO_SPEED_MAX 2

typedef struct {
    double prev_counter;
    int32_t prev_fps;
} CLOCK_TIMER;

void Clock_Init(void);

int32_t Clock_SyncTicks(void);

int32_t Clock_GetTurboSpeed(void);
void Clock_CycleTurboSpeed(bool forward);
void Clock_SetTurboSpeed(int32_t value);
double Clock_GetSpeedMultiplier(void);

// High-precision number of milliseconds since the launch of the application
double Clock_GetHighPrecisionCounter(void);
int32_t Clock_GetLogicalFrame(void);
int32_t Clock_GetDrawFrame(void);

void Clock_ResetTimer(CLOCK_TIMER *timer);

// Check how many logical frames passed since the last run of this function.
// The `timer` argument is used to store the previous frame count and should
// not be reused between contexts this function is called with.
double Clock_GetElapsedLogicalFrames(CLOCK_TIMER *timer);
double Clock_GetElapsedDrawFrames(CLOCK_TIMER *timer);
double Clock_GetElapsedMilliseconds(CLOCK_TIMER *timer);

// Similar logic to Clock_GetElapsedLogicalFrames, except reset the timer only
// if there are enough elapsed frames.
bool Clock_CheckElapsedLogicalFrames(CLOCK_TIMER *timer, int32_t wait);
bool Clock_CheckElapsedDrawFrames(CLOCK_TIMER *timer, int32_t wait);
bool Clock_CheckElapsedMilliseconds(CLOCK_TIMER *timer, int32_t wait);

// The same as Clock_CheckElapsedMilliseconds, except does not scale the result
// by the turbo cheat multiplier.
bool Clock_CheckElapsedRawMilliseconds(CLOCK_TIMER *timer, int32_t how_often);

void Clock_GetDateTime(char *date_time);

int32_t Clock_GetFrameAdvance(void);
