#pragma once

#include "../../config/types.h"

#include <stdint.h>

typedef enum {
    TS_HEADING = 0,
    TS_BACKGROUND = 1,
    TS_REQUESTED = 2,
} TEXT_STYLE;

extern void Output_DrawTextBackground(
    UI_STYLE ui_style, int32_t sx, int32_t sy, int32_t w, int32_t h, int32_t z,
    TEXT_STYLE text_style);
extern void Output_DrawTextOutline(
    UI_STYLE ui_style, int32_t sx, int32_t sy, int32_t w, int32_t h, int32_t z,
    TEXT_STYLE text_style);
