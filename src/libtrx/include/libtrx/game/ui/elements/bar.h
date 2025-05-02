#pragma once

#include "../../../config/types.h"
#include "../common.h"

// shared properties of common ingame bars
#define UI_BAR_WIDTH 208.0f
#define UI_BAR_HEIGHT 18.0f
#define UI_BAR_BORDER 2.0f
#define UI_BAR_PADDING 2.0f
#if TR_VERSION == 1
    #define UI_BAR_BLINK_THRESHOLD 0.2f
#else
    #define UI_BAR_BLINK_THRESHOLD 0.25f
#endif

typedef struct {
    BAR_COLOR color;
    int32_t w;
    int32_t h;
    int32_t value;
    int32_t max_value;
} UI_BAR_SETTINGS;

// draw functions
void UI_Bar(UI_BAR_SETTINGS settings);
