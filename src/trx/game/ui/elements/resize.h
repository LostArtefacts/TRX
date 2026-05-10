#pragma once

#include <trx/game/ui/common.h>

// Resize child widget to a specified size in pixels.
// A negative size means to use the child's size.
// A zero value means to hide the child, but participate in the layout pass.

typedef struct {
    float w;
    float h;
    // 0.0 aligns to the start, 0.5 centers, 1.0 aligns to the end.
    float align_h;
    float align_v;
} UI_RESIZE_SETTINGS;

void UI_BeginResize(float w, float h);
void UI_BeginResizeEx(UI_RESIZE_SETTINGS settings);
void UI_EndResize(void);
