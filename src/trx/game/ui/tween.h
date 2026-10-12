#pragma once

#include <stdint.h>

// The curves a tween moves along. Each maps how far a move has gone in time,
// from 0 to 1, to how far the value has gone.
typedef enum {
    UI_EASE_LINEAR,
    UI_EASE_QUAD_IN,
    UI_EASE_QUAD_OUT,
    UI_EASE_QUAD_IN_OUT,
} UI_EASE;

// A number that moves to a target over some ticks. Reading it while a frame is
// drawn gives the value part way between two ticks, so what it moves glides at
// any frame rate.
typedef struct {
    float from;
    float to;
    int32_t start;
    int32_t ticks;
    UI_EASE ease;
} UI_TWEEN;

float UI_Ease_Apply(UI_EASE ease, float ratio);

// Puts the value at once where it is told, and stops any move.
void UI_Tween_Set(UI_TWEEN *tween, float value);

// Starts moving to a target from wherever the value is now, after waiting
// `delay` ticks. The value holds still while it waits.
void UI_Tween_To(
    UI_TWEEN *tween, float target, int32_t ticks, int32_t delay, UI_EASE ease);

float UI_Tween_Get(const UI_TWEEN *tween);
float UI_Tween_GetTarget(const UI_TWEEN *tween);

// Reports whether the value is still on its way, counted in whole ticks. A
// value that waits to start is on its way.
bool UI_Tween_IsMoving(const UI_TWEEN *tween);
