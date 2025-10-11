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

typedef enum {
    UI_BAR_LARA_HP,
    UI_BAR_LARA_AIR,
    UI_BAR_LARA_STAMINA,
    UI_BAR_LARA_EXPOSURE,
    UI_BAR_ENEMY_HP,
    UI_BAR_ALLY_HP,
    UI_BAR_PROGRESS,
    UI_BAR_NUMBER_OF,
} UI_BAR_TYPE;

typedef struct {
    UI_BAR_TYPE type;
    int32_t w;
    int32_t h;
    int32_t value;
    int32_t max_value;
} UI_BAR_SETTINGS;

// draw functions
void UI_Bar(UI_BAR_SETTINGS settings);
