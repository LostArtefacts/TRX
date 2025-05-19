#pragma once

#include "clock/timer.h"

#include <stdint.h>

#define FADER_ANY (-1)
#define FADER_TRANSPARENT 0
#define FADER_SEMI_BLACK 127
#define FADER_ALMOST_BLACK 192
#define FADER_BLACK 255

typedef struct {
    int32_t initial;
    int32_t target;

    // This value controls how much to keep the last frame after the animation
    // is done (1.0 = one second).
    double debuff;
    double duration;
} FADER_ARGS;

typedef struct {
    FADER_ARGS args;
    CLOCK_TIMER timer;
    bool target_drawn;
} FADER;

void Fader_InitEx(FADER *fader, FADER_ARGS args);
void Fader_Init(FADER *fader, int32_t initial, int32_t target, double duration);
bool Fader_IsActive(const FADER *fader);
int32_t Fader_GetCurrentValue(const FADER *fader);
void Fader_Draw(FADER *fader);
