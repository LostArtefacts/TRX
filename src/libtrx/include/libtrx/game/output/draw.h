#pragma once

#include "../../config/types.h"

#include <stdint.h>

typedef enum {
    TS_HEADING,
    TS_BACKGROUND,
    TS_BACKGROUND_HEAVY,
    TS_REQUESTED,
} TEXT_STYLE;

extern void Output_DrawTextBackground(
    UI_STYLE ui_style, int32_t sx, int32_t sy, int32_t w, int32_t h, int32_t z,
    TEXT_STYLE text_style);
extern void Output_DrawTextOutline(
    UI_STYLE ui_style, int32_t sx, int32_t sy, int32_t w, int32_t h, int32_t z,
    TEXT_STYLE text_style);
extern void Output_DrawScreenSprite(
    int32_t sx, int32_t sy, int32_t sz, int32_t scale_h, int32_t scale_v,
    int32_t sprite_idx, int16_t shade);
