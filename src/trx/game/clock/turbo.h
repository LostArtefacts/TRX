#pragma once

#include <stdint.h>

#define CLOCK_TURBO_SPEED_MIN -2
#define CLOCK_TURBO_SPEED_MAX 2

void Clock_CycleTurboSpeed(bool forward);

int32_t Clock_GetTurboSpeed(void);
void Clock_SetTurboSpeed(int32_t value);

// Runs the game at the given speed for as long as the player keeps the key
// down, and puts their own speed back when they let go. Calling either one
// twice over does nothing, so both can be driven straight from the key state.
// What is held is never written to the settings file.
void Clock_HoldTurboSpeed(int32_t value);
void Clock_ReleaseTurboSpeed(void);

double Clock_GetSpeedMultiplier(void);
