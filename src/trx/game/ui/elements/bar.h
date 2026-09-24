#pragma once

#include <trx/config/types.h>
#include <trx/game/ui/common.h>
#include <trx/game/ui/settings.h>
#include <trx/version.h>

// shared properties of common ingame bars
#define UI_BAR_WIDTH 208.0f
#define UI_BAR_HEIGHT 18.0f
#define UI_BAR_BORDER 2.0f
#define UI_BAR_PADDING 2.0f
#define UI_BAR_BLINK_THRESHOLD (g_TRVersion == 1 ? 0.2f : 0.25f)

typedef struct {
    UI_BAR_TYPE type;
    float w;
    float h;
    int32_t value;
    int32_t max_value;
    bool preview;

    // Draws a plain frame in this color and lets the fill reach it, instead
    // of using the frame and padding from the bar appearance. A fully
    // transparent color keeps the appearance frame.
    RGBA_8888 border_color;

    // Thickness of the plain frame, in the same units as w and h. Values below
    // one unit are raised to it.
    float border_width;

    // Blends between the ramp colors even when the smooth bars setting is
    // turned off.
    bool force_smooth;

    // Takes w and h as final canvas units, rather than as sizes the bars scale
    // and the appearance still have to be applied to.
    bool absolute_size;
} UI_BAR_SETTINGS;

// draw functions
void UI_Bar(UI_BAR_SETTINGS settings);
