#pragma once

#include <libtrx/config/types.h>
#include <libtrx/game/ui/common.h>

// shared properties of common ingame bars
#define UI_BAR_WIDTH 208
#define UI_BAR_HEIGHT 18
#define UI_BAR_BLINK_THRESHOLD 0.2f

typedef struct {
    BAR_COLOR color;
    int32_t w;
    int32_t h;
    int32_t value;
    int32_t max_value;
} UI_BAR_SETTINGS;

// draw functions
void UI_Bar(UI_BAR_SETTINGS settings);
